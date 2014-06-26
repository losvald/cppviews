#include "view_tree.hpp"

#include "../smvd_display.hpp"

#include "wx/log.h"

template<unsigned char view_type>
class ViewInfo {};

namespace {

// XXX: for testing/demo only
template<unsigned char view_type>
ViewTree::ItemDataBase* CreateViewInfo(const wxPoint& first = wxDefaultPosition,
                                       const wxPoint& last = wxDefaultPosition) {
  auto vi = new ViewInfo<view_type>;
  vi->Init(first, last);
  return vi;
}

}  // namespace

ViewTree::ViewTree(wxWindow* parent, wxWindowID id, const wxPoint& pos,
                   const wxSize& size, long style)
    : wxTreeCtrl(parent, id, pos, size, style) {
  auto id_root = AddRoot("root", -1, -1, CreateViewInfo<kViewTypeChain>(
      wxPoint(0, 0), wxPoint(100, 200)));
  auto id_1 = AppendItem(id_root, "_1", -1, -1, CreateViewInfo<kViewTypeChain>(
      wxPoint(0, 0), wxPoint(80, 100)));
  AppendItem(id_root, "_2", -1, -1, CreateViewInfo<kViewTypeMono>(
      wxPoint(81, 101), wxPoint(100, 200)));
  AppendItem(id_1, "_1_1", -1, -1, CreateViewInfo<kViewTypeMono>(
      wxPoint(5, 10), wxPoint(80, 50)));
  ExpandAll();
}

unsigned ViewTree::GetLevel(const wxTreeItemId& id) const {
  int lvl = 0;
  for (wxTreeItemId p = id; (p = GetItemParent(p)).IsOk(); )
    ++lvl;
  return lvl;
}

class DisplayRef : public wxObject {
 public:
  DisplayRef(Display* display) : display_(display) {}
  static Display* Get(const wxEvent& evt) {
    return static_cast<DisplayRef*>(evt.GetEventUserData())->display_;
  }

  // template<class EventTag, class ViewInfoType, class EventType>
  // static void Bind(const EventTag& event_type,
  //                  void (ViewInfoType::*method)(EventType&),
  //                  ViewInfoType* view_info,
  //                  Display* display) {
  //   display->Bind(event_type, method, view_info, wxID_ANY, wxID_ANY,
  //                 new DisplayRef(display));
  // }
 private:
  Display* display_;
};

#define SMVD_VIEW_INFO_BIND(event_type, method, display)                \
  display->Bind(event_type, &ViewInfo::method, this, wxID_ANY, wxID_ANY, \
                new DisplayRef(display))
#define SMVD_VIEW_INFO_UNBIND(event_type, method, display)      \
  display->Unbind(event_type, &ViewInfo::method, this)

template<>
class ViewInfo<kViewTypeChain> : public ViewTree::ItemDataBase {
 public:
  ViewInfo() : ViewTree::ItemDataBase(kViewTypeChain) {}
  void BindDisplayEvents(Display* display) {
    wxLogVerbose("Binding chain display events");
    SMVD_VIEW_INFO_BIND(wxEVT_LEFT_DOWN, OnDisplayLeftDown, display);
    SMVD_VIEW_INFO_BIND(wxEVT_LEFT_DCLICK, OnDisplayLeftDClick, display);
  }

  void UnbindDisplayEvents(Display* display) {
    wxLogVerbose("Unbinding chain display events");
    SMVD_VIEW_INFO_UNBIND(wxEVT_LEFT_DOWN, OnDisplayLeftDown, display);
    SMVD_VIEW_INFO_UNBIND(wxEVT_LEFT_DCLICK, OnDisplayLeftDClick, display);
  }

 private:
  void OnDisplayLeftDown(wxMouseEvent& evt) {
    auto display = DisplayRef::Get(evt);
    wxPoint pos = display->DeviceToIndices(evt.GetPosition());
    wxLogVerbose("CHAIN left down @ (%d %d)", pos.x, pos.y);
  }
  void OnDisplayLeftDClick(wxMouseEvent& evt) {
    auto display = DisplayRef::Get(evt);
    wxPoint pos = display->DeviceToIndices(evt.GetPosition());
    wxLogVerbose("CHAIN left dclick @ (%d %d)", pos.x, pos.y);
  }
};

template<>
class ViewInfo<kViewTypeMono> : public ViewTree::ItemDataBase {
 public:
  ViewInfo() : ViewTree::ItemDataBase(kViewTypeMono) {}
  void BindDisplayEvents(Display* display) {
    wxLogVerbose("Binding mono display events");
    SMVD_VIEW_INFO_BIND(wxEVT_LEFT_DOWN, OnDisplayLeftDown, display);
    SMVD_VIEW_INFO_BIND(wxEVT_LEFT_DCLICK, OnDisplayLeftDClick, display);
  }
  void UnbindDisplayEvents(Display* display) {
    wxLogVerbose("Unbinding mono display events");
    SMVD_VIEW_INFO_UNBIND(wxEVT_LEFT_DOWN, OnDisplayLeftDown, display);
    SMVD_VIEW_INFO_UNBIND(wxEVT_LEFT_DCLICK, OnDisplayLeftDClick, display);
  }

 private:
  void OnDisplayLeftDown(wxMouseEvent&) {
    wxLogVerbose("MONO left down");
  }
  void OnDisplayLeftDClick(wxMouseEvent&) {
    wxLogVerbose("MONO left dclick");
  }
};

// TODO: specialize for more view types
