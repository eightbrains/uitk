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

#ifndef UITK_SEGMENTED_CONTROL_H
#define UITK_SEGMENTED_CONTROL_H

#include "Widget.h"

#include <functional>
#include <unordered_set>

namespace uitk {

class SegmentedControl : public Widget
{
    using Super = Widget;
public:
    SegmentedControl();
    explicit SegmentedControl(const std::vector<std::string>& items);
    ~SegmentedControl();

    void clearItems();
    SegmentedControl* addItem(const std::string& name);
    /// Tooltip is intentionally required: the meaning of icons may not be obvious to
    /// all users, especially those with visual difficulties.
    SegmentedControl* addItem(Theme::StandardIcon icon);
    SegmentedControl* addItem(const Theme::Icon& icon);
    SegmentedControl* addItem(Theme::StandardIcon icon, const std::string& name);
    SegmentedControl* addItem(const Theme::Icon& icon, const std::string& name);

    /// Sets the tooltip for an indivdual index. Super::setTooltip() will set the
    /// tooltip for the entire widget, which is probably not what you want.
    SegmentedControl* setTooltip(int index, const std::string& tooltip);

    enum class Action {
        kButton,            /// segment acts as a momentary push-button (like Button)
        kSelectOne,         /// only one segment can be toggled on (like a radio button)
        kSelectMultiple };  /// multiple segments can be toggled on

    Action action() const;
    SegmentedControl* setAction(Action act);

    bool isSegmentOn(int index) const;
    SegmentedControl* setSegmentOn(int index, bool on);

    enum class DrawStyle {
        kNormal = 0,
        kNoDecoration   /// no border or background; like iOS 7 and later
    };
    DrawStyle drawStyle() const;
    /// Sets the drawing style of the segmented control. Calling this on
    /// derived classes is likely to have no effect.
    SegmentedControl* setDrawStyle(DrawStyle s);

    /// Sets a function that will be called when a segment is clicked.
    /// The single argument is the segment currently clicked; if the action is
    /// kSelectMultiple, use isSegmentOn() to determine the states of other
    /// segments.
    SegmentedControl* setOnClicked(std::function<void(int)> onClicked);

    /// Acts as if the user clicked on the index: the onClicked callback
    /// will be called, unlike setSegmentOn(), and any toggle action will
    /// be performed.
    void performClick(int index);

    AccessibilityInfo accessibilityInfo() override;
    Size preferredSize(const LayoutContext& context) const override;
    void layout(const LayoutContext& context) override;
    EventResult mouse(const MouseEvent& e) override;
    void mouseExited() override;
    bool acceptsKeyFocus() const override;
    EventResult key(const KeyEvent& e) override;
    void keyFocusEnded() override;
    void draw(UIContext& context) override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk

#endif // UITK_SEGMENTED_CONTROL_H
