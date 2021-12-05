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

#include <uitk/uitk.h>

using namespace uitk;

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
    LabelTest()
    {
        mDefaultLabel = new Label("Shy Gypsy 投桃报李");
        mDefaultLabel->setBorderColor(Color(0.5f, 0.5f, 0.5f));
        mDefaultLabel->setBorderWidth(PicaPt(1.0f));
        addChild(mDefaultLabel);

        mLabel = new Label("Agillion AVAST fill triffling");
        mLabel->setBorderColor(Color(0.5f, 0.5f, 0.5f));
        mLabel->setBorderWidth(PicaPt(1));
        addChild(mLabel);

        mHoriz = new SegmentedControl({ "L", "C", "R" });
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
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        auto em = context.theme.params().labelFont.metrics(context.dc).lineHeight;
        return Size(20.0f * em, 7.0f * em);
    }

    void layout(const LayoutContext& context) override
    {
        auto setToPref = [&context](Widget *w) {
            auto pref = w->preferredSize(context);
            w->setFrame(Rect(PicaPt(0), PicaPt(0), pref.width, pref.height));
        };

        for (auto *child : children()) {
            setToPref(child);
        }

        PicaPt y(8);
        mDefaultLabel->setPosition(Point(PicaPt(8), y));
        y += mDefaultLabel->frame().height + PicaPt(8);
        mHoriz->setPosition(Point(PicaPt(8), y));
        mVert->setPosition(Point(mHoriz->frame().maxX() + PicaPt(8), y));

        mLabel->setFrame(Rect(mHoriz->frame().x, mHoriz->frame().maxY() + PicaPt(8),
                              15.0f * mHoriz->frame().height,
                              3.0f * mLabel->frame().height));

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
    Label *mLabel;
    SegmentedControl *mHoriz;
    SegmentedControl *mVert;
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

        mOnOff = new Button("On/Off");
        mOnOff->setToggleable(true);
        addChild(mOnOff);

        mOnOffDisabled = new Button("On/Off");
        mOnOffDisabled->setToggleable(true);
        mOnOffDisabled->setOn(true);
        mOnOffDisabled->setEnabled(false);
        addChild(mOnOffDisabled);

        mCheckbox = new Checkbox("Checkbox");
        addChild(mCheckbox);
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        auto button = context.theme.calcPreferredButtonSize(context.dc,
                                                            context.theme.params().labelFont,
                                                            mDisabled->label()->text());
        return Size(5.0f * button.width, 5.5f * button.height);
    }

    void layout(const LayoutContext& context) override
    {
        auto setToPref = [&context](Widget *w) {
            auto pref = w->preferredSize(context);
            w->setFrame(Rect(PicaPt(0), PicaPt(0), pref.width, pref.height));
        };

        for (auto *child : children()) {
            setToPref(child);
        }

        auto x = PicaPt::kZero;
        auto y = PicaPt::kZero;
        mHappy->setFrame(Rect(x, y, mHappy->frame().width, mHappy->frame().height));
        mAngry->setFrame(Rect(mHappy->frame().maxX(), y, mAngry->frame().width, mAngry->frame().height));
        mDisabled->setFrame(Rect(mAngry->frame().maxX(), y, mDisabled->frame().width, mDisabled->frame().height));
        mLabel->setFrame(Rect(mDisabled->frame().maxX(), y, 3.0f * mLabel->frame().height, mLabel->frame().height));

        y += 1.5f * mHappy->frame().height;
        mOnOff->setFrame(Rect(x, y, mOnOff->frame().width, mOnOff->frame().height));
        mOnOffDisabled->setFrame(Rect(mOnOff->frame().maxX(), y,
                                      mOnOffDisabled->frame().width, mOnOffDisabled->frame().height));

        y += 1.5f * mHappy->frame().height;
        mCheckbox->setFrame(Rect(x, y, mCheckbox->frame().width, mCheckbox->frame().height));

        Super::layout(context);
    }

