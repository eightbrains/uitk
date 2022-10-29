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

#include "PopupWindow.h"

#include "Application.h"

namespace uitk {

struct PopupWindow::Impl
{
    Window *parent = nullptr;  // we do not own this
    std::function<void()> onDone;
};

PopupWindow::PopupWindow(const PicaPt& w, const PicaPt& h, const std::string& title /*= ""*/)
    : Window(title, w, h, Window::Flags::Value(Window::Flags::kPopup))
    , mImpl(new Impl())
{
    this->setOnWindowWillClose([this](Window &) {
        this->deleteLater();
    });
}

void PopupWindow::cancel()
{
    if (mImpl->parent) {
        mImpl->parent->setPopupWindow(nullptr);
    }
    this->close();
}

Window* PopupWindow::window() { return this; }

void PopupWindow::showPopup(Window *parent, int osX, int osY)
{
    mImpl->parent = parent;
    auto osRect = this->osFrame();
    if (!Application::instance().isOriginInUpperLeft()) {
        osY -= int(std::round(osRect.height));
    }
    this->setOSFrame(osX, osY, int(std::round(osRect.width)), int(std::round(osRect.height)));
    mImpl->parent->setPopupWindow(this);
    this->show(true);
}

} // namespace uitk
