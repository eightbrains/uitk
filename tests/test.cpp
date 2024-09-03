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

#include <uitk/uitk.h>

#include "TestCase.h"

#include <sstream>

using namespace uitk;

struct SizePx
{
    float width;
    float height;

    SizePx() : width(0.0f), height(0.0f) {}
    SizePx(float w, float h) : width(w), height(h) {}
};

class IPixelBased
{
public:
    virtual float xPx() const = 0;
    virtual float yPx() const = 0;
    virtual float widthPx() const = 0;
    virtual float heightPx() const = 0;
};

class TestWidget : public Widget, public IPixelBased
{
    using Super = Widget;
protected:
    SizePx mPxPref;
    PicaPt mOnePx;
public:
    explicit TestWidget(const SizePx& prefSize) : Widget(), mPxPref(prefSize) {}
    ~TestWidget() {}

    float xPx() const override { return frame().x / mOnePx; }
    float yPx() const override { return frame().y / mOnePx; }
    float widthPx() const override { return frame().width / mOnePx; }
    float heightPx() const override { return frame().height / mOnePx; }

    Size preferredSize(const LayoutContext &context) const override
    {
        auto onePx = context.dc.onePixel();
        // We don't know the size of a pixel in the constructor, so we just pass
        // kDimGrow.asFloat(), which is 32000 px. But if we need to compare against
        // kDimGrow, val * onePx will actually be smaller than kDimPx, so check that
        // and substitute kDimGrow. (Since our sizes in the test cases are about 300 px,
        // anything large is essentially kDimGrow anyway)
        auto w = mPxPref.width * onePx;
        if (mPxPref.width >= kDimGrow.asFloat()) {
            w = kDimGrow;
        }
        auto h = mPxPref.height * onePx;
        if (mPxPref.height >= kDimGrow.asFloat()) {
            h = kDimGrow;
        }
        return Size(w, h);
    }

    void layout(const LayoutContext& context) override
    {
        mOnePx = context.dc.onePixel();
        Super::layout(context);
    }
};

class TestSublayout1D : public Layout1D, public IPixelBased
{
    using Super = Layout1D;
protected:
    PicaPt mOnePx;
public:
    TestSublayout1D(Dir dir) : Layout1D(dir) {}

    float xPx() const override { return frame().x / mOnePx; }
    float yPx() const override { return frame().y / mOnePx; }
    float widthPx() const override { return frame().width / mOnePx; }
    float heightPx() const override { return frame().height / mOnePx; }

    void layout(const LayoutContext& context) override
    {
        mOnePx = context.dc.onePixel();
        Super::layout(context);
    }
};

class TestLayout1D : public Layout1D
{
    using Super = Layout1D;
protected:
    SizePx mSize;
    PicaPt mOnePx;
public:
    explicit TestLayout1D(Dir dir, const SizePx& size) : Layout1D(dir), mSize(size) {}
    ~TestLayout1D() {}

    const PicaPt& onePx() const { return mOnePx; }

    Size preferredSize(const LayoutContext &context) const override
    {
        auto onePx = context.dc.onePixel();
        return Size(mSize.width * onePx, mSize.height * onePx);
    }

    void layout(const LayoutContext& context) override
    {
        mOnePx = context.dc.onePixel();
        Super::layout(context);
    }
};

class TestGridLayout : public GridLayout
{
    using Super = GridLayout;
protected:
    SizePx mSize;
    PicaPt mOnePx;
public:
    explicit TestGridLayout(const SizePx& size) : GridLayout(), mSize(size) {}
    ~TestGridLayout() {}

    const PicaPt& onePx() const { return mOnePx; }

    Size preferredSize(const LayoutContext &context) const override
    {
        auto onePx = context.dc.onePixel();
        return Size(mSize.width * onePx, mSize.height * onePx);
    }

    void layout(const LayoutContext& context) override
    {
        mOnePx = context.dc.onePixel();
        Super::layout(context);
    }
};

//-----------------------------------------------------------------------------
class LayoutTest : public TestCase
{
protected:
    int mAlign;
    float mSpacingPx;
    std::array<float, 4> mMarginsPx;
    SizePx mSizePx;
    std::vector<SizePx> mInputPxSizes;
    std::vector<SizePx> mSublayoutPxSizes;
    std::vector<SizePx> mExpectedPxSizes;

    enum class SublayoutDir { kSame, kOpposite };
    SublayoutDir mSublayoutDir = SublayoutDir::kSame;

public:
    LayoutTest(const std::string& name, int align = 0, float spacingPx = 0.f,
               const std::array<float, 4>& marginsPx = { 0.0f, 0.0f, 0.0f, 0.0f })
        : TestCase(name)
    {
        mAlign = align;
        mSpacingPx = spacingPx;
        mMarginsPx = marginsPx;
    }

    ~LayoutTest() {}
    
    std::string run() override
    {
        std::string err;
        err = runLayout(Dir::kHoriz);
        if (!err.empty()) {
            return "horiz: " + err;
        }
        err = runLayout(Dir::kVert);
        if (!err.empty()) {
            return "vert: " + err;
        }

        return "";
    }

