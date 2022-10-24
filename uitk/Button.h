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

#ifndef UITK_BUTTON_H
#define UITK_BUTTON_H

#include "Widget.h"

#include <functional>

namespace uitk {

class Icon;
class Label;
class IconAndText;

class Button : public Widget {
    using Super = Widget;
public:
    explicit Button(const std::string& text);
    explicit Button(Theme::StandardIcon icon);
    explicit Button(const Theme::Icon& icon);
    Button(Theme::StandardIcon icon, const std::string& text);
    Button(const Theme::Icon& icon, const std::string& text);

    ~Button();

    bool toggleable() const;
    Button* setToggleable(bool toggleable);

    bool isOn() const;
    /// Requires isToggleable to be true
    Button* setOn(bool isOn);

    Button* setOnClicked(std::function<void(Button*)> onClicked);

    Label* label() const;  // always exists
    Icon* icon() const;  // always exists

    enum class DrawStyle {
        kNormal = 0,
        kDialogDefault, /// this should be set by the dialog;
                        ///   you should not need to call this outside of a dialog
        kNoDecoration,  /// no border or background; like iOS 7 and later. Useful for icon buttons.
        kAccessory      /// style for buttons that are part of a widget, like the X button that
                        ///      clears text
    };
    DrawStyle drawStyle() const;
    /// Sets the drawing style of the button. Calling this on
    /// derived classes is likely to have no effect.
    Button* setDrawStyle(DrawStyle s);

    /// Performs a click action, as if the user clicked the button with a mouse.
    /// This will toggle on/off if the button is toggleable, and will call
    /// the on-clicked callback function.
    void performClick();

    bool acceptsKeyFocus() const override;

    Size preferredSize(const LayoutContext& context) const override;
    void layout(const LayoutContext& context) override;

    Widget::EventResult mouse(const MouseEvent &e) override;
    Widget::EventResult key(const KeyEvent& e) override;

    void draw(UIContext& context) override;

protected:
    IconAndText* cell() const;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_BUTTON_H

