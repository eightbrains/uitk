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

#include "StandardIconPainter.h"

#include <nativedraw.h>

#define _USE_MATH_DEFINES  // spec doesn't include M_PI, nor does MVSC
#include <math.h>

namespace uitk {

// Techniques used:
// - Centering works for both odd and even pixel sizes if you offset in from the edges,
//   rather than out from the center.  (This is usually denoted by "inset").
// - Insetting the frame by 1/2 the stroke width guarantees that the stroke will fill
//   an integer number of pixels (if the frame boundaries are on an integer multiple,
//   and the stroke width is an integer multiple).
// - Angles tend to be 45 deg or 30/60 deg. Frequently we know how long we want one
//   of the edges to be. If you make the other edge equal, you get a nice 45 deg
//   angle. If the long edge of the triangle is twice the short edge, you get a 30/60/90
//   triangle. These angles also look good as a line. So instead of computing sines/cosines
//   most angles are constructed as a line from start to start + (side1, side2);
// - Using height to measure lengths of the design is better than the width, since the
//   height of an icon is usually more important than its width.
void StandardIconPainter::drawX(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.707f * sw, 0.707f * sw);
    // Use path instead of two lines so that drawing with alpha < 1 does not have
    // a highlight where the two lines cross.
    auto path = dc.createBezierPath();
    path->moveTo(r.upperLeft());
    path->lineTo(r.lowerRight());
    path->moveTo(r.upperRight());
    path->lineTo(r.lowerLeft());
    dc.drawPath(path, kPaintStroke);
}

void StandardIconPainter::drawXCircle(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r = strokeCircle(dc, r, sw);
    r.inset(0.707f * sw, 0.707f * sw);

    auto path = dc.createBezierPath();
    path->moveTo(r.upperLeft());
    path->lineTo(r.lowerRight());
    path->moveTo(r.upperRight());
    path->lineTo(r.lowerLeft());
    dc.drawPath(path, kPaintStroke);
}

void StandardIconPainter::drawPrevScreen(DrawContext& dc, const Size& size, const Color& fg) const
{
    drawChevronLeft(dc, size, fg);
}

void StandardIconPainter::drawNextScreen(DrawContext& dc, const Size& size, const Color& fg) const
{
    drawChevronRight(dc, size, fg);
}

void StandardIconPainter::drawTwistyClosed(DrawContext& dc, const Size& size, const Color& fg) const
{
    drawTriangleRight(dc, size, fg);
}

void StandardIconPainter::drawTwistyOpen(DrawContext& dc, const Size& size, const Color& fg) const
{
    drawTriangleDown(dc, size, fg);
}

void StandardIconPainter::drawError(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto path = dc.createBezierPath();
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.5f * sw, 0.5f * sw);
    auto c = r.center();
    auto radius = 0.5f * r.height;
    const int nSides = 8;
    const float twoPi = 2.0f * float(M_PI);
    for (int i = 0;  i < nSides;  ++i) {
        float angle = float(i) * twoPi / float(nSides) + twoPi / float(2 * nSides);
        auto x = c.x + radius * std::cos(angle);
        auto y = c.y + radius * std::sin(angle);
        if (i == 0) {
            path->moveTo(Point(x, y));
        } else {
            path->lineTo(Point(x, y));
        }
    }
    path->close();
    dc.drawPath(path, kPaintStroke);
    drawExclamationPoint(dc, r.insetted(0.25f * r.width, 0.25f * r.height), sw);
}

void StandardIconPainter::drawWarning(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.5f * sw, 0.5f * sw);
    std::vector<Point> tri = {
        Point(r.midX(), r.y),
        Point(r.x, r.maxY()),
        Point(r.maxX(), r.maxY()),
        Point(r.midX(), r.y)
    };
    dc.setStrokeJoinStyle(kJoinRound);
    dc.drawLines(tri);
    auto epRect = r.insetted(0.25f * r.width, 0.25f * r.height);
    // Start the exclamation point a little lower, otherwise it blends in
    // with the narrow top of the triangle.
    auto newY = 0.333f * r.height;
    epRect.height -= newY - epRect.y;
    epRect.y = newY;
    drawExclamationPoint(dc, epRect, sw);
}

void StandardIconPainter::drawInfo(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    strokeCircle(dc, r, sw);  // ignore the suggested rect returned, for consistency with error and warning
    drawExclamationPoint(dc, r.insetted(0.25f * r.width, 0.25f * r.height), sw);
}

void StandardIconPainter::drawHelp(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    strokeCircle(dc, r, sw);  // ignore the suggested rect returned, for consistency with error and warning
    r = r.insetted(0.25f * r.width, 0.25f * r.height);

    auto c = r.center();
    auto path = dc.createBezierPath();
    float kCtrlWeight = 0.551784f;
    auto arcRadius = 0.5f * r.height;
    auto arcSide = 0.707f * 0.5f * r.height;
    Point arcStart(c.x - arcSide, c.y - arcSide);
    Point arcEnd(c.x + arcSide, c.y - arcSide);
    path->moveTo(arcStart);
    path->cubicTo(arcStart + kCtrlWeight * Point(arcSide, -arcSide),
                  arcEnd + kCtrlWeight * Point(-arcSide, -arcSide),
                  arcEnd);
    auto unit = 0.333f * arcRadius;
    auto e = c /*+ Point(unit, -unit)*/;
    auto tan = 0.2f * Point(0.707f * unit, -0.707f * unit);
    path->cubicTo(arcEnd + kCtrlWeight * Point(arcSide, arcSide), e + tan, e);
    e = Point(c.x, r.y + 0.7f * r.height);
    path->cubicTo(e - tan, e - Point(PicaPt::kZero, 0.25f * unit), e);
    dc.drawPath(path, kPaintStroke);

    auto radius = 0.65f * sw;
    dc.drawEllipse(Rect(r.midX() - radius, r.maxY() - radius, 2.0f * radius, 2.0f * radius), kPaintFill);
}

void StandardIconPainter::drawSearch(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    drawMagnifyingGlass(dc, r, sw);
}

void StandardIconPainter::drawHistory(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto rect = calcContentRect(size);
    strokeCircle(dc, rect, sw);  // keep rect as the large rect, the hands make more sense with r = h/2
    auto c = rect.center();
    auto r = 0.6f * 0.5f * rect.height;
    std::vector<Point> minute = { c, Point(c.x, c.y - r) };
    std::vector<Point> hour = { c, Point(c.x + 0.707f * r, c.y + 0.707f * r) };
    dc.drawLines(minute);
    dc.drawLines(hour);
}

void StandardIconPainter::drawMenu(DrawContext& dc, const Size& size, const Color& fg) const
{
    setStroke(dc, size, fg);
    auto r = calcContentRect(size);

    const auto onePx = dc.onePixel();
    const float dyFactor = 1.25f;
    auto maxStroke = (0.8f * r.height) / (2.0f * dyFactor + 1.0f); // +1: 1/2 a stroke on top and bottom
    maxStroke = std::max(onePx, dc.floorToNearestPixel(maxStroke));
    auto sw = std::min(maxStroke, dc.roundToNearestPixel(0.15f * r.height));
    if (r.height > PicaPt(72 / 4)) {
        sw = std::min(maxStroke, dc.roundToNearestPixel(0.1f * r.height));
    }
    auto dy = dc.roundToNearestPixel(0.333f * r.height);
    if (dy == sw) {
        sw -= onePx;
        if (sw == PicaPt::kZero) {
            sw = onePx;
            dy += onePx;
        }
    }
    dc.setStrokeWidth(sw);
    dc.setStrokeEndCap(kEndCapButt);

    auto c = r.center();
    c.y = dc.offsetPixelForStroke(dc.roundToNearestPixel(c.y), sw);
    auto x2 = r.y + 2.0f * sw;
    auto line2len = r.width - x2;
    dc.drawLines({ Point(r.x, c.y - dy), Point(r.x + sw, c.y - dy) });
    dc.drawLines({ Point(x2, c.y - dy), Point(r.maxX(), c.y - dy) });
    dc.drawLines({ Point(r.x, c.y), Point(r.x + sw, c.y) });
    dc.drawLines({ Point(x2, c.y), Point(r.y + 0.9f * r.width, c.y) });
    dc.drawLines({ Point(r.x, c.y + dy), Point(r.x + sw, c.y + dy) });
    dc.drawLines({ Point(x2, c.y + dy), Point(r.maxX(), c.y + dy) });
}

void StandardIconPainter::drawAdd(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    drawPlusOrMinus(dc, r, sw, kPlus);
}

void StandardIconPainter::drawRemove(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    drawPlusOrMinus(dc, r, sw, kMinus);
}

void StandardIconPainter::drawAddCircle(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r = strokeCircle(dc, r, sw);
    drawPlusOrMinus(dc, r, sw, kPlus);
}

void StandardIconPainter::drawRemoveCircle(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r = strokeCircle(dc, r, sw);
    drawPlusOrMinus(dc, r, sw, kMinus);
}

void StandardIconPainter::drawExpand(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    auto inset = dc.ceilToNearestPixel(0.1f * r.width);
    r = r.insetted(inset, inset);
    drawChevron(dc, r, sw, 90.0f);
}

void StandardIconPainter::drawContract(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    auto inset = dc.ceilToNearestPixel(0.1f * r.width);
    r = r.insetted(inset, inset);
    drawChevron(dc, r, sw, -90.0f);
}

void StandardIconPainter::drawMoreHoriz(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto rect = calcContentRect(size);
    auto r = rect.width / 10.0f;  // 3 circles, with 1 circle spacing between = 10 radii
    auto c = rect.center();
    dc.setFillColor(fg);
    dc.drawEllipse(Rect(rect.x, c.y - r, 2.0f * r, 2.0f * r), kPaintFill);
    dc.drawEllipse(Rect(c.x - r, c.y - r, 2.0f * r, 2.0f * r), kPaintFill);
    dc.drawEllipse(Rect(rect.maxX() - 2.0f * r, c.y - r, 2.0f * r, 2.0f * r), kPaintFill);
}

void StandardIconPainter::drawMoreVert(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto rect = calcContentRect(size);
    auto r = rect.width / 10.0f;  // 3 circles, with 1 circle spacing between = 10 radii
    auto c = rect.center();
    dc.setFillColor(fg);
    dc.drawEllipse(Rect(c.x - r, rect.y, 2.0f * r, 2.0f * r), kPaintFill);
    dc.drawEllipse(Rect(c.x - r, c.y - r, 2.0f * r, 2.0f * r), kPaintFill);
    dc.drawEllipse(Rect(c.x - r, rect.maxY() - 2.0f * r, 2.0f * r, 2.0f * r), kPaintFill);
}

void StandardIconPainter::drawLocked(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    dc.setFillColor(fg);
    auto r = calcContentRect(size);
    drawLock(dc, r, sw, kLocked);
}

void StandardIconPainter::drawUnlocked(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    dc.setFillColor(fg);
    auto r = calcContentRect(size);
    drawLock(dc, r, sw, kUnlocked);
}

void StandardIconPainter::drawSettings(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    dc.setFillColor(fg);
    auto r = calcContentRect(size);

    auto addCCWCircle = [](BezierPath& path, const Point& c, const PicaPt& r) {
        path.moveTo(Point(c.x, c.y - r));
        path.quarterEllipseTo(Point(c.x - r, c.y - r), Point(c.x - r, c.y));
        path.quarterEllipseTo(Point(c.x - r, c.y + r), Point(c.x, c.y + r));
        path.quarterEllipseTo(Point(c.x + r, c.y + r), Point(c.x + r, c.y));
        path.quarterEllipseTo(Point(c.x + r, c.y - r), Point(c.x, c.y - r));
        path.close();
    };
    auto addCircle = [](BezierPath& path, const Point& c, const PicaPt& r) {
        path.moveTo(Point(c.x, c.y - r));
        path.quarterEllipseTo(Point(c.x + r, c.y - r), Point(c.x + r, c.y));
        path.quarterEllipseTo(Point(c.x + r, c.y + r), Point(c.x, c.y + r));
        path.quarterEllipseTo(Point(c.x - r, c.y + r), Point(c.x - r, c.y));
        path.quarterEllipseTo(Point(c.x - r, c.y - r), Point(c.x, c.y - r));
        path.close();
    };

    int n = 7;
    auto gearWidth = 0.6f;  // range in [0, 1]
    float angle = 2.0f * float(M_PI) / float(n);
    float bezierWeight = (4.0f / 3.0f) * std::tan(0.25f * (0.5f * angle));
    auto outerRadius = 0.5f * r.height;
    auto gearRadius = 0.8f * outerRadius;
    auto midRadius = gearRadius - (1.0f + gearWidth) * sw;
    auto innerRadius = 0.15f * outerRadius;

    auto center = r.center();
    auto dOuterCP = outerRadius * bezierWeight;
    auto dGearCP = gearRadius * bezierWeight;
    auto theta0 = 0.0f;
    auto theta1 = gearWidth * angle;
    auto theta2 = angle;
    auto x0 = center.x + outerRadius * std::cos(theta0);
    auto y0 = center.y + outerRadius * std::sin(theta0);
    auto x1 = center.x + outerRadius * std::cos(theta1);
    auto y1 = center.y + outerRadius * std::sin(theta1);
    auto x2 = center.x + gearRadius * std::cos(theta1);
    auto y2 = center.y + gearRadius * std::sin(theta1);
    auto x3 = center.x + gearRadius * std::cos(theta2);
    auto y3 = center.y + gearRadius * std::sin(theta2);
    auto x4 = center.x + outerRadius * std::cos(theta2);
    auto y4 = center.y + outerRadius * std::sin(theta2);
    float ninetyDeg = 0.5f * float(M_PI);
    std::vector<Point> rawPoints = {
        Point(x0, y0) + Point(dOuterCP * std::cos(theta0 + ninetyDeg), dOuterCP * std::sin(theta0 + ninetyDeg)),
        Point(x1, y1) - Point(dOuterCP * std::cos(theta1 + ninetyDeg), dOuterCP * std::sin(theta1 + ninetyDeg)),
        Point(x1, y1),
        Point(x2, y2),
        Point(x2, y2) + Point(dGearCP * std::cos(theta1 + ninetyDeg), dGearCP * std::sin(theta1 + ninetyDeg)),
        Point(x3, y3) - Point(dGearCP * std::cos(theta2 + ninetyDeg), dGearCP * std::sin(theta2 + ninetyDeg)),
        Point(x3, y3),
        Point(x4, y4)
    };

    auto path = dc.createBezierPath();
    for (int i = 0;  i < n;  ++i) {
        std::vector<Point> pts = rawPoints;
        rotatePoints(&pts, center, -float(i) * angle * 180.0f / float(M_PI));
        if (i == 0) {
            path->moveTo(Point(x0, y0));
        }
        path->cubicTo(pts[0], pts[1], pts[2]);
        path->lineTo(pts[3]);
        path->cubicTo(pts[4], pts[5], pts[6]);
        if (i == n - 1) {
            path->close();
        } else {
            path->lineTo(pts.back());
        }
    }
    addCCWCircle(*path, center, midRadius);
    addCircle(*path, center, innerRadius);

    dc.drawPath(path, kPaintFill);
}