private:
    Button *mHappy;
    Button *mAngry;
    Button *mDisabled;
    Button *mOnOff;
    Button *mOnOffDisabled;
    Checkbox *mCheckbox;
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
        addChild(mSelectOne);

        mSelectMany = new SegmentedControl({"B", "I", "U"});
        mSelectMany->setAction(SegmentedControl::Action::kSelectMultiple);
        addChild(mSelectMany);
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        auto pref1 = mTooSmall->preferredSize(context);
        auto pref2 = mTooLarge->preferredSize(context);
        return Size(pref1.width + pref1.height + 1.3f * pref2.width, 3.0f * pref1.height);
    }

    void layout(const LayoutContext& context) override
    {
        auto prefSm = mTooSmall->preferredSize(context);
        auto prefLg = mTooLarge->preferredSize(context);
        auto prefOne = mSelectOne->preferredSize(context);
        auto prefMany = mSelectMany->preferredSize(context);

        auto y = PicaPt::kZero;
        mTooSmall->setFrame(Rect(PicaPt::kZero, y, 0.8f * prefSm.width, prefSm.height));
        mTooLarge->setFrame(Rect(mTooSmall->frame().maxX() + 0.5f * prefSm.height, y,
                                 1.333f * prefLg.width, prefLg.height));
        y += 1.25f * prefSm.height;
        mSelectOne->setFrame(Rect(PicaPt::kZero, y, prefOne.width, prefOne.height));
        mSelectMany->setFrame(Rect(mSelectOne->frame().maxX() + 0.5f * prefSm.height, y,
                                   prefMany.width, prefMany.height));

        Super::layout(context);
    }

private:
    SegmentedControl *mTooSmall;
    SegmentedControl *mTooLarge;
    SegmentedControl *mSelectOne;
    SegmentedControl *mSelectMany;
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
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        auto pref = mCombo->preferredSize(context);
        return Size(pref.width, 1.5f * pref.height);
    }

    void layout(const LayoutContext& context) override
    {
        auto pref = mCombo->preferredSize(context);
        mCombo->setFrame(Rect(PicaPt::kZero, PicaPt::kZero, bounds().width, pref.height));

        Super::layout(context);
    }

private:
    ComboBox *mCombo;
};

class SliderTest : public Widget
{
    using Super = Widget;
public:
    SliderTest()
    {
        mIntEdit = new NumberEdit();
        addChild(mIntEdit);
        mIntLabel = new Label("");
        mIntLabel->setAlignment(Alignment::kLeft | Alignment::kVCenter);
        addChild(mIntLabel);

        mInt = new Slider();
        mInt->setLimits(0, 100);
        mInt->setValue(50);
        mIntEdit->setLimits(mInt->intMinLimit(), mInt->intMaxLimit(), mInt->intIncrement());
        mInt->setOnValueChanged([this](SliderLogic *s) {
            mIntEdit->setValue(s->intValue());
            mIntLabel->setText(std::to_string(s->intValue()));
        });
        addChild(mInt);

        mDoubleEdit = new NumberEdit();
        addChild(mDoubleEdit);
        mDoubleLabel = new Label("");
        mDoubleLabel->setAlignment(Alignment::kLeft | Alignment::kVCenter);
        addChild(mDoubleLabel);

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
        return Size(PicaPt(200), 3.0f * prefHeight + 3.0f * spacing);
    }

