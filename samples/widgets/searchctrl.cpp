/////////////////////////////////////////////////////////////////////////////
// Program:     wxWidgets Widgets Sample
// Name:        searchctrl.cpp
// Purpose:     Shows wxSearchCtrl
// Author:      Robin Dunn
// Created:     9-Dec-2006
// Copyright:   (c) 2006
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// for compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"


#if wxUSE_SEARCHCTRL

// for all others, include the necessary headers
#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/log.h"
    #include "wx/radiobox.h"
    #include "wx/statbox.h"
#endif

#include "wx/artprov.h"
#include "wx/sizer.h"
#include "wx/stattext.h"
#include "wx/checkbox.h"
#include "wx/menu.h"

#include "wx/srchctrl.h"

#include "widgets.h"

#include "icons/text.xpm"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// control ids
enum
{
     ID_SEARCH_CB = wxID_HIGHEST,
     ID_CANCEL_CB,
     ID_MENU_CB,
     ID_CUST_BMPS_CB,
     ID_READ_ONLY_CB,
     ID_BIG_SIZE_CB,

     ID_SEARCHMENU,
     ID_SEARCHMENU_LAST = ID_SEARCHMENU + 5
};


// ----------------------------------------------------------------------------
// ColourPickerWidgetsPage
// ----------------------------------------------------------------------------

class SearchCtrlWidgetsPage : public WidgetsPage
{
public:
    SearchCtrlWidgetsPage(WidgetsBookCtrl *book, wxImageList *imaglist);

    virtual wxWindow *GetWidget() const wxOVERRIDE { return m_srchCtrl; }
    virtual wxTextEntryBase *GetTextEntry() const wxOVERRIDE { return m_srchCtrl; }
    virtual void RecreateWidget() wxOVERRIDE;

    // lazy creation of the content
    virtual void CreateContent() wxOVERRIDE;

protected:

    void OnToggleSearchButton(wxCommandEvent&);
    void OnToggleCancelButton(wxCommandEvent&);
    void OnToggleSearchMenu(wxCommandEvent&);
    void OnToggleCustomBitmaps(wxCommandEvent&);
    void OnToggleReadOnly(wxCommandEvent&);
    void OnToggleBigSize(wxCommandEvent&);

    void OnText(wxCommandEvent& event);
    void OnTextEnter(wxCommandEvent& event);

    void OnSearchMenu(wxCommandEvent& event);

    void OnSearch(wxCommandEvent& event);
    void OnSearchCancel(wxCommandEvent& event);

    wxMenu* CreateTestMenu();

    // (re)create the control
    void CreateControl();


    wxSearchCtrl*       m_srchCtrl;
    wxCheckBox*         m_searchBtnCheck;
    wxCheckBox*         m_cancelBtnCheck;
    wxCheckBox*         m_menuBtnCheck;
    wxCheckBox*         m_custBmpsCheck;
    wxCheckBox*         m_readOnlyCheck;
    wxCheckBox*         m_bigSizeCheck;

private:
    wxDECLARE_EVENT_TABLE();
    DECLARE_WIDGETS_PAGE(SearchCtrlWidgetsPage)
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(SearchCtrlWidgetsPage, WidgetsPage)
    EVT_CHECKBOX(ID_SEARCH_CB, SearchCtrlWidgetsPage::OnToggleSearchButton)
    EVT_CHECKBOX(ID_CANCEL_CB, SearchCtrlWidgetsPage::OnToggleCancelButton)
    EVT_CHECKBOX(ID_MENU_CB, SearchCtrlWidgetsPage::OnToggleSearchMenu)
    EVT_CHECKBOX(ID_CUST_BMPS_CB, SearchCtrlWidgetsPage::OnToggleCustomBitmaps)
    EVT_CHECKBOX(ID_READ_ONLY_CB, SearchCtrlWidgetsPage::OnToggleReadOnly)
    EVT_CHECKBOX(ID_BIG_SIZE_CB, SearchCtrlWidgetsPage::OnToggleBigSize)

