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

#ifndef UITK_IMAGE_VIEW_H
#define UITK_IMAGE_VIEW_H

#include "Widget.h"

namespace uitk {

/// This class displays an image. It is not intended to be used for icons, which
/// are better as a resolution-independent Icon. However, for cases where bitmap
/// icons are necessary, this is serviceable, but it may be difficult getting
/// pixel-perfect icons an the wide variety of resolutions available.
class ImageView : public Widget
{
    using Super = Widget;
public:
    ImageView();
    explicit ImageView(std::shared_ptr<Image> image);
    virtual ~ImageView();

    std::shared_ptr<Image> image() const;
    ImageView* setImage(std::shared_ptr<Image> image);

    enum class Mode {
        kFixed = 0,  /// image is displayed its native size
        kAspect,     /// image is displayed as large as possible maintaining
                     ///   its aspect ratio (default)
        kStretch };  /// image is stretched to fit the size of the ImageView

    Mode mode() const;
    ImageView* setMode(Mode mode);

    Size preferredSize(const LayoutContext &context) const override;
    void draw(UIContext &context) override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_IMAGE_VIEW_H
