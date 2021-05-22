///////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/srchctrl.h
// Purpose:     wxSearchCtrl class declaration for Win32
// Author:      Ali Kettab
// Modified by: Youcef Kouchkar (a.k.a. Tomay)
// Created:     2021-05-01
// Copyright:   Ali Kettab
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MSW_SEARCHCTRL_H_
#define _WX_MSW_SEARCHCTRL_H_

#if wxUSE_SEARCHCTRL

#include "wx/bitmap.h"
#include "wx/msw/private.h"
#include "wx/menu.h"

// ----------------------------------------------------------------------------
// wxSearchCtrl is basically a wxTextCtrl
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxSearchCtrl : public wxSearchCtrlBase
{
public:
    // creation
    // --------
    wxSearchCtrl() {}
    wxSearchCtrl(wxWindow *parent,
                 wxWindowID id,
                 const wxString& value = wxEmptyString,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize,
                 const long style = 0,
                 const wxValidator& validator = wxDefaultValidator,
                 const wxString& name = wxSearchCtrlNameStr);

    virtual ~wxSearchCtrl() {}

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxString& value = wxEmptyString,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                const long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxSearchCtrlNameStr);

#if wxUSE_MENUS
    // set/get search button menu.
    // ---------------------------
    virtual void SetMenu(wxMenu *menu) wxOVERRIDE;
    virtual wxMenu * GetMenu() wxOVERRIDE;
#endif // wxUSE_MENUS

    // set/get search options.
    bool HasMenu() const;
    virtual void ShowSearchButton(bool show) wxOVERRIDE;
    virtual bool IsSearchButtonVisible() const wxOVERRIDE;
    virtual void ShowCancelButton(bool show) wxOVERRIDE;
    virtual bool IsCancelButtonVisible() const wxOVERRIDE;
    virtual void SetDescriptiveText(const wxString& strText) wxOVERRIDE;
    virtual wxString GetDescriptiveText() const wxOVERRIDE;

    // set custom search/cancel bitmaps
    void SetSearchBitmap(const wxBitmap& bitmap);
    void SetCancelBitmap(const wxBitmap& bitmap);

#if wxUSE_MENUS
    void SetSearchMenuBitmap(const wxBitmap& bitmap);
#endif // wxUSE_MENUS

    // wxWindow overrides
    virtual bool SetFont(const wxFont& font) wxOVERRIDE;
    virtual bool SetBackgroundColour(const wxColour& colour) wxOVERRIDE;
    virtual bool SetForegroundColour(const wxColour& colour) wxOVERRIDE;

// search/cancel button states.
private:
    struct NcButton
    {
        NcButton()
        {
            m_bitmap = wxNullBitmap;
            m_customBitmap = wxNullBitmap;
            m_hasCustomBitmap = false;
            m_visible = false;
            m_leftDown = false;
            m_state = State_Normal;
        }

        virtual ~NcButton() {}

        enum State
        {
            State_Normal,
            State_Current, // a.k.a. "hot" or "hovering".
            State_Pressed, // a.k.a. "selected" in public API for some reason.
            State_Disabled,
            State_Max
        };

        wxBitmap m_bitmap;
        wxBitmap m_customBitmap;
        bool m_hasCustomBitmap;
        bool m_visible;
        bool m_leftDown;
        State m_state;
    };

    struct NcMenuButton : NcButton
    {
        NcMenuButton() : NcButton()
        {
#if wxUSE_MENUS
            m_bmpMenu = wxNullBitmap;
            m_customMenuBitmap = wxNullBitmap;
            m_hasCustomMenuBitmap = false;
            m_menu = NULL;
#endif // wxUSE_MENUS
        }

        virtual ~NcMenuButton() wxOVERRIDE
        {
#if wxUSE_MENUS
            delete m_menu;
#endif // wxUSE_MENUS
        }

#if wxUSE_MENUS
        wxBitmap m_bmpMenu;
        wxBitmap m_customMenuBitmap;
        bool m_hasCustomMenuBitmap;
        wxMenu *m_menu;
#endif // wxUSE_MENUS
    };

protected:
    void RefreshSearchButton();
    void RefreshCancelButton();
    wxBitmap DrawSearchBitmap(const int height, const bool hasDrop);
    wxBitmap DrawCancelBitmap(const int height);

    // events.
    void OnCancelButton(wxCommandEvent& event);
    void OnTextEnter(wxCommandEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnDPIChanged(wxDPIChangedEvent& event);
    void OnDisplayChanged(wxDisplayChangedEvent& event);
    void OnSysColourChanged(wxSysColourChangedEvent& event);

    // intercept WM_XXX and EM_SETREADONLY messages.
    virtual bool MSWHandleMessage(WXLRESULT *result, WXUINT message,
                                  WXWPARAM wParam, WXLPARAM lParam) wxOVERRIDE;

private:
    // force the edit control to update its non-client area by sending the
    // WM_NCCALCSIZE message.
    void RefreshNc();

    int GetMultiplier() const;
    wxColour ClearBg(wxDC& dc);

    // get the search button rect of the non-client area in client area coordinates.
    RECT GetNcSearchButtonRect() const;

    // get the cancel button rect of the non-client area in client area coordinates.
    RECT GetNcCancelButtonRect() const;

    // generate an event of the given type for the search/cancel button clicks.
    void SendNcLeftUpEvent(wxEventType eventType);

#if wxUSE_MENUS
    void PopupSearchMenu();
#endif // wxUSE_MENUS

private:
    NcMenuButton m_searchButton;
    NcButton m_cancelButton;
    bool m_mouseTracking;

private:
    wxDECLARE_DYNAMIC_CLASS(wxSearchCtrl);
    wxDECLARE_EVENT_TABLE();
};

#endif // wxUSE_SEARCHCTRL

#endif // _WX_MSW_SEARCHCTRL_H_
