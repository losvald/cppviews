#ifndef CPPVIEWS_BENCH_GUI_SMVD_H_
#define CPPVIEWS_BENCH_GUI_SMVD_H_

#include "../util/sparse_matrix.hpp"

#include <wx/app.h>

typedef SparseMatrix<double> SM;

class SmvdApp : public wxApp {
 public:
  void LoadMatrix(const wxString& path);
  inline bool IsMatrixLoaded() const { return !sm_path_.empty(); }
  const SM& matrix() const { return sm_; }
 protected:
  bool OnInit() override;
 private:
  SM sm_;
  std::string sm_path_;
};
DECLARE_APP(SmvdApp)

#endif  /* CPPVIEWS_BENCH_GUI_SMVD_H_ */
