#include "view_tree.hpp"

ViewTree::ViewTree(wxWindow* parent, wxWindowID id, const wxPoint& pos,
                   const wxSize& size, long style)
    : wxTreeCtrl(parent, id, pos, size, style) {
  auto id_root = AddRoot("root", -1, -1, new ItemDataBase(
      kViewTypeChain, wxPoint(0, 0), wxPoint(100, 200)));
  auto id_1 = AppendItem(id_root, "_1", -1, -1, new ItemDataBase(
      kViewTypeChain, wxPoint(0, 0), wxPoint(80, 100)));
  AppendItem(id_root, "_2", -1, -1, new ItemDataBase(
      kViewTypeMono, wxPoint(81, 101), wxPoint(100, 200)));
  AppendItem(id_1, "_1_1", -1, -1, new ItemDataBase(
      kViewTypeMono, wxPoint(5, 10), wxPoint(80, 50)));
  ExpandAll();
}

unsigned ViewTree::GetLevel(const wxTreeItemId& id) const {
  int lvl = 0;
  for (wxTreeItemId p = id; (p = GetItemParent(p)).IsOk(); )
    ++lvl;
  return lvl;
}
