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

#include "Icon.h"

#include "Application.h"
#include "UIContext.h"
#include "themes/IconPainter.h"

namespace uitk {

struct Icon::Impl
{
    StandardIcon icon = StandardIcon::kNone;
    std::function<void(UIContext&)> drawFunc;
    Color fg = Color(0.0f, 0.0f, 0.0f, 0.0f);
};

Icon::Icon(StandardIcon icon)
    : mImpl(new Impl())
{
    mImpl->icon = icon;
}

Icon::Icon(std::function<void(UIContext&)> drawFunc)
    : mImpl(new Impl())
{
    mImpl->drawFunc = drawFunc;
}

void Icon::setColor(const Color& fg)
{
    mImpl->fg = fg;
}

const Color& Icon::color() const
{
    return mImpl->fg;
}

Size Icon::preferredSize(const LayoutContext& context) const
{
    auto em = context.theme.params().labelFont.pointSize();
    return Size(em, em);
}

void Icon::draw(UIContext& context)
{
    Super::draw(context);
    bool hasFrame = (borderWidth() > PicaPt::kZero && borderColor().alpha() > 0.0f);
    auto size = bounds().size();

    if (hasFrame) {
        size.width -= 2.0f * borderWidth();
        size.height -= 2.0f * borderWidth();
        context.dc.translate(borderWidth(), borderWidth());
    }

    auto painter = Application::instance().iconPainter();
    auto fg = context.theme.params().textColor;
    if (mImpl->fg.alpha() > 0.0f) {
        fg = mImpl->fg;
    }

    if (mImpl->drawFunc) {
        mImpl->drawFunc(context);
    } else {
        switch (mImpl->icon) {
            case StandardIcon::kNone:
                break;
            case StandardIcon::kCloseX:
                painter->drawX(context.dc, size, fg);
                break;
            case StandardIcon::kCloseXCircle:
                painter->drawXCircle(context.dc, size, fg);
                break;
            case StandardIcon::kPrevScreen:
                painter->drawPrevScreen(context.dc, size, fg);
                break;
            case StandardIcon::kNextScreen:
                painter->drawNextScreen(context.dc, size, fg);
                break;
            case StandardIcon::kTwistyClosed:
                painter->drawTwistyClosed(context.dc, size, fg);
                break;
            case StandardIcon::kTwistyOpen:
                painter->drawTwistyOpen(context.dc, size, fg);
                break;
            case StandardIcon::kError:
                painter->drawError(context.dc, size, fg);
                break;
            case StandardIcon::kWarning:
                painter->drawWarning(context.dc, size, fg);
                break;
            case StandardIcon::kInfo:
                painter->drawInfo(context.dc, size, fg);
                break;
            case StandardIcon::kHelp:
                painter->drawHelp(context.dc, size, fg);
                break;
            case StandardIcon::kSearch:
                painter->drawSearch(context.dc, size, fg);
                break;
            case StandardIcon::kHistory:
                painter->drawHistory(context.dc, size, fg);
                break;
            case StandardIcon::kMenu:
                painter->drawMenu(context.dc, size, fg);
                break;
            case StandardIcon::kAdd:
                painter->drawAdd(context.dc, size, fg);
                break;
            case StandardIcon::kRemove:
                painter->drawRemove(context.dc, size, fg);
                break;
            case StandardIcon::kAddCircle:
                painter->drawAddCircle(context.dc, size, fg);
                break;
            case StandardIcon::kRemoveCircle:
                painter->drawRemoveCircle(context.dc, size, fg);
                break;
            case StandardIcon::kExpand:
                painter->drawExpand(context.dc, size, fg);
                break;
            case StandardIcon::kContract:
                painter->drawContract(context.dc, size, fg);
                break;
            case StandardIcon::kMoreHoriz:
                painter->drawMoreHoriz(context.dc, size, fg);
                break;
            case StandardIcon::kMoreVert:
                painter->drawMoreVert(context.dc, size, fg);
                break;
            case StandardIcon::kLocked:
                painter->drawLocked(context.dc, size, fg);
                break;
            case StandardIcon::kUnlocked:
                painter->drawUnlocked(context.dc, size, fg);
                break;
            case StandardIcon::kSettings:
                painter->drawSettings(context.dc, size, fg);
                break;
            case StandardIcon::kChevronLeft:
                painter->drawChevronLeft(context.dc, size, fg);
                break;
            case StandardIcon::kChevronRight:
                painter->drawChevronRight(context.dc, size, fg);
                break;
            case StandardIcon::kChevronUp:
                painter->drawChevronUp(context.dc, size, fg);
                break;
            case StandardIcon::kChevronDown:
                painter->drawChevronDown(context.dc, size, fg);
                break;
            case StandardIcon::kChevronLeftCircle:
                painter->drawChevronLeftCircle(context.dc, size, fg);
                break;
            case StandardIcon::kChevronRightCircle:
                painter->drawChevronRightCircle(context.dc, size, fg);
                break;
            case StandardIcon::kChevronUpCircle:
                painter->drawChevronUpCircle(context.dc, size, fg);
                break;
            case StandardIcon::kChevronDownCircle:
                painter->drawChevronDownCircle(context.dc, size, fg);
                break;
            case StandardIcon::kTriangleLeft:
                painter->drawTriangleLeft(context.dc, size, fg);
                break;
            case StandardIcon::kTriangleRight:
                painter->drawTriangleRight(context.dc, size, fg);
                break;
            case StandardIcon::kTriangleUp:
                painter->drawTriangleUp(context.dc, size, fg);
                break;
            case StandardIcon::kTriangleDown:
                painter->drawTriangleDown(context.dc, size, fg);
                break;
            case StandardIcon::kTriangleLeftCircle:
                painter->drawTriangleLeftCircle(context.dc, size, fg);
                break;
            case StandardIcon::kTriangleRightCircle:
                painter->drawTriangleRightCircle(context.dc, size, fg);
                break;
            case StandardIcon::kTriangleUpCircle:
                painter->drawTriangleUpCircle(context.dc, size, fg);
                break;
            case StandardIcon::kTriangleDownCircle:
                painter->drawTriangleDownCircle(context.dc, size, fg);
                break;
            case StandardIcon::kRefresh:
                painter->drawRefresh(context.dc, size, fg);
                break;
            case StandardIcon::kArrowLeft:
                painter->drawArrowLeft(context.dc, size, fg);
                break;
            case StandardIcon::kArrowRight:
                painter->drawArrowRight(context.dc, size, fg);
                break;
            case StandardIcon::kArrowUp:
                painter->drawArrowUp(context.dc, size, fg);
                break;
            case StandardIcon::kArrowDown:
                painter->drawArrowDown(context.dc, size, fg);
                break;
            case StandardIcon::kArrowLeftCircle:
                painter->drawArrowLeftCircle(context.dc, size, fg);
                break;
            case StandardIcon::kArrowRightCircle:
                painter->drawArrowRightCircle(context.dc, size, fg);
                break;
            case StandardIcon::kArrowUpCircle:
                painter->drawArrowUpCircle(context.dc, size, fg);
                break;
            case StandardIcon::kArrowDownCircle:
                painter->drawArrowDownCircle(context.dc, size, fg);
                break;
            case StandardIcon::kMacCmd:
                painter->drawMacCmd(context.dc, size, fg);
                break;
            case StandardIcon::kMacShift:
                painter->drawMacShift(context.dc, size, fg);
                break;
            case StandardIcon::kMacOption:
                painter->drawMacOption(context.dc, size, fg);
                break;
            case StandardIcon::kNewFile:
                painter->drawNewFile(context.dc, size, fg);
                break;
            case StandardIcon::kOpenFile:
                painter->drawOpenFile(context.dc, size, fg);
                break;
            case StandardIcon::kSaveFile:
                painter->drawSaveFile(context.dc, size, fg);
                break;
            case StandardIcon::kPrint:
                painter->drawPrint(context.dc, size, fg);
                break;
            case StandardIcon::kExport:
                painter->drawExport(context.dc, size, fg);
                break;
            case StandardIcon::kExternal:
                painter->drawExternal(context.dc, size, fg);
                break;
            case StandardIcon::kBoldStyle:
                painter->drawBoldStyle(context.dc, size, fg);
                break;
            case StandardIcon::kItalicStyle:
                painter->drawItalicStyle(context.dc, size, fg);
                break;
            case StandardIcon::kUnderlineStyle:
                painter->drawUnderlineStyle(context.dc, size, fg);
                break;
            case StandardIcon::kAlignLeft:
                painter->drawAlignLeft(context.dc, size, fg);
                break;
            case StandardIcon::kAlignCenter:
                painter->drawAlignCenter(context.dc, size, fg);
                break;
            case StandardIcon::kAlignRight:
                painter->drawAlignRight(context.dc, size, fg);
                break;
            case StandardIcon::kAlignJustify:
                painter->drawAlignJustify(context.dc, size, fg);
                break;
            case StandardIcon::kBulletList:
                painter->drawBulletList(context.dc, size, fg);
                break;
            case StandardIcon::kNumericList:
                painter->drawNumericList(context.dc, size, fg);
                break;
            case StandardIcon::kPlay:
                painter->drawPlay(context.dc, size, fg);
                break;
            case StandardIcon::kPause:
                painter->drawPause(context.dc, size, fg);
                break;
            case StandardIcon::kStop:
                painter->drawStop(context.dc, size, fg);
                break;
            case StandardIcon::kFastForward:
                painter->drawFastForward(context.dc, size, fg);
                break;
            case StandardIcon::kFastReverse:
                painter->drawFastReverse(context.dc, size, fg);
                break;
            case StandardIcon::kSkipForward:
                painter->drawSkipForward(context.dc, size, fg);
                break;
            case StandardIcon::kSkipBackward:
                painter->drawSkipBackward(context.dc, size, fg);
                break;
            case StandardIcon::kShuffle:
                painter->drawShuffle(context.dc, size, fg);
                break;
            case StandardIcon::kLoop:
                painter->drawLoop(context.dc, size, fg);
                break;
            case StandardIcon::kVolumeMute:
                painter->drawVolumeMute(context.dc, size, fg);
                break;
            case StandardIcon::kVolumeSoft:
                painter->drawVolumeSoft(context.dc, size, fg);
                break;
            case StandardIcon::kVolumeMedium:
                painter->drawVolumeMedium(context.dc, size, fg);
                break;
            case StandardIcon::kVolumeLoud:
                painter->drawVolumeLoud(context.dc, size, fg);
                break;
            case StandardIcon::kZoomIn:
                painter->drawZoomIn(context.dc, size, fg);
                break;
            case StandardIcon::kZoomOut:
                painter->drawZoomOut(context.dc, size, fg);
                break;
            case StandardIcon::kRecordAudio:
                painter->drawRecordAudio(context.dc, size, fg);
                break;
            case StandardIcon::kRecordVideo:
                painter->drawRecordVideo(context.dc, size, fg);
                break;
            case StandardIcon::kNoAudio:
                painter->drawNoAudio(context.dc, size, fg);
                break;
            case StandardIcon::kNoVideo:
                painter->drawNoVideo(context.dc, size, fg);
                break;
            case StandardIcon::kCamera:
                painter->drawCamera(context.dc, size, fg);
                break;
            case StandardIcon::kFolder:
                painter->drawFolder(context.dc, size, fg);
                break;
            case StandardIcon::kFile:
                painter->drawFile(context.dc, size, fg);
                break;
            case StandardIcon::kTrash:
                painter->drawTrash(context.dc, size, fg);
                break;
            case StandardIcon::kEdit:
                painter->drawEdit(context.dc, size, fg);
                break;
            case StandardIcon::kHome:
                painter->drawHome(context.dc, size, fg);
                break;
            case StandardIcon::kPicture:
                painter->drawPicture(context.dc, size, fg);
                break;
            case StandardIcon::kDocument:
                painter->drawDocument(context.dc, size, fg);
                break;
            case StandardIcon::kUser:
                painter->drawUser(context.dc, size, fg);
                break;
            case StandardIcon::kColor:
                painter->drawColor(context.dc, size, fg);
                break;
            case StandardIcon::kStar:
                painter->drawStar(context.dc, size, fg);
                break;
            case StandardIcon::kHeart:
                painter->drawHeart(context.dc, size, fg);
                break;
            case StandardIcon::kMail:
                painter->drawMail(context.dc, size, fg);
                break;
            case StandardIcon::kAttachment:
                painter->drawAttachment(context.dc, size, fg);
                break;
            case StandardIcon::kCalendar:
                painter->drawCalendar(context.dc, size, fg);
                break;
            case StandardIcon::kChat:
                painter->drawChat(context.dc, size, fg);
                break;
            case StandardIcon::kConversation:
                painter->drawConversation(context.dc, size, fg);
                break;
        }
    }

    if (hasFrame) {
        context.dc.translate(-borderWidth(), -borderWidth());
    }
}

} // namespace uitk
