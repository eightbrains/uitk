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

#include "Layout.h"

#include "UIContext.h"
#include "themes/Theme.h"

#include <algorithm>
#include <numeric>

namespace uitk {

namespace {

Size getRequestedSize(Widget *w, const LayoutContext& context)
{
    auto pref = w->preferredSize(context);
    float fixedWidthEm = w->fixedWidthEm();
    float fixedHeightEm = w->fixedHeightEm();
    if (fixedWidthEm > 0.0f) {
        pref.width = fixedWidthEm * context.theme.params().labelFont.pointSize();
    }
    if (fixedHeightEm > 0.0f) {
        pref.height = fixedHeightEm * context.theme.params().labelFont.pointSize();
    }
    return pref;
}

enum class FitMajorAxis { kNo = 0, kYes, kYesIfPositive };
std::vector<PicaPt> calcSizes(Dir dir, const PicaPt& majorAxisSize,
                              const PicaPt& onePx, const std::vector<PicaPt>& sizes,
                              const PicaPt& spacing, FitMajorAxis fit)
{
    auto totalSize = majorAxisSize - spacing * float(std::max(size_t(0), sizes.size() - size_t(1)));

    std::vector<PicaPt> outSizes = sizes;
    std::vector<bool> isStretch(sizes.size(), false);

    PicaPt totalNonStretch;
    int nStretch = 0;
    for (size_t i = 0;  i < sizes.size();  ++i) {
        if (sizes[i] < totalSize) {
            // Round the sizes so that they are on pixel boundaries
            auto px = outSizes[i] / onePx;
            outSizes[i] = std::round(px) * onePx;
            totalNonStretch += outSizes[i];
        } else {
            nStretch += 1;
            outSizes[i] = PicaPt::kZero;
            isStretch[i] = true;
        }
    }

    if (totalNonStretch <= totalSize) {
        if (fit != FitMajorAxis::kNo || nStretch > 0) {
            if (nStretch == 0 && totalNonStretch < totalSize) {
                for (size_t i = 0;  i < isStretch.size();  ++i) {
                    if (fit == FitMajorAxis::kYes || sizes[i] > PicaPt::kZero) {
                        isStretch[i] = true;
                        nStretch += 1;
                    }
                }
            }
            // Distribute extra space. We need to do this with pixels so that we
            // can get nice, crisp alignment.
            if (nStretch > 0) {
                float totalStretchPx = (totalSize - totalNonStretch) / onePx;
                float stretchPx = std::floor(totalStretchPx / float(nStretch));
                for (size_t i = 0;  i < sizes.size();  ++i) {
                    if (isStretch[i]) {
                        outSizes[i] += stretchPx * onePx;
                    }
                }
                float extraPx = totalStretchPx - stretchPx * float(nStretch);
                assert(extraPx < float(nStretch));
                if (extraPx > 0.0f) {
                    for (size_t i = 0;  extraPx > 0.0f && i < sizes.size();  ++i) {
                        if (isStretch[i]) {
                            if (extraPx >= 1.0f) {
                                outSizes[i] += onePx;
                                extraPx -= 1.0f;
                            } else {
                                outSizes[i] += std::round(extraPx) * onePx;
                                extraPx = 0.0f;
                            }
                        }
                    }
                }
            }
        }
    } else {
        int nNonStretch = int(sizes.size()) - nStretch;
        float excessPx = (totalNonStretch - totalSize) / onePx;
        float toRemovePx = excessPx / float(nNonStretch);
        for (size_t i = 0;  i < sizes.size();  ++i) {
            if (!isStretch[i]) {
                outSizes[i] -= toRemovePx * onePx;
            }
        }
        toRemovePx = excessPx - toRemovePx * float(nNonStretch);
        if (toRemovePx > 0.0f) {
            for (size_t i = 0;  i < sizes.size();  ++i) {
                if (!isStretch[i]) {
                    outSizes[i] -= onePx;
                    toRemovePx -= 1.0f;
                }
            }
        }
    }

    return outSizes;
}

LayoutContext contextWithMargins(const LayoutContext& context, Dir dir,
                                 const Size& widgetSize, const std::array<PicaPt, 4>& margins)
{
    if (dir == Dir::kHoriz) {
        auto h = std::min(context.constraints.height, widgetSize.height);
        if (h < Widget::kDimGrow) {
            return context.withHeight(h - margins[1] - margins[3]);
        } else {
            return context;
        }
    } else {
        auto w = std::min(context.constraints.width, widgetSize.width);
        if (w < Widget::kDimGrow) {
            return context.withWidth(w - margins[0] - margins[2]);
        } else {
            return context;
        }
    }
}

} // namespace

//-----------------------------------------------------------------------------
struct Layout::Stretch::Impl
{
    Dir dir;
};

Layout::Stretch::Stretch(Dir dir)
    : mImpl(new Impl())
{
    mImpl->dir = dir;
}

Layout::Stretch::~Stretch()
{
}

Size Layout::Stretch::preferredSize(const LayoutContext& context) const
{
    if (mImpl->dir == Dir::kHoriz) {
        return Size(Widget::kDimGrow, context.dc.onePixel());
    } else {
        return Size(context.dc.onePixel(), Widget::kDimGrow);
    }
}

//-----------------------------------------------------------------------------
struct Layout::SpacingEm::Impl
{
    Dir dir;
    float ems;
};

Layout::SpacingEm::SpacingEm(Dir dir, float em /*= 1.0f*/)
    : mImpl(new Impl())
{
    mImpl->dir = dir;
    mImpl->ems = em;
}

Layout::SpacingEm::~SpacingEm()
{
}

Size Layout::SpacingEm::preferredSize(const LayoutContext& context) const
{
    auto em = context.theme.params().labelFont.pointSize();
    auto size = context.dc.roundToNearestPixel(mImpl->ems * em);
    if (mImpl->dir == Dir::kHoriz) {
        return Size(size, PicaPt(1));
    } else {
        return Size(PicaPt(1), size);
    }
}

float Layout::SpacingEm::ems() const { return mImpl->ems; }

Layout::SpacingEm* Layout::SpacingEm::setEms(float ems)
{
    mImpl->ems = ems;
    setNeedsLayout();
    return this;
}
//-----------------------------------------------------------------------------
struct Layout::Impl
{
    int align = 0;
    float spacingEm = 0.0f;
    PicaPt spacing = PicaPt::kZero;
    std::array<float, 4> marginsEm = { 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<PicaPt, 4> margins = { PicaPt::kZero, PicaPt::kZero, PicaPt::kZero, PicaPt::kZero};
    bool spacingUnset = true;
    // No marginsUnset because we want the default margin to be zero (otherwise
    // nested layouts have lots of extra spacing).

    PicaPt calcSpacing(const LayoutContext& context, const PicaPt& em) const
    {
        if (spacingUnset) {
            return context.theme.calcLayoutSpacing(context.dc);  // already rounded by Theme
        } else if (this->spacingEm > 0.0f) {
            return context.dc.roundToNearestPixel(this->spacingEm * em);
        } else {
            return context.dc.roundToNearestPixel(this->spacing);
        }

    }

    std::array<PicaPt, 4> calcMargins(const LayoutContext& context, const PicaPt& em) const
    {
        if (this->marginsEm[0] != 0.0f || this->marginsEm[1] != 0.0f ||
            this->marginsEm[2] != 0.0f || this->marginsEm[3] != 0.0f) {
            return std::array<PicaPt, 4>{ context.dc.roundToNearestPixel(this->marginsEm[0] * em),
                                          context.dc.roundToNearestPixel(this->marginsEm[1] * em),
                                          context.dc.roundToNearestPixel(this->marginsEm[2] * em),
                                          context.dc.roundToNearestPixel(this->marginsEm[3] * em) };
        } else {
            return std::array<PicaPt, 4>{ context.dc.roundToNearestPixel(this->margins[0]),
                                          context.dc.roundToNearestPixel(this->margins[1]),
                                          context.dc.roundToNearestPixel(this->margins[2]),
                                          context.dc.roundToNearestPixel(this->margins[3]) };
        }
    }
};

Layout::Layout()
    : mImpl(new Impl())
{
}

int Layout::alignment() const { return mImpl->align; }

Layout* Layout::setAlignment(int alignment)
{
    mImpl->align = alignment;
    setNeedsLayout();
    setNeedsDraw();
    return this;
}

std::array<float, 4> Layout::marginsEm() const { return mImpl->marginsEm; }

Layout* Layout::setMarginsEm(float em)
{
    return setMarginsEm(em, em, em, em);
}

Layout* Layout::setMarginsEm(float leftEm, float topEm, float rightEm, float bottomEm)
{
    mImpl->marginsEm[0] = leftEm;
    mImpl->marginsEm[1] = topEm;
    mImpl->marginsEm[2] = rightEm;
    mImpl->marginsEm[3] = bottomEm;
    mImpl->margins[0] = PicaPt::kZero;
    mImpl->margins[1] = PicaPt::kZero;
    mImpl->margins[2] = PicaPt::kZero;
    mImpl->margins[3] = PicaPt::kZero;
    setNeedsLayout();
    return this;
}

std::array<PicaPt, 4> Layout::margins() const { return mImpl->margins; }

Layout* Layout::setMargins(const PicaPt& m)
{
    return setMargins(m, m, m, m);
}

Layout* Layout::setMargins(const PicaPt& left, const PicaPt& top,
                           const PicaPt& right, const PicaPt& bottom)
{
    mImpl->marginsEm[0] = 0.0f;
    mImpl->marginsEm[1] = 0.0f;
    mImpl->marginsEm[2] = 0.0f;
    mImpl->marginsEm[3] = 0.0f;
    mImpl->margins[0] = left;
    mImpl->margins[1] = top;
    mImpl->margins[2] = right;
    mImpl->margins[3] = bottom;
    setNeedsLayout();
    return this;
}

float Layout::spacingEm() const { return mImpl->spacingEm; }

Layout* Layout::setSpacingEm(float em)
{
    mImpl->spacingUnset = false;
    mImpl->spacingEm = em;
    mImpl->spacing = PicaPt::kZero;
    setNeedsLayout();
    return this;
}

const PicaPt& Layout::spacing() const { return mImpl->spacing; }

Layout* Layout::setSpacing(const PicaPt& s)
{
    mImpl->spacingUnset = false;
    mImpl->spacing = s;
    mImpl->spacingEm = 0.0f;
    setNeedsLayout();
    return this;
}

PicaPt Layout::calcSpacing(const LayoutContext& context, const PicaPt& em) const
{
    return mImpl->calcSpacing(context, em);
}

std::array<PicaPt, 4> Layout::calcMargins(const LayoutContext& context, const PicaPt& em) const
{
    return mImpl->calcMargins(context, em);
}

//-----------------------------------------------------------------------------

struct Layout1D::Impl
{
    Dir dir;
};

Layout1D::Layout1D(Dir dir)
    : mImpl(new Impl())
{
    mImpl->dir = dir;
}

Layout1D::Layout1D(Dir dir, const std::vector<Widget*>& children)
    : mImpl(new Impl())
{
    mImpl->dir = dir;
    for (auto *child : children) {
        addChild(child);
    }
}

Layout1D::~Layout1D()
{
}

Dir Layout1D::dir() const { return mImpl->dir; }

Size Layout1D::preferredSize(const LayoutContext &origContext) const
{
    auto em = origContext.theme.params().labelFont.pointSize();
    auto spacing = calcSpacing(origContext, em);
    auto margins = calcMargins(origContext, em);
    auto context = contextWithMargins(origContext, mImpl->dir, Size(kDimGrow, kDimGrow), margins);

    // dir: preferred size is max of all the elements (so if one is kDimGrow, the result is kDimGrow)
    // transverse: preferred size is the max non-grow size. (This may prove to be insufficient, in
    // which case we probably need a minimumSize() or something.)
    Size size;
    PicaPt maxFixedTransverse, maxTransverse;
    if (mImpl->dir == Dir::kHoriz) {
        for (auto *child : children()) {
            auto pref = getRequestedSize(child, context);
            size.width += pref.width;
            maxTransverse = std::max(maxTransverse, pref.height);
            if (pref.height < kDimGrow) {
                maxFixedTransverse = std::max(maxFixedTransverse, pref.height);
            }
        }
        size.width += margins[0] + margins[2] + float(children().size() - 1) * spacing;
        if (maxFixedTransverse.asFloat() > 1e-3f) {
            size.height = maxFixedTransverse;
        } else {
            size.height = maxTransverse;
        }
        size.height += margins[1] + margins[3];
    } else {
        for (auto *child : children()) {
            auto pref = getRequestedSize(child, context);
            maxTransverse = std::max(maxTransverse, pref.width);
            if (pref.width < kDimGrow) {
                maxFixedTransverse = std::max(maxFixedTransverse, pref.width);
            }
            size.height += pref.height;
        }
        if (maxFixedTransverse.asFloat() > 1e-3f) {
            size.width = maxFixedTransverse;
        } else {
            size.width = maxTransverse;
        }
        size.width += margins[0] + margins[2];
        size.height += margins[1] + margins[3] + float(children().size() - 1) * spacing;
    }

    if (borderColor().alpha() >= 0.001f) {
        auto border = context.dc.ceilToNearestPixel(borderWidth());
        size.width += 2.0f * border;
        size.height += 2.0f * border;
    }

    size.width = std::min(size.width, kDimGrow);
    size.height = std::min(size.height, kDimGrow);
    return size;
}

void Layout1D::layout(const LayoutContext& origContext)
{
    auto em = origContext.theme.params().labelFont.pointSize();
    auto spacing = calcSpacing(origContext, em);
    auto margins = calcMargins(origContext, em);
    auto context = contextWithMargins(origContext, mImpl->dir, bounds().size(), margins);

    std::vector<Size> prefs;
    std::vector<PicaPt> prefComponent;
    auto &childs = children();
    prefs.reserve(childs.size());
    prefComponent.reserve(childs.size());

    for (auto *child : childs) {
        prefs.push_back(getRequestedSize(child, context));
    }

    int halign = (alignment() & Alignment::kHorizMask);
    int valign = (alignment() & Alignment::kVertMask);
    auto border = context.dc.ceilToNearestPixel(borderWidth());
    if (borderColor().alpha() < 0.001f) {
        border = PicaPt::kZero;
    }
    auto b = bounds();
    b = Rect(margins[0] + border, margins[1] + border,
             b.width - margins[0] - margins[2] - 2.0f * border,
             b.height - margins[1] - margins[3] - 2.0f * border);
    if (mImpl->dir == Dir::kHoriz) {
        for (auto &p : prefs) {
            prefComponent.push_back(p.width);
        }
        auto sizes = calcSizes(mImpl->dir, b.width, context.dc.onePixel(), prefComponent,
                               spacing, (halign == 0 ? FitMajorAxis::kYes : FitMajorAxis::kNo));
        auto x = b.x;
        if (halign == Alignment::kHCenter) {
            auto w = std::accumulate(sizes.begin(), sizes.end(), PicaPt::kZero) + float(sizes.size() - 1) * spacing;
            x = context.dc.roundToNearestPixel(b.x + 0.5f * (b.width - w));
        } else if (halign == Alignment::kRight) {
            auto w = std::accumulate(sizes.begin(), sizes.end(), PicaPt::kZero) + float(sizes.size() - 1) * spacing;
            x = context.dc.roundToNearestPixel(b.maxX() - w);
        }
        for (size_t i = 0;  i < prefs.size();  ++i) {
            Rect r(x, b.y, sizes[i], b.height);
            if (valign == 0) {
                ;  // r is already expand, we just want to evaluate this possibility first,
                   // since it is most likely
            } else if (valign == Alignment::kVCenter) {
                r.height = prefs[i].height;
                r.y = context.dc.roundToNearestPixel(r.y + 0.5f * (b.height - r.height));
            } else if (valign == Alignment::kBottom) {
                r.height = prefs[i].height;
                r.y = b.maxY() - r.height;
            } else {
                r.height = prefs[i].height;
            }
            r.height = std::min(b.height, context.dc.roundToNearestPixel(r.height));
            childs[i]->setFrame(r);
            x = r.maxX() + spacing;
        }
    } else {
        for (auto &p : prefs) {
            prefComponent.push_back(p.height);
        }
        auto sizes = calcSizes(mImpl->dir, b.height, context.dc.onePixel(), prefComponent,
                               spacing, (valign == 0 ? FitMajorAxis::kYes : FitMajorAxis::kNo));
        auto y = b.y;
        if (valign == Alignment::kVCenter) {
            auto h = std::accumulate(sizes.begin(), sizes.end(), PicaPt::kZero) + float(sizes.size() - 1) * spacing;
            y = context.dc.roundToNearestPixel(b.y + 0.5f * (b.height - h));
        } else if (valign == Alignment::kBottom) {
            auto h = std::accumulate(sizes.begin(), sizes.end(), PicaPt::kZero) + float(sizes.size() - 1) * spacing;
            y = context.dc.roundToNearestPixel(b.maxY() - h);
        }
        for (size_t i = 0;  i < prefs.size();  ++i) {
            Rect r(b.x, y, b.width, sizes[i]);
            if (halign == 0) {
                ;  // r is already expand, we just want to evaluate this possibility first,
                   // since it is most likely
            } else if (halign == Alignment::kHCenter) {
                r.width = prefs[i].width;
                r.x = context.dc.roundToNearestPixel(r.x + 0.5f * (b.width - r.width));
            } else if (halign == Alignment::kRight) {
                r.width = prefs[i].width;
                r.x = b.maxX() - r.width;
            } else {
                r.width = prefs[i].width;
            }
            r.width = std::min(b.width, context.dc.roundToNearestPixel(r.width));
            childs[i]->setFrame(r);
            y = r.maxY() + spacing;
        }
    }

    Super::layout(context);
}

//-----------------------------------------------------------------------------
HLayout::Stretch::Stretch() : Layout::Stretch(Dir::kHoriz) {}
HLayout::Stretch::~Stretch() {}

HLayout::HLayout()
    : Layout1D(Dir::kHoriz)
{
}

HLayout::HLayout(const std::vector<Widget*>& children)
    : Layout1D(Dir::kHoriz, children)
{

}
void HLayout::addStretch() { addChild(new Layout::Stretch(Dir::kHoriz)); }

void HLayout::addSpacingEm(float em /*= 1.0f*/) { addChild(new Layout::SpacingEm(Dir::kHoriz, em)); };

//-----------------------------------------------------------------------------
VLayout::Stretch::Stretch() : Layout::Stretch(Dir::kVert) {}
VLayout::Stretch::~Stretch() {}

VLayout::VLayout()
    : Layout1D(Dir::kVert)
{
}

VLayout::VLayout(const std::vector<Widget*>& children)
    : Layout1D(Dir::kVert, children)
{
}

void VLayout::addStretch() { addChild(new Layout::Stretch(Dir::kVert)); }

void VLayout::addSpacingEm(float em /*= 1.0f*/) { addChild(new Layout::SpacingEm(Dir::kVert, em)); };

//-----------------------------------------------------------------------------
struct GridLayout::Impl
{
    std::vector<std::vector<Widget*>> rows;  // these are references; parent owns
    bool expandToWidth = true;
    bool expandToHeight = false;

