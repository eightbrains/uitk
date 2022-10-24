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

#ifndef UITK_ICON_H
#define UITK_ICON_H

#include "CellWidget.h"

#include <functional>

namespace uitk {

class Icon : public CellWidget
{
    using Super = CellWidget;
public:
    explicit Icon(Theme::StandardIcon icon);
    explicit Icon(Theme::Icon drawFunc);

    /// Returns true if the icon is Theme::StandardIcon::kNone and there is no
    /// custom Theme::Icon.
    bool isEmpty() const;

    Icon* setIcon(Theme::Icon icon);
    Icon* setIcon(Theme::StandardIcon icon);

    const Color& color() const;
    Icon* setColor(const Color& fg);

    // ---- CellWidget ----
    /// Sets the color, but does not request a redraw. This is useful when
    /// using the icon as a child of another object, so that the icon can
    /// draw using the parent's style.
    void setForegroundColorNoRedraw(const Color& fg) override;
    // ----

    Size preferredSize(const LayoutContext& context) const override;
    void draw(UIContext& context) override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace uitk
#endif // UITK_ICON_H