    virtual TestLayout1D* setupLayout(Dir dir)
    {
        auto subdir = dir;
        if (mSublayoutDir == SublayoutDir::kOpposite) {
            if (dir == Dir::kHoriz) {
                subdir = Dir::kVert;
            } else {
                subdir = Dir::kHoriz;
            }
        }

        // Setup layout
        TestLayout1D *layout = nullptr;
        if (dir == Dir::kHoriz) {
            layout = new TestLayout1D(Dir::kHoriz, mSizePx);
            if (mAlign != 0) {
                layout->setAlignment(mAlign);
            }
            for (auto &pref : mInputPxSizes) {
                if (pref.width >= 0) {
                    layout->addChild(new TestWidget(pref));
                } else {
                    // Sublayout cannot be TestLayout1D, since that overrides preferredSize()
                    Layout1D *sublayout = new TestSublayout1D(subdir);
                    sublayout->setSpacing(PicaPt::kZero);
                    for (auto &subPref : mSublayoutPxSizes) {
                        sublayout->addChild(new TestWidget(subPref));
                    }
                    layout->addChild(sublayout);
                }
            }
        } else {
            layout = new TestLayout1D(Dir::kVert, SizePx(mSizePx.height, mSizePx.width));
            if (mAlign != 0) {
                layout->setAlignment(((mAlign & Alignment::kVertMask) >> 4) |
                                     ((mAlign & Alignment::kHorizMask) << 4));  // swap vert / horiz
            }
            for (auto &pref : mInputPxSizes) {
                if (pref.width >= 0) {
                    layout->addChild(new TestWidget(SizePx(pref.height, pref.width)));
                } else {
                    Layout1D *sublayout = new TestSublayout1D(subdir);
                    sublayout->setSpacing(PicaPt::kZero);
                    for (auto &subPref : mSublayoutPxSizes) {
                        sublayout->addChild(new TestWidget(SizePx(subPref.height, subPref.width)));
                    }
                    layout->addChild(sublayout);
                }
            }
        }

        return layout;
    }

    virtual std::string runLayout(Dir dir)
    {
        TestLayout1D *layout = setupLayout(dir);

        Window window("UITK test", layout->frame().width, layout->frame().height);
        Widget *root = new Widget();
        // Put layout in a child, so that the window will resize the root, but we can manually size the layout
        root->addChild(layout);
        window.addChild(root);
        window.setOnWindowLayout([layout, this, root](Window &, const LayoutContext &context) {
            root->setFrame(Rect(PicaPt::kZero, PicaPt::kZero, PicaPt(10.0f), PicaPt(10.0f)));
            auto pref = layout->preferredSize(context);
            layout->setFrame(Rect(PicaPt::kZero, PicaPt::kZero, pref.width, pref.height));
            auto onePx = context.dc.onePixel();
            layout->setSpacing(mSpacingPx * onePx);
            layout->setMargins(mMarginsPx[0] * onePx, mMarginsPx[1] * onePx,
                               mMarginsPx[2] * onePx, mMarginsPx[3] * onePx);
        });
        // The actual size of the window does not really matter (as long as it is not zero), as we will
        // set the layout's size manually above.
        window.resize(Size(PicaPt(500), PicaPt(500)));

        // Evaluate if layout worked properly
        auto &children = layout->children();
        if (children.size() != mExpectedPxSizes.size()) {
            return "layout has incorrect number of children: got " + std::to_string(children.size()) + ", expected " + std::to_string(mExpectedPxSizes.size()) + "\n";
        }
        float expectedStartPx = (dir == Dir::kHoriz ? mMarginsPx[0] : mMarginsPx[1]);
        float expectedSizesSum = 0.0f;
        for (auto &sz : mExpectedPxSizes) { expectedSizesSum += sz.width; }
        if (mAlign & Alignment::kHCenter) {
            expectedStartPx += 0.5f * (mSizePx.width - expectedSizesSum + float(mExpectedPxSizes.size() - 1) * mSpacingPx);
        } else if (mAlign & Alignment::kRight) {
            expectedStartPx = mSizePx.width - expectedSizesSum + float(mExpectedPxSizes.size() - 1) * mSpacingPx;
        }
        for (size_t i = 0;  i < children.size();  ++i) {
            auto *tw = dynamic_cast<IPixelBased*>(children[i]);
            if (dir == Dir::kHoriz) {
                if (std::abs(tw->widthPx() - mExpectedPxSizes[i].width) > 1e-3) {
                    return "item " + std::to_string(i) + ": got width " + std::to_string(tw->widthPx()) + ", expected " + std::to_string(mExpectedPxSizes[i].width) + "\n" + layoutDescription(layout);
                }
                if (std::abs(tw->heightPx() - mExpectedPxSizes[i].height) > 1e-3) {
                    return "item " + std::to_string(i) + ": got height " + std::to_string(tw->heightPx()) + ", expected " + std::to_string(mExpectedPxSizes[i].height) + "\n" + layoutDescription(layout);
                }
                if (std::abs(tw->xPx() - expectedStartPx) > 1e-3f) {
                    return "item " + std::to_string(i) + ": got x = " + std::to_string(tw->xPx()) + ", expected " + std::to_string(expectedStartPx) + "\n" + layoutDescription(layout);
                }

                if (mAlign & Alignment::kVCenter) {
                    auto expectedY = mMarginsPx[1] + 0.5f * (mSizePx.height - mExpectedPxSizes[i].height);
                    expectedY = std::round(expectedY);
                    if (std::abs(tw->yPx() - expectedY) > 1e-3f) {
                        return "item " + std::to_string(i) + ": minor aligned center, got y = " + std::to_string(tw->yPx()) + ", expected " + std::to_string(expectedY) + "\n" + layoutDescription(layout);
                    }
                } else if (mAlign & Alignment::kBottom) {
                    auto expectedY = mMarginsPx[1] + mSizePx.height - mExpectedPxSizes[i].height;
                    if (std::abs(tw->yPx() - expectedY) > 1e-3f) {
                        return "item " + std::to_string(i) + ": minor aligned bottom, got y = " + std::to_string(tw->yPx()) + ", expected " + std::to_string(expectedY) + "\n" + layoutDescription(layout);
                    }
                } else {
                    if (std::abs(tw->yPx() - mMarginsPx[1]) > 1e-3f) {
                        return "item " + std::to_string(i) + ": minor aligned top, got y = " + std::to_string(tw->yPx()) + ", expected 0.0f\n" + layoutDescription(layout);
                    }
                }
            } else {
                // Note: we flip expected width/height for vertical layouts
                //       so that we can use the same data but still exercise the vertical layout code
                if (std::abs(tw->widthPx() - mExpectedPxSizes[i].height) > 1e-3) {
                    return "item " + std::to_string(i) + ": got width " + std::to_string(tw->widthPx()) + ", expected " + std::to_string(mExpectedPxSizes[i].height) + "\n" + layoutDescription(layout);
                }
                if (std::abs(tw->heightPx() - mExpectedPxSizes[i].width) > 1e-3) {
                    return "item " + std::to_string(i) + ": got height " + std::to_string(tw->heightPx()) + ", expected " + std::to_string(mExpectedPxSizes[i].width) + "\n" + layoutDescription(layout);
                }
                if (std::abs(tw->yPx() - expectedStartPx) > 1e-3f) {
                    return "item " + std::to_string(i) + ": got y = " + std::to_string(tw->yPx()) + ", expected " + std::to_string(expectedStartPx) + "\n" + layoutDescription(layout);
                }

                if (mAlign & Alignment::kHCenter) {
                    auto expectedX = mMarginsPx[0] + 0.5f * (mSizePx.height - mExpectedPxSizes[i].height);
                    expectedX = std::round(expectedX);
                    if (std::abs(tw->xPx() - expectedX) > 1e-3f) {
                        return "item " + std::to_string(i) + ": minor aligned center, got x = " + std::to_string(tw->xPx()) + ", expected " + std::to_string(expectedX) + "\n" +
                            layoutDescription(layout);
                    }
                } else if (mAlign & Alignment::kBottom) {
                    auto expectedX = mMarginsPx[0] + mSizePx.height - mExpectedPxSizes[i].height;
                    if (std::abs(tw->xPx() - expectedX) > 1e-3f) {
                        return "item " + std::to_string(i) + ": minor aligned bottom, got x = " + std::to_string(tw->xPx()) + ", expected " + std::to_string(expectedX) + "\n" +
                            layoutDescription(layout);
                    }
                } else {
                    if (std::abs(tw->xPx() - mMarginsPx[0]) > 1e-3f) {
                        return "item " + std::to_string(i) + ": minor aligned top, got x = " + std::to_string(tw->xPx()) + ", expected 0.0f\n" + layoutDescription(layout);
                    }
                }
            }
            expectedStartPx += mExpectedPxSizes[i].width + mSpacingPx;
        }

        return "";
    }

