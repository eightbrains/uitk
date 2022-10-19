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

#include "ColorEdit.h"

#include "Application.h"
#include "Events.h"
#include "PopupWindow.h"
#include "UIContext.h"
#include "Window.h"
#include "themes/Theme.h"

namespace uitk {

namespace {
static const int kNHueDivisions = 12;
static const int kNSatDivisions = 8;
static const int kNValDivisions = 8;
static const int kNGreyDivisions = 8;
}  // namespace

class ColorSelector : public Widget
{
    using Super = Widget;
public:
    enum class Mode { kThousands, k12Hues, k8Greys, kManyGreys, kClear };

    ColorSelector(Mode mode)
    : mMode(mode), mCurrentCoord({ -1, -1 })
    {}

    void setOnValueChanged(std::function<void(Color)> onValueChanged)
    {
        mOnValueChanged = onValueChanged;
    }

    void onChanged()
    {
        auto color = colorAtCoord(mCurrentCoord);
        if (color.red() >= 0.0f) {  // < 0 is invalid color, means coord is invalid
            if (mOnValueChanged) {
                mOnValueChanged(color);
            }
        }
    }

    void layout(const LayoutContext& context) override
    {
        Super::layout(context);
        switch (mMode) {
            case Mode::kThousands:
            case Mode::kManyGreys:
                mBoxHeight = std::min(context.dc.onePixel(),
                                      context.dc.floorToNearestPixel(PicaPt::fromStandardPixels(1.0)));
                mBoxWidth = mBoxHeight;
                break;
            case Mode::k12Hues:
                mBoxWidth = bounds().width / float(kNHueDivisions);
                mBoxHeight = bounds().height / float(calcNVertDivisions());
                break;
            case Mode::k8Greys:
                mBoxWidth = bounds().width / float(kNGreyDivisions + 1);
                mBoxHeight = bounds().height;
                break;
            case Mode::kClear:
                mBoxWidth = bounds().width;
                mBoxHeight = bounds().height;
                break;
        }
    }

    Widget::EventResult mouse(const MouseEvent& e) override
    {
        auto result = Super::mouse(e);
        if (result != EventResult::kIgnored) {
            return result;
        }

        if ((e.type == MouseEvent::Type::kButtonDown && e.button.button == MouseButton::kLeft)
            || (e.type == MouseEvent::Type::kDrag && e.drag.buttons == int(MouseButton::kLeft))) {
            auto newCoord = coordAtPoint(e.pos);
            if (newCoord != mCurrentCoord) {
                mCurrentCoord = newCoord;
                setNeedsDraw();
            }
        } else if (e.type == MouseEvent::Type::kButtonUp && e.button.button == MouseButton::kLeft) {
            onChanged();
        }
        return EventResult::kConsumed;
    }

    bool acceptsKeyFocus() const override{ return true; }

