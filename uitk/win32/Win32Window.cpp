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
#include "../Cursor.h"
#include "../Events.h"
#include "../OSCursor.h"
#include "../TextEditorLogic.h"
#include "Win32Application.h"
#include "Win32Menubar.h"
#include "Win32Utils.h"

#include <unordered_map>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>

// Trick to get us our HINSTANCE 
// See https://devblogs.microsoft.com/oldnewthing/20041025-00/?p=37483
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

namespace uitk {

namespace {
    
static const int kDefaultPos = -10000;

static const std::unordered_map<int, Key> kVK2Key = {
    { VK_BACK, Key::kBackspace },
    { VK_TAB, Key::kTab },
    //{ VK_ENTER, Key::kEnter },
    { VK_RETURN, Key::kReturn },
    { VK_ESCAPE, Key::kEscape },
    { VK_SPACE, Key::kSpace },
    { VK_MULTIPLY, Key::kNumMultiply },
    { VK_ADD, Key::kNumPlus },
    { VK_OEM_COMMA, Key::kNumComma },
    { VK_SUBTRACT, Key::kNumMinus },
    { VK_DECIMAL, Key::kNumPeriod },
    { VK_DIVIDE, Key::kNumSlash },
    { VK_DELETE, Key::kDelete },
    //{ VK_LSHIFT, Key::kLeftShift },
    //{ VK_RSHIFT, Key::kRightShift },
    { VK_SHIFT, Key::kShift },
    { VK_CONTROL, Key::kCtrl },
    { VK_CAPITAL, Key::kCapsLock },
    { VK_NUMLOCK, Key::kNumLock },
    { VK_LEFT, Key::kLeft },
    { VK_RIGHT, Key::kRight },
    { VK_UP, Key::kUp },
    { VK_DOWN, Key::kDown },
    { VK_HOME, Key::kHome },
    { VK_END, Key::kEnd },
    { VK_PRIOR, Key::kPageUp },
    { VK_NEXT, Key::kPageDown },
    { VK_SNAPSHOT, Key::kPrintScreen },
};

}  // namespace

// See https://devblogs.microsoft.com/oldnewthing/20041018-00/?p=37543 for
// pitfalls in detecting double-clicks, triple-clicks, etc.
class ClickCounter
{
public:
    ClickCounter()
    {
        reset();
    }

    int nClicks() const { return mNClicks;  }

    void reset()
    {
        mButton = uitk::MouseButton::kNone;
        mNClicks = 0;
        mLastClickTime = 0;
    }

    int click(MouseButton button, LONG clickTime, int x, int y)
    {
        POINT pt = { x, y };

        // Raymond Chen and also the docs for GetMessageTime() recommend using now - last <= doubleClickTime
        // to detect double+ clicks across a timer wrap. It would seem like this would consider any click
        // after the wrap to be within the time limit. However, subtracting a large is adding -largeNumber
        // (which always valid, since INT_MAX is |INT_MIN - 1|, so -intVal never overflows, which is
        // undefined behavior). So now we are adding two large negative numbers. This should overflow,
        // triggering undefined behavior. Apparently MSVC handles overflow by wrapping (as does clang,
        // at least at time of experiment), and since MSVC is what you use on Windows, everything is okay.
        if (button != mButton || !PtInRect(&mClickRect, pt)
            || clickTime - mLastClickTime > LONG(GetDoubleClickTime())) {
            mButton = button;
            mNClicks = 0;
        }
        mNClicks++;

        mLastClickTime = clickTime;
        SetRect(&mClickRect, x, y, x, y);
        InflateRect(&mClickRect,
                    GetSystemMetrics(SM_CXDOUBLECLK) / 2,
                    GetSystemMetrics(SM_CYDOUBLECLK) / 2);

        return mNClicks;
    }

private:
    MouseButton mButton;
    int mNClicks;
    LONG mLastClickTime;
    RECT mClickRect;
};

LRESULT CALLBACK UITKWndProc(HWND hwnd, UINT uMsg,
                             WPARAM wParam, LPARAM lParam);
                             
struct Win32Window::Impl {
    static const wchar_t *wndclass;

    IWindowCallbacks& callbacks;
    HWND hwnd = 0;
    Window::Flags::Value flags;
    std::string title;
    Cursor cursor;
    std::shared_ptr<DrawContext> dc;
    ClickCounter clickCounter;
    bool isFinishedConstructing = false;  // wndproc callbacks can happen before constructor finishes
    bool needsUpdateMenu = false;
    bool needsLayout = true;
    uitk::TextEditorLogic* textEditor = nullptr;  // reference; we do not own
    uitk::Rect textEditorFrame_window;  // in window coordinates

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

