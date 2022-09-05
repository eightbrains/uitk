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

#include "GetBorderTheme.h"

namespace uitk {

namespace {

class RecordingDrawContext : public DrawContext
{
public:
    DrawContext& mRealDC;
    GetBorderTheme::FramePath& mFramePath;

public:
    RecordingDrawContext(DrawContext& realDC, GetBorderTheme::FramePath& framePath)
        : DrawContext(nullptr, 10000, 10000, 72.0f, 72.0f)
        , mRealDC(realDC), mFramePath(framePath)
    {
    }

    std::shared_ptr<DrawContext> createBitmap(BitmapType type, int width, int height,
                                              float dpi = 72.0f) override
        { return mRealDC.createBitmap(type, width, height, dpi); }

    std::shared_ptr<BezierPath> createBezierPath() const override
        { return mRealDC.createBezierPath(); }

    std::shared_ptr<TextLayout> createTextLayout(
                const char *utf8, const Font& font, const Color& color,
                const Size& size = Size::kZero,
                int alignment = Alignment::kLeft | Alignment::kTop,
                TextWrapping wrap = kWrapWord) const override
        { return mRealDC.createTextLayout(utf8, font, color, size, alignment, wrap); }
    std::shared_ptr<TextLayout> createTextLayout(
                const Text& t,
                const Size& size = Size::kZero,
                int alignment = Alignment::kLeft | Alignment::kTop,
                TextWrapping wrap = kWrapWord) const override
        { return mRealDC.createTextLayout(t, size, alignment, wrap); }
    std::shared_ptr<TextLayout> createTextLayout(
                const Text& t,
                const Font& defaultReplacementFont,
                const Color& defaultReplacementColor,
                const Size& size = Size::kZero,
                int alignment = Alignment::kLeft | Alignment::kTop,
                TextWrapping wrap = kWrapWord) const override
        { return mRealDC.createTextLayout(t, defaultReplacementFont, defaultReplacementColor,
                                          size, alignment, wrap); }

    void beginDraw() override {}
    void endDraw() override {}

    void save() override {}
    void restore() override {}
    // These are not used by themes at the moment, so can ignore
    void translate(const PicaPt& dx, const PicaPt& dy) override {}
    void rotate(float degrees) override {}
    void scale(float sx, float sy) override {}

    void setFillColor(const Color& color) override {}
    void setStrokeColor(const Color& color) override {}
    void setStrokeWidth(const PicaPt& w) override {}
    void setStrokeEndCap(EndCapStyle cap) override {}
    void setStrokeJoinStyle(JoinStyle join) override {}
    void setStrokeDashes(const std::vector<PicaPt> lengths, const PicaPt& offset) override {}

    Color fillColor() const override { return Color::kBlack; }
    Color strokeColor() const override { return Color::kBlack; }
    PicaPt strokeWidth() const override { return PicaPt::fromPixels(1.0f, 96.0f); }
    EndCapStyle strokeEndCap() const override { return kEndCapButt; }
    JoinStyle strokeJoinStyle() const override { return kJoinRound; }

    void fill(const Color& color) override {}
    void clearRect(const Rect& rect) override {}

    void drawLines(const std::vector<Point>& lines) override {}
    void drawRect(const Rect& rect, PaintMode mode) override
    {
        if ((rect.width > mFramePath.rect.width && rect.height >= mFramePath.rect.height) ||
            (rect.height > mFramePath.rect.height && rect.width >= mFramePath.rect.height)) {
            mFramePath.type = GetBorderTheme::Type::kRect;
            mFramePath.rect = rect;
            mFramePath.rectRadius = PicaPt::kZero;
            mFramePath.path.reset();
        }
    }

    void drawRoundedRect(const Rect& rect, const PicaPt& radius, PaintMode mode) override
    {
        if ((rect.width > mFramePath.rect.width && rect.height >= mFramePath.rect.height) ||
            (rect.height > mFramePath.rect.height && rect.width >= mFramePath.rect.height)) {
            mFramePath.type = GetBorderTheme::Type::kRect;
            mFramePath.rect = rect;
            mFramePath.rectRadius = radius;
            mFramePath.path.reset();
        }
    }

    void drawEllipse(const Rect& rect, PaintMode mode) override
    {
        if ((rect.width > mFramePath.rect.width && rect.height >= mFramePath.rect.height) ||
            (rect.height > mFramePath.rect.height && rect.width >= mFramePath.rect.height)) {
            mFramePath.type = GetBorderTheme::Type::kEllipse;
            mFramePath.rect = rect;
            mFramePath.rectRadius = PicaPt::kZero;
            mFramePath.path.reset();
        }
    }

    void drawPath(std::shared_ptr<BezierPath> path, PaintMode mode) override
    {
        if (mFramePath.rect.isEmpty()) {
            mFramePath.type = GetBorderTheme::Type::kPath;
            mFramePath.path = path;
        }
    }

    void drawText(const char *textUTF8, const Point& topLeft, const Font& font, PaintMode mode) override {}
    void drawText(const TextLayout& layout, const Point& topLeft) override {}

    void drawImage(std::shared_ptr<Image> image, const Rect& destRect) override {}

    void clipToRect(const Rect& rect) override {}
    void clipToPath(std::shared_ptr<BezierPath> path) override {}

    Color pixelAt(int x, int y) override { return Color::kBlack; }
    std::shared_ptr<Image> copyToImage() override { return nullptr; }
    Font::Metrics fontMetrics(const Font& font) const override { return mRealDC.fontMetrics(font); }
    TextMetrics textMetrics(const char *textUTF8, const Font& font,
                                PaintMode mode = kPaintFill) const override
        { return mRealDC.textMetrics(textUTF8, font, mode); }
    void calcContextPixel(const Point& point, float *x, float *y) override
    {
        if (x) { *x = point.x.toPixels(dpi()); }
        if (y) { *y = point.y.toPixels(dpi()); }
    }
};

} // namespace

std::shared_ptr<DrawContext> GetBorderTheme::drawContext(DrawContext& realDC)
{
    return std::make_shared<RecordingDrawContext>(realDC, mFrame);
}

} // namespace uitk
