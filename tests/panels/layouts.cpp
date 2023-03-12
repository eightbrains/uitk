//-----------------------------------------------------------------------------
// Copyright 2021 - 2023 Eight Brains Studios, LLC
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

namespace layouts {

struct SizePx
{
    float width;
    float height;
};

class TestWidget : public Widget
{
    using Super = Widget;
public:
    TestWidget(const Size& size, Dir dir = Dir::kHoriz)
        : mDir(dir), mSize(size)
    {
        auto cb = new Checkbox(dir == Dir::kHoriz ? "<->" : "Grow");
        cb->setOnClicked([this](Button*) {
            mStretch = !mStretch;
            setNeedsLayout();
        });
        addChild(cb);
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        if (mDir == Dir::kHoriz) {
            if (mStretch) {
                return Size(kDimGrow, mSize.height);
            }
            return mSize;
        } else {
            // Return height for width here, because mSize represents major/minor
            // axes, not x/y.
            if (mStretch) {
                return Size(mSize.height, kDimGrow);
            }
            return Size(mSize.height, mSize.width);
        }
    }

    void layout(const LayoutContext& context) override
    {
        auto pref = children()[0]->preferredSize(context);
        auto &r = bounds();
        children()[0]->setFrame(Rect(r.midX() - 0.5f * pref.width, r.midY() - 0.5f * pref.height,
                                     pref.width, pref.height));

        Super::layout(context);
    }

protected:
    Dir mDir;
    Size mSize;
    bool mStretch = false;
};

/*class FixedSizeLayout : public Layout1D
{
    using Super = Layout1D;
public:
    FixedSizeLayout(Dir dir, const Size& size)
        : Layout1D(dir)
        , mSize(size)
    {}

    Size preferredSize(const LayoutContext& size) const override { return mSize; }

protected:
    Size mSize;
}; */

class Panel : public Widget
{
    using Super = Widget;

    static const int kTypeHoriz = 0;
    static const int kTypeVert = 1;
    static const int kTypeGrid = 2;

public:
    Panel()
    {
        mSpacingEm = 1.0f;
        mMarginEm = 0.0f;

//        mLayout = new FixedSizeLayout(Dir::kHoriz, Size(PicaPt(400.0f), PicaPt(200.0f))),
        mLayout = new HLayout();
        addChild(mLayout);

        ComboBox *layoutType;
        SliderLogic *spacing, *margin;
        NumberEdit *spacingNum, *marginNum;

        mPropertyGrid = new GridLayout({
            {
                new Label("Layout type"),
                layoutType = (new ComboBox())->addItem("HLayout", kTypeHoriz)
                                             ->addItem("VLayout", kTypeVert)
                                             ->addItem("Grid", kTypeGrid)
            },
            {
                new Label("Spacing (em)"),
                new HLayout({
                    spacing = (new Slider())->setLimits(0.0f, 3.0f, 0.01f)
                                            ->setValue(mSpacingEm),
                    spacingNum = (new NumberEdit())->setLimits(0.0f, 3.0f, 0.01f)
                                                   ->setValue(mSpacingEm)
                })
            },
            {
                new Label("Margin (em)"),
                new HLayout({
                    margin = (new Slider())->setLimits(0.0f, 1.0f, 0.01f)
                                           ->setValue(mMarginEm),
                    marginNum = (new NumberEdit())->setLimits(0.0f, 1.0f, 0.01f)
                                                  ->setValue(mMarginEm)
                })
            },
            {
                new Label("Alignment"),
                mHorizAlign = (new SegmentedControl())
                    ->addItem("0")
                    ->addItem("L")
                    ->addItem("C")
                    ->addItem("R"),
            },
            {
                new Layout::SpacingEm(Dir::kHoriz, 1.0f),  // placeholder
                mVertAlign = (new SegmentedControl())
                    ->addItem("0")
                    ->addItem("T")
                    ->addItem("C")
                    ->addItem("B"),
            }
        });
        mPropertyGrid->setSpacingEm(1.0f);
        mPropertyGrid->setMarginsEm(1.0f);
        addChild(mPropertyGrid);

        layoutType->setOnSelectionChanged([this](ComboBox *b) {
            recreateLayout(b->selectedValue());
        });

        spacing->setOnValueChanged([this, spacingNum](SliderLogic *slider) {
            mSpacingEm = float(slider->doubleValue());
            spacingNum->setValue(mSpacingEm);
            updateLayoutConfig();
        });
        spacingNum->setOnValueChanged([this, spacing](NumberEdit *num) {
            mSpacingEm = float(num->doubleValue());
            spacing->setValue(mSpacingEm);
            updateLayoutConfig();
        });
        margin->setOnValueChanged([this, marginNum](SliderLogic *slider) {
            mMarginEm = float(slider->doubleValue());
            marginNum->setValue(mMarginEm);
            updateLayoutConfig();
        });
        marginNum->setOnValueChanged([this, margin](NumberEdit *num) {
            mMarginEm = float(num->doubleValue());
            margin->setValue(mMarginEm);
            updateLayoutConfig();
        });

        mHorizAlign->setAction(SegmentedControl::Action::kSelectOne);
        mVertAlign->setAction(SegmentedControl::Action::kSelectOne);
        mHorizAlign->setSegmentOn(0, true);
        mVertAlign->setSegmentOn(0, true);
        mHorizAlign->setOnClicked([this](int idx) { this->updateLayoutConfig(); });
        mVertAlign->setOnClicked([this](int idx) { this->updateLayoutConfig(); });

        recreateLayout(layoutType->selectedValue());
    }