void StandardIconPainter::drawChevronLeft(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    drawChevron(dc, r, sw, 0.0f);
}

void StandardIconPainter::drawChevronRight(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    drawChevron(dc, r, sw, 180.0f);
}

void StandardIconPainter::drawChevronUp(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    drawChevron(dc, r, sw, -90.0f);
}

void StandardIconPainter::drawChevronDown(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    drawChevron(dc, r, sw, 90.0f);
}

void StandardIconPainter::drawChevronLeftCircle(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r = strokeCircle(dc, r, sw);
    drawChevron(dc, r, sw, 0.0f);
}

void StandardIconPainter::drawChevronRightCircle(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r = strokeCircle(dc, r, sw);
    drawChevron(dc, r, sw, 180.0f);
}

void StandardIconPainter::drawChevronUpCircle(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r = strokeCircle(dc, r, sw);
    drawChevron(dc, r, sw, -90.0f);
}

void StandardIconPainter::drawChevronDownCircle(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r = strokeCircle(dc, r, sw);
    drawChevron(dc, r, sw, 90.0f);
}

void StandardIconPainter::drawTriangleLeft(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    drawTriangle(dc, r, 0.0f);
}

void StandardIconPainter::drawTriangleRight(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    drawTriangle(dc, r, 180.0f);
}

void StandardIconPainter::drawTriangleUp(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    drawTriangle(dc, r, -90.0f);
}

void StandardIconPainter::drawTriangleDown(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    drawTriangle(dc, r, 90.0f);
}

void StandardIconPainter::drawTriangleLeftCircle(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r = strokeCircle(dc, r, sw);
    drawTriangle(dc, r, 0.0f);
}

void StandardIconPainter::drawTriangleRightCircle(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r = strokeCircle(dc, r, sw);
    drawTriangle(dc, r, 180.0f);
}

void StandardIconPainter::drawTriangleUpCircle(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r = strokeCircle(dc, r, sw);
    drawTriangle(dc, r, -90.0f);
}

void StandardIconPainter::drawTriangleDownCircle(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r = strokeCircle(dc, r, sw);
    drawTriangle(dc, r, 90.0f);
}

void StandardIconPainter::drawRefresh(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto rect = calcContentRect(size);
    rect.inset(0.5f * sw, 0.5f * sw);

    auto arrowLen = 0.3f * rect.height;
    auto r = 0.5f * rect.height;
    auto arcWeight = 0.3572656f;  // 4/3 * tan(60deg / 4)
    auto dx = 0.866f * r;
    auto dy = 0.5f * r;

    auto path = dc.createBezierPath();
    path->moveTo(Point(rect.maxX(), rect.midY()));
    path->quarterEllipseTo(rect.lowerRight(), Point(rect.midX(), rect.maxY()));
    path->quarterEllipseTo(rect.lowerLeft(), Point(rect.x, rect.midY()));
    path->quarterEllipseTo(rect.upperLeft(), Point(rect.midX(), rect.y));
    // x ought to be midX() + dx, but it looks better to have the arrow
    // aligned with the edge. For small icons this looks okay, although
    // larger icons look a little uncircular in this section if you look closely.
    // Firefox fudges this with a solid arrow, but that looks odd compared
    // to everything else.
    auto endpt = Point(rect.maxX(), rect.midY() - dy);
    path->cubicTo(Point(rect.midX() + r * arcWeight, rect.y),
                  endpt + Point(-dy * arcWeight, -dx * arcWeight),  // need to swap dx/dy here
                  endpt);

    path->moveTo(endpt + Point(PicaPt::kZero, -arrowLen));
    path->lineTo(endpt);
    path->lineTo(endpt + Point(-arrowLen, PicaPt::kZero));

    dc.drawPath(path, kPaintStroke);
}

void StandardIconPainter::drawArrowLeft(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    drawArrow(dc, r, 0.0f, sw);
}

void StandardIconPainter::drawArrowRight(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    drawArrow(dc, r, 180.0f, sw);
}

void StandardIconPainter::drawArrowUp(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    drawArrow(dc, r, -90.0f, sw);
}

void StandardIconPainter::drawArrowDown(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    drawArrow(dc, r, 90.0f, sw);
}

void StandardIconPainter::drawArrowLeftCircle(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r = strokeCircle(dc, r, sw);
    drawArrow(dc, r, 0.0f, sw);
}

void StandardIconPainter::drawArrowRightCircle(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r = strokeCircle(dc, r, sw);
    drawArrow(dc, r, 180.0f, sw);
}

void StandardIconPainter::drawArrowUpCircle(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r = strokeCircle(dc, r, sw);
    drawArrow(dc, r, -90.0f, sw);
}

void StandardIconPainter::drawArrowDownCircle(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r = strokeCircle(dc, r, sw);
    drawArrow(dc, r, 90.0f, sw);
}

void StandardIconPainter::drawMacCmd(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    dc.setStrokeEndCap(kEndCapButt);
    auto r = calcContentRect(size);
    r.inset(0.5f * sw, 0.5f * sw);  // so edge of circle doesn't go outside rect
    auto inset = dc.offsetPixelForStroke(dc.roundToNearestPixel(0.333f * r.width), sw);
    // In order for the circle to reach out to the edge of the rect this must be exactly
    // half the inset value. If this is fractional it does not matter, because we are
    // not drawing straight lines using this value.
    auto endInset = 0.5f * inset;
    auto radius = inset - endInset;  // == endInset, but this makes it clearer what we are doing

    // A self-crossing path with a non-opaque stroke does not have
    // highlights where the segments cross over, unlike having separate lines.
    auto path = dc.createBezierPath();
    path->moveTo(Point(r.x + endInset, r.y + inset));
    path->quarterEllipseTo(Point(r.x + endInset - radius, r.y + inset),
                           Point(r.x + endInset - radius, r.y + inset - radius));
    path->quarterEllipseTo(Point(r.x + endInset - radius, r.y + inset - 2.0f * radius),
                           Point(r.x + endInset, r.y + inset - 2.0f * radius));
    path->quarterEllipseTo(Point(r.x + endInset + radius, r.y + inset - 2.0f * radius),
                           Point(r.x + endInset + radius, r.y + inset - radius));
    path->lineTo(Point(r.x + inset, r.maxY() - endInset));
    path->quarterEllipseTo(Point(r.x + inset, r.maxY() - endInset + radius),
                           Point(r.x + inset - radius, r.maxY() - endInset + radius));
    path->quarterEllipseTo(Point(r.x + inset - 2.0f * radius, r.maxY() - endInset + radius),
                           Point(r.x + inset - 2.0f * radius, r.maxY() - endInset));
    path->quarterEllipseTo(Point(r.x + inset - 2.0f * radius, r.maxY() - endInset - radius),
                           Point(r.x + inset - radius, r.maxY() - endInset - radius));
    path->lineTo(Point(r.maxX() - endInset, r.maxY() - inset));
    path->quarterEllipseTo(Point(r.maxX() - endInset + radius, r.maxY() - inset),
                           Point(r.maxX() - endInset + radius, r.maxY() - inset + radius));
    path->quarterEllipseTo(Point(r.maxX() - endInset + radius, r.maxY() - inset + 2.0f * radius),
                           Point(r.maxX() - endInset, r.maxY() - inset + 2.0f * radius));
    path->quarterEllipseTo(Point(r.maxX() - endInset - radius, r.maxY() - inset + 2.0f * radius),
                           Point(r.maxX() - endInset - radius, r.maxY() - inset + radius));
    path->lineTo(Point(r.maxX() - inset, r.y + endInset));
    path->quarterEllipseTo(Point(r.maxX() - inset, r.y + endInset - radius),
                           Point(r.maxX() - inset + radius, r.y + endInset - radius));
    path->quarterEllipseTo(Point(r.maxX() - inset + 2.0f * radius, r.y + endInset - radius),
                           Point(r.maxX() - inset + 2.0f * radius, r.y + endInset));
    path->quarterEllipseTo(Point(r.maxX() - inset + 2.0f * radius, r.y + endInset + radius),
                           Point(r.maxX() - inset + radius, r.y + endInset + radius));
    path->close();

    dc.drawPath(path, kPaintStroke);
}

void StandardIconPainter::drawMacShift(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    dc.setStrokeJoinStyle(kJoinMiter);
    auto r = calcContentRect(size);
    auto c = r.center();
    auto inset = dc.offsetPixelForStroke(dc.roundToNearestPixel(0.25f * r.width), sw);

    std::vector<Point> pts = {
        Point(c.x, r.y - 0.5f * sw),
        Point(r.maxX(), r.y + 0.5f * r.width),
        Point(r.maxX() - inset, r.y + 0.5f * r.width),
        Point(r.maxX() - inset, r.maxY() - 0.5f * sw),
        Point(r.x + inset, r.maxY() - 0.5f * sw),
        Point(r.x + inset, r.y + 0.5f * r.width),
        Point(r.x, r.y + 0.5f * r.width),
        Point(c.x, r.y - 0.5f * sw),
    };
    dc.drawLines(pts);
}

void StandardIconPainter::drawMacOption(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    dc.setStrokeJoinStyle(kJoinMiter);
    dc.setStrokeEndCap(kEndCapButt);
    auto r = calcContentRect(size);
    auto inset = dc.roundToNearestPixel(0.333f * r.width);
    dc.drawLines({ Point(r.x, r.y + 0.5f * sw),
                   Point(r.x + inset, r.y + 0.5f * sw),
                   Point(r.maxX() - inset, r.maxY() - 0.5f * sw),
                   Point(r.maxX(), r.maxY() - 0.5f * sw) });
    dc.drawLines({ Point(r.maxX() - inset, r.y + 0.5f * sw),
                   Point(r.maxX(), r.y + 0.5f * sw) });
}

void StandardIconPainter::drawNewFile(DrawContext& dc, const Size& size, const Color& fg) const
{
    drawFile(dc, size, fg);
}

void StandardIconPainter::drawOpenFile(DrawContext& dc, const Size& size, const Color& fg) const
{
    drawFolder(dc, size, fg);
}

void StandardIconPainter::drawSaveFile(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.5f * sw, 0.5f * sw);
    auto borderRadius = calcBorderRadius(r);
    auto cornerLen = 0.2f * r.width;
    auto topRectInset = dc.roundToNearestPixel(cornerLen + 0.5 * sw);
    auto topRectBottom = dc.roundToNearestPixel(0.25f * r.width);
    auto bottomRectInset = dc.roundToNearestPixel(0.2f * r.width);
    auto bottomRectTop = dc.roundToNearestPixel(0.4f * r.width);

    auto path = dc.createBezierPath();
    path->moveTo(Point(r.maxX() - cornerLen, r.y));
    path->lineTo(Point(r.maxX(), r.y + cornerLen));
    path->lineTo(Point(r.maxX(), r.maxY() - borderRadius));
    path->quarterEllipseTo(r.lowerRight(), Point(r.maxX() - borderRadius, r.maxY()));
    path->lineTo(Point(r.x + borderRadius, r.maxY()));
    path->quarterEllipseTo(r.lowerLeft(), Point(r.x, r.maxY() - borderRadius));
    path->lineTo(Point(r.x, r.y + borderRadius));
    path->quarterEllipseTo(r.upperLeft(), Point(r.x + borderRadius, r.y));
    path->close();
    dc.drawPath(path, kPaintStroke);

    path = dc.createBezierPath();
    path->moveTo(Point(r.x + topRectInset, r.y + 0.5f * sw));
    path->lineTo(Point(r.x + topRectInset, r.y + topRectBottom - borderRadius));
    path->quarterEllipseTo(Point(r.x + topRectInset, r.y + topRectBottom),
                           Point(r.x + topRectInset + borderRadius, r.y + topRectBottom));
    path->lineTo(Point(r.maxX() - topRectInset - borderRadius, r.y + topRectBottom));
    path->quarterEllipseTo(Point(r.maxX() - topRectInset, r.y + topRectBottom),
                           Point(r.maxX() - topRectInset, r.y + topRectBottom - borderRadius));
    path->lineTo(Point(r.maxX() - topRectInset, r.y + 0.5f * sw));
    dc.setStrokeEndCap(kEndCapButt);
    dc.drawPath(path, kPaintStroke);

    path = dc.createBezierPath();
    path->moveTo(Point(r.x + bottomRectInset, r.maxY() - 0.5f * sw));
    path->lineTo(Point(r.x + bottomRectInset, r.maxY() - bottomRectTop + borderRadius));
    path->quarterEllipseTo(Point(r.x + bottomRectInset, r.maxY() - bottomRectTop),
                           Point(r.x + bottomRectInset + borderRadius, r.maxY() - bottomRectTop));
    path->lineTo(Point(r.maxX() - bottomRectInset - borderRadius, r.maxY() - bottomRectTop));
    path->quarterEllipseTo(Point(r.maxX() - bottomRectInset, r.maxY() - bottomRectTop),
                           Point(r.maxX() - bottomRectInset, r.maxY() - bottomRectTop + borderRadius));
    path->lineTo(Point(r.maxX() - bottomRectInset, r.maxY() - 0.5f * sw));
    dc.drawPath(path, kPaintStroke);
}

