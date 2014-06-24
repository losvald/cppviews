#ifndef CPPVIEWS_BENCH_GUI_SM_VIEW_TREE_HPP_
#define CPPVIEWS_BENCH_GUI_SM_VIEW_TREE_HPP_

#include "sm/view_type.hpp"

#include <wx/treectrl.h>

#include <cstdlib>

class ViewTree : public wxTreeCtrl {
 public:
  class ItemDataBase : public wxTreeItemData {
   public:
    inline ItemDataBase(ViewType type, const wxPoint& first,
                        const wxPoint& last)
        : type_(type),
          first_(first),
          last_(last) {}
    inline virtual ~ItemDataBase() = default;
    inline ViewType type() const { return type_; }
    inline int col_count() const { return abs(last_.x - first_.x) + 1; }
    inline int row_count() const { return abs(last_.y - first_.y) + 1; }
    inline int col_offset() const { return first_.x; }
    inline int row_offset() const { return first_.y; }
    inline wxPoint first() const { return first_; }
    inline wxPoint last() const { return last_; }
    inline wxPoint top_left() const {
      return wxPoint(first_.x < last_.x ? first_.x : last_.x,
                     first_.y < last_.y ? first_.y : last_.y);
    }
    inline wxPoint bottom_right() const {
      return wxPoint(first_.x > last_.x ? first_.x : last_.x,
                     first_.y > last_.y ? first_.y : last_.y);
    }
    inline wxSize delta() const {
      wxSize s;
      s.IncBy(last_), s.DecBy(first_);
      return s;
    }
    inline wxSize dims() const { return wxSize(col_count(), row_count()); }
   private:
    ViewType type_;
    wxPoint first_;
    wxPoint last_;
  };

  ViewTree(wxWindow* parent, wxWindowID id, const wxPoint& pos,
           const wxSize& size, long style);
  unsigned GetLevel(const wxTreeItemId& id) const;
  inline const ItemDataBase& GetViewInfo(const wxTreeItemId& id) const {
    return *static_cast<ViewTree::ItemDataBase*>(GetItemData(id));
  }
};

#endif  /* CPPVIEWS_BENCH_GUI_SM_VIEW_TREE_HPP_ */
