//-----------------------------------------------------------------------------
// Copyright 2021 Eight Brains Studios, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "Win32Window.h"

#include "../Application.h"
#include "../Events.h"
#include "Win32Application.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>

// Trick to get us our HINSTANCE 
// See https://devblogs.microsoft.com/oldnewthing/20041025-00/?p=37483
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

namespace {
    
static const int kDefaultPos = -10000;

}  // namespace

namespace uitk {

LRESULT CALLBACK UITKWndProc(HWND hwnd, UINT uMsg,
                             WPARAM wParam, LPARAM lParam);
                             
struct Win32Window::Impl {
    static const char *wndclass;

    IWindowCallbacks& callbacks;
    HWND hwnd = 0;
    std::string title;
    std::shared_ptr<DrawContext> dc;
    bool needsLayout = true;

    void updateDrawContext()
    {
        RECT r;
        GetClientRect(hwnd, &r);
        int width = r.right - r.left;
        int height = r.bottom - r.top;
        float dpi = float(GetDpiForWindow(hwnd));
        if (dpi == 0.0) {  // only happens if invalid window
            dpi = 96.0;
        }

        // Make sure we reset() the pointer to ensure that the previous
        // context is actually disposed of, otherwise we get an access
        // denied error creating another one.
        this->dc.reset();
        this->dc = DrawContext::fromHwnd(this->hwnd, width, height, dpi);
    }
};
const char *Win32Window::Impl::wndclass = nullptr;

Win32Window::Win32Window(IWindowCallbacks& callbacks,
                         const std::string& title, int width, int height,
                         Window::Flags::Value flags)
    : Win32Window(callbacks, title, kDefaultPos, kDefaultPos, width, height, flags)
{
}

Win32Window::Win32Window(IWindowCallbacks& callbacks,
                         const std::string& title, int x, int y,
                         int width, int height,
                         Window::Flags::Value flags)
    : mImpl(new Impl{callbacks})
{
    if (!Impl::wndclass) {
        Impl::wndclass = "UITK_window";
        WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
        wcex.style         = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc   = UITKWndProc;
        wcex.cbClsExtra    = 0;
        wcex.cbWndExtra    = sizeof(LONG_PTR);
        wcex.hInstance     = HINST_THISCOMPONENT;
        wcex.hbrBackground = NULL;
        wcex.lpszMenuName  = NULL;
        wcex.hCursor       = LoadCursor(NULL, IDI_APPLICATION);
        wcex.lpszClassName = Impl::wndclass;
        RegisterClassEx(&wcex);
    }

    if (x == kDefaultPos) {
        x = CW_USEDEFAULT;
    }
    if (y == kDefaultPos) {
        y = CW_USEDEFAULT;
    }
    mImpl->hwnd = CreateWindow(Impl::wndclass,
                               "",
                               WS_OVERLAPPEDWINDOW,
                               x, y,
                               width, height,
                               NULL,  // parent
                               NULL,  // menu
                               HINST_THISCOMPONENT,
                               this); // passed to WM_CREATE message, useful for a this pointer
    auto &win32app = static_cast<Win32Application&>(Application::instance().osApplication());
    win32app.registerWindow(mImpl->hwnd, this);
    mImpl->updateDrawContext();
    setTitle(title);
    if (!(flags & 1)) {
        show(true);
    }
}

Win32Window::~Win32Window()
{
    if (mImpl->hwnd) {
        close();
    }
}

bool Win32Window::isShowing() const
{
    return IsWindowVisible(mImpl->hwnd);
}

void Win32Window::show(bool show)
{
    if (show) {
        ShowWindow(mImpl->hwnd, SW_SHOWNORMAL);  // show and activate
    } else {
        ShowWindow(mImpl->hwnd, SW_HIDE);
    }
}

void Win32Window::close()
{
    if (mImpl->hwnd) {
        mImpl->dc = nullptr;  // release
        DestroyWindow(mImpl->hwnd);
        mImpl->hwnd = 0;
    }
}

void Win32Window::setTitle(const std::string& title)
{
    const int kNullTerminated = -1;
    int nCharsNeeded = MultiByteToWideChar(CP_UTF8, 0, title.c_str(),
                                           kNullTerminated, NULL, 0);
    WCHAR *wtitle = new WCHAR[nCharsNeeded + 1];  // nCharsNeeded includes \0, but +1 just in case
    wtitle[0] = '\0';  // in case conversion fails
    MultiByteToWideChar(CP_UTF8, 0, title.c_str(), kNullTerminated, wtitle, nCharsNeeded);

    SetWindowTextW(mImpl->hwnd, wtitle);
    mImpl->title = title;

    delete [] wtitle;
}

Rect Win32Window::contentRect() const
{
    RECT rectPx;
    GetClientRect(mImpl->hwnd, &rectPx);
    return Rect(PicaPt::kZero, PicaPt::kZero,
                PicaPt::fromPixels(float(rectPx.right - rectPx.left), mImpl->dc->dpi()),
                PicaPt::fromPixels(float(rectPx.bottom - rectPx.top), mImpl->dc->dpi()));
}

void Win32Window::postRedraw() const
{
    InvalidateRect(mImpl->hwnd, NULL, TRUE /*erase background*/);
}

void Win32Window::raiseToTop() const
{
    ShowWindow(mImpl->hwnd, SW_SHOWNORMAL);
}

void* Win32Window::nativeHandle() { return mImpl->hwnd; }

IWindowCallbacks& Win32Window::callbacks() { return mImpl->callbacks; }

void Win32Window::onMoved()
{
    if (!mImpl->dc) {
        return;
    }

    float oldDPI = mImpl->dc->dpi();
    mImpl->updateDrawContext();
    if (mImpl->dc->dpi() != oldDPI) {
        mImpl->needsLayout = true;
    }
}

void Win32Window::onResize()
{
    mImpl->updateDrawContext();
    // We are probably going to get a series of resize messages in a row,
    // so defer the layout until we actually try to draw.
    mImpl->needsLayout = true;
}

void Win32Window::onLayout()
{
    mImpl->callbacks.onLayout(*mImpl->dc);
    mImpl->needsLayout = false;
}

void Win32Window::onDraw()
{
    if (mImpl->needsLayout) {
        onLayout();
    }
    mImpl->callbacks.onDraw(*mImpl->dc);
}

void Win32Window::onMouse(MouseEvent& e, int x, int y)
{
    e.pos = Point(PicaPt::fromPixels(float(x), mImpl->dc->dpi()),
                  PicaPt::fromPixels(float(y), mImpl->dc->dpi()));
    mImpl->callbacks.onMouse(e);
}

void Win32Window::onActivated(const Point& currentMousePos)
{
}

void Win32Window::onDeactivated()
{
}

int getKeymods(WPARAM wParam)
{
    int keymods = 0;
    if (wParam & MK_SHIFT) { keymods |= KeyModifier::kShift; }
    if (wParam & MK_CONTROL) { keymods |= KeyModifier::kCtrl; }
    return keymods;
}

int getButtons(WPARAM wParam)
{
    int buttons = 0;
    if (wParam & MK_LBUTTON) { buttons |= int(MouseButton::kLeft); }
    if (wParam & MK_MBUTTON) { buttons |= int(MouseButton::kMiddle); }
    if (wParam & MK_RBUTTON) { buttons |= int(MouseButton::kRight); }
    if (wParam & MK_XBUTTON1) { buttons |= int(MouseButton::kButton4); }
    if (wParam & MK_XBUTTON2) { buttons |= int(MouseButton::kButton5); }
    return buttons;
}

MouseButton getXButton(WPARAM wParam)
{
    if (GET_XBUTTON_WPARAM(wParam) & XBUTTON1) { return MouseButton::kButton4; }
    if (GET_XBUTTON_WPARAM(wParam) & XBUTTON2) { return MouseButton::kButton5; }
    return MouseButton::kNone;
}

MouseEvent makeMouseEvent(MouseEvent::Type type, MouseButton b, int nClicks, WPARAM wParam)
{
    int buttons = getButtons(wParam);

    MouseEvent e;
    e.type = type;
    if (type == MouseEvent::Type::kMove && buttons != 0) {
        e.type = MouseEvent::Type::kDrag;
        e.drag.buttons = buttons;
    }
    if (type == MouseEvent::Type::kButtonDown || type == MouseEvent::Type::kButtonUp) {
        e.button.button = b;
    }
    if (type == MouseEvent::Type::kButtonDown) {
        e.button.nClicks = nClicks;
    }
    if (type == MouseEvent::Type::kScroll) {
        e.scroll.dx = PicaPt::kZero;
        // The Microsoft docs say that the wheel amounts are in units of 120 (WHEEL_PARAM).
        // They specifically say that this allows for finer-grained click amounts.
        // We want one normal mouse wheel to result in 1.0, which will then be converted
        // to the number of lines scrolled in Window::onMouse().
        e.scroll.dy = PicaPt(float(GET_WHEEL_DELTA_WPARAM(wParam)) / float(WHEEL_DELTA));
    }
    e.keymods = getKeymods(wParam);
    return e;
}

LRESULT CALLBACK UITKWndProc(HWND hwnd, UINT message,
                             WPARAM wParam, LPARAM lParam)
{
    Win32Window *w = (Win32Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (message) {
        case WM_CREATE: {
            auto *create = (CREATESTRUCT*)lParam;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)create->lpCreateParams);
            return 0;  // 0 to continue creation, -1 to cancel
        }
        case WM_DESTROY: {
            auto& win32app = static_cast<Win32Application&>(Application::instance().osApplication());
            win32app.unregisterWindow(hwnd);
            return 0;
        }
        case WM_PAINT:
            w->onDraw();
            return 0;
        case WM_SIZING: {  // WM_SIZE is sent when done, but we want live resizing
            auto *rectPx = (RECT*)lParam;  // can change this to alter the changing size
            w->onResize();
            return TRUE;
        }
        case WM_MOVE:
            w->onMoved();  // in case changed screens
            return 0;
        case WM_DISPLAYCHANGE:  // display resolution changed
            w->onResize();
            return 0;  // docs do not say what should be returned
        case WM_MOUSEMOVE:
            w->onMouse(makeMouseEvent(MouseEvent::Type::kMove,
                                      MouseButton::kNone, 0, wParam),
                       GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_LBUTTONDOWN:
            w->onMouse(makeMouseEvent(MouseEvent::Type::kButtonDown,
                                      MouseButton::kLeft, 1, wParam),
                       GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_LBUTTONDBLCLK:
            w->onMouse(makeMouseEvent(MouseEvent::Type::kButtonDown,
                                      MouseButton::kLeft, 2, wParam),
                       GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_LBUTTONUP:
            w->onMouse(makeMouseEvent(MouseEvent::Type::kButtonUp,
                                      MouseButton::kLeft, 0, wParam),
                       GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_MBUTTONDOWN:
            w->onMouse(makeMouseEvent(MouseEvent::Type::kButtonDown,
                                      MouseButton::kMiddle, 1, wParam),
                       GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_MBUTTONDBLCLK:
            w->onMouse(makeMouseEvent(MouseEvent::Type::kButtonDown,
                                      MouseButton::kMiddle, 2, wParam),
                       GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_MBUTTONUP:
            w->onMouse(makeMouseEvent(MouseEvent::Type::kButtonUp,
                                      MouseButton::kMiddle, 0, wParam),
                       GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_RBUTTONDOWN:
            w->onMouse(makeMouseEvent(MouseEvent::Type::kButtonDown,
                                      MouseButton::kRight, 1, wParam),
                       GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_RBUTTONDBLCLK:
            w->onMouse(makeMouseEvent(MouseEvent::Type::kButtonDown,
                                      MouseButton::kRight, 2, wParam),
                       GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_RBUTTONUP:
            w->onMouse(makeMouseEvent(MouseEvent::Type::kButtonUp,
                                      MouseButton::kRight, 0, wParam),
                       GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_XBUTTONDOWN:
            w->onMouse(makeMouseEvent(MouseEvent::Type::kButtonDown,
                                      getXButton(wParam), 1, wParam),
                       GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_XBUTTONDBLCLK:
            w->onMouse(makeMouseEvent(MouseEvent::Type::kButtonDown,
                                      getXButton(wParam), 2, wParam),
                       GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_XBUTTONUP:
            w->onMouse(makeMouseEvent(MouseEvent::Type::kButtonUp,
                                      getXButton(wParam), 0, wParam),
                       GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_MOUSEWHEEL:
            w->onMouse(makeMouseEvent(MouseEvent::Type::kScroll,
                                      MouseButton::kNone, 0, wParam),
                                      GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_DWMCOLORIZATIONCOLORCHANGED:
            Application::instance().onSystemThemeChanged();
            return 0;
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
}

}  // namespace uitk
