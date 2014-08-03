#ifndef CPPVIEWS_BENCH_GUI_SMVD_NAVIGATOR_HPP_
#define CPPVIEWS_BENCH_GUI_SMVD_NAVIGATOR_HPP_

#include "sm_displayable.hpp"

#include <wx/window.h>

class wxDC;

class Navigator : public wxWindow, public SMDisplayable {
 public:
  Navigator(wxWindow* parent);
  void SetDisplayRegion(int row_from, int col_from, int row_to, int col_to);

 private:
  inline void OnMouseEnter(wxMouseEvent& evt) {
    SMDisplayable::OnMouseEnter(evt);
  }
  inline void OnMouseLeave(wxMouseEvent& evt) {
    SMDisplayable::OnMouseLeave(evt);
  }
  void OnMouseMotion(wxMouseEvent&);
  void OnPaint(wxPaintEvent&);
  DECLARE_EVENT_TABLE()

  int disp_row_from_, disp_row_to_;
  int disp_col_from_, disp_col_to_;
};

#endif  /* CPPVIEWS_BENCH_GUI_SMVD_NAVIGATOR_HPP_ */
