#ifndef CPPVIEWS_BENCH_GUI_SMVD_FRAME_HPP_
#define CPPVIEWS_BENCH_GUI_SMVD_FRAME_HPP_

#include "gen/smvd_gui.h"
#include "sm/view_type.hpp"

class wxWindow;
class wxCommandEvent;
class ViewConfigDialog;

class MainFrame : public MainFrameBase {
 public:
  MainFrame(wxWindow* parent);
  void SelectView(const wxTreeItemId& id);
 protected:
  void OnOpen(wxCommandEvent&) override;
  void OnQuit(wxCommandEvent&) override;
  void OnZoomSliderScroll(wxScrollEvent&) override;
  void OnViewChoice(wxCommandEvent&) override;
  void OnViewConfigure(wxCommandEvent&) override;
  void OnViewTreeSelChanged(wxTreeEvent&) override;
 private:
  void DisplayMatrix();
  void UpdateViewPanel(ViewType type);

  friend class ViewConfigDialog;
};

#endif  /* CPPVIEWS_BENCH_GUI_SMVD_FRAME_HPP_ */
