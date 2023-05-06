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

#include "Button.h"

#include "Icon.h"
#include "Label.h"
#include "IconAndText.h"
#include "Events.h"
#include "UIContext.h"
#include "themes/Theme.h"

namespace uitk {

struct Button::Impl {
    IconAndText *cell = nullptr;  // we do not own this
    DrawStyle drawStyle = DrawStyle::kNormal;
    std::function<void(Button*)> onClicked = nullptr;
    bool isOn = false;
    bool isToggleable = false;
};

Button::Button(const std::string& text)
    : mImpl(new Impl())
{
    mImpl->cell = new IconAndText();
    mImpl->cell->label()->setText(text);
    addChild(mImpl->cell);  // takes ownership
}

Button::Button(Theme::StandardIcon stdIcon)
    : mImpl(new Impl())
{
    mImpl->cell = new IconAndText();
    mImpl->cell->icon()->setIcon(stdIcon);
    addChild(mImpl->cell);  // takes ownership
}

Button::Button(const Theme::Icon& icon)
    : mImpl(new Impl())
{
    mImpl->cell = new IconAndText();
    mImpl->cell->icon()->setIcon(icon);
    addChild(mImpl->cell);  // takes ownership
}


Button::Button(Theme::StandardIcon stdIcon, const std::string& text)
    : mImpl(new Impl())
{
    mImpl->cell = new IconAndText();
    mImpl->cell->icon()->setIcon(stdIcon);
    mImpl->cell->label()->setText(text);
    addChild(mImpl->cell);  // takes ownership
}

Button::Button(const Theme::Icon& icon, const std::string& text)
    : mImpl(new Impl())
{
    mImpl->cell = new IconAndText();
    mImpl->cell->icon()->setIcon(icon);
    mImpl->cell->label()->setText(text);
    addChild(mImpl->cell);  // takes ownership
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

Label* Button::label() const { return mImpl->cell->label(); }

Icon* Button::icon() const { return mImpl->cell->icon(); }

IconAndText* Button::cell() const { return mImpl->cell; }

Button::DrawStyle Button::drawStyle() const { return mImpl->drawStyle; }

// Design note:
// This is clunky, since it cannot really apply to derived classes.
// Cocoa's solution is for a checkbox to be a draw style of NSButton,
// but that is also a little clunky, and checkbox->isChecked() reads
// better and is more memorable than checkbox->isOn(). Inheritance-happy
// designs have Button and Checkbox inherit from a BaseButton class,
// which seems a bit overkill, but might be cleaner.
Button* Button::setDrawStyle(DrawStyle s)
{
    mImpl->drawStyle = s;
    setNeedsDraw();
    return this;
}

void Button::performClick()
{
    if (!enabled()) {
        return;
    }
    
    if (mImpl->isToggleable) {
        mImpl->isOn = !mImpl->isOn;
    } else {
        mImpl->isOn = false;
    }

    if (mImpl->onClicked) {
        mImpl->onClicked(this);
    }
    setNeedsDraw();
}

bool Button::acceptsKeyFocus() const { return true; }

AccessibilityInfo Button::accessibilityInfo()
{
    auto info = Super::accessibilityInfo();
    info.type = AccessibilityInfo::Type::kButton;
    if (auto *c = cell()) {
        info.text = c->label()->text();
    } else if (auto *lbl = label()) {
        info.text = lbl->text();
    }
    if (info.text.empty()) {  // lbl might be null (in theory) or more probably, is icon-only
        info.text = tooltip();
    }
    if (info.text.empty()) {
        info.text = "icon";
    }
    info.performLeftClick = [this]() { performClick(); };
    return info;
}

Size Button::preferredSize(const LayoutContext& context) const
{
    auto font = label()->font();
    auto margins = context.theme.calcPreferredButtonMargins(context.dc, font);
    auto pref = mImpl->cell->preferredSize(context);
    return Size(pref.width + 2.0f * margins.width,
                pref.height + 2.0f * margins.height);  // normally margins.height is zero
}

void Button::layout(const LayoutContext& context)
{
    auto &r = bounds();
    auto pref = mImpl->cell->preferredSize(context);
    auto x = std::max(PicaPt::kZero, context.dc.roundToNearestPixel(0.5f * (r.width - pref.width)));
    auto w = std::min(r.width, pref.width);
    auto y = std::max(PicaPt::kZero, context.dc.roundToNearestPixel(0.5f * (r.height - pref.height)));
    auto h = std::min(r.height, pref.height);
    mImpl->cell->setFrame(Rect(x, y, w, h));

    Super::layout(context);
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

            performClick();
            break;
        }
        default:
            break;
    }

    return result;
}

Widget::EventResult Button::key(const KeyEvent& e)
{
    auto result = Super::key(e);
    if (result != Widget::EventResult::kIgnored) {
        return result;
    }
    switch (e.key) {
        case Key::kSpace:
        case Key::kEnter:
        case Key::kReturn:
            if (e.type == KeyEvent::Type::kKeyDown && !e.isRepeat) {
                setState(MouseState::kMouseDown);
            } else if (e.type == KeyEvent::Type::kKeyUp) {  // need to check if up, could be repeated down
                if (state() == MouseState::kMouseDown) {
                    performClick();
                }
                setState(MouseState::kNormal);
            }
            return Widget::EventResult::kConsumed;
        case Key::kEscape:
            setState(MouseState::kNormal);
            return Widget::EventResult::kConsumed;
        default:
            return Widget::EventResult::kIgnored;
    }
}

void Button::draw(UIContext& context)
{
    auto themeState = this->themeState();
    Theme::ButtonDrawStyle bdStyle;
    switch (mImpl->drawStyle) {
        case DrawStyle::kNormal:
            bdStyle = Theme::ButtonDrawStyle::kNormal;
            break;
        case DrawStyle::kDialogDefault:
            bdStyle = Theme::ButtonDrawStyle::kDialogDefault;
            break;
        case DrawStyle::kNoDecoration:
            bdStyle = Theme::ButtonDrawStyle::kNoDecoration;
            break;
        case DrawStyle::kAccessory:
            bdStyle = Theme::ButtonDrawStyle::kAccessory;
            break;
    }
    context.theme.drawButton(context, bounds(), bdStyle, style(themeState), themeState, isOn());
    mImpl->cell->setThemeState(themeState);
    auto ws = context.theme.buttonTextStyle(themeState, bdStyle, mImpl->isOn);
    mImpl->cell->setForegroundColorNoRedraw(ws.fgColor);

    Super::draw(context);
}

}  // namespace uitk