    std::string getIMEString(long gcsType)
    {
        std::string result;
        if (this->textEditor) {
            HIMC imeContext = ImmGetContext(this->hwnd);
            auto nBytes = ImmGetCompositionStringW(imeContext, gcsType, nullptr, 0);
            if (nBytes >= 0) {
                char* compositionUTF16 = new char[nBytes + 2];  // +2 because u0000 is 2 bytes
                if (ImmGetCompositionStringW(imeContext, gcsType, compositionUTF16, nBytes) >= 0) {
                    compositionUTF16[nBytes] = 0;
                    compositionUTF16[nBytes + 1] = 0;
                    result = utf8FromWin32Unicode((wchar_t*)compositionUTF16);
                }
                delete[] compositionUTF16;
            }
            ImmReleaseContext(this->hwnd, imeContext);
        }
        return result;
    }
};
const wchar_t *Win32Window::Impl::wndclass = nullptr;

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
    // You cannot show 0x0 windows, so force to at least one pixel.
    // Note that @#$! windows.h #defines max and min, so you cannot use
    // std::max().
    width = max(1, width);
    height = max(1, height);

    mImpl->flags = flags;

    if (!Impl::wndclass) {
        Impl::wndclass = L"UITK_window";
        WNDCLASSEXW wcex = { sizeof(WNDCLASSEXW) };
        wcex.style         = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc   = UITKWndProc;
        wcex.cbClsExtra    = 0;
        wcex.cbWndExtra    = sizeof(LONG_PTR);
        wcex.hInstance     = HINST_THISCOMPONENT;
        wcex.hbrBackground = NULL;
        wcex.lpszMenuName  = NULL;
        // If we set a cursor, Windows will reset the cursor back to
        // this every time the mouse moves, which is annoying if you
        // want a different cursor. On the other hand, you need the
        // same code handling the WM_SETCURSOR message anyway, otherwise
        // the cursor will remain the resize cursor when the mouse moves
        // over the border and into the client area. I does not seem to
        // reduce the number of times the message is called, but one can
        // hope, anyway. (If we did not want to support changing cursors,
        // we should do wcex.hCursor = LoadCursor(NULL, IDI_APPLICATION)
        // so that the cursor would be the application cursor.)
        wcex.hCursor       = NULL;
        wcex.lpszClassName = Impl::wndclass;
        RegisterClassExW(&wcex);
    }

    DWORD style = 0;
    if (flags & Window::Flags::kPopup) {
        style |= WS_POPUP | WS_BORDER;
    } else {
        style |= WS_OVERLAPPEDWINDOW;
        // CW_USEDEFAULT is only valid with WS_OVERLAPPEDWINDOW
        if (x == kDefaultPos) {
            x |= CW_USEDEFAULT;
        }
        if (y == kDefaultPos) {
            y |= CW_USEDEFAULT;
        }
    }
    DWORD exStyle = 0;
    if (flags & Window::Flags::kDialog) {
        // Note that WS_DLGFRAME is used by normal windows (part of WS_CAPTION)
        // Also, we do not use WS_EX_TOPMOST because that would make the dialog
        // system-modal, which is not what we want.
        exStyle |= WS_EX_DLGMODALFRAME;
    }

    // CreateWindow() executes WM_GETMINMAXINFO, WM_NCCREATE, WM_NCCALCSIZE, WM_CREATE events
    // synchronously before the function finishes.
    mImpl->hwnd = CreateWindowExW(exStyle,
                                  Impl::wndclass,
                                  L"",
                                  style,
                                  x, y,
                                  width, height,
                                  NULL,  // parent
                                  NULL,  // menu
                                  HINST_THISCOMPONENT,
                                  this); // passed to WM_CREATE message, useful for a this pointer
    assert(IsWindowUnicode(mImpl->hwnd));
    auto &win32app = static_cast<Win32Application&>(Application::instance().osApplication());
    win32app.registerWindow(mImpl->hwnd, this);
    mImpl->updateDrawContext();
    setTitle(title);

    // We need to set the menubar, but do not do it now, because that will
    // cause a bunch of events to get sent synchronously, and we want to get
    // out of the constructor so that Window::mImpl::window actually has a value.
    mImpl->needsUpdateMenu = true;

    // Windows are not sent an initial size message. Normal windows will get one
    // when the menu is updated (possibly when they become visible), but others
    // will not.
    // TODO: we probably need an onCalcPreferredSize() callback or something.
    // HACK: this crashes if a window goes away too soon (since 'this' no longer exists).
    if (flags & Window::Flags::kDialog) {
        Application::instance().scheduleLater(nullptr, [this]() { this->onLayout(); });
    }

    mImpl->isFinishedConstructing = true;
}

Win32Window::~Win32Window()
{
    if (mImpl->hwnd) {
        close();
    }
}

void Win32Window::setNeedsUpdateMenubar()
{
    // Defer the update, just in case the user is adding multiple menu items,
    // we do not want to be updating the menubar after each one.
    mImpl->needsUpdateMenu = true;
}

