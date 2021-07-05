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

#include "Button.h"

#include "Label.h"
#include "Events.h"
#include "UIContext.h"
#include "themes/Theme.h"

namespace uitk {

struct Button::Impl {
    Label *label;
    bool isOn = false;
    bool isToggleable = false;
    std::function<void(Button*)> onClicked = nullptr;
};

Button::Button(const std::string& text)
    : mImpl(new Impl())
{
    mImpl->label = new Label(text);
    mImpl->label->setAlignment(Alignment::kCenter);
    addChild(mImpl->label);  // takes ownership
}

Button::~Button()
{
    // Super owns mImpl->label
}

bool Button::toggleable() const { return mImpl->isToggleable; }

Button* Button::setToggleable(bool toggleable) {
    mImpl->isToggleable = toggleable;
    return this;
}

bool Button::isOn() const { return mImpl->isOn; }

Button* Button::setOn(bool isOn) {
    if (mImpl->isToggleable) {
        mImpl->isOn = isOn;
        setNeedsDraw();
    }
    return this;
}

Button* Button::setOnClicked(std::function<void(Button*)> onClicked)
{
    mImpl->onClicked = onClicked;
    return this;
}

Label* Button::label() const { return mImpl->label; }

Size Button::preferredSize(const LayoutContext& context) const
{
    return context.theme.calcPreferredButtonSize(context.dc, context.theme.params().labelFont,
                                                 mImpl->label->text());
}

void Button::layout(const LayoutContext& context)
{
    Super::layout(context);

    auto &r = frame();
    mImpl->label->setFrame(Rect(PicaPt::kZero, PicaPt::kZero, r.width, r.height));
}

Widget::EventResult Button::mouse(const MouseEvent &e)
{
    auto result = Super::mouse(e);

    switch (e.type) {
        case MouseEvent::Type::kButtonDown:
            // We don't do anything for button down, but it *does* change state,
            // so consume it, since we want to be the grab widget.
            result = EventResult::kConsumed;
            break;
        case MouseEvent::Type::kButtonUp: {
            result = EventResult::kConsumed;

            if (mImpl->isToggleable) {
                mImpl->isOn = !mImpl->isOn;
            } else {
                mImpl->isOn = false;
            }

            if (mImpl->onClicked) {
                mImpl->onClicked(this);
            }
            break;
        }
        default:
            break;
    }

    return result;
}

void Button::draw(UIContext& context)
{
    auto state = this->state();
    context.theme.drawButton(context, bounds(), style(state), state, isOn());
    mImpl->label->setWidgetState(state);
    auto ws = context.theme.buttonTextStyle(state, mImpl->isOn);
    mImpl->label->setTextColor(ws.fgColor);

    Super::draw(context);
}

}  // namespace uitk
