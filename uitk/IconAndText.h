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

#ifndef UITK_ICON_AND_TEXT_H
#define UITK_ICON_AND_TEXT_H

#include "CellWidget.h"

#include <string>

namespace uitk {

class Icon;
class Label;

/// This is a convenience class that handles (optional) icon + (optional) text.
/// It's raison d'etre is to allow widgets like Button and SegmentedControl
/// to reuse code, so it is mostly expected to be an internal class, but is
/// exposed in case anyone else needs it. (Note that the convention is for
/// the owner of LabelCell to export the label() and icon() calls itself,
/// since the existence of this class is an implementation detail that has
/// no value for the user to know about.)
class IconAndText : public CellWidget {
    using Super = CellWidget;
public:
    IconAndText();
    ~IconAndText();

    // Q: since the pointer always exists, why not make this a reference?
    // A: because C++ makes us play @#$! compiler with . and ->, and
    //    everywhere else we return a pointer, so nobody is going to
    //    remember to put a . instead of a -> here.
    Label* label() const;  /// pointer always exists
    Icon* icon() const;  /// pointer always exists

    bool iconIsFullFrame() const;
    /// If true, the icon is sized to bounds() if the cell is icon-only.
    /// If false, the icon is sized to the theme icon size.
    /// Default is false.
    IconAndText* setIconIsFullFrame(bool isFull);

    // ---- CellWidget ----
    /// Sets the color, but does not request a redraw. This is useful when
    /// using the icon as a child of another object, so that the icon can
    /// draw using the parent's style.
    void setForegroundColorNoRedraw(const Color& c) override;
    // ----

    Size preferredSize(const LayoutContext& context) const override;
    void layout(const LayoutContext& context) override;
    void draw(UIContext& ui) override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_ICON_AND_TEXT_H

