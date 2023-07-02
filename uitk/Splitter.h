//-----------------------------------------------------------------------------
// Copyright 2021 - 2023 Eight Brains Studios, LLC
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

#ifndef UITK_SPLITTER_H
#define UITK_SPLITTER_H

#include "Widget.h"

namespace uitk {

class Length;

class Splitter : public Widget
{
    using Super = Widget;
public:
    Splitter(Dir dir);

    /// Adds the widget to right (or below, depending). Takes ownership.
    Splitter* addPanel(Widget *panel);
    /// Removes the widget and returns it and ownership to the caller.
    /// Returns nullptr if panel is not in splitter.
    Widget* removePanel(Widget *panel);

    Dir direction() const;

    Splitter* setPanelLengths(const std::vector<Length>& lengths);
    /// Sets the lengths of the panels. Lengths that are missing or negative
    /// will be set to remainingSpace / nEmpty. Setting a negative length
    /// is useful if you want to specify the length of the first and last
    /// panels (for example), without needing to know the size of the
    /// middle panel(s).
    Splitter* setPanelLengths(const std::vector<PicaPt>& lengths);
    /// Sets the lengths of the panels. Lengths that are missing or negative
    /// will be set to remainingSpace / nEmpty. Setting a negative length
    /// is useful if you want to specify the length of the first and last
    /// panels (for example), without needing to know the size of the
    /// middle panel(s).
    Splitter* setPanelLengthsEm(const std::vector<float>& lengths);
    Splitter* setPanelLengthsPercent(const std::vector<float>& lengths);

    /// Returns the actual lengths of each panel. This is undefined before
    /// layout() is called.
    std::vector<PicaPt> panelLengths() const;

    AccessibilityInfo accessibilityInfo() override;

    Size preferredSize(const LayoutContext& context) const override;
    void layout(const LayoutContext& context) override;

    void draw(UIContext& context) override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace uitk
#endif // UITK_SPLITTER_H
