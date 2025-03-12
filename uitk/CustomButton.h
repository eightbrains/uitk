//-----------------------------------------------------------------------------
// Copyright 2021 - 2024 Eight Brains Studios, LLC
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

#ifndef UITK_CUSTOM_BUTTON_H
#define UITK_CUSTOM_BUTTON_H

#include "Button.h"

namespace uitk {

/// This class is `Button` with two differences: the foreground color for each
/// state can be set, and the icon-only version scales the icon as the button's
/// frame gets larger.
class CustomButton : public Button
{
    using Super = Button;
public:
    explicit CustomButton(const std::string& text);
    explicit CustomButton(Theme::StandardIcon icon);
    explicit CustomButton(const Theme::Icon& icon);
    CustomButton(Theme::StandardIcon icon, const std::string& text);
    CustomButton(const Theme::Icon& icon, const std::string& text);

    Color foregroundColor(MouseState state, bool isOn = false) const;
    /// Sets the foreground color for the specific state. Color(0, 0, 0, 0)
    /// indicates the default theme color (which is set for all states
    /// by default). If only MouseState::kNormal is set, kMouseOver and
    /// kMouseDown will be automatically calculated based on the theme
    /// (including handling dark/light mode).
    CustomButton* setForegroundColor(MouseState state, const Color& color, bool isOn = false);

    void layout(const LayoutContext& context) override;
    void draw(UIContext& context) override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_CUSTOM_BUTTON_H
