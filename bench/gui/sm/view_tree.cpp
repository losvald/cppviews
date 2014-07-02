#include "view_tree.hpp"

#include "../smvd_display.hpp"
#include "../../util/rapidjson/document.h"
#include "../../util/rapidjson/filereadstream.h"
#include "../../util/rapidjson/prettywriter.h"

#include "wx/log.h"

#include <ostream>
#include <stack>

#include <cstdio>

namespace {

class JsonOutputStream {
 public:
  JsonOutputStream(std::ostream& os) : os_(os) {}
  void Put (char ch) { os_.put(ch); }
  void Flush() {}
 private:
  std::ostream& os_;
};

class JsonPrettyWriter : public rapidjson::PrettyWriter<JsonOutputStream> {
 public:
  JsonPrettyWriter(JsonOutputStream& jos)
      : rapidjson::PrettyWriter<JsonOutputStream>(jos) {
    SetIndent(' ', 2);
  }
};

typedef rapidjson::GenericValue<rapidjson::Document::EncodingType,
                                rapidjson::Document::AllocatorType> JsonValue;

class ViewInfoBase : public ViewTree::ItemDataBase {
 public:
  static ViewInfoBase* Create(ViewType type,
                              ViewTree::ItemDataBase::Direction direction);

  void WriteConfig(std::ostream* os) const override {
    JsonOutputStream jos(*os);
    JsonPrettyWriter pw(jos);
    pw.StartObject();
    WriteConfig(pw);
    pw.EndObject();
  }