    std::string layoutDescription(TestLayout1D *layout)
    {
        auto onePx = layout->onePx();
        auto printChild = [onePx](std::stringstream& s, const char *indent, int idx, Widget *child) {
            s << indent << "[" << idx << "]: (" << child->frame().x / onePx << ", " << child->frame().y / onePx
            << ") " << child->frame().width / onePx << " x " << child->frame().height / onePx;
        };
        std::stringstream s;
        s << "    Layout [" << (layout->dir() == Dir::kHoriz ? "kHoriz" : "kVert") << "], spacing: "
          << mSpacingPx << "px, margins px: {" << mMarginsPx[0] << ", " << mMarginsPx[1] << ", "
          << mMarginsPx[2] << ", " << mMarginsPx[3] << "}, size: (" << layout->frame().width / onePx
          << " x " << layout->frame().height / onePx << ")" << std::endl;
        int i = 0;
        for (auto *child : layout->children()) {
            printChild(s, "        ", i++, child);
            if (auto *sub = dynamic_cast<TestSublayout1D*>(child)) {
                s << ", layout: " << (sub->dir() == Dir::kHoriz ? "kHoriz" : "kVert");
            }
            s << std::endl;
            if (auto *sub = dynamic_cast<TestSublayout1D*>(child)) {
                int j = 0;
                for (auto *subchild : sub->children()) {
                    printChild(s, "            ", j++, subchild);
                    s << std::endl;
                }
            }
        }
        return s.str();
    }
};

class NoItemsLayoutTest : public LayoutTest
{
public:
    NoItemsLayoutTest() : LayoutTest("layout (no items)")
    {
        mSizePx = SizePx(300, 100);
        mInputPxSizes = {};  // just tests that we don't crash or something
        mExpectedPxSizes = {};
    }
};

