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

#ifndef UITK_OSWINDOW_H
#define UITK_OSWINDOW_H

#include <nativedraw.h>

#include <string>

namespace uitk {

class MouseEvent;

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

class OSWindow
{
public:
    virtual ~OSWindow() {}

    virtual bool isShowing() const = 0;
    virtual void show(bool show) = 0;

    virtual void close() = 0;

    virtual void setTitle(const std::string& title) = 0;

    virtual Rect contentRect() const = 0;

    virtual void postRedraw() const = 0;

    virtual void raiseToTop() const = 0;

    virtual void* nativeHandle() = 0;
    virtual IWindowCallbacks& callbacks() = 0;
};

}  // namespace uitk
#endif // UITK_OSWINDOW_H
