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

namespace widgets {

class SizeTest : public Widget
{
    using Super = Widget;
public:
    SizeTest()
    {
        mRect = new Widget();
        mRect->setBorderColor(Color(0.5f, 0.5f, 0.5f));
        mRect->setBorderWidth(PicaPt(1));
        mRect->setFrame(Rect(PicaPt::kZero, PicaPt::kZero,
                             PicaPt(36), PicaPt(18)));
        addChild(mRect);

        mWidth = new Label("1/2 inch");
        mWidth->setAlignment(Alignment::kHCenter);
        mWidth->setFrame(Rect(mRect->frame().x, mRect->frame().maxY(),
                              mRect->frame().width, mRect->frame().height));
        addChild(mWidth);

        mHeight = new Label("1/4 inch");
        mHeight->setAlignment(Alignment::kVCenter);
        mHeight->setFrame(Rect(mRect->frame().maxX(), mRect->frame().y,
                               PicaPt(36), mRect->frame().height));
        addChild(mHeight);
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        return Size(PicaPt(72), PicaPt(36));
    }

private:
    Widget *mRect;
    Label *mWidth;
    Label *mHeight;
};

class LabelTest : public Widget
{
    using Super = Widget;
public:
    static constexpr char kShortText[] = "Agillion AVAST fill triffling";
    static constexpr char kLongText[] = "Agillion AVAST fill triffling Toast flings tiny brittle Egypt";

    LabelTest()
    {
        mDefaultLabel = new Label("Shy Gypsy 投桃报李");
        mDefaultLabel->setBorderColor(Color(0.5f, 0.5f, 0.5f));
        mDefaultLabel->setBorderWidth(PicaPt(1.0f));
        addChild(mDefaultLabel);

        mWrapped = new Label("This is some lovely text, adroitly wrapped");
        mWrapped->setWordWrapEnabled(true);
        mWrapped->setBorderColor(Color(0.5f, 0.5f, 0.5f));
        mWrapped->setBorderWidth(PicaPt(1.0f));
        addChild(mWrapped);

        mLabel = new Label(kShortText);
        mLabel->setBorderColor(Color(0.5f, 0.5f, 0.5f));
        mLabel->setBorderWidth(PicaPt(1));
        mLabel->setWordWrapEnabled(true);
        addChild(mLabel);

        mHoriz = new SegmentedControl({ "L", "C", "R" });
        mHoriz->setAccessibilityText("Horizontal alignment");
        mHoriz->setTooltip(0, "Align::kLeft");
        mHoriz->setTooltip(1, "Align::kCenter");
        mHoriz->setTooltip(2, "Align::kRight");
        mHoriz->setAction(SegmentedControl::Action::kSelectOne);
        mHoriz->setSegmentOn(0, true);
        mHoriz->setOnClicked([this](int indexClicked) {
            if (indexClicked == 0) {
                setHoriz(Alignment::kLeft);
            } else if (indexClicked == 1) {
                setHoriz(Alignment::kHCenter);
            } else if (indexClicked == 2) {
                setHoriz(Alignment::kRight);
            }
        });
        addChild(mHoriz);

        mVert = new SegmentedControl({"T", "C", "B"});
        mVert->setAccessibilityText("Vertical alignment");
        mVert->setTooltip(0, "Align::kTop");
        mVert->setTooltip(1, "Align::kCenter");
        mVert->setTooltip(2, "Align::kBottom");
        mVert->setAction(SegmentedControl::Action::kSelectOne);
        mVert->setSegmentOn(0, true);
        mVert->setOnClicked([this](int indexClicked) {
            if (indexClicked == 0) {
                setVert(Alignment::kTop);
            } else if (indexClicked == 1) {
                setVert(Alignment::kVCenter);
            } else if (indexClicked == 2) {
                setVert(Alignment::kBottom);
            }
        });
        addChild(mVert);

        mWrapLabel = new Checkbox("Wrap text");
        mWrapLabel->setOnClicked([this](Button *cb) {
            if (cb->isOn()) {
                mLabel->setText(kLongText);
            } else {
                mLabel->setText(kShortText);
            }
        });
        addChild(mWrapLabel);
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        auto em = context.theme.params().labelFont.metrics(context.dc).lineHeight;
        return Size(30.0f * em, 11.0f * em);
    }

    void layout(const LayoutContext& context) override
    {
        auto em = context.theme.params().labelFont.pointSize();
        auto wrappedWidth = 12.0f * em;

        PicaPt y(8);
        auto pref = mDefaultLabel->preferredSize(context);
        mDefaultLabel->setFrame(Rect(PicaPt(8), y, pref.width, pref.height));
        pref = mWrapped->preferredSize(context.withWidth(wrappedWidth));
        mWrapped->setFrame(Rect(mDefaultLabel->frame().maxX() + PicaPt(8), PicaPt::kZero,
                                wrappedWidth, pref.height));
        y += mWrapped->frame().height + PicaPt(8);
        pref = mHoriz->preferredSize(context);
        mHoriz->setFrame(Rect(PicaPt(8), y, pref.width, pref.height));
        pref = mVert->preferredSize(context);
        mVert->setFrame(Rect(mHoriz->frame().maxX() + PicaPt(8), y, pref.width, pref.height));
        pref = mWrapLabel->preferredSize(context);
        mWrapLabel->setFrame(Rect(mVert->frame().maxX() + PicaPt(8), y, pref.width, pref.height));

        pref = mLabel->preferredSize(context);
        mLabel->setFrame(Rect(mHoriz->frame().x, mHoriz->frame().maxY() + PicaPt(8),
                              15.0f * mHoriz->frame().height,
                              3.0f * pref.height));

        Super::layout(context);
    }

