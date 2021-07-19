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

#ifndef UITK_WINDOW_H
#define UITK_WINDOW_H

#include <memory>
#include <string>

#include "OSWindow.h"

namespace uitk {

class DrawContext;
struct Point;

struct MouseEvent;
class Widget;
struct LayoutContext;
class PopupMenu;

class Window : public IWindowCallbacks
{
    // Design notes:
    // Q: Why not make creation a factory function on Application?
    // A: That would prevent users from inheriting from Window, which is
    //    useful.
    // Q: Well, that forces users to manually manage an object whose contents
    //    (the native window) belongs more to Application than Window.
    // A: True, although native windows need to be manually managed, too.
public:
    struct Flags {  // the struct provides scoping like enum class
        enum Value {
            kNormal = 0,
            kPopup = (1 << 0)
        };
    };

    /// Creates a window with default (x, y) position.
    /// width, height are in operating-system coordinates. The window is not
    /// shown.
    Window(const std::string& title, int width, int height,
           Flags::Value flags = Flags::kNormal);
    /// Creates a window. x, y, width, height are in operating-system
    /// coordinates. The window is not shown.
    Window(const std::string& title, int x, int y, int width, int height,
           Flags::Value flags = Flags::kNormal);
    virtual ~Window();

    bool isShowing() const;
    Window* show(bool show);

    void close();

    /// Schedules the window for deletion at a point in the event loop where
    /// it is safe. This function is safe to call in the setOnWindowWillClose
    /// callback. (Delete is not safe, as it may delete lambda that is executing.)
    void deleteLater();

    std::string title() const;
    Window* setTitle(const std::string& title);

    /// Resizes the window so that the content rect is of the specified size.
    /// The actual window may be larger due to the title bar (if the size of the
    /// window includes the title bar on the OS) and the menubar (if a menubar is
    /// specified and if the OS includes the menubar as part of the window).
    /// Use setOSFrame if you need to set the size of the actual window (this is
    /// not normally helpful).
    void resize(const Size& contentSize);
    void move(const PicaPt& dx, const PicaPt& dy);

    OSWindow::OSRect osFrame() const;
    /// Sets the window rectangle of the operating system's window, in operating
    /// system coordinates (pixels in Windows and X11, virtual-pixels in macOS).
    /// Note that this is NOT the content rect, as the operating system may include
    /// titlebars and/or menubars in the window area. resize() is probably a
    /// more convenient function if you need to change the window's size.
    void setOSFrame(float x, float y, float width, float height);

    /// Returns the point in the operating system's window manager of the
    /// point passed it. This is useful for positioning popup windows.
    OSWindow::OSPoint convertWindowToOSPoint(const Point& windowPt) const;

    /// Adds the child to the Window. Returns pointer this window.
    Window* addChild(Widget *child);

    /// Schedules a redraw.
    void setNeedsDraw();

    void raiseToTop() const;

    PicaPt borderWidth() const;
    
    void* nativeHandle();

    void setOnWindowWillShow(std::function<void(Window& w, const LayoutContext& context)> onWillShow);
    void setOnWindowDidDeactivate(std::function<void(Window& w)> onDidDeactivate);
    void setOnWindowShouldClose(std::function<bool(Window& w)> onShouldClose);
    void setOnWindowWillClose(std::function<void(Window& w)> onWillClose);

public:
    /// Directs mouse events directly to the widget specified until mouse up.
    /// Useful for when a widget needs to capture mouse drag events outside
    /// its frame.
    void setMouseGrab(Widget *w);
    Widget* mouseGrabWidget() const;

    // On macOS windows without a titlebar do not get activated/deactivated
    // messages, so we need to register the popup menu
    void setPopupMenu(PopupMenu *menu);

    void onResize(const DrawContext& dc) override;
    void onLayout(const DrawContext& dc) override;
    void onDraw(DrawContext& dc) override;
    void onMouse(const MouseEvent& e) override;
    void onActivated(const Point& currentMousePos) override;
    void onDeactivated() override;
    bool onWindowShouldClose() override;
    void onWindowWillClose() override;

protected:
    /// Posts a redraw message to the event loop scheduling a redraw.
    /// Note that this does not immediately redraw the window, that only
    /// happens when the event loop handles the redraw event.
    void postRedraw() const;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace uitk

#endif // UITK_WINDOW_H
