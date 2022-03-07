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

// No includes, this is included directly as a .cpp file, which avoids the
// hassle of a bunch of mostly identical header files.

namespace text {

class TextTestWidget : public Widget
{
    using Super = Widget;
public:
    static const int kNone = 0;
    static const int kShowSizeSlider = (1 << 0);

    TextTestWidget(const Text& text, int flags)
    {
        mHoriz = new SegmentedControl({"L", "C", "R"});
        mHoriz->setAction(SegmentedControl::Action::kSelectOne);
        mHoriz->setSegmentOn(0, true);
        addChild(mHoriz);
        mVert = new SegmentedControl({"T", "C", "B"});
        mVert->setAction(SegmentedControl::Action::kSelectOne);
        mVert->setSegmentOn(0, true);
        addChild(mVert);
        if (flags & kShowSizeSlider) {
            mSizeSlider = new Slider();
            mSizeSlider->setLimits(0.75, 2.0, 0.001);
            mSizeSlider->setValue(1.0);
            addChild(mSizeSlider);
        }
        mLabel = new Label(text);
        mLabel->setBorderWidth(PicaPt(1));
        mLabel->setBorderColor(Color(0.5f, 0.5f, 0.5f));
        addChild(mLabel);

        mHoriz->setOnClicked([this](int idx) {
            int alignments[] = { Alignment::kLeft, Alignment::kHCenter, Alignment::kRight };
            setHorizAlign(alignments[idx]);
        });
        mVert->setOnClicked([this](int idx) {
            int alignments[] = { Alignment::kTop, Alignment::kVCenter, Alignment::kBottom };
            setVertAlign(alignments[idx]);
        });
        if (mSizeSlider) {
            mSizeSlider->setOnValueChanged([this](SliderLogic *slider) {
                mLabel->setFont(mLabel->font().fontWithPointSize(mBaseFontSize * slider->doubleValue()));
            });
        }
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        auto em = context.theme.params().labelFont.pointSize();
        auto prefHoriz = mHoriz->preferredSize(context);
        auto prefLabel = mLabel->preferredSize(context);
        auto prefSlider = (mSizeSlider ? mSizeSlider->preferredSize(context) : Size::kZero);
        PicaPt w = std::max(prefHoriz.width + em + mVert->preferredSize(context).width,
                            prefLabel.width + 3.0f * em);
        return Size(w, prefHoriz.height + prefSlider.height + 0.5f + em + 4.0f * em);
    }

    void layout(const LayoutContext& context) override
    {
        // TODO: we need an onThemeChanged() or onConfig() callback so that we can set
        // do things like this. Change this to use that when we have it.
        mBaseFontSize = context.theme.params().labelFont.pointSize();

        auto em = context.theme.params().labelFont.pointSize();
        auto pref = mHoriz->preferredSize(context);
        mHoriz->setFrame(Rect(PicaPt::kZero, PicaPt::kZero, pref.width, pref.height));
        pref = mHoriz->preferredSize(context);
        mVert->setFrame(Rect(mHoriz->frame().maxX() + em, mHoriz->frame().y, pref.width, pref.height));
        auto y = mHoriz->frame().maxY();
        if (mSizeSlider) {
            pref = mSizeSlider->preferredSize(context);
            mSizeSlider->setFrame(Rect(mHoriz->frame().x, mHoriz->frame().maxY(),
                                       mVert->frame().maxX() - mHoriz->frame().x, pref.height));
            y = mSizeSlider->frame().maxY();
        }
        y = context.dc.ceilToNearestPixel(y + 0.5f * em);
        mLabel->setFrame(Rect(mHoriz->frame().x, y, bounds().width, bounds().height - y));

        Super::layout(context);
    }

private:
    SegmentedControl *mHoriz;
    SegmentedControl *mVert;
    Slider *mSizeSlider = nullptr;
    Label *mLabel;

    PicaPt mBaseFontSize;