    Widget::EventResult key(const KeyEvent& e) override
    {
        auto result = Super::key(e);
        if (result != EventResult::kIgnored) {
            return result;
        }

        if (e.type == KeyEvent::Type::kKeyDown) {
            switch (e.key) {
                case Key::kLeft: {
                    mCurrentCoord = { std::max(0, mCurrentCoord.x - 1), std::max(0, mCurrentCoord.y) };
                    Color c = colorAtCoord(mCurrentCoord);
                    while (c.red() < 0.0f && mCurrentCoord.x > 0) {
                        mCurrentCoord.x -= 1;
                        c = colorAtCoord(mCurrentCoord);
                    }
                    setNeedsDraw();
                    break;
                }
                case Key::kRight: {
                    mCurrentCoord = { std::max(0, mCurrentCoord.x + 1), std::max(0, mCurrentCoord.y) };
                    int maxN = calcNHorizDivisions();
                    if (mMode == Mode::k8Greys || mMode == Mode::kManyGreys) {
                        maxN += 1;
                    }
                    mCurrentCoord = { std::min(mCurrentCoord.x, maxN - 1), mCurrentCoord.y };
                    Color c = colorAtCoord(mCurrentCoord);
                    while (c.red() < 0.0f && mCurrentCoord.x < maxN - 1) {
                        mCurrentCoord.x += 1;
                        c = colorAtCoord(mCurrentCoord);
                    }
                    setNeedsDraw();
                    break;
                }
                case Key::kUp: {
                    mCurrentCoord = { std::max(0, mCurrentCoord.x), std::max(0, mCurrentCoord.y - 1) };
                    setNeedsDraw();
                    break;
                }
                case Key::kDown: {
                    mCurrentCoord = { std::max(0, mCurrentCoord.x), std::max(0, mCurrentCoord.y + 1) };
                    mCurrentCoord = { mCurrentCoord.x, std::min(mCurrentCoord.y, calcNVertDivisions() - 1) };
                    setNeedsDraw();
                    break;
                }
                case Key::kSpace:
                case Key::kEnter:
                case Key::kReturn:
                    onChanged();
                    return EventResult::kConsumed;
                default:
                    break;
            }
        }
        return EventResult::kIgnored;
    }