    void setHoriz(int align)
    {
        mLabel->setAlignment((align & Alignment::kHorizMask) |
                             (mLabel->alignment() & Alignment::kVertMask));
    }

    void setVert(int align)
    {
        mLabel->setAlignment((mLabel->alignment() & Alignment::kHorizMask) |
                             (align & Alignment::kVertMask));
    }

private:
    Label *mDefaultLabel;
    Label *mWrapped;
    Label *mLabel;
    SegmentedControl *mHoriz;
    SegmentedControl *mVert;
    Checkbox *mWrapLabel;
};

class ButtonTest : public Widget
{
    using Super = Widget;
public:
    ButtonTest()
    {
        mLabel = new Label(" :|");
        addChild(mLabel);

        // Use a word with descenders to test alignment
        mHappy = new Button("Happy");
        mHappy->setOnClicked([this](Button*) { mLabel->setText(" :)"); });
        addChild(mHappy);

        mAngry = new Button("Angry");  // has different descenders
        mAngry->setOnClicked([this](Button*) { mLabel->setText(" >("); });
        addChild(mAngry);

        mDisabled = new Button("Disabled");
        mDisabled->setEnabled(false);
        mDisabled->setOnClicked([this](Button*) { mLabel->setText(" :("); });
        addChild(mDisabled);

        mIconAndText = new Button(Theme::StandardIcon::kSettings, "Config");
        addChild(mIconAndText);

        mIconOnly = new Button(Theme::StandardIcon::kSettings);
        addChild(mIconOnly);

        mUndecoratedNormal = new Button(Theme::StandardIcon::kSaveFile, "Save");
        mUndecoratedNormal->setDrawStyle(Button::DrawStyle::kNoDecoration);
        addChild(mUndecoratedNormal);

        // Test state
        mOnOff = new Button("On/Off");
        mOnOff->setToggleable(true);
        addChild(mOnOff);

        mOnOffDisabled = new Button("On/Off");
        mOnOffDisabled->setToggleable(true);
        mOnOffDisabled->setOn(true);
        mOnOffDisabled->setEnabled(false);
        addChild(mOnOffDisabled);

        mUndecorated1 = new Button(Theme::StandardIcon::kStar, "Yay");
        mUndecorated1->setDrawStyle(Button::DrawStyle::kNoDecoration);
        mUndecorated1->setToggleable(true);
        mUndecorated1->setOn(false);
        addChild(mUndecorated1);

        mUndecorated2 = new Button(Theme::StandardIcon::kStar);
        mUndecorated2->setDrawStyle(Button::DrawStyle::kNoDecoration);
        mUndecorated2->setToggleable(true);
        mUndecorated2->setOn(false);
        addChild(mUndecorated2);

        mCheckbox = new Checkbox("Checkbox");
        addChild(mCheckbox);

        mRadio1 = new RadioButton("Radio 1");
        addChild(mRadio1);
        mRadio2 = new RadioButton("Radio 2");
        addChild(mRadio2);
        mRadio1->setOnClicked([this](Button*) { mRadio2->setOn(false); });
        mRadio2->setOnClicked([this](Button*) { mRadio1->setOn(false); });
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        auto button = mDisabled->preferredSize(context);
        return Size(6.0f * button.width, 5.5f * button.height);
    }

