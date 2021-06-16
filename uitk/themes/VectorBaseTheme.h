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

#ifndef UITK_VECTOR_BASE_THEME_H
#define UITK_VECTOR_BASE_THEME_H

#include "Theme.h"

namespace uitk {

class VectorBaseTheme : public Theme
{
public:
    explicit VectorBaseTheme(const Params& params, const PicaPt& borderWidth,
                             const PicaPt& borderRadius);

    const Params& params() const override;
    void setParams(const Params& params) override;

    Size calcPreferredButtonSize(const LayoutContext& ui, const Font& font,
                                 const std::string& text) const override;
    // Returns the size for the checkbox, not the checkbox + text
    Size calcPreferredCheckboxSize(const LayoutContext& ui,
                                   const Font& font) const override;

    void drawWindowBackground(UIContext& ui, const Size& size) const override;
    void drawFrame(UIContext& ui, const Rect& frame,
                   const WidgetStyle& style) const override;
    void drawButton(UIContext& ui, const Rect& frame,
                    const WidgetStyle& style, WidgetState state,
                    bool isOn) const override;
    const WidgetStyle& buttonTextStyle(WidgetState state, bool isOn) const override;
    void drawCheckbox(UIContext& ui, const Rect& frame,
                      const WidgetStyle& style, WidgetState state,
                      bool isOn) const override;

protected:
    void setVectorParams(const Params& params);

protected:
    Params mParams;
    PicaPt mBorderWidth;
    PicaPt mBorderRadius;

    WidgetStyle mButtonStyles[4];
    WidgetStyle mButtonOnStyles[4];
    WidgetStyle mCheckboxStyles[4];
    WidgetStyle mCheckboxOnStyles[4];
};

}  // namespace uitk
#endif // UITK_VECTOR_BASE_THEME_H