class OneItemLayoutTest : public LayoutTest
{
public:
    OneItemLayoutTest() : LayoutTest("layout (one item, pref != kDimGrow)")
    {
        mSizePx = SizePx(300, 100);
        mInputPxSizes = { mSizePx };
        mExpectedPxSizes = { mSizePx };
    }
};

class OneGrowingItemLayoutTest : public LayoutTest
{
public:
    OneGrowingItemLayoutTest() : LayoutTest("layout (one item, pref == kDimGrow)")
    {
        mSizePx = SizePx(300, 100);
        mInputPxSizes = { SizePx(Widget::kDimGrow.asFloat(), 100) };
        mExpectedPxSizes = { mSizePx };
    }
};

class ExpandItemsLayoutTest : public LayoutTest
{
public:
    ExpandItemsLayoutTest() : LayoutTest("layout (size=300, items={50, 50, 50})")
    {
        mSizePx = SizePx(300, 100);
        mInputPxSizes = { SizePx(50, 100), SizePx(50, 100), SizePx(50, 100) };
        mExpectedPxSizes = { SizePx(100, 100), SizePx(100, 100), SizePx(100, 100) };
    }
};

class GrowItemsLayoutTest : public LayoutTest
{
public:
    GrowItemsLayoutTest() : LayoutTest("layout (size=300, items={grow, 100, grow})")
    {
        mSizePx = SizePx(300, 100);
        mInputPxSizes = { SizePx(Widget::kDimGrow.asFloat(), 100),
                          SizePx(100, 100),
                          SizePx(Widget::kDimGrow.asFloat(), 100) };
        mExpectedPxSizes = { SizePx(100, 100), SizePx(100, 100), SizePx(100, 100) };
    }
};

class ShrinkItemsLayoutTest : public LayoutTest
{
public:
    ShrinkItemsLayoutTest() : LayoutTest("layout (size=300, items={100, 150, 200}")
    {
        mSizePx = SizePx(300, 100);
        mInputPxSizes = { SizePx(100, 100), SizePx(150, 100), SizePx(200, 100) };
        mExpectedPxSizes = { SizePx(50, 100), SizePx(100, 100), SizePx(150, 100) };
    }
};

class AlignExpandLayoutTest : public LayoutTest
{
public:
    AlignExpandLayoutTest() : LayoutTest("layout (align-minor: expand)")
    {
        mSizePx = SizePx(200, 200);
        mInputPxSizes = { SizePx(100, 100), SizePx(100, 150) };
        mExpectedPxSizes = { SizePx(100, 200), SizePx(100, 200) };
    }
};

class AlignTopLayoutTest : public LayoutTest
{
public:
    AlignTopLayoutTest() : LayoutTest("layout (align-minor: top)", Alignment::kTop)
    {
        mSizePx = SizePx(200, 200);
        mInputPxSizes = { SizePx(100, 100), SizePx(100, 150) };
        mExpectedPxSizes = { SizePx(100, 100), SizePx(100, 150) };
    }
};

class AlignCenterLayoutTest : public LayoutTest
{
public:
    AlignCenterLayoutTest() : LayoutTest("layout (align-minor: center)", Alignment::kCenter)
    {
        mSizePx = SizePx(200, 200);
        mInputPxSizes = { SizePx(100, 100), SizePx(100, 150) };
        mExpectedPxSizes = { SizePx(100, 100), SizePx(100, 150) };
    }
};

class AlignBottomLayoutTest : public LayoutTest
{
public:
    AlignBottomLayoutTest() : LayoutTest("layout (align-minor: bottom)", Alignment::kBottom)
    {
        mSizePx = SizePx(200, 200);
        mInputPxSizes = { SizePx(100, 100), SizePx(100, 150) };
        mExpectedPxSizes = { SizePx(100, 100), SizePx(100, 150) };
    }
};

class AlignHCenterLayoutTest : public LayoutTest
{
public:
    AlignHCenterLayoutTest() : LayoutTest("layout (align: hcenter)", Alignment::kHCenter)
    {
        mSizePx = SizePx(200, 200);
        mInputPxSizes = { SizePx(50, 100), SizePx(50, 150) };
        mExpectedPxSizes = { SizePx(50, 200), SizePx(50, 200) };
    }
};

class AlignRightLayoutTest : public LayoutTest
{
public:
    AlignRightLayoutTest() : LayoutTest("layout (align: right)", Alignment::kRight)
    {
        mSizePx = SizePx(200, 200);
        mInputPxSizes = { SizePx(50, 100), SizePx(50, 150) };
        mExpectedPxSizes = { SizePx(50, 200), SizePx(50, 200) };
    }
};

class NestedFixedLayoutTest : public LayoutTest
{
public:
    NestedFixedLayoutTest() : LayoutTest("layout (nested fixed)")
    {
        mSizePx = SizePx(300, 100);
        mInputPxSizes = { SizePx(50, 100), SizePx(-1, 100), SizePx(50, 100) };
        mSublayoutPxSizes = { SizePx(50, 100), SizePx(50, 100) };
        // The extra space in 33.333 px, so the first item gets one extra pixel
        // and the others none.
        mExpectedPxSizes = { SizePx(84, 100), SizePx(133, 100), SizePx(83, 100) };
    }
};