    void layout(const LayoutContext& context) override
    {
        auto x = PicaPt::kZero;
        auto y = PicaPt::kZero;

        auto pref = mHappy->preferredSize(context);
        mHappy->setFrame(Rect(x, y, pref.width, pref.height));
        pref = mAngry->preferredSize(context);
        mAngry->setFrame(Rect(mHappy->frame().maxX(), y, pref.width, pref.height));
        pref = mDisabled->preferredSize(context);
        mDisabled->setFrame(Rect(mAngry->frame().maxX(), y, pref.width, pref.height));
        pref = mLabel->preferredSize(context);
        mLabel->setFrame(Rect(mDisabled->frame().maxX(), y, 3.0f * pref.height, pref.height));

        auto em = context.theme.params().labelFont.pointSize();
        pref = mIconAndText->preferredSize(context);
        mIconAndText->setFrame(Rect(context.dc.roundToNearestPixel(mLabel->frame().maxX()),
                                    y, pref.width, pref.height));
        pref = mIconOnly->preferredSize(context);
        mIconOnly->setFrame(Rect(mIconAndText->frame().maxX(), y, pref.width, pref.height));
        pref = mUndecoratedNormal->preferredSize(context);
        mUndecoratedNormal->setFrame(Rect(mIconOnly->frame().maxX(), y, pref.width, pref.height));

        y += 1.5f * mHappy->frame().height;
        pref = mOnOff->preferredSize(context);
        mOnOff->setFrame(Rect(x, y, pref.width, pref.height));
        pref = mOnOffDisabled->preferredSize(context);
        mOnOffDisabled->setFrame(Rect(mOnOff->frame().maxX(), y, pref.width, pref.height));

        pref = mUndecorated1->preferredSize(context);
        mUndecorated1->setFrame(Rect(mIconAndText->frame().x, y, pref.width, pref.height));
        pref = mUndecorated2->preferredSize(context);
        mUndecorated2->setFrame(Rect(mIconOnly->frame().x, y, pref.width, pref.height));

        y += 1.5f * mHappy->frame().height;
        pref = mCheckbox->preferredSize(context);
        mCheckbox->setFrame(Rect(x, y, pref.width, pref.height));

        pref = mRadio1->preferredSize(context);
        mRadio1->setFrame(Rect(mUndecorated1->frame().midX(), y, pref.width, pref.height));
        pref = mRadio2->preferredSize(context);
        y += context.dc.roundToNearestPixel(std::max(mCheckbox->frame().height, mRadio1->frame().height));
        mRadio2->setFrame(Rect(mUndecorated1->frame().midX(), y, pref.width, pref.height));

        Super::layout(context);
    }

private:
    Button *mHappy;
    Button *mAngry;
    Button *mDisabled;
    Button *mIconAndText;
    Button *mIconOnly;
    Button *mUndecoratedNormal;
    Button *mOnOff;
    Button *mOnOffDisabled;
    Button *mUndecorated1;
    Button *mUndecorated2;
    Checkbox *mCheckbox;
    RadioButton *mRadio1;
    RadioButton *mRadio2;
    Label *mLabel;
};

class SegmentsTest : public Widget
{
    using Super = Widget;
public:
    SegmentsTest()
    {
        mTooSmall = new SegmentedControl({"duck", "partridge", "quail"});
        addChild(mTooSmall);

        mTooLarge = new SegmentedControl({"duck", "partridge", "quail"});
        addChild(mTooLarge);

        mSelectOne = new SegmentedControl({"Left", "Center", "Right"});
        mSelectOne->setAction(SegmentedControl::Action::kSelectOne);
        mSelectOne->setAccessibilityText("SegmentedControl (select one, text)");
        addChild(mSelectOne);

        mSelectMany = new SegmentedControl({"B", "I", "U"});
        mSelectMany->setAction(SegmentedControl::Action::kSelectMultiple);
        mSelectMany->setAccessibilityText("SegmentedControl (select multiple, text)");
        addChild(mSelectMany);

        mIconAndText = new SegmentedControl();
        mIconAndText->addItem(Theme::StandardIcon::kAlignLeft, "Left");
        mIconAndText->addItem(Theme::StandardIcon::kAlignCenter, "Center");
        mIconAndText->addItem(Theme::StandardIcon::kAlignRight, "Right");
        mIconAndText->setAction(SegmentedControl::Action::kSelectOne);
        mIconAndText->setAccessibilityText("SegmentedControl (select one, icon + text)");
        addChild(mIconAndText);

        mIconOnly = new SegmentedControl();
        mIconOnly->addItem(Theme::StandardIcon::kBoldStyle);
        mIconOnly->addItem(Theme::StandardIcon::kItalicStyle);
        mIconOnly->addItem(Theme::StandardIcon::kUnderlineStyle);
        mIconOnly->setAction(SegmentedControl::Action::kSelectMultiple);
        mIconOnly->setAccessibilityText("SegmentedControl (select multiple, icon only)");
        addChild(mIconOnly);

        mUndecoratedButtons = new SegmentedControl();
        mUndecoratedButtons->setDrawStyle(SegmentedControl::DrawStyle::kNoDecoration);
        mUndecoratedButtons->addItem(Theme::StandardIcon::kFolder);
        mUndecoratedButtons->addItem(Theme::StandardIcon::kSaveFile);
        mUndecoratedButtons->addItem(Theme::StandardIcon::kPrint);
        mUndecoratedButtons->setAccessibilityText("SegmentedControl (buttons, undecorated)");
        addChild(mUndecoratedButtons);

        mUndecoratedSelectOne = new SegmentedControl();
        mUndecoratedSelectOne->setDrawStyle(SegmentedControl::DrawStyle::kNoDecoration);
        mUndecoratedSelectOne->addItem(Theme::StandardIcon::kAlignLeft, "Left");
        mUndecoratedSelectOne->addItem(Theme::StandardIcon::kAlignCenter, "Center");
        mUndecoratedSelectOne->addItem(Theme::StandardIcon::kAlignRight, "Right");
        mUndecoratedSelectOne->setAction(SegmentedControl::Action::kSelectOne);
        mUndecoratedSelectOne->setAction(SegmentedControl::Action::kSelectOne);
        mUndecoratedSelectOne->setAccessibilityText("SegmentedControl (select one, icons + text, undecorated)");
        addChild(mUndecoratedSelectOne);

        mUndecoratedSelectMany = new SegmentedControl({"B", "I", "U"});
        mUndecoratedSelectMany->setDrawStyle(SegmentedControl::DrawStyle::kNoDecoration);
        mUndecoratedSelectMany->setAction(SegmentedControl::Action::kSelectMultiple);
        mUndecoratedSelectMany->setAccessibilityText("SegmentedControl (select multiple, text, undecorated)");
        addChild(mUndecoratedSelectMany);
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        auto pref1 = mTooSmall->preferredSize(context);
        auto pref2 = mTooLarge->preferredSize(context);
        auto em = context.dc.roundToNearestPixel(context.theme.params().labelFont.pointSize());
        return Size(pref1.width + pref1.height + 1.3f * pref2.width,
                    5.0f * pref1.height + em);
    }

