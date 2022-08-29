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

class ClickCounter;

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
    void toggleMinimize() override;
    void toggleMaximize() override;

    void close() override;

    void raiseToTop() const override;

    void setTitle(const std::string& title) override;

    void setCursor(const Cursor& cursor) override;

    Rect contentRect() const override;
    OSRect osContentRect() const override;
    void setContentSize(const Size& size) override ;

    float dpi() const override;
    OSRect osFrame() const override;
    void setOSFrame(float x, float y, float width, float height) override;

    OSScreen osScreen() const;

    void postRedraw() const override;

    void beginModalDialog(OSWindow* w) override;
    void endModalDialog(OSWindow* w) override;

    PicaPt borderWidth() const override;

    Point currentMouseLocation() const override;

    void* nativeHandle() override;
    IWindowCallbacks& callbacks() override;
    void callWithLayoutContext(std::function<void(const DrawContext&)> f) override;

    // frame is in window coordinates
    void setTextEditing(TextEditorLogic* te, const Rect& frame) override;

    // Since each window has its own copy of the menus, if the structure
    // of the menus changes, it needs to be reloaded.
    void setNeedsUpdateMenubar();

public:  // these are public so that WndProc (which cannot be a member function) can use them
    ClickCounter& clickCounter();
    void updateIMEText(long lParam); // do NOT use LPARAM, we want to keep windows.h out of this!
    void applyIMEText();
    void cancelIMEText();
    void showIMEWindow();
    void hideIMEWindow();
    void updateCursor() const;
    bool menubarNeedsUpdate() const;
    void updateMenubar();

    void onMoved();
    void onResize();
    void onLayout();
    void onDraw();
    void onMouse(MouseEvent& e, int x, int y);
    void onKey(const KeyEvent& e);
    void onText(const TextEvent& e);
    void onActivated(const Point& currentMousePos);
    void onDeactivated();
    void onMenuWillShow();
    void onMenuActivated(MenuId id);
    bool onWindowShouldClose();
    void onWindowWillClose();

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_WIN32_WINDOW_H
