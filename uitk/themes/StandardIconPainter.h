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

#ifndef STANDARD_ICON_PAINTER_H
#define STANDARD_ICON_PAINTER_H

#include "IconPainter.h"

#include <nativedraw.h>

#include <vector>
#include <functional>

namespace uitk {

class StandardIconPainter : public IconPainter
{
public:
    void drawEmpty(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawX(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawXCircle(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawPrevScreen(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawNextScreen(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawTwistyClosed(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawTwistyOpen(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawError(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawWarning(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawInfo(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawHelp(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawSearch(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawHistory(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawMenu(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawCheckmark(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawAdd(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawRemove(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawAddCircle(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawRemoveCircle(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawExpand(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawContract(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawMoreHoriz(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawMoreVert(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawLocked(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawUnlocked(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawSettings(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawChevronLeft(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawChevronRight(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawChevronUp(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawChevronDown(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawChevronLeftCircle(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawChevronRightCircle(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawChevronUpCircle(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawChevronDownCircle(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawTriangleLeft(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawTriangleRight(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawTriangleUp(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawTriangleDown(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawTriangleLeftCircle(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawTriangleRightCircle(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawTriangleUpCircle(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawTriangleDownCircle(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawRefresh(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawArrowLeft(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawArrowRight(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawArrowUp(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawArrowDown(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawArrowLeftCircle(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawArrowRightCircle(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawArrowUpCircle(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawArrowDownCircle(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawMacCmd(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawMacShift(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawMacOption(DrawContext& dc, const Size& size, const Color& fg) const override;

    void drawNewFile(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawOpenFile(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawSaveFile(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawPrint(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawExport(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawExternal(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawBoldStyle(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawItalicStyle(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawUnderlineStyle(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawAlignLeft(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawAlignCenter(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawAlignRight(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawAlignJustify(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawBulletList(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawNumericList(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawPlay(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawPause(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawStop(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawFastForward(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawFastReverse(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawSkipForward(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawSkipBackward(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawShuffle(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawLoop(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawVolumeMute(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawVolumeSoft(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawVolumeMedium(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawVolumeLoud(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawZoomIn(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawZoomOut(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawRecordAudio(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawRecordVideo(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawNoAudio(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawNoVideo(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawCamera(DrawContext& dc, const Size& size, const Color& fg) const override;

    void drawFolder(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawFile(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawTrash(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawEdit(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawHome(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawPicture(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawDocument(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawUser(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawColor(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawStar(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawHeart(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawMail(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawAttachment(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawCalendar(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawChat(DrawContext& dc, const Size& size, const Color& fg) const override;
    void drawConversation(DrawContext& dc, const Size& size, const Color& fg) const override;

    /// Sets the stroke parameters and returns the stroke width (so that it can be
    /// used to properly locate lines). The stroke width will always be an integer
    /// number of hardware pixels. In addition to being used internally, this is
    /// useful for create custom icons with consistent values as the standard icons.
    virtual PicaPt setStroke(DrawContext& dc, const Size& size, const Color& fg) const;

protected:
    /// Calculates the content rect (always square and centered) given the size.
    Rect calcContentRect(const Size& size) const;
    /// Returns the border radius for rounded rects
    PicaPt calcBorderRadius(const Rect& contentRect) const;
    /// Strokes a circle and returns the content rect for a sub-icon within the circle.
    /// The stroke will be contained within the rect.
    Rect strokeCircle(DrawContext& dc, const Rect& r, const PicaPt& strokeWidth) const;
    /// Returns a clipping path to clip out the area where the slash will be drawn.
    virtual std::shared_ptr<BezierPath> clipRectForSlash(DrawContext& dc, const Rect& r,
                                                         const PicaPt& strokeWidth) const;
    /// Draws a slash (for no-... icons). For best results do:
    ///   dc.save();
    ///   dc.clipToPath(clipRectForSlash(dc, r, strokeWidth));
    ///   // ... draw icon ...
    ///   dc.restore();
    ///   drawSlash(dc, r, strokeWidth);
    void drawSlash(DrawContext& dc, const Rect& r, const PicaPt& strokeWidth) const;

    virtual void drawChevron(DrawContext& dc, const Rect& r, const PicaPt& strokeWidth, float angleDeg) const;
    virtual void drawTriangle(DrawContext& dc, const Rect& r, float angleDeg) const;
    virtual void drawArrow(DrawContext& dc, const Rect& r, float angleDeg, const PicaPt& strokeWidth) const;
    virtual void drawExclamationPoint(DrawContext& dc, const Rect& r, const PicaPt& strokeWidth) const;
    enum PlusMinusGlyph { kPlus, kMinus };
    virtual void drawPlusOrMinus(DrawContext& dc, const Rect& r, const PicaPt& strokeWidth,
                                 PlusMinusGlyph glyph) const;
    /// Returns the rectangle for drawing within the glass part
    enum GlassOptions { kAdjustForGlyphInGlass, kDontAdjust };
    virtual Rect drawMagnifyingGlass(DrawContext& dc, const Rect& r, const PicaPt& strokeWidth,
                                     GlassOptions opt = kAdjustForGlyphInGlass) const;
    enum LockGlyph { kLocked, kUnlocked };
    virtual void drawLock(DrawContext& dc, const Rect& r, const PicaPt& strokeWidth,
                          LockGlyph glyph) const;
    virtual void drawAlignedLines(DrawContext& dc, const Rect& r, const PicaPt& strokeWidth,
                                  int alignment) const;
    virtual void drawList(DrawContext& dc, const Rect& r, const PicaPt& strokeWidth,
                          const PicaPt& lineIndent,
                          std::function<void(int, DrawContext&, const PicaPt&)> drawBullet) const;
    /// Volume ranges from 0.0 - 1.0.
    virtual void drawVolume(DrawContext& dc, const Rect& r, const Color& fg, const PicaPt& strokeWidth,
                            float volume) const;

    /// +Angle is counterclockwise
    void rotatePoints(std::vector<Point> *pts, const Point& center, float angleDeg) const;
};

} // namespace uitk
#endif // STANDARD_ICON_PAINTER_H