bool Win32Window::menubarNeedsUpdate() const { return mImpl->needsUpdateMenu; }

void Win32Window::updateMenubar()
{
    // Calling SetMenu() generates a message, which interrupts this, so we
    // need to say we are updated before we get finished to avoid an infinite loop.
    mImpl->needsUpdateMenu = false;

    // WS_OVERLAPPEDWINDOW is 0x0, cannot compare with that
    bool isNormalStyle = !((GetWindowStyle(mImpl->hwnd) & WS_POPUP) ||
                           (GetWindowExStyle(mImpl->hwnd) & WS_EX_DLGMODALFRAME));
    if (isNormalStyle) {
        if (auto* win32menubar = dynamic_cast<Win32Menubar*>(&Application::instance().menubar())) {
            // The docs for CreateMenu() state that "resources associated with a menu
            // that is assigned to a window are freed automatically". Otherwise we need
            // to call DestroyMenu(). But... if there's already a menu, SetMenu() does
            // not destroy it.
            HMENU old = GetMenu(mImpl->hwnd);
            SetMenu(mImpl->hwnd, (HMENU)win32menubar->createNativeMenubar());
            DestroyMenu(old);  // empirically, destroying a null menu is okay
        }
    }
}

bool Win32Window::isShowing() const
{
    // Make sure this is callable after we have destroyed the window, but before the
    // destroy message has removed the window from the list.
    return (mImpl->hwnd != NULL && IsWindowVisible(mImpl->hwnd));
}

void Win32Window::show(bool show,
                       std::function<void(const DrawContext&)> onWillShow)
{
    if (show) {
        if (!isShowing() && onWillShow) {
            onWillShow(*mImpl->dc);
        }
        if (mImpl->flags & Window::Flags::kPopup) {
            ShowWindow(mImpl->hwnd, SW_SHOWNA);  // show, but do not activate
        } else {
            ShowWindow(mImpl->hwnd, SW_SHOWNORMAL);  // show and activate
        }
    } else {
        ShowWindow(mImpl->hwnd, SW_HIDE);
    }
}

void Win32Window::toggleMinimize()
{
    if (IsIconic(mImpl->hwnd)) {
        ShowWindow(mImpl->hwnd, SW_RESTORE);
    } else {
        ShowWindow(mImpl->hwnd, SW_MINIMIZE);
    }
}

void Win32Window::toggleMaximize()
{
    if (IsZoomed(mImpl->hwnd)) {
        ShowWindow(mImpl->hwnd, SW_RESTORE);
    }
    else {
        ShowWindow(mImpl->hwnd, SW_MAXIMIZE);
    }
}

void Win32Window::close()
{
    if (mImpl->hwnd) {
        mImpl->dc = nullptr;  // release
        auto hwnd = mImpl->hwnd;
        mImpl->hwnd = 0;
        DestroyWindow(hwnd);
    }
}

void Win32Window::raiseToTop() const
{
    BringWindowToTop(mImpl->hwnd);
}

void Win32Window::setTitle(const std::string& title)
{
    auto wtitle = win32UnicodeFromUTF8(title);
    SetWindowTextW(mImpl->hwnd, wtitle.c_str());
    mImpl->title = title;
}

void Win32Window::setCursor(const Cursor& cursor)
{
    mImpl->cursor = cursor;
    updateCursor();
}

void Win32Window::updateCursor() const
{
    if (auto* osCursor = mImpl->cursor.osCursor()) {
        osCursor->set();
    }
}

Rect Win32Window::contentRect() const
{
    RECT rectPx;
    GetClientRect(mImpl->hwnd, &rectPx);
    return Rect(PicaPt::fromPixels(float(rectPx.left), mImpl->dc->dpi()),
                PicaPt::fromPixels(float(rectPx.top), mImpl->dc->dpi()),
                PicaPt::fromPixels(float(rectPx.right - rectPx.left), mImpl->dc->dpi()),
                PicaPt::fromPixels(float(rectPx.bottom - rectPx.top), mImpl->dc->dpi()));
}

float Win32Window::dpi() const { return mImpl->dc->dpi(); }

OSRect Win32Window::osContentRect() const
{
    RECT clientRect;
    GetClientRect(mImpl->hwnd, &clientRect);
    POINT ulOS = { clientRect.left, clientRect.top };
    ClientToScreen(mImpl->hwnd, &ulOS);
    RECT windowOS;
    GetWindowRect(mImpl->hwnd, &windowOS);
    return OSRect{float(ulOS.x), float(ulOS.y),
                  float(windowOS.right - windowOS.left), float(windowOS.bottom - windowOS.top)};
}