class NestedGrowLayoutTest : public LayoutTest
{
public:
    NestedGrowLayoutTest() : LayoutTest("layout (nested grow)")
    {
        mSizePx = SizePx(300, 100);
        mInputPxSizes = { SizePx(50, 100), SizePx(-1, 100), SizePx(50, 100) };
        mSublayoutPxSizes = { SizePx(10000, 100), SizePx(10000, 100) };
        mExpectedPxSizes = { SizePx(50, 100), SizePx(200, 100), SizePx(50, 100) };
    }
};

class NestedGrow2LayoutTest : public LayoutTest
{
public:
    NestedGrow2LayoutTest() : LayoutTest("layout (nested grow 2)")
    {
        mSizePx = SizePx(300, 100);
        mInputPxSizes = { SizePx(50, 100), SizePx(-1, 100), SizePx(Widget::kDimGrow.asFloat(), 100) };
        mSublayoutPxSizes = { SizePx(Widget::kDimGrow.asFloat(), 100),
                              SizePx(Widget::kDimGrow.asFloat(), 100) };
        mExpectedPxSizes = { SizePx(50, 100), SizePx(125, 100), SizePx(125, 100) };
    }
};

class MarginsLayoutTest : public LayoutTest
{
public:
    MarginsLayoutTest() : LayoutTest("layout (margins, spacing)", 0, 5.0f, { 2.0f, 2.0f, 2.0f, 2.0f })
    {
        mSizePx = SizePx(300, 104);
        mInputPxSizes = { SizePx(Widget::kDimGrow.asFloat(), 100),
                          SizePx(Widget::kDimGrow.asFloat(), 100),
                          SizePx(Widget::kDimGrow.asFloat(), 100) };
        mExpectedPxSizes = { SizePx(96, 100), SizePx(95, 100), SizePx(95, 100) };
    }
};

class TransverseFixedLayoutTest : public LayoutTest
{
public:
    TransverseFixedLayoutTest() : LayoutTest("layout (transverse with fixed and grow)", 0)
    {
        mSizePx = SizePx(300, 100);
        mInputPxSizes = { SizePx(Widget::kDimGrow.asFloat(), 100),
                          SizePx(-1, 100) };
        mSublayoutDir = SublayoutDir::kOpposite;
        mSublayoutPxSizes = { SizePx(Widget::kDimGrow.asFloat(), 100),
                              SizePx(100, 100) };
        mExpectedPxSizes = { SizePx(200, 100), SizePx(100, 100) };
    }
};

class TransverseConstraintLayoutTest : public LayoutTest
{
    using Super = LayoutTest;

    class WordWrapWidget : public TestWidget
    {
    public:
        WordWrapWidget(Dir dir, float baseSize)
            : TestWidget(SizePx(0.0f, 0.0f))
            , mDir(dir), mBaseSize(baseSize)
        {
        }

        Size preferredSize(const LayoutContext& context) const
        {
            auto onePx = context.dc.onePixel();
            auto zero = PicaPt::kZero;
            if (mDir == Dir::kHoriz) {
                return Size(mBaseSize + std::max(zero, context.constraints.height - mBaseSize),
                            context.constraints.height);
            } else {
                return Size(context.constraints.width,
                            mBaseSize + std::max(zero, context.constraints.width - mBaseSize));
            }
        }

    private:
        Dir mDir;
        float mBaseSize;
    };

public:
    float kMarginPx;
    TestLayout1D *mLayout = nullptr;

    TransverseConstraintLayoutTest() : LayoutTest("layout (transverse constraint)", 0)
    {
        kMarginPx = 20;
        mSizePx = SizePx(100, 100);
        mExpectedPxSizes = { SizePx(60, 60), SizePx(40, 60) };
    }

    TestLayout1D* setupLayout(Dir dir) override
    {
        mLayout = new TestLayout1D(dir, mSizePx);
        if (dir == Dir::kHoriz) {
            mMarginsPx = { 0.0f, kMarginPx, 0.0f, kMarginPx };
        } else {
            mMarginsPx = { kMarginPx, 0.0f, kMarginPx, 0.0f };
        }
        mLayout->addChild(new WordWrapWidget(dir, 20));
        if (dir == Dir::kHoriz) {
            mLayout->addChild(new TestWidget(SizePx(Widget::kDimGrow.asFloat(), 60)));
        } else {
            mLayout->addChild(new TestWidget(SizePx(60, Widget::kDimGrow.asFloat())));
        }

        return mLayout;
    }
};

//-----------------------------------------------------------------------------
class GridTest : public TestCase
{
protected:
    struct Item
    {
        int column;
        int row;
        SizePx sizePx;
        int nItems = 1;
    };
    SizePx mSizePx;
    std::vector<Item> mItems;
    std::vector<float> mExpectedColumnWidthsPx;
    std::vector<float> mExpectedRowHeightsPx;
    int mAlign = 0;
    float mSpacingPx = 0.0f;
    std::array<float, 4> mMarginsPx = { 0.0, 0.0, 0.0, 0.0 };
    bool mExpandToWidth = true;
    bool mExpandToHeight = false;

public:
    GridTest(const std::string& name) : TestCase(name)
    {
    }