    void draw(UIContext& ui) override
    {
        Super::draw(ui);
        auto &r = bounds();

        ui.dc.setStrokeWidth(PicaPt::kZero);
        ui.dc.drawRect(r, kPaintStroke); // HACK: so key focus gets the proper rectangle

        auto boxWidth = r.width / float(kNHueDivisions);
        if (mMode == Mode::kThousands) {
            const int nw = int(std::floor(r.width / mBoxWidth));
            const int nh = int(std::floor(r.height / mBoxHeight));
            const int halfH = nh / 2;
            for (int j = 0;  j < nh;  ++j) {
                for (int i = 0;  i < nw;  ++i) {
                    float hue = float(i) * 360.0f / float(nw);
                    float sat = (j <= halfH ? float(j) / float(halfH) : 1.0f);
                    float val = (j <= halfH ? 1.0f : 1.0f - float(j - halfH) / float(halfH));
                    auto x = r.x + ui.dc.roundToNearestPixel(float(i) * mBoxWidth);
                    auto w = r.x + ui.dc.roundToNearestPixel(float(i + 1) * mBoxWidth) - x;
                    auto y = r.y + ui.dc.roundToNearestPixel(float(j) * mBoxHeight);
                    auto h = r.y + ui.dc.roundToNearestPixel(float(j + 1) * mBoxHeight) - y;
                    ui.dc.setFillColor(HSVColor(hue, sat, val).toColor());
                    ui.dc.drawRect(Rect(x, y, w, h), kPaintFill);
                }
            }
        } else if (mMode == Mode::k12Hues) {
            const int nSatDivisions = 8; // s = 0 and s = 1 are not shown
            const int nValDivisions = 8; // v = 0 and v = 1 are not shown
            // s = 0 (white) and v = 0 (black) are not shown;  (s=1, v=1) is the central row
            const int nVertDivisions = calcNVertDivisions();
            auto boxHeight = r.height / float(nVertDivisions);
            for (int i = 0;  i < kNHueDivisions;  ++i) {
                for (int j = 0;  j < nVertDivisions;  ++j) {
                    auto x = r.x + ui.dc.roundToNearestPixel(float(i) * boxWidth);
                    auto w = r.x + ui.dc.roundToNearestPixel(float(i + 1) * boxWidth) - x;
                    auto y = r.y + ui.dc.roundToNearestPixel(float(j) * boxHeight);
                    auto h = r.y + ui.dc.roundToNearestPixel(float(j + 1) * boxHeight) - y;
                    ui.dc.setFillColor(colorAtCoord({ i, j }));
                    ui.dc.drawRect(Rect(x, y, w, h), kPaintFill);
                }
            }
        } else if (mMode == Mode::k8Greys || mMode == Mode::kManyGreys) {
            ui.dc.setFillColor(Color::kPurple);
            ui.dc.drawRect(bounds(), kPaintFill);

            int nDivs = kNGreyDivisions;
            if (mMode == Mode::kManyGreys) {
                nDivs = int(std::floor(r.width / mBoxWidth));
            }
            for (int i = 0;  i <= nDivs;  ++i) {
                auto x = r.x + ui.dc.roundToNearestPixel(float(i) * mBoxWidth);
                auto w = r.x + ui.dc.roundToNearestPixel(float(i + 1) * mBoxWidth) - x;
                ui.dc.setFillColor(colorAtCoord({ i, 0 }));
                ui.dc.drawRect(Rect(x, PicaPt::kZero, w, bounds().height), kPaintFill);
            }
        } else if (mMode == Mode::kClear) {
            const auto strokeWidth = PicaPt::fromStandardPixels(1.0f);
            auto clearBox = Rect(r.x, r.y, bounds().width, bounds().height);
            clearBox.inset(0.5f * strokeWidth, 0.5f * strokeWidth);
            ui.dc.setStrokeColor(Color(0.5f, 0.5f, 0.5f));
            ui.dc.setStrokeWidth(strokeWidth);
            ui.dc.drawRect(clearBox, kPaintStroke);
            ui.dc.setStrokeColor(ui.theme.params().textColor);  // this always contrasts well with bg
            ui.dc.drawLines({ clearBox.upperLeft(), clearBox.lowerRight() });
            ui.dc.drawLines({ clearBox.lowerLeft(), clearBox.upperRight() });
        } else {
            assert(false);  // bad mode
        }

        if (mDrawSelection && colorAtCoord(mCurrentCoord).red() >= 0.0f) {
            Rect boxRect(ui.dc.roundToNearestPixel(float(mCurrentCoord.x) * mBoxWidth),
                         ui.dc.roundToNearestPixel(float(mCurrentCoord.y) * mBoxHeight),
                         ui.dc.roundToNearestPixel(mBoxWidth),
                         ui.dc.roundToNearestPixel(mBoxHeight));
            auto strokeWidth = PicaPt::fromStandardPixels(1.0f);
            ui.dc.setStrokeColor(ui.theme.params().accentColor);
            ui.dc.setStrokeWidth(strokeWidth);
            if (mMode == Mode::kThousands || mMode == Mode::kManyGreys) {
                ui.dc.drawRect(boxRect.insetted(-0.5f * strokeWidth, -0.5f * strokeWidth), kPaintStroke);
            } else {
                ui.dc.drawRect(boxRect.insetted(0.5f * strokeWidth, 0.5f * strokeWidth), kPaintStroke);
            }
        }
    }

protected:
    int calcNHorizDivisions() const
    {
        if (mMode == Mode::k12Hues) {
            return kNHueDivisions;
        } else if (mMode == Mode::k8Greys) {
            return kNGreyDivisions;
        } else if (mMode == Mode::kThousands || mMode == Mode::kManyGreys) {
            return int(std::floor(bounds().width / mBoxHeight));
        } else {
            return 1;
        }
    }

    int calcNVertDivisions() const
    {
        if (mMode == Mode::k12Hues) {
            return (kNSatDivisions - 2) + (kNValDivisions - 2) + 1;
        } else if (mMode == Mode::kThousands) {
            return int(std::floor(bounds().height / mBoxHeight));
        } else {
            return 1;
        }
    }

    struct Coord {
        int x = 0;
        int y = 0;
        bool operator==(const Coord& rhs) { return (x == rhs.x && y == rhs.y); }
        bool operator!=(const Coord& rhs) { return (x != rhs.x || y != rhs.y); }
    };
    Coord coordAtPoint(const Point& p)
    {
        auto &r = bounds();
        return Coord{ int((p.x - r.x) / mBoxWidth), int((p.y - r.y) / mBoxHeight) };
    }

