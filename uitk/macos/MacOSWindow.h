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

#ifndef UITK_MACOS_WINDOW_H
#define UITK_MACOS_WINDOW_H

#include "../OSWindow.h"

namespace uitk {

class MacOSWindow : public OSWindow
{
public:
    MacOSWindow(IWindowCallbacks& callbacks,
                const std::string& title, int width, int height,
                Window::Flags::Value flags);
    MacOSWindow(IWindowCallbacks& callbacks,
                const std::string& title, int x, int y, int width, int height,
                Window::Flags::Value flags);
    virtual ~MacOSWindow();

    bool isShowing() const override;
    void show(bool show) override;

    void close() override;

    void setTitle(const std::string& title) override;

    Rect contentRect() const override;

    void postRedraw() const override;

    void raiseToTop() const override;

    void* nativeHandle() override;
    IWindowCallbacks& callbacks() override;

public:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_OSWINDOW_H