    void layout(const LayoutContext& context) override
    {
        auto x = PicaPt::kZero;
        auto y = PicaPt::kZero;
        auto sliderHeight = mInt->preferredSize(context).height;
        auto spacing = 0.25f * sliderHeight;
        auto labelWidth = 3.0f * sliderHeight;
        auto sliderWidth = frame().width - spacing - labelWidth;

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

class TextEditTest : public Widget
{
    using Super = Widget;
public:
    TextEditTest()
    {
        mString = new StringEdit();
        mString->setPlaceholderText("Edit string");
        addChild(mString);
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        auto pref = mString->preferredSize(context);
        return Size(PicaPt(200.0), 2.0f * pref.height);
    }

    void layout(const LayoutContext& context) override
    {
        auto pref = mString->preferredSize(context);
        mString->setFrame(Rect(PicaPt::kZero, PicaPt::kZero, bounds().width, pref.height));
        
        Super::layout(context);
    }

private:
    StringEdit *mString;
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
        mIncButton->setOnClicked([this](Button *b) {
            int n = std::atoi(mIncLabel->text().c_str());
            mIncLabel->setText(std::to_string(n + 1));
        });
        mScroll->addChild(mIncButton);
        mScroll->addChild(mIncLabel);

        mSliderLabel = new Label("33");
        mSlider = new Slider();
        mSlider->setValue(33);
        mSlider->setOnValueChanged([this](SliderLogic *s) {
            mSliderLabel->setText(std::to_string(s->intValue()));
        });
        mScroll->addChild(mSlider);
        mScroll->addChild(mSliderLabel);

        mButton2 = new Button("Magic");
        mButton2->setToggleable(true);
        mScroll->addChild(mButton2);
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

class AllWidgetsTest : public Widget
{
    using Super = Widget;
public:
    AllWidgetsTest()
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
        mText = new TextEditTest();
        addChild(mText);
        mScroll = new ScrollTest();
        addChild(mScroll);
        mListView = new ListViewTest();
        addChild(mListView);
    }

    void layout(const LayoutContext& context)
    {
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
        pref = mText->preferredSize(context);
        mText->setFrame(Rect(x, mProgress->frame().maxY(), pref.width, pref.height));
        pref = mScroll->preferredSize(context);
        mScroll->setFrame(Rect(x, mText->frame().maxY(), pref.width, pref.height));

        x += PicaPt(400);
        pref = mListView->preferredSize(context);
        mListView->setFrame(Rect(x, PicaPt::kZero, pref.width, pref.height));

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
    TextEditTest *mText;
    ScrollTest *mScroll;
    ListViewTest *mListView;
};

static const MenuId kMenuIdNew = 1;
static const MenuId kMenuIdQuit = 2;
static const MenuId kMenuIdDisabled = 10;
static const MenuId kMenuIdCheckable = 11;
static const MenuId kMenuIdAlpha = 30;
static const MenuId kMenuIdBeta = 31;
static const MenuId kMenuIdToggleAlpha = 32;

class Document : public Window
{
public:
    static Document* createNewDocument()
    {
        return new Document();
    }

    Document()
        : Window("UITK test widgets", 1024, 768)
    {
        setOnMenuWillShow([this](Menubar& menubar) {
            menubar.setItemEnabled(kMenuIdDisabled, false);
            menubar.setItemChecked(kMenuIdCheckable, mModel.testChecked);
            menubar.setItemChecked(kMenuIdAlpha, mModel.alphaChecked);
        });

        setOnMenuActivated(kMenuIdNew, [](){ Document::createNewDocument(); });
        setOnMenuActivated(kMenuIdQuit, [](){ Application::instance().quit(); });
        setOnMenuActivated(kMenuIdDisabled,
                           [](){ Application::instance().quit(); /* shouldn't get here b/c disabled */ });
        setOnMenuActivated(kMenuIdCheckable,
                           [this](){ mModel.testChecked = !mModel.testChecked; });
        setOnMenuActivated(kMenuIdToggleAlpha,
                           [this](){ mModel.alphaChecked = !mModel.alphaChecked; });

        addChild((new AllWidgetsTest())
                   ->setFrame(Rect(PicaPt(0), PicaPt(0), PicaPt(1024), PicaPt(768))));
//        addChild((new Label("Egypt"))
//                 ->setFrame(Rect(PicaPt(0), PicaPt(0), PicaPt(300), PicaPt(600))));
        show(true);
    }

private:
    struct {
        bool testChecked = true;
        bool alphaChecked = true;
    } mModel;
};

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
#else
int main(int argc, char *argv[])
#endif
{
    Application app;
    app.setExitWhenLastWindowCloses(true);

    auto *submenu = new Menu();
    submenu->addItem("Item 1", 20, ShortcutKey::kNone);
    submenu->addItem("Item 2", 21, ShortcutKey::kNone);
    submenu->addItem("Item 3", 22, ShortcutKey::kNone);
    auto subsubmenu = new Menu();
    subsubmenu->addItem("Alpha", kMenuIdAlpha, ShortcutKey::kNone);
    subsubmenu->addItem("Beta", kMenuIdBeta, ShortcutKey::kNone);
    subsubmenu->addItem("Toggle alpha action", kMenuIdToggleAlpha,
                        ShortcutKey(KeyModifier::kCtrl, Key::kA));
    submenu->addMenu("Subsubmenu", subsubmenu);
    submenu->addItem("Item 4", 23, ShortcutKey::kNone);

    auto *submenu2 = new Menu();
    submenu2->addItem("First", 40, ShortcutKey::kNone);
    submenu2->addItem("Second", 41, ShortcutKey::kNone);
    submenu2->addItem("Third", 42, ShortcutKey::kNone);

    app.menubar().newMenu("File")
        ->addItem("New", kMenuIdNew, ShortcutKey(KeyModifier::kCtrl, Key::kN))
        ->addSeparator()
        ->addItem("Quit", kMenuIdQuit, ShortcutKey(KeyModifier::kCtrl, Key::kQ));
    //app.menubar().newMenu("Edit");
    app.menubar().newMenu("_Test")
        ->addItem("_Disabled", kMenuIdDisabled, ShortcutKey(KeyModifier::kCtrl, Key::kD))
        ->addItem("_Checkable", kMenuIdCheckable, ShortcutKey::kNone)
        ->addSeparator()
        ->addMenu("Submenu", submenu)
        ->addMenu("Submenu 2", submenu2);
    app.menubar().newMenu("Empty");
    //app.menubar().newMenu("Help");

    // TODO: this is a memory leak, figure out a way for UITK to manage these
    Document::createNewDocument();

    return app.run();
}
