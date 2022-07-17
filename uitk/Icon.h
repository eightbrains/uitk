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

#ifndef UITK_ICON_H
#define UITK_ICON_H

#include "Widget.h"

#include <functional>

namespace uitk {

// Design note: Icon::Standard just looks strange. This definition probably
// should go in some global definitions file, though.
enum class StandardIcon {
    kNone = 0,
    
    kCloseX = 1,
    kCloseXCircle,
    kPrevScreen,
    kNextScreen,
    kTwistyClosed,
    kTwistyOpen,
    kError,
    kWarning,
    kInfo,
    kHelp,
    kSearch,
    kHistory,
    kMenu,
    kAdd,
    kRemove,
    kAddCircle,
    kRemoveCircle,
    kExpand,
    kContract,
    kMoreHoriz,
    kMoreVert,
    kLocked,
    kUnlocked,
    kSettings,
    kChevronLeft,
    kChevronRight,
    kChevronUp,
    kChevronDown,
    kChevronLeftCircle,
    kChevronRightCircle,
    kChevronUpCircle,
    kChevronDownCircle,
    kTriangleLeft,
    kTriangleRight,
    kTriangleUp,
    kTriangleDown,
    kTriangleLeftCircle,
    kTriangleRightCircle,
    kTriangleUpCircle,
    kTriangleDownCircle,
    kRefresh,
    kArrowLeft,
    kArrowRight,
    kArrowUp,
    kArrowDown,
    kArrowLeftCircle,
    kArrowRightCircle,
    kArrowUpCircle,
    kArrowDownCircle,
    kMacCmd,
    kMacShift,
    kMacOption,

    kNewFile = 300,
    kOpenFile,
    kSaveFile,
    kPrint,
    kExport,
    kExternal,
    kBoldStyle,
    kItalicStyle,
    kUnderlineStyle,
    kAlignLeft,
    kAlignCenter,
    kAlignRight,
    kAlignJustify,
    kBulletList,
    kNumericList,
    kPlay,
    kPause,
    kStop,
    kFastForward,
    kFastReverse,
    kSkipForward,
    kSkipBackward,
    kShuffle,
    kLoop,
    kVolumeMute,
    kVolumeSoft,
    kVolumeMedium,
    kVolumeLoud,
    kZoomIn,
    kZoomOut,
    kRecordAudio,
    kRecordVideo,
    kNoAudio,
    kNoVideo,
    kCamera,

    kFolder = 500,
    kFile,
    kTrash,
    kHome,
    kPicture,
    kDocument,
    kEdit,
    kUser,
    kColor,
    kStar,
    kHeart,
    kMail,
    kAttachment,
    kCalendar,
    kChat,
    kConversation
};

class Icon : public Widget
{
    using Super = Widget;
public:
    explicit Icon(StandardIcon icon);
    explicit Icon(std::function<void(UIContext&)> drawFunc);

    void setColor(const Color& fg);
    const Color& color() const;

    Size preferredSize(const LayoutContext& context) const override;
    void draw(UIContext& context) override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace uitk
#endif // UITK_ICON_H
