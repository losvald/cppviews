#include "sm_displayable.hpp"

#include "smvd.hpp"

#include <wx/brush.h>
#include <wx/dc.h>
#include <wx/pen.h>
#include <wx/string.h>

#include <wx/log.h>

void SMDisplayable::DrawMatrix(int row_from, int col_from,
                               int row_to, int col_to,
                               wxDC* dc) {
  // compute the logical region in pixels
  int x = col_from, y = row_from;
  Scale(&x, &y);
  const int w = Scale(col_to - col_from);
  const int h = Scale(row_to - row_from);

  // fill the white rectangle for the matrix area
  const auto& bg_color = dc->GetBackground();
  dc->SetPen(*wxWHITE_PEN);
  dc->DrawRectangle(x, y, w, h);
  dc->SetPen(*wxBLACK_PEN);
  dc->SetBackground(bg_color);

  const auto& sm = wxGetApp().matrix();
  if (zoom_factor_lg_ > 0) {
    // for zoomed-in matrix, draw nonzero elements as hollow rectangles
    dc->SetBrush(*wxTRANSPARENT_BRUSH);
    const int scale = 1 << zoom_factor_lg_;
    y = row_from << zoom_factor_lg_;
    for (int row = row_from; row < row_to; ++row) {
      for (auto col_range = sm.nonzero_col_range(row, col_from, col_to);
           col_range.first != col_range.second; ++col_range.first) {
        x = *col_range.first << zoom_factor_lg_;
        dc->DrawRectangle(x, y, scale, scale);
      }
      y += scale;
    }
  } else {
    // otherwise, draw nonzero elements as points (using "max" interpolation)
    const int scale_lg = -zoom_factor_lg_;
    const int mask = (1 << scale_lg) - 1;
    y = (row_from + mask) >> scale_lg;
    for (int row = row_from; row < row_to; ++row) {
      for (auto col_range = sm.nonzero_col_range(row, col_from, col_to);
           col_range.first != col_range.second; ++col_range.first) {
        x = *col_range.first >> scale_lg;
        dc->DrawPoint(x, y);
      }
      y += (row & mask) == 0;
    }
  }
}

void SMDisplayable::SetIndices(wxInt64 x, wxInt64 y) {
  InvertedScale(&x, &y);
  col_label_->SetLabelText(wxString::Format("%lld", x));
  row_label_->SetLabelText(wxString::Format("%lld", y));

  if (zoom_factor_lg_ >= 0) {
    const auto& sm = wxGetApp().matrix();
    if (x >= 0 && y >= 0 &&
        x < wxInt64(sm.col_count()) && y < wxInt64(sm.row_count())) {
      val_label_->SetLabelText(
          wxString::Format("%lg", double(wxGetApp().matrix()(y, x))));
    } else
      val_label_->SetLabelText("");
  } else
    val_label_->SetLabelText("???");
}

void SMDisplayable::OnMouseEnter(wxMouseEvent& evt) {
  info_panel_->Enable();
}

void SMDisplayable::OnMouseLeave(wxMouseEvent& evt) {
  info_panel_->Disable();
}
