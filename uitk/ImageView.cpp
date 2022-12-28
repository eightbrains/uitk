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

#include "ImageView.h"

#include "UIContext.h"

namespace uitk {

struct ImageView::Impl
{
    Mode mode = Mode::kAspect;
    std::shared_ptr<Image> image;
};

ImageView::ImageView()
    : mImpl(new Impl())
{
}

ImageView::ImageView(std::shared_ptr<Image> image)
    : mImpl(new Impl())
{
    mImpl->image = image;
}

ImageView::~ImageView()
{
}

std::shared_ptr<Image> ImageView::image() const { return mImpl->image; }

ImageView* ImageView::setImage(std::shared_ptr<Image> image)
{
    mImpl->image = image;
    setNeedsDraw();
    return this;
}

ImageView::Mode ImageView::mode() const { return mImpl->mode; }

ImageView* ImageView::setMode(Mode mode)
{
    mImpl->mode = mode;
    setNeedsDraw();
    return this;
}

Size ImageView::preferredSize(const LayoutContext &context) const
{
    if (mImpl->image) {
        return Size(context.dc.ceilToNearestPixel(mImpl->image->width()),
                    context.dc.ceilToNearestPixel(mImpl->image->height()));
    } else {
        auto size = context.dc.roundToNearestPixel(0.75f * context.theme.params().labelFont.pointSize());
        return Size(size, size);
    }
}

void ImageView::draw(UIContext &context)
{
    Super::draw(context);

    if (mImpl->image) {
        Rect r(PicaPt::kZero, PicaPt::kZero, frame().width, frame().height);
        if (borderWidth() > PicaPt::kZero && borderColor().alpha() > 0.0f) {
            r.inset(borderWidth(), borderWidth());
        }
        auto imgWidth = mImpl->image->width();
        auto imgHeight = mImpl->image->height();
        if (imgWidth < r.width && imgHeight < r.height) {
            context.dc.drawImage(mImpl->image,
                                 Rect(r.midX() - 0.5f * imgWidth, r.midY() - 0.5f * imgHeight,
                                      imgWidth, imgHeight));
        } else {
            Rect drawRect;
            switch (mImpl->mode) {
                case Mode::kFixed:
                    drawRect = Rect(r.x + bounds().x, r.y + bounds().y, imgWidth, imgHeight);
                    context.dc.save();
                    context.dc.clipToRect(r);
                    context.dc.drawImage(mImpl->image, drawRect);
                    context.dc.restore();
                    break;
                case Mode::kStretch:
                    drawRect = r;
                    context.dc.drawImage(mImpl->image, drawRect);
                    break;
                case Mode::kAspect: {
                    auto widgetAspect = r.width / r.height;
                    auto imgAspect = imgWidth / imgHeight;
                    if (imgAspect > widgetAspect) {
                        auto h = r.width / imgAspect;
                        drawRect = Rect(r.x, r.midY() - 0.5f * h, r.width, h);
                    } else {
                        auto w = r.height * imgAspect;
                        drawRect = Rect(r.midX() - 0.5f * w, r.y, w, r.height);
                    }
                    context.dc.drawImage(mImpl->image, drawRect);
                    break;
                }
            }
        }
    }
}

} // namespace uitk
