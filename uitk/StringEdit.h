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

#ifndef UITK_STRING_EDIT_H
#define UITK_STRING_EDIT_H

#include "Widget.h"

namespace uitk {

class StringEdit : public Widget
{
    using Super = Widget;
public:
    StringEdit();
    ~StringEdit();

    const std::string& text() const;
    StringEdit* setText(const std::string& text);

    const std::string& placeholderText() const;
    StringEdit* setPlaceholderText(const std::string& text);

    Size preferredSize(const LayoutContext& context) const override;
    void layout(const LayoutContext& context) override;

    EventResult mouse(const MouseEvent& e) override;
    void key(const KeyEvent& e) override;
    void keyFocusEnded() override;

    void draw(UIContext& context) override;

    /// Called whenever the text changes in response to user input.
    /// Is not called when the text is changed directly through setText().
    void setOnTextChanged(std::function<void(const std::string&)> onChanged);

    /// Called whenever the text is commited via Enter/Return or losing
    /// focus.
    void setOnValueChanged(std::function<void(StringEdit*)> onChanged);
    
private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_STRING_EDIT_H
