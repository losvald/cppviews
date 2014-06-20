#include "smvd.hpp"

#include "smvd_display.hpp"
// #include "smvd_navigator.hpp"
#include "smvd_frame.hpp"

#include <wx/wx.h>
#include <wx/log.h>

#include <iostream>
#include <fstream>

#define SMVD_FILE_EXT ".smvd"

// application methods

bool SmvdApp::OnInit() {
  MainFrame* frame = new MainFrame(nullptr);
  frame->Show();

  wxLog::SetVerbose();
  wxLog* logger = new wxLogStream;
  wxLog::SetActiveTarget(logger);
  wxLogInfo("Showed main frame");
  return true;
}

void SmvdApp::LoadMatrix(const wxString& path) {
  std::ifstream ifs(path.c_str());
  sm_.Init(ifs);
  wxLogVerbose("Loaded matrix: %d x %d\n",
               int(sm_.row_count()), int(sm_.col_count()));

  // replace the extension with .smv and use as the default save path
  sm_path_ = path.substr(0, path.rfind('.')) + SMVD_FILE_EXT;
  wxLogVerbose("Default save path: %s\n", sm_path_.c_str());
}

IMPLEMENT_APP(SmvdApp)