void Win32Window::setContentSize(const Size& size)
{
    auto style = GetWindowStyle(mImpl->hwnd);
    auto exStyle = GetWindowExStyle(mImpl->hwnd);
    auto contentRect = osContentRect();
    RECT r = { LONG(contentRect.x),
               LONG(contentRect.y),
               LONG(contentRect.x + size.width.toPixels(dpi())),
               LONG(contentRect.y + size.height.toPixels(dpi())) };
    BOOL hasMenu = TRUE;
    if (style & WS_CHILD) {  // docs say GetMenu() for child window is undefined
        hasMenu = FALSE;
    } else if (GetMenu(mImpl->hwnd) == NULL) {
        hasMenu = FALSE;
    }
    AdjustWindowRectEx(&r, style, hasMenu, exStyle);
    setOSFrame(float(r.left), float(r.top), float(r.right - r.left), float(r.bottom - r.top));
}

OSRect Win32Window::osFrame() const
{
    RECT r;
    GetWindowRect(mImpl->hwnd, &r);
    return OSRect{float(r.left), float(r.top),
                  float(r.right - r.left), float(r.bottom - r.top)};
}

void Win32Window::setOSFrame(float x, float y, float width, float height)
{
    // Direct2D seems to have a limit of 16386 x 16386 before it fails to create a swap chain.
    // (This is a little curious, since 2^14 is 16384). This might be dependent on the video
    // card, but given this was determined on an Intel NUC (Intel IGP graphics), this seems safe.
    // Large windows are not normally a problem, but the font menu can sometimes produce them,
    // particularly during development of Win32Application::availableFontFamilies()!
    width = min(16383.0f, width);  // note @#$! windows.h #defines min/max; this isn't std::min()
    height = min(16383.0f, height);

    MoveWindow(mImpl->hwnd, int(x), int(y),
               int(std::round(width)), int(std::round(height)), FALSE /* don't repaint */);
    // MoveWindow() does not send WM_SIZING, it sends WM_SIZE. If we put onResize() there,
    // we seem to get an infinite draw loop. My understanding was that InvalidateRect()
    // (in postRedraw()) coalesces draw requests, but maybe not?
    onResize();
}

OSScreen Win32Window::osScreen() const
{
    auto hMonitor = MonitorFromWindow(mImpl->hwnd, MONITOR_DEFAULTTONULL);
    if (hMonitor) {
        MONITORINFO monitor;
        monitor.cbSize = sizeof(monitor);
        GetMonitorInfo(hMonitor, &monitor);
        return {
            { float(monitor.rcWork.left),
              float(monitor.rcWork.top),
              float(monitor.rcWork.right - monitor.rcWork.left),
              float(monitor.rcWork.bottom - monitor.rcWork.top) },
            { float(monitor.rcMonitor.left),
              float(monitor.rcMonitor.top),
              float(monitor.rcMonitor.right - monitor.rcMonitor.left),
              float(monitor.rcMonitor.bottom - monitor.rcMonitor.top) },
            dpi()
        };
    } else {
        return { { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }, 96.0f };
    }
}

void Win32Window::postRedraw() const
{
    InvalidateRect(mImpl->hwnd, NULL, TRUE /*erase background*/);
}

void Win32Window::beginModalDialog(OSWindow* w)
{
    auto dlgFrame = w->osFrame();
    auto f = osFrame();
    dlgFrame.x = f.x + 0.5f * f.width - 0.5f * dlgFrame.width;
    dlgFrame.y = f.y + 0.5f * f.height - 0.5f * dlgFrame.height;
    w->setOSFrame(dlgFrame.x, dlgFrame.y, dlgFrame.width, dlgFrame.height);
    w->show(true, [](const uitk::DrawContext&) {});
}

void Win32Window::endModalDialog(OSWindow* w)
{
    w->show(false, [](const uitk::DrawContext&) {});
}

PicaPt Win32Window::borderWidth() const
{
    return PicaPt::fromPixels(float(GetSystemMetrics(SM_CXSIZEFRAME)), dpi());
}

Point Win32Window::currentMouseLocation() const
{
    POINT pos;
    GetCursorPos(&pos);
    ScreenToClient(mImpl->hwnd, &pos);
    return Point(PicaPt::fromPixels(float(pos.x), dpi()),
                 PicaPt::fromPixels(float(pos.y), dpi()));
}

void* Win32Window::nativeHandle() { return mImpl->hwnd; }

IWindowCallbacks& Win32Window::callbacks() { return mImpl->callbacks; }

void Win32Window::callWithLayoutContext(std::function<void(const DrawContext&)> f)
{
    f(*mImpl->dc);
}

void Win32Window::setTextEditing(TextEditorLogic* te, const Rect& frame)
{
    mImpl->textEditor = te;
    mImpl->textEditorFrame_window = frame;
}