    Color colorAtCoord(const Coord& coord)
    {
        if (coord.x < 0 || coord.y < 0) { return Color(-1.0f, -1.0f, -1.0f); }
        if (mMode == Mode::k12Hues) {
            float hueDeg = float(coord.x) * 360.0f / float(kNHueDivisions);
            float sat = 1.0f;
            float val = 1.0f;
            if (coord.y <= kNSatDivisions - 2) {
                sat = float(coord.y + 1) / float(kNSatDivisions);
            } else if (coord.y == kNSatDivisions - 2) {
                ;
            } else {
                val = 1.0f - float((coord.y + 1) - (kNSatDivisions - 2) - 1) / float(kNValDivisions);
            }
            return HSVColor(hueDeg, sat, val).toColor();
        } else if (mMode == Mode::kThousands) {
            int nw = int(std::floor(frame().width / mBoxWidth));
            int nh = int(std::floor(frame().height / mBoxHeight));
            int halfH = nh / 2;
            float hueDeg = std::min(359.9f, float(coord.x) * 360.0f / float(nw));
            float sat = (coord.y <= halfH ? float(coord.y) / float(halfH) : 1.0f);
            float val = (coord.y <= halfH ? 1.0f : 1.0f - float(coord.y - halfH) / float(halfH));
            return HSVColor(hueDeg, sat, val).toColor();
        } else if (mMode == Mode::k8Greys || mMode == Mode::kManyGreys) {
            const int nDivs = calcNHorizDivisions();
            int idx = coord.x;
            if (idx >= 0 && idx <= nDivs) {
                float r = std::min(1.0f, float(idx) / float(nDivs));
                return Color(r, r, r);
            } else {
                return Color(-1.0f, -1.0f, 1.0f);
            }
        } else if (mMode == Mode::kClear){
            return Color::kTransparent;
        } else {
            assert(false);
            return Color(-1.0f, -1.0f, 1.0f);
        }
    }
private:
    Mode mMode;
    PicaPt mBoxWidth;
    PicaPt mBoxHeight;
    bool mDrawSelection = true;
    Coord mCurrentCoord;
    std::function<void(Color)> mOnValueChanged;
};

class ColorPanel : public Widget
{
    using Super = Widget;
public:
    ColorPanel(ColorEdit::Mode mode)
    {
        mColor = new ColorSelector(mode == ColorEdit::Mode::kDiscrete
                                       ? ColorSelector::Mode::k12Hues
                                       : ColorSelector::Mode::kThousands);
        addChild(mColor);

        mClear = new ColorSelector(ColorSelector::Mode::kClear);
        addChild(mClear);  // order of adding is tab order

        mGrey = new ColorSelector(mode == ColorEdit::Mode::kDiscrete
                                       ? ColorSelector::Mode::k8Greys
                                       : ColorSelector::Mode::kManyGreys);
        addChild(mGrey);

        mColor->setOnValueChanged([this](Color c) { this->onValueChange(c); });
        mGrey->setOnValueChanged([this](Color c) { this->onValueChange(c); });
        mClear->setOnValueChanged([this](Color c) { this->onValueChange(c); });
    }

    void layout(const LayoutContext& context) override
    {
        auto &r = bounds();
        auto oneSquareX = r.width / 13.0f;  // 12 divisions (6 * 2) + 2 half divisions for margin
        auto oneSquareY = r.height / 15.0f;
        auto margin = context.dc.roundToNearestPixel(0.5f * oneSquareX);

        auto h = context.dc.roundToNearestPixel(r.height - 2.0f * margin - margin - oneSquareY);
        mColor->setFrame(Rect(margin, margin, r.width - 2.0f * margin, h));
        h = context.dc.roundToNearestPixel(oneSquareY);
        mClear->setFrame(Rect(margin, mColor->frame().maxY() + margin,
                              context.dc.roundToNearestPixel(oneSquareX), h));
        mGrey->setFrame(Rect(margin + context.dc.roundToNearestPixel(2.0f * oneSquareX), mClear->frame().y,
                             context.dc.roundToNearestPixel(float(kNGreyDivisions + 1) * oneSquareX), h));

        Super::layout(context);
    }

