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

#ifndef UITK_ACCESSIBILITY_H
#define UITK_ACCESSIBILITY_H

#include <nativedraw.h>

#include <functional>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

namespace uitk
{

class Widget;

struct AccessibilityInfo
{
    enum class Type {
        kNone = 0,

        kContainer,
        kRadioGroup,
        kList,

        kLabel,
        kMenuItem,  /// menu item, or any item drawn by the widget but which acts as a separate child
        kButton,
        kCheckbox,
        kRadioButton,
        kIncDec,
        kSlider,
        kCombobox,
        kTextEdit,
        kPassword,  /// password or any text that should not be displayed/spoken
    };

    Type type;
    Widget *widget = nullptr;
    Rect frameWinCoord;  /// frame is in window coordinates
    std::string text;
    std::string placeholderText;

    std::variant<std::monostate, bool, int, double, std::string> value = std::monostate{};  // initializes to empty
    int indexInParent = -1;

    std::function<void()> performLeftClick;
    std::function<void()> performIncrementNumeric;
    std::function<void()> performDecrementNumeric;
    std::function<void()> performSelectAll;

    // --- Everything here is not filled out in accessibilityInfo()
    std::vector<AccessibilityInfo> children;
    bool isVisibleToUser = true;  // set false if visible() is false by widget or any parent
    // ---

public:
    using UID = std::pair<Widget*, int>;
    
    /// Returns a unique ID suitable for identifying an AccessibilityElement again
    /// when it is recreated. Note that some widgets may have pieces with separate
    /// accessibility elements but the same widget pointer because the widget draws
    /// them directly. In this case, the widget should set indexInParent.
    /// (Setting indexInParent is fine even if they subwidgets are actual widgets.)
    UID uniqueId() const;

    /// Returns a string representing this object which can be useful for debugging,
    /// since trees are annoying to examine in a debugger. Also it gives clarity into
    /// what the structure actually is, compared to how the OS decides to interpret it.
    /// It may be useful to call this function from the debugger rather than the
    /// in the program. Note that children are not populated until the top-level
    /// call finishes, so calling this in Widget::accessibilityInfo() may not produce
    /// the expected results.
    std::string debugDescription(const std::string& indent = "") const;
};

}  // namespace uitk

#endif // UITK_ACCESSIBILITY_H
