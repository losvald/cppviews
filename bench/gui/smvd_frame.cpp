#include "smvd_frame.hpp"

#include "smvd.hpp"
#include "smvd_display.hpp"
#include "sm/view_tree.hpp"
#include "sm/view_type.hpp"

#include <wx/filedlg.h>
#include <wx/string.h>

#include <vector>

class ViewTypeChoice : public ViewTypeSparseArray<wxString> {
 public:
  operator const wxString*() const { return get().data(); }
  static const wxString& get(ViewType type) { return get()[type]; }

  static const ViewTypeChoice& get() {
    static ViewTypeChoice instance;
    return instance;
  }

 private:
  ViewTypeChoice() {
    (*this)[kViewTypeMono] = "mono";
    (*this)[kViewTypeChain] = "chain";
  }
};

MainFrame::MainFrame(wxWindow* parent) : MainFrameBase(parent) {
  navigator_->SetInfoPanel(display_info_panel_, row_label_, col_label_,
                           val_label_);
  display_->SetInfoPanel(display_info_panel_, row_label_, col_label_,
                         val_label_);
  display_->SetZoomPanel(zoom_slider_, zoom_lg_label_);
  display_->SetNavigator(navigator_);
  display_->SetViewTree(view_tree_);
  type_choice_->Set(ViewTypeChoice::get().size(), ViewTypeChoice::get());
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

template<unsigned char view_type>
class ViewParamsChoice;

class ViewParamsChoiceBase : public std::vector<wxString> {
 public:
  static const ViewParamsChoiceBase& get(ViewType);
  operator const wxString*() const { return data(); }
 protected:
  ViewParamsChoiceBase(size_t count) : std::vector<wxString>(count) {}
};

template<>
struct ViewParamsChoice<kViewTypeChain> : public ViewParamsChoiceBase {
  ViewParamsChoice() : ViewParamsChoiceBase(4) {
    typedef typename ViewParams<kViewTypeChain>::Dir Dir;
    (*this)[Dir::kRight] = L"\u2192";
    (*this)[Dir::kDown] = L"\u2193";
    (*this)[Dir::kLeft] = L"\u2190";
    (*this)[Dir::kUp] = L"\u2191";
  }
};

template<>
struct ViewParamsChoice<kViewTypeMono> : public ViewParamsChoiceBase {
  ViewParamsChoice() : ViewParamsChoiceBase(4) {
    typedef typename ViewParams<kViewTypeMono>::Dir Dir;
    (*this)[Dir::kRightDown] = L"\u2198";
    (*this)[Dir::kLeftDown] = L"\u2199";
    (*this)[Dir::kLeftUp] = L"\u2196";
    (*this)[Dir::kRightUp] = L"\u2197";
  }
};

const ViewParamsChoiceBase& ViewParamsChoiceBase::get(ViewType type) {
  // using switch case is easier than using Singleton CRTP in each subclass
#define SMVD_SWITCH_CASE(view_type) \
    case view_type: { static ViewParamsChoice<view_type> p; return p; }
  switch (type) {
    SMVD_SWITCH_CASE(kViewTypeChain);
    SMVD_SWITCH_CASE(kViewTypeMono);
  }
#undef SMVD_SWITCH_CASE
  throw "unreachable";
}

void MainFrame::OnViewTreeSelChanged(wxTreeEvent& evt) {
  SelectView(evt.GetItem());
}

void MainFrame::OnViewChoice(wxCommandEvent& evt) {
  UpdateViewPanel(static_cast<ViewType>(evt.GetInt()));
}

void MainFrame::UpdateViewPanel(ViewType type) {
  auto choices = ViewParamsChoiceBase::get(type);
  dir_choice_->Set(choices.size(), choices);
  dir_choice_->SetSelection(0);
}

void MainFrame::SelectView(const wxTreeItemId& id) {
  auto v = static_cast<ViewTree::ItemDataBase*>(
      view_tree_->GetItemData(id));
  wxLogVerbose("OnViewTreeSelChanged(%d)", v->type());
  type_choice_->SetSelection(v->type());
  level_spin_->SetValue(view_tree_->GetLevel(id));
  UpdateViewPanel(v->type());
  display_->Refresh();
}

void MainFrame::DisplayMatrix() {
  navigator_->Refresh();
  display_->DisplayMatrix();
}