    void layout(const LayoutContext& context) override
    {
        auto prefSm = mTooSmall->preferredSize(context);
        auto prefLg = mTooLarge->preferredSize(context);
        auto prefOne = mSelectOne->preferredSize(context);
        auto prefMany = mSelectMany->preferredSize(context);
        auto prefIconText = mIconAndText->preferredSize(context);
        auto prefIcon = mIconOnly->preferredSize(context);
        auto prefUnButtons = mUndecoratedButtons->preferredSize(context);
        auto prefUnOne = mUndecoratedSelectOne->preferredSize(context);
        auto prefUnMany = mUndecoratedSelectMany->preferredSize(context);

        auto y = PicaPt::kZero;
        auto spacing = context.dc.roundToNearestPixel(0.5f * prefSm.height);
        mTooSmall->setFrame(Rect(PicaPt::kZero, y,
                                 context.dc.roundToNearestPixel(0.8f * prefSm.width), prefSm.height));
        mTooLarge->setFrame(Rect(mTooSmall->frame().maxX() + spacing, y,
                                 context.dc.roundToNearestPixel(1.333f * prefLg.width), prefLg.height));
        y += context.dc.roundToNearestPixel(1.25f * prefSm.height);
        mSelectOne->setFrame(Rect(PicaPt::kZero, y, prefOne.width, prefOne.height));
        mSelectMany->setFrame(Rect(mSelectOne->frame().maxX() + spacing, y,
                                   prefMany.width, prefMany.height));

        y += context.dc.roundToNearestPixel(1.25f * prefSm.height);
        mIconAndText->setFrame(Rect(PicaPt::kZero, y, prefIconText.width, prefIconText.height));
        mIconOnly->setFrame(Rect(mIconAndText->frame().maxX() + spacing, y, prefIcon.width, prefIcon.height));

        y += context.dc.roundToNearestPixel(1.25f * prefSm.height);
        mUndecoratedButtons->setFrame(Rect(PicaPt::kZero, y, prefUnButtons.width, prefUnButtons.height));
        mUndecoratedSelectOne->setFrame(Rect(mUndecoratedButtons->frame().maxX() + spacing, y,
                                             prefUnOne.width, prefUnOne.height));
        mUndecoratedSelectMany->setFrame(Rect(mUndecoratedSelectOne->frame().maxX() + spacing, y,
                                              prefUnMany.width, prefUnMany.height));

        Super::layout(context);
    }

private:
    SegmentedControl *mTooSmall;
    SegmentedControl *mTooLarge;
    SegmentedControl *mSelectOne;
    SegmentedControl *mSelectMany;
    SegmentedControl *mIconAndText;
    SegmentedControl *mIconOnly;
    SegmentedControl *mUndecoratedButtons;
    SegmentedControl *mUndecoratedSelectOne;
    SegmentedControl *mUndecoratedSelectMany;
};

class ComboBoxTest : public Widget
{
    using Super = Widget;
public:
    ComboBoxTest()
    {
        mCombo = new ComboBox();
        mCombo->addItem("Magic");
        mCombo->addItem("More magic");
        mCombo->addItem("Deep magic");
        mCombo->addItem("Deep magic from before the dawn of time");
        addChild(mCombo);

        mColor = new ColorEdit();
        addChild(mColor);

        mContinuousColorEdit = new Checkbox("Continuous");
        mContinuousColorEdit->setOnClicked([this](Button *b) {
            mColor->setMode(b->isOn() ? ColorEdit::Mode::kContinuous : ColorEdit::Mode::kDiscrete );
        });
        addChild(mContinuousColorEdit);

        mFonts = new FontListComboBox();
        addChild(mFonts);

        mFontsUseFont = new Checkbox("Use font in menu");
        mFontsUseFont->setOnClicked([this](Button *b) {
            mFonts->setDrawWithFont(b->isOn());
        });
        addChild(mFontsUseFont);
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        auto pref = mCombo->preferredSize(context);
        return Size(pref.width, 5.0f * pref.height);
    }