void StandardIconPainter::drawPrint(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.5 * sw, 0.5 * sw);

    auto topInset = dc.roundToNearestPixel(0.25f * r.height);
    auto bottomInset = dc.roundToNearestPixel(0.2f * r.height);
    auto borderRadius = calcBorderRadius(r);
//    auto separation = dc.roundToNearestPixel(0.05f * r.width);
    auto separation = PicaPt::kZero;
    auto paperOutInset = dc.roundToNearestPixel(0.2f * r.width);
    auto paperOutTopInset = dc.roundToNearestPixel(0.333f * r.height);  // from bottom

    auto path = dc.createBezierPath();
    path->moveTo(Point(r.x + paperOutInset + separation - 2.0f * sw, r.maxY() - bottomInset));
    path->lineTo(Point(r.x + borderRadius, r.maxY() - bottomInset));
    path->quarterEllipseTo(Point(r.x, r.maxY() - bottomInset),
                           Point(r.x, r.maxY() - bottomInset - borderRadius));
    path->lineTo(Point(r.x, r.y + topInset + borderRadius));
    path->quarterEllipseTo(Point(r.x, r.y + topInset), Point(r.x + borderRadius, r.y + topInset));
    path->lineTo(Point(r.maxX() - borderRadius, r.y + topInset));
    path->quarterEllipseTo(Point(r.maxX(), r.y + topInset), Point(r.maxX(), r.y + topInset + borderRadius));
    path->lineTo(Point(r.maxX(), r.maxY() - bottomInset - borderRadius));
    path->quarterEllipseTo(Point(r.maxX(), r.maxY() - bottomInset),
                           Point(r.maxX() - borderRadius, r.maxY() - bottomInset));
    path->lineTo(Point(r.maxX() - paperOutInset - separation + 2.0f * sw, r.maxY() - bottomInset));
    dc.drawPath(path, kPaintStroke);

    auto onLightInset = dc.roundToNearestPixel(0.45f * r.height);
    dc.drawLines({ Point(r.x + paperOutInset, r.y + onLightInset),
                   Point(r.x + paperOutInset + sw, r.y + onLightInset) });

    // paper out
    auto overhang = dc.roundToNearestPixel(0.05f * r.width);
    if (paperOutInset < 3.0f * overhang) {
        overhang = PicaPt::kZero;
    }
    dc.drawLines({ Point(r.x + paperOutInset - overhang, r.maxY() - paperOutTopInset),
                   Point(r.maxX() - paperOutInset + overhang, r.maxY() - paperOutTopInset) });

    dc.setStrokeEndCap(kEndCapButt);

    dc.drawLines({ Point(r.x + paperOutInset, r.maxY() - paperOutTopInset + 0.5f * sw),
                   Point(r.x + paperOutInset, r.maxY()),
                   Point(r.maxX() - paperOutInset, r.maxY()),
                   Point(r.maxX() - paperOutInset, r.maxY() - paperOutTopInset + 0.5f * sw) });

    // paper in on top
    dc.drawLines({ Point(r.x + paperOutInset, r.y + topInset - sw),
                   Point(r.x + paperOutInset, r.y),
                   Point(r.maxX() - paperOutInset, r.y),
                   Point(r.maxX() - paperOutInset, r.y + topInset - sw)
    });
}

void StandardIconPainter::drawExport(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);

    auto onePx = dc.onePixel();
    int nStrokePx = int(std::round(sw / onePx));
    int widthPx = int(std::round(r.width / onePx));
    if (nStrokePx & 0x1) {
        if ((widthPx & 0x1) == 0) {
            r.width -= onePx;
        }
    } else {
        if (widthPx & 0x1) {
            r.width -= onePx;
        }
    }
    auto cx = r.midX();

    r.inset(0.5 * sw, 0.5 * sw);

    auto arrowLen = dc.roundToNearestPixel(0.2f * r.width);
    auto midSideInset = std::min(dc.roundToNearestPixel(0.333f * r.width), // ideal
                                 (cx - r.x) - 1.5f * sw);                  // very small icons
    auto sideInset = std::min(dc.roundToNearestPixel(0.2f * r.width),  // ideal
                              std::max(PicaPt::kZero, 0.5f * r.width - arrowLen - sw));
    auto topInset = std::max(dc.roundToNearestPixel(0.4f * r.width), // ideal
                             arrowLen + 2.0f * onePx);               // very small icons
    auto arrowBottomInset = std::max(2.0f * dc.onePixel(), dc.roundToNearestPixel(0.15f * r.width));

    dc.drawLines({ Point(r.x + midSideInset, r.y + topInset),
                   Point(r.x + sideInset, r.y + topInset),
                   Point(r.x + sideInset, r.maxY()),
                   Point(r.maxX() - sideInset, r.maxY()),
                   Point(r.maxX() - sideInset, r.y + topInset),
                   Point(r.maxX() - midSideInset, r.y + topInset),
    });

    // Draw the arrow ourselves (instead of calling drawArrow with a rotation) to
    // ensure it is precisely centered).
    auto top = r.y + 0.25f * sw;
    auto path = dc.createBezierPath();
    path->moveTo(Point(cx, top));
    path->lineTo(Point(cx, r.maxY() - arrowBottomInset));
    path->moveTo(Point(cx - arrowLen, top + arrowLen));
    path->lineTo(Point(cx, top));
    path->lineTo(Point(cx + arrowLen, top + arrowLen));
    dc.drawPath(path, kPaintStroke);
}

void StandardIconPainter::drawExternal(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.5f * sw, 0.5f * sw);

    auto arrowLen = 0.25f * r.height;
    auto gapLen = 0.5f * r.height;
    auto borderRadius = calcBorderRadius(r);

    auto path = dc.createBezierPath();
    path->moveTo(Point(r.maxX() - gapLen, r.y));
    path->lineTo(Point(r.x + borderRadius, r.y));
    path->quarterEllipseTo(r.upperLeft(), Point(r.x, r.y + borderRadius));
    path->lineTo(Point(r.x, r.maxY() - borderRadius));
    path->quarterEllipseTo(r.lowerLeft(), Point(r.x + borderRadius, r.maxY()));
    path->lineTo(Point(r.maxX() - borderRadius, r.maxY()));
    path->quarterEllipseTo(r.lowerRight(), Point(r.maxX(), r.maxY() - borderRadius));
    path->lineTo(Point(r.maxX(), r.y + gapLen));

    path->moveTo(r.center());
    path->lineTo(r.upperRight());
    path->moveTo(Point(r.maxX() - arrowLen, r.y));
    path->lineTo(r.upperRight());
    path->lineTo(Point(r.maxX(), r.y + arrowLen));

    dc.drawPath(path, kPaintStroke);
}

void StandardIconPainter::drawBoldStyle(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);

    // Design note: why not draw text? Because we want the character to fill
    // the height, and the point size of a font has no relationship with its pixel height.
    // Also, this way everything is consistent. across platforms.
    auto aspectRatio = 0.75f;
    auto thickness = dc.roundToNearestPixel(0.1333f * r.height);
    auto bottomWidth = aspectRatio * r.height;
    auto topWidth = 0.925f * bottomWidth;
    auto topHeight = 0.5f * r.height + 0.25f * thickness;

    // It's easier to draw to half-B's that are overlapping than to use
    // one bezier curve.
    auto addHalfB = [](std::shared_ptr<BezierPath> path, const Rect& r, const PicaPt& thickness) {
        auto outerRadius = 0.5f * r.height;
        auto innerRadius = outerRadius - thickness;

        // Draw from top left, down, and around counter-clockwise
        path->moveTo(Point(r.x, r.y));
        path->lineTo(Point(r.x, r.maxY()));
        path->lineTo(Point(r.maxX() - outerRadius, r.maxY()));
        path->quarterEllipseTo(Point(r.maxX(), r.maxY()),
                               Point(r.maxX(), r.maxY() - outerRadius));
        path->quarterEllipseTo(Point(r.maxX(), r.y),
                               Point(r.maxX() - outerRadius, r.y));
        path->close();

        // Remove the top of the B, so draw in the other direction
        // (bottom left, up, and around clockwise).
        auto top = r.y + thickness;
        auto bottom = r.maxY() - thickness;
        path->moveTo(Point(r.x + thickness, bottom));
        path->lineTo(Point(r.x + thickness, top));
        path->lineTo(Point(r.maxX() - thickness - innerRadius, top));
        path->quarterEllipseTo(Point(r.maxX() - thickness, top),
                               Point(r.maxX() - thickness, top + innerRadius));
        path->quarterEllipseTo(Point(r.maxX() - thickness, bottom),
                               Point(r.maxX() - thickness - innerRadius, bottom));
        path->close();
    };
    auto path = dc.createBezierPath();
    addHalfB(path, Rect(r.x, r.y, topWidth, topHeight), thickness);
    auto y = r.y + topHeight - thickness;
    addHalfB(path, Rect(r.x, y, bottomWidth, r.height - y), thickness);
    dc.drawPath(path, kPaintFill);
}

void StandardIconPainter::drawItalicStyle(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);

    // Design note: why not draw text? Because we want the character to fill
    // the height, and the point size of a font has no relationship with its pixel height.
    // Also, this way everything is consistent. across platforms.
    auto angle = 10.0f /* deg */ * float(M_PI) / 180.0f;  // angle is degrees off vertical
    auto toWidth = std::tan(angle);
    auto thickness = dc.roundToNearestPixel(0.1333f * r.height);
    auto xThickness = thickness / std::sin(0.5f * float(M_PI) - angle);
    auto width = r.height * toWidth + xThickness;

    auto iRect = r.insetted(0.5f * (r.width - width), PicaPt::kZero);
    auto path = dc.createBezierPath();
    path->moveTo(iRect.lowerLeft());
    path->lineTo(Point(iRect.maxX() - xThickness, iRect.y));
    path->lineTo(iRect.upperRight());
    path->lineTo(Point(iRect.x + xThickness, iRect.maxY()));
    path->close();
    dc.drawPath(path, kPaintFill);
}

void StandardIconPainter::drawUnderlineStyle(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);

    // Design note: why not draw text? Because we want the character to fill
    // the height, and the point size of a font has no relationship with its pixel height.
    // Also, this way everything is consistent. across platforms.
    auto underlineWidth = std::max(dc.onePixel(), dc.roundToNearestPixel(0.05f * r.height));
    auto uHeight = 0.85f * r.height;
    auto aspectRatio = 0.75f;
    auto thickness = dc.roundToNearestPixel(0.1333f * r.height);  // measure from height, like B and I
    auto x0 = r.x + 0.5f * (r.width - dc.roundToNearestPixel(aspectRatio * uHeight));
    auto x1 = x0 + thickness;
    auto outerRadius = 0.5f * (r.maxX() - 2.0f * x0);
    auto innerRadius = 0.5f * (r.maxX() - 2.0f * x1);

    auto path = dc.createBezierPath();
    path->moveTo(Point(x0, r.y));
    path->lineTo(Point(x0, r.y + uHeight - outerRadius));
    path->quarterEllipseTo(Point(x0, r.y + uHeight),
                           Point(x0 + outerRadius, r.y + uHeight));
    path->quarterEllipseTo(Point(r.maxX() - x0, r.y + uHeight),
                           Point(r.maxX() - x0, r.y + uHeight - outerRadius));
    path->lineTo(Point(r.maxX() - x0, r.y));
    path->lineTo(Point(r.maxX() - x1, r.y));
    path->lineTo(Point(r.maxX() - x1, r.y + uHeight - outerRadius));
    path->quarterEllipseTo(Point(r.maxX() - x1, r.y + uHeight - outerRadius + innerRadius),
                           Point(r.maxX() - x1 - innerRadius, r.y + uHeight - outerRadius + innerRadius));
    path->quarterEllipseTo(Point(r.x + x1, r.y + uHeight - outerRadius + innerRadius),
                           Point(r.x + x1, r.y + uHeight - outerRadius));
    path->lineTo(Point(x1, r.y));
    path->close();
    dc.drawPath(path, kPaintFill);

    auto y = r.maxY() - 0.5f * underlineWidth;
    dc.setStrokeEndCap(kEndCapButt);
    dc.drawLines({ Point(x0, y), Point(r.maxX() - x0, y) });
}

void StandardIconPainter::drawAlignLeft(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    drawAlignedLines(dc, r, sw, Alignment::kLeft);
}

void StandardIconPainter::drawAlignCenter(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    drawAlignedLines(dc, r, sw, Alignment::kCenter);
}

void StandardIconPainter::drawAlignRight(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    drawAlignedLines(dc, r, sw, Alignment::kRight);
}

void StandardIconPainter::drawAlignJustify(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    drawAlignedLines(dc, r, sw, Alignment::kJustify);
}