    void setHorizAlign(int align)
    {
        auto clearedAlign = (mLabel->alignment() & (~Alignment::kHorizMask));
        mLabel->setAlignment(clearedAlign | align);
    }

    void setVertAlign(int align)
    {
        auto clearedAlign = (mLabel->alignment() & (~Alignment::kVertMask));
        mLabel->setAlignment(clearedAlign | align);
    }

};

class Panel : public Widget
{
    using Super = Widget;
public:
    Panel()
    {
        const Color kCyan(0.0f, 1.0f, 1.0f);  // kBlue is a little too dark in dark mode
        Text t("red green blue strike purple underline\nnormal underline bold italic", Font(), Color::kTextDefault);
        t.setBackgroundColor(Color::kRed, 0, 9);
        t.setColor(Color::kGreen, 4, 5);
        t.setColor(kCyan, 10, 4);
        t.setStrikethrough(10, 11);
        t.setStrikethroughColor(Color::kPurple, 15, 6);
        t.setColor(Color::kPurple, 22, 6);
        t.setUnderlineStyle(kUnderlineSingle, 22, 16);
        t.setUnderlineStyle(kUnderlineSingle, 46, 9);
        t.setBold(56, 4);
        t.setItalic(61, 6);
        mSimple = new TextTestWidget(t, TextTestWidget::kShowSizeSlider);
        addChild(mSimple);

        t = Text("single double dotted wavy\ndotdot wavywavy", Font(), Color::kTextDefault);
        t.setUnderlineStyle(kUnderlineSingle, 0, 6);
        t.setUnderlineStyle(kUnderlineDouble, 7, 6);
        t.setUnderlineStyle(kUnderlineDotted, 14, 6);
        t.setUnderlineStyle(kUnderlineWavy, 21, 4);
        // ensure dotted and wavy continue smoothly between runs
        t.setUnderlineStyle(kUnderlineDotted, 26, 3);
        t.setUnderlineColor(Color::kRed, 26, 3);
        t.setUnderlineStyle(kUnderlineDotted, 29, 3);
        t.setUnderlineColor(Color(0.0f, 0.8f, 0.2f), 29, 3);
        t.setUnderlineStyle(kUnderlineWavy, 33, 4);
        t.setUnderlineColor(Color::kRed, 33, 4);
        t.setUnderlineStyle(kUnderlineWavy, 37, 4);
        t.setUnderlineColor(Color(0.0f, 0.8f, 0.2f), 37, 4);
        mUnderline = new TextTestWidget(t, TextTestWidget::kShowSizeSlider);
        addChild(mUnderline);

        t = Text("little big\nsmall big small\nlarge tiny", Font(), Color::kTextDefault);
        t.setPointSize(PicaPt(12));
        t.setPointSize(PicaPt(18), 7, 3);
        t.setUnderlineStyle(kUnderlineSingle, 0, 10);
        t.setPointSize(PicaPt(18), 17, 3);
        t.setPointSize(PicaPt(18), 27, 5);
        // The font size is fixed in the Text, so the user cannot change it.
        mLittleBig = new TextTestWidget(t, TextTestWidget::kNone);
        addChild(mLittleBig);
    }

    void layout(const LayoutContext& context) override
    {
        auto em = context.theme.params().labelFont.pointSize();
        auto margin = em;

        auto pref = mSimple->preferredSize(context);
        mSimple->setFrame(Rect(em, em, pref.width, pref.height));

        pref = mUnderline->preferredSize(context);
        mUnderline->setFrame(Rect(mSimple->frame().maxX() + em, mSimple->frame().y,
                                  pref.width, pref.height));

        pref = mLittleBig->preferredSize(context);
        mLittleBig->setFrame(Rect(mSimple->frame().x, mSimple->frame().maxY() + em,
                                  pref.width, 9.0f * em));

        Super::layout(context);
    }

private:
    TextTestWidget *mSimple;
    TextTestWidget *mUnderline;
    TextTestWidget *mLittleBig;
};

}  // namespace text
