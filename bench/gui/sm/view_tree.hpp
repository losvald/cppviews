#ifndef CPPVIEWS_BENCH_GUI_SM_VIEW_TREE_HPP_
#define CPPVIEWS_BENCH_GUI_SM_VIEW_TREE_HPP_

#include "sm/view_type.hpp"

#include <wx/treectrl.h>

#include <cstdlib>

class ViewTree : public wxTreeCtrl {
 public:
  ViewTree(wxWindow* parent, wxWindowID id, const wxPoint& pos,
           const wxSize& size, long style);

  class ItemDataBase : public wxTreeItemData {
   public:
    inline ItemDataBase(ViewType type, const wxPoint& first,
                        const wxPoint& last)
        : type_(type),
          first_(first),
          last_(last) {}
    inline virtual ~ItemDataBase() = default;
    inline ViewType type() { return type_; }
    inline int col_count() { return abs(last_.x - first_.x) + 1; }
    inline int row_count() { return abs(last_.y - first_.y) + 1; }
    inline int col_offset() { return first_.x; }
    inline int row_offset() { return first_.y; }
    inline wxPoint first() { return first_; }
    inline wxPoint last() { return last_; }
    inline wxSize delta() {
      wxSize s;
      s.IncBy(last_), s.DecBy(first_);
      return s;
    }
    inline wxSize dims() { return wxSize(col_count(), row_count()); }
   private:
    ViewType type_;
    wxPoint first_;
    wxPoint last_;
  };
};

#endif  /* CPPVIEWS_BENCH_GUI_SM_VIEW_TREE_HPP_ */