void StandardIconPainter::drawBulletList(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    dc.setFillColor(fg);
    auto r = calcContentRect(size);
    r.inset(0.5f * sw, 0.5 * sw);

    auto bulletRadius = 1.333f * sw;
    auto lineInset = std::max(4.0f * bulletRadius, 0.25f * r.width);

    auto drawBullet = [&r, &bulletRadius](int i, DrawContext& dc, const PicaPt& y) {
        dc.drawEllipse(Rect(r.x, y - bulletRadius,
                            2.0f * bulletRadius, 2.0f * bulletRadius), kPaintFill);
    };
    drawList(dc, r, sw, lineInset, drawBullet);
}

void StandardIconPainter::drawNumericList(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.5f * sw, 0.5 * sw);

    bool useOnlyTwo = false;
    auto minBulletHeight = 2.5f * sw;
    auto bulletHeight = 2.0f * dc.roundToNearestPixel(0.075f * r.height);  // ensure divisible by 2
    if (bulletHeight < minBulletHeight) {
        bulletHeight = dc.roundToNearestPixel(0.333f * r.height);  // does not need to divide by 2
        useOnlyTwo = true;
    }
    auto lineInset = std::max(1.5f * bulletHeight,       // ideal
                              bulletHeight + 2.0f * sw); // minimal (small icons)
    Point iHat(0.5f * r.width, PicaPt::kZero);
    Point jHat(PicaPt::kZero, 0.5f * r.width);

    auto drawBullet = [&r, &bulletHeight, &iHat, &jHat, useOnlyTwo](int i, DrawContext& dc, const PicaPt& y) {
        Rect br(r.x, y - 0.5f * bulletHeight, bulletHeight, bulletHeight);
        if (useOnlyTwo) {
            if (i == 0) {
                br.y = r.y + dc.roundToNearestPixel(0.25f * r.height - 0.5f * bulletHeight);
            } else {
                br.y = r.y + dc.roundToNearestPixel(0.75f * r.height - 0.5f * bulletHeight);
            }
            if (i >= 2) {
                return;
            }
        }
        br.inset(0.1f * br.width, PicaPt::kZero);
        auto path = dc.createBezierPath();
        switch (i) {
            case 0:  // 1
                path->moveTo(Point(br.midX() - 0.25f * bulletHeight, br.y + 0.25f * bulletHeight));
                path->lineTo(Point(br.midX(), br.y));
                path->lineTo(Point(br.midX(), br.maxY()));
                break;
            case 1: {  // 2
                Point start(br.x, br.y + 0.3f * br.height);
                path->moveTo(start);
                path->cubicTo(start - 0.05f * jHat,
                              Point(br.midX(), br.y) - 0.1f * iHat,
                              Point(br.midX(), br.y));
                path->quarterEllipseTo(br.upperRight(), Point(br.maxX(), br.midY()) - 0.075f * jHat);
                path->cubicTo(Point(br.maxX(), br.midY()) + 0.05f * jHat,
                              br.lowerLeft() - 0.1f * jHat,
                              br.lowerLeft());
                path->lineTo(br.lowerRight());
                break;
                }
            case 2: {  // 3
                auto ry = 0.25f * br.height;
                path->moveTo(Point(br.x, br.y + ry));
                path->quarterEllipseTo(br.upperLeft(), Point(br.midX(), br.y));
                path->quarterEllipseTo(br.upperRight(), Point(br.maxX(), br.y + ry));
                path->quarterEllipseTo(Point(br.maxX(), br.midY()), br.center());
                path->quarterEllipseTo(Point(br.maxX(), br.midY()), Point(br.maxX(), br.maxY() - ry));
                path->quarterEllipseTo(br.lowerRight(), Point(br.midX(), br.maxY()));
                path->quarterEllipseTo(br.lowerLeft(), Point(br.x, br.maxY() - ry));
                break;
                }
            case 3: {  // 4
                auto x = br.x + 0.8f * br.width;
                auto y = br.y + 0.75f * br.height;
                path->moveTo(Point(x, br.maxY()));
                path->lineTo(Point(x, br.y));
                path->lineTo(Point(br.x, y));
                path->lineTo(Point(br.maxX(), y));
                break;
                }
            default:
                break;
        }
        dc.drawPath(path, kPaintStroke);
    };
    drawList(dc, r, sw, lineInset, drawBullet);
}

void StandardIconPainter::drawPlay(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    dc.setFillColor(fg);
    auto r = calcContentRect(size);
    auto path = dc.createBezierPath();
    path->moveTo(r.upperLeft());
    path->lineTo(r.lowerLeft());
    path->lineTo(Point(r.maxX(), r.midY()));
    path->close();
    dc.drawPath(path, kPaintFill);
}

void StandardIconPainter::drawPause(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    dc.setFillColor(fg);
    auto r = calcContentRect(size);
    // The play-triangle is visually not there at the corners, so inset the pause icon
    // vertically so it has the same apparent size as the play icon.
    r.inset(PicaPt::kZero, dc.roundToNearestPixel(0.05f * r.width));

    // We want five equal parts:  .|.|.
    auto barWidth = dc.ceilToNearestPixel(0.2f * r.width);
    // but at low pixel counds, barWidth can be rather different than 20%. So, we ceil()
    // the width, for consistency, and adjust the inset, which is not actually important
    auto inset = dc.floorToNearestPixel(0.5f * (r.width - 3.0f * barWidth));

    dc.drawRect(Rect(r.x + inset, r.y, barWidth, r.height), kPaintFill);
    dc.drawRect(Rect(r.x + inset + 2.0f * barWidth, r.y, barWidth, r.height), kPaintFill);
}

void StandardIconPainter::drawStop(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    dc.setFillColor(fg);
    auto r = calcContentRect(size);
    // The play-triangle is visually not there at the corners, so inset the pause icon
    // vertically so it has the same apparent size as the play icon.
    auto inset = dc.roundToNearestPixel(0.05f * r.width);
    r.inset(inset, inset);
    dc.drawRect(r, kPaintFill);
}

void StandardIconPainter::drawFastForward(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    dc.setFillColor(fg);
    auto r = calcContentRect(size);
    r.inset(PicaPt::kZero, 0.19f * r.width);  // rounding to a pixel is problematic for low-pixel counts
    const auto edgeLen = r.height;  // for clarity in reading
    const auto firstLen = r.width - edgeLen;

    auto path = dc.createBezierPath();
    path->moveTo(r.upperLeft());
    path->lineTo(r.lowerLeft());
    path->lineTo(Point(r.x + firstLen, r.maxY() - 0.5f * firstLen));
    path->lineTo(Point(r.x + firstLen, r.maxY()));
    path->lineTo(Point(r.maxX(), r.midY()));
    path->lineTo(Point(r.x + firstLen, r.y));
    path->lineTo(Point(r.x + firstLen, r.y + 0.5f * firstLen));
    path->close();
    dc.drawPath(path, kPaintFill);
}

void StandardIconPainter::drawFastReverse(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    dc.setFillColor(fg);
    auto r = calcContentRect(size);
    r.inset(PicaPt::kZero, 0.19f * r.width);  // rounding to a pixel is problematic for low-pixel counts
    const auto edgeLen = r.height;  // for clarity in reading
    const auto firstLen = r.width - edgeLen;

    auto path = dc.createBezierPath();
    path->moveTo(r.upperRight());
    path->lineTo(r.lowerRight());
    path->lineTo(Point(r.maxX() - firstLen, r.maxY() - 0.5f * firstLen));
    path->lineTo(Point(r.maxX() - firstLen, r.maxY()));
    path->lineTo(Point(r.x, r.midY()));
    path->lineTo(Point(r.maxX() - firstLen, r.y));
    path->lineTo(Point(r.maxX() - firstLen, r.y + 0.5f * firstLen));
    path->close();
    dc.drawPath(path, kPaintFill);
}

void StandardIconPainter::drawSkipForward(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    dc.setFillColor(fg);
    auto r = calcContentRect(size);
    auto barWidth = dc.ceilToNearestPixel(0.1f * r.width);
    auto firstLen = r.width - barWidth;

    auto path = dc.createBezierPath();
    path->moveTo(r.upperLeft());
    path->lineTo(r.lowerLeft());
    path->lineTo(Point(r.x + firstLen, r.maxY() - 0.5f * firstLen));
    path->lineTo(Point(r.x + firstLen, r.maxY()));
    path->lineTo(r.lowerRight());
    path->lineTo(r.upperRight());
    path->lineTo(Point(r.x + firstLen, r.y));
    path->lineTo(Point(r.x + firstLen, r.y + 0.5f * firstLen));
    path->close();
    dc.drawPath(path, kPaintFill);
}

void StandardIconPainter::drawSkipBackward(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    dc.setFillColor(fg);
    auto r = calcContentRect(size);
    auto barWidth = dc.ceilToNearestPixel(0.1f * r.width);
    auto firstLen = r.width - barWidth;

    auto path = dc.createBezierPath();
    path->moveTo(r.upperRight());
    path->lineTo(r.lowerRight());
    path->lineTo(Point(r.maxX() - firstLen, r.maxY() - 0.5f * firstLen));
    path->lineTo(Point(r.maxX() - firstLen, r.maxY()));
    path->lineTo(r.lowerLeft());
    path->lineTo(r.upperLeft());
    path->lineTo(Point(r.maxX() - firstLen, r.y));
    path->lineTo(Point(r.maxX() - firstLen, r.y + 0.5f * firstLen));
    path->close();
    dc.drawPath(path, kPaintFill);
}

void StandardIconPainter::drawShuffle(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.5 * sw, 0.5 * sw);
    // Often the shuffle and loop icons are wider than they are high, so base the
    // values off the height, which is the constant (as far as the aspect ratio is concerned).
    auto halfEdge = dc.roundToNearestPixel(0.15f * r.height);
    auto cpX = 0.666f * r.height;

    auto path = dc.createBezierPath();
    path->moveTo(Point(r.x, r.y + halfEdge));
    path->cubicTo(Point(r.x + cpX, r.y + halfEdge),
                  Point(r.maxX() - cpX, r.maxY() - halfEdge),
                  Point(r.maxX(), r.maxY() - halfEdge));

    path->moveTo(Point(r.x, r.maxY() - halfEdge));
    path->cubicTo(Point(r.x + cpX, r.maxY() - halfEdge),
                  Point(r.maxX() - cpX, r.y + halfEdge),
                  Point(r.maxX(), r.y + halfEdge));

    path->moveTo(Point(r.maxX() - halfEdge, r.y));
    path->lineTo(Point(r.maxX(), r.y + halfEdge));
    path->lineTo(Point(r.maxX() - halfEdge, r.y + 2.0f * halfEdge));

    path->moveTo(Point(r.maxX() - halfEdge, r.maxY()));
    path->lineTo(Point(r.maxX(), r.maxY() - halfEdge));
    path->lineTo(Point(r.maxX() - halfEdge, r.maxY() - 2.0f * halfEdge));

    dc.drawPath(path, kPaintStroke);
}

void StandardIconPainter::drawLoop(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.5 * sw, 0.5 * sw);
    // Often the shuffle and loop icons are wider than they are high, so base the
    // values off the height, which is the constant (as far as the aspect ratio is concerned).
    auto halfEdge = dc.roundToNearestPixel(0.15f * r.height);
    auto radius = 0.5f * r.height - halfEdge;

    auto path = dc.createBezierPath();
    path->moveTo(Point(r.x, r.midY()));
    path->quarterEllipseTo(Point(r.x, r.midY() - radius), Point(r.x + radius, r.midY() - radius));
    path->lineTo(Point(r.maxX() - radius, r.midY() - radius));

    path->moveTo(Point(r.maxX(), r.midY()));
    path->quarterEllipseTo(Point(r.maxX(), r.midY() + radius), Point(r.maxX() - radius, r.midY() + radius));
    path->lineTo(Point(r.x + radius, r.midY() + radius));

    path->moveTo(Point(r.maxX() - radius - halfEdge, r.y));
    path->lineTo(Point(r.maxX() - radius,            r.y + halfEdge));
    path->lineTo(Point(r.maxX() - radius - halfEdge, r.y + 2.0f * halfEdge));

    path->moveTo(Point(r.x + radius + halfEdge, r.maxY()));
    path->lineTo(Point(r.x + radius,            r.maxY() - halfEdge));
    path->lineTo(Point(r.x + radius + halfEdge, r.maxY() +- 2.0f * halfEdge));

    dc.drawPath(path, kPaintStroke);
}

void StandardIconPainter::drawVolumeMute(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    drawVolume(dc, r, fg, sw, 0.0f);
}

void StandardIconPainter::drawVolumeSoft(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    drawVolume(dc, r, fg, sw, 0.1f);
}

void StandardIconPainter::drawVolumeMedium(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    drawVolume(dc, r, fg, sw, 0.5f);
}

void StandardIconPainter::drawVolumeLoud(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    drawVolume(dc, r, fg, sw, 1.0f);
}

void StandardIconPainter::drawZoomIn(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    auto glassRect = drawMagnifyingGlass(dc, r, sw);
    drawPlusOrMinus(dc, glassRect, sw, kPlus);
}

void StandardIconPainter::drawZoomOut(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    auto glassRect = drawMagnifyingGlass(dc, r, sw);
    drawPlusOrMinus(dc, glassRect, sw, kMinus);
}

