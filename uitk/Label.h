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

#ifndef UITK_LABEL_H
#define UITK_LABEL_H

#include "Widget.h"

#include <string>

namespace uitk {

struct Alignment {
    static const int kLeft = (1 << 0);
    static const int kHCenter = (1 << 1);
    static const int kRight = (1 << 2);
    static const int kTop = (1 << 4);
    static const int kVCenter = (1 << 5);
    static const int kBottom = (1 << 6);
    static const int kCenter = kHCenter | kVCenter;
    static const int kHorizMask = 0b00001111;
    static const int kVertMask = 0b11110000;
};

class Label : public Widget {
    using Super = Widget;
public:
    Label(const std::string& text);
    ~Label();

    const std::string& text() const;
    Label* setText(const std::string& text);

    int alignment() const;
    Label* setAlignment(int align);

    Size preferredSize(const LayoutContext& context) const override;
    void draw(UIContext& context) override;

    // Widgets that use a label as a child can set the label state
    // as the parent's state changes so that the colors are correct.
    // This should not be called if using a Label as a UI element directly.
    void setWidgetState(Theme::WidgetState state);

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_LABEL_H
