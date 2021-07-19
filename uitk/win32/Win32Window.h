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

#ifndef UITK_WIN32_WINDOW_H
#define UITK_WIN32_WINDOW_H

#include "../OSWindow.h"
#include "../Window.h"

namespace uitk {

class Win32Window : public OSWindow
{
public:
    Win32Window(IWindowCallbacks& callbacks,
                const std::string& title, int width, int height,
                Window::Flags::Value flags);
    Win32Window(IWindowCallbacks& callbacks,
                const std::string& title, int x, int y, int width, int height,
                Window::Flags::Value flags);
    ~Win32Window();

    bool isShowing() const override;
    void show(bool show,
              std::function<void(const DrawContext&)> onWillShow) override;

    void close() override;

    void setTitle(const std::string& title) override;

    Rect contentRect() const override;
    OSRect osContentRect() const override;

    float dpi() const override;
    OSRect osFrame() const override;
    void setOSFrame(float x, float y, float width, float height) override;

    void postRedraw() const override;

    void raiseToTop() const override;

    PicaPt borderWidth() const override;

    Point currentMouseLocation() const override;

    void* nativeHandle() override;
    IWindowCallbacks& callbacks() override;

public:
    void onMoved();
    void onResize();
    void onLayout();
    void onDraw();
    void onMouse(MouseEvent& e, int x, int y);
    void onActivated(const Point& currentMousePos);
    void onDeactivated();
    bool onWindowShouldClose();
    void onWindowWillClose();

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_WIN32_WINDOW_H
