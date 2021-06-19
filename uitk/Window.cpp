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

#include "Window.h"

#include "Application.h"
#include "Events.h"
#include "OSWindow.h"
#include "UIContext.h"
#include "Widget.h"
#include "themes/Theme.h"
#include "themes/EmpireTheme.h"

#if defined(__APPLE__)
#include "macos/MacOSWindow.h"
#elif defined(_WIN32) || defined(_WIN64)  // _WIN32 covers everything except 64-bit ARM
#include "win32/Win32Window.h"
#else
#include "x11/X11Window.h"
#endif

#include <nativedraw.h>

namespace uitk {

struct Window::Impl
{
    std::shared_ptr<Theme> theme;
    std::unique_ptr<OSWindow> window;
    std::string title;
    std::unique_ptr<Widget> rootWidget;
    bool inResize = false;
    bool inMouse = false;
    bool needsDraw = false;
};

Window::Window(const std::string& title, int width, int height,
               Flags::Value flags /*= Flags::kNone*/)
    : Window(title, -1, -1, width, height, flags)
{
}

Window::Window(const std::string& title, int x, int y, int width, int height,
               Flags::Value flags /*= Flags::kNone*/)
    : mImpl(new Impl())
{
    // Create theme before the window, in case a draw is requested immediately
    // on creation (as it is on Win32).
    mImpl->theme = Application::instance().theme();
    mImpl->rootWidget = std::make_unique<Widget>();
    mImpl->rootWidget->setWindow(this);

#if defined(__APPLE__)
    mImpl->window = std::make_unique<MacOSWindow>(*this, title, x, y, width, height, flags);
#elif defined(_WIN32) || defined(_WIN64)
    mImpl->window = std::make_unique<Win32Window>(*this, title, x, y, width, height, flags);
#else
    mImpl->window = std::make_unique<X11Window>(*this, title, x, y, width, height, flags);
#endif
}

Window::~Window()
{
    mImpl->theme.reset();
    mImpl->window.reset();
    mImpl->rootWidget.reset();
}

void* Window::nativeHandle() { return mImpl->window->nativeHandle(); }

bool Window::isShowing() const { return mImpl->window->isShowing(); }

Window* Window::show(bool show)
{
    mImpl->window->show(show);
    return this;
}

void Window::close() { mImpl->window->close(); }

std::string Window::title() const { return mImpl->title; }

Window* Window::setTitle(const std::string& title)
{
    mImpl->title = title;
    mImpl->window->setTitle(title);
    return this;
}

void Window::setNeedsDraw()
{
    if (mImpl->inMouse || mImpl->inResize) {
        mImpl->needsDraw = true;
    } else {
        postRedraw();
    }
}

void Window::postRedraw() const { mImpl->window->postRedraw(); }

void Window::raiseToTop() const { mImpl->window->raiseToTop(); }

Window* Window::addChild(Widget *child)
{
    mImpl->rootWidget->addChild(child);
    return this;
}

void Window::onMouse(const MouseEvent& e)
{
    mImpl->inMouse = true;

    mImpl->rootWidget->setState(Theme::WidgetState::kMouseOver);  // so onDeactivated works
    mImpl->rootWidget->mouse(e);

    mImpl->inMouse = false;
    if (mImpl->needsDraw) {
        postRedraw();
        mImpl->needsDraw = false;
    }
}

void Window::onResize(const DrawContext& dc)
{
    if (!mImpl->window) {
        return;
    }

    mImpl->inResize = true;
    auto contentRect = mImpl->window->contentRect();
    mImpl->rootWidget->setFrame(Rect(PicaPt::kZero, PicaPt::kZero,
                                     contentRect.width, contentRect.height));
    onLayout(dc);
    mImpl->inResize = false;
}

void Window::onLayout(const DrawContext& dc)
{
    LayoutContext context = { *mImpl->theme, dc };
    mImpl->rootWidget->layout(context);
}

void Window::onDraw(DrawContext& dc)
{
    UIContext context { *mImpl->theme, dc };
    auto size = Size(PicaPt::fromPixels(float(dc.width()), dc.dpi()),
                     PicaPt::fromPixels(float(dc.height()), dc.dpi()));
    dc.beginDraw();
    mImpl->theme->drawWindowBackground(context, size);
    mImpl->rootWidget->draw(context);
    dc.endDraw();

    mImpl->needsDraw = false;  // should be false anyway, just in case
}

void Window::onActivated(const Point& currentMousePos)
{
    // If mouse is over the window when it is activated (especially by
    // Alt-Tab), send an artifical mouse move event, so that if the mouse
    // is over a control it will be properly highlighted.
    auto frame = mImpl->window->contentRect();
    if (frame.contains(currentMousePos)) {
        MouseEvent me;
        me.pos = currentMousePos;
        me.type = MouseEvent::Type::kMove;
        onMouse(me);
    }
}

void Window::onDeactivated()
{
    mImpl->rootWidget->mouseExited();
    // TODO: don't draw if nothing has changed (i.e. state())
    postRedraw();
}

}  // namespace uitk
