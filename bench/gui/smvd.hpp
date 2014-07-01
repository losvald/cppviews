#ifndef CPPVIEWS_BENCH_GUI_SMVD_H_
#define CPPVIEWS_BENCH_GUI_SMVD_H_

#include "../util/sparse_matrix.hpp"

#include <wx/app.h>
#include <wx/string.h>

typedef SparseMatrix<double> SM;

class SmvdApp : public wxApp {
 public:
  void LoadMatrix(const wxString& path);
  inline bool IsMatrixLoaded() const { return !config_path_.empty(); }
  inline const SM& matrix() const { return sm_; }
  inline const wxString& config_path() const { return config_path_; }
 protected:
  bool OnInit() override;
 private:
  SM sm_;
  wxString config_path_;
};
DECLARE_APP(SmvdApp)

#endif  /* CPPVIEWS_BENCH_GUI_SMVD_H_ */
