#ifndef CPPVIEWS_BENCH_GUI_SMVD_FRAME_HPP_
#define CPPVIEWS_BENCH_GUI_SMVD_FRAME_HPP_

#include "gen/smvd_gui.h"

class wxWindow;
class wxCommandEvent;

class MainFrame : public MainFrameBase {
 public:
  MainFrame(wxWindow* parent);
 protected:
  void OnOpen(wxCommandEvent&) override;
  void OnQuit(wxCommandEvent&) override;
  void OnZoomSliderScroll(wxScrollEvent&) override;
  void OnViewChoice(wxCommandEvent&) override;
 private:
  void DisplayMatrix();
};

#endif  /* CPPVIEWS_BENCH_GUI_SMVD_FRAME_HPP_ */
