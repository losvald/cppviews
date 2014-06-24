#include "smvd_display.hpp"
#include "smvd_navigator.hpp"
#include "sm/view_creator.hpp"
#include "sm/view_tree.hpp"
#include "sm/view_type.hpp"

#include "smvd.hpp"

#include <wx/dcclient.h>
#include <wx/dcgraph.h>
#include <wx/log.h>
#include <wx/graphics.h>
#include <wx/slider.h>

#include <algorithm>
#include <chrono>
#include <queue>

BEGIN_EVENT_TABLE(Display, wxScrolledCanvas)
EVT_ENTER_WINDOW(Display::OnMouseEnter)
EVT_LEAVE_WINDOW(Display::OnMouseLeave)
EVT_MOTION(Display::OnMouseMotion)
EVT_MOUSEWHEEL(Display::OnMouseWheel)
EVT_PAINT(Display::OnPaint)
EVT_RIGHT_DOWN(Display::OnMouseRightDown)
EVT_RIGHT_UP(Display::OnMouseRightUp)
EVT_SIZE(Display::OnSize)
END_EVENT_TABLE()

Display::Display(wxWindow* parent, wxWindowID id,
                 const wxPoint& pos, const wxSize& size,
                 long style)
    : wxScrolledCanvas(parent, id, pos, size, style) {
  // typedef ViewCreator<kViewTypeChain> VC;
  // static VC vc;
  // ViewCreatorBase& b = vc;
  // Bind(wxEVT_LEFT_DOWN, &ViewCreatorBase::OnMouseLeftDown, &b);
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

void Display::OnPaint(wxPaintEvent& evt) {
  wxPaintDC dc(this);
  DoPrepareDC(dc);

  int x, y;
  GetClientSize(&x, &y);
  wxLogVerbose("Repainting %d x %d Display", x, y);

  if (!wxGetApp().IsMatrixLoaded())
    return;

  // compute the beginning of the visible row/col range
  GetViewStart(&x, &y);
  if (zoom_factor_lg_ < 0)
    x <<= -zoom_factor_lg_, y <<= -zoom_factor_lg_;
  const int row_from = y;
  const int col_from = x;

  // compute the end of the visible row/col range
  GetClientSize(&x, &y);  // Using a bit trick: x & -(x > 0) == max(x, 0)
  const int scale_mask = (1 << (zoom_factor_lg_ & -(zoom_factor_lg_ > 0))) - 1;
  if (zoom_factor_lg_ > 0) {
    wxASSERT(scale_mask == (1 << zoom_factor_lg_) - 1);
    x = (x + scale_mask) >> zoom_factor_lg_;
    y = (y + scale_mask) >> zoom_factor_lg_;
  } else {
    wxASSERT(scale_mask == 0);
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

  // Draw views

  static const wxColor lvl_color[] = {
    *wxBLACK,
    wxColor(0xFFu, 0, 0xFFu),
    *wxBLUE,
    *wxGREEN,
    *wxRED,
  };
  wxColor color;

  // transparency (%) is going to be incremented by kAlphaNormInc from 0.3
  float alpha_norm = 0.2f;
  static const float kAlphaNormInc = 0.1f;
  static const float kAlphaNormMax = 0.8f;

  // pen width is going to be decremented by kPenWidthInc down to 1
  static const int kPenWidthInc = 2;
  int pen_width = (sizeof(lvl_color) - 1) / sizeof(wxColor) * kPenWidthInc + 1;

  // create a graphics context to allow for transparent colors
  // wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
  wxGCDC gcdc(dc);
  auto const gc = &gcdc;

  static const auto& kViewBrush = *wxTRANSPARENT_BRUSH;
  gc->SetBrush(kViewBrush);
  wxPen pen; pen.SetJoin(wxJOIN_MITER);

  std::queue<wxTreeItemId> q;
  q.push(view_tree_->GetRootItem());
  int lvl_cur = -1;
  int lvl_cnt[2] = {1, 0};
  for (bool lvl_odd = false, lvl_done = true; !q.empty(); lvl_odd = !lvl_odd) {
    // if a new level begins, initialize it first
    if (lvl_done) {
      ++lvl_cur;
      alpha_norm = std::min(alpha_norm + kAlphaNormInc, kAlphaNormMax);
      color.SetRGBA(lvl_color[lvl_cur].GetRGB() |
                    (wxUint32(0xFF * alpha_norm) << 24));
      pen.SetColour(color);
      pen.SetWidth(pen_width -= kPenWidthInc);
      gc->SetPen(pen);
      wxLogVerbose("Begin drawing view level %d", lvl_cur);
    }

    // draw the view represented by the current node
    const auto& view_id = q.front();
    const auto& vi = view_tree_->GetViewInfo(view_id);
    wxPoint p_tl(vi.top_left()), p_br(vi.bottom_right());
    Scale(&p_tl.x, &p_tl.y), Scale(&p_br.x, &p_br.y);
    p_br += wxPoint(scale_mask, scale_mask);  // round up (if zoomed)
    int w(p_br.x - p_tl.x + 1), h(p_br.y - p_tl.y + 1);
    // translate the coordinates as per: http://trac.wxwidgets.org/ticket/12109
    p_tl = CalcScrolledPosition(p_tl);
    bool selected = (view_id == view_tree_->GetSelection());
    if (selected)
      gc->SetBrush(wxColor(0xFFu, 0xFFu, 0, 0x60));
    gc->DrawRectangle(p_tl.x, p_tl.y, w, h);
    if (selected)
      gc->SetBrush(kViewBrush);
    wxLogVerbose("Drawing view: %s (x=%d, y=%d, w=%d, h=%d, alpha=%02X)",
                 view_tree_->GetItemText(view_id), p_tl.x, p_tl.y, w, h,
                 (unsigned)color.Alpha());

    // pop the current node and update the current level info accordingly
    lvl_done = (--lvl_cnt[lvl_odd] == 0);
    q.pop();

    // push children nodes and update the next level count accordingly
    lvl_cnt[!lvl_odd] -= q.size();
    wxTreeItemIdValue cookie;
    for (auto id = view_tree_->GetFirstChild(view_id, cookie); id.IsOk();
         id = view_tree_->GetNextChild(view_id, cookie)) {
      q.push(id);
    }
    lvl_cnt[!lvl_odd] += q.size();
  }
}

void Display::OnMouseMotion(wxMouseEvent& evt) {
  if (!wxGetApp().IsMatrixLoaded())
    return;

  int x, y;
  CalcUnscrolledPosition(evt.GetX(), evt.GetY(), &x, &y);
  SetIndices(x, y);

  // propagate the event to the creator
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