    void calcPreferredRowColSize(const LayoutContext& context,
                                 std::vector<PicaPt> *colSizes, std::vector<PicaPt> *rowSizes)
    {
        auto onePx = context.dc.onePixel();

        colSizes->clear();
        rowSizes->clear();
        rowSizes->resize(rows.size(), PicaPt::kZero);
        for (size_t y = 0;  y < rows.size();  ++y) {
            auto &row = rows[y];
            colSizes->resize(std::max(colSizes->size(), row.size()),
                             PicaPt::kZero);  // only sets new elements to zero
            for (size_t x = 0;  x < row.size();  ++x) {
                Size pref;
                if (row[x]) {
                    pref = getRequestedSize(row[x], context);
                }
                (*colSizes)[x] = std::max((*colSizes)[x], pref.width);
                (*rowSizes)[y] = std::max((*rowSizes)[y], pref.height);
            }
        }

        for (auto &c : *colSizes) {
            c = context.dc.ceilToNearestPixel(c);
        }
        for (auto &r : *rowSizes) {
            r = context.dc.ceilToNearestPixel(r);
        }
    }

    std::vector<std::vector<Rect>> calcFrames(const LayoutContext& context, const Size& contentSize,
                                              const PicaPt& spacing, int alignment)
    {
        std::vector<std::vector<Rect>> frames;
        frames.reserve(this->rows.size());
        for (auto &r : this->rows) {
            frames.push_back({});
            frames.back().resize(r.size());
        }

        std::vector<PicaPt> colSizes;
        std::vector<PicaPt> rowSizes;
        this->calcPreferredRowColSize(context, &colSizes, &rowSizes);
        auto w = contentSize.width;
        if (w == PicaPt::kZero && !colSizes.empty()) {
            for (auto &cz : colSizes) {
                w += cz;
            }
            w += spacing * int(colSizes.size() - 1);
        }

        Rect rect(PicaPt::kZero, PicaPt::kZero, std::min(w, context.constraints.width), contentSize.height);
        colSizes = calcSizes(Dir::kHoriz, rect.width, context.dc.onePixel(), colSizes, spacing,
                             (this->expandToWidth ? FitMajorAxis::kYesIfPositive : FitMajorAxis::kNo));
        // Recalc rows, in case column widths change that
        std::vector<std::vector<Size>> prefsConstrained;
        prefsConstrained.reserve(this->rows.size());
        for (size_t r = 0;  r < this->rows.size();  ++r) {
            auto &row = this->rows[r];
            prefsConstrained.push_back({});
            prefsConstrained.reserve(row.size());
            auto h = PicaPt::kZero;
            for (size_t c = 0;  c < row.size();  ++c) {
                if (row[c]) {
                    auto pref = row[c]->preferredSize(context.withWidth(colSizes[c]));
                    h = std::max(h, context.dc.ceilToNearestPixel(pref.height));
                    prefsConstrained[r].push_back(pref);
                }
            }
            rowSizes[r] = std::max(rowSizes[r], h);
        }
        if (rect.height == PicaPt::kZero) {
            for (auto &cz : rowSizes) {
                rect.height += cz;
            }
        }
        rowSizes = calcSizes(Dir::kVert, rect.height, context.dc.onePixel(), rowSizes, spacing,
                             (this->expandToHeight ? FitMajorAxis::kYesIfPositive : FitMajorAxis::kNo));

        int halign = (alignment & Alignment::kHorizMask);
        int valign = (alignment & Alignment::kVertMask);

        auto y = rect.y;
        for (size_t r = 0;  r < this->rows.size();  ++r) {
            auto &row = this->rows[r];
            auto x = rect.x;
            for (size_t c = 0;  c < row.size();  ++c) {
                Rect f(x, y, colSizes[c], rowSizes[r]);
                if (row[c]) {
                    Size pref = ((halign || valign) ? getRequestedSize(row[c], context) : Size::kZero);
                    if (pref.width > colSizes[c]) {
                        pref = prefsConstrained[r][c];
                    }
                    if (halign == 0) {
                        ; // nothing to do, but be clear about that
                    } else {
                        if (pref.width < f.width) {
                            if (halign & Alignment::kLeft) {
                                f.width = context.dc.roundToNearestPixel(pref.width);
                            } else if (halign & Alignment::kHCenter) {
                                f.x = context.dc.roundToNearestPixel(f.midX() - 0.5f * pref.width);
                                f.width = context.dc.roundToNearestPixel(pref.width);
                            } else if (halign & Alignment::kRight) {
                                f.x = context.dc.roundToNearestPixel(f.maxX()) - context.dc.roundToNearestPixel(pref.width);
                                f.width = context.dc.roundToNearestPixel(pref.width);
                            }
                        }
                    }
                    if (valign == 0) {
                        ; // nothing to do, but be clear about that
                    } else {
                        if (pref.height < f.height) {
                            if (valign & Alignment::kTop) {
                                f.height = context.dc.roundToNearestPixel(pref.height);
                            } else if (valign & Alignment::kVCenter) {
                                f.y = context.dc.roundToNearestPixel(f.midY() - 0.5f * pref.height);
                                f.height = context.dc.roundToNearestPixel(pref.height);
                            } else if (valign & Alignment::kBottom) {
                                f.y = context.dc.roundToNearestPixel(f.maxY()) - context.dc.roundToNearestPixel(pref.height);
                                f.height = context.dc.roundToNearestPixel(pref.height);
                            }
                        }
                    }
                }
                frames[r][c] = f;
                x += colSizes[c] + spacing;
            }
            y += rowSizes[r] + spacing;
        }

        return frames;
    }
};

GridLayout::GridLayout()
    : mImpl(new Impl())
{
}

GridLayout::GridLayout(const std::vector<std::vector<Widget*>>& rowsOfChildren)
    : mImpl(new Impl())
{
    mImpl->rows = rowsOfChildren;
    for (auto &row : rowsOfChildren) {
        for (auto *child : row) {
            Super::addChild(child);  // NOT addChild(child, row, col)
        }
    }
}

GridLayout::~GridLayout()
{
}

bool GridLayout::expandToFitWidth() const { return mImpl->expandToWidth; }

GridLayout* GridLayout::setExpandToFitWidth(bool expand)
{
    mImpl->expandToWidth = expand;
    setNeedsLayout();
    return this;
}

bool GridLayout::expandToFitHeight() const { return mImpl->expandToHeight; }

GridLayout* GridLayout::setExpandToFitHeight(bool expand)
{
    mImpl->expandToHeight = expand;
    setNeedsLayout();
    return this;
}

void GridLayout::addChild(Widget *child, int row, int column)
{
    while (row >= mImpl->rows.size()) {
        mImpl->rows.emplace_back();
    }
    while (column >= mImpl->rows[row].size()) {
        mImpl->rows[row].push_back(nullptr);
    }
    mImpl->rows[row][column] = child;
    Super::addChild(child);
}

Size GridLayout::preferredSize(const LayoutContext &context) const
{
    auto em = context.theme.params().labelFont.pointSize();
    auto spacing = calcSpacing(context, em);
    auto margins = calcMargins(context, em);
    auto border = PicaPt::kZero;
    if (borderColor().alpha() > 1e-5f && borderWidth() > PicaPt::kZero) {
        border = context.dc.roundToNearestPixel(borderWidth());
    }

    Size pref;
    auto frames = mImpl->calcFrames(context, Size::kZero, spacing, alignment());
    if (!frames.empty() && !frames.back().empty()) {
        pref.width = frames.back().back().maxX();
        for (auto &f : frames.back()) {
            pref.height = std::max(pref.height, f.maxY());
        }
    }

    pref.width += margins[0] + margins[2] + 2.0f * border;
    pref.height += margins[1] + margins[3] + 2.0f * border;

    return pref;
}

void GridLayout::layout(const LayoutContext& context)
{
    auto em = context.theme.params().labelFont.pointSize();
    auto spacing = calcSpacing(context, em);
    auto margins = calcMargins(context, em);
    auto border = PicaPt::kZero;
    if (borderColor().alpha() > 1e-5f && borderWidth() > PicaPt::kZero) {
        border = context.dc.roundToNearestPixel(borderWidth());
    }

    int nCols = 0;
    for (size_t r = 0;  r < mImpl->rows.size();  ++r) {
        nCols = std::max(nCols, int(mImpl->rows[r].size()));
    }

    auto size = bounds().size();
    size.width -= margins[0] + margins[2] + 2.0f * border;
    size.height -= margins[1] + margins[3] + 2.0f * border;
    auto frames = mImpl->calcFrames(context.withWidth(size.width), size, spacing, alignment());
    for (size_t r = 0;  r < mImpl->rows.size();  ++r) {
        auto &row = mImpl->rows[r];
        for (size_t c = 0;  c < row.size();  ++c) {
            if (row[c]) {
                frames[r][c].translate(margins[0] + border, margins[1] + border);
                row[c]->setFrame(frames[r][c]);
            }
        }
    }

    Super::layout(context);
}

}  // namespace uitk