    void layout(const LayoutContext& context) override
    {
        auto spacing = context.dc.roundToNearestPixel(0.5f * context.theme.params().labelFont.pointSize());
        auto y = PicaPt::kZero;

        auto pref = mCombo->preferredSize(context);
        mCombo->setFrame(Rect(PicaPt::kZero, y, bounds().width, pref.height));
        y = mCombo->frame().maxY() + spacing;

        pref = mColor->preferredSize(context);
        mColor->setFrame(Rect(PicaPt::kZero, y, pref.width, pref.height));

        pref = mContinuousColorEdit->preferredSize(context);
        mContinuousColorEdit->setFrame(Rect(mColor->frame().maxX() + spacing, y,
                                            pref.width, mColor->frame().height));
        y = mColor->frame().maxY() + spacing;

        pref = mFonts->preferredSize(context);
        mFonts->setFrame(Rect(PicaPt::kZero, y, pref.width, mColor->frame().height));

        pref = mFontsUseFont->preferredSize(context);
        mFontsUseFont->setFrame(Rect(mFonts->frame().maxX() + spacing, y,
                                     pref.width, mFonts->frame().height));

        Super::layout(context);
    }

private:
    ComboBox *mCombo;
    ColorEdit *mColor;
    Checkbox *mContinuousColorEdit;
    FontListComboBox *mFonts;
    Checkbox *mFontsUseFont;
};

class SliderTest : public Widget
{
    using Super = Widget;
public:
    SliderTest()
    {
        mIntEdit = new NumberEdit();
        mIntLabel = new Label("");
        mIntLabel->setAlignment(Alignment::kLeft | Alignment::kVCenter);

        mInt = new Slider();
        mInt->setLimits(0, 100);
        mInt->setValue(50);
        mIntEdit->setLimits(mInt->intMinLimit(), mInt->intMaxLimit(), mInt->intIncrement());
        mInt->setOnValueChanged([this](SliderLogic *s) {
            mIntEdit->setValue(s->intValue());
            mIntLabel->setText(std::to_string(s->intValue()));
        });
        addChild(mInt);     // order of adding is tab order
        addChild(mIntEdit);
        addChild(mIntLabel);

        mDoubleEdit = new NumberEdit();
        mDoubleLabel = new Label("");
        mDoubleLabel->setAlignment(Alignment::kLeft | Alignment::kVCenter);

        mDouble = new Slider();
        mDouble->setLimits(0.0, 1.0, 0.01);
        mDouble->setValue(0.25);
        mDoubleEdit->setLimits(mDouble->doubleMinLimit(), mDouble->doubleMaxLimit(),
                               mDouble->doubleIncrement());
        mDouble->setOnValueChanged([this](SliderLogic *s) {
            mDoubleEdit->setValue(s->doubleValue());
            mDoubleLabel->setText(std::to_string(s->doubleValue()));
        });
        addChild(mDouble);
        addChild(mDoubleEdit);
        addChild(mDoubleLabel);

        mIntEdit->setOnValueChanged([this](NumberEdit *n) {
            mInt->setValue(n->intValue());
            mIntLabel->setText(std::to_string(n->intValue()));
        });
        mDoubleEdit->setOnValueChanged([this](NumberEdit *n) {
            mDouble->setValue(n->doubleValue());
            mDoubleLabel->setText(std::to_string(n->doubleValue()));
        });

        mDisabled = new Slider();
        mDisabled->setLimits(0, 100);
        mDisabled->setValue(50);
        mDisabled->setEnabled(false);
        addChild(mDisabled);

        mIntEdit->setValue(mInt->intValue());
        mDoubleEdit->setValue(mDouble->doubleValue());
        mIntLabel->setText(std::to_string(mInt->intValue()));
        mDoubleLabel->setText(std::to_string(mDouble->doubleValue()));
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        auto prefHeight = mInt->preferredSize(context).height;
        auto spacing = 0.5f * prefHeight;
        return Size(PicaPt(250), 3.0f * prefHeight + 3.0f * spacing);
    }

    void layout(const LayoutContext& context) override
    {
        auto x = PicaPt::kZero;
        auto y = PicaPt::kZero;
        auto sliderHeight = mInt->preferredSize(context).height;
        auto spacing = 0.25f * sliderHeight;
        auto labelWidth = 3.0f * sliderHeight;
        auto sliderWidth = frame().width - 2.0f * (spacing + labelWidth);

        mInt->setFrame(Rect(x, y, sliderWidth, sliderHeight));
        mIntEdit->setFrame(Rect(mInt->frame().maxX() + spacing, y, labelWidth, sliderHeight));
        mIntLabel->setFrame(Rect(mIntEdit->frame().maxX() + spacing, y, labelWidth, sliderHeight));
        y += sliderHeight + spacing;
        mDouble->setFrame(Rect(x, y, sliderWidth, sliderHeight));
        mDoubleEdit->setFrame(Rect(mDouble->frame().maxX() + spacing, y, labelWidth, sliderHeight));
        mDoubleLabel->setFrame(Rect(mDoubleEdit->frame().maxX() + spacing, y, labelWidth, sliderHeight));
        y += sliderHeight + spacing;
        mDisabled->setFrame(Rect(x, y, sliderWidth, sliderHeight));

        Super::layout(context);
    }

private:
    Slider *mInt;
    Slider *mDouble;
    Slider *mDisabled;
    NumberEdit *mIntEdit;
    NumberEdit *mDoubleEdit;
    Label *mIntLabel;
    Label *mDoubleLabel;
};

class ProgressBarTest : public Widget
{
    using Super = Widget;
public:
    ProgressBarTest()
    {
        mProgress = new ProgressBar();
        mProgress->setValue(66.6f);
        addChild(mProgress);
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        auto pref = mProgress->preferredSize(context);
        return Size(PicaPt(200.0f), 2.0f * pref.height);
    }

    void layout(const LayoutContext& context) override
    {
        mProgress->setFrame(Rect(PicaPt::kZero, PicaPt::kZero,
                                 frame().width, mProgress->preferredSize(context).height));
        Super::layout(context);
    }

private:
    ProgressBar *mProgress;
};

class WaitingTest : public Widget
{
    using Super = Widget;
public:
    WaitingTest()
    {
        mStart1 = new Button("Start (1)");
        mStart1->setToggleable(true);
        mStart1->setTooltip("Start/stop waiting indicator");
        addChild(mStart1);

        mWaiting1 = new Waiting();
        addChild(mWaiting1);

        mStart2 = new Button("Start (2)");
        mStart2->setToggleable(true);
        mStart2->setTooltip("Start/stop waiting indicator");
        addChild(mStart2);

        mWaiting2 = new Waiting();
        addChild(mWaiting2);

        mStart1->setOnClicked([this](Button *b) {
            mWaiting1->setAnimating(b->isOn());
        });
        mStart2->setOnClicked([this](Button *b) {
            mWaiting2->setAnimating(b->isOn());
        });
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        auto pref = mStart1->preferredSize(context);
        return Size(PicaPt(200.0f), 2.0f * pref.height);
    }