    void recreateLayout(int type)
    {
        delete removeChild(mLayout);
        if (type == kTypeHoriz) {
            mLayout = new HLayout();
            mLayout->addChild(new TestWidget(Size(PicaPt(mSizes[0].width), PicaPt(mSizes[0].height))));
            mLayout->addChild(new TestWidget(Size(PicaPt(mSizes[1].width), PicaPt(mSizes[1].height))));
            mLayout->addChild(new TestWidget(Size(PicaPt(mSizes[2].width), PicaPt(mSizes[2].height))));
        } else if (type == kTypeVert) {
            mLayout = new VLayout();
            mLayout->addChild(new TestWidget(Size(PicaPt(mSizes[0].width), PicaPt(mSizes[0].height)),
                                             Dir::kVert));
            mLayout->addChild(new TestWidget(Size(PicaPt(mSizes[1].width), PicaPt(mSizes[1].height)),
                                             Dir::kVert));
            mLayout->addChild(new TestWidget(Size(PicaPt(mSizes[2].width), PicaPt(mSizes[2].height)),
                                             Dir::kVert));
        } else if (type == kTypeGrid) {
            auto *grid = new GridLayout();
            grid->addChild(new TestWidget(Size(PicaPt(mSizes[0].width), PicaPt(mSizes[0].height))), 0, 0);
            grid->addChild(new TestWidget(Size(PicaPt(mSizes[1].width), PicaPt(mSizes[1].height))), 0, 1);
            grid->addChild(new TestWidget(Size(PicaPt(mSizes[2].width), PicaPt(mSizes[2].height))), 1, 0);
            grid->addChild(new TestWidget(Size(PicaPt(mSizes[4].width), PicaPt(mSizes[4].height))), 2, 1);
            mLayout = grid;
        } else {
            assert(false);
            mLayout = nullptr;
        }
        addChild(mLayout);
        updateLayoutConfig();
    }

