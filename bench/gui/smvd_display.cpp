#include "smvd_display.hpp"
#include "smvd_navigator.hpp"

#include "smvd.hpp"

#include <wx/dc.h>
#include <wx/log.h>
#include <wx/slider.h>

#include <algorithm>
#include <chrono>

BEGIN_EVENT_TABLE(Display, wxScrolledCanvas)
EVT_ENTER_WINDOW(Display::OnMouseEnter)
EVT_LEAVE_WINDOW(Display::OnMouseLeave)
EVT_MOTION(Display::OnMouseMotion)
EVT_MOUSEWHEEL(Display::OnMouseWheel)
EVT_RIGHT_DOWN(Display::OnMouseRightDown)
EVT_RIGHT_UP(Display::OnMouseRightUp)
EVT_SIZE(Display::OnSize)
END_EVENT_TABLE()

Display::Display(wxWindow* parent, wxWindowID id,
                 const wxPoint& pos, const wxSize& size,
                 long style)
    : wxScrolledCanvas(parent, id, pos, size, style) {
  // DisableKeyboardScrolling();
  SetVirtualSize(1000, 1000);  // TODO
}

void Display::DisplayMatrix() {
  UpdateZoomRange();
  Zoom(0);
}

void Display::Zoom(int zoom_factor_lg) {
  zoom_factor_lg_ = zoom_factor_lg;
  const auto& sm = wxGetApp().matrix();
  int x = sm.col_count(), y = sm.row_count();
  Scale(&x, &y);
  wxLogVerbose("Display::Zoom(%d) %d %d", zoom_factor_lg_, x, y);
  SetVirtualSize(x, y);
  int scroll_rate = zoom_factor_lg > 0 ? 1 << zoom_factor_lg : 1;
  SetScrollRate(scroll_rate, scroll_rate);
  Refresh();

  zoom_slider_->SetValue(zoom_factor_lg);
  zoom_label_->SetLabelText(wxString::Format("%d", zoom_factor_lg_));
}

void Display::OnDraw(wxDC& dc) {
  int x, y;
  GetClientSize(&x, &y);
  wxLogVerbose("Repainting Display #%d: %d x %d", x, y);

  if (!wxGetApp().IsMatrixLoaded())
    return;

  // compute the beginning of the visible row/col range
  GetViewStart(&x, &y);
  if (zoom_factor_lg_ < 0)
    x <<= -zoom_factor_lg_, y <<= -zoom_factor_lg_;
  const int row_from = y;
  const int col_from = x;

  // compute the end of the visible row/col range
  GetClientSize(&x, &y);
  if (zoom_factor_lg_ > 0) {
    x = (x + (1 << zoom_factor_lg_) - 1) >> zoom_factor_lg_;
    y = (y + (1 << zoom_factor_lg_) - 1) >> zoom_factor_lg_;
  } else {
    x <<= -zoom_factor_lg_, y <<= -zoom_factor_lg_;
  }
  x += col_from, y += row_from;
  const auto& sm = wxGetApp().matrix();
  const int row_to = std::min(y, sm.row_count());
  const int col_to = std::min(x, sm.col_count());

  using namespace std::chrono;
  auto tp_before = steady_clock::now();
  DrawMatrix(row_from, col_from, row_to, col_to, &dc);
  auto tp_after = steady_clock::now();
  wxLogVerbose("Display::DrawMatrix(zoom_factor_lg=%d) time: %lld ns\n",
               zoom_factor_lg_,
               1LL * duration_cast<nanoseconds>(tp_after - tp_before).count());

  navigator_->SetDisplayRegion(row_from, col_from, row_to, col_to);
}

void Display::OnMouseMotion(wxMouseEvent& evt) {
  if (!wxGetApp().IsMatrixLoaded())
    return;

  int x, y;
  CalcUnscrolledPosition(evt.GetX(), evt.GetY(), &x, &y);
  SetIndices(x, y);
}

void Display::OnMouseRightDown(wxMouseEvent& evt) {
  drag_ = true;
  drag_pos_ = evt.GetPosition();
}

void Display::OnMouseRightUp(wxMouseEvent& evt) {
  // double-click generates two events, so handle just the first one
  if (!drag_)
    return;

  drag_ = false;
  auto delta = (drag_pos_ - evt.GetPosition()) /
      (1 << std::max(0, zoom_factor_lg_));
  Scroll(GetViewStart() + delta);
  Refresh();
}

void Display::OnMouseWheel(wxMouseEvent& evt) {
  int zoom_factor_lg = zoom_factor_lg_;
  if (evt.GetWheelRotation() >= 0) {
    if (++zoom_factor_lg > zoom_factor_lg_max_)
      return;
  } else if (--zoom_factor_lg < zoom_factor_lg_min_)
    return;

  Zoom(zoom_factor_lg);

  wxLogVerbose("Changed display_.zoom_factor_lg_: %d\n", zoom_factor_lg_);
}

void Display::OnSize(wxSizeEvent& evt) {
  UpdateZoomRange();
}

void Display::UpdateZoomRange() {
  if (!wxGetApp().IsMatrixLoaded())
    return;

  const auto& sm = wxGetApp().matrix();
  int x, y;
  GetClientSize(&x, &y);
  zoom_factor_lg_min_ = std::min(
      std::min(0, BestZoomFactorLg(sm.col_count(), x)),
      std::min(0, BestZoomFactorLg(sm.row_count(), y)));
  zoom_factor_lg_max_ = std::max(0, BestZoomFactorLg(1, std::max(x, y)));
  zoom_slider_->SetRange(zoom_factor_lg_min_, zoom_factor_lg_max_);
  wxLogVerbose("Display::UpdateZoomRange [%d, %d]", zoom_factor_lg_min_,
               zoom_factor_lg_max_);
}

void Display::SetZoomPanel(wxSlider* zoom_slider, wxStaticText* zoom_label) {
  zoom_slider_ = zoom_slider;
  zoom_label_ = zoom_label;
}

void Display::SetNavigator(Navigator* navigator) { navigator_ = navigator; }