void StandardIconPainter::drawRecordAudio(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.5f * sw, 0.5f * sw);

    auto micXInset = dc.roundToNearestPixel(0.333f * r.width);
    auto micRadius = 0.5f * r.width - micXInset;
    auto micHeight = 0.666f * r.height;
    auto hemiBottom = r.y + 0.8333f * r.height;
    auto hemiRadius = micRadius + (hemiBottom - micHeight);
    auto baseLen = r.maxY() - hemiBottom;

    auto path = dc.createBezierPath();
    path->moveTo(Point(r.x + micXInset, r.y + micRadius));
    path->quarterEllipseTo(Point(r.x + micXInset, r.y), Point(r.x + micXInset + micRadius, r.y));
    path->quarterEllipseTo(Point(r.maxX() - micXInset, r.y),
                           Point(r.maxX() - micXInset, r.y + micRadius));
    path->lineTo(Point(r.maxX() - micXInset, r.y + micHeight - micRadius));
    path->quarterEllipseTo(Point(r.maxX() - micXInset, r.y + micHeight),
                           Point(r.x + micXInset + micRadius, r.y + micHeight));
    path->quarterEllipseTo(Point(r.x + micXInset, r.y + micHeight),
                           Point(r.x + micXInset, r.y + micHeight - micRadius));
    path->close();

    path->moveTo(Point(r.midX() - hemiRadius, hemiBottom - hemiRadius));
    path->quarterEllipseTo(Point(r.midX() - hemiRadius, hemiBottom),
                           Point(r.midX(), hemiBottom));
    path->quarterEllipseTo(Point(r.midX() + hemiRadius, hemiBottom),
                           Point(r.midX() + hemiRadius, hemiBottom - hemiRadius));

    path->moveTo(Point(r.midX(), hemiBottom));
    path->lineTo(Point(r.midX(), r.maxY()));
    path->moveTo(Point(r.midX() - 0.5f * baseLen, r.maxY()));
    path->lineTo(Point(r.midX() + 0.5f * baseLen, r.maxY()));

    dc.drawPath(path, kPaintStroke);
}

void StandardIconPainter::drawRecordVideo(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.5f * sw, 0.5f * sw);

    auto vInset = dc.roundToNearestPixel(0.2f * r.height);
    auto vInset2 = dc.roundToNearestPixel(0.4f * r.height);
    auto bodyWidth = dc.roundToNearestPixel(0.7f * r.width);
    auto x2 = r.x + bodyWidth + 2.0f * sw;
    auto triLen = r.maxX() - x2;

    auto path = dc.createBezierPath();
    path->addRoundedRect(Rect(r.x, r.y + vInset, bodyWidth, r.height - 2.0f * vInset), 0.05f * r.width);
    path->moveTo(Point(x2, r.y + vInset2));
    path->lineTo(Point(x2 + triLen, r.y + vInset2 - triLen));
    path->lineTo(Point(x2 + triLen, r.maxY() - vInset2 + triLen));
    path->lineTo(Point(x2, r.maxY() - vInset2));
    path->close();
    dc.drawPath(path, kPaintStroke);
}

void StandardIconPainter::drawNoAudio(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    dc.save();
    dc.clipToPath(clipRectForSlash(dc, r, sw));
    drawRecordAudio(dc, r.size(), fg);
    dc.restore();
    drawSlash(dc, r, sw);
}

void StandardIconPainter::drawNoVideo(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    dc.save();
    dc.clipToPath(clipRectForSlash(dc, r, sw));
    drawRecordVideo(dc, r.size(), fg);
    dc.restore();
    drawSlash(dc, r, sw);
}

void StandardIconPainter::drawCamera(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.5f * sw, 0.5f * sw);

    auto borderRadius = calcBorderRadius(r);
    auto veryTopInset = dc.roundToNearestPixel(0.1f * r.height);
    auto topInset = dc.roundToNearestPixel(0.25f * r.height);
    auto bottomInset = dc.roundToNearestPixel(0.05f * r.height);
    auto prismXInset = dc.roundToNearestPixel(0.25f * r.height);
    auto prismHeight = topInset - veryTopInset;
    auto shutterHeight = dc.roundToNearestPixel(0.1f * r.height);
    auto shutterWidth = shutterHeight;
    auto lensRadius = 0.25f * r.height;

    auto path = dc.createBezierPath();
    path->moveTo(Point(r.x, r.y + topInset + borderRadius));
    path->quarterEllipseTo(Point(r.x, r.y + topInset), Point(r.x + borderRadius, r.y + topInset));
    path->lineTo(Point(r.x + prismXInset, r.y + topInset));
    path->lineTo(Point(r.x + prismXInset + prismHeight, r.y + veryTopInset));
    path->lineTo(Point(r.maxX() - prismXInset - prismHeight, r.y + veryTopInset));
    path->lineTo(Point(r.maxX() - prismXInset, r.y + topInset));
    path->lineTo(Point(r.maxX() - borderRadius, r.y + topInset));
    path->quarterEllipseTo(Point(r.maxX(), r.y + topInset),
                           Point(r.maxX(), r.y + topInset + borderRadius));
    path->lineTo(Point(r.maxX(), r.maxY() - bottomInset - borderRadius));
    path->quarterEllipseTo(Point(r.maxX(), r.maxY() - bottomInset),
                           Point(r.maxX() - borderRadius, r.maxY() - bottomInset));
    path->lineTo(Point(r.x + borderRadius, r.maxY() - bottomInset));
    path->quarterEllipseTo(Point(r.x, r.maxY() - bottomInset),
                           Point(r.x, r.maxY() - bottomInset - borderRadius));
    path->close();

    auto cy = 0.5f * (topInset + r.height - bottomInset);
    path->addEllipse(Rect(r.midX() - lensRadius, cy - lensRadius, 2.0f * lensRadius, 2.0f * lensRadius));

    dc.drawPath(path, kPaintStroke);
}

void StandardIconPainter::drawFolder(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    auto inset = std::max(dc.onePixel(), dc.roundToNearestPixel(0.05f * r.height));
    r.inset(0.5 * sw, inset + 0.5 * sw);
    auto bottomHeight = dc.roundToNearestPixel(0.666f * r.height);
    auto midHeight = dc.roundToNearestPixel(0.25f * (r.height - bottomHeight));

    auto path = dc.createBezierPath();
    path->moveTo(Point(r.x, r.maxY() - bottomHeight));
    path->lineTo(r.lowerLeft());
    path->lineTo(r.lowerRight());
    path->lineTo(Point(r.maxX(), r.maxY() - bottomHeight));
    path->lineTo(Point(r.x, r.maxY() - bottomHeight));
    path->lineTo(r.upperLeft());
    path->lineTo(Point(r.x + dc.roundToNearestPixel(0.4f * r.width), r.y));
    path->lineTo(Point(r.x + dc.roundToNearestPixel(0.4f * r.width), r.y + midHeight));
    path->lineTo(Point(r.maxX(), r.y + midHeight));
    path->lineTo(Point(r.maxX(), r.maxY() - bottomHeight));
    dc.drawPath(path, kPaintStroke);
}

void StandardIconPainter::drawFile(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(dc.roundToNearestPixel(0.1f * r.width) + 0.5f * sw, 0.5f * sw);
    auto earLen = dc.roundToNearestPixel(0.5f * r.width);
    dc.setStrokeJoinStyle(kJoinMiter);  // files always seem to have sharp corners, even Apple's
    auto path = dc.createBezierPath();
    // We need to be a little careful about the order we draw these in,
    // otherwise we can get a sharp angle which results in mitre that extends out of
    // the rect.
    path->moveTo(Point(r.maxX(), r.y + earLen));
    path->lineTo(Point(r.maxX() - earLen, r.y));
    path->lineTo(r.upperLeft());
    path->lineTo(r.lowerLeft());
    path->lineTo(r.lowerRight());
    path->lineTo(Point(r.maxX(), r.y + earLen));
    path->lineTo(Point(r.maxX() - earLen, r.y + earLen));
    path->lineTo(Point(r.maxX() - earLen, r.y));
    dc.drawPath(path, kPaintStroke);
}

void StandardIconPainter::drawTrash(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.5f * sw, 0.5f * sw);

    auto borderRadius = calcBorderRadius(r);
    auto lidInset = dc.roundToNearestPixel(0.1f * r.height);
    lidInset = std::max(2.0f * sw, lidInset);
    auto handleXInset = dc.roundToNearestPixel(0.333f * r.width);
    auto topSideInset = dc.roundToNearestPixel(0.1f * r.height);
    auto bottomSideInset = dc.roundToNearestPixel(0.15f * r.height);
    if (bottomSideInset == topSideInset) {
        bottomSideInset = topSideInset + dc.onePixel();
    }

    auto path = dc.createBezierPath();
    path->moveTo(Point(r.x, r.y + lidInset));
    path->lineTo(Point(r.maxX(), r.y + lidInset));

    path->moveTo(Point(r.x + handleXInset, r.y + lidInset));
    path->lineTo(Point(r.x + handleXInset, r.y + borderRadius));
    path->quadraticTo(Point(r.x + handleXInset, r.y), Point(r.x + handleXInset + borderRadius, r.y));
    path->lineTo(Point(r.maxX() - handleXInset - borderRadius, r.y));
    path->quadraticTo(Point(r.maxX() - handleXInset, r.y),
                      Point(r.maxX() - handleXInset, r.y + borderRadius));
    path->lineTo(Point(r.maxX() - handleXInset, r.y + lidInset));

    path->moveTo(Point(r.x + topSideInset, r.y + lidInset));
    path->lineTo(Point(r.x + bottomSideInset, r.maxY()));
    path->lineTo(Point(r.maxX() - bottomSideInset, r.maxY()));
    path->lineTo(Point(r.maxX() - topSideInset, r.y + lidInset));

    Rect xRect = r.insetted(0.333f * r.width, 0.333f * r.height)
                  .translated(PicaPt::kZero, 0.05f * r.height);
    path->moveTo(xRect.upperLeft());
    path->lineTo(xRect.lowerRight());
    path->moveTo(xRect.upperRight());
    path->lineTo(xRect.lowerLeft());

    dc.drawPath(path, kPaintStroke);
}

void StandardIconPainter::drawEdit(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.5f * sw, 0.5f * sw);

    auto halfThickness = dc.roundToNearestPixel(0.2f * r.height);

    Point end1(r.maxX() - halfThickness, r.y);
    Point end2(r.maxX(), r.y + halfThickness);

    auto path = dc.createBezierPath();
    path->moveTo(r.lowerLeft());
    path->lineTo(Point(r.x, r.maxY() - halfThickness));
    path->lineTo(end1);
    path->lineTo(end2);
    path->lineTo(Point(r.x + halfThickness, r.maxY()));
    path->close();

    path->moveTo(Point(r.maxX() - 1.75f * halfThickness, r.y + 0.75f * halfThickness));
    path->lineTo(Point(r.maxX() - 0.75f * halfThickness, r.y + 1.75f * halfThickness));

    dc.drawPath(path, kPaintStroke);
}

void StandardIconPainter::drawHome(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.5f * sw, 0.5f * sw);

    auto borderRadius = calcBorderRadius(r);
    auto xInset = dc.roundToNearestPixel(0.1f * r.width);
    auto roofBottom = dc.roundToNearestPixel(0.333f * r.height);
    auto doorInset = dc.roundToNearestPixel(0.333f * r.width);
    auto doorHeight = dc.roundToNearestPixel(0.5f * r.height);

    auto path = dc.createBezierPath();
    path->moveTo(Point(r.midX(), r.y));
    path->lineTo(Point(r.x, r.y + roofBottom));
    path->lineTo(Point(r.x + xInset, r.y + roofBottom));
    path->lineTo(Point(r.x + xInset, r.maxY() - borderRadius));
    path->quarterEllipseTo(Point(r.x + xInset, r.maxY()), Point(r.x + xInset + borderRadius, r.maxY()));
    path->lineTo(Point(r.maxX() - xInset - borderRadius, r.maxY()));
    path->quarterEllipseTo(Point(r.maxX() - xInset, r.maxY()),
                           Point(r.maxX() - xInset, r.maxY() - borderRadius));
    path->lineTo(Point(r.maxX() - xInset, r.y + roofBottom));
    path->lineTo(Point(r.maxX(), r.y + roofBottom));
    path->close();

    path->moveTo(Point(r.x + doorInset, r.maxY()));
    path->lineTo(Point(r.x + doorInset, r.maxY() - doorHeight + borderRadius));
    path->quarterEllipseTo(Point(r.x + doorInset, r.maxY() - doorHeight),
                           Point(r.x + doorInset + borderRadius, r.maxY() - doorHeight));
    path->lineTo(Point(r.maxX() - doorInset - borderRadius, r.maxY() - doorHeight));
    path->quarterEllipseTo(Point(r.maxX() - doorInset, r.maxY() - doorHeight),
                           Point(r.maxX() - doorInset, r.maxY() - doorHeight + borderRadius));
    path->lineTo(Point(r.maxX() - doorInset, r.maxY()));

    dc.drawPath(path, kPaintStroke);
}

void StandardIconPainter::drawPicture(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.5f * sw, 0.5f * sw);

    auto yInset = dc.roundToNearestPixel(0.1f * r.height);
    auto path = dc.createBezierPath();
    path->addRect(r.insetted(PicaPt::kZero, yInset));

    auto x1 = r.x + 0.1f * r.width;
    auto x3 = r.maxX() - 0.1f * r.width;
    auto h1 = 0.6f * r.height;
    auto h2 = 0.4f * r.height;
    auto yBot = r.maxY() - yInset;
    path->moveTo(Point(x1, yBot));
    path->lineTo(Point(x1 + 0.5f * h1, yBot - h1));
    path->lineTo(Point(x1 + h1, yBot));
    path->moveTo(Point(x3, yBot));
    path->lineTo(Point(x3 - 0.5f * h2, yBot - h2));
    auto dy = 0.33f * h2;
    path->lineTo(Point(x3 - 0.5f * h2 - dy, yBot - h2 + dy));

    dc.drawPath(path, kPaintStroke);
}