  virtual void WriteConfig(JsonPrettyWriter& writer) const {
#define SMVD_THIS_COORD(which, suffix, c)       \
    String(#which suffix).Int(which().c)
#define SMVD_THIS_POINT(which)                                          \
    SMVD_THIS_COORD(which, "_row", y).SMVD_THIS_COORD(which, "_col", x)

    writer
        .SMVD_THIS_POINT(first)
        .SMVD_THIS_POINT(last);
#undef SMVD_THIS_POINT
#undef SMVD_THIS_COORD
  }

  virtual void ReadConfig(const JsonValue& val) {
#define SMVD_THIS_COORD(name) val[name].GetInt()
#define SMVD_THIS_POINT(which)                      \
    wxPoint(SMVD_THIS_COORD(#which "_col"),      \
            SMVD_THIS_COORD(#which "_row"))

    Init(SMVD_THIS_POINT(first), SMVD_THIS_POINT(last));
#undef SMVD_THIS_POINT
#undef SMVD_THIS_COORD
  }

 protected:
  ViewInfoBase(ViewType type) : ViewTree::ItemDataBase(type) {}
};

template<unsigned char view_type>
class ViewInfo : public ViewInfoBase {
 public:
  ViewInfo() : ViewInfoBase(static_cast<ViewType>(view_type)) {}
};

struct State : public wxObject {
  State(Display* display, ViewTree* tree) : display(*display), tree(*tree) {}
  State(const State&) = delete;
  static State& Get(const wxEvent& evt) {
    return *static_cast<State*>(evt.GetEventUserData());
  }

  Display& display;
  ViewTree& tree;
  wxPoint shrink_first = wxDefaultPosition;
};

#define SMVD_VIEW_INFO_BIND(event_type, method, display, tree)          \
  display->Bind(event_type, &ViewInfo::method, this, wxID_ANY, wxID_ANY, \
                new State(display, tree))
#define SMVD_VIEW_INFO_UNBIND(event_type, method, display)      \
  display->Unbind(event_type, &ViewInfo::method, this)

template<>
class ViewInfo<kViewTypeChain> : public ViewInfoBase {
 public:
  static constexpr ViewType kType = kViewTypeChain;
  typedef ViewParams<kType> ParamsType;

  ViewInfo() : ViewInfoBase(kType) {}
  void BindDisplayEvents(Display* display, ViewTree* tree) override {
    wxLogVerbose("Binding chain display events");
    SMVD_VIEW_INFO_BIND(wxEVT_LEFT_DOWN, OnDisplayLeftDown, display, tree);
    SMVD_VIEW_INFO_BIND(wxEVT_LEFT_DCLICK, OnDisplayLeftDClick, display, tree);
  }

  void UnbindDisplayEvents(Display* display) override {
    wxLogVerbose("Unbinding chain display events");
    SMVD_VIEW_INFO_UNBIND(wxEVT_LEFT_DOWN, OnDisplayLeftDown, display);
    SMVD_VIEW_INFO_UNBIND(wxEVT_LEFT_DCLICK, OnDisplayLeftDClick, display);
  }

 private:
  void OnDisplayLeftDown(wxMouseEvent& evt) {
    auto& s = State::Get(evt);
    wxPoint pos = s.display.DeviceToIndices(evt.GetPosition());
    wxLogVerbose("CHAIN left down @ (%d %d)", pos.x, pos.y);

    // automatically reset the shrink state unless MarkFirst() is called
    struct Shrinker {
      Shrinker(State& s) : s_(s) {}
      void Mark(const wxPoint& first) { first_ = first; }
      bool marked() const { return s_.shrink_first != wxDefaultPosition; }
      ~Shrinker() { s_.shrink_first = first_;
        wxLogVerbose("Reset shrinker to: %d %d", first_.x, first_.y);
      }
     private:
      wxPoint first_ = wxDefaultPosition;
      State& s_;
    } shrinker(s);

    ViewTree& tree = s.tree;
    wxASSERT(tree.GetSelection() == GetId());
    if (!Contains(pos)) {
      wxLogVerbose("Clicked outside, doing nothing!");
      return;
    }

    const auto& prev_id = tree.GetLastChild(GetId());
    wxPoint first;
    if (prev_id.IsOk()) {
      auto& prev_info = tree.GetViewInfo(prev_id);
      if (!Ahead(pos, prev_info)) {
        wxLogVerbose("shrink_first=%d", s.shrink_first.y);
        if (Within(pos, prev_info)) {
          if (prev_info.Contains(pos)) {
            if (!shrinker.marked()) {
              shrinker.Mark(pos);
              wxLogVerbose("Shrink from: %d", pos.y);
            } else {
              wxLogVerbose("Shrink to: %d", pos.y);
              wxPoint first = Translate(s.shrink_first, prev_info.first());
              wxPoint last = Translate(pos, prev_info.last());
              ExcludeSideward(first, &last);
              prev_info.Init(first, last);
              wxLogVerbose("Shrinking: [%d %d] (%d %d)",
                           prev_info.top_left().y, prev_info.top_left().x,
                           prev_info.bottom_right().y,
                           prev_info.bottom_right().x);
              s.display.Refresh();
            }
          } else {
            ExcludeSideward(prev_info.first(), &pos);
            Extend(pos, &prev_info);
            wxLogVerbose("Unshrink");
            s.display.Refresh();
          }
        }
        wxLogVerbose("Not clicked ahead, not creating a view.");
        return;
      }
      first = prev_info.last();
      Translate(1, &first);
      first = Translate(top_left(), first);
    } else {
      first = this->top_left();
    }

    wxPoint last = Translate(bottom_right(), pos);
    wxPoint delta = last - first;
    if (delta.x == 0 || delta.y == 0) {
      wxLogVerbose("Not creating view with a zero dimension");
      return;
    }

    Translate(-1, &last);
    static int gen_id = 0;
    wxString text = wxString::Format("_new%02d", gen_id++);
    auto info = new ViewInfo<kViewTypeChain>;
    info->Init(first, last);
    info->set_direction(!dir_);
    tree.AppendItem(GetId(), text, -1, -1, info);
    s.display.Refresh();
  }

  void OnDisplayLeftDClick(wxMouseEvent& evt) {
    auto& s = State::Get(evt);
    wxPoint pos = s.display.DeviceToIndices(evt.GetPosition());
    wxLogVerbose("CHAIN left dclick @ (%d %d)", pos.x, pos.y);

    // discard the shrink mark set in the down event (which precedes this one)
    s.shrink_first = wxDefaultPosition;

    ViewTree& tree = s.tree;
    wxASSERT(tree.GetSelection() == GetId());
    if (!Contains(pos)) {
      wxLogVerbose("Dclicked outside, doing nothing!");
      return;
    }

    const auto& prev_id = tree.GetLastChild(GetId());
    if (prev_id.IsOk()) {
      auto& prev_info = tree.GetViewInfo(prev_id);
      if (Ahead(pos, prev_info)) {
        // extending the only child doesn't make much sense
        if (tree.GetChildrenCount(GetId()) != 1) {
          // a single-click event always precedes, so we just extend the last
          Extend(Translate(top_left(), bottom_right()), &prev_info);
          wxLogVerbose("Last");
          s.display.Refresh();
        }
      } else if (Within(pos, prev_info)) {
        if (!prev_info.Contains(pos)) {
          wxPoint delta = pos - prev_info.bottom_right();
          if (delta.y > 0 || delta.x > 0) { // both can't be >0 if down/right
            prev_info.Init(prev_info.top_left(),
                           Translate(bottom_right(), prev_info.last()));
          } else {
            prev_info.Init(Translate(top_left(), prev_info.first()),
                           prev_info.bottom_right());
          }
          wxLogVerbose("Extend");
          s.display.Refresh();
        }
        wxLogVerbose("Not dclicked ahead");
      }
    }
  }

 private:
  // Extends a view s.t. it includes the point p
  void Extend(const wxPoint& p, ViewTree::ItemDataBase* info) const {
    const wxPoint& tl = info->top_left();
    const wxPoint& br = info->bottom_right();
    info->Init(wxPoint(std::min(p.x, tl.x), std::min(p.y, tl.y)),
               wxPoint(std::max(p.x, br.x), std::max(p.y, br.y)));
  }

  // Translates a point the specified number of units ahead (could be negative)
  void Translate(int ahead, wxPoint* p) {
    int right = (dir_ == ParamsType::Dir::kRight);
    *p += wxPoint(right * ahead, !right * ahead);
  }

  // Returns the point which is exactly as much ahead as p
  wxPoint Translate(const wxPoint& p, const wxPoint& p_ref) const {
    return dir_ == ParamsType::Dir::kRight
        ? wxPoint(p_ref.x, p.y)
        : wxPoint(p.x, p_ref.y);
  }

  void ExcludeSideward(const wxPoint& first, wxPoint* last) const {
#define SMVD_THIS_EXPR(c) last->c -= (first.c < last->c) - (last->c < first.c)
    if (dir_ == ParamsType::Dir::kRight)
      SMVD_THIS_EXPR(y);
    else
      SMVD_THIS_EXPR(x);
#undef SMVD_THIS_EXPR
  }

  // Checks if p is strictly ahead of info (i.e., forward w.r.t direction)
  bool Ahead(const wxPoint& p, const ViewTree::ItemDataBase& info) const {
#define SMVD_THIS_EXPR(c) p.c > info.last().c
    return dir_ == ParamsType::Dir::kRight
        ? SMVD_THIS_EXPR(x)
        : SMVD_THIS_EXPR(y);
#undef SMVD_THIS_EXPR
  }

  // Checks if the coordinate of p parallel to the direction is within a view
  bool Within(const wxPoint& p, const ViewTree::ItemDataBase& info) const {
#define SMVD_THIS_EXPR(c) info.first().c <= p.c && p.c <= info.last().c
    return dir_ == ParamsType::Dir::kRight
        ? SMVD_THIS_EXPR(x)
        : SMVD_THIS_EXPR(y);
#undef SMVD_THIS_EXPR
  }
};

template<>
class ViewInfo<kViewTypeMono> : public ViewInfoBase {
 public:
  ViewInfo() : ViewInfoBase(kViewTypeMono) {}
  void BindDisplayEvents(Display* display, ViewTree* tree) {
    wxLogVerbose("Binding mono display events");
    SMVD_VIEW_INFO_BIND(wxEVT_LEFT_DOWN, OnDisplayLeftDown, display, tree);
    SMVD_VIEW_INFO_BIND(wxEVT_LEFT_UP, OnDisplayLeftUp, display, tree);
    SMVD_VIEW_INFO_BIND(wxEVT_LEFT_DCLICK, OnDisplayLeftDClick, display, tree);
  }
  void UnbindDisplayEvents(Display* display) {
    wxLogVerbose("Unbinding mono display events");
    SMVD_VIEW_INFO_UNBIND(wxEVT_LEFT_DOWN, OnDisplayLeftDown, display);
    SMVD_VIEW_INFO_UNBIND(wxEVT_LEFT_UP, OnDisplayLeftUp, display);
    SMVD_VIEW_INFO_UNBIND(wxEVT_LEFT_DCLICK, OnDisplayLeftDClick, display);
  }

 private:
  void OnDisplayLeftDown(wxMouseEvent& evt) {
    wxLogVerbose("MONO left down %d", evt.LeftDClick());
  }
  void OnDisplayLeftUp(wxMouseEvent& evt) {
    wxLogVerbose("MONO left up %d", evt.LeftDClick());
  }
  void OnDisplayLeftDClick(wxMouseEvent& evt) {
    wxLogVerbose("MONO left dclick %d", evt.LeftDClick());
  }
};

template<>
class ViewInfo<kViewTypeDiag> : public ViewInfoBase {
 public:
  ViewInfo() : ViewInfoBase(kViewTypeDiag) {}
  void WriteConfig(JsonPrettyWriter& w) const override {
    ViewInfoBase::WriteConfig(w);
    w.String("block_rows").Int(block_dims_.y);
    w.String("block_cols").Int(block_dims_.x);
  }
  void ReadConfig(const JsonValue& val) override {
    ViewInfoBase::ReadConfig(val);
    block_dims_.y = val["block_rows"].GetInt();
    block_dims_.x = val["block_cols"].GetInt();
  }
 private:
  wxSize block_dims_ = wxSize(1, 1);
};

template<>
class ViewInfo<kViewTypeImpl> : public ViewInfoBase {
 public:
  ViewInfo() : ViewInfoBase(kViewTypeImpl) {}
  void WriteConfig(JsonPrettyWriter& w) const override {
    ViewInfoBase::WriteConfig(w);
    w.String("function").String(fun_str_.mb_str(), fun_str_.size());
  }
  void ReadConfig(const JsonValue& val) override {
    ViewInfoBase::ReadConfig(val);
    fun_str_ = val["function"].GetString();
  }

 private:
  wxString fun_str_ = "(int row, int col) {\nreturn 0;\n}";
};

// TODO: specialize for more view types

ViewInfoBase*
ViewInfoBase::Create(ViewType type, ViewTree::ItemDataBase::Direction dir) {
  ViewInfoBase* info;
#define SMVD_SWITCH_CASE(view_type)             \
  case view_type: {                             \
    info = new ViewInfo<view_type>;             \
    break;                                      \
  }
  switch (type) {
    SMVD_SWITCH_CASE(kViewTypeChain);
    SMVD_SWITCH_CASE(kViewTypeMono);
    SMVD_SWITCH_CASE(kViewTypeFull);
    SMVD_SWITCH_CASE(kViewTypeSparse);
    SMVD_SWITCH_CASE(kViewTypeDiag);
    SMVD_SWITCH_CASE(kViewTypeImpl);
    default:
      wxASSERT("Missing switch case");
      return nullptr;
  }
#undef SMVD_SWITCH_CASE
  info->set_direction(dir);
  return info;
}

// use a helper class to avoid dependency on rapidjson in the header
class ViewTreeBuilder {
 public:
  ViewTreeBuilder(ViewTree& tree, JsonValue& json) : tree_(tree) {
    const auto& root_id = tree.GetRootItem();
    tree_.DeleteChildren(root_id);
    tree_.ChangeViewType(root_id, static_cast<ViewType>(json["type"].GetInt()));
    Build(json, static_cast<ViewInfoBase*>(tree_.GetItemData(root_id)));
  }

 private:
  void Build(const JsonValue& val, ViewInfoBase* info) {
    info->ReadConfig(val);
    for (auto m = val.MemberBegin(), m_end = val.MemberEnd(); m != m_end; ++m) {
      const char* name = m->name.GetString();
      if (name[0] == '_') {
        ViewType child_type = GetViewType(m->value);
        auto child_dir = static_cast<ViewTree::ItemDataBase::Direction>(
            m->value["direction"].GetInt());
        auto child_info = ViewInfoBase::Create(child_type, child_dir);
        tree_.AppendItem(info->GetId(), name, -1, -1, child_info);
        Build(m->value, child_info);
      }
    }
  }

  static ViewType GetViewType(const JsonValue& val) {
    return static_cast<ViewType>(val["type"].GetInt());
  }

  ViewTree& tree_;
};

}  // namespace

ViewTree::ViewTree(wxWindow* parent, wxWindowID id, const wxPoint& pos,
                   const wxSize& size, long style)
    : wxTreeCtrl(parent, id, pos, size, style) {
  // establish the invariant that root always exists
  auto root_info = new ViewInfo<kViewTypeSparse>;
  root_info->Init(wxPoint(0, 0), -wxDefaultPosition); // set ranges to [0, 1]
  root_info->set_direction(0);
  AddRoot("root", -1, -1, root_info);
}

void ViewTree::ChangeViewType(const wxTreeItemId& id, ViewType type) {
  const auto& old_info = GetViewInfo(id);
  ItemDataBase* info = ViewInfoBase::Create(type, old_info.direction());
  info->Init(old_info.first(), old_info.last());
  SetItemData(id, info);
  delete &old_info;  // note that SetItemData() doesn't free the old one
}

unsigned ViewTree::GetLevel(const wxTreeItemId& id) const {
  int lvl = 0;
  for (wxTreeItemId p = id; (p = GetItemParent(p)).IsOk(); )
    ++lvl;
  return lvl;
}

void ViewTree::ReadConfig(const char* path) {
  FILE* fin = fopen(path, "r");
  if (fin == nullptr)
    return;

  using namespace rapidjson;
  char buffer[1 << 16];            // 65K buffer
  FileReadStream is(fin, buffer, sizeof(buffer));
  Document doc;
  doc.ParseStream(is);
  ViewTreeBuilder(*this, doc);
}

void ViewTree::WriteConfig(std::ostream* os) const {
  JsonOutputStream jos(*os);
  JsonPrettyWriter pw(jos);

  std::stack<wxTreeItemId> stack;
  std::stack<wxTreeItemId> children;
  const auto& root_id = GetRootItem();
  stack.push(root_id);
  do {
    const auto id = stack.top();
    stack.pop();
    if (!id.IsOk()) {
      pw.EndObject();
      continue;
    }

    if (id != root_id) {  // top-level view doesn't have a name in JSON
      const wxString& name = GetItemText(id);
      pw.String(name.mb_str(), name.length());
    }
    auto info = static_cast<const ViewInfoBase*>(GetItemData(id));
    pw.StartObject()
        .String("type").Int(info->type())
        .String("direction").Int(info->direction());
    info->WriteConfig(pw);

    // push the unset id as a sentinel value for the end of a nesting level
    stack.emplace();
    stack.top().Unset();

    // push the children in reverse order
    wxTreeItemIdValue cookie;
    for (wxTreeItemId child = GetFirstChild(id, cookie);
         child.IsOk(); child = GetNextChild(id, cookie)) {
      children.push(std::move(child));
    }
    for (; !children.empty(); children.pop())
      stack.push(std::move(children.top()));
  } while (!stack.empty());
}

bool ViewTree::ItemDataBase::Contains(const wxPoint& indices) const {
  return wxRect(top_left(), bottom_right()).Contains(indices);
}
