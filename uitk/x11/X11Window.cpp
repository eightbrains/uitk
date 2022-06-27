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

#include "X11Window.h"

#include "../private/Utils.h"
#include "../Application.h"
#include "../Cursor.h"
#include "../Events.h"
#include "../OSCursor.h"
#include "../TextEditorLogic.h"
#include "../private/Utils.h"
#include "X11Application.h"

#include <string.h>  // for memset()
#include <X11/Xlib.h>
#include <X11/Xatom.h>

namespace uitk {

namespace {

static const char kNetWMHidden[] = "_NET_WM_STATE_HIDDEN";
static const char kNetWMMaximizedVert[] = "_NET_WM_STATE_MAXIMIZED_VERT";
static const char kNetWMMaximizedHorz[] = "_NET_WM_STATE_MAXIMIZED_HORZ";
static const int _NET_WM_STATE_REMOVE = 0;
static const int _NET_WM_STATE_ADD = 1;
static const int _NET_WM_STATE_TOGGLE = 2;

bool hasWMProperty(Display *d, ::Window xwin, const char *prop)
{
    long maxLen = 64;
    Atom actualType;
    int actualFormat;
    unsigned long bytesRemaining, nStates = 0;
    Atom* states = NULL;

    Atom wmState = XInternAtom(d, "_NET_WM_STATE", False);
    Atom wmProp = XInternAtom(d, prop, False);

    if (XGetWindowProperty(d, xwin, wmState, 0, maxLen, False /* don't delete*/,
                           XA_ATOM, &actualType, &actualFormat,
                           &nStates, &bytesRemaining,
                           (unsigned char**)&states) == Success) {
        for (unsigned long i = 0;  i < nStates;  ++i) {
            if (states[i] == wmProp) {
                return true;
            }
        }
    }
    return false;
}

void setIMEPosition(XIC xic, int windowX, int windowY)
{
    // I cannot figure out a way to set the spot location if you are using
    // preedit callbacks. (Indeed, the docs says that XNSpotLocation is only
    // applicable if XIMPreeditPosition is used.) However, it seems that
    // GTK applications are somehow able to set the location.
    XVaNestedList preedit_attr;
    XPoint spot;
    spot.x = windowX;
    spot.y = windowY;
    XVaNestedList attr = XVaCreateNestedList(0, XNSpotLocation, &spot, NULL);
    auto result = XSetICValues(xic, XNPreeditAttributes, attr, NULL);
    XFree(attr);
}

void addCodePoint(std::string *utf8, uint32_t utf32)
{
    if (utf32 < 0x0080) {
        char c = (utf32 & 0b01111111);
        utf8->push_back(c);
    } else if (utf32 < 0x0800) {
        char c1 = (0b110 << 5) | ((utf32 & 0b11111000000) >> 6);
        char c2 = ( 0b10 << 6) |  (utf32 & 0b00000111111);
        utf8->push_back(c1);
        utf8->push_back(c2);
    } else if (utf32 < 0x10000) {
        char c1 = (0b1110 << 4) | ((utf32 & 0b1111000000000000) >> 12) ;
        char c2 = (  0b10 << 6) | ((utf32 & 0b0000111111000000) >> 6);
        char c3 = (  0b10 << 6) |  (utf32 & 0b0000000000111111);
        utf8->push_back(c1);
        utf8->push_back(c2);
        utf8->push_back(c3);
    } else {
        char c1 = (0b11110 << 3) | ((utf32 & 0b111000000000000000000) >>18);
        char c2 = (   0b10 << 6) | ((utf32 & 0b000111111000000000000) >>12);
        char c3 = (   0b10 << 6) | ((utf32 & 0b000000000111111000000) >> 6);
        char c4 = (   0b10 << 6) |  (utf32 & 0b000000000000000111111);
        utf8->push_back(c1);
        utf8->push_back(c2);
        utf8->push_back(c3);
        utf8->push_back(c4);
    }
}

// macOS and win32 do this using the native functions.
// Note that std::c16rtomb() cannot convert from UTF-16 to UTF-8
// Also note that wchar_t is implementation-defined and may NOT be the
// same size as char16_t! (Windows uses 2 bytes, GCC on Linux uses 4)
std::string convertUtf16ToUtf8(const char16_t *utf16)
{
    std::string utf8;

    const char16_t *c = utf16;
    while (*c != '\0') {
        if (*c < 0xd800) {
            addCodePoint(&utf8, uint32_t(*c));
        } else {
            uint32_t utf32 = 0;
            utf32 = uint32_t((*c - 0xd800) * 0x0400);
            c++;
            if (*c == '\0') {
                break; // unexpected end: don't add this (invalid) code point
            }
            if (*c >= 0xdc00) {  // this should always be true
                utf32 += uint32_t(*c - 0xdc00);
            }
            utf32 += uint32_t(0x10000);
            addCodePoint(&utf8, utf32);
        }
        c++;
    }

    return utf8;
}

bool testUtf16Conversion() {
    struct ConvTest {
        const char16_t *utf16;
        std::string utf8result;
        ConvTest(const char16_t *u16, const char *u8)
            : utf16(u16), utf8result(u8)
        {}
    };
    std::vector<ConvTest> tests = {
        ConvTest( u"Z\0", "Z" ),            // 005a -> 00 5a
        ConvTest( u"\u00a3\0",  "\u00a3" ),  // 00a3 -> c2 a3
        ConvTest( u"\u0939\0",  "\u0939" ),  // 0930 -> e0 a4 b9
        ConvTest( u"\u20ac\0",  "\u20ac" ),  // 20ac -> e2 82 ac
        ConvTest( u"\ud55c\0",  "\ud55c" ),  // d55c -> ed 95 9c
        ConvTest( u"\U00010348\0", u8"\U00010348" ), // asdf
        ConvTest( u"\U00010437\0", u8"\U00010437" )  // d801 dc37 -> 01 
    };
    for (auto &t : tests) {
        assert(convertUtf16ToUtf8(t.utf16) == t.utf8result);
    }
    return true;
}
static bool tested = testUtf16Conversion();

// IME has been enabled
int preeditStart(XIM xim, XPointer clientData, XPointer callData)
{
    return -1;  // preedit length has no limit
}

// Caret moves
void preeditCaret(XIM xim, XPointer clientData,
                  XIMPreeditCaretCallbackStruct *callData)
{
    auto *w = (X11Window*)clientData;
    if (w && callData) {
        int arg = -1;
        if (callData->direction == XIMAbsolutePosition) {
            arg = callData->position;
        }
        int newOffset = w->imeMove(int(callData->direction), arg);
        callData->position = newOffset; // return the new position
    }
}

// Preedit text has changed
void preeditDraw(XIM xim, XPointer clientData,
                 XIMPreeditDrawCallbackStruct *callData)
{
    auto *w = (X11Window*)clientData;
    if (w && callData) {
        // The text is a mess. The Xlib docs say that the result will either
        // be multibyte or wchar. But wchar is implementation dependent, and
        // is 4 bytes on GCC/Linux. I assume that this means it is encoded as
        // UTF-32, but who knows?!
        if (callData->text) {
            if (callData->text->encoding_is_wchar) {
                std::string utf8;
                if (sizeof(wchar_t) == 4) {
                    for (uint32_t *c = (uint32_t*)callData->text->string.wide_char;
                         *c != 0;
                         ++c) {
                        addCodePoint(&utf8, *c);
                    }
                } else {  // sizeof(wchar_t) == 2)
                    utf8 = convertUtf16ToUtf8((char16_t*)callData->text->string.wide_char);
                }
                w->imeUpdate(utf8.c_str(), callData->chg_first,
                             callData->chg_length, callData->caret);
            } else {
                w->imeUpdate(callData->text->string.multi_byte,
                             callData->chg_first, callData->chg_length,
                             callData->caret);
            }
        } else {
            w->imeUpdate(nullptr, callData->chg_first, callData->chg_length,
                         callData->caret);
        }
    }
}

// IME has been disabled
void preeditDone(XIM xim, XPointer client_data, XPointer callData)
{
    // nothing to deallocate
}

}  // namespace

static X11Window* gActiveWindow = nullptr;
    
struct X11Window::Impl {
    IWindowCallbacks& callbacks;
    Display *display;
    ::Window xwindow = 0;
    XIC xic = nullptr;
    int xscreenNo = 0;
    int width;
    int height;
    int flags = 0;
    float dpi = 96.0f;
    std::shared_ptr<DrawContext> dc;
    std::string title;
    TextEditorLogic *textEditor = nullptr;
    Rect textRect;
    bool showing = false;