    void layout(const LayoutContext& context) override
    {
        auto spacing = context.dc.roundToNearestPixel(0.5f * context.theme.params().labelFont.pointSize());
        auto pref = mStart1->preferredSize(context);
        auto h = pref.height;
        mStart1->setFrame(Rect(bounds().x, bounds().y, pref.width, pref.height));
        mWaiting1->setFrame(Rect(mStart1->frame().maxX() + spacing, mStart1->frame().y, h, h));
        mStart2->setFrame(Rect(mWaiting1->frame().maxX() + 4.0f * spacing, mWaiting1->frame().y,
                               pref.width, pref.height));
        mWaiting2->setFrame(Rect(mStart2->frame().maxX() + spacing, mStart2->frame().y, h, h));
        Super::layout(context);
    }

private:
    Button *mStart1;
    Waiting *mWaiting1;
    Button *mStart2;
    Waiting *mWaiting2;
};

class TextEditTest : public Widget
{
    using Super = Widget;
public:
    TextEditTest()
    {
        mString = new StringEdit();
        mString->setPlaceholderText("Edit string");
        addChild(mString);

        mSearch = new SearchBar();
        mSearch->setPlaceholderText("Search");
        addChild(mSearch);

        mPassword = new StringEdit();
        mPassword->setPlaceholderText("Password");
        mPassword->setIsPassword(true);
        addChild(mPassword);

        mShowPassword = new Button(Theme::StandardIcon::kEye);
        mShowPassword->setTooltip("Show/hide password");
        mShowPassword->setDrawStyle(Button::DrawStyle::kNoDecoration);
        mShowPassword->setToggleable(true);
        addChild(mShowPassword);
        mShowPassword->setOnClicked([this](Button *b) {
            mPassword->setIsPassword(!b->isOn());
        });

        mArea = new StringEdit();
        mArea->setMultiline(true);
        mArea->setPlaceholderText("Multiline text");
        addChild(mArea);
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        auto pref = mString->preferredSize(context);
        return Size(PicaPt(200.0), 6.0f * pref.height);
    }

    void layout(const LayoutContext& context) override
    {
        auto spacing = context.dc.roundToNearestPixel(0.25f * context.theme.params().labelFont.pointSize());
        auto pref = mString->preferredSize(context);
        auto buttonWidth = pref.height;
        auto y = PicaPt::kZero;
        auto w = bounds().width - buttonWidth - spacing;
        mString->setFrame(Rect(PicaPt::kZero, y, w, pref.height));

        y = mString->frame().maxY() + spacing;
        pref = mSearch->preferredSize(context);
        mSearch->setFrame(Rect(PicaPt::kZero, y, w, pref.height));

        y = mSearch->frame().maxY() + spacing;
        pref = mPassword->preferredSize(context);
        mPassword->setFrame(Rect(PicaPt::kZero, y, w, pref.height));
        mShowPassword->setFrame(Rect(mPassword->frame().maxX() + spacing, y, pref.height, pref.height));
        y = mPassword->frame().maxY() + spacing;

        mArea->setFrame(Rect(PicaPt::kZero, y, w, bounds().height - y));

        Super::layout(context);
    }

private:
    StringEdit *mString;
    SearchBar *mSearch;
    StringEdit *mPassword;
    Button *mShowPassword;
    StringEdit *mArea;
};

class ScrollTest : public Widget
{
    using Super = Widget;
public:
    ScrollTest()
    {
        mScroll = new ScrollView();
        addChild(mScroll);

        mIncLabel = new Label("0");
        mIncButton = new Button("Increment");
        mIncButton->setTooltip("Add one to the value");
        mIncButton->setOnClicked([this](Button *b) {
            int n = std::atoi(mIncLabel->text().c_str());
            mIncLabel->setText(std::to_string(n + 1));
        });
        mScroll->content()->addChild(mIncButton);
        mScroll->content()->addChild(mIncLabel);

        mSliderLabel = new Label("33");
        mSlider = new Slider();
        mSlider->setValue(33);
        mSlider->setOnValueChanged([this](SliderLogic *s) {
            mSliderLabel->setText(std::to_string(s->intValue()));
        });
        mScroll->content()->addChild(mSlider);
        mScroll->content()->addChild(mSliderLabel);

        mButton2 = new Button("Magic");
        mButton2->setTooltip("Toggles the magic!");
        mButton2->setToggleable(true);
        mScroll->content()->addChild(mButton2);
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        return Size(PicaPt(200), PicaPt(150));
    }

