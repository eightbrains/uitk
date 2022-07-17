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

#ifndef UITK_ICON_PAINTER_H
#define UITK_ICON_PAINTER_H

namespace uitk {

class Color;
class DrawContext;
struct Size;

class IconPainter
{
public:
    virtual ~IconPainter() {}

    virtual void drawX(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawXCircle(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawPrevScreen(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawNextScreen(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawTwistyClosed(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawTwistyOpen(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawError(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawWarning(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawInfo(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawHelp(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawSearch(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawHistory(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawMenu(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawAdd(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawRemove(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawAddCircle(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawRemoveCircle(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawExpand(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawContract(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawMoreHoriz(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawMoreVert(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawLocked(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawUnlocked(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawSettings(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawChevronLeft(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawChevronRight(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawChevronUp(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawChevronDown(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawChevronLeftCircle(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawChevronRightCircle(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawChevronUpCircle(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawChevronDownCircle(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawTriangleLeft(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawTriangleRight(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawTriangleUp(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawTriangleDown(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawTriangleLeftCircle(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawTriangleRightCircle(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawTriangleUpCircle(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawTriangleDownCircle(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawRefresh(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawArrowLeft(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawArrowRight(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawArrowUp(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawArrowDown(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawArrowLeftCircle(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawArrowRightCircle(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawArrowUpCircle(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawArrowDownCircle(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawMacCmd(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawMacShift(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawMacOption(DrawContext& dc, const Size& size, const Color& fg) const = 0;

    virtual void drawNewFile(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawOpenFile(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawSaveFile(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawPrint(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawExport(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawExternal(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawBoldStyle(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawItalicStyle(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawUnderlineStyle(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawAlignLeft(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawAlignCenter(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawAlignRight(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawAlignJustify(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawBulletList(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawNumericList(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawPlay(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawPause(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawStop(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawFastForward(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawFastReverse(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawSkipForward(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawSkipBackward(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawShuffle(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawLoop(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawVolumeMute(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawVolumeSoft(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawVolumeMedium(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawVolumeLoud(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawZoomIn(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawZoomOut(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawRecordAudio(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawRecordVideo(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawNoAudio(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawNoVideo(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawCamera(DrawContext& dc, const Size& size, const Color& fg) const = 0;

    virtual void drawFolder(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawFile(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawTrash(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawHome(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawPicture(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawDocument(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawEdit(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawUser(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawColor(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawStar(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawHeart(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawMail(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawAttachment(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawCalendar(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawChat(DrawContext& dc, const Size& size, const Color& fg) const = 0;
    virtual void drawConversation(DrawContext& dc, const Size& size, const Color& fg) const = 0;
};

}  // namespace uitk
#endif // UITK_ICON_PAINTER_H
