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

#include "Dialog.h"

#include "Application.h"
#include "Button.h"
#include "Events.h"
#include "Label.h"
#include "UIContext.h"
#include "Window.h"
#include "themes/Theme.h"

#if defined(__APPLE__)
#include "macos/MacOSDialog.h"
#elif defined(_WIN32) || defined(_WIN64)
#include "win32/Win32Dialog.h"
#else
#endif

namespace uitk {

namespace {
class Alert : public Dialog
{
    using Super = Dialog;
public:
    Alert(const std::string& message, const std::string& info)
    {
        mMessage = new Label(message);
        mMessage->setWordWrapEnabled(true);
        addChild(mMessage);

        mInfo = new Label(info);
        mInfo->setWordWrapEnabled(true);
        mInfo->setFont(mInfo->font().fontWithScaledPointSize(0.85f));
        addChild(mInfo);
    }

    void addButton(const std::string& text)
    {
        auto *b = new Button(text);
        b->setOnClicked([this](Button *b) { this->onButton(b); });
        addChild(b);
        mButtons.push_back(b);
    }

    void showModal(Window *w, std::function<void(Dialog::Result, int)> onDone) override
    {
        if (mButtons.empty()) {
            addButton("Ok");
        }
        setAsDefaultButton(mButtons[0]);
        mButtons[0]->setDrawStyle(Button::DrawStyle::kDialogDefault);
        Super::showModal(w, onDone);
    }

    void onButton(Button *b)
    {
        for (size_t i = 0;  i < mButtons.size();  ++i) {
            if (mButtons[i] == b) {
                if (i == 1) {
                    cancel();
                } else {
                    finish(int(i));
                }
            }
        }
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        auto em = context.theme.params().labelFont.pointSize();
        const auto margin = context.theme.params().dialogMargins;

        // Text is most readable between 60 and 80 characters,
        // which is roughly 40 - 45 ems.
        auto sixtyChars = 40.0f * em;
        auto messagePref = mMessage->preferredSize(context.withWidth(sixtyChars));
        auto infoPref = mInfo->preferredSize(context.withWidth(sixtyChars));
        auto buttonWidth = PicaPt::kZero;
        for (auto *b : mButtons) {
            buttonWidth += std::max(6.0f * em, b->preferredSize(context).width) + em;
        }
        if (!mButtons.empty()) {
            buttonWidth -= em;
        }
        if (mButtons.size() > 2) {
            buttonWidth += 2.0f * em;
        }

        auto w = std::min(sixtyChars,
                          std::max(std::min(sixtyChars, messagePref.width),
                                   std::min(sixtyChars, infoPref.width)));
        w = std::max(w, buttonWidth);
        w = std::max(20.0f * em, w);
        auto h = (mMessage->text().empty()
                    ? PicaPt::kZero
                    : mMessage->preferredSize(context.withWidth(w)).height) +
                 (mInfo->text().empty()
                    ? PicaPt::kZero
                    : em + mInfo->preferredSize(context.withWidth(w)).height) +
                 margin +
                 (mButtons.empty() ? em : mButtons[0]->preferredSize(context).height);
        return Size(w + 2.0f * margin, h + 2.0f * margin);
    }

