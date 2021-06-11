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

namespace uitk {

struct X11Window::Impl {
    IWindowCallbacks& callbacks;
    Display *display;
    ::Window xwindow = 0;
    int width;
    int height;
    float dpi;
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
    X11Application& x11app = static_cast<X11Application&>(Application::instance().osApplication());
    Display *display = static_cast<Display*>(x11app.display());
    mImpl->display = display;
    mImpl->xwindow = XCreateSimpleWindow(display, XDefaultRootWindow(display),
                                         x, y, width, height, 1, 0, 0);
    x11app.registerWindow(mImpl->xwindow, this);

    // Tell the window manager we want to be notified when a window is closed,
    // otherwise X will just kill our connection.
    Atom wmDeleteMsg = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, mImpl->xwindow, &wmDeleteMsg, 1);
        
    setTitle(title);

    // Now that we are ready to display the window, turn on receiving events
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

    show(true);  // also creates draw context
}

X11Window::~X11Window()
{
    if (mImpl->xwindow) {
        close();
    }
}

bool X11Window::isShowing() const { return mImpl->showing; }

void X11Window::show(bool show)
{
    if (show) {
        XMapRaised(mImpl->display, mImpl->xwindow);
        // Mapping determines which screen we are on
        mImpl->updateDrawContext();
    } else {
        XUnmapWindow(mImpl->display, mImpl->xwindow);
    }
    mImpl->showing = show;
}

void X11Window::close()
{
    mImpl->dc = nullptr;
    XDestroyWindow(mImpl->display, mImpl->xwindow);
    mImpl->xwindow = 0;
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
}

void* X11Window::nativeHandle()
{
    return (void*)mImpl->xwindow;
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
    e.pos = Point(PicaPt::fromPixels(float(x), mImpl->dpi),
                  PicaPt::fromPixels(float(y), mImpl->dpi));
    mImpl->callbacks.onMouse(e);
}

void X11Window::onActivated(const Point& currentMousePos)
{
}

void X11Window::onDeactivated()
{
}

}  // namespace uitk
