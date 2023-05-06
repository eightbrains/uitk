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

#ifndef UITK_X11_WINDOW_H
#define UITK_X11_WINDOW_H

#include "../OSWindow.h"
#include "../Window.h" // for Window::Flags

#include <memory>

namespace uitk {

class X11Window : public OSWindow
{
public:
    X11Window(IWindowCallbacks& callbacks,
              const std::string& title, int width, int height,
              Window::Flags::Value flags);
    X11Window(IWindowCallbacks& callbacks,
              const std::string& title, int x, int y, int width, int height,
              Window::Flags::Value flags);
    ~X11Window();

    bool isShowing() const override;
    void show(bool show,
              std::function<void(const DrawContext&)> onWillShow) override;
    void toggleMinimize() override;
    void toggleMaximize() override;

    void close() override;

    void raiseToTop() const override;

    void setTitle(const std::string& title) override;

    void setCursor(const Cursor& cursor) override;

    Rect contentRect() const override;
    void setContentSize(const Size& size) override;
    OSRect osContentRect() const override;

    float dpi() const override;
    OSRect osFrame() const override;
    void setOSFrame(float x, float y, float width, float height) override;

    OSScreen osScreen() const;

    PicaPt borderWidth() const override;

    void postRedraw() const override;

    void beginModalDialog(OSWindow *w) override;
    void endModalDialog(OSWindow *w) override;

    Point currentMouseLocation() const override;

    void* nativeHandle() override;
    IWindowCallbacks& callbacks() override;
    void callWithLayoutContext(std::function<void(const DrawContext&)> f) override;
    void setTextEditing(TextEditorLogic *te, const Rect& frame) override;

    void setNeedsAccessibilityUpdate() override;
    void setAccessibleElements(const std::vector<AccessibilityInfo>& elements) override;

    void onResize();
    void onLayout();
    void onDraw();
    void onMouse(MouseEvent& e, int x, int y);
    void onKey(const KeyEvent& e);
    void onText(const TextEvent& e);
    void onActivated(const Point& currentMousePos);
    void onDeactivated();
    bool onWindowShouldClose();
    void onWindowWillClose();

public:
    void* xic() const;
    bool isEditing() const;
    int imeMove(int ximDir, int arg);
    void imeUpdate(const char *utf8, int start, int len, int newOffset);
    void imeDone();

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_X11_WINDOW_H
