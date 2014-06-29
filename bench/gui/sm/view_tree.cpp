#include "view_tree.hpp"

#include "../smvd_display.hpp"

#include "wx/log.h"

#include <ostream>

namespace {

template<unsigned char view_type>
class ViewInfo : public ViewTree::ItemDataBase {
 public:
  ViewInfo() : ViewTree::ItemDataBase(static_cast<ViewType>(view_type)) {}
};

template<unsigned char view_type>
ViewTree::ItemDataBase* CreateViewInfo(const wxPoint& first = wxDefaultPosition,
                                       const wxPoint& last = wxDefaultPosition,
                                       ViewTree::ItemDataBase::Direction
                                       direction = 0);

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
class ViewInfo<kViewTypeChain> : public ViewTree::ItemDataBase {
 public:
  static constexpr ViewType kType = kViewTypeChain;
  typedef ViewParams<kType> ParamsType;

  ViewInfo() : ViewTree::ItemDataBase(kType) {}
  void BindDisplayEvents(Display* display, ViewTree* tree) {
    wxLogVerbose("Binding chain display events");
    SMVD_VIEW_INFO_BIND(wxEVT_LEFT_DOWN, OnDisplayLeftDown, display, tree);
    SMVD_VIEW_INFO_BIND(wxEVT_LEFT_DCLICK, OnDisplayLeftDClick, display, tree);
  }

  void UnbindDisplayEvents(Display* display) {
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
    tree.AppendItem(GetId(), text, -1, -1, CreateViewInfo<kViewTypeChain>(
        first, last, !dir_));
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
class ViewInfo<kViewTypeMono> : public ViewTree::ItemDataBase {
 public:
  ViewInfo() : ViewTree::ItemDataBase(kViewTypeMono) {}
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

// TODO: specialize for more view types

// XXX: for testing/demo only
template<unsigned char view_type>
ViewTree::ItemDataBase* CreateViewInfo(const wxPoint& first,
                                       const wxPoint& last,
                                       ViewTree::ItemDataBase::Direction
                                       direction) {
  auto vi = new ViewInfo<view_type>;
  vi->Init(first, last);
  vi->set_direction(direction);
  return vi;
}

}  // namespace

ViewTree::ViewTree(wxWindow* parent, wxWindowID id, const wxPoint& pos,
                   const wxSize& size, long style)
    : wxTreeCtrl(parent, id, pos, size, style) {
  // XXX: for testing/demo only
  auto id_root = AddRoot("root", -1, -1, CreateViewInfo<kViewTypeMono>(
      wxPoint(0, 0), wxPoint(100, 200)));
  auto id_1 = AppendItem(id_root, "_1", -1, -1, CreateViewInfo<kViewTypeChain>(
      wxPoint(0, 0), wxPoint(80, 100)));
  AppendItem(id_root, "_2", -1, -1, CreateViewInfo<kViewTypeChain>(
      wxPoint(81, 101), wxPoint(100, 200)));
  AppendItem(id_1, "_1_1", -1, -1, CreateViewInfo<kViewTypeMono>(
      wxPoint(5, 10), wxPoint(80, 50)));

  ExpandAll();
}

void ViewTree::ChangeViewType(const wxTreeItemId& id, ViewType type) {
  ItemDataBase* info;
  const auto& old_info = GetViewInfo(id);
#define SMVD_SWITCH_CASE(view_type)                                     \
  case view_type: {                                                     \
    info = new ViewInfo<view_type>;                                     \
    info->Init(old_info.first(), old_info.last());                      \
    break;                                                              \
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
      return;
  }
#undef SMVD_SWITCH_CASE

  SetItemData(id, info);
}

unsigned ViewTree::GetLevel(const wxTreeItemId& id) const {
  int lvl = 0;
  for (wxTreeItemId p = id; (p = GetItemParent(p)).IsOk(); )
    ++lvl;
  return lvl;
}

void ViewTree::ItemDataBase::WriteConfig(std::ostream* os) const {
  const auto& tl = top_left();
  const auto& br = bottom_right();
  *os << "row_min: " << tl.y << "\n"
      << "row_max: " << br.y << "\n"
      << "col_min: " << tl.x << "\n"
      << "col_max: " << br.x << "\n";
}

bool ViewTree::ItemDataBase::Contains(const wxPoint& indices) const {
  return wxRect(top_left(), bottom_right()).Contains(indices);
}
