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

#ifndef UITK_COMBOBOX_H
#define UITK_COMBOBOX_H

#include "Widget.h"

#include <functional>
#include <string>

namespace uitk {

class ComboBox : public Widget {
    using Super = Widget;
public:
    ComboBox();
    ~ComboBox();

    void clear();
    ComboBox* addItem(const std::string& text, int value = 0);
    ComboBox* addSeparator();

    /// Returns the text of the item at the requested index, or "" if the
    /// index is invalid.
    const std::string& textAtIndex(int index) const;
    ComboBox* setTextAtIndex(int index, const std::string& text);
    /// Returns the value of the item at the requested index, or 0 if the
    /// index is invalid.
    int valueAtIndex(int index) const;

    /// Returns the selected index or -1 if there is none.
    int selectedIndex() const;
    ComboBox* setSelectedIndex(int index);
    /// Sets the selected index to the item with requested value. (If multiple
    /// items have the value, one of them will be chosen.) If no items have the
    /// value, nothing will be changed.
    ComboBox* setSelectedValue(int value);
    /// Sets the selected index to the item with requested text. (If multiple
    /// items have the text, one of them will be chosen.) If no items have the
    /// text, nothing will be changed.
    ComboBox* setSelectedText(const std::string& text);

    ComboBox* setOnSelectionChanged(std::function<void(ComboBox*)> onChanged);

    Size preferredSize(const LayoutContext& context) const override;
    void layout(const LayoutContext& context) override;
    EventResult mouse(const MouseEvent& e) override;
    void draw(UIContext& context) override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_COMBOBOX_H