    void layout(const LayoutContext& context) override
    {
        auto em = context.theme.params().labelFont.pointSize();
        const auto margin = context.theme.params().dialogMargins;
        auto w = bounds().width - 2.0f * margin;

        if (!mMessage->text().empty()) {
            auto pref = mMessage->preferredSize(context.withWidth(w));
            mMessage->setFrame(Rect(margin, margin, w, pref.height));
        } else {
            mMessage->setFrame(Rect(margin, margin, PicaPt::kZero, PicaPt::kZero));
        }
        if (!mInfo->text().empty()) {
            auto pref = mInfo->preferredSize(context.withWidth(w));
            mInfo->setFrame(Rect(margin, mMessage->frame().maxY() + em, w, pref.height));
        } else {
            mInfo->setFrame(Rect(margin, mMessage->frame().maxY(), PicaPt::kZero, PicaPt::kZero));
        }
        PicaPt x = bounds().maxX() - margin;
        for (size_t i = 0;  i < mButtons.size();  ++i) {
            auto *b = mButtons[i];
            Size pref = b->preferredSize(context);
            pref.width = std::max(6.0f * em, pref.width);
            x -= pref.width;
            b->setFrame(Rect(x, mInfo->frame().maxY() + margin, pref.width, pref.height));
            if (i == 1) {
                x -= 2.0f * em;
            }
            x -= em;
        }
        Super::layout(context);
    }

private:
    Label *mMessage;
    Label *mInfo;
    std::vector<Button*> mButtons;
};

}  // namespace

//-----------------------------------------------------------------------------
void Dialog::showAlert(Window *w,
                      const std::string& title,
                      const std::string& message,
                      const std::string& info)
{
    Dialog::showAlert(w, title, message, info, {"Ok"}, [](Dialog::Result, int) {});
}

void Dialog::showAlert(Window *w,
                      const std::string& title,
                      const std::string& message,
                      const std::string& info,
                      const std::vector<std::string>& buttons,
                      std::function<void(Dialog::Result, int)> onDone)
{
    if (Application::instance().supportsNativeDialogs()) {
#if defined(__APPLE__)
        MacOSDialog::showAlert(w, title, message, info, buttons, onDone);
#elif defined(_WIN32) || defined(_WIN64)
        Win32Dialog::showAlert(w, title, message, info, buttons, onDone);
#else
        assert(false);
#endif
    } else {
        auto *dlg = new Alert(message, info);
        dlg->setTitle(title);
        for (auto &b : buttons) {
            dlg->addButton(b);
        }
        dlg->showModal(w, [dlg, onDone](Dialog::Result r, int idx) {
            if (onDone) {
                onDone(r, idx);
            }
            delete dlg;
        });
        Application::instance().beep();
    }
}

//-----------------------------------------------------------------------------
struct Dialog::Impl
{
    std::string title;
    Window *owningWindow = nullptr;  // we do not own this
    Window *ourWindow = nullptr;  // we own this (if not null)
    std::function<void(Result, int)> onDone;
    Button *defaultButton = nullptr;  // we do not own this
};

Dialog::Dialog()
    : mImpl(new Impl())
{
}

Dialog::~Dialog()
{
    if (mImpl->ourWindow) {  // really shouldn't get here
        mImpl->ourWindow->removeChild(this);  // we own ourselves now
        mImpl->ourWindow->close();
        mImpl->ourWindow->deleteLater();
    }
}

const std::string& Dialog::title() const { return mImpl->title; }

Dialog* Dialog::setTitle(const std::string& title)
{
    mImpl->title = title;
    return this;
}

void Dialog::setAsDefaultButton(Button *button)
{
    mImpl->defaultButton = button;
    button->setDrawStyle(Button::DrawStyle::kDialogDefault);
}

void Dialog::showModal(Window *w, std::function<void(Result, int)> onDone)
{
    mImpl->onDone = onDone;
    if (w) {
        if (w->beginModalDialog(this)) {
            mImpl->owningWindow = w;
        }
    } else {
        auto title = mImpl->title;
        if (title.empty()) {
            title = Application::instance().applicationName();
        }
        mImpl->ourWindow = new Window(title, 0, 0, 640, 480, Window::Flags::kDialog);
        mImpl->ourWindow->addChild(this);  // we do not own ourselves now, the window does
        mImpl->ourWindow->setOnWindowWillClose([this](Window &w) { w.removeChild(this); });
        mImpl->ourWindow->resizeToFit();
        mImpl->ourWindow->centerInScreen();
        mImpl->ourWindow->show(true);
    }
}

void Dialog::finish(int value)
{
    if (mImpl->owningWindow) {
        mImpl->owningWindow->endModalDialog();
    }
    if (mImpl->ourWindow) {
        mImpl->ourWindow->close();
        mImpl->ourWindow->deleteLater();
        mImpl->ourWindow = nullptr;
    }
    if (mImpl->onDone) {
        // We are probably in an event handler (of a button, most likely),
        // so post the callback, in case it does something interesting,
        // like delete the object because everything is complete.
        Application::instance().scheduleLater(nullptr, [this, value]() {
            mImpl->onDone(Result::kFinished, value);
        });
    }
}

void Dialog::cancel()
{
    if (mImpl->owningWindow) {
        mImpl->owningWindow->endModalDialog();
    }
    if (mImpl->ourWindow) {
        mImpl->ourWindow->close();
        mImpl->ourWindow->deleteLater();
        mImpl->ourWindow = nullptr;
    }
    if (mImpl->onDone) {
        Application::instance().scheduleLater(nullptr, [this]() {
            mImpl->onDone(Result::kCancelled, 0);
        });
    }
}

Size Dialog::preferredSize(const LayoutContext& context) const
{
    // If we only have one child, it must be a layout (what use is a dialog
    // with only one widget?), in which case
    auto &childs = children();
    if (childs.size() == 1) {
        Size size;
        for (auto *child : childs) {
            auto pref = child->preferredSize(context);
            size.width = std::max(size.width, pref.width);
            size.height = std::max(size.height, pref.height);
        }
        if (size.width <= PicaPt::kZero) {
            size.width = kDimGrow;
        }
        if (size.height <= PicaPt::kZero) {
            size.height = kDimGrow;
        }
        return size;
    } else {
        return Super::preferredSize(context);
    }
}

void Dialog::layout(const LayoutContext& context)
{
    // If we only have one child, it must be a layout (what use is a dialog
    // with only one widget?), in which case
    auto &childs = children();
    if (childs.size() == 1) {
        childs[0]->setFrame(bounds());
    }

    Super::layout(context);
}

Widget::EventResult Dialog::key(const KeyEvent& e)
{
    if (e.key == Key::kReturn || e.key == Key::kEnter) {
        if (e.type == KeyEvent::Type::kKeyDown) {
            if (mImpl->defaultButton) {
                mImpl->defaultButton->performClick();
            }
        }
        return EventResult::kIgnored;
    } else if (e.key == Key::kEscape) {
        if (e.type == KeyEvent::Type::kKeyDown) {
            cancel();
        }
        return EventResult::kIgnored;
    } else {
        return Super::key(e);
    }
}

}  // namespace uitk
