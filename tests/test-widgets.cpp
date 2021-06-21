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
        auto button = context.theme.calcPreferredButtonSize(context,
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
        return Size(pref1.width + pref1.height + 1.3 * pref2.width, 3.0f * pref1.height);
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

        Super::layout(context);
    }

private:
    SizeTest *mSizing;
    LabelTest *mLabels;
    ButtonTest *mButtons;
    SegmentsTest *mSegments;
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

    Window w("UITK test widgets", 1024, 768);

    w.addChild((new AllWidgetsTest())
               ->setFrame(Rect(PicaPt(0), PicaPt(0), PicaPt(300), PicaPt(768))));
//    w.addChild((new Label("Egypt"))
//               ->setFrame(Rect(PicaPt(0), PicaPt(0), PicaPt(300), PicaPt(600))));
    w.show(true);
    return app.run();
}