    void updateLayoutConfig()
    {
        int alignment = 0;
        if (mHorizAlign->isSegmentOn(0)) {
            alignment |= 0;
        } else if (mHorizAlign->isSegmentOn(1)) {
            alignment |= Alignment::kLeft;
        } else if (mHorizAlign->isSegmentOn(2)) {
            alignment |= Alignment::kHCenter;
        } else if (mHorizAlign->isSegmentOn(3)) {
            alignment |= Alignment::kRight;
        }
        if (mVertAlign->isSegmentOn(0)) {
            alignment |= 0;
        } else if (mVertAlign->isSegmentOn(1)) {
            alignment |= Alignment::kTop;
        } else if (mVertAlign->isSegmentOn(2)) {
            alignment |= Alignment::kVCenter;
        } else if (mVertAlign->isSegmentOn(3)) {
            alignment |= Alignment::kBottom;
        }
        mLayout->setAlignment(alignment);
        mLayout->setSpacingEm(mSpacingEm);
        mLayout->setMarginsEm(mMarginEm);
    }

    void layout(const LayoutContext& context) override
    {
        auto em = context.theme.params().labelFont.pointSize();
        auto margin = context.dc.roundToNearestPixel(em);

        // We manually layout the test layout and the grid, rather than using
        // a layout, because it is easier to recreate the test layout that way.
        auto gridWidth = context.dc.roundToNearestPixel(25.0f * em);
        auto b = bounds();
        if (auto hlayout = dynamic_cast<HLayout*>(mLayout)) {
            mLayout->setFrame(Rect(margin, margin,
                                         context.dc.roundToNearestPixel(PicaPt(400)),
                                         context.dc.roundToNearestPixel(PicaPt(200))));
        } else if (auto vlayout = dynamic_cast<VLayout*>(mLayout)) {
            mLayout->setFrame(Rect(margin, margin,
                                         context.dc.roundToNearestPixel(PicaPt(200)),
                                         context.dc.roundToNearestPixel(PicaPt(400))));
        } else if (auto grid = dynamic_cast<GridLayout*>(mLayout)) {
            auto pref = mLayout->preferredSize(context);
            mLayout->setFrame(Rect(margin, margin,
                                         context.dc.roundToNearestPixel(PicaPt(400)),
                                         context.dc.roundToNearestPixel(std::min(pref.height, PicaPt(400)))));
        }
        mPropertyGrid->setFrame(Rect(b.maxX() - gridWidth, b.y, gridWidth, b.height));

        mLayout->setBorderColor(Color(0.5f, 0.5f, 0.5f));
        mLayout->setBorderWidth(context.dc.roundToNearestPixel(PicaPt::fromStandardPixels(1.0f)));

        auto &widgets = mLayout->children();
        for (size_t i = 0;  i < widgets.size();  ++i) {
            widgets[i]->setFrame(Rect(PicaPt::kZero, PicaPt::kZero,
                                      PicaPt(mSizes[i].width), PicaPt(mSizes[i].height)));
            widgets[i]->setBorderColor(context.theme.params().textColor);
            widgets[i]->setBorderWidth(context.dc.roundToNearestPixel(PicaPt::fromStandardPixels(1.0f)));
            widgets[i]->setBackgroundColor(mColors[i]);
        }

        Super::layout(context);
    }

private:
    std::vector<Color> mColors = { Color(Color::kRed, 0.2f), Color(Color::kOrange, 0.2f),
                                   Color(Color::kYellow, 0.2f), Color(Color::kGreen, 0.2f),
                                   Color(Color::kBlue, 0.2f), Color(Color::kPurple, 0.2f) };
    std::vector<SizePx> mSizes = { SizePx{ 75, 50 }, { 50, 75 }, { 100, 50 },
                                     { 50, 100 }, { 37, 37 }, { 50, 50 } };
    SegmentedControl *mHorizAlign;
    SegmentedControl *mVertAlign;
    float mSpacingEm;
    float mMarginEm;
//    FixedSizeLayout *mLayout;
    GridLayout *mPropertyGrid;
    Layout *mLayout;
};

}  // namespace layouts
