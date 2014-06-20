#include "smvd_navigator.hpp"

#include "smvd.hpp"

#include <wx/brush.h>
#include <wx/dcclient.h>
#include <wx/log.h>
#include <wx/pen.h>

#include <algorithm>
#include <chrono>

BEGIN_EVENT_TABLE(Navigator, wxWindow)
EVT_ENTER_WINDOW(Navigator::OnMouseEnter)
EVT_LEAVE_WINDOW(Navigator::OnMouseLeave)
EVT_MOTION(Navigator::OnMouseMotion)
EVT_PAINT(Navigator::OnPaint)
END_EVENT_TABLE()

Navigator::Navigator(wxWindow* parent) : wxWindow(parent, wxID_ANY) {}

void Navigator::OnMouseMotion(wxMouseEvent& evt) {
  if (!wxGetApp().IsMatrixLoaded())
    return;

  SetIndices(evt.GetX(), evt.GetY());
}

void Navigator::OnPaint(wxPaintEvent& evt) {
  if (!wxGetApp().IsMatrixLoaded())
    return;

  // calculate zoom factor, assuming width = height
  const auto& sm = wxGetApp().matrix();
  zoom_factor_lg_ = BestZoomFactorLg(std::max(sm.row_count(), sm.col_count()),
                                     GetClientSize().GetWidth());
  wxClientDC dc(this);
  using namespace std::chrono;
  auto tp_before = steady_clock::now();
  DrawMatrix(0, 0, sm.row_count(), sm.col_count(), &dc);
  auto tp_after = steady_clock::now();
  wxLogVerbose("Navigator::DrawMatrix(zoom_factor_lg=%d) time: %lld ns\n",
               zoom_factor_lg_,
               1LL * duration_cast<nanoseconds>(tp_after - tp_before).count());

  dc.SetPen(*wxYELLOW_PEN);
  dc.SetBrush(*wxTRANSPARENT_BRUSH);
  dc.DrawRectangle(Scale(disp_col_from_) - 1,
                   Scale(disp_row_from_) - 1,
                   Scale(disp_col_to_ - disp_col_from_) + 2,
                   Scale(disp_row_to_ - disp_row_from_) + 2);
}

void Navigator::SetDisplayRegion(int row_from, int col_from,
                                 int row_to, int col_to) {
  disp_row_from_ = row_from, disp_col_from_ = col_from;
  disp_row_to_ = row_to, disp_col_to_ = col_to;
  wxLogVerbose("Navigator::SetDisplayRegion(%d, %d, %d, %d)",
               row_from, col_from, row_to, col_to);
  Refresh();
}