void StandardIconPainter::drawDocument(DrawContext& dc, const Size& size, const Color& fg) const
{
    drawFile(dc, size, fg);

    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.5f * sw, 0.5f * sw);

    auto xInset = dc.roundToNearestPixel(0.25f * r.width);
    auto top = dc.roundToNearestPixel(0.45f * r.height);
    auto dy = dc.roundToNearestPixel(0.1f * r.height);
    dy = std::max(2.0f * sw, dy);

    auto y = r.maxY();
    while (y >= top) {
        dc.drawLines({ Point(r.x + xInset, y), Point(r.maxX() - xInset, y) });
        y -= dy;
    }
}

void StandardIconPainter::drawUser(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.5f * sw, 0.5f * sw);

    auto headRadius = 0.25f * r.height;
    auto headCY = r.midY() - 0.2f * r.height;
    auto dxCP = 0.2f * r.width;
    auto dyCP = 0.4f * r.height;

    auto path = dc.createBezierPath();
    path->addEllipse(Rect(r.midX() - headRadius, headCY - headRadius,
                          2.0f * headRadius, 2.0f * headRadius));

    Point start(r.x + 0.75f * sw, r.maxY());    // subtract off extra from x to compensate for mitre
    Point end(r.maxX() - 0.75f * sw, r.maxY());

    path->moveTo(start);
    path->cubicTo(start + Point(dxCP, -dyCP),
                  end + Point(-dxCP, -dyCP),
                  end);
    path->close();

    dc.drawPath(path, kPaintStroke);
}

void StandardIconPainter::drawColor(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.5f * sw, 0.5f * sw);

    Point iHat(0.5f * r.width, PicaPt::kZero);
    Point jHat(PicaPt::kZero, 0.5f * r.height);
    Point midLeft(r.x, r.midY());
    Point midTop(r.midX(), r.y);
    Point midRight(r.maxX(), r.midY());
    Point centerish(r.x + 0.6f * r.width, r.y + 0.4f * r.height);
    Point midBottom(r.midX() + 0.1f * iHat.x, r.maxY());

    auto path = dc.createBezierPath();
    path->moveTo(midLeft);
    path->cubicTo(midLeft - 0.51f * jHat, midTop - 0.51f * iHat, midTop);
    path->cubicTo(midTop + 0.9f * iHat, midRight + 0.4f * iHat - 0.4f * jHat, midRight);
//    path->cubicTo(midRight + 0.5f * jHat, midBottom + 0.5f * iHat, midBottom);
    path->cubicTo(midRight - 0.2f * iHat + 0.2f * jHat, centerish + 0.4f * iHat - 0.4f * jHat, centerish);
    path->cubicTo(centerish - 0.4f * iHat + 0.4f * jHat, midBottom + 0.5f * iHat - 0.5f * jHat, midBottom);
    path->cubicTo(midBottom - 0.2f * iHat + 0.2f * jHat, midLeft + 0.9f * jHat, midLeft);

    dc.drawPath(path, kPaintStroke);

    path = dc.createBezierPath();
    path->addCircle(r.lowerLeft() + 0.7f * iHat - 0.5f * jHat, 0.1f * r.width);
    path->addCircle(r.lowerLeft() + 0.55f * iHat - 1.0f * jHat, 0.09f * r.width);
    path->addCircle(r.lowerLeft() + 0.8f * iHat - 1.5f * jHat, 0.08f * r.width);
    dc.drawPath(path, kPaintFill);
}

void StandardIconPainter::drawStar(DrawContext& dc, const Size& size, const Color& fg) const
{
    struct RPoint {
        float r;      // [0, 1], with 1 being 1 radii
        float angle;  // [0, 1], multplied with the swept angle

        Point toPoint(const Point& center, const PicaPt& radius, float theta, float thetaSweep)
        {
            float rad = theta + angle * thetaSweep;
            float sinT = std::sin(rad);
            float cosT = std::cos(rad);
            return Point(center.x - r * radius * sinT, center.y + r * radius * cosT);
        }
    };

    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    // The widest part of the star is just a little bit smaller than the width,
    // so we do not need to inset for the stroke width. But we do need to move down a
    // little bit to center the star in the box.
    r.translate(PicaPt::kZero, 0.075f * r.height);

    int n = 5;
    float inner = 0.333f;  // [0, 1], with 1 being 1 radii
    float pointyness = 0.99f;  // 0 is round, 1 is sharp

    float sweepAngle = 2.0f * float(M_PI) / float(n);
    auto radius = 0.5f * r.height;
    float mid = 0.5f * (inner + 1.0f);
    auto center = r.center();

    auto path = dc.createBezierPath();

    for (int i = 0;  i < n;  ++i) {
        float theta = -0.5f * sweepAngle + float(i) * sweepAngle;
        Point lastInner = RPoint{inner, -0.5f}.toPoint(center, radius, theta, sweepAngle);
        Point outerPt = RPoint{1.0f, 0.0f}.toPoint(center, radius, theta, sweepAngle);
        Point innerPt = RPoint{inner, 0.5f}.toPoint(center, radius, theta, sweepAngle);
        Point nextOuter = RPoint{1.0f, 1.0f}.toPoint(center, radius, theta, sweepAngle);

        Point half1 = 0.5f * lastInner + 0.5f * outerPt;
        Point half2 = 0.5f * innerPt + 0.5f * outerPt;
        Point half3 = 0.5f * innerPt + 0.5f * nextOuter;
        if (i == 0) {
            path->moveTo(half1);
        }
        path->cubicTo((1.0f - pointyness) * half1 + pointyness * outerPt,
                      (1.0f - pointyness) * half2 + pointyness * outerPt,
                      half2);
        path->cubicTo((1.0f - pointyness) * half2 + pointyness * innerPt,
                      (1.0f - pointyness) * half3 + pointyness * innerPt,
                      half3);
    }

/*    for (int i = 0;  i < n;  ++i) {
        float theta = -0.5f * sweepAngle + float(i) * sweepAngle;
        Point lastInner = RPoint{inner, -0.5f}.toPoint(center, radius, theta, sweepAngle);
        Point outerPt = RPoint{1.0f, 0.0f}.toPoint(center, radius, theta, sweepAngle);
        Point innerPt = RPoint{inner, 0.5f}.toPoint(center, radius, theta, sweepAngle);
        Point nextOuter = RPoint{1.0f, 1.0f}.toPoint(center, radius, theta, sweepAngle);

        Point half1 = 0.5f * lastInner + 0.5f * outerPt;
        Point half2 = 0.5f * innerPt + 0.5f * outerPt;
        Point half3 = 0.5f * innerPt + 0.5f * nextOuter;
        if (i == 0) {
            path->moveTo(half1);
        }
        path->lineTo(outerPt);
        path->lineTo(innerPt);
        path->lineTo(half3);
    } */

    dc.drawPath(path, kPaintStroke);
}

void StandardIconPainter::drawHeart(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.5f * sw, 0.5f * sw);

    auto radius = 0.25f * r.width;  // not changeable
    auto cpWeight = 1.25f * radius;

    auto path = dc.createBezierPath();
    path->moveTo(Point(r.midX(), r.y + radius));
    path->quarterEllipseTo(Point(r.midX(), r.y), Point(r.midX() - radius, r.y));
    path->quarterEllipseTo(Point(r.x , r.y), Point(r.x, r.y + radius));
    path->cubicTo(Point(r.x, r.y + radius + cpWeight),
                  Point(r.midX(), r.maxY()),
                  Point(r.midX(), r.maxY()));
    path->cubicTo(Point(r.midX(), r.maxY()),
                  Point(r.maxX(), r.y + radius + cpWeight),
                  Point(r.maxX(), r.y + radius));
    path->quarterEllipseTo(Point(r.maxX(), r.y), Point(r.maxX() - radius, r.y));
    path->quarterEllipseTo(Point(r.midX(), r.y), Point(r.midX(), r.y + radius));
    dc.drawPath(path, kPaintStroke);
}

void StandardIconPainter::drawMail(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.5f * sw, 0.5f * sw);

    auto yInset = dc.roundToNearestPixel(0.125f * size.height);
    auto flapStartInset = dc.roundToNearestPixel(0.25f * size.height);
    auto flapBottom = dc.roundToNearestPixel(0.6f * size.height);

    auto path = dc.createBezierPath();
    path->addRoundedRect(r.insetted(PicaPt::kZero, yInset), 0.05f * r.width);
    path->moveTo(Point(r.x, r.y + flapStartInset));
    path->lineTo(Point(r.midX(), r.y + flapBottom));
    path->lineTo(Point(r.maxX(), r.y + flapStartInset));
    dc.drawPath(path, kPaintStroke);
}

void StandardIconPainter::drawAttachment(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.5f * sw, 0.5f * sw);

    auto topOffset = dc.roundToNearestPixel(0.25f * r.height);
    auto dy = dc.roundToNearestPixel(0.175f * r.height);
    dy = std::max(2.0f * sw, dy);
    auto endOffset = dc.roundToNearestPixel(0.75f * r.width);
    auto largeRadius = 1.5f * dy;
    auto medRadius = dy;
    auto smallRadius = 0.5f * dy;

    auto path = dc.createBezierPath();
    path->moveTo(Point(r.x + endOffset, r.y + topOffset + 3.0f * dy));
    path->lineTo(Point(r.x + largeRadius, r.y + topOffset + 3.0f * dy));
    path->quarterEllipseTo(Point(r.x, r.y + topOffset + 3.0f * dy),
                           Point(r.x, r.y + topOffset + 3.0f * dy - largeRadius));
    path->quarterEllipseTo(Point(r.x, r.y + topOffset),
                           Point(r.x + largeRadius, r.y + topOffset));
    path->lineTo(Point(r.maxX() - medRadius, r.y + topOffset));
    path->quarterEllipseTo(Point(r.maxX(), r.y + topOffset),
                           Point(r.maxX(), r.y + topOffset + medRadius));
    path->quarterEllipseTo(Point(r.maxX(), r.y + topOffset + 2.0f * dy),
                           Point(r.maxX() - medRadius, r.y + topOffset + 2.0f * dy));
    path->lineTo(Point(r.x + dy + smallRadius, r.y + topOffset + 2.0f * dy));
    path->quarterEllipseTo(Point(r.x + dy, r.y + topOffset + 2.0f * dy),
                           Point(r.x + dy, r.y + topOffset + 2.0f * dy - smallRadius));
    path->quarterEllipseTo(Point(r.x + dy, r.y + topOffset + dy),
                           Point(r.x + dy + smallRadius, r.y + topOffset + dy));
    path->lineTo(Point(r.x + endOffset, r.y + topOffset + dy));
    dc.drawPath(path, kPaintStroke);
}

void StandardIconPainter::drawCalendar(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.5f * sw, 0.5f * sw);

    auto borderRadius = calcBorderRadius(r);
    auto top = dc.roundToNearestPixel(0.2f * r.height);
    auto ringXInset = dc.roundToNearestPixel(0.25f * r.width);

    auto path = dc.createBezierPath();
    path->addRoundedRect(Rect(r.x, r.y + top, r.width, r.height - top), borderRadius);
    path->moveTo(Point(r.x, r.y + top + sw));
    path->lineTo(Point(r.maxX(), r.y + top + sw));
    path->moveTo(Point(r.x + ringXInset, r.y));
    path->lineTo(Point(r.x + ringXInset, r.y + top));
    path->moveTo(Point(r.maxX() - ringXInset, r.y));
    path->lineTo(Point(r.maxX() - ringXInset, r.y + top));
    dc.drawPath(path, kPaintStroke);

    // It's difficult to draw lines and have it look like a calendar
    dc.drawRect(Rect(r.x + dc.roundToNearestPixel(0.2f * r.width),
                     r.y + top + dc.roundToNearestPixel(0.2f * r.height),
                     dc.roundToNearestPixel(0.3f * r.width),
                     dc.roundToNearestPixel(0.2f * r.width)),
                kPaintFill);
}