    void layout(const LayoutContext& context) override
    {
        mScroll->setFrame(Rect(PicaPt::kZero, PicaPt::kZero,
                                 frame().width, frame().height - PicaPt(36)));

        auto y = PicaPt::kZero;
        auto pref = mIncButton->preferredSize(context);
        auto spacing = 0.5f * pref.height;
        mIncButton->setFrame(Rect(PicaPt::kZero, y, pref.width, pref.height));
        pref = mIncLabel->preferredSize(context);
        mIncLabel->setFrame(Rect(mIncButton->frame().maxX() + spacing, mIncButton->frame().y,
                                 3.0f * pref.height, pref.height));
        y += mIncButton->frame().maxY() + spacing;
        pref = mSlider->preferredSize(context);
        mSlider->setFrame(Rect(PicaPt::kZero, y, PicaPt(100), pref.height));
        mSliderLabel->setFrame(Rect(mSlider->frame().maxX() + spacing, y,
                                    3.0f * pref.height, pref.height));

        pref = mButton2->preferredSize(context);
        mButton2->setFrame(Rect(PicaPt(190), PicaPt(100), pref.width, pref.height));

        mScroll->setContentSize(Size(mButton2->frame().maxX(), mButton2->frame().maxY()));

        Super::layout(context);
    }

private:
    ScrollView *mScroll;

    Button *mIncButton;
    Label *mIncLabel;
    Slider *mSlider;
    Label *mSliderLabel;
    Button *mButton2;
};

class ListViewTest : public Widget
{
    using Super = Widget;
public:
    ListViewTest()
    {
        mMode = new SegmentedControl({ "D", "0", "1", "2+" });
        mMode->setAction(SegmentedControl::Action::kSelectOne);
        mMode->setSegmentOn(2, true);
        mMode->setOnClicked([this](int idx) {
            mLV->setEnabled(idx != 0);
            switch (idx) {
                case 1:  mLV->setSelectionModel(ListView::SelectionMode::kNoItems); break;
                case 2:  mLV->setSelectionModel(ListView::SelectionMode::kSingleItem); break;
                case 3:  mLV->setSelectionModel(ListView::SelectionMode::kMultipleItems); break;
                default: break;
            }
        });
        addChild(mMode);

        mLV = new ListView();
        mLV->setSelectionModel(ListView::SelectionMode::kSingleItem);
        for (int i = 0; i < 1000; ++i) {
            mLV->addStringCell("Item " + std::to_string(i + 1));
        }
        addChild(mLV);
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        return Size(PicaPt(100), PicaPt(300));
    }

    void layout(const LayoutContext& context) override
    {
        auto pref = mMode->preferredSize(context);
        auto x = PicaPt::kZero;
        auto y = PicaPt::kZero;
        mMode->setFrame(Rect(x, y, pref.width, pref.height));
        y += pref.height + PicaPt(8);
        mLV->setFrame(Rect(x, y, bounds().width, bounds().height - y));

        Super::layout(context);
    }

private:
    SegmentedControl *mMode;
    ListView *mLV;
};

class ImageViewPanel : public Widget
{
    using Super = Widget;
public:
    ImageViewPanel()
    {
        mImage = new ImageView();
        addChild(mImage);
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        auto em = context.theme.params().labelFont.pointSize();
        auto size = context.dc.roundToNearestPixel(3.0f * em);
        return Size(size, size);
    }

    void layout(const LayoutContext& context) override
    {
        const auto dpi = context.dc.dpi();

        auto pref = preferredSize(context);
        if (mImage->image().dpi() != dpi) {
            mImage->setImage(calcFractalImage(context.dc, 0x84bd219f,
                                              int(pref.width.toPixels(dpi)), int(pref.height.toPixels(dpi)),
                                              dpi, FractalColor::kGrey));
        }
        mImage->setFrame(Rect(PicaPt::kZero, PicaPt::kZero, pref.width, pref.height));

        Super::layout(context);
    }

private:
    ImageView *mImage;
};

class DrawTimingPanel : public Widget
{
    using Super = Widget;
public:
    DrawTimingPanel()
    {
        mStart = new Button("Start Draw Timing");
        mStart->setTooltip("Get approximate timing for drawing");
        addChild(mStart);

        mDirty = new AlwaysDirty();
        addChild(mDirty);

        mProgress = new ProgressBar();
        mProgress->setVisible(false);
        addChild(mProgress);

        mLabel = new Label("");
        addChild(mLabel);

        mStart->setOnClicked([this](Button*) { onStart(); });
    }

    void onStart()
    {
        mNDraws = 0;
        mDirty->dirty = true;
        mStart->setEnabled(false);
        mLabel->setVisible(false);
        mProgress->setVisible(true);
        mNDraws = 0;
        mStartTime = Application::instance().microTime();
        Application::instance().scheduleLater(window(), mTimingLengthSecs,
                                              Application::ScheduleMode::kOnce,
                                              [this](Application::ScheduledId) {
            onEnd();
        });
    }

    void onEnd()
    {
        auto now = Application::instance().microTime();
        mDirty->dirty = false;
        mStart->setEnabled(true);
        mProgress->setVisible(false);
        auto dt = now - mStartTime;
        auto tpf = dt / float(mNDraws);
        // No good way to get printf("%.1f", n) with iostreams, but we can be
        // pretty sure that we are not going to draw faster than 0.1 msec, so
        // round to an integer value of 0.1 msec and then divide by ten.
        tpf = std::round(tpf * 1e4) / 10.0f;
        std::stringstream info;
        info << "~" << tpf << " ms/draw (" << mNDraws << " draws)";
        mLabel->setText(info.str());
        mLabel->setVisible(true);
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        auto em = context.theme.params().labelFont.pointSize();
        auto pref = mStart->preferredSize(context);
        return Size(context.dc.roundToNearestPixel(25.0f * em),
                    context.dc.roundToNearestPixel(pref.height + em));
    }

