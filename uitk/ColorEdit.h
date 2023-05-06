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

#ifndef UITK_COLOR_EDIT_H
#define UITK_COLOR_EDIT_H

#include "Widget.h"

#include <functional>

namespace uitk {

class ColorEdit : public Widget {
    using Super = Widget;
public:
    ColorEdit();
    ~ColorEdit();

    enum class Mode {
        kDiscrete,      /// Displays a large set of useful colors; this is useful
                        /// for applications like a word processor, or spreadsheet,
                        /// as it is easy to select (and re-select) a color like
                        /// "red" or "green-blue". This is the default mode.
        kContinuous     /// Displays a continuous set of colors; this is useful
                        /// for applications like an image editor that need a more
                        /// precise color selected.
    };
    const Mode mode() const;
    ColorEdit* setMode(Mode mode);

    const Color& color() const;
    ColorEdit* setColor(const Color& c);

    ColorEdit* setOnColorChanged(std::function<void(ColorEdit*)> onChanged);

    Size preferredSize(const LayoutContext& context) const override;
    EventResult mouse(const MouseEvent& e) override;
    bool acceptsKeyFocus() const override;
    EventResult key(const KeyEvent& e) override;
    void draw(UIContext& context) override;
    AccessibilityInfo accessibilityInfo() override;

protected:
    bool shouldAutoGrab() const override;
    void showPopup();

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_COLOR_EDIT_H
