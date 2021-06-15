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

    Size preferredSize(const LayoutContext& context) const
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

        mLeft = new Button("L");
        mLeft->setOnClicked([this](Button*) { setHoriz(Alignment::kLeft); } );
        addChild(mLeft);
        mHCenter = new Button("C");
        mHCenter->setOnClicked([this](Button*) { setHoriz(Alignment::kHCenter); } );
        addChild(mHCenter);
        mRight = new Button("R");
        mRight->setOnClicked([this](Button*) { setHoriz(Alignment::kRight); } );
        addChild(mRight);
        mTop = new Button("T");
        mTop->setOnClicked([this](Button*) { setVert(Alignment::kTop); } );
        addChild(mTop);
        mVCenter = new Button("C");
        mVCenter->setOnClicked([this](Button*) { setVert(Alignment::kVCenter); } );
        addChild(mVCenter);
        mBottom = new Button("B");
        mBottom->setOnClicked([this](Button*) { setVert(Alignment::kBottom); } );
        addChild(mBottom);
    }

    Size preferredSize(const LayoutContext& context) const
    {
        auto em = context.theme.params().labelFont.metrics(context.dc).lineHeight;
        return Size(20.0f * em, 7.0f * em);
    }

    void layout(const LayoutContext& context)
    {
        auto setToPref = [&context](Widget *w) {
            auto pref = w->preferredSize(context);
            w->setFrame(Rect(PicaPt(0), PicaPt(0), pref.width, pref.height));
        };

        for (auto *child : children()) {
            setToPref(child);
        }

        PicaPt y(8);
        mLeft->setPosition(Point(PicaPt(8), y));
        mHCenter->setPosition(Point(mLeft->frame().maxX(), y));
        mRight->setPosition(Point(mHCenter->frame().maxX(), y));
        mTop->setPosition(Point(mRight->frame().maxX() + PicaPt(8), y));
        mVCenter->setPosition(Point(mTop->frame().maxX(), y));
        mBottom->setPosition(Point(mVCenter->frame().maxX(), y));

        mLabel->setFrame(Rect(mLeft->frame().x, mLeft->frame().maxY() + PicaPt(8),
                              mBottom->frame().maxX() - mLeft->frame().x,
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
    Button *mLeft;
    Button *mHCenter;
    Button *mRight;
    Button *mTop;
    Button *mVCenter;
    Button *mBottom;
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
    }

    Size preferredSize(const LayoutContext& context) const
    {
        auto button = context.theme.calcPreferredButtonSize(context,
                                                            context.theme.params().labelFont,
                                                            mDisabled->label()->text());
        return Size(5.0f * button.width, 2.25f * button.height);
    }

    void layout(const LayoutContext& context)
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

        Super::layout(context);
    }

private:
    Button *mHappy;
    Button *mAngry;
    Button *mDisabled;
    Button *mOnOff;
    Button *mOnOffDisabled;
    Label *mLabel;
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

        Super::layout(context);
    }

private:
    SizeTest *mSizing;
    LabelTest *mLabels;
    ButtonTest *mButtons;
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
               ->setFrame(Rect(PicaPt(0), PicaPt(0), PicaPt(300), PicaPt(600))));
//    w.addChild((new Label("Egypt"))
//               ->setFrame(Rect(PicaPt(0), PicaPt(0), PicaPt(300), PicaPt(600))));
    w.show(true);
    return app.run();
}
