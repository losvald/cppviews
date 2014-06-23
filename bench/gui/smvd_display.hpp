#ifndef CPPVIEWS_BENCH_GUI_SMVD_DISPLAY_HPP_
#define CPPVIEWS_BENCH_GUI_SMVD_DISPLAY_HPP_

#include "sm_displayable.hpp"

#include <wx/scrolwin.h>

class wxDC;
class wxSlider;
class wxStaticText;
class Navigator;

class Display : public wxScrolledCanvas, public SMDisplayable {
 public:
  Display(wxWindow* parent, wxWindowID id, const wxPoint& pos,
          const wxSize& size, long style);
  void OnDraw(wxDC&) override;
  void DisplayMatrix();
  void Zoom(int zoom_factor_lg);
  void SetZoomPanel(wxSlider*, wxStaticText*);
  void SetNavigator(Navigator* navigator);

 private:
  void UpdateZoomRange();

  inline void OnMouseEnter(wxMouseEvent& evt) {
    SMDisplayable::OnMouseEnter(evt);
  }
  inline void OnMouseLeave(wxMouseEvent& evt) {
    SMDisplayable::OnMouseLeave(evt);
  }
  void OnMouseMotion(wxMouseEvent&);
  void OnMouseRightDown(wxMouseEvent&);
  void OnMouseRightUp(wxMouseEvent&);
  void OnMouseWheel(wxMouseEvent&);
  void OnSize(wxSizeEvent&);
  DECLARE_EVENT_TABLE()

  wxPoint drag_pos_;
  bool drag_;
  int zoom_factor_lg_min_;
  int zoom_factor_lg_max_;
  wxSlider* zoom_slider_;
  wxStaticText* zoom_label_;
  Navigator* navigator_;
};

#endif  /* CPPVIEWS_BENCH_GUI_SMVD_DISPLAY_HPP_ */