    std::string run() override
    {
        // Setup layout
        TestGridLayout *layout = new TestGridLayout(mSizePx);  // will be owned by root widget
        for (const auto &item : mItems) {
            auto *w = new TestWidget(item.sizePx);
            if (item.nItems <= 1) {
                layout->addChild(w, item.row, item.column);
            } else {
                auto *subl = new TestSublayout1D(Dir::kHoriz);
                subl->setMargins(PicaPt::kZero, PicaPt::kZero, PicaPt::kZero, PicaPt::kZero);
                subl->setSpacing(PicaPt::kZero);
                for (int i = 0;  i < item.nItems;  ++i) {
                    subl->addChild(new TestWidget(SizePx(float(item.sizePx.width) / float(item.nItems),
                                                         item.sizePx.height)));
                }
                layout->addChild(subl, item.row, item.column);
            }
        }

        Window window("UITK test", layout->frame().width, layout->frame().height);
        Widget *root = new Widget();
        // Put layout in a child, so that the window will resize the root, but we can manually size the layout
        root->addChild(layout);
        window.addChild(root);
        window.setOnWindowLayout([layout, this, root](Window &w, const LayoutContext &context) {
            // Set the root widget with some size so that layout() doesn't assert
            // (since we shouldn't be doing what we are doing, that is, resizing the
            // layout independently).
            root->setFrame(Rect(PicaPt::kZero, PicaPt::kZero, PicaPt(10.0f), PicaPt(10.0f)));

            auto pref = layout->preferredSize(context);
            layout->setFrame(Rect(PicaPt::kZero, PicaPt::kZero, pref.width, pref.height));
            auto onePx = context.dc.onePixel();
            layout->setAlignment(mAlign);
            layout->setSpacing(mSpacingPx * onePx);
            layout->setMargins(mMarginsPx[0] * onePx, mMarginsPx[1] * onePx,
                               mMarginsPx[2] * onePx, mMarginsPx[3] * onePx);
            layout->setExpandToFitWidth(mExpandToWidth);
            layout->setExpandToFitHeight(mExpandToHeight);
        });
        // The actual size of the window does not really matter (as long as it is not zero), as we will
        // set the layout's size manually above.
        window.resize(Size(PicaPt(500), PicaPt(500)));

        // Evaluate if layout succeeded
        auto &children = layout->children();
        if (children.size() != mItems.size()) {
            return "layout has incorrect number of children: got " + std::to_string(children.size()) + ", expected " + std::to_string(mItems.size()) + "\n";
        }
        std::vector<float> expectedColumnsPx(mExpectedColumnWidthsPx.size(), mMarginsPx[0]);
        for (size_t i = 1;  i < mExpectedColumnWidthsPx.size();  ++i) {
            expectedColumnsPx[i] = expectedColumnsPx[i - 1] + mSpacingPx + mExpectedColumnWidthsPx[i - 1];
        }
        std::vector<float> expectedRowsPx(mExpectedRowHeightsPx.size(), mMarginsPx[1]);
        for (size_t i = 1;  i < mExpectedRowHeightsPx.size();  ++i) {
            expectedRowsPx[i] = expectedRowsPx[i - 1] + mSpacingPx + mExpectedRowHeightsPx[i - 1];
        }
        for (size_t i = 0;  i < children.size();  ++i) {
            auto *tw = dynamic_cast<IPixelBased*>(children[i]);
            auto row = mItems[i].row;
            auto col = mItems[i].column;
            float expectedXPx = expectedColumnsPx[col];
            float expectedYPx = expectedRowsPx[row];
            if ((mAlign & Alignment::kHorizMask) == 0) {
                if (std::abs(tw->widthPx() - mExpectedColumnWidthsPx[col]) > 1e-3) {
                    return "item " + std::to_string(i) + ": got width " + std::to_string(tw->widthPx()) + ", expected " + std::to_string(mExpectedColumnWidthsPx[col]) + "\n" + layoutDescription(layout);
                }
            } else {
                if (std::abs(tw->widthPx() - mItems[i].sizePx.width) > 1e-3) {
                    return "item " + std::to_string(i) + ": got width " + std::to_string(tw->widthPx()) + ", expected " + std::to_string(mItems[i].sizePx.width) + "\n" + layoutDescription(layout);
                }
            }
            if ((mAlign & Alignment::kVertMask) == 0) {
                if (std::abs(tw->heightPx() - mExpectedRowHeightsPx[row]) > 1e-3) {
                    return "item " + std::to_string(i) + ": got height " + std::to_string(tw->heightPx()) + ", expected " + std::to_string(mExpectedRowHeightsPx[row]) + "\n" + layoutDescription(layout);
                }
            } else {
                if (std::abs(tw->heightPx() - mItems[i].sizePx.height) > 1e-3) {
                    return "item " + std::to_string(i) + ": got height " + std::to_string(tw->heightPx()) + ", expected " + std::to_string(mItems[i].sizePx.height) + "\n" + layoutDescription(layout);
                }
            }

            if (mAlign & Alignment::kHCenter) {
                expectedXPx += std::round(0.5f * (mExpectedColumnWidthsPx[row] - mItems[i].sizePx.width));
                if (std::abs(tw->xPx() - expectedXPx) > 1e-3f) {
                    return "item " + std::to_string(i) + ": aligned hcenter, got x = " + std::to_string(tw->xPx()) + ", expected " + std::to_string(expectedXPx) + "\n" + layoutDescription(layout);
                }
            } else if (mAlign & Alignment::kRight) {
                expectedXPx += std::round(mExpectedColumnWidthsPx[row] - mItems[i].sizePx.width);
                if (std::abs(tw->xPx() - expectedXPx) > 1e-3f) {
                    return "item " + std::to_string(i) + ": aligned right, got x = " + std::to_string(tw->xPx()) + ", expected " + std::to_string(expectedXPx) + "\n" + layoutDescription(layout);
                }
            } else {
                if (std::abs(tw->xPx() - expectedXPx) > 1e-3f) {
                    return "item " + std::to_string(i) + ": aligned left, got x = " + std::to_string(tw->xPx()) + ", expected " + std::to_string(expectedXPx) + "\n" + layoutDescription(layout);
                }
            }

            if (mAlign & Alignment::kVCenter) {
                expectedYPx += std::round(0.5f * (mExpectedRowHeightsPx[row] - mItems[i].sizePx.height));
                if (std::abs(tw->yPx() - expectedYPx) > 1e-3f) {
                    return "item " + std::to_string(i) + ": aligned vcenter, got y = " + std::to_string(tw->yPx()) + ", expected " + std::to_string(expectedYPx) + "\n" + layoutDescription(layout);
                }
            } else if (mAlign & Alignment::kBottom) {
                expectedYPx += std::round(mExpectedRowHeightsPx[row] - mItems[i].sizePx.height);
                if (std::abs(tw->yPx() - expectedYPx) > 1e-3f) {
                    return "item " + std::to_string(i) + ": aligned bottom, got y = " + std::to_string(tw->yPx()) + ", expected " + std::to_string(expectedYPx) + "\n" + layoutDescription(layout);
                }
            } else {
                if (std::abs(tw->yPx() - expectedYPx) > 1e-3f) {
                    return "item " + std::to_string(i) + ": aligned top, got y = " + std::to_string(tw->yPx()) + ", expected 0.0f\n" + layoutDescription(layout);
                }
            }
        }

        return "";
    }

