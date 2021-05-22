///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/srchctrl.cpp
// Purpose:     wxSearchCtrl class implementation for Win32
// Author:      Ali Kettab
// Modified by: Youcef Kouchkar (a.k.a. Tomay)
// Created:     2021-05-01
// Copyright:   Ali Kettab
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_SEARCHCTRL

#include "wx/srchctrl.h"

#ifndef WX_PRECOMP
    #include "wx/dcmemory.h"
#endif // WX_PRECOMP

#include "wx/display.h"
#include "wx/msw/dc.h"
#include <windowsx.h> // needed by GET_X_LPARAM and GET_Y_LPARAM macros.

#if wxUSE_UXTHEME
   #include "wx/msw/uxtheme.h"
#endif // wxUSE_UXTHEME

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// arguments to wxColour::ChangeLightness() for making the search/cancel bitmaps
// foreground colour, respectively.
static const int SEARCH_BITMAP_LIGHTNESS = 130; // slightly lighter.
static const int CANCEL_BITMAP_LIGHTNESS = 150; // a bit more lighter.

wxBEGIN_EVENT_TABLE(wxSearchCtrl, wxSearchCtrlBase)
    EVT_SEARCH_CANCEL(wxID_ANY, wxSearchCtrl::OnCancelButton)
    EVT_TEXT_ENTER(wxID_ANY, wxSearchCtrl::OnTextEnter)
    EVT_SIZE(wxSearchCtrl::OnSize)
    EVT_DPI_CHANGED(wxSearchCtrl::OnDPIChanged)
    EVT_DISPLAY_CHANGED(wxSearchCtrl::OnDisplayChanged)
    EVT_SYS_COLOUR_CHANGED(wxSearchCtrl::OnSysColourChanged)
wxEND_EVENT_TABLE()

wxIMPLEMENT_DYNAMIC_CLASS(wxSearchCtrl, wxSearchCtrlBase);

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxSearchCtrl creation
// ----------------------------------------------------------------------------

// creation
// --------

wxSearchCtrl::wxSearchCtrl(wxWindow *parent,
                           wxWindowID id,
                           const wxString& value,
                           const wxPoint& pos,
                           const wxSize& size,
                           const long style,
                           const wxValidator& validator,
                           const wxString& name)
{
    m_mouseTracking = false;
    Create(parent, id, value, pos, size, style, validator, name);
}

bool wxSearchCtrl::Create(wxWindow *parent,
                          wxWindowID id,
                          const wxString& value,
                          const wxPoint& pos,
                          const wxSize& size,
                          const long style,
                          const wxValidator& validator,
                          const wxString& name)
{
    if ( !wxSearchCtrlBase::Create(parent, id, value, pos, size,
                                   (style | wxTE_PROCESS_ENTER) &
                                   ~(wxHSCROLL | wxVSCROLL), validator, name) )
        return false;

    SetHint(_("Search"));
    m_searchButton.m_visible = true;
    RefreshSearchButton();
    RefreshNc();

    return true;
}

// search control specific interfaces.
#if wxUSE_MENUS
void wxSearchCtrl::SetMenu(wxMenu *menu)
{
    if ( menu == m_searchButton.m_menu )
        return;

    bool hadMenu = m_searchButton.m_menu;
    delete m_searchButton.m_menu;
    m_searchButton.m_menu = menu;

    // only the menu that has been changed.
    if ( m_searchButton.m_menu && hadMenu )
        return;

    if ( !m_searchButton.m_visible )
        m_searchButton.m_visible = true;

    RefreshSearchButton();
    RefreshNc();
}

wxMenu * wxSearchCtrl::GetMenu()
{
    return m_searchButton.m_menu;
}
#endif // wxUSE_MENUS

bool wxSearchCtrl::HasMenu() const
{
#if wxUSE_MENUS
    return m_searchButton.m_menu;
#else // !wxUSE_MENUS
    return false;
#endif // wxUSE_MENUS
}

void wxSearchCtrl::ShowSearchButton(bool show)
{
    if ( show == m_searchButton.m_visible )
        return;

    // only hide the button if we don't need it for the menu, otherwise it needs
    // to remain shown.
    if ( m_searchButton.m_visible && m_searchButton.m_menu )
        return;

    m_searchButton.m_visible = show;
    RefreshSearchButton();
    RefreshNc();
}

bool wxSearchCtrl::IsSearchButtonVisible() const
{
    return m_searchButton.m_visible;
}

void wxSearchCtrl::ShowCancelButton(bool show)
{
    if ( show == m_cancelButton.m_visible )
        return;

    m_cancelButton.m_visible = show;
    RefreshCancelButton();
    RefreshNc();
}

bool wxSearchCtrl::IsCancelButtonVisible() const
{
    return m_cancelButton.m_visible;
}

void wxSearchCtrl::SetDescriptiveText(const wxString& strText)
{
    SetHint(strText);
}

wxString wxSearchCtrl::GetDescriptiveText() const
{
    return GetHint();
}

// operations
// ----------

