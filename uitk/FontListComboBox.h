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

#ifndef UITK_FONT_LIST_COMBO_BOX_H
#define UITK_FONT_LIST_COMBO_BOX_H

#include "ComboBox.h"

namespace uitk {

class FontListComboBox : public ComboBox
{
    using Super = ComboBox;
public:
    FontListComboBox();
    FontListComboBox(const std::vector<std::string>& fontNames);

    bool drawWithFont() const;
    /// If true the menu items are drawn in the font they represent, otherwise
    /// they will be drawn in the normal ComboBox font. The ComboBox itself
    /// is always drawn in the ComboBox font, since variations in font size
    /// and contents (e.g. symbol fonts) may make the font difficult to read.
    /// The default value is false, draw normally.
    FontListComboBox* setDrawWithFont(bool with);

    /// Adds a font to the list. Note that this is different than using the
    /// base class addItem() functions, which will not work when drawing with
    /// the font is true.
    void addFont(const std::string& fontName);

    void willChangeSelection() override;
    void didChangeSelection() override;
    void willShowMenu() override;
    void didHideMenu() override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_FONT_LIST_COMBO_BOX_H