    EVT_TEXT(wxID_ANY, SearchCtrlWidgetsPage::OnText)
    EVT_TEXT_ENTER(wxID_ANY, SearchCtrlWidgetsPage::OnTextEnter)

    EVT_MENU_RANGE(ID_SEARCHMENU, ID_SEARCHMENU_LAST,
                   SearchCtrlWidgetsPage::OnSearchMenu)

    EVT_SEARCH(wxID_ANY, SearchCtrlWidgetsPage::OnSearch)
    EVT_SEARCH_CANCEL(wxID_ANY, SearchCtrlWidgetsPage::OnSearchCancel)
wxEND_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

#if defined(__WXMSW__) || defined(__WXMAC__) || defined(__WXGTK20__)
    #define FAMILY_CTRLS NATIVE_CTRLS
#else
    #define FAMILY_CTRLS GENERIC_CTRLS
#endif

IMPLEMENT_WIDGETS_PAGE(SearchCtrlWidgetsPage, "SearchCtrl",
                       FAMILY_CTRLS | EDITABLE_CTRLS | ALL_CTRLS);

SearchCtrlWidgetsPage::SearchCtrlWidgetsPage(WidgetsBookCtrl *book,
                                     wxImageList *imaglist)
                  : WidgetsPage(book, imaglist, text_xpm)
{
}

void SearchCtrlWidgetsPage::CreateContent()
{
    m_srchCtrl = NULL;

    CreateControl();


    wxSizer* box = new wxStaticBoxSizer(
        new wxStaticBox(this, -1, "Options"),
        wxVERTICAL);

    m_searchBtnCheck = new wxCheckBox(this, ID_SEARCH_CB, "Search button");
    m_cancelBtnCheck = new wxCheckBox(this, ID_CANCEL_CB, "Cancel button");
    m_menuBtnCheck   = new wxCheckBox(this, ID_MENU_CB,   "Search menu");
    m_custBmpsCheck  = new wxCheckBox(this, ID_CUST_BMPS_CB, "Custom bitmaps");
    m_readOnlyCheck  = new wxCheckBox(this, ID_READ_ONLY_CB, "Read-Only");
    m_bigSizeCheck   = new wxCheckBox(this, ID_BIG_SIZE_CB,  "Big size");

    m_searchBtnCheck->SetValue(true);

    box->Add(m_searchBtnCheck, wxSizerFlags().Border());
    box->Add(m_cancelBtnCheck, wxSizerFlags().Border());
    box->Add(m_menuBtnCheck,   wxSizerFlags().Border());
    box->Add(m_custBmpsCheck,  wxSizerFlags().Border());
    box->Add(m_readOnlyCheck,  wxSizerFlags().Border());
    box->Add(m_bigSizeCheck,   wxSizerFlags().Border());

    wxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(box, wxSizerFlags().Expand().TripleBorder());
    sizer->Add(m_srchCtrl, wxSizerFlags().Centre().TripleBorder());

    SetSizer(sizer);
}

void SearchCtrlWidgetsPage::CreateControl()
{
    if (m_srchCtrl)
        m_srchCtrl->Destroy();

    long style = GetAttrs().m_defaultFlags;

    m_srchCtrl = new wxSearchCtrl(this, -1, wxEmptyString, wxDefaultPosition,
                                  FromDIP(wxSize(150, -1)), style);
}

void SearchCtrlWidgetsPage::RecreateWidget()
{
    CreateControl();

    GetSizer()->Add(m_srchCtrl, wxSizerFlags().Centre().TripleBorder());

    Layout();
}

wxMenu* SearchCtrlWidgetsPage::CreateTestMenu()
{
    wxMenu* menu = new wxMenu;
    wxMenuItem* menuItem = menu->Append(wxID_ANY, "Recent Searches", "", wxITEM_NORMAL);
    menuItem->Enable(false);
    for ( int i = 0; i < ID_SEARCHMENU_LAST - ID_SEARCHMENU; i++ )
    {
        wxString itemText = wxString::Format("item %i",i);
        wxString tipText = wxString::Format("tip %i",i);
        menu->Append(ID_SEARCHMENU+i, itemText, tipText, wxITEM_CHECK);
    }
    return menu;
}


// ----------------------------------------------------------------------------
// event handlers
// ----------------------------------------------------------------------------

void SearchCtrlWidgetsPage::OnToggleSearchButton(wxCommandEvent&)
{
    m_srchCtrl->ShowSearchButton( m_searchBtnCheck->GetValue() );
}

void SearchCtrlWidgetsPage::OnToggleCancelButton(wxCommandEvent&)
{
    m_srchCtrl->ShowCancelButton( m_cancelBtnCheck->GetValue() );

}

void SearchCtrlWidgetsPage::OnToggleSearchMenu(wxCommandEvent&)
{
    if ( m_menuBtnCheck->GetValue() )
        m_srchCtrl->SetMenu( CreateTestMenu() );
    else
        m_srchCtrl->SetMenu(NULL);

    m_srchCtrl->ShowSearchButton(m_searchBtnCheck->GetValue());
}

void SearchCtrlWidgetsPage::OnToggleCustomBitmaps(wxCommandEvent&)
{
    if ( m_custBmpsCheck->GetValue() )
    {
        m_srchCtrl->SetSearchBitmap(wxArtProvider::GetBitmap(wxART_FIND,
                                    wxART_BUTTON, wxSize(256, 256)));
        m_srchCtrl->SetSearchMenuBitmap(wxArtProvider::GetBitmap(wxART_LIST_VIEW,
                                        wxART_BUTTON, wxSize(256, 256)));
        m_srchCtrl->SetCancelBitmap(wxArtProvider::GetBitmap(wxART_ERROR,
                                    wxART_BUTTON, wxSize(256, 256)));
    }
    else
    {
        m_srchCtrl->SetSearchBitmap(wxNullBitmap);
        m_srchCtrl->SetSearchMenuBitmap(wxNullBitmap);
        m_srchCtrl->SetCancelBitmap(wxNullBitmap);
    }
}

void SearchCtrlWidgetsPage::OnToggleReadOnly(wxCommandEvent&)
{
    m_srchCtrl->SetEditable(!m_readOnlyCheck->GetValue());
}

void SearchCtrlWidgetsPage::OnToggleBigSize(wxCommandEvent&)
{
    wxFont font = m_srchCtrl->GetFont();

    if ( m_bigSizeCheck->GetValue() )
    {
        m_srchCtrl->SetMinSize(m_srchCtrl->GetSize() * 5);
        font.SetPointSize(font.GetPointSize() * 5);
    }
    else
    {
        m_srchCtrl->SetMinSize(FromDIP(wxSize(150, wxDefaultCoord)));
        font.SetPointSize(m_srchCtrl->GetDefaultAttributes().font.GetPointSize());
    }

    m_srchCtrl->SetFont(font);
    Layout();
}

void SearchCtrlWidgetsPage::OnText(wxCommandEvent& event)
{
    wxLogMessage("Search control: text changes, contents is \"%s\".",
                 event.GetString());
}

void SearchCtrlWidgetsPage::OnTextEnter(wxCommandEvent& event)
{
    wxLogMessage("Search control: enter pressed, contents is \"%s\".",
                 event.GetString());
}

void SearchCtrlWidgetsPage::OnSearchMenu(wxCommandEvent& event)
{
    int id = event.GetId() - ID_SEARCHMENU;
    wxLogMessage("Search menu: \"item %i\" selected (%s).",
                 id, event.IsChecked() ? "checked" : "unchecked");
}

void SearchCtrlWidgetsPage::OnSearch(wxCommandEvent& event)
{
    wxLogMessage("Search button: search for \"%s\".", event.GetString());
}

void SearchCtrlWidgetsPage::OnSearchCancel(wxCommandEvent& event)
{
    wxLogMessage("Cancel button pressed.");

    event.Skip();
}

#endif  //  wxUSE_SEARCHCTRL
