#include "smvd_frame.hpp"

#include "smvd.hpp"
#include "smvd_display.hpp"

#include <wx/filedlg.h>
#include <wx/string.h>

MainFrame::MainFrame(wxWindow* parent) : MainFrameBase(parent) {
  navigator_->SetInfoPanel(display_info_panel_, row_label_, col_label_,
                           val_label_);
  display_->SetInfoPanel(display_info_panel_, row_label_, col_label_,
                         val_label_);
  display_->SetZoomPanel(zoom_slider_, zoom_lg_label_);
  display_->SetNavigator(navigator_);
}

void MainFrame::OnOpen(wxCommandEvent&) {
  wxFileDialog diag(this,
                    "Open Matrix file", "", "",
                    "Matrix Market files (*.mtx)|*.mtx",
                    wxFD_OPEN | wxFD_FILE_MUST_EXIST);
  if (diag.ShowModal() == wxID_CANCEL)
    return;

  wxGetApp().LoadMatrix(diag.GetPath());
  DisplayMatrix();
  display_->SetFocus();
}

void MainFrame::OnQuit(wxCommandEvent&) {
  Close();
}

void MainFrame::OnZoomSliderScroll(wxScrollEvent&) {
  display_->Zoom(zoom_slider_->GetValue());
}

void MainFrame::DisplayMatrix() {
  navigator_->Refresh();
  display_->DisplayMatrix();
}
