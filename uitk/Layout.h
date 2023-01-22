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

#ifndef UITK_LAYOUT_H
#define UITK_LAYOUT_H

#include "Widget.h"

#include <array>

namespace uitk {

class Layout : public Widget
{
public:
    using Stretch = Widget;

    class SpacingEm : public Widget
    {
    public:
        explicit SpacingEm(Dir dir, float em = 1.0f);
        ~SpacingEm();
        Size preferredSize(const LayoutContext& context) const override;

        float ems() const;
        SpacingEm* setEms(float ems);

    private:
        struct Impl;
        std::unique_ptr<Impl> mImpl;
    };

private:
    using Super = Widget;

public:
    Layout();

    int alignment() const;
    /// Sets alignment. A value of 0 for the vertical or horizontal part
    /// means size to fit. For instance, (kLeft | 0), or just kLeft, would
    /// align left horizontally and size to fit vertically. The default
    /// value is (0 | 0), that is size to fit on both the major and minor
    /// axes. The use of non-zero for the major axis (the 'dir' value passed
    /// to the constructor) will cause layout() is equivalent to adding
    /// stretches.  For instance, kCenter and { stretch, widget, stretch }
    /// are equivalent.
    /// Note: see documentation in GridLayout for differences in behavior
    ///       compared to Layout1D.
    /// Design note: the use of 0 for size to fit conflicts with the principle
    ///   of named values. However, it seems reasonable because the default
    ///   is size to fit, so most usages will be unsetting the default, and
    ///   will therefore read correctly. It might be work putting a kSizeToFit
    ///   in alignment, but it does not apply to text in general, and would
    ///   not be used by the nativedraw library itself. Having a separate
    ///   alignment for layouts seems likely to trip people up. So given the
    ///   options, 0 seems like a reasonable compromise.
    Layout* setAlignment(int alignment);

    /// Returns the margins in em units, of the default label font.
    /// If the margins were set with setMarginsEm(), these values will not be
    /// accurate until after layout() has been called on the current
    /// DrawContext.
    std::array<float, 4> marginsEm() const;
    /// Sets the margins in units of ems of the default label font.
    /// The advantage to this is that the margins automatically scale with the
    /// default font size, and you can call this from the constructor of a
    /// widget because the actual PicaPt is determined later.
    Layout* setMarginsEm(float em);
    /// Sets the margins in units of ems of the default label font.
    /// The advantage to this is that the margins automatically scale with the
    /// default font size, and you can call this from the constructor of a
    /// widget because the actual PicaPt is determined later.
    Layout* setMarginsEm(float leftEm, float topEm, float rightEm, float bottomEm);

    /// Returns the margins. If the margins were set with setMarginsEm(), these
    /// values will not be accurate until after layout() has been called on
    /// the current DrawContext.
    std::array<PicaPt, 4> margins() const;
    /// Sets the margins. If using this when constructing a widget you probably
    /// want to use setMarginsEm(), since you do not know the unit sizes of
    /// things (such as the font) yet.
    Layout* setMargins(const PicaPt& m);
    /// Sets the margins. If using this when constructing a widget you probably
    /// want to use setMarginsEm(), since you do not know the unit sizes of
    /// things (such as the font) yet.
    Layout* setMargins(const PicaPt& left, const PicaPt& top, const PicaPt& right, const PicaPt& bottom);

    /// Returns the sapcing in-between elements. If the spacing were set with
    /// setSpacing(), the value will not be accurate until after layout()
    /// has been called on the current DrawContext.
    float spacingEm() const;
    /// Sets the spacing in-between elements, in em units (using the default
    /// label height). The advantage to this is that the spacing automatically
    /// with the default font size, and you can call this from the constructor
    /// of a widget because the actual PicaPt value is determined later.
    Layout* setSpacingEm(float em);

    /// Returns the sapcing in-between elements. If the spacing were set with
    /// setSpacingEm(), the value will not be accurate until after layout()
    /// has been called on the current DrawContext.
    const PicaPt& spacing() const;
    /// Sets the spacing in-between elements. If using this when constructing a
    /// widget you probably want to use setMarginsEm(), since you do not know
    /// the unit sizes of things (such as the font) yet.
    Layout* setSpacing(const PicaPt&);

    Size preferredSize(const LayoutContext &context) const = 0;

protected:
    /// Returns the actual PicaPt of the spacing, computed from spacing or
    /// spacingEm as applicable.
    PicaPt calcSpacing(const DrawContext& dc, const PicaPt& em) const;
    /// Returns the actual PicaPts of the margins, computed from margins or
    /// marginsEm as applicable.
    std::array<PicaPt, 4> calcMargins(const DrawContext& dc, const PicaPt& em) const;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

class Layout1D : public Layout
{
    using Super = Layout;
public:
    explicit Layout1D(Dir dir);
    /// Takes ownership of children
    Layout1D(Dir dir, const std::vector<Widget*>& children);
    virtual ~Layout1D();

    Dir dir() const;

    Size preferredSize(const LayoutContext &context) const override;
    void layout(const LayoutContext& context) override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

class HLayout : public Layout1D
{
    using Super = Layout1D;
public:
    HLayout();
    explicit HLayout(const std::vector<Widget*>& children);

    void addStretch();
    void addSpacingEm(float em = 1.0f);
};

class VLayout : public Layout1D
{
    using Super = Layout1D;
public:
    VLayout();
    explicit VLayout(const std::vector<Widget*>& children);

    void addStretch();
    void addSpacingEm(float em = 1.0f);
};

/// Arranges children into a grid. Note that you MUST use
/// addChild(child, row, column), otherwise the child will be ignored!
/// The alignment value determines how the child is arranged within
/// the cell. Unlike horizontal and vertical layouts, the value of 0
/// for a direction does not affect the size of the layout; instead, it
/// expand the widget to fill that cell. Whether the cells themselves
/// expand to fill the layout's size depends on the the values for
/// expandToFitWidth (default: true) and expandToFitHeight (default: false)
class GridLayout : public Layout
{
    using Super = Layout;
public:
    explicit GridLayout();
    GridLayout(const std::vector<std::vector<Widget*>>& rowsOfChildren);
    virtual ~GridLayout();

    bool expandToFitWidth() const;
    /// If true, the total width of the grid will be equal to the width
    /// of bounds().width. If false, the total width of the grid will be
    /// the sum of the preferred widths or bounds().width, whichever is
    /// smaller. Default is true.
    GridLayout* setExpandToFitWidth(bool expand);

    bool expandToFitHeight() const;
    /// If true, the total height of the grid will be equal to the height
    /// of bounds().height. If false, the total height of the grid will be
    /// the sum of the preferred heights or bounds().height, whichever is
    /// smaller. Default is false. (False is generally what you want,
    /// otherwise extra height results in extra apparent spacing between
    /// rows, which is visually unappealing.)
    GridLayout* setExpandToFitHeight(bool expand);

    /// Takes ownership of child
    void addChild(Widget *child, int row, int column);

    Size preferredSize(const LayoutContext &context) const override;
    void layout(const LayoutContext& context) override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_LAYOUT_H