    void layout(const LayoutContext& context) override
    {
        auto em = context.theme.params().labelFont.pointSize();
        auto spacing = context.dc.roundToNearestPixel(0.5f * em);

        auto pref = mStart->preferredSize(context);
        mStart->setFrame(Rect(PicaPt::kZero, PicaPt::kZero,
                              pref.width, pref.height));
        auto x = mStart->frame().maxX() + spacing;
        mDirty->setFrame(Rect(x, mStart->frame().y,
                              bounds().width - x, mStart->frame().height));
        mLabel->setFrame(mDirty->frame());
        mProgress->setFrame(mLabel->frame());

        Super::layout(context);
    }

    void draw(UIContext &ui) override
    {
        if (mDirty->dirty) {
            auto now = Application::instance().microTime();
            auto dt = now - mStartTime;
            mProgress->setValue(std::min(100.f, float(dt / mTimingLengthSecs) * 100.0f));
            mNDraws += 1;
        }
        Super::draw(ui);
    }

private:
    class AlwaysDirty : public Widget
    {
    public:
        bool dirty = false;

        void draw(UIContext &ui) override
        {
            Widget::draw(ui);
            if (dirty) {
                Application::instance().scheduleLater(window(), [this]() { setNeedsDraw(); });
            }
        }
    };

    const float mTimingLengthSecs = 2.0f;

    Button *mStart;
    AlwaysDirty *mDirty;
    ProgressBar *mProgress;
    Label *mLabel;
    double mStartTime;
    int mNDraws;
};

class AllWidgetsPanel : public Widget
{
    using Super = Widget;
public:
    AllWidgetsPanel()
    {
        mSizing = new SizeTest();
        addChild(mSizing);
        mLabels = new LabelTest();
        addChild(mLabels);
        mButtons = new ButtonTest();
        addChild(mButtons);
        mSegments = new SegmentsTest();
        addChild(mSegments);
        mCombos = new ComboBoxTest();
        addChild(mCombos);
        mSliders = new SliderTest();
        addChild(mSliders);
        mProgress = new ProgressBarTest();
        addChild(mProgress);
        mWaiting = new WaitingTest();
        addChild(mWaiting);
        mText = new TextEditTest();
        addChild(mText);
        mScroll = new ScrollTest();
        addChild(mScroll);
        mListView = new ListViewTest();
        addChild(mListView);
        mImageView = new ImageViewPanel();
        addChild(mImageView);
        mDrawTiming = new DrawTimingPanel();
        addChild(mDrawTiming);
    }

    void layout(const LayoutContext& context)
    {
        auto spacing = context.dc.roundToNearestPixel(context.theme.params().labelFont.pointSize());
        auto x = PicaPt::kZero;
        auto pref = mSizing->preferredSize(context);
        mSizing->setFrame(Rect(x, PicaPt::kZero, pref.width, pref.height));
        pref = mLabels->preferredSize(context);
        mLabels->setFrame(Rect(x, mSizing->frame().maxY(), pref.width, pref.height));
        pref = mButtons->preferredSize(context);
        mButtons->setFrame(Rect(x, mLabels->frame().maxY(), pref.width, pref.height));
        pref = mSegments->preferredSize(context);
        mSegments->setFrame(Rect(x, mButtons->frame().maxY(), pref.width, pref.height));
        pref = mCombos->preferredSize(context);
        mCombos->setFrame(Rect(x, mSegments->frame().maxY(), pref.width, pref.height));
        pref = mSliders->preferredSize(context);
        mSliders->setFrame(Rect(x, mCombos->frame().maxY(), pref.width, pref.height));
        pref = mProgress->preferredSize(context);
        mProgress->setFrame(Rect(x, mSliders->frame().maxY(), pref.width, pref.height));
        pref = mWaiting->preferredSize(context);
        mWaiting->setFrame(Rect(x, mProgress->frame().maxY(), pref.width, pref.height));
        pref = mText->preferredSize(context);
        mText->setFrame(Rect(x, mWaiting->frame().maxY(), pref.width, pref.height));

        x += PicaPt(350);
        pref = mListView->preferredSize(context);
        mListView->setFrame(Rect(x, PicaPt::kZero, pref.width, pref.height));
        pref = mScroll->preferredSize(context);
        mScroll->setFrame(Rect(x, mListView->frame().maxY() + spacing, pref.width, pref.height));

        pref = mImageView->preferredSize(context);
        mImageView->setFrame(Rect(x, mScroll->frame().maxY() + spacing, pref.width, pref.height));

        pref = mDrawTiming->preferredSize(context);
        mDrawTiming->setFrame(Rect(x, mImageView->frame().maxY() + spacing,
                                   pref.width, pref.height));

        Super::layout(context);
    }

private:
    SizeTest *mSizing;
    LabelTest *mLabels;
    ButtonTest *mButtons;
    SegmentsTest *mSegments;
    ComboBoxTest *mCombos;
    SliderTest *mSliders;
    ProgressBarTest *mProgress;
    WaitingTest *mWaiting;
    TextEditTest *mText;
    ScrollTest *mScroll;
    ListViewTest *mListView;
    ImageViewPanel *mImageView;
    DrawTimingPanel *mDrawTiming;
};

}  // namespace widgets
