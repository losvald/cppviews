#ifndef CPPVIEWS_BENCH_GUI_SM_DISPLAYABLE_HPP_
#define CPPVIEWS_BENCH_GUI_SM_DISPLAYABLE_HPP_

#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/stattext.h>

class wxDC;
class wxMouseEvent;

class SMDisplayable {
 public:
  SMDisplayable() : zoom_factor_lg_(0) {}

  inline void SetInfoPanel(wxPanel* panel,
                           wxStaticText* row_label,
                           wxStaticText* col_label,
                           wxStaticText* val_label) {
    info_panel_ = panel;
    row_label_ = row_label;
    col_label_ = col_label;
    val_label_ = val_label;
  }

  void SetIndices(wxInt64 row, wxInt64 col);
 protected:
  template<typename T>
  inline T Scale(T x) {
    return zoom_factor_lg_ >= 0 ? x << zoom_factor_lg_ : x >> -zoom_factor_lg_;
  }

  template<typename T>
  inline void Scale(T* x, T* y) { return Scale(zoom_factor_lg_, x, y); }

  static inline int BestZoomFactorLg(size_t sm_dim, size_t dim) {
    int zoom_factor_lg = 0;
    if (sm_dim <= dim) {
      do {
        ++zoom_factor_lg;
      } while ((sm_dim <<= 1) <= dim);
      --zoom_factor_lg;
      sm_dim >>= 1;
    } else {
      do {
        --zoom_factor_lg;
      } while ((sm_dim >>= 1) > dim);
    }
    return zoom_factor_lg;
  }

  void DrawMatrix(int row_from, int col_from, int row_to, int col_to, wxDC*);

  wxPanel* info_panel_;
  wxStaticText* row_label_;
  wxStaticText* col_label_;
  wxStaticText* val_label_;
  int zoom_factor_lg_;

 protected:
  void OnMouseEnter(wxMouseEvent&);
  void OnMouseLeave(wxMouseEvent&);

 private:
  template<typename T>
  static inline void Scale(int factor_lg, T* x, T* y) {
    if (factor_lg >= 0)
      *x <<= factor_lg, *y <<= factor_lg;
    else {
      *x >>= -factor_lg, *y >>= -factor_lg;
      // factor_lg = -factor_lg;
      // T factor = T(); ++factor, factor <<= factor_lg;  // == (1 << factor_lg)
      // *x = (*x + factor - 1) >> factor_lg;
      // *y = (*y + factor - 1) >> factor_lg;
    }
  }
};

#endif  /* CPPVIEWS_BENCH_GUI_SM_DISPLAYABLE_HPP_ */