    bool drawRequested = false;
    bool needsLayout = true;

    void updateDrawContext()
    {
        XWindowAttributes attrs;
        XGetWindowAttributes(this->display, this->xwindow, &attrs);
        this->width = attrs.width;
        this->height = attrs.height;
        this->xscreenNo = XScreenNumberOfScreen(attrs.screen);
        X11Application& x11app = static_cast<X11Application&>(Application::instance().osApplication());
        this->dpi = x11app.dpiForScreen(this->xscreenNo);

        this->dc = DrawContext::fromX11(this->display, &this->xwindow,
                                        this->width, this->height, this->dpi);
    }

    void destroyWindow()
    {
        // Unregister the window from the application, because there may
        // be unprocessed events in the queue for the window.
        auto& x11app = static_cast<X11Application&>(Application::instance().osApplication());
        x11app.unregisterWindow(this->xwindow);

        this->dc = nullptr;

        XDestroyIC(this->xic);
        this->xic = nullptr;

        XDestroyWindow(this->display, this->xwindow);
        this->xwindow = 0;
    }
};

X11Window::X11Window(IWindowCallbacks& callbacks,
                     const std::string& title, int width, int height,
                     Window::Flags::Value flags)
    : X11Window(callbacks, title, width, height, -1, -1, flags)
{}

X11Window::X11Window(IWindowCallbacks& callbacks,
                     const std::string& title, int x, int y,
                     int width, int height,
                     Window::Flags::Value flags)
    : mImpl(new Impl{callbacks})
{
    mImpl->flags = flags;

    // X does not seem to support windows with 0 width or height
    width = std::max(1, width);
    height = std::max(1, height);
    
    X11Application& x11app = static_cast<X11Application&>(Application::instance().osApplication());
    Display *display = static_cast<Display*>(x11app.display());
    mImpl->display = display;
    mImpl->xwindow = XCreateSimpleWindow(display, XDefaultRootWindow(display),
                                         x, y, width, height, 1, 0, 0);
    x11app.registerWindow(mImpl->xwindow, this);

    if (flags & Window::Flags::kPopup) {
        Atom type = XInternAtom(mImpl->display, "_NET_WM_WINDOW_TYPE", False);
        long value = XInternAtom(mImpl->display, "_NET_WM_WINDOW_TYPE_POPUP_MENU", False);
        XChangeProperty(mImpl->display, mImpl->xwindow, type, XA_ATOM, 32,
                        PropModeReplace, (unsigned char *)&value, 1);
    } else {
        // Tell the window manager we want to be notified when a window is
        // closed, otherwise X will just kill our connection.
        Atom wmDeleteMsg = XInternAtom(display, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(display, mImpl->xwindow, &wmDeleteMsg, 1);
        
        setTitle(title);
    }

    XIMCallback startCallback, caretCallback, drawCallback, doneCallback;
    startCallback.client_data = (XPointer)this;
    startCallback.callback = (XIMProc)preeditStart;
    caretCallback.client_data = (XPointer)this;
    caretCallback.callback = (XIMProc)preeditCaret;
    drawCallback.client_data = (XPointer)this;
    drawCallback.callback = (XIMProc)preeditDraw;
    doneCallback.client_data = (XPointer)this;
    doneCallback.callback = (XIMProc)preeditDone;
    XVaNestedList preeditAttr = XVaCreateNestedList(
        0,
        XNPreeditStartCallback, &startCallback,
        XNPreeditCaretCallback, &caretCallback,
        XNPreeditDrawCallback, &drawCallback,
        XNPreeditDoneCallback, &doneCallback,
        NULL);

    mImpl->xic = XCreateIC(static_cast<XIM>(x11app.xim()),
                           XNInputStyle, XIMPreeditCallbacks | XIMStatusNothing,
                           XNClientWindow, mImpl->xwindow,
                           XNPreeditAttributes, preeditAttr,
                           NULL);
    if (!mImpl->xic) {
        // This XIM module doesn't support preedit callbacks,
        // Try something more basic.
        mImpl->xic = XCreateIC(static_cast<XIM>(x11app.xim()),
                           XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                           XNClientWindow, mImpl->xwindow,
                           XNPreeditAttributes, preeditAttr,
                           NULL);
    }
    if (!mImpl->xic) {
        // Ok, well, let's make it really basic.
        mImpl->xic = XCreateIC(static_cast<XIM>(x11app.xim()),
                           XNInputStyle, XIMPreeditNone | XIMStatusNothing,
                           XNClientWindow, mImpl->xwindow,
                           XNPreeditAttributes, preeditAttr,
                           NULL);
    }
    assert(mImpl->xic);
    XFree(preeditAttr);

    // Now that window can be properly displayed, turn on receiving events
    // See https://tronche.com/gui/x/xlib/events/processing-overview.html
    // for information on the masks and the events generated.
    XSelectInput(mImpl->display, mImpl->xwindow,
                 StructureNotifyMask |  // map/unmap/destroy/resize/move
                 ExposureMask |
                 // ResizeRedirectMask |  // allows us to intercept resize
                 KeyPressMask | KeyReleaseMask |
                 ButtonPressMask | ButtonReleaseMask |
                 PointerMotionMask | ButtonMotionMask |
                 KeymapStateMask |  // keyboard state at enter and focus in
                 EnterWindowMask | LeaveWindowMask |
                 FocusChangeMask);

    mImpl->updateDrawContext();
}

X11Window::~X11Window()
{
    if (mImpl->xwindow) {
        // Cannot call close() here, because it will call onWindowShouldClose(),
        // and that is no longer an option.
        mImpl->destroyWindow();
    }
}

void* X11Window::xic() const { return mImpl->xic; }

int X11Window::imeMove(int ximDir, int arg)
{
    auto *edit = mImpl->textEditor;
    if (!edit) {
        return 0;
    }

    auto conv = edit->imeConversion();

    switch ((XIMCaretDirection)ximDir) {
        case XIMForwardChar:
        case XIMForwardWord: {
            if (conv.cursorOffset < conv.text.size()) {
                conv.cursorOffset = nextCodePointUtf8(conv.text.c_str(), conv.cursorOffset);
            }
            break;
        }
        case XIMBackwardChar:
        case XIMBackwardWord: {
            conv.cursorOffset = prevCodePointUtf8(conv.text.c_str(), conv.cursorOffset);
            break;
        }
        case XIMCaretUp:
        case XIMPreviousLine:
        case XIMLineStart:
            conv.cursorOffset = 0;
            break;
        case XIMCaretDown:
        case XIMNextLine:
        case XIMLineEnd:
            conv.cursorOffset = conv.text.size();  // after last glyph
            break;
        case XIMAbsolutePosition:
            conv.cursorOffset = arg;
            break;
        case XIMDontChange:
        default:
            break;
    }

    edit->setIMEConversion(conv);
    postRedraw();

    return conv.cursorOffset;
}

void X11Window::imeUpdate(const char *utf8, int startCP, int lenCP,
                          int newOffsetCP)
{
    // https://www.x.org/releases/X11R7.7/doc/libX11/libX11/libX11.html#Input_Method_Overview states that start and len will never be negative

    auto *edit = mImpl->textEditor;
    if (!edit) {
        return;
    }
    auto conv = edit->imeConversion();

    if (conv.start < 0) {  // empty conversion start is set to kInvalidIndex
        conv.start = edit->selection().start;
    }

    auto r = edit->glyphRectAtIndex(conv.start);
    int x = int(std::round((mImpl->textRect.x + r.x).toPixels(mImpl->dpi)));
    int y = int(std::round((mImpl->textRect.y + r.y).toPixels(mImpl->dpi)));
    setIMEPosition(mImpl->xic, x, y);

    // Note that the indices are *code points* NOT byte indices.
    auto cpToIdx = utf8IndicesForCodePointIndices(conv.text.c_str());
    int start = cpToIdx[startCP];
    int len = cpToIdx[startCP + lenCP] - start;

    if (utf8) {
        if (len > 0) {  // replace:  delete then insert as normal
            conv.text.erase(start, len);
        }
        conv.text.insert(start, utf8);
    } else {  // delete
        conv.text.erase(start, len);
    }

    // The new offset is specified in the new preedit text
    cpToIdx = utf8IndicesForCodePointIndices(conv.text.c_str());
    conv.cursorOffset = cpToIdx[newOffsetCP];

    edit->setIMEConversion(conv);

    postRedraw();
}

void X11Window::imeDone()
{
}

bool X11Window::isEditing() const { return (mImpl->textEditor != nullptr); }

bool X11Window::isShowing() const { return mImpl->showing; }

void X11Window::show(bool show,
                     std::function<void(const DrawContext&)> onWillShow)
{
    if (show) {
        if (!mImpl->showing && onWillShow) {
            onWillShow(*mImpl->dc);
        }
        // If this is a popup window it requires being set as transient,
        // which on X11 (and not Win32 or macOS) requires the window is is
        // transient for.
        if (mImpl->flags & Window::Flags::kPopup) {
            if (gActiveWindow) {
                XSetTransientForHint(mImpl->display, mImpl->xwindow,
                                     (::Window)gActiveWindow->nativeHandle());
            }
            // Set override-direct, which allows the transient window to grab
            // them mouse, also is required to have no titlebar (despite being
            // transient and having _NET_WM_WINDOW_TYPE as popup menu).
            XSetWindowAttributes wa;
            wa.override_redirect = True;
            XChangeWindowAttributes(mImpl->display, mImpl->xwindow,
                                    CWOverrideRedirect, &wa);
        }

        XMapRaised(mImpl->display, mImpl->xwindow);
        gActiveWindow = this;
        // Mapping determines which screen we are on
        mImpl->updateDrawContext();
    } else {
        XUnmapWindow(mImpl->display, mImpl->xwindow);
    }
    mImpl->showing = show;
}

void X11Window::toggleMinimize()
{
    if (hasWMProperty(mImpl->display, mImpl->xwindow, kNetWMHidden)) {
        XMapRaised(mImpl->display, mImpl->xwindow);
    } else {
        XIconifyWindow(mImpl->display, mImpl->xwindow, mImpl->xscreenNo);
    }
}

void X11Window::toggleMaximize()
{
    int change;
    if (hasWMProperty(mImpl->display, mImpl->xwindow, kNetWMMaximizedVert)
        || hasWMProperty(mImpl->display, mImpl->xwindow, kNetWMMaximizedHorz)) {
        change = _NET_WM_STATE_REMOVE;
    } else {
        change = _NET_WM_STATE_ADD;
    }

    Atom wmState = XInternAtom(mImpl->display, "_NET_WM_STATE", False);
    if (wmState == None) {
        return;
    }

    XClientMessageEvent e;
    memset(&e, 0, sizeof(e));
    e.type = ClientMessage;
    e.window = mImpl->xwindow;
    e.message_type = wmState;
    e.format = 32;
    e.data.l[0] = change;
    e.data.l[1] = XInternAtom(mImpl->display, kNetWMMaximizedVert, False);
    e.data.l[2] = XInternAtom(mImpl->display, kNetWMMaximizedHorz, False);
    e.data.l[3] = 1;  // 1 = normal applications (2 = pagers and others)
    e.data.l[4] = 0;
    XSendEvent(mImpl->display, RootWindow(mImpl->display, mImpl->xscreenNo),
               False, SubstructureRedirectMask | SubstructureNotifyMask,
               (XEvent *)&e);
    XFlush(mImpl->display);
}

void X11Window::close()
{
    if (mImpl->xwindow) {
        if (onWindowShouldClose()) {
            onWindowWillClose();
            XUnmapWindow(mImpl->display, mImpl->xwindow);
        }
    }
}

void X11Window::raiseToTop() const
{
    XRaiseWindow(mImpl->display, mImpl->xwindow);
    gActiveWindow = (X11Window*)this;
}

void X11Window::setTitle(const std::string& title)
{
    mImpl->title = title;
    // WM_NAME uses XTextProperty, which does not support UTF-8, which would
    // require a conversion of some sort. Freedesktop.org indicates
    // _NET_WM_NAME should take precedence so we will use that.
    XChangeProperty(mImpl->display, mImpl->xwindow,
                    XInternAtom(mImpl->display, "_NET_WM_NAME", False),
                    XInternAtom(mImpl->display, "UTF8_STRING", False),
                    8, PropModeReplace, (const unsigned char*)title.c_str(),
                    int(title.size()));
}

void X11Window::setCursor(const Cursor& cursor)
{
    if (auto *osCursor = cursor.osCursor()) {
        osCursor->set((void*)mImpl->xwindow, mImpl->display);
    }
}

Rect X11Window::contentRect() const
{
    return Rect(PicaPt::kZero, PicaPt::kZero,
                PicaPt::fromPixels(mImpl->width, mImpl->dpi),
                PicaPt::fromPixels(mImpl->height, mImpl->dpi));
}

OSWindow::OSRect X11Window::osContentRect() const
{
    return osFrame();
}

void X11Window::setContentSize(const Size& size)
{
    auto f = osFrame();
    setOSFrame(f.x, f.y,
               size.width.toPixels(dpi()), size.height.toPixels(dpi()));
}

float X11Window::dpi() const { return mImpl->dpi; }

OSWindow::OSRect X11Window::osFrame() const
{
    // Note that XGetWindowAttributes has x, y, but they refer to the distance
    // from the outer upper-left of the window to the inside upper left.
    ::Window rootWindow;
    int x, y;
    unsigned int width, height, borderWidth, depth;
    XGetGeometry(mImpl->display, mImpl->xwindow, &rootWindow,
                 &x, &y, &width, &height, &borderWidth, &depth);
    int rootX, rootY;
    ::Window childRet;
    XTranslateCoordinates(mImpl->display, mImpl->xwindow, rootWindow,
                          x, y, &rootX, &rootY, &childRet);

    // XGetGeometry gives the coordinates in terms of the parent, which is
    // not necessarily the root window, if the window manager reparents the
    // window. But if we translate (0, 0), the result is totally wrong. So
    // Subtract off x, y here.
    return OSRect{ float(rootX - x), float(rootY - y),
                   float(width), float(height) };
}

void X11Window::setOSFrame(float x, float y, float width, float height)
{
    // Note that XMoveResizeWindow moves the window to the position specified,
    // and the window manager adjusts the titlebar accordingly. This is not
    // the same as what XGetGeometry gives us, though.
    
    // We just want the root window, but we need to use the current (x, y)
    // to translate the coordinates, because the window has not moved yet,
    // and XTranslateCoordinates seems to fail if the coordinates are outside
    // the window. We can then add in the new (x, y).
    ::Window rootWindow;
    int _x, _y;
    unsigned int _width, _height, borderWidth, depth;
    XGetGeometry(mImpl->display, mImpl->xwindow, &rootWindow,
                 &_x, &_y, &_width, &_height, &borderWidth, &depth);
    int osX, osY;
    ::Window childRet;
    XTranslateCoordinates(mImpl->display, rootWindow, mImpl->xwindow,
                          _x, _y, &osX, &osY, &childRet);
    osX += x;
    osY += y;

    XMoveResizeWindow(mImpl->display, mImpl->xwindow,
                      int(std::round(osX)), int(std::round(osY)),
                      (unsigned int)std::round(width),
                      (unsigned int)std::round(height));
}

PicaPt X11Window::borderWidth() const
{
    XWindowAttributes wa;
    XGetWindowAttributes(mImpl->display, mImpl->xwindow, &wa);
    return PicaPt::fromPixels(wa.border_width, dpi());
}

void X11Window::postRedraw() const
{
    // X does not coalesce exposure events, so avoid sending another event if
    // we haven't redrawn from the first one yet.
    if (!mImpl->drawRequested) {
        mImpl->drawRequested = true;
        XEvent e;
        memset(&e, 0, sizeof(e));
        e.type = Expose;
        e.xexpose.window = mImpl->xwindow;
        XSendEvent(mImpl->display, mImpl->xwindow, False, ExposureMask, &e);
        XFlush(mImpl->display);
    }
}

void X11Window::beginModalDialog(OSWindow *w)
{
    Atom type = XInternAtom(mImpl->display, "_NET_WM_WINDOW_TYPE", False);
    long value = XInternAtom(mImpl->display, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    XChangeProperty(mImpl->display, (::Window)w->nativeHandle(), type, XA_ATOM,
                    32, PropModeReplace, (unsigned char *)&value, 1);
    XSetTransientForHint(mImpl->display, (::Window)w->nativeHandle(),
                         (::Window)this->nativeHandle());

    w->show(true, [](const uitk::DrawContext&) {});

    type = XInternAtom(mImpl->display, "_NET_WM_STATE", False);
    value = XInternAtom(mImpl->display, "_NET_WM_STATE_MODAL", False);
    XChangeProperty(mImpl->display, (::Window)w->nativeHandle(), type, XA_ATOM,
                    32, PropModeReplace, (unsigned char *)&value, 1);
}

void X11Window::endModalDialog(OSWindow *w)
{
    w->show(false, [](const uitk::DrawContext&) {});
}

Point X11Window::currentMouseLocation() const
{
    // Note: XQueryPointer returns False and (x, y) = (0, 0) if the point
    //       is not on the same screen as the window.
    ::Window root, child;
    int rootX, rootY, x, y;
    unsigned int keyModsAndButtons;
    XQueryPointer(mImpl->display, mImpl->xwindow, &root, &child,
                  &rootX, &rootY, &x, &y, &keyModsAndButtons);
    return Point(PicaPt::fromPixels(x, dpi()),
                 PicaPt::fromPixels(y, dpi()));
}

void* X11Window::nativeHandle() { return (void*)mImpl->xwindow; }
IWindowCallbacks& X11Window::callbacks() { return mImpl->callbacks;}

void X11Window::callWithLayoutContext(std::function<void(const DrawContext&)> f)
{
    f(*mImpl->dc);
}

void X11Window::setTextEditing(TextEditorLogic *te, const Rect& frame)
{
    mImpl->textEditor = te;
    mImpl->textRect = frame;
}

void X11Window::onResize()
{
    mImpl->updateDrawContext();
    mImpl->callbacks.onResize(*mImpl->dc);
    // We need to relayout after a resize, but defer until the draw because
    // we are probably going to get another resize event immediately after
    // this one.
    mImpl->needsLayout = true;
}

void X11Window::onLayout()
{
    mImpl->callbacks.onLayout(*mImpl->dc);
    mImpl->needsLayout = false;
}

void X11Window::onDraw()
{
    if (mImpl->needsLayout) {
        onLayout();
    }

    // Reset the draw requested flag, so that requesting an exposure during
    // would work.
    mImpl->drawRequested = false;

    mImpl->callbacks.onDraw(*mImpl->dc);
}

void X11Window::onMouse(MouseEvent& e, int x, int y)
{
    // Marking a popup window as transient (so it acts like a popup window)
    // requires the window it's transienting on. Since the other two platforms
    // do not require this, and it's not clear how to make a clean API that
    // requires the window only for popup windows, we just keep track of the
    // active window and use that in show(). There is not a good way to check
    // what the window stacking order is (and if there were, what do you do
    // for multiple monitors?), but it seems like a mouse click is a reasonable
    // proxy. This will certainly work for popup menus (you have to click to
    // trigger the menu), although it would fail for dialog boxes that appear
    // not as a result of user interaction (such as an error for a long-running
    // operation) if there are multiple windows and the dialog is for the
    // non-active window and nothing has been clicked in the active window.
    gActiveWindow = this;

    e.pos = Point(PicaPt::fromPixels(float(x), mImpl->dpi),
                  PicaPt::fromPixels(float(y), mImpl->dpi));
    mImpl->callbacks.onMouse(e);
}

void X11Window::onKey(const KeyEvent& e)
{
    mImpl->callbacks.onKey(e);
}

void X11Window::onText(const TextEvent& e)
{
    if (mImpl->textEditor) {
        mImpl->textEditor->setIMEConversion(TextEditorLogic::IMEConversion());
    }
    mImpl->callbacks.onText(e);
}

void X11Window::onActivated(const Point& currentMousePos)
{
    XSetICFocus(mImpl->xic);
    mImpl->callbacks.onActivated(currentMousePos);
}

void X11Window::onDeactivated()
{
    mImpl->callbacks.onDeactivated();
}

bool X11Window::onWindowShouldClose()
{
    return mImpl->callbacks.onWindowShouldClose();
}

void X11Window::onWindowWillClose()
{
    mImpl->callbacks.onWindowWillClose();
}

}  // namespace uitk