void Win32Window::showIMEWindow()
{
    if (mImpl->textEditor) {
        auto sel = mImpl->textEditor->selection();
        auto glyphRect = mImpl->textEditor->glyphRectAtIndex(sel.start);
        auto cursorPt = mImpl->textEditorFrame_window.upperLeft() + glyphRect.lowerLeft();
        auto winX = LONG(cursorPt.x.toPixels(mImpl->dc->dpi()));
        auto winY = LONG(cursorPt.y.toPixels(mImpl->dc->dpi()));

        // Some IMEs use the caret position and some need to be told where to go manually,
        // so we do both.

        CreateCaret(mImpl->hwnd, NULL, 1, int(std::ceil(glyphRect.height.toPixels(mImpl->dc->dpi()))));
        SetCaretPos(winX, winY);

        HIMC imeContext = ImmGetContext(mImpl->hwnd);
        COMPOSITIONFORM cf;
        cf.dwStyle = CFS_FORCE_POSITION;
        cf.ptCurrentPos.x = winX;
        cf.ptCurrentPos.y = winY;
        ImmSetCompositionWindow(imeContext, &cf);
        ImmReleaseContext(mImpl->hwnd, imeContext);
    }
}

void Win32Window::hideIMEWindow()
{
    DestroyCaret();
}

void Win32Window::updateIMEText(long lParam)
{
    if (mImpl->textEditor) {
        if (lParam != 0) {
            auto composition = mImpl->getIMEString(GCS_COMPSTR);
            auto sel = mImpl->textEditor->selection();
            if (sel.end > sel.start) {
                // TODO: it would be nice if pressing ESC to cancel the unconverted test would undo the delete
                //       (e.g. add characters, select some, type some pinyin, press ESC to cancel;
                //       should get original text back, ideally with the original selection)
                mImpl->textEditor->deleteSelection();
            }

            // Windows allows moving within the unconverted text. This feature of ImmGetCompositionString()
            // is undocumented, but I am assuming that the return value is in bytes because,
            // a) it is bytes when we get the string, and b) the whole point of the IME is to turn
            // ASCII into unicode and ASCII is bytes. Our text offsets are always into the UTF-8
            // string (that is, byte offsets).
            HIMC imeContext = ImmGetContext(mImpl->hwnd);
            auto offset = ImmGetCompositionStringW(imeContext, GCS_CURSORPOS, nullptr, 0);
            ImmReleaseContext(mImpl->hwnd, imeContext);

            TextEditorLogic::IMEConversion conv(sel.start, composition);
            conv.cursorOffset = offset;
            mImpl->textEditor->setIMEConversion(conv);

        } else {
            // lParam == 0 means composition was cancelled
            mImpl->textEditor->setIMEConversion(TextEditorLogic::IMEConversion());
        }
    }
}

void Win32Window::applyIMEText()
{
    if (mImpl->textEditor) {
        auto conversion = mImpl->getIMEString(GCS_RESULTSTR);
        if (!conversion.empty()) {
            auto sel = mImpl->textEditor->selection();
            int newPos = sel.start + int(conversion.size());
            mImpl->textEditor->insertText(sel.start, conversion);
            mImpl->textEditor->setSelection(
                TextEditorLogic::Selection(newPos, newPos, TextEditorLogic::Selection::CursorLocation::kEnd));
        }
        updateIMEText(GCS_COMPSTR);
    }
}

void Win32Window::cancelIMEText()
{
    if (mImpl->textEditor) {
        mImpl->textEditor->setIMEConversion(TextEditorLogic::IMEConversion());
    }
}

void Win32Window::setNeedsAccessibilityUpdate()
{
    // unsupported at this time
}

void Win32Window::setAccessibleElements(const std::vector<AccessibilityInfo>& elements)
{
    // unsupported at this time
}

ClickCounter& Win32Window::clickCounter() { return mImpl->clickCounter;  }

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
    // For popup menus a WM_SIZE message gets sent in CreateWindow(), event
    // though it does not seem to for other types. And despite the constructor
    // calling updateDrawContext() with no problem, it crashes if we call it a
    // second time.
    // TODO: figure out what is really going on here.
    if (mImpl->isFinishedConstructing) {
        mImpl->updateDrawContext();
    }

    // We are probably going to get a series of resize messages in a row,
    // so defer the layout until we actually try to draw. However, if we
    // are not visible, we are not going to draw (nor are we likely to
    // get a series of resize messages from resizing via the mouse).
    if (isShowing() || !mImpl->isFinishedConstructing) {
        mImpl->needsLayout = true;
    } else {
        // WndProc events will happen before the constructor is finished.
        // We cannot call back to Window functions, since Window::mImpl::window
        // will not be set yet (since the Win32Window constructor is not finished).
        onLayout();
    }
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

void Win32Window::onKey(const KeyEvent& e)
{
    mImpl->clickCounter.reset();
    mImpl->callbacks.onKey(e);
}

