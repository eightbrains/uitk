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

#include "../Application.h"
#include "../Events.h"
#include "X11Application.h"

#include <string.h>  // for memset()
#include <X11/Xlib.h>
#include <X11/Xatom.h>

namespace uitk {

static X11Window* gActiveWindow = nullptr;
    
struct X11Window::Impl {
    IWindowCallbacks& callbacks;
    Display *display;
    ::Window xwindow = 0;
    int width;
    int height;
    int flags = 0;
    float dpi = 96.0f;
    std::shared_ptr<DrawContext> dc;
    std::string title;
    bool showing = false;

    bool drawRequested = false;
    bool needsLayout = true;

    void updateDrawContext()
    {
        XWindowAttributes attrs;
        XGetWindowAttributes(this->display, this->xwindow, &attrs);
        this->width = attrs.width;
        this->height = attrs.height;
        int screenNo = XScreenNumberOfScreen(attrs.screen);
        X11Application& x11app = static_cast<X11Application&>(Application::instance().osApplication());
        this->dpi = x11app.dpiForScreen(screenNo);

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

void X11Window::close()
{
    if (mImpl->xwindow) {
        if (onWindowShouldClose()) {
            onWindowWillClose();
            mImpl->destroyWindow();
        }
    }
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

void X11Window::raiseToTop() const
{
    XRaiseWindow(mImpl->display, mImpl->xwindow);
    gActiveWindow = (X11Window*)this;
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
    // for multiple montiors?), but it seems like a mouse click is a reasonable
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
    mImpl->callbacks.onText(e);
}

void X11Window::onActivated(const Point& currentMousePos)
{
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
