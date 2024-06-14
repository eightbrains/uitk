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

#ifndef UITK_LABEL_H
#define UITK_LABEL_H

#include "CellWidget.h"

#include <string>

namespace uitk {

class Text;

class Label : public CellWidget {
    using Super = CellWidget;
public:
    explicit Label(const std::string& text);
    explicit Label(const Text& richText);
    ~Label();

    const std::string& text() const;
    Label* setText(const std::string& text);

    const Text& richText() const;
    Label* setRichText(const Text& richText);

    /// Returns true if word-wrapping is enabled, false otherwise.
    /// Default is false (disabled).
    bool wordWrapEnabled() const;
    /// Sets word wrapping. Default is false (disabled).
    Label* setWordWrapEnabled(bool enabled);

    int alignment() const;
    Label* setAlignment(int align);

    /// Returns the font. Note that it is frequently convenient to set the
    /// the font size or style (e.g. smaller, or bold) when the label is
    /// created. This function will report the application theme's label
    /// font if a font has not been set. In most cases this is fine, but
    /// if the window has its own theme, you should get the font from the
    /// theme.
    Font font() const;
    /// Sets the font. Note that calling this makes the font static;
    /// the default font will change if the theme changes.
    Label* setFont(const Font& font);

    const Color& textColor() const;
    /// Sets the text color. If the color is Color(0, 0, 0, 0), the color
    /// will be automatically chosen. (Use setVisible(false) if you wish to
    /// hide the label.)
    Label* setTextColor(const Color& c);

    //---- CellWidget ----
    /// Same as setTextColor(), but does not call setNeedsDraw(). If you need
    /// to set the text color within a draw (for a child element), use this.
    /// (Since this is not intended for label configuration it does not return
    /// a pointer itself.)
    void setForegroundColorNoRedraw(const Color& c) override;
    // ----

    Widget* setFrame(const Rect& frame) override;
    void themeChanged(const Theme& theme) override;
    AccessibilityInfo accessibilityInfo() override;
    Size preferredSize(const LayoutContext& context) const override;
    void layout(const LayoutContext& context) override;
    void draw(UIContext& context) override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_LABEL_H