    std::string layoutDescription(TestGridLayout *layout)
    {
        auto onePx = layout->onePx();
        std::stringstream s;
        s << "    GridLayout, spacing: "
          << mSpacingPx << "px, margins px: {" << mMarginsPx[0] << ", " << mMarginsPx[1] << ", "
          << mMarginsPx[2] << ", " << mMarginsPx[3] << "}, size: (" << layout->frame().width / onePx
          << " x " << layout->frame().height / onePx << ")" << std::endl;
        int i = 0;
        for (auto *child : layout->children()) {
            s << "        [" << i << "] " << mItems[i].column << ", " << mItems[i].row << ": (" << child->frame().x / onePx << ", " << child->frame().y / onePx
              << ") " << child->frame().width / onePx << " x " << child->frame().height / onePx << std::endl;
            i += 1;
        }

        return s.str();
    }
};

class NoItemsGridTest : public GridTest
{
public:
    NoItemsGridTest() : GridTest("grid (empty)")
    {
        mSizePx = SizePx(300, 300);
    }
};

class OneItemGridTest : public GridTest
{
public:
    OneItemGridTest() : GridTest("grid (one item)")
    {
        mSizePx = SizePx(300, 300);
        // You would not want to put your one item anywhere besides (0, 0), but this
        // tests that it actually works.
        mItems = { { 1, 1, SizePx(100, 100) } };
        mExpectedColumnWidthsPx = { 0.0f, 300.0f };
        mExpectedRowHeightsPx = { 0.0f, 100.0f };
    }
};

class OneItemNonDefaultExpandGridTest : public GridTest
{
public:
    OneItemNonDefaultExpandGridTest() : GridTest("grid (one item, no expand width, expand height)")
    {
        // This isn't really testing one item, it is testing expand to height and
        // don't expand to width.
        mSizePx = SizePx(300, 300);
        mItems = { { 1, 1, SizePx(100, 100) } };
        mExpectedColumnWidthsPx = { 0.0f, 100.0f };
        mExpectedRowHeightsPx = { 0.0f, 300.0f };
        mExpandToWidth = false;
        mExpandToHeight = true;
    }
};

class ShrinkGridTest : public GridTest
{
public:
    ShrinkGridTest() : GridTest("grid (shrink items)")
    {
        mSizePx = SizePx(300, 200);
        mItems = { { 0, 0, SizePx(300, 200) } ,
                   { 1, 0, SizePx(300, 200) },
                   { 0, 1, SizePx(300, 200) },
                   { 1, 1, SizePx(300, 200) },
        };
        mExpectedColumnWidthsPx = { 150.0f, 150.0f };
        mExpectedRowHeightsPx = { 100.0f, 100.0f };
    }
};

class FullGridTest : public GridTest
{
public:
    FullGridTest() : GridTest("grid (2x2)")
    {
        mSizePx = SizePx(300, 300);
        // Each item and each row, col have different dimensions, to detect swapping them
        // (unlikely though that is).
        mItems = { { 0, 0, SizePx(50, 125) } ,
                   { 1, 0, SizePx(200, 100) },
                   { 0, 1, SizePx(100, 50) },
                   { 1, 1, SizePx(75, 175) },
        };
        mExpectedColumnWidthsPx = { 100.0f, 200.0f };
        mExpectedRowHeightsPx = { 125.0f, 175.0f };
    }
};

class SparseGridTest : public GridTest
{
public:
    SparseGridTest() : GridTest("grid (2x2, sparse)")
    {
        mSizePx = SizePx(300, 300);
        mItems = { { 1, 0, SizePx(200, 125) },
                   { 0, 1, SizePx(100, 175) },
        };
        mExpectedColumnWidthsPx = { 100.0f, 200.0f };
        mExpectedRowHeightsPx = { 125.0f, 175.0f };
    }
};

