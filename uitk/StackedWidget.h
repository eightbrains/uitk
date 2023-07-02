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

#ifndef UITK_STACKED_WIDGET_H
#define UITK_STACKED_WIDGET_H

#include "Widget.h"

namespace uitk {

/// This holds a "stack" of widgets (often called "panels") on top of each
/// other, but only displays one child at a time. When a widget becomes
/// visible, it is resized to the size of the owning stacked widget.
class StackedWidget : public Widget
{
    using Super = Widget;
public:
    static constexpr int kNoIndex = -1;

    StackedWidget();
    ~StackedWidget();

    /// Adds panel to bottom of stack. Takes ownership.
    StackedWidget* addPanel(Widget *w);
    /// Removes panel, returns the pointer, and returns ownership to caller.
    Widget* removePanel(Widget *w);

    int indexShowing() const;
    /// Sets the child that is displayed. Set to kNoIndex to display no child.
    void setIndexShowing(int index);

    /// Returns the current child or nullptr if no child is displayed.
    Widget* currentPanel() const;

    enum class PreferredSize {
        kCurrentPanel,      /// preferredSize() returns the preferredSize()
                            /// of the current panel
        kMaxPanelSize };    /// preferredSize() returns the largest preferredSize()
                            /// of all the panels. This is useful so that other
                            /// items in a layout do not shift when the current
                            /// panel changes. (Default)
    PreferredSize preferredSizeCalculation() const;
    StackedWidget* setPreferredizeCalculation(PreferredSize& mode);

    Size preferredSize(const LayoutContext& context) const override;
    void layout(const LayoutContext& context) override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_STACKED_WIDGET_H
