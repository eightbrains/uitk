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

#include "Theme.h"

#include "IconPainter.h"
#include "../Application.h"
#include "../UIContext.h"

namespace uitk {

Theme::WidgetStyle Theme::WidgetStyle::merge(const WidgetStyle& s) const
{
    WidgetStyle newStyle;

    if (s.flags & kBGColorSet) {
        newStyle.bgColor = s.bgColor;
    } else {
        newStyle.bgColor = this->bgColor;
    }

    if (s.flags & kFGColorSet) {
        newStyle.fgColor = s.fgColor;
    } else {
        newStyle.fgColor = this->fgColor;
    }

    if (s.flags & kBorderColorSet) {
        newStyle.borderColor = s.borderColor;
    } else {
        newStyle.borderColor = this->borderColor;
    }

    if (s.flags & kBorderWidthSet) {
        newStyle.borderWidth = s.borderWidth;
    } else {
        newStyle.borderWidth = this->borderWidth;
    }

    if (s.flags & kBorderRadiusSet) {
        newStyle.borderRadius = s.borderRadius;
    } else {
        newStyle.borderRadius = this->borderRadius;
    }

    return newStyle;
}

void Theme::drawIcon(UIContext& ui, const Rect& r, const Icon& icon, const Color& color) const
{
    ui.dc.save();
    icon(ui.dc, ui.theme, r, color);
    ui.dc.restore();
}

void Theme::drawIcon(UIContext& ui, const Rect& r, StandardIcon icon, const Color& color) const
{
    ui.dc.save();
    ui.dc.translate(r.x, r.y);
    auto painter = Application::instance().iconPainter();

    switch (icon) {
        case StandardIcon::kNone:
            break;
        case StandardIcon::kEmpty:
            painter->drawEmpty(ui.dc, r.size(), color);
            break;
        case StandardIcon::kCloseX:
            painter->drawX(ui.dc, r.size(), color);
            break;
        case StandardIcon::kCloseXCircle:
            painter->drawXCircle(ui.dc, r.size(), color);
            break;
        case StandardIcon::kPrevScreen:
            painter->drawPrevScreen(ui.dc, r.size(), color);
            break;
        case StandardIcon::kNextScreen:
            painter->drawNextScreen(ui.dc, r.size(), color);
            break;
        case StandardIcon::kTwistyClosed:
            painter->drawTwistyClosed(ui.dc, r.size(), color);
            break;
        case StandardIcon::kTwistyOpen:
            painter->drawTwistyOpen(ui.dc, r.size(), color);
            break;
        case StandardIcon::kError:
            painter->drawError(ui.dc, r.size(), color);
            break;
        case StandardIcon::kWarning:
            painter->drawWarning(ui.dc, r.size(), color);
            break;
        case StandardIcon::kInfo:
            painter->drawInfo(ui.dc, r.size(), color);
            break;
        case StandardIcon::kHelp:
            painter->drawHelp(ui.dc, r.size(), color);
            break;
        case StandardIcon::kSearch:
            painter->drawSearch(ui.dc, r.size(), color);
            break;
        case StandardIcon::kHistory:
            painter->drawHistory(ui.dc, r.size(), color);
            break;
        case StandardIcon::kMenu:
            painter->drawMenu(ui.dc, r.size(), color);
            break;
        case StandardIcon::kCheckmark:
            painter->drawCheckmark(ui.dc, r.size(), color);
            break;
        case StandardIcon::kAdd:
            painter->drawAdd(ui.dc, r.size(), color);
            break;
        case StandardIcon::kRemove:
            painter->drawRemove(ui.dc, r.size(), color);
            break;
        case StandardIcon::kAddCircle:
            painter->drawAddCircle(ui.dc, r.size(), color);
            break;
        case StandardIcon::kRemoveCircle:
            painter->drawRemoveCircle(ui.dc, r.size(), color);
            break;
        case StandardIcon::kExpand:
            painter->drawExpand(ui.dc, r.size(), color);
            break;
        case StandardIcon::kContract:
            painter->drawContract(ui.dc, r.size(), color);
            break;
        case StandardIcon::kMoreHoriz:
            painter->drawMoreHoriz(ui.dc, r.size(), color);
            break;
        case StandardIcon::kMoreVert:
            painter->drawMoreVert(ui.dc, r.size(), color);
            break;
        case StandardIcon::kLocked:
            painter->drawLocked(ui.dc, r.size(), color);
            break;
        case StandardIcon::kUnlocked:
            painter->drawUnlocked(ui.dc, r.size(), color);
            break;
        case StandardIcon::kSettings:
            painter->drawSettings(ui.dc, r.size(), color);
            break;
        case StandardIcon::kChevronLeft:
            painter->drawChevronLeft(ui.dc, r.size(), color);
            break;
        case StandardIcon::kChevronRight:
            painter->drawChevronRight(ui.dc, r.size(), color);
            break;
        case StandardIcon::kChevronUp:
            painter->drawChevronUp(ui.dc, r.size(), color);
            break;
        case StandardIcon::kChevronDown:
            painter->drawChevronDown(ui.dc, r.size(), color);
            break;
        case StandardIcon::kChevronLeftCircle:
            painter->drawChevronLeftCircle(ui.dc, r.size(), color);
            break;
        case StandardIcon::kChevronRightCircle:
            painter->drawChevronRightCircle(ui.dc, r.size(), color);
            break;
        case StandardIcon::kChevronUpCircle:
            painter->drawChevronUpCircle(ui.dc, r.size(), color);
            break;
        case StandardIcon::kChevronDownCircle:
            painter->drawChevronDownCircle(ui.dc, r.size(), color);
            break;
        case StandardIcon::kTriangleLeft:
            painter->drawTriangleLeft(ui.dc, r.size(), color);
            break;
        case StandardIcon::kTriangleRight:
            painter->drawTriangleRight(ui.dc, r.size(), color);
            break;
        case StandardIcon::kTriangleUp:
            painter->drawTriangleUp(ui.dc, r.size(), color);
            break;
        case StandardIcon::kTriangleDown:
            painter->drawTriangleDown(ui.dc, r.size(), color);
            break;
        case StandardIcon::kTriangleLeftCircle:
            painter->drawTriangleLeftCircle(ui.dc, r.size(), color);
            break;
        case StandardIcon::kTriangleRightCircle:
            painter->drawTriangleRightCircle(ui.dc, r.size(), color);
            break;
        case StandardIcon::kTriangleUpCircle:
            painter->drawTriangleUpCircle(ui.dc, r.size(), color);
            break;
        case StandardIcon::kTriangleDownCircle:
            painter->drawTriangleDownCircle(ui.dc, r.size(), color);
            break;
        case StandardIcon::kRefresh:
            painter->drawRefresh(ui.dc, r.size(), color);
            break;
        case StandardIcon::kArrowLeft:
            painter->drawArrowLeft(ui.dc, r.size(), color);
            break;
        case StandardIcon::kArrowRight:
            painter->drawArrowRight(ui.dc, r.size(), color);
            break;
        case StandardIcon::kArrowUp:
            painter->drawArrowUp(ui.dc, r.size(), color);
            break;
        case StandardIcon::kArrowDown:
            painter->drawArrowDown(ui.dc, r.size(), color);
            break;
        case StandardIcon::kArrowLeftCircle:
            painter->drawArrowLeftCircle(ui.dc, r.size(), color);
            break;
        case StandardIcon::kArrowRightCircle:
            painter->drawArrowRightCircle(ui.dc, r.size(), color);
            break;
        case StandardIcon::kArrowUpCircle:
            painter->drawArrowUpCircle(ui.dc, r.size(), color);
            break;
        case StandardIcon::kArrowDownCircle:
            painter->drawArrowDownCircle(ui.dc, r.size(), color);
            break;
        case StandardIcon::kMacCmd:
            painter->drawMacCmd(ui.dc, r.size(), color);
            break;
        case StandardIcon::kMacShift:
            painter->drawMacShift(ui.dc, r.size(), color);
            break;
        case StandardIcon::kMacOption:
            painter->drawMacOption(ui.dc, r.size(), color);
            break;
        case StandardIcon::kNewFile:
            painter->drawNewFile(ui.dc, r.size(), color);
            break;
        case StandardIcon::kOpenFile:
            painter->drawOpenFile(ui.dc, r.size(), color);
            break;
        case StandardIcon::kSaveFile:
            painter->drawSaveFile(ui.dc, r.size(), color);
            break;
        case StandardIcon::kPrint:
            painter->drawPrint(ui.dc, r.size(), color);
            break;
        case StandardIcon::kExport:
            painter->drawExport(ui.dc, r.size(), color);
            break;
        case StandardIcon::kExternal:
            painter->drawExternal(ui.dc, r.size(), color);
            break;
        case StandardIcon::kBoldStyle:
            painter->drawBoldStyle(ui.dc, r.size(), color);
            break;
        case StandardIcon::kItalicStyle:
            painter->drawItalicStyle(ui.dc, r.size(), color);
            break;
        case StandardIcon::kUnderlineStyle:
            painter->drawUnderlineStyle(ui.dc, r.size(), color);
            break;
        case StandardIcon::kAlignLeft:
            painter->drawAlignLeft(ui.dc, r.size(), color);
            break;
        case StandardIcon::kAlignCenter:
            painter->drawAlignCenter(ui.dc, r.size(), color);
            break;
        case StandardIcon::kAlignRight:
            painter->drawAlignRight(ui.dc, r.size(), color);
            break;
        case StandardIcon::kAlignJustify:
            painter->drawAlignJustify(ui.dc, r.size(), color);
            break;
        case StandardIcon::kBulletList:
            painter->drawBulletList(ui.dc, r.size(), color);
            break;
        case StandardIcon::kNumericList:
            painter->drawNumericList(ui.dc, r.size(), color);
            break;
        case StandardIcon::kPlay:
            painter->drawPlay(ui.dc, r.size(), color);
            break;
        case StandardIcon::kPause:
            painter->drawPause(ui.dc, r.size(), color);
            break;
        case StandardIcon::kStop:
            painter->drawStop(ui.dc, r.size(), color);
            break;
        case StandardIcon::kFastForward:
            painter->drawFastForward(ui.dc, r.size(), color);
            break;
        case StandardIcon::kFastReverse:
            painter->drawFastReverse(ui.dc, r.size(), color);
            break;
        case StandardIcon::kSkipForward:
            painter->drawSkipForward(ui.dc, r.size(), color);
            break;
        case StandardIcon::kSkipBackward:
            painter->drawSkipBackward(ui.dc, r.size(), color);
            break;
        case StandardIcon::kShuffle:
            painter->drawShuffle(ui.dc, r.size(), color);
            break;
        case StandardIcon::kLoop:
            painter->drawLoop(ui.dc, r.size(), color);
            break;
        case StandardIcon::kVolumeMute:
            painter->drawVolumeMute(ui.dc, r.size(), color);
            break;
        case StandardIcon::kVolumeSoft:
            painter->drawVolumeSoft(ui.dc, r.size(), color);
            break;
        case StandardIcon::kVolumeMedium:
            painter->drawVolumeMedium(ui.dc, r.size(), color);
            break;
        case StandardIcon::kVolumeLoud:
            painter->drawVolumeLoud(ui.dc, r.size(), color);
            break;
        case StandardIcon::kZoomIn:
            painter->drawZoomIn(ui.dc, r.size(), color);
            break;
        case StandardIcon::kZoomOut:
            painter->drawZoomOut(ui.dc, r.size(), color);
            break;
        case StandardIcon::kRecordAudio:
            painter->drawRecordAudio(ui.dc, r.size(), color);
            break;
        case StandardIcon::kRecordVideo:
            painter->drawRecordVideo(ui.dc, r.size(), color);
            break;
        case StandardIcon::kNoAudio:
            painter->drawNoAudio(ui.dc, r.size(), color);
            break;
        case StandardIcon::kNoVideo:
            painter->drawNoVideo(ui.dc, r.size(), color);
            break;
        case StandardIcon::kCamera:
            painter->drawCamera(ui.dc, r.size(), color);
            break;
        case StandardIcon::kFolder:
            painter->drawFolder(ui.dc, r.size(), color);
            break;
        case StandardIcon::kFile:
            painter->drawFile(ui.dc, r.size(), color);
            break;
        case StandardIcon::kTrash:
            painter->drawTrash(ui.dc, r.size(), color);
            break;
        case StandardIcon::kEdit:
            painter->drawEdit(ui.dc, r.size(), color);
            break;
        case StandardIcon::kHome:
            painter->drawHome(ui.dc, r.size(), color);
            break;
        case StandardIcon::kPicture:
            painter->drawPicture(ui.dc, r.size(), color);
            break;
        case StandardIcon::kDocument:
            painter->drawDocument(ui.dc, r.size(), color);
            break;
        case StandardIcon::kUser:
            painter->drawUser(ui.dc, r.size(), color);
            break;
        case StandardIcon::kColor:
            painter->drawColor(ui.dc, r.size(), color);
            break;
        case StandardIcon::kStar:
            painter->drawStar(ui.dc, r.size(), color);
            break;
        case StandardIcon::kHeart:
            painter->drawHeart(ui.dc, r.size(), color);
            break;
        case StandardIcon::kMail:
            painter->drawMail(ui.dc, r.size(), color);
            break;
        case StandardIcon::kAttachment:
            painter->drawAttachment(ui.dc, r.size(), color);
            break;
        case StandardIcon::kCalendar:
            painter->drawCalendar(ui.dc, r.size(), color);
            break;
        case StandardIcon::kChat:
            painter->drawChat(ui.dc, r.size(), color);
            break;
        case StandardIcon::kConversation:
            painter->drawConversation(ui.dc, r.size(), color);
            break;
    }
    ui.dc.restore();
}

} // namespace uitk