void StandardIconPainter::drawChat(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.5f * sw, 0.5f * sw);

    auto br = 0.1f * r.width;
    auto top = dc.roundToNearestPixel(0.1f * r.height);
    auto bottom = r.height - sw;  // the mitre at the point is fairly large
    auto pointHeight = dc.roundToNearestPixel(0.2f * r.height);
    auto pointWidth = 1.0f * pointHeight;
    auto pointXInset = dc.roundToNearestPixel(0.2f * r.width);
    auto lineXInset = dc.roundToNearestPixel(0.2f * r.width);

    // It is important that the lines are aligned, have the same spacing
    // between each other, and have equal spacing above and below, especially
    // at low pixel counts. To avoid problems like where the height is a prime
    // number and not divisible by any number, we let the top be flexible.
    auto onePx = dc.onePixel();
    PicaPt vPadding = sw;
    PicaPt dy = sw;
    int nLines = 4;

    // For sizes where the stroke width is an appreciable fraction of the height,
    // it is easier to figure out the best spacing by hand and look it up.
    const float kMaxNStrokes = 21.001f;
    float strokePx = std::round(sw / onePx);
    auto h = (bottom - pointHeight) - top - sw;
    float nStrokesHeight = h / (strokePx * onePx);  // in case sw is not an integer number of pixels
    if (nStrokesHeight <= kMaxNStrokes && std::abs(nStrokesHeight - std::round(nStrokesHeight)) > 0.001f) {
        // Don't use round, since 0.5f is common, and round alternates which direction n.5 rounds
        auto ds = nStrokesHeight - std::floor(nStrokesHeight);
        if (ds > 0.5f) {
            ds = -(1.0f - ds);
        }
        auto dPx = dc.roundToNearestPixel(ds * strokePx * onePx);
        if (top - dPx >= PicaPt::kZero) {
            top -= dPx;
            h -= dPx;
            nStrokesHeight = h / (strokePx * onePx);
        }
    }

    if (nStrokesHeight < 4.999f) {
        nLines = 0;
    } else if (nStrokesHeight <= kMaxNStrokes && std::abs(nStrokesHeight - std::round(nStrokesHeight)) < 0.001f)
    {
        int nStrokes = int(std::round(nStrokesHeight));
        struct LineLayout { int nLines; float spacing; float padding; };
        static LineLayout layouts[] = {
            { 0, 0.0f, 0.0f },  // 0 strokes
            { 0, 0.0f, 0.0f },  // 1 stroke
            { 0, 0.0f, 0.0f },  // 2 strokes
            { 0, 0.0f, 0.0f },  // 3 strokes
            { 0, 0.0f, 0.0f },  // 4 strokes
            { 2, 1.0f, 1.0f },  // 5 strokes  [_|_|_]
            { 0, 0.0f, 0.0f },  // 6 strokes  [] (no good solution)
            { 3, 1.0f, 1.0f },  // 7 strokes  [__|_|__]
            { 2, 2.0f, 2.0f },  // 8 strokes  [__|__|__]
            { 3, 1.0f, 2.0f },  // 9 strokes  [__|_|_|__] ({4, 1, 1} also works)
            { 2, 2.0f, 3.0f },  // 10 strokes [___|__|___]
            { 3, 2.0f, 2.0f },  // 11 strokes [__|__|__|__] ({5, 1, 1} also works)
            { 0, 0.0f, 0.0f },  // 12 strokes [] ({2, 4, 3} works, but is not a good solution)
            { 3, 2.0f, 3.0f },  // 13 strokes [___|__|__|___] ({5, 1, 2} also works)
            { 4, 2.0f, 2.0f },  // 14 strokes [__|__|__|__|__]
            { 3, 3.0f, 3.0f },  // 15 strokes [___|___|___|___]
            { 4, 2.0f, 3.0f },  // 16 strokes [___|__|__|__|___]
            { 5, 2.0f, 2.0f },  // 17 strokes [__|__|__|__|__|__]
            { 0, 0.0f, 0.0f },  // 18 strokes [] ({4, 2, 4} works, but is awkward)
            { 3, 4.0f, 4.0f },  // 19 strokes [____|____|____|____]
            { 0, 0.0f, 0.0f },  // 20 strokes []
            { 4, 3.0f, 4.0f },  // 21 strokes [____|___|___|___|____]
        };
        auto *layout = &layouts[nStrokes];
        if (layout->nLines == 0) {
            if (top >= sw) {
                top -= sw;
                h += sw;
                layout++;
            } else {
                top += sw;
                h -= sw;
                layout--;
            }
        }
        nLines = layout->nLines;
        dy = layout->spacing * sw + sw;
        vPadding = layout->padding * sw;
    } else {
        dy = h / float(nLines + 1);
        vPadding = dy - 0.5f * sw;
    }

    auto path = dc.createBezierPath();
    path->moveTo(Point(r.x + pointXInset, r.y + bottom - pointHeight));
    path->lineTo(Point(r.x + pointXInset, r.y + bottom));
    path->lineTo(Point(r.x + pointXInset + pointWidth, r.y + bottom - pointHeight));
    path->lineTo(Point(r.maxX() - br, r.y + bottom - pointHeight));
    path->quarterEllipseTo(Point(r.maxX(), r.y + bottom - pointHeight),
                           Point(r.maxX(), r.y + bottom - pointHeight - br));
    path->lineTo(Point(r.maxX(), r.y + top + br));
    path->quarterEllipseTo(Point(r.maxX(), r.y + top),
                           Point(r.maxX() - br, r.y + top));
    path->lineTo(Point(r.x + br, r.y + top));
    path->quarterEllipseTo(Point(r.x, r.y + top),
                           Point(r.x, r.y + top + br));
    path->lineTo(Point(r.x, r.y + bottom - pointHeight - br));
    path->quarterEllipseTo(Point(r.x, r.y + bottom - pointHeight),
                           Point(r.x + br, r.y + bottom - pointHeight));
    path->close();
    dc.setStrokeJoinStyle(kJoinMiter);  // to get sharp point
    dc.drawPath(path, kPaintStroke);

    if (nLines >= 2) {
        auto y0 = r.y + top + sw + vPadding;
        path = dc.createBezierPath();
        for (int j = 0;  j < nLines;  ++j) {
            auto y = y0 + float(j) * dy;
            path->moveTo(Point(r.x + lineXInset, y));
            path->lineTo(Point(r.maxX() - lineXInset, y));
        }
        dc.setStrokeJoinStyle(kJoinRound);
        dc.drawPath(path, kPaintStroke);
    }
}

void StandardIconPainter::drawConversation(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto sw = setStroke(dc, size, fg);
    auto r = calcContentRect(size);
    r.inset(0.5f * sw, 0.5f * sw);

    auto br = 0.1f * r.width;
    auto bubbleWidth = dc.roundToNearestPixel((0.65f * r.width));
    auto bubbleHeight = dc.roundToNearestPixel((0.7f * r.height));
    auto topLeft = r.y;
    auto topRight = r.maxY() - bubbleHeight;
    auto pointHeight = dc.roundToNearestPixel(0.2f * r.height);
    auto pointWidth = 1.0f * pointHeight;
    auto pointXInset = dc.roundToNearestPixel(0.15f * r.width);
    Rect bubbleRect;

    auto path = dc.createBezierPath();
    // left bubble
    bubbleRect = Rect(r.x, r.y + topLeft, bubbleWidth, bubbleHeight);
    path->moveTo(Point(bubbleRect.x + pointXInset, bubbleRect.maxY() - pointHeight));
    path->lineTo(Point(bubbleRect.x + pointXInset, bubbleRect.maxY()));
    //path->lineTo(Point(bubbleRect.x + pointXInset + pointWidth, bubbleRect.maxY() - pointHeight));
    path->lineTo(Point(r.maxX() - bubbleWidth, bubbleRect.maxY() - pointHeight));
    //path->lineTo(Point(bubbleRect.maxX() - br, bubbleRect.maxY() - pointHeight));
    //path->quarterEllipseTo(Point(bubbleRect.maxX(), bubbleRect.maxY() - pointHeight),
    //                       Point(bubbleRect.maxX(), bubbleRect.maxY() - pointHeight - br));
    path->moveTo(Point(bubbleRect.maxX(), r.y + topRight));
    path->lineTo(Point(bubbleRect.maxX(), bubbleRect.y + br));
    path->quarterEllipseTo(Point(bubbleRect.maxX(), bubbleRect.y),
                           Point(bubbleRect.maxX() - br, bubbleRect.y));
    path->lineTo(Point(bubbleRect.x + br, bubbleRect.y));
    path->quarterEllipseTo(Point(bubbleRect.x, bubbleRect.y),
                           Point(bubbleRect.x, bubbleRect.y + br));
    path->lineTo(Point(bubbleRect.x, bubbleRect.maxY() - pointHeight - br));
    path->quarterEllipseTo(Point(bubbleRect.x, bubbleRect.maxY() - pointHeight),
                           Point(bubbleRect.x + br, bubbleRect.maxY() - pointHeight));
    //path->close();
    path->lineTo(Point(bubbleRect.x + pointXInset, bubbleRect.maxY() - pointHeight));

    // right bubble
    bubbleRect = Rect(r.maxX() - bubbleWidth, r.y + topRight, bubbleWidth, bubbleHeight);
    path->moveTo(Point(bubbleRect.maxX() - pointXInset - pointWidth, bubbleRect.maxY() - pointHeight));
    path->lineTo(Point(bubbleRect.maxX() - pointXInset, bubbleRect.maxY()));
    path->lineTo(Point(bubbleRect.maxX() - pointXInset, bubbleRect.maxY() - pointHeight));
    path->lineTo(Point(bubbleRect.maxX() - br, bubbleRect.maxY() - pointHeight));
    path->quarterEllipseTo(Point(r.maxX(), bubbleRect.maxY() - pointHeight),
                           Point(r.maxX(), bubbleRect.maxY() - pointHeight - br));
    path->lineTo(Point(bubbleRect.maxX(), bubbleRect.y + br));
    path->quarterEllipseTo(Point(bubbleRect.maxX(), bubbleRect.y),
                           Point(bubbleRect.maxX() - br, bubbleRect.y));
    path->lineTo(Point(bubbleRect.x + br, bubbleRect.y));
    path->quarterEllipseTo(Point(bubbleRect.x, bubbleRect.y),
                           Point(bubbleRect.x, bubbleRect.y + br));
    path->lineTo(Point(bubbleRect.x, bubbleRect.maxY() - pointHeight - br));
    path->quarterEllipseTo(Point(bubbleRect.x, bubbleRect.maxY() - pointHeight),
                           Point(bubbleRect.x + br, bubbleRect.maxY() - pointHeight));
    path->close();

    dc.setStrokeJoinStyle(kJoinMiter);  // to get sharp point
    dc.drawPath(path, kPaintStroke);
}

PicaPt StandardIconPainter::setStroke(DrawContext& dc, const Size& size, const Color& fg) const
{
    auto w = std::max(dc.onePixel(), dc.roundToNearestPixel(size.height / 16.0f));
    dc.setFillColor(fg);
    dc.setStrokeColor(fg);
    dc.setStrokeWidth(w);
    dc.setStrokeEndCap(kEndCapRound);
    return w;
}

Rect StandardIconPainter::calcContentRect(const Size& size) const
{
    if (size.width == size.height) {
        return Rect(PicaPt::kZero, PicaPt::kZero, size.width, size.height);
    } else if (size.width > size.height) {
        return Rect(0.5f * (size.width - size.height), PicaPt::kZero, size.height, size.height);
    } else {
        return Rect(PicaPt::kZero, 0.5f * (size.width - size.height), size.width, size.height);
    }
}

PicaPt StandardIconPainter::calcBorderRadius(const Rect& contentRect) const
{
    return 0.05f * contentRect.height;  // height is more consistent between icons than width
}

Rect StandardIconPainter::strokeCircle(DrawContext& dc, const Rect& r, const PicaPt& strokeWidth) const
{
    auto p = dc.createBezierPath();
    p->addEllipse(r.insetted(0.5f * strokeWidth, 0.5f * strokeWidth));
    dc.drawPath(p, kPaintStroke);

    PicaPt delta = 0.25f * r.width;  // don't round to pixel, will not be centered on small icons
    return r.insetted(delta, delta);
}

std::shared_ptr<BezierPath> StandardIconPainter::clipRectForSlash(DrawContext& dc, const Rect& r,
                                                                  const PicaPt& strokeWidth) const
{
    auto hyp = 4.0f * 0.707f * strokeWidth;  // TODO: 3.0f is better for very small sizes
    auto clipPath = dc.createBezierPath();
    clipPath->moveTo(Point(r.x, r.y + hyp));
    clipPath->lineTo(r.lowerLeft());
    clipPath->lineTo(Point(r.maxX() - hyp, r.maxY()));
    clipPath->close();
    clipPath->moveTo(Point(r.x + hyp, r.y));
    clipPath->lineTo(r.upperRight());
    clipPath->lineTo(Point(r.maxX(), r.maxY() - hyp));
    clipPath->close();
    dc.clipToPath(clipPath);
    return clipPath;
}

void StandardIconPainter::drawSlash(DrawContext& dc, const Rect& r, const PicaPt& strokeWidth) const
{
    auto halfStroke = Point(0.5 * strokeWidth, 0.5 * strokeWidth);
    dc.drawLines({ r.upperLeft() + halfStroke, r.lowerRight() - halfStroke });
}

void StandardIconPainter::drawChevron(DrawContext& dc, const Rect& r, const PicaPt& strokeWidth,
                                      float angleDeg) const
{
    // 0 deg faces left
    auto center = r.center();
    auto half = 0.5f * (r.height - strokeWidth);
    std::vector<Point> pts = {
        Point(center.x + 0.5f * half - 0.5f * strokeWidth, center.y - half),
        Point(center.x - 0.5f * half - 0.5f * strokeWidth, center.y),
        Point(center.x + 0.5f * half - 0.5f * strokeWidth, center.y + half)
    };
    // Q: Why rotate by hand, instead of translating and rotated the dc?
    // A: I worry that rotate(angle), rotate(-angle) may result in slight floating
    //    point error, which is rather paranoid, but would be bad for pixel precision.
    //    We could save() and restore(), but that copies a lot of data.
    if (angleDeg != 0.0f) {
        rotatePoints(&pts, center, angleDeg);
    }
    dc.drawLines(pts);
}

void StandardIconPainter::drawTriangle(DrawContext& dc, const Rect& r, float angleDeg) const
{
    // 0 deg faces left
    auto center = r.center();
    auto horiz = 0.5f * r.width;
    // Without an offset the triangle is a little too left to be visually centered.
    auto offset = 0.075f * r.width;
    std::vector<Point> pts = {
        Point(center.x - 0.5f * horiz - offset, center.y),
        Point(center.x + 0.5f * horiz - offset, center.y - 0.866025f * horiz),
        Point(center.x + 0.5f * horiz - offset, center.y + 0.866025f * horiz)
    };
    // Q: Why rotate by hand, instead of translating and rotated the dc?
    // A: See drawChevron(). Since this is a twisty, there are probably more of them
    //    on the screen, too.
    if (angleDeg != 0.0f) {
        rotatePoints(&pts, center, angleDeg);
    }
    auto path = dc.createBezierPath();
    path->moveTo(pts[0]);
    path->lineTo(pts[1]);
    path->lineTo(pts[2]);
    path->close();
    dc.drawPath(path, kPaintFill);
}

