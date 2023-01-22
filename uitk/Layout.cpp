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

#include <numeric>

namespace uitk {

namespace {

enum class FitMajorAxis { kNo = 0, kYes, kYesIfPositive };
std::vector<PicaPt> calcSizes(Dir dir, const PicaPt& majorAxisSize,
                              const PicaPt& onePx, const std::vector<PicaPt>& sizes,
                              PicaPt& spacing, FitMajorAxis fit)
{
    auto totalSize = majorAxisSize - spacing * float(std::max(0ul, sizes.size() - 1ul));

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

} // namespace

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
    float spacingEm = 1.0f;
    PicaPt spacing = PicaPt::kZero;
    std::array<float, 4> marginsEm = { 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<PicaPt, 4> margins = { PicaPt::kZero, PicaPt::kZero, PicaPt::kZero, PicaPt::kZero};

    PicaPt calcSpacing(const DrawContext& dc, const PicaPt& em) const
    {
        if (this->spacingEm > 0.0f) {
            return dc.roundToNearestPixel(this->spacingEm * em);
        } else {
            return dc.roundToNearestPixel(this->spacing);
        }

    }
    std::array<PicaPt, 4> calcMargins(const DrawContext& dc, const PicaPt& em) const
    {
        std::array<PicaPt, 4> actual;
        if (this->marginsEm[0] != 0.0f || this->marginsEm[1] != 0.0f ||
            this->marginsEm[2] != 0.0f || this->marginsEm[3] != 0.0f) {
            return std::array<PicaPt, 4>{ dc.roundToNearestPixel(this->marginsEm[0] * em),
                                          dc.roundToNearestPixel(this->marginsEm[1] * em),
                                          dc.roundToNearestPixel(this->marginsEm[2] * em),
                                          dc.roundToNearestPixel(this->marginsEm[3] * em) };
        } else {
            return std::array<PicaPt, 4>{ dc.roundToNearestPixel(this->margins[0]),
                                          dc.roundToNearestPixel(this->margins[1]),
                                          dc.roundToNearestPixel(this->margins[2]),
                                          dc.roundToNearestPixel(this->margins[3]) };
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
    mImpl->spacingEm = em;
    mImpl->spacing = PicaPt::kZero;
    setNeedsLayout();
    return this;
}

const PicaPt& Layout::spacing() const { return mImpl->spacing; }

Layout* Layout::setSpacing(const PicaPt& s)
{
    mImpl->spacing = s;
    mImpl->spacingEm = 0.0f;
    setNeedsLayout();
    return this;
}

PicaPt Layout::calcSpacing(const DrawContext& dc, const PicaPt& em) const
{
    return mImpl->calcSpacing(dc, em);
}

std::array<PicaPt, 4> Layout::calcMargins(const DrawContext& dc, const PicaPt& em) const
{
    return mImpl->calcMargins(dc, em);
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

Size Layout1D::preferredSize(const LayoutContext &context) const
{
    auto em = context.theme.params().labelFont.pointSize();
    auto spacing = calcSpacing(context.dc, em);
    auto margins = calcMargins(context.dc, em);

    Size size;
    if (mImpl->dir == Dir::kHoriz) {
        for (auto *child : children()) {
            auto pref = child->preferredSize(context);
            size.width += pref.width;
            size.height = std::max(size.height, pref.height);
        }
        size.width += margins[0] + margins[2] + float(children().size() - 1) * spacing;
        size.height += margins[1] + margins[3];
    } else {
        for (auto *child : children()) {
            auto pref = child->preferredSize(context);
            size.width = std::max(size.width, pref.width);
            size.height += pref.height;
        }
        size.width += margins[0] + margins[2];
        size.height += margins[1] + margins[3] + float(children().size() - 1) * spacing;
    }

    size.width = std::min(size.width, kDimGrow);
    size.height = std::min(size.height, kDimGrow);
    return size;
}

void Layout1D::layout(const LayoutContext& context)
{
    auto em = context.theme.params().labelFont.pointSize();
    auto spacing = calcSpacing(context.dc, em);
    auto margins = calcMargins(context.dc, em);

    std::vector<Size> prefs;
    std::vector<PicaPt> prefComponent;
    auto &childs = children();
    prefs.reserve(childs.size());
    prefComponent.reserve(childs.size());

    for (auto *child : childs) {
        prefs.push_back(child->preferredSize(context));
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
HLayout::HLayout()
    : Layout1D(Dir::kHoriz)
{
}

HLayout::HLayout(const std::vector<Widget*>& children)
    : Layout1D(Dir::kHoriz, children)
{

}
void HLayout::addStretch() { addChild(new Layout::Stretch()); }

void HLayout::addSpacingEm(float em /*= 1.0f*/) { addChild(new Layout::SpacingEm(Dir::kHoriz, em)); };

//-----------------------------------------------------------------------------
VLayout::VLayout()
    : Layout1D(Dir::kVert)
{
}

VLayout::VLayout(const std::vector<Widget*>& children)
    : Layout1D(Dir::kVert, children)
{
}

void VLayout::addStretch() { addChild(new Layout::Stretch()); }

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
                    pref = row[x]->preferredSize(context);
                }
                (*colSizes)[x] = std::max((*colSizes)[x], pref.width);
                (*rowSizes)[y] = std::max((*rowSizes)[y], pref.height);
            }
        }

        for (auto &c : *colSizes) {
            c = context.dc.roundToNearestPixel(c);
        }
        for (auto &r : *rowSizes) {
            r = context.dc.roundToNearestPixel(r);
        }
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
    auto spacing = calcSpacing(context.dc, em);
    auto margins = calcMargins(context.dc, em);
    auto border = PicaPt::kZero;
    if (borderColor().alpha() > 1e-5f && borderWidth() > PicaPt::kZero) {
        border = context.dc.roundToNearestPixel(borderWidth());
    }

    std::vector<PicaPt> cols;
    std::vector<PicaPt> rows;
    mImpl->calcPreferredRowColSize(context, &cols, &rows);

    Size pref = Size::kZero;
    for (auto &c : cols) {
        pref.width += c;
    }
    for (auto &r : rows) {
        pref.height += r;
    }
    pref.width += margins[0] + margins[2] + float(std::max(0ul, cols.size() - 1)) * spacing + 2.0f * border;
    pref.height += margins[1] + margins[3] + float(std::max(0ul, rows.size() - 1)) * spacing + 2.0f * border;
    return pref;
}

void GridLayout::layout(const LayoutContext& context)
{
    auto em = context.theme.params().labelFont.pointSize();
    auto spacing = calcSpacing(context.dc, em);
    auto margins = calcMargins(context.dc, em);
    auto border = PicaPt::kZero;
    if (borderColor().alpha() > 1e-5f && borderWidth() > PicaPt::kZero) {
        border = context.dc.roundToNearestPixel(borderWidth());
    }

    std::vector<PicaPt> cols;
    std::vector<PicaPt> rows;
    mImpl->calcPreferredRowColSize(context, &cols, &rows);

    auto rect = bounds();
    rect = Rect(rect.x + margins[0] + border,
                rect.y + margins[1] + border,
                rect.width - margins[0] - margins[2] - 2.0f * border,
                rect.height - margins[1] - margins[3] - 2.0f * border);
    cols = calcSizes(Dir::kHoriz, rect.width, context.dc.onePixel(), cols, spacing,
                     (mImpl->expandToWidth ? FitMajorAxis::kYesIfPositive : FitMajorAxis::kNo));
    rows = calcSizes(Dir::kVert, rect.height, context.dc.onePixel(), rows, spacing,
                     (mImpl->expandToHeight ? FitMajorAxis::kYesIfPositive : FitMajorAxis::kNo));

    int halign = (alignment() & Alignment::kHorizMask);
    int valign = (alignment() & Alignment::kVertMask);

    auto y = rect.y;
    for (size_t r = 0;  r < mImpl->rows.size();  ++r) {
        auto &row = mImpl->rows[r];
        auto x = rect.x;
        for (size_t c = 0;  c < row.size();  ++c) {
            if (row[c]) {
                Size pref = ((halign || valign) ? row[c]->preferredSize(context) : Size::kZero);
                Rect f(x, y, cols[c], rows[r]);
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
                row[c]->setFrame(f);
            }
            x += cols[c] + spacing;
        }
        y += rows[r] + spacing;
    }

    Super::layout(context);
}

}  // namespace uitk
