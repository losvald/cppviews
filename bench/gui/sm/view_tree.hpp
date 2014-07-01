#ifndef CPPVIEWS_BENCH_GUI_SM_VIEW_TREE_HPP_
#define CPPVIEWS_BENCH_GUI_SM_VIEW_TREE_HPP_

#include "sm/view_type.hpp"

#include <wx/treectrl.h>

#include <cstdlib>
#include <iosfwd>

class Display;

namespace {

class JsonPrettyWriter;

}  // namespace

class ViewTree : public wxTreeCtrl {
 public:
  class ItemDataBase : public wxTreeItemData {
   public:
    typedef unsigned char Direction;
    inline ItemDataBase(ViewType type, const wxPoint& first = wxDefaultPosition,
                        const wxPoint& last = wxDefaultPosition)
        : dir_(0),  // TODO: add ctor arg
          type_(type),
          first_(first),
          last_(last) {}
    inline virtual ~ItemDataBase() = default;
    inline void Init(const wxPoint& first, const wxPoint& last) {
      first_ = first, last_ = last;
    }
    void WriteConfig(std::ostream* os) const;
    inline virtual void BindDisplayEvents(Display* display, ViewTree* tree) {}
    inline virtual void UnbindDisplayEvents(Display* display) {}
    bool Contains(const wxPoint& indices) const;
    inline ViewType type() const { return type_; }
    inline void set_direction(Direction dir) { dir_ = dir; }
    Direction direction() const { return dir_; }
    inline int col_count() const { return abs(last_.x - first_.x) + 1; }
    inline int row_count() const { return abs(last_.y - first_.y) + 1; }
    inline int col_offset() const { return first_.x; }
    inline int row_offset() const { return first_.y; }
    inline wxPoint first() const { return first_; }
    inline wxPoint last() const { return last_; }
    // define corner accessors: top_left/right(), bottom_left/right()
#define SMVD_RELAX_POINT_COORD(c, op)           \
    (first_.c op last_.c ? first_.c : last_.c)
#define SMVD_CORNER_ACCESSOR(name, op_x, op_y)  \
    inline wxPoint name() const {                      \
      return wxPoint(SMVD_RELAX_POINT_COORD(x, op_x),  \
                     SMVD_RELAX_POINT_COORD(y, op_y)); \
    }
    SMVD_CORNER_ACCESSOR(top_left,     <, <)
    SMVD_CORNER_ACCESSOR(top_right,    <, >)
    SMVD_CORNER_ACCESSOR(bottom_left,  >, <)
    SMVD_CORNER_ACCESSOR(bottom_right, >, >)
#undef SMVD_CORNER_ACCESSOR
#undef SMVD_RELAX_POINT_COORD
    // inline wxPoint top_left() const {
    //   return wxPoint(first_.x < last_.x ? first_.x : last_.x,
    //                  first_.y < last_.y ? first_.y : last_.y);
    // }
    // inline wxPoint bottom_right() const {
    //   return wxPoint(first_.x > last_.x ? first_.x : last_.x,
    //                  first_.y > last_.y ? first_.y : last_.y);
    // }
    inline wxSize delta() const {
      wxSize s;
      s.IncBy(last_), s.DecBy(first_);
      return s;
    }
    inline wxSize dims() const { return wxSize(col_count(), row_count()); }

   protected:
    virtual void WriteConfig(JsonPrettyWriter& writer) const;

    Direction dir_;

   private:
    ViewType type_;
    wxPoint first_;
    wxPoint last_;

    friend class ViewTree;
  };

  ViewTree(wxWindow* parent, wxWindowID id, const wxPoint& pos,
           const wxSize& size, long style);
  unsigned GetLevel(const wxTreeItemId& id) const;
  inline ItemDataBase& GetViewInfo(const wxTreeItemId& id) {
    return *static_cast<ViewTree::ItemDataBase*>(GetItemData(id));
  }
  inline const ItemDataBase& GetViewInfo(const wxTreeItemId& id) const {
    return const_cast<ViewTree*>(this)->GetViewInfo(id);
  }
  void ChangeViewType(const wxTreeItemId& id, ViewType type);
  void WriteConfig(std::ostream* os) const;
};

#endif  /* CPPVIEWS_BENCH_GUI_SM_VIEW_TREE_HPP_ */