class WithLayoutGridTest : public GridTest
{
public:
    WithLayoutGridTest() : GridTest("grid (with sublayout)")
    {
        mSizePx = SizePx(300, 300);
        mItems = { { 0, 0, SizePx(150, 300), 2 },
                   { 1, 0, SizePx(150, 300) },
        };
        mExpectedColumnWidthsPx = { 150.f, 150.f };
        mExpectedRowHeightsPx = { 300.0f };
    }
};

class MarginsGridTest : public GridTest
{
public:
    MarginsGridTest() : GridTest("grid (margins, spacing)")
    {
        mSpacingPx = 2.0f;
        mMarginsPx = { 2.0f, 4.0f, 6.0f, 8.0f };
        mSizePx = SizePx(300, 300);
        mItems = { { 0, 0, SizePx(50, 125) } ,
                   { 1, 0, SizePx(200, 100) },
                   { 0, 1, SizePx(100, 50) },
                   { 1, 1, SizePx(75, 175) },
        };
        mExpectedColumnWidthsPx = { 95.0f, 195.0f };
        mExpectedRowHeightsPx = { 118.0f, 168.0f };

    }
};

class LeftTopGridTest : public GridTest
{
public:
    LeftTopGridTest() : GridTest("grid (align left|top)")
    {
        mSizePx = SizePx(300, 300);
        mItems = { { 0, 0, SizePx(150, 150) } ,
                   { 1, 0, SizePx(100, 125) },
                   { 0, 1, SizePx(50, 75) },
                   { 1, 1, SizePx(150, 150) },
        };
        mExpectedColumnWidthsPx = { 150.0f, 150.0f };
        mExpectedRowHeightsPx = { 150.0f, 150.0f };
        mAlign = Alignment::kLeft | Alignment::kTop;
    }
};

class CenterGridTest : public GridTest
{
public:
    CenterGridTest() : GridTest("grid (align hcenter|vcenter)")
    {
        mSizePx = SizePx(300, 300);
        mItems = { { 0, 0, SizePx(150, 150) } ,
                   { 1, 0, SizePx(100, 130) },
                   { 0, 1, SizePx(50, 70) },
                   { 1, 1, SizePx(150, 150) },
        };
        mExpectedColumnWidthsPx = { 150.0f, 150.0f };
        mExpectedRowHeightsPx = { 150.0f, 150.0f };
        mAlign = Alignment::kCenter;
    }
};

class BottomRightGridTest : public GridTest
{
public:
    BottomRightGridTest() : GridTest("grid (align right|bottom)")
    {
        mSizePx = SizePx(300, 300);
        mItems = { { 0, 0, SizePx(150, 150) } ,
                   { 1, 0, SizePx(100, 125) },
                   { 0, 1, SizePx(50, 75) },
                   { 1, 1, SizePx(150, 150) },
        };
        mExpectedColumnWidthsPx = { 150.0f, 150.0f };
        mExpectedRowHeightsPx = { 150.0f, 150.0f };
        mAlign = Alignment::kRight | Alignment::kBottom;
    }
};

//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    Application app;

    std::vector<std::shared_ptr<TestCase>> tests = {
        std::make_shared<NoItemsLayoutTest>(),
        std::make_shared<OneItemLayoutTest>(),
        std::make_shared<OneGrowingItemLayoutTest>(),
        std::make_shared<ExpandItemsLayoutTest>(),
        std::make_shared<GrowItemsLayoutTest>(),
        std::make_shared<ShrinkItemsLayoutTest>(),
        std::make_shared<AlignExpandLayoutTest>(),
        std::make_shared<AlignTopLayoutTest>(),
        std::make_shared<AlignCenterLayoutTest>(),
        std::make_shared<AlignBottomLayoutTest>(),
        std::make_shared<AlignHCenterLayoutTest>(),
        std::make_shared<AlignRightLayoutTest>(),
        std::make_shared<NestedFixedLayoutTest>(),
        std::make_shared<NestedGrowLayoutTest>(),
        std::make_shared<NestedGrow2LayoutTest>(),
        std::make_shared<MarginsLayoutTest>(),
        std::make_shared<TransverseFixedLayoutTest>(),
        std::make_shared<TransverseConstraintLayoutTest>(),
        std::make_shared<NoItemsGridTest>(),
        std::make_shared<OneItemGridTest>(),
        std::make_shared<OneItemNonDefaultExpandGridTest>(),
        std::make_shared<ShrinkGridTest>(),
        std::make_shared<FullGridTest>(),
        std::make_shared<SparseGridTest>(),
        std::make_shared<WithLayoutGridTest>(),
        std::make_shared<MarginsGridTest>(),
        std::make_shared<LeftTopGridTest>(),
        std::make_shared<CenterGridTest>(),
        std::make_shared<BottomRightGridTest>(),
    };

    int nPass = 0, nFail = 0;
    for (auto t : tests) {
        if (t->runTest()) {
            nPass++;
        } else {
            nFail++;
        }
    }

    if (nFail == 0) {
        std::cout << "All tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << nFail << " test" << (nFail == 1 ? "" : "s") << " failed" << std::endl;
        return nFail;
    }
}