void Win32Window::onText(const TextEvent& e)
{
    mImpl->callbacks.onText(e);
}

void Win32Window::onActivated(const Point& currentMousePos)
{
    mImpl->callbacks.onActivated(currentMousePos);
}

void Win32Window::onDeactivated()
{
    mImpl->clickCounter.reset();
    mImpl->callbacks.onDeactivated();
}

void Win32Window::onMenuWillShow()
{
    mImpl->callbacks.onMenuWillShow();
}

void Win32Window::onMenuActivated(MenuId id)
{
    mImpl->callbacks.onMenuActivated(id);
}

bool Win32Window::onWindowShouldClose()
{
    return mImpl->callbacks.onWindowShouldClose();
}

void Win32Window::onWindowWillClose()
{
    mImpl->hwnd = 0;  // otherwise delete calls close, which tries to destroy the window again
    return mImpl->callbacks.onWindowWillClose();
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

KeyEvent makeKeyEvent(KeyEvent::Type type, bool isRepeat, WPARAM wParam)
{
    KeyEvent e;
    e.type = type;
    e.isRepeat = isRepeat;
    e.nativeKey = int(wParam);
    e.keymods = 0;
    e.key = Key::kUnknown;
    if (GetKeyState(VK_SHIFT) & 0x8000) { e.keymods |= KeyModifier::kShift; }
    if (GetKeyState(VK_CONTROL) & 0x8000) { e.keymods |= KeyModifier::kCtrl; }

    auto keyIt = kVK2Key.find(int(wParam));
    if (keyIt != kVK2Key.end()) {
        e.key = keyIt->second;
    } else {
        if (wParam >= '0' && wParam <= '9') {
            e.key = Key(wParam);
        } else if (wParam >= 'A' && wParam <= 'Z') {
            e.key = Key(wParam + 32);
        }
    }

    return e;
}

LRESULT CALLBACK UITKWndProc(HWND hwnd, UINT message,
                             WPARAM wParam, LPARAM lParam)
{
    Win32Window *w = (Win32Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (w && w->menubarNeedsUpdate()) {
        w->updateMenubar();
    }

    //void *hex = (void*)message;
    //DPrint() << "[debug] message: " << hex << ", w: " << w;

    switch (message) {
        case WM_CREATE: {
            auto *create = (CREATESTRUCT*)lParam;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)create->lpCreateParams);
            return 0;  // 0 to continue creation, -1 to cancel
        }
        case WM_CLOSE:
            if (w->onWindowShouldClose()) {
                DestroyWindow(hwnd);
            }
            return 0;
        case WM_DESTROY: {
            w->onWindowWillClose();
            auto& win32app = static_cast<Win32Application&>(Application::instance().osApplication());
            win32app.unregisterWindow(hwnd);
            return 0;
        }
        //case WM_NCDESTROY:
        //    // WM_DESTROY is sent to the window, then the child windows are destroyed,
        //    // and finally WM_NCDESTROY is sent to the window
        //    return 0;
        case WM_ACTIVATE:
            // WM_ACTIVATEAPP, which is only called if the user is switching between apps.
            // Since we could have multiple windows (e.g. editing multiple documents) that
            // the user switches between, we want WM_ACTIVATE, which is sent regardless of
            // whether the window is (de)activated for a different app or within the app.
            if ((wParam & 0xff) == WA_INACTIVE) {
                w->onDeactivated();
            } else {
                w->onActivated(w->currentMouseLocation());
            }
            return 0;
        case WM_MOUSEACTIVATE: {
            // WM_MOUSEACTIVATE is sent when the mouse is clicked in a window.
            // If this is a popup window, we should not activate it, as the main
            // window should stay active. (And if we do activate the child, then
            // things get pretty messed up, which clicks not getting delivered
            // to the next popup window, and the main window does not get activated
            // again.
            bool isPopup = (GetWindowLongA(hwnd, GWL_STYLE) & WS_POPUP);
            if (isPopup) {
                return MA_NOACTIVATE;
            } else {
                return MA_ACTIVATE;
            }
        }
        case WM_PAINT: {
            // Even if we are using Direct2D to draw, we still need to call BeginPaint()
            // and EndPaint(), otherwise the paint region will never be cleared and we
            // will have endless redraws.
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            w->onDraw();
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_SIZING: {  // WM_SIZE is sent when done, but we want live resizing
            auto *rectPx = (RECT*)lParam;  // can change this to alter the changing size
            w->onResize();
            return TRUE;
        }
        case WM_SIZE:
            w->onResize();
            return 0;  // Note this is different from the return value for WM_SIZING!
        case WM_MOVE:
            w->onMoved();  // in case changed screens
            return 0;
        case WM_DISPLAYCHANGE:  // display resolution changed
            w->onResize();
            return 0;  // docs do not say what should be returned
        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT) {
                w->updateCursor();
                return TRUE;  // halt further processing of the cursor change request
            }
            return DefWindowProc(hwnd, message, wParam, lParam);
        case WM_MOUSEMOVE:
            w->onMouse(makeMouseEvent(MouseEvent::Type::kMove,
                                      MouseButton::kNone, 0, wParam),
                       GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_LBUTTONDOWN: {
            auto x = GET_X_LPARAM(lParam);
            auto y = GET_Y_LPARAM(lParam);
            int nClicks = w->clickCounter().click(MouseButton::kLeft,
                                                  GetMessageTime(), x, y);
            w->onMouse(makeMouseEvent(MouseEvent::Type::kButtonDown,
                                      MouseButton::kLeft, nClicks, wParam),
                       x, y);
            return 0;
        }
        case WM_LBUTTONUP:
            w->onMouse(makeMouseEvent(MouseEvent::Type::kButtonUp,
                                      MouseButton::kLeft, 0, wParam),
                       GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_MBUTTONDOWN: {
            auto x = GET_X_LPARAM(lParam);
            auto y = GET_Y_LPARAM(lParam);
            int nClicks = w->clickCounter().click(MouseButton::kLeft,
                                                  GetMessageTime(), x, y);
            w->onMouse(makeMouseEvent(MouseEvent::Type::kButtonDown,
                                      MouseButton::kMiddle, nClicks, wParam),
                       x, y);
            return 0;
        }
        case WM_MBUTTONUP:
            w->onMouse(makeMouseEvent(MouseEvent::Type::kButtonUp,
                                      MouseButton::kMiddle, 0, wParam),
                       GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_RBUTTONDOWN: {
            auto x = GET_X_LPARAM(lParam);
            auto y = GET_Y_LPARAM(lParam);
            int nClicks = w->clickCounter().click(MouseButton::kLeft,
                                                  GetMessageTime(), x, y);
            w->onMouse(makeMouseEvent(MouseEvent::Type::kButtonDown,
                                      MouseButton::kRight, nClicks, wParam),
                       x, y);
            return 0;
        }
        case WM_RBUTTONUP:
            w->onMouse(makeMouseEvent(MouseEvent::Type::kButtonUp,
                                      MouseButton::kRight, 0, wParam),
                       GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_XBUTTONDOWN: {
            auto x = GET_X_LPARAM(lParam);
            auto y = GET_Y_LPARAM(lParam);
            int nClicks = w->clickCounter().click(MouseButton::kLeft,
                                                  GetMessageTime(), x, y);
            w->onMouse(makeMouseEvent(MouseEvent::Type::kButtonDown,
                                      getXButton(wParam), nClicks, wParam),
                       x, y);
            return 0;
        }
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
        // We can get double-click messages if we set the CS_DBLCLKS style on the window class,
        // but we have to do triple clicks ourselves. This makes the double-click messages
        // pointless and more complicated, so we do both double and triple clicks ourselves.
        //case WM_LBUTTONDBLCLK:  // requires window have CS_DBLCLKS style
        //    w->onMouse(makeMouseEvent(MouseEvent::Type::kButtonDown,
        //        MouseButton::kLeft, 2, wParam),
        //        GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        //    return 0;
        //case WM_MBUTTONDBLCLK:  // requires window have CS_DBLCLKS style
        //    w->onMouse(makeMouseEvent(MouseEvent::Type::kButtonDown,
        //        MouseButton::kMiddle, 2, wParam),
        //        GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        //    return 0;
        //case WM_RBUTTONDBLCLK:  // requires window have CS_DBLCLKS style
        //    w->onMouse(makeMouseEvent(MouseEvent::Type::kButtonDown,
        //        MouseButton::kRight, 2, wParam),
        //        GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        //    return 0;
        //case WM_XBUTTONDBLCLK:
        //    w->onMouse(makeMouseEvent(MouseEvent::Type::kButtonDown,
        //        getXButton(wParam), 2, wParam),
        //        GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        //    return 0;

        case WM_KEYDOWN: {  // Alt+key and F-keys use WM_SYSKEYDOWN/UP
            int nRepeats = (lParam & 0xffff);
            if (nRepeats == 0) {
                nRepeats = 1;
            }
            auto e = makeKeyEvent(KeyEvent::Type::kKeyDown, false, wParam);
            w->onKey(e);

            e.isRepeat = true;
            for (int i = 1;  i < nRepeats;  ++i) {
                w->onKey(e);
            }
            return 0;
        }
        case WM_KEYUP:
            w->onKey(makeKeyEvent(KeyEvent::Type::kKeyUp, false, wParam));
            return 0;
        case WM_CHAR: {
            // Backspace, tab, escape, enter are sent as WM_CHAR, even though they are really
            // special keys, not text entry keys. (Enter is a "control key" in our case, because
            // WM_KEYDOWN already handles it, and it adds a new line, not a '\r' (13), which
            // is what Windows sends us.)
            if (wParam < 32 /* space */) {
                return 0;
            }

            WCHAR utf16bytes[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
            // wParam is UTF-16. We copy into array, which has extra nulls so we will be null-terminated
            memcpy(utf16bytes, (char*)&wParam, sizeof(wParam));

            TextEvent e;
            e.utf8 = utf8FromWin32Unicode(utf16bytes);
            w->onText(e);
            return 0;
        }
        case WM_UNICHAR: {  // this only sent if an application calls Send/PostMessage()
            TextEvent e;
            if (wParam == UNICODE_NOCHAR) {
                return TRUE;  // indicates we support unicode
            }
            else {
                char utf8bytes[16];  // UTF-8 uses 4 bytes maximum (plus a \0)
                int32_t utf32 = int32_t(wParam);
                int utf8index = 0;
                if (utf32 <= 0x007f) { utf8bytes[utf8index++] = char(utf32); }
                else if (utf32 <= 0x07ff) {
                    utf8bytes[utf8index++] = 0b11000000 | (utf32 & 0b00011111);
                    utf32 = utf32 >> 5;
                    utf8bytes[utf8index++] = 0b10000000 | (utf32 & 0b00111111);
                }
                else if (utf32 <= 0xffff) {
                    if (utf32 <= 0x07ff) {
                        utf8bytes[utf8index++] = 0b11100000 | (utf32 & 0b00001111);
                        utf32 = utf32 >> 4;
                        utf8bytes[utf8index++] = 0b10000000 | (utf32 & 0b00111111);
                        utf32 = utf32 >> 6;
                        utf8bytes[utf8index++] = 0b10000000 | (utf32 & 0b00111111);
                    }
                    else {
                        utf8bytes[utf8index++] = 0b11110000 | (utf32 & 0b00000111);
                        utf32 = utf32 >> 3;
                        utf8bytes[utf8index++] = 0b10000000 | (utf32 & 0b00111111);
                        utf32 = utf32 >> 6;
                        utf8bytes[utf8index++] = 0b10000000 | (utf32 & 0b00111111);
                        utf32 = utf32 >> 6;
                        utf8bytes[utf8index++] = 0b10000000 | (utf32 & 0b00111111);
                    }
                    utf8bytes[utf8index] = '\0';
                    e.utf8 = utf8bytes;
                    w->onText(e);

                }
                return FALSE;
            }
        }
        case WM_IME_SETCONTEXT:
            lParam = lParam & (~ISC_SHOWUICOMPOSITIONWINDOW);  // we show the composition, so remove this
            return DefWindowProc(hwnd, message, wParam, lParam);
        case WM_IME_STARTCOMPOSITION:
            w->showIMEWindow();
            return DefWindowProc(hwnd, message, wParam, lParam);
        case WM_IME_COMPOSITION:
            if (lParam & GCS_RESULTSTR) {  // apply conversion
                w->applyIMEText();
            } else {
                w->updateIMEText(long(lParam));
            }
            return 0;
        case WM_IME_ENDCOMPOSITION:
            w->cancelIMEText();
            w->hideIMEWindow();
            return 0;
        case WM_INPUTLANGCHANGE:
        case WM_IME_SELECT:
        case WM_IME_REQUEST:
        case WM_IME_CONTROL:
        case WM_IME_NOTIFY:
        case WM_IME_CHAR:
            return DefWindowProc(hwnd, message, wParam, lParam);
        case WM_ENTERMENULOOP:
            w->onMenuWillShow();
            return 0;
        case WM_COMMAND: {
            // AppendMenu() offers a UINT as a menu item identifier, but
            // since WM_COMMAND uses the high word of 32-bit wParam for
            // other information, only 16-bits is left in the low word
            // for the identifier :(. WM_MENUCOMMAND gives us the menu index,
            // from which we can retrieve the full menu item ID, but that
            // requires the menu to have MNS_NOTIFYBYPOS assigned in menuItem.dwFlags,
            // and it does not work for submenus, so that is not an option.
            if (HIWORD(wParam) == 0 ||  // user clicked on a menu
                HIWORD(wParam) == 1) {  // user pressed accelerator key
                w->onMenuActivated(MenuId(LOWORD(wParam)));
                return 0;  // return 0: handled
            }
            // This is a control message. Not sure if we need to pass to
            // DefWindowProc, but just to be safe...
            return DefWindowProc(hwnd, message, wParam, lParam);
        }
        case WM_DWMCOLORIZATIONCOLORCHANGED:
            Application::instance().onSystemThemeChanged();
            return 0;
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
}

}  // namespace uitk