void StandardIconPainter::drawArrow(DrawContext& dc, const Rect& r, float angleDeg,
                                    const PicaPt& strokeWidth) const
{
    // 0 deg faces left
    auto center = r.center();
    auto horiz = 0.4f * r.height;
    std::vector<Point> pts1 = {
        Point(r.x + 0.5f * strokeWidth + horiz, center.y - horiz),
        Point(r.x + 0.5f * strokeWidth,         center.y),
        Point(r.x + 0.5f * strokeWidth + horiz, center.y + horiz)
    };
    std::vector<Point> pts2 = {
        Point(r.x + 0.6f * strokeWidth, center.y),  // 0.6: a little extra to avoid overwrites
        Point(r.maxX() - 0.5f * strokeWidth, center.y)
    };
    if (angleDeg != 0.0f) {
        rotatePoints(&pts1, center, angleDeg);
        rotatePoints(&pts2, center, angleDeg);
    }
    auto path = dc.createBezierPath();
    path->moveTo(pts1[0]);
    path->lineTo(pts1[1]);
    path->lineTo(pts1[2]);
    path->moveTo(pts2[0]);
    path->lineTo(pts2[1]);
    dc.drawPath(path, kPaintStroke);
}

void StandardIconPainter::drawExclamationPoint(DrawContext& dc, const Rect& r,
                                               const PicaPt& strokeWidth) const
{
    std::vector<Point> pts = { Point(r.midX(), r.y), Point(r.midX(), r.y + 0.666f * r.height) };
    dc.drawLines(pts);
    auto radius = 1.0f * strokeWidth;
    dc.drawEllipse(Rect(r.midX() - radius, r.maxY() - radius, 2.0f * radius, 2.0f * radius), kPaintFill);
}

void StandardIconPainter::drawPlusOrMinus(DrawContext& dc, const Rect& r, const PicaPt& strokeWidth,
                                          PlusMinusGlyph glyph) const
{
    auto halfStroke = PicaPt::kZero;
    if (strokeWidth <= 1.5f * dc.onePixel()) {
        dc.setStrokeEndCap(kEndCapButt);
    } else {
        halfStroke = 0.5f * strokeWidth;
    }
    auto c = r.center();

    auto onePx = dc.onePixel();
    int strokePx = int(std::round(strokeWidth / onePx));
    int sizePx = int(std::round(r.height / onePx));
    auto path = dc.createBezierPath();
    if (strokePx & 0x1) {  // odd stroke width; need odd size so height/2 is in middle of pixel
        if ((sizePx & 0x1) == 0) {
            sizePx -= 1;
        }
    } else {  // even stroke width; need even size so height/2 is in-between pixels
        if ((sizePx & 0x1) == 1) {
            sizePx -= 1;
        }
    }

    path->moveTo(Point(r.x + halfStroke, c.y));
    path->lineTo(Point(r.maxX() - halfStroke, c.y));

    if (glyph == kPlus) {  // (sizePx is already correct, and we assume w == h)
        path->moveTo(Point(c.x, r.y + halfStroke));
        path->lineTo(Point(c.x, r.maxY() - halfStroke));
    }

    dc.drawPath(path, kPaintStroke);
}

Rect StandardIconPainter::drawMagnifyingGlass(DrawContext& dc, const Rect& r, const PicaPt& strokeWidth) const
{
    auto glassSize = dc.roundToNearestPixel(0.8f * r.height);

    // Make sure that the iconRect is always able to center a line (since + and - are the
    // most common items to draw inside)
    auto onePx = dc.onePixel();
    auto iconSize = std::min(glassSize - 6.0f * strokeWidth,
                             dc.roundToNearestPixel(0.5f * glassSize));
    iconSize = std::max(3.0f * strokeWidth, iconSize);
    int strokePx = int(std::round(strokeWidth / onePx));
    int iconPx = int(std::round(iconSize / onePx));
    if (strokePx & 0x1) {  // stroke is odd:  iconPx needs to be odd
        if ((iconPx & 0x1) == 0) {
            iconSize += onePx;
            glassSize += onePx;
        }
    } else {  // stroke is even:  iconPx needs to be even
        if ((iconPx & 0x1) == 1) {
            iconSize += onePx;
            glassSize += onePx;
        }
    }
    auto glassRect = Rect(r.x, r.y, glassSize, glassSize);
    strokeCircle(dc, glassRect, strokeWidth);  // use this instead of dc.drawEllipse because we want to draw within the rect
    // Ignore strokeCircle's returned rect, it is very important that the icon rect is
    // properly sized, otherwise the lines aren't centered for small icon sizes.
    Rect iconRect(r.x + 0.5f * (glassSize - iconSize), r.y + 0.5f * (glassSize - iconSize),
                  iconSize, iconSize);

    auto toCircle = 0.707f * 0.5f * glassRect.width;
    std::vector<Point> handle = {
        Point(glassRect.midX() + toCircle, glassRect.midY() + toCircle),
        Point(r.maxX() - 0.5f * strokeWidth, r.maxY() - 0.5f * strokeWidth)
    };
    dc.drawLines(handle);

    return iconRect;
}

void StandardIconPainter::drawLock(DrawContext& dc, const Rect& r, const PicaPt& strokeWidth,
                                   LockGlyph glyph) const
{
    const auto lockX = dc.roundToNearestPixel(0.1f * r.width);
    const auto lockHeight = dc.roundToNearestPixel(0.5f * r.height);
    auto center = r.center();
    auto boltRadius = 0.5f * 0.666f * (r.width - 2.0f * lockX);
    auto x1 = dc.offsetPixelForStroke(dc.roundToNearestPixel(center.x - boltRadius), strokeWidth);
    boltRadius = center.x - x1;
    auto top = r.y + 0.5f * strokeWidth;

    auto path = dc.createBezierPath();
    path->addRoundedRect(Rect(lockX, r.maxY() - lockHeight, r.width - 2.0f * lockX, lockHeight),
                         0.1f * r.height);
    dc.drawPath(path, kPaintFill);
    // we need to clip the bottom, because the rounded ends will overlap with
    // the lock, and the fg is not necessary opaque.
    dc.save();
    dc.clipToRect(Rect(r.x, r.y, r.width, r.maxY() - lockHeight));
    path = dc.createBezierPath();
    if (glyph == kUnlocked) {
        path->moveTo(Point(x1, top + boltRadius));
    } else {
        path->moveTo(Point(x1, r.maxY() - lockHeight));
        path->lineTo(Point(x1, top + boltRadius));
    }
    path->quarterEllipseTo(Point(x1, top), Point(x1 + boltRadius, top));
    path->quarterEllipseTo(Point(x1 + 2.0f * boltRadius, top),
                           Point(x1 + 2.0f * boltRadius, top + boltRadius));
    path->lineTo(Point(x1 + 2.0f * boltRadius, r.maxY() - lockHeight));
    dc.drawPath(path, kPaintStroke);
    dc.restore();
}

void StandardIconPainter::drawAlignedLines(DrawContext& dc, const Rect& r, const PicaPt& strokeWidth,
                                           int alignment) const
{
    static float offsets3[] = { 0.0f, 0.4f, 0.2f, 1.0f };
    static float offsets4[] = { 0.0f, 0.4f, 0.2f, 0.0f };
    static float offsets5[] = { 0.0f, 0.4f, 0.2f, 0.4f, 0.0f };
    static float offsets6[] = { 0.0f, 0.4f, 0.2f, 0.2f, 0.4f, 0.0f };
    static float justify6[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    int align = (alignment & Alignment::kHorizMask);
    auto naturalNLines = int(r.height.asFloat() / (2.0f * strokeWidth.asFloat()));
    auto nLines = std::max(3, std::min(5, naturalNLines));
    auto dy = dc.floorToNearestPixel(r.height / float(nLines - 1));
    if (float(nLines - 1) * dy > r.height - strokeWidth) {
        dy -= dc.onePixel();
    }

    float *offsets = offsets3;
    if (nLines == 4) {
        offsets = offsets4;
    } else if (nLines == 5) {
        offsets = offsets5;
    } else if (nLines == 6) {
        offsets = offsets6;
    }
    if (align == Alignment::kJustify) {
        offsets = justify6;
    }
    auto y = r.x + 0.5 * strokeWidth;
    for (int i = 0;  i < nLines;  ++i) {
        PicaPt x1, x2;
        switch (align) {
            default:
            case Alignment::kLeft:
                x1 = r.x;
                x2 = r.maxX() - offsets[i] * r.width;
                break;
            case Alignment::kHCenter:
                x1 = r.x + 0.5f * offsets[i] * r.width;
                x2 = r.maxX() - 0.5f * offsets[i] * r.width;
                break;
            case Alignment::kRight:
                x1 = r.x + offsets[i] * r.width;
                x2 = r.maxX();
                break;
        }
        dc.drawLines({ Point(x1 + 0.5f * strokeWidth, y), Point(x2 - 0.5f * strokeWidth, y) });
        y += dy;
    }
}

void StandardIconPainter::drawList(DrawContext& dc, const Rect& r, const PicaPt& strokeWidth,
                                   const PicaPt& lineIndent,
                                   std::function<void(int, DrawContext&, const PicaPt&)> drawBullet) const
{
/*    int n = 3;
    auto dy = (r.height - 2.0f * strokeWidth) / float(n - 1);
    float px = dy.asFloat() / dc.onePixel().asFloat();
    if (std::abs(px - std::floor(px)) > 0.001f) {  // must be half a pixel
        n = 4;
        dy = (r.height - 2.0f * strokeWidth) / float(n - 1);
    }

    auto y = r.x + strokeWidth;
    for (int i = 0;  i < n;  ++i) {
        drawBullet(i, dc, y);
        dc.drawLines({ Point(r.x + lineIndent, y), Point(r.maxX(), y) });
        y += dy;
    }
 */
    int n = 3;
    auto h = r.height;
    auto dy = dc.floorToNearestPixel(h / float(n));
    auto vPadding = dc.ceilToNearestPixel(0.5f * dy);  // err towards extra space on top
    auto extraPixels = (h - float(n) * dy) / dc.onePixel();
    if (extraPixels > 1.998f) {
        vPadding += dc.onePixel();
    }

    auto y = r.y + vPadding;
    for (int i = 0;  i < n;  ++i) {
        drawBullet(i, dc, y);
        dc.drawLines({ Point(r.x + lineIndent, y), Point(r.maxX(), y) });
        y += dy;
    }
}

void StandardIconPainter::drawVolume(DrawContext& dc, const Rect& r, const Color& fg,
                                     const PicaPt& strokeWidth, float volume) const
{
    bool isMute = (volume == 0.0f);
    auto w = r.width - strokeWidth;
    auto speakerWidth = dc.roundToNearestPixel(0.4f * w);
    auto speakerInset = dc.roundToNearestPixel(0.25f * r.height);
    auto debug = speakerInset / (r.height - strokeWidth);
    auto x0 = r.x + 0.5f * strokeWidth;
    auto x1 = x0 + 0.5f * speakerWidth;  // does not need to be aligned
    auto x2 = x0 + speakerWidth;  // this is a vertical line, needs to be aligned
    auto y1 = r.y + 0.5f * strokeWidth + speakerInset;
    auto y2 = r.maxY() - 0.5f * strokeWidth - speakerInset;
    auto slashWidth = strokeWidth;

    if (isMute) {
        dc.save();
        dc.clipToPath(clipRectForSlash(dc, r, strokeWidth));
    }

    auto path = dc.createBezierPath();
    path->moveTo(Point(x0, y1));
    path->lineTo(Point(x1, y1));
    path->lineTo(Point(x2, y1 - (x2 - x1)));
    path->lineTo(Point(x2, y2 + (x2 - x1)));
    path->lineTo(Point(x1, y2));
    path->lineTo(Point(x0, y2));
    path->close();
    dc.setFillColor(fg);
    dc.drawPath(path, kPaintFill);

    int n = 3;
    auto dx = (w - speakerWidth) / float(n);
    auto yMid = r.midY();
    // Calculate max radius using chord length = 2 * r * sin(theta/2).
    // Theta is 90 deg => r.height = 2 * r * 0.707 (not accounting for stroke width of end caps)
    auto chordLen = r.height - strokeWidth;
    auto maxRadius = 0.70710678f * chordLen;
    auto cx = r.maxX() - 0.5f * strokeWidth - maxRadius;

    for (int i = 0;  i < 3;  ++i) {
        auto radius = maxRadius - float(n - 1 - i) * dx;
        auto radiusOverSqrt2 = 0.70710678f * radius;
        path = dc.createBezierPath();
        path->moveTo(Point(cx + radiusOverSqrt2, yMid - radiusOverSqrt2));
        // Note that the first argument is a *control point*, which means that the curve
        // does NOT go through it!  'x = cx + radiusOverSqrt2' is also NOT a right angle
        path->quarterEllipseTo(Point(cx + 2.0f * radiusOverSqrt2, yMid), Point(cx + radiusOverSqrt2, yMid + radiusOverSqrt2));

        if (isMute || volume > float(i) / float(n)) {
            dc.setStrokeColor(fg);
        } else {
            dc.setStrokeColor(Color(fg, 0.333f * fg.alpha()));
        }
        dc.drawPath(path, kPaintStroke);
    }

    if (isMute) {
        dc.restore();
        drawSlash(dc, r, strokeWidth);
    }
}

void StandardIconPainter::rotatePoints(std::vector<Point> *pts, const Point& center, float angleDeg) const
{
    // Note that +y is down, so +angle rotates clockwise instead
    // of counterclockwise like normal, which is confusing
    float sinT = std::sin(-angleDeg * float(M_PI) / 180.0f);
    float cosT = std::cos(-angleDeg * float(M_PI) / 180.0f);
    for (auto &p : *pts) {
        auto x = p.x - center.x;
        auto y = p.y - center.y;
        p.x = cosT * x - sinT * y + center.x;
        p.y = sinT * x + cosT * y + center.y;
    }
}

} // namespace uitk
