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

#ifndef UITK_SEARCH_BAR_H
#define UITK_SEARCH_BAR_H

#include "Widget.h"

#include <functional>

namespace uitk {

class Icon;

class SearchBar : public Widget
{
    using Super = Widget;
public:
    SearchBar();
    ~SearchBar();

    Icon* icon() const;

    const std::string& text() const;
    SearchBar* setText(const std::string& text);

    const std::string& placeholderText() const;
    SearchBar* setPlaceholderText(const std::string& text);

    int alignment() const;
    /// Sets the text alignment; vertical alignment may be ignored.
    SearchBar* setAlignment(int alignment);

    /// Called whenever the text changes in response to user input.
    /// Is not called when the text is changed directly through setText().
    void setOnTextChanged(std::function<void(const std::string&)> onChanged);

    /// Called whenever the text is commited via Enter/Return or losing
    /// focus.
    void setOnValueChanged(std::function<void(SearchBar*)> onChanged);

    Size preferredSize(const LayoutContext& context) const override;
    void layout(const LayoutContext& context) override;
    void draw(UIContext& context) override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_SEARCH_BAR_H
