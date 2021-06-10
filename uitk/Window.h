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

namespace uitk {

class DrawContext;
class Point;

class MouseEvent;
class Widget;

class IWindowCallbacks
{
public:
    IWindowCallbacks() {}
    virtual ~IWindowCallbacks() {}

    virtual void onResize(const DrawContext& dc) = 0;
    virtual void onLayout(const DrawContext& dc) = 0;
    virtual void onDraw(DrawContext& dc) = 0;
    virtual void onMouse(const MouseEvent& e) = 0;
    virtual void onActivated(const Point& currentMousePos) = 0;
    virtual void onDeactivated() = 0;
};

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
            kNone = 0
        };
    };

    /// Creates a window with default (x, y) position.
    /// width, height are in operating-system coordinates.
    Window(const std::string& title, int width, int height,
           Flags::Value flags = Flags::kNone);
    /// Creates a window. x, y, width, height are in operating-system
    /// coordinates.
    Window(const std::string& title, int x, int y, int width, int height,
           Flags::Value flags = Flags::kNone);
    virtual ~Window();

    bool isShowing() const;
    Window* show(bool show);

    void close();

    std::string title() const;
    Window* setTitle(const std::string& title);

    /// Adds the child to the Window. Returns pointer this window.
    Window* addChild(Widget *child);

    /// Posts a redraw message to the event loop scheduling a redraw.
    /// Note that this does not immediately redraw the window, that only
    /// happens when the event loop handles the redraw event.
    void postRedraw() const;

    void raiseToTop() const;

    void* nativeHandle();

public:
    void onResize(const DrawContext& dc) override;
    void onLayout(const DrawContext& dc) override;
    void onDraw(DrawContext& dc) override;
    void onMouse(const MouseEvent& e) override;
    void onActivated(const Point& currentMousePos) override;
    void onDeactivated() override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace uitk

#endif // UITK_WINDOW_H
