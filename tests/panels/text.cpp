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
    static const int kShowGlyphsRects = (1 << 1);

    TextTestWidget(const Text& text, int flags)
    {
        mFlags = flags;

        mHoriz = new SegmentedControl({"L", "C", "R"});
        mHoriz->setAccessibilityText("Horizontal alignment");
        mHoriz->setTooltip(0, "Align::kLeft");
        mHoriz->setTooltip(1, "Align::kCenter");
        mHoriz->setTooltip(2, "Align::kRight");
        mHoriz->setAction(SegmentedControl::Action::kSelectOne);
        mHoriz->setSegmentOn(0, true);
        addChild(mHoriz);
        mVert = new SegmentedControl({"T", "C", "B"});
        mVert->setAccessibilityText("Vertical alignment");
        mVert->setTooltip(0, "Align::kTop");
        mVert->setTooltip(1, "Align::kCenter");
        mVert->setTooltip(2, "Align::kBottom");
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
                mLabel->setFont(mLabel->font().fontWithPointSize(mBaseFontSize * float(slider->doubleValue())));
            });
        }
    }

    const Text& richText() const { return mLabel->richText(); }
    void setRichText(const Text& t) { mLabel->setRichText(t); }

    Size preferredSize(const LayoutContext& context) const override
    {
        auto em = context.theme.params().labelFont.pointSize();
        auto prefHoriz = mHoriz->preferredSize(context);
        auto prefLabel = mLabel->preferredSize(context);
        auto prefSlider = (mSizeSlider ? mSizeSlider->preferredSize(context) : Size::kZero);
        PicaPt w = std::max(prefHoriz.width + em + mVert->preferredSize(context).width,
                            prefLabel.width + 3.0f * em);
        return Size(w, prefHoriz.height + prefSlider.height + 0.5f + em + 6.0f * em);
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

    void draw(UIContext& context) override
    {
        Super::draw(context);

        // This is pretty inefficient, generating glyphs every draw, but it shouldn't be a problem,
        // and it's a lot simpler.
        if (mFlags & kShowGlyphsRects) {
            auto labelMetrics = context.theme.params().labelFont.metrics(context.dc);
            auto margin = context.dc.ceilToNearestPixel(labelMetrics.descent);
            auto layout = context.dc.createTextLayout(mLabel->richText(), mLabel->frame().size(),
                                                      mLabel->alignment());
            auto glyphs = layout->glyphs();

            context.dc.save();
            context.dc.setStrokeColor(Color(0.5f, 0.5f, 0.5f));
            context.dc.setStrokeWidth(context.dc.onePixel());
            for (auto &g : glyphs) {
                context.dc.drawRect(g.frame + Point(margin, margin) + mLabel->frame().upperLeft(),
                                    kPaintStroke);
            }
            context.dc.restore();
        }
    }

private:
    int mFlags;
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
        Text t("red green blue strike purple underline\nnormal underline bold italic TT py1", Font(), Color::kTextDefault);
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
        t.setSuperscript(69, 1);
        t.setSubscript(72, 2);
        mSimple = new TextTestWidget(t, TextTestWidget::kShowSizeSlider);
        addChild(mSimple);

        t = Text("single double dotted wavy\ndotdot wavywavy strike\nsuperTstrikeonetwodotwavy\nsubgstrikeonetwodotwavy", Font(), Color::kTextDefault);
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
        t.setStrikethrough(42, 6);
        t.setSuperscript(55, 19);
        t.setStrikethrough(55, 6);
        t.setUnderlineStyle(kUnderlineSingle, 61, 3);
        t.setUnderlineStyle(kUnderlineDouble, 64, 3);
        t.setUnderlineStyle(kUnderlineDotted, 67, 3);
        t.setUnderlineStyle(kUnderlineWavy, 70, 4);
        t.setSubscript(79, 19);
        t.setStrikethrough(79, 6);
        t.setUnderlineStyle(kUnderlineSingle, 85, 3);
        t.setUnderlineStyle(kUnderlineDouble, 88, 3);
        t.setUnderlineStyle(kUnderlineDotted, 91, 3);
        t.setUnderlineStyle(kUnderlineWavy, 94, 4);
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

        t = Text("little big\n\nO(n2) H2O2", Font(), Color::kTextDefault);
        t.setPointSize(PicaPt(18), 7, 3);
        t.setSuperscript(15, 1);
        t.setSubscript(19, 1);
        t.setSubscript(21, 1);
        mGlyphs = new TextTestWidget(t, TextTestWidget::kShowGlyphsRects);
        addChild(mGlyphs);

        t = Text("", Font(), Color::kTextDefault);
        mSpacings = new TextTestWidget(t, TextTestWidget::kShowGlyphsRects);
        addChild(mSpacings);
        mCharSpacingLabel = new Label("Char spacing");
        addChild(mCharSpacingLabel);
        mCharSpacingSlider = new Slider();
        mCharSpacingSlider->setLimits(-2.0, 5.0, 0.001);
        mCharSpacingSlider->setValue(0.0);
        addChild(mCharSpacingSlider);
        mLineHeightLabel = new Label("Line height");
        addChild(mLineHeightLabel);
        mLineHeightSlider = new Slider();
        mLineHeightSlider->setLimits(0.75, 2.0, 0.001);
        mLineHeightSlider->setValue(1.0);
        addChild(mLineHeightSlider);
        updateSpacingText();
        mCharSpacingSlider->setOnValueChanged([this](SliderLogic *) {
            updateSpacingText();
        });
        mLineHeightSlider->setOnValueChanged([this](SliderLogic *) {
            updateSpacingText();
        });

        t = Text("Lorem ipsum dolor\nsit amet consectetur\nadipiscing elit sed do\neiusmod tempor\nincididunt ut labore\net dolore magna aliqua.\nUt enim ad minim\nveniam quis nostrud\nexercitation ullamco\nlaboris nisi ut aliquip\nex ea commodo\nconsequat.", Font(), Color::kTextDefault);
        t.setStrikethrough(22, 4);
        t.setStrikethrough(159, 12);
        t.setStrikethrough(218, 9);
        mTallSpacings = new TextTestWidget(t, TextTestWidget::kNone);
        addChild(mTallSpacings);
        mTallLineHeightSlider = new Slider();
        mTallLineHeightSlider->setLimits(1.0, 2.0, 0.001);
        mTallLineHeightSlider->setValue(1.0);
        addChild(mTallLineHeightSlider);
        mTallLineHeightSlider->setOnValueChanged([this](SliderLogic *) {
            auto t = mTallSpacings->richText();  // copy
            t.setLineHeightMultiple(float(mTallLineHeightSlider->doubleValue()));
            mTallSpacings->setRichText(t);
        });
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

        pref = mGlyphs->preferredSize(context);
        mGlyphs->setFrame(Rect(mLittleBig->frame().maxX() + em, mLittleBig->frame().y,
                               pref.width, mLittleBig->frame().height));

        pref = mSpacings->preferredSize(context);
        mSpacings->setFrame(Rect(mGlyphs->frame().maxX() + em, mGlyphs->frame().y,
                                 20 * em, mGlyphs->frame().height));
        pref = mCharSpacingLabel->preferredSize(context);
        mCharSpacingLabel->setFrame(Rect(mSpacings->frame().x, mSpacings->frame().maxY(),
                                         pref.width, pref.height));
        mCharSpacingSlider->setFrame(Rect(mCharSpacingLabel->frame().maxX(), mCharSpacingLabel->frame().y,
                                          mSpacings->frame().width - pref.width, pref.height));
        mLineHeightLabel->setFrame(Rect(mCharSpacingLabel->frame().x, mCharSpacingLabel->frame().maxY(),
                                         pref.width, pref.height));
        mLineHeightSlider->setFrame(Rect(mLineHeightLabel->frame().maxX(), mLineHeightLabel->frame().y,
                                          mSpacings->frame().width - pref.width, pref.height));

        pref = mTallSpacings->preferredSize(context);
        mTallSpacings->setFrame(Rect(em, mLineHeightSlider->frame().y + em, 12.0f * em, 30.0f * em));
        mTallLineHeightSlider->setFrame(Rect(mTallSpacings->frame().x, mTallSpacings->frame().maxY(),
                                             mTallSpacings->frame().width, em));

        Super::layout(context);
    }

private:
    TextTestWidget *mSimple;
    TextTestWidget *mUnderline;
    TextTestWidget *mLittleBig;
    TextTestWidget *mGlyphs;
    TextTestWidget *mSpacings;
    TextTestWidget *mTallSpacings;
    Label *mCharSpacingLabel;
    Label *mLineHeightLabel;
    Slider *mCharSpacingSlider;
    Slider *mLineHeightSlider;
    Slider *mTallLineHeightSlider;

    void updateSpacingText()
    {
        Text t("lorem ipsum dolor\nsit amet consectetur\nadipiscing elit", Font(), Color::kTextDefault);
        t.setStrikethrough(22, 4);  // all platforms draw their own strikethrough
        t.setCharacterSpacing(PicaPt(float(mCharSpacingSlider->doubleValue())));
        t.setLineHeightMultiple(float(mLineHeightSlider->doubleValue()));
        mSpacings->setRichText(t);
    }
};

}  // namespace text
