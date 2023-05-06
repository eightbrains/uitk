//-----------------------------------------------------------------------------
// Copyright 2021 - 2022 Eight Brains Studios, LLC
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

#ifndef UITK_OSWINDOW_H
#define UITK_OSWINDOW_H

#include "OSMenu.h"
#include "Accessibility.h"

#include <nativedraw.h>

#include <functional>
#include <string>

namespace uitk {

struct KeyEvent;
struct MouseEvent;
struct TextEvent;
class TextEditorLogic;

class Cursor;

class IWindowCallbacks
{
public:
    IWindowCallbacks() {}
    virtual ~IWindowCallbacks() {}

    virtual void onResize(const DrawContext& dc) = 0;
    virtual void onLayout(const DrawContext& dc) = 0;
    virtual void onDraw(DrawContext& dc) = 0;
    virtual void onMouse(const MouseEvent& e) = 0;
    virtual void onKey(const KeyEvent& e) = 0;
    virtual void onText(const TextEvent& e) = 0;
    virtual void onActivated(const Point& currentMousePos) = 0;
    virtual void onDeactivated() = 0;
    virtual void onMenuWillShow() = 0;
    virtual void onMenuActivated(MenuId id) = 0;
    virtual void onThemeChanged() = 0;
    virtual void onUpdateAccessibility() = 0;
    virtual bool onWindowShouldClose() = 0;
    virtual void onWindowWillClose() = 0;
};

struct OSPoint
{
    float x;
    float y;
};

struct OSRect
{
    float x;
    float y;
    float width;
    float height;
};

struct OSScreen
{
    /// This is the normal area where windows can go (excludes menubar in macOS,
    /// system bar on Windows, etc.)
    OSRect desktopFrame;
    /// This is the actual area of the monitor (what a fullscreen window gets)
    OSRect fullscreenFrame;
    float dpi;
};

class OSWindow
{
public:
    virtual ~OSWindow() {}

    virtual bool isShowing() const = 0;
    // Q: Why not call onWillShow in Window instead of forcing the logic duplicated
    //    on each platform?
    // A: The lifetime of the DrawContext can only be properly controlled
    //    from the platform side.
    virtual void show(bool show, std::function<void(const DrawContext&)> onWillShow) = 0;
    virtual void toggleMinimize() = 0;
    virtual void toggleMaximize() = 0;

    virtual void close() = 0;

    virtual void raiseToTop() const = 0;

    virtual void setTitle(const std::string& title) = 0;

    virtual void setCursor(const Cursor& cursor) = 0;

    // This is the drawable rectangle. It may or may not have upper left at (0, 0)
    virtual Rect contentRect() const = 0;
    // This sets the size of the content area;  the actual OS frame will
    // probably be larger.
    virtual void setContentSize(const Size& size) = 0;

    // This is the contentRect in OS coordinates, same as osFrame().
    virtual OSRect osContentRect() const = 0;

    virtual float dpi() const = 0;
    virtual OSRect osFrame() const = 0;
    virtual void setOSFrame(float x, float y, float width, float height) = 0;

    virtual PicaPt borderWidth() const = 0;

    virtual OSScreen osScreen() const = 0;

    virtual void postRedraw() const = 0;

    virtual void beginModalDialog(OSWindow *w) = 0;
    virtual void endModalDialog(OSWindow *w) = 0;

    // The current mouse location, in window coordinates. Note that the current
    // mouse location may not actually be in this window;  the window might not
    // even be active.
    virtual Point currentMouseLocation() const = 0;

    virtual void* nativeHandle() = 0;
    virtual IWindowCallbacks& callbacks() = 0;
    virtual void callWithLayoutContext(std::function<void(const DrawContext&)> f) = 0;

    // Frame is in window coordinates
    virtual void setTextEditing(TextEditorLogic *te, const Rect& frame) = 0;

    virtual void setNeedsAccessibilityUpdate() = 0;
    virtual void setAccessibleElements(const std::vector<AccessibilityInfo>& elements) = 0;
};

}  // namespace uitk
#endif // UITK_OSWINDOW_H