    void onValueChange(Color c)
    {
        if (mOnDone) {
            mOnDone(c);
        }
    }

    void setOnDone(std::function<void(Color)> onDone) { mOnDone = onDone; }

private:
    ColorSelector *mColor;
    ColorSelector *mGrey;
    ColorSelector *mClear;
    std::function<void(Color)> mOnDone;
};

//-----------------------------------------------------------------------------
struct ColorEdit::Impl
{
    Mode mode;
    Color color;
    std::function<void(ColorEdit*)> onChanged;
};

ColorEdit::ColorEdit()
    : mImpl(new Impl())
{
    mImpl->mode = Mode::kDiscrete;
    mImpl->color = Color::kBlack;
}

ColorEdit::~ColorEdit()
{
}

const ColorEdit::Mode ColorEdit::mode() const { return mImpl->mode; }

ColorEdit* ColorEdit::setMode(Mode mode)
{
    mImpl->mode = mode;
    return this;
}

const Color& ColorEdit::color() const { return mImpl->color; }

ColorEdit* ColorEdit::setColor(const Color& c)
{
    mImpl->color = c;
    setNeedsDraw();
    return this;
}

ColorEdit* ColorEdit::setOnColorChanged(std::function<void(ColorEdit*)> onChanged)
{
    mImpl->onChanged = onChanged;
    return this;
}

// We don't want grabbing because we are going to open a popup window.
// But it is not right to return kIgnored for the mouse click, either.
bool ColorEdit::shouldAutoGrab() const { return false; }

void ColorEdit::showPopup()
{
    auto *parentWindow = window();
    assert(parentWindow);
    auto ll = convertToWindowFromLocal(bounds().lowerLeft());
    auto osLL = parentWindow->convertWindowToOSPoint(ll);

    auto em = frame().height;
    PopupWindow *popup = new PopupWindow(10.0f * em, 10.0f * em, "ColorPopup");

    auto *panel = new ColorPanel(mImpl->mode);
    popup->addChild(panel);  // popup now owns panel
    panel->setOnDone([this, popup](Color c) {
        this->setColor(c);
        if (mImpl->onChanged) {
            mImpl->onChanged(this);
        }
        popup->cancel();
    });

    popup->showPopup(parentWindow, osLL.x, osLL.y);
}

Size ColorEdit::preferredSize(const LayoutContext& context) const
{
    auto h = context.theme.calcStandardHeight(context.dc, context.theme.params().labelFont);
    return Size(3.0f * h, h);
}

Widget::EventResult ColorEdit::mouse(const MouseEvent& e)
{
    switch (e.type) {
        case MouseEvent::Type::kButtonDown: {
            // Don't Super::mouse() here, because we do not want to be set as the grab widget,
            // since we are opening a popup menu.
            showPopup();
            return EventResult::kConsumed;
        }
        default:
            return Super::mouse(e);
    }
}

bool ColorEdit::acceptsKeyFocus() const { return true; }

Widget::EventResult ColorEdit::key(const KeyEvent& e)
{
    auto result = Super::key(e);
    if (result != EventResult::kIgnored) {
        return result;
    }

    if (e.type == KeyEvent::Type::kKeyDown) {
        switch (e.key) {
            case Key::kSpace:
            case Key::kEnter:
            case Key::kReturn:
                showPopup();
                return EventResult::kConsumed;
            default:
                break;
        }
    }
    return EventResult::kIgnored;
}

void ColorEdit::draw(UIContext& context)
{
    Super::draw(context);
    context.theme.drawColorEdit(context, bounds(), mImpl->color, style(themeState()), themeState());
}

}  // namespace uitk