void wxSearchCtrl::RefreshNc()
{
    // Usually all windows get WM_NCCALCSIZE when their size is set, however if
    // the initial size is fixed, it's not going to change and this message won't
    // be sent at all, meaning that we won't get a chance to tell Windows that we
    // need extra space for our custom left/right borders, when using search/cancel
    // buttons. So force sending this message by using SWP_FRAMECHANGED (and use
    // SWP_NOXXX to avoid doing anything else).
    ::SetWindowPos(GetHwnd(), NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE |
                   SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

// set a custom search bitmap.
void wxSearchCtrl::SetSearchBitmap(const wxBitmap& bitmap)
{
    m_searchButton.m_customBitmap = bitmap;
    m_searchButton.m_hasCustomBitmap = bitmap.IsOk();
    RefreshSearchButton();
    RefreshNc();
}

// set a custom cancel bitmap.
void wxSearchCtrl::SetCancelBitmap(const wxBitmap& bitmap)
{
    m_cancelButton.m_customBitmap = bitmap;
    m_cancelButton.m_hasCustomBitmap = bitmap.IsOk();
    RefreshCancelButton();
    RefreshNc();
}

// set a custom search-menu bitmap.
#if wxUSE_MENUS
void wxSearchCtrl::SetSearchMenuBitmap(const wxBitmap& bitmap)
{
    m_searchButton.m_customMenuBitmap = bitmap;
    m_searchButton.m_hasCustomMenuBitmap = bitmap.IsOk();
    RefreshSearchButton();
    RefreshNc();
}
#endif // wxUSE_MENUS

// wxWindow overrides
//-------------------

bool wxSearchCtrl::SetFont(const wxFont& font)
{
    if ( !wxSearchCtrlBase::SetFont(font) )
        return false;

    // recreate the bitmaps as their size may have changed.
    RefreshSearchButton();
    RefreshCancelButton();
    RefreshNc();

    return true;
}

bool wxSearchCtrl::SetBackgroundColour(const wxColour& colour)
{
    if ( !wxSearchCtrlBase::SetBackgroundColour(colour) )
        return false;

    // when the background colour changes, then redraw the bitmaps so that the
    // correct colour shows in their "transparent" area.
    RefreshSearchButton();
    RefreshCancelButton();
    RefreshNc();

    return true;
}

bool wxSearchCtrl::SetForegroundColour(const wxColour& colour)
{
    if ( !wxSearchCtrlBase::SetForegroundColour(colour) )
        return false;

    // when the foreground colour changes, then redraw the bitmaps so that the
    // correct colour shows in their "transparent" area.
    RefreshSearchButton();
    RefreshCancelButton();
    RefreshNc();

    return true;
}

static void RescaleBitmap(wxBitmap& bitmap, const wxSize& size)
{
    wxCHECK_RET(size.IsFullySpecified(), wxS("a new size must be given."));

#if wxUSE_IMAGE
    wxImage image = bitmap.ConvertToImage();
    image.Rescale(size.x, size.y);
    bitmap = image;
#else // !wxUSE_IMAGE
   // fallback method of scaling the bitmap.
   wxBitmap bmpNew(size, bitmap.GetDepth());
   bmpNew.UseAlpha(bitmap.HasAlpha());
   {
        wxMemoryDC dc(bmpNew);
        double sclX = (double)size.x / bitmap.GetWidth();
        double sclY = (double)size.y / bitmap.GetHeight();
        dc.SetUserScale(sclX, sclY);
        dc.DrawBitmap(bitmap, 0, 0);
   }
   bitmap = bmpNew;
#endif // wxUSE_IMAGE
}

void wxSearchCtrl::RefreshSearchButton()
{
    wxSize size = GetClientSize();
    int bmpWidth = size.y;

    if ( bmpWidth <= 0 )
        return;

    // this is a custom search bitmap.
    if ( m_searchButton.m_hasCustomBitmap && m_searchButton.m_customBitmap.IsOk() )
    {
        wxBitmap bitmap(bmpWidth, bmpWidth);
        wxMemoryDC dcMem;
        dcMem.SelectObject(bitmap);

        // clear the background.
        ClearBg(dcMem);

        dcMem.SelectObject(wxNullBitmap);
        wxImage customImage = m_searchButton.m_customBitmap.ConvertToImage();

        switch ( m_searchButton.m_state )
        {
            // NcButton::State_Normal, NcButton::State_Disabled or NcButton::State_Max
            default:
                break;

            case NcButton::State_Current:
                customImage.ChangeHSV(0, 0.2, 0.2);
                break;

            case NcButton::State_Pressed:
                customImage.ChangeHSV(0, 0.2, -0.2);
                break;
        }

        // disabled.
        if ( m_searchButton.m_state == NcButton::State_Disabled )
        {
            customImage = customImage.ConvertToGreyscale();
            customImage = customImage.ConvertToDisabled();
        }

        wxImage image = bitmap.ConvertToImage();
        image.Paste(customImage.Scale(bmpWidth, bmpWidth, wxIMAGE_QUALITY_HIGH),
                    0, 0, wxIMAGE_ALPHA_BLEND_COMPOSE);
        m_searchButton.m_bitmap = image;
    }
    else if ( !m_searchButton.m_menu )
        m_searchButton.m_bitmap = DrawSearchBitmap(bmpWidth, false);

#if wxUSE_MENUS
    // this is a custom search menu bitmap.
    if ( m_searchButton.m_hasCustomMenuBitmap && m_searchButton.m_customMenuBitmap.IsOk() )
    {
        wxBitmap bitmap(bmpWidth, bmpWidth);
        wxMemoryDC dcMem;
        dcMem.SelectObject(bitmap);

        // clear the background.
        ClearBg(dcMem);

        dcMem.SelectObject(wxNullBitmap);
        wxImage customImage = m_searchButton.m_customMenuBitmap.ConvertToImage();

        switch ( m_searchButton.m_state )
        {
            // NcButton::State_Normal, NcButton::State_Disabled or NcButton::State_Max
            default:
                break;

            case NcButton::State_Current:
                customImage.ChangeHSV(0, 0.2, 0.2);
                break;

            case NcButton::State_Pressed:
                customImage.ChangeHSV(0, 0.2, -0.2);
                break;
        }

        // disabled.
        if ( m_searchButton.m_state == NcButton::State_Disabled )
        {
            customImage = customImage.ConvertToGreyscale();
            customImage = customImage.ConvertToDisabled();
        }

        wxImage image = bitmap.ConvertToImage();
        image.Paste(customImage.Scale(bmpWidth, bmpWidth, wxIMAGE_QUALITY_HIGH),
                    0, 0, wxIMAGE_ALPHA_BLEND_COMPOSE);
        m_searchButton.m_bmpMenu = image;
    }
    else if ( m_searchButton.m_menu )
        m_searchButton.m_bmpMenu = DrawSearchBitmap(bmpWidth, true);
#endif // wxUSE_MENUS
}

void wxSearchCtrl::RefreshCancelButton()
{
    wxSize size = GetClientSize();
    int bmpWidth = size.y;

    if ( bmpWidth <= 0 )
        return;

    // this is a custom cancel bitmap.
    if ( m_cancelButton.m_hasCustomBitmap && m_cancelButton.m_customBitmap.IsOk() )
    {
        wxBitmap bitmap(bmpWidth, bmpWidth);
        wxMemoryDC dcMem;
        dcMem.SelectObject(bitmap);

        // clear the background.
        ClearBg(dcMem);

        dcMem.SelectObject(wxNullBitmap);
        wxImage customImage = m_cancelButton.m_customBitmap.ConvertToImage();

        switch ( m_cancelButton.m_state )
        {
            // NcButton::State_Normal, NcButton::State_Disabled or NcButton::State_Max
            default:
                break;

            case NcButton::State_Current:
                customImage.ChangeHSV(0, 0.2, 0.2);
                break;

            case NcButton::State_Pressed:
                customImage.ChangeHSV(0, 0.2, -0.2);
                break;
        }

        // disabled.
        if ( m_cancelButton.m_state == NcButton::State_Disabled )
        {
            customImage = customImage.ConvertToGreyscale();
            customImage = customImage.ConvertToDisabled();
        }

        wxImage image = bitmap.ConvertToImage();
        image.Paste(customImage.Scale(bmpWidth, bmpWidth, wxIMAGE_QUALITY_HIGH),
                    0, 0, wxIMAGE_ALPHA_BLEND_COMPOSE);
        m_cancelButton.m_bitmap = image;
    }
    else
        m_cancelButton.m_bitmap = DrawCancelBitmap(bmpWidth);
}

// icons are rendered at 6-8 times larger than necessary and downscaled for
// antialiasing.
int wxSearchCtrl::GetMultiplier() const
{
    int depth = wxDisplay(this).GetDepth();

    if ( depth < 24 )
        return 6;

    return 8;
}

wxColour wxSearchCtrl::ClearBg(wxDC& dc)
{
    wxColour clrBg;

    // disabled or read-only.
    if ( !IsEnabled() || (!IsEditable() && !m_hasBgCol && !m_hasFgCol) )
        clrBg = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
    else if ( m_hasBgCol ) // we have a custom background colour.
        clrBg = wxSearchCtrlBase::GetBackgroundColour();
    else // otherwise, get the appropriate theme color.
        clrBg = MSWGetThemeColour(L"EDIT", EP_EDITTEXT, ETS_NORMAL,
                                  ThemeColourBackground, wxSYS_COLOUR_WINDOW);

    dc.SetBrush(clrBg);
    dc.SetPen(clrBg);
    dc.Clear();

    return clrBg;
}

wxBitmap wxSearchCtrl::DrawSearchBitmap(const int height, const bool hasDrop)
{
    wxColour clrFg;

    switch ( m_searchButton.m_state )
    {
        // NcButton::State_Normal, NcButton::State_Disabled or NcButton::State_Max
        default:
            clrFg = GetForegroundColour().ChangeLightness(SEARCH_BITMAP_LIGHTNESS);
            break;

        case NcButton::State_Current:
            clrFg = GetForegroundColour().ChangeLightness(SEARCH_BITMAP_LIGHTNESS + 10);
            break;

        case NcButton::State_Pressed:
            clrFg = GetForegroundColour().ChangeLightness(SEARCH_BITMAP_LIGHTNESS - 10);
            break;
    }

    // disabled.
    if ( m_searchButton.m_state == NcButton::State_Disabled )
        clrFg.MakeDisabled();

    // ------------------------------------------------------------------------
    // begin drawing code.
    // ------------------------------------------------------------------------

    // total size is (16|20)x16.
    // glass, 12x12, in the top-left corner.
    // handle, (10,10)-(16,16).
    // drop, (13,6)-(19,6)-(16,9).

    int multiplier = GetMultiplier();
    int width = hasDrop ? height * 20 / 16. : height;
    int penWidth = height * multiplier / 16.;
    wxBitmap bitmap(width * multiplier, height * multiplier);
    wxMemoryDC dcMem;
    dcMem.SelectObject(bitmap);

    // clear the background.
    wxColour clrBg = ClearBg(dcMem);

    // draw the glass.
    dcMem.SetBrush(clrFg);
    dcMem.SetPen(clrFg);
    int radius = 6 * penWidth;
    dcMem.DrawCircle(radius, radius, radius);
    dcMem.SetBrush(clrBg);
    dcMem.SetPen(clrBg);
    dcMem.DrawCircle(radius, radius, radius - penWidth * 2);

    // draw the handle.
    dcMem.SetBrush(clrFg);
    dcMem.SetPen(clrFg);
    int handleCornerShift = penWidth / sqrt(2);

    int lineStart = radius + radius / sqrt(2);
    int lineEnd = height * multiplier - lineStart - handleCornerShift;

    wxPoint handlePolygon[] =
    {
        wxPoint(-handleCornerShift, handleCornerShift),
        wxPoint(-handleCornerShift, -handleCornerShift),
        wxPoint(handleCornerShift, -handleCornerShift),
        wxPoint(lineEnd + handleCornerShift, lineEnd - handleCornerShift),
        wxPoint(lineEnd - handleCornerShift, lineEnd + handleCornerShift)
    };

    dcMem.DrawPolygon(WXSIZEOF(handlePolygon), handlePolygon, lineStart, lineStart);

    // draw the drop triangle.
    if ( hasDrop )
    {
        wxPoint dropPolygon[] =
        {
            // triangle left point.
            wxPoint(13 * penWidth, 6 * penWidth),
            // triangle right point.
            wxPoint(19 * penWidth, 6 * penWidth),
            // triangle bottom point.
            wxPoint(16 * penWidth, 9 * penWidth)
        };

        dcMem.DrawPolygon(WXSIZEOF(dropPolygon), dropPolygon);
    }

    dcMem.SelectObject(wxNullBitmap);

    // ------------------------------------------------------------------------
    // end drawing code.
    // ------------------------------------------------------------------------

    RescaleBitmap(bitmap, wxSize(width, height));

    return bitmap;
}

wxBitmap wxSearchCtrl::DrawCancelBitmap(const int height)
{
    wxColour clrFg;

    switch ( m_cancelButton.m_state )
    {
        // NcButton::State_Normal, NcButton::State_Disabled or NcButton::State_Max
        default:
            clrFg = GetForegroundColour().ChangeLightness(CANCEL_BITMAP_LIGHTNESS);
            break;

        case NcButton::State_Current:
            clrFg = GetForegroundColour().ChangeLightness(CANCEL_BITMAP_LIGHTNESS + 10);
            break;

        case NcButton::State_Pressed:
            clrFg = GetForegroundColour().ChangeLightness(CANCEL_BITMAP_LIGHTNESS - 10);
            break;
    }

    // disabled.
    if ( m_cancelButton.m_state == NcButton::State_Disabled )
        clrFg.MakeDisabled();

    // ------------------------------------------------------------------------
    // begin drawing code.
    // ------------------------------------------------------------------------

    // total size is 16x16.
    // circle, 15x15, in the center.
    // cross lines, (5,5)-(11,11) and (11,5)-(5,11).

    int multiplier = GetMultiplier();
    int penWidth = height * multiplier / 16.;
    wxBitmap bitmap(height * multiplier, height * multiplier);
    wxMemoryDC dcMem;
    dcMem.SelectObject(bitmap);

    // clear the background.
    wxColour clrBg = ClearBg(dcMem);

    // draw the circle.
    dcMem.SetBrush(clrFg);
    dcMem.SetPen(clrFg);
    int radius = height * multiplier / 2;
    dcMem.DrawCircle(radius, radius, radius - penWidth);

    // draw the cross.
    int lineStart = 5 * penWidth;
    int lineEnd = height * multiplier - lineStart * 2;
    dcMem.SetBrush(clrBg);
    dcMem.SetPen(clrBg);
    int crossCornerShift = penWidth / sqrt(2);

    wxPoint crossPolygon1[] =
    {
        wxPoint(-crossCornerShift, crossCornerShift),
        wxPoint(-crossCornerShift, -crossCornerShift),
        wxPoint(crossCornerShift, -crossCornerShift),
        wxPoint(lineEnd + crossCornerShift, lineEnd - crossCornerShift),
        wxPoint(lineEnd + crossCornerShift, lineEnd + crossCornerShift),
        wxPoint(lineEnd - crossCornerShift, lineEnd + crossCornerShift)
    };

    dcMem.DrawPolygon(WXSIZEOF(crossPolygon1), crossPolygon1, lineStart, lineStart);

    wxPoint crossPolygon2[] =
    {
        wxPoint(lineEnd - crossCornerShift, -crossCornerShift),
        wxPoint(lineEnd + crossCornerShift, -crossCornerShift),
        wxPoint(lineEnd + crossCornerShift, crossCornerShift),
        wxPoint(crossCornerShift, lineEnd + crossCornerShift),
        wxPoint(-crossCornerShift, lineEnd + crossCornerShift),
        wxPoint(-crossCornerShift, lineEnd - crossCornerShift)
    };

    dcMem.DrawPolygon(WXSIZEOF(crossPolygon2), crossPolygon2, lineStart, lineStart);

    // stop drawing on the bitmap before possibly calling RescaleBitmap() below.
    dcMem.SelectObject(wxNullBitmap);

    // ------------------------------------------------------------------------
    // end drawing code.
    // ------------------------------------------------------------------------

    RescaleBitmap(bitmap, wxSize(height, height));

    return bitmap;
}

// generate an event of the given type for the search/cancel button clicks.
void wxSearchCtrl::SendNcLeftUpEvent(wxEventType eventType)
{
    wxCommandEvent event(eventType, GetId());
    event.SetEventObject(this);

    if ( eventType == wxEVT_SEARCH )
        // it's convenient to have the string to search for directly in the event
        // instead of having to retrieve it from the control in the event handler
        // code later, so provide it here.
        event.SetString(GetValue());

    GetEventHandler()->ProcessEvent(event);
    SetFocus();

#if wxUSE_MENUS
    if ( eventType == wxEVT_SEARCH )
        // this happens automatically, just like on Mac OS X.
        PopupSearchMenu();
#endif // wxUSE_MENUS
}

void wxSearchCtrl::OnCancelButton(wxCommandEvent& event)
{
    Clear();
    event.Skip();
}

void wxSearchCtrl::OnTextEnter(wxCommandEvent& WXUNUSED(event))
{
    if ( !IsEmpty() )
        SendNcLeftUpEvent(wxEVT_SEARCH);
}

void wxSearchCtrl::OnSize(wxSizeEvent& event)
{
    RefreshSearchButton();
    RefreshCancelButton();
    RefreshNc();
    event.Skip();
}

void wxSearchCtrl::OnDPIChanged(wxDPIChangedEvent& event)
{
    RefreshSearchButton();
    RefreshCancelButton();
    RefreshNc();
    event.Skip();
}

void wxSearchCtrl::OnDisplayChanged(wxDisplayChangedEvent& event)
{
    RefreshSearchButton();
    RefreshCancelButton();
    RefreshNc();
    event.Skip();
}

void wxSearchCtrl::OnSysColourChanged(wxSysColourChangedEvent& event)
{
    RefreshSearchButton();
    RefreshCancelButton();
    RefreshNc();
    event.Skip();
}

// get the search button rect of the non-client area in client area coordinates.
RECT wxSearchCtrl::GetNcSearchButtonRect() const
{
    HWND hwnd = GetHwnd();
    RECT rcClient;
    RECT rcSearchButton;

    ::GetClientRect(hwnd, &rcClient);

    int buttonHeight = rcClient.bottom - rcClient.top;
    int buttonWidth = HasMenu() && !m_searchButton.m_hasCustomMenuBitmap ?
                      buttonHeight * 20 / 16. : buttonHeight;

    if ( GetParent()->GetLayoutDirection() == wxLayout_LeftToRight )
        rcSearchButton = { rcClient.left - buttonWidth, rcClient.top,
                           rcClient.left, rcClient.bottom };
    else // GetParent()->GetLayoutDirection() = wxLayout_RightToLeft
        rcSearchButton = { rcClient.right, rcClient.top,
                           rcClient.right + buttonWidth, rcClient.bottom };

    return rcSearchButton;
}

// get the cancel button rect of the non-client area in client area coordinates.
RECT wxSearchCtrl::GetNcCancelButtonRect() const
{
    HWND hwnd = GetHwnd();
    RECT rcClient;
    RECT rcCancelButton;

    ::GetClientRect(hwnd, &rcClient);

    int buttonHeight = rcClient.bottom - rcClient.top;

    if ( GetParent()->GetLayoutDirection() == wxLayout_LeftToRight )
        rcCancelButton = { rcClient.right, rcClient.top,
                           rcClient.right + buttonHeight, rcClient.bottom };
    else // GetParent()->GetLayoutDirection() = wxLayout_RightToLeft
        rcCancelButton = { rcClient.left - buttonHeight, rcClient.top,
                           rcClient.left, rcClient.bottom };

    return rcCancelButton;
}

bool wxSearchCtrl::MSWHandleMessage(WXLRESULT *result, WXUINT message,
                                    WXWPARAM wParam, WXLPARAM lParam)
{
    switch ( message )
    {
        case WM_NCCALCSIZE:
        {
            // ask the widget to calculate the border size.
            *result = MSWDefWindowProc(message, wParam, lParam);

            LPRECT pRect = reinterpret_cast<LPRECT>(lParam);
            int buttonHeight = pRect->bottom - pRect->top;

            if ( m_searchButton.m_visible )
            {
                int buttonWidth = HasMenu() &&
                                !m_searchButton.m_hasCustomMenuBitmap ?
                                buttonHeight * 20 / 16. : buttonHeight;

                if ( GetParent()->GetLayoutDirection() == wxLayout_LeftToRight )
                    pRect->left += buttonWidth;
                else // GetParent()->GetLayoutDirection() = wxLayout_RightToLeft
                    pRect->right -= buttonWidth;
            }

            if ( m_cancelButton.m_visible )
            {
                if ( GetParent()->GetLayoutDirection() == wxLayout_LeftToRight )
                    pRect->right -= buttonHeight;
                else // GetParent()->GetLayoutDirection() = wxLayout_RightToLeft
                    pRect->left += buttonHeight;
            }
        }
        return true;

        case WM_NCPAINT:
        {
            HWND hwnd = GetHwnd();
            HDC hdc = ::GetWindowDC(hwnd);
            RECT rcSearchButton = GetNcSearchButtonRect();
            RECT rcCancelButton = GetNcCancelButtonRect();
            HRGN hrgnClip = ::CreateRectRgn(0, 0, 0, 0);
            RECT rcWindow;

            ::GetWindowRect(hwnd, &rcWindow);

            // convert the screen coordinates of rcWindow to client area ones.
            ::MapWindowPoints(NULL, hwnd, reinterpret_cast<LPPOINT>(&rcWindow), 2);

            // calculate the clipping region.
            if ( m_searchButton.m_visible )
            {
                // convert the client area coordinates of rcSearchButton to logical
                // ones.
                ::OffsetRect(&rcSearchButton, -rcWindow.left, -rcWindow.top);

                ::LPtoDP(hdc, reinterpret_cast<LPPOINT>(&rcSearchButton), 2);
                HRGN hrgnSearchButton = ::CreateRectRgnIndirect(&rcSearchButton);
                ::CombineRgn(hrgnClip, hrgnClip, hrgnSearchButton, RGN_OR);
                ::DeleteObject(hrgnSearchButton);
            }

            if ( m_cancelButton.m_visible )
            {
                // convert the client area coordinates of rcCancelButton to logical
                // ones.
                ::OffsetRect(&rcCancelButton, -rcWindow.left, -rcWindow.top);

                ::LPtoDP(hdc, reinterpret_cast<LPPOINT>(&rcCancelButton), 2);
                HRGN hrgnCancelButton = ::CreateRectRgnIndirect(&rcCancelButton);
                ::CombineRgn(hrgnClip, hrgnClip, hrgnCancelButton, RGN_OR);
                ::DeleteObject(hrgnCancelButton);
            }

            // exclude the borders and the client area from the paint operation,
            // keeping only the calculated clipping region.
            // because otherwise, this would result in an annoying flicker.
            ::ExtSelectClipRgn(hdc, hrgnClip, RGN_AND);

            // clean up.
            ::DeleteObject(hrgnClip);

            // draw the search and cancel bitmaps.
            wxDCTemp dc(hdc);

            if ( m_searchButton.m_visible )
            {
                if ( !HasMenu() )
                {
                    if ( m_searchButton.m_bitmap.IsOk() )
                        dc.DrawBitmap(m_searchButton.m_bitmap,
                                      rcSearchButton.left, rcSearchButton.top);
                }
                else if ( m_searchButton.m_bmpMenu.IsOk() )
                    dc.DrawBitmap(m_searchButton.m_bmpMenu,
                                  rcSearchButton.left, rcSearchButton.top);
            }

            if ( m_cancelButton.m_visible && m_cancelButton.m_bitmap.IsOk() )
                dc.DrawBitmap(m_cancelButton.m_bitmap, rcCancelButton.left,
                              rcCancelButton.top);

            // clean up.
            ::ReleaseDC(hwnd, hdc);

            // then, ask the widget to paint its non-client area, such as the
            // frame, etc.
            *result = MSWDefWindowProc(message, wParam, lParam);
        }
        return true;

        case WM_NCHITTEST:
        {
            // get the screen coordinates of the mouse.
            POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

            // convert the screen coordinates of point to client area ones.
            ::MapWindowPoints(NULL, GetHwnd(), &point, 2);

            // get the position of the search and cancel buttons.
            RECT rcSearchButton = GetNcSearchButtonRect();
            RECT rcCancelButton = GetNcCancelButtonRect();

            // check that the mouse is within the search or cancel buttons.
            if ( (m_searchButton.m_visible && ::PtInRect(&rcSearchButton, point)) ||
                 (m_cancelButton.m_visible && ::PtInRect(&rcCancelButton, point)) )
                return HTBORDER;
        }
        break;

        case WM_NCLBUTTONDBLCLK:
        case WM_NCLBUTTONDOWN:
        {
            // get the screen coordinates of the mouse.
            POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

            // convert the screen coordinates of point to client area ones.
            ::MapWindowPoints(NULL, GetHwnd(), &point, 2);

            // get the position of the search and cancel buttons.
            RECT rcSearchButton = GetNcSearchButtonRect();
            RECT rcCancelButton = GetNcCancelButtonRect();

            // check that the mouse is within the search button.
            if ( m_searchButton.m_visible && ::PtInRect(&rcSearchButton, point) )
            {
                ::SetCapture(GetHwnd());
                m_searchButton.m_state = NcButton::State_Pressed;
                m_searchButton.m_leftDown = true;

                // redraw the non-client area to reflect the change.
                RefreshSearchButton();
                RefreshNc();
            }
            // check that the mouse is within the cancel button.
            else if ( m_cancelButton.m_visible && ::PtInRect(&rcCancelButton, point) )
            {
                ::SetCapture(GetHwnd());
                m_cancelButton.m_state = NcButton::State_Pressed;
                m_cancelButton.m_leftDown = true;

                // redraw the non-client area to reflect the change.
                RefreshCancelButton();
                RefreshNc();
            }
        }
        break;

        case WM_NCMOUSEMOVE:
        {
            // get the screen coordinates of the mouse.
            POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

            // convert the screen coordinates of point to client area ones.
            ::MapWindowPoints(NULL, GetHwnd(), &point, 2);

            bool refreshNc = false;

            if ( m_searchButton.m_visible )
            {
                NcButton::State oldState = m_searchButton.m_state;

                // get the position of the search button.
                RECT rcSearchButton = GetNcSearchButtonRect();

                // check that the mouse is within the search button.
                if ( ::PtInRect(&rcSearchButton, point) )
                    m_searchButton.m_state = NcButton::State_Current;
                else
                    m_searchButton.m_state = NcButton::State_Normal;

                if ( m_searchButton.m_state != oldState )
                {
                    RefreshSearchButton();
                    refreshNc = true;
                }
            }

            if ( m_cancelButton.m_visible )
            {
                NcButton::State oldState = m_cancelButton.m_state;

                // get the position of the cancel button.
                RECT rcCancelButton = GetNcCancelButtonRect();

                // check that the mouse is within the cancel button.
                if ( ::PtInRect(&rcCancelButton, point) )
                    m_cancelButton.m_state = NcButton::State_Current;
                else
                    m_cancelButton.m_state = NcButton::State_Normal;

                if ( m_cancelButton.m_state != oldState )
                {
                    RefreshCancelButton();
                    refreshNc = true;
                }
            }

            if ( !m_mouseTracking )
            {
                // enable mouse tracking.
                TRACKMOUSEEVENT tme;
                tme.cbSize = sizeof(tme);
                tme.hwndTrack = GetHwnd();
                tme.dwFlags = TME_NONCLIENT | TME_LEAVE;
                tme.dwHoverTime = HOVER_DEFAULT;
                ::TrackMouseEvent(&tme);
                m_mouseTracking = true;
            }

            // redraw the non-client area to reflect the change.
            // to prevent flicker, we only redraw the button if its state has
            // changed.
            if ( refreshNc )
                RefreshNc();
        }
        break;

        case WM_NCMOUSELEAVE:
        {
            bool refreshNc = false;

            if ( m_searchButton.m_visible )
            {
                NcButton::State oldState = m_searchButton.m_state;

                if ( IsEnabled() )
                    m_searchButton.m_state = NcButton::State_Normal;

                if ( m_searchButton.m_state != oldState )
                {
                    RefreshSearchButton();
                    refreshNc = true;
                }
            }

            if ( m_cancelButton.m_visible )
            {
                NcButton::State oldState = m_cancelButton.m_state;

                if ( IsEnabled() )
                    m_cancelButton.m_state = NcButton::State_Normal;

                if ( m_cancelButton.m_state != oldState )
                {
                    RefreshCancelButton();
                    refreshNc = true;
                }
            }

            m_mouseTracking = false;

            // redraw the non-client area to reflect the change.
            // to prevent flicker, we only redraw the button if its state has
            // changed.
            if ( refreshNc )
                RefreshNc();
        }
        break;

        case WM_MOUSEMOVE:
        {
            if ( !(wParam & MK_LBUTTON) )
                break;

            // get the screen coordinates of the mouse.
            POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

            bool refreshNc = false;

            if ( m_searchButton.m_visible )
            {
                NcButton::State oldState = m_searchButton.m_state;

                // get the position of the search button.
                RECT rcSearchButton = GetNcSearchButtonRect();

                // check that the mouse is within the search button.
                if ( ::PtInRect(&rcSearchButton, point) && m_searchButton.m_leftDown )
                    m_searchButton.m_state = NcButton::State_Pressed;
                else
                    m_searchButton.m_state = NcButton::State_Normal;

                if ( m_searchButton.m_state != oldState )
                {
                    RefreshSearchButton();
                    refreshNc = true;
                }
            }

            if ( m_cancelButton.m_visible )
            {
                NcButton::State oldState = m_cancelButton.m_state;

                // get the position of the cancel button.
                RECT rcCancelButton = GetNcCancelButtonRect();

                // check that the mouse is within the cancel button.
                if ( ::PtInRect(&rcCancelButton, point) && m_cancelButton.m_leftDown )
                    m_cancelButton.m_state = NcButton::State_Pressed;
                else
                    m_cancelButton.m_state = NcButton::State_Normal;

                if ( m_cancelButton.m_state != oldState )
                {
                    RefreshCancelButton();
                    refreshNc = true;
                }
            }

            // redraw the non-client area to reflect the change.
            // to prevent flicker, we only redraw the button if its state has
            // changed.
            if ( refreshNc )
                RefreshNc();
        }
        break;

        case WM_LBUTTONUP:
        {
            // get the screen coordinates of the mouse.
            POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

            if ( m_searchButton.m_visible && m_searchButton.m_leftDown )
            {
                // get the position of the search button.
                RECT rcSearchButton = GetNcSearchButtonRect();

                // check that the mouse is within the search button, and clicked.
                if ( ::PtInRect(&rcSearchButton, point) )
                {
                    m_searchButton.m_state = NcButton::State_Current;
                    SendNcLeftUpEvent(wxEVT_SEARCH);
                }
                else
                    m_searchButton.m_state = NcButton::State_Normal;

                m_searchButton.m_leftDown = false;
                ::ReleaseCapture();

                // redraw the non-client area to reflect the change.
                RefreshSearchButton();
                RefreshNc();
            }
            else if ( m_cancelButton.m_visible && m_cancelButton.m_leftDown )
            {
                // get the position of the cancel button.
                RECT rcCancelButton = GetNcCancelButtonRect();

                // check that the mouse is within the cancel button, and clicked.
                if ( ::PtInRect(&rcCancelButton, point) )
                {
                    m_cancelButton.m_state = NcButton::State_Current;
                    SendNcLeftUpEvent(wxEVT_SEARCH_CANCEL);
                }
                else
                    m_cancelButton.m_state = NcButton::State_Normal;

                m_cancelButton.m_leftDown = false;
                ::ReleaseCapture();

                // redraw the non-client area to reflect the change.
                RefreshCancelButton();
                RefreshNc();
            }
        }
        break;

        case WM_ENABLE:
        {
            // get the screen coordinates of the mouse.
            POINT point;
            wxGetCursorPosMSW(&point);

            // convert the screen coordinates of point to client area ones.
            ::MapWindowPoints(NULL, GetHwnd(), &point, 2);

            bool refreshNc = false;

            if ( m_searchButton.m_visible )
            {
                NcButton::State oldState = m_searchButton.m_state;

                // get the position of the search button.
                RECT rcSearchButton = GetNcSearchButtonRect();

                // disabled.
                if ( !wParam )
                    m_searchButton.m_state = NcButton::State_Disabled;
                else
                {
                    // check that the mouse is within the search button.
                    if ( ::PtInRect(&rcSearchButton, point) )
                        m_searchButton.m_state = NcButton::State_Current;
                    else
                        m_searchButton.m_state = NcButton::State_Normal;
                }

                if ( m_searchButton.m_state != oldState )
                {
                    RefreshSearchButton();
                    refreshNc = true;
                }
            }

            if ( m_cancelButton.m_visible )
            {
                NcButton::State oldState = m_cancelButton.m_state;

                // get the position of the cancel button.
                RECT rcCancelButton = GetNcCancelButtonRect();

                // disabled.
                if ( !wParam )
                    m_cancelButton.m_state = NcButton::State_Disabled;
                else
                {
                    // check that the mouse is within the cancel button.
                    if ( ::PtInRect(&rcCancelButton, point) )
                        m_cancelButton.m_state = NcButton::State_Current;
                    else
                        m_cancelButton.m_state = NcButton::State_Normal;
                }

                if ( m_cancelButton.m_state != oldState )
                {
                    RefreshCancelButton();
                    refreshNc = true;
                }
            }

            // redraw the non-client area to reflect the change.
            // to prevent flicker, we only redraw the button if its state has
            // changed.
            if ( refreshNc )
                RefreshNc();
        }
        break;

        case EM_SETREADONLY:
        {
            // ask the widget to set/remove the ES_READONLY style.
            *result = MSWDefWindowProc(message, wParam, lParam);

            if ( wParam == IsEditable() )
                break;

            bool refreshNc = false;

            if ( m_searchButton.m_visible )
            {
                RefreshSearchButton();
                refreshNc = true;
            }

            if ( m_cancelButton.m_visible )
            {
                RefreshCancelButton();
                refreshNc = true;
            }

            // redraw the non-client area to reflect the change.
            // to prevent flicker, we only redraw the button if its state has
            // changed.
            if ( refreshNc )
                RefreshNc();
        }
        return true;
    }

    return wxSearchCtrlBase::MSWHandleMessage(result, message, wParam, lParam);
}

#if wxUSE_MENUS
void wxSearchCtrl::PopupSearchMenu()
{
    if ( m_searchButton.m_menu )
        PopupMenu(m_searchButton.m_menu);
}
#endif // wxUSE_MENUS

#endif // wxUSE_SEARCHCTRL
