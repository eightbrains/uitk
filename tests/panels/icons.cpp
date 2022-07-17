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

// No includes, this is included directly as a .cpp file, which avoids the
// hassle of a bunch of mostly identical header files.

namespace icons {

class AllIcons : public ScrollView
{
    using Super = ScrollView;
public:
    AllIcons()
    {
        std::vector<StandardIcon> icons = {
            StandardIcon::kCloseX,
            StandardIcon::kCloseXCircle,
            StandardIcon::kPrevScreen,
            StandardIcon::kNextScreen,
            StandardIcon::kTwistyClosed,
            StandardIcon::kTwistyOpen,
            StandardIcon::kError,
            StandardIcon::kWarning,
            StandardIcon::kInfo,
            StandardIcon::kHelp,
            StandardIcon::kSearch,
            StandardIcon::kHistory,
            StandardIcon::kMenu,
            StandardIcon::kAdd,
            StandardIcon::kRemove,
            StandardIcon::kAddCircle,
            StandardIcon::kRemoveCircle,
            StandardIcon::kExpand,
            StandardIcon::kContract,
            StandardIcon::kMoreHoriz,
            StandardIcon::kMoreVert,
            StandardIcon::kLocked,
            StandardIcon::kUnlocked,
            StandardIcon::kSettings,
            StandardIcon::kChevronLeft,
            StandardIcon::kChevronRight,
            StandardIcon::kChevronUp,
            StandardIcon::kChevronDown,
            StandardIcon::kChevronLeftCircle,
            StandardIcon::kChevronRightCircle,
            StandardIcon::kChevronUpCircle,
            StandardIcon::kChevronDownCircle,
            StandardIcon::kTriangleLeft,
            StandardIcon::kTriangleRight,
            StandardIcon::kTriangleUp,
            StandardIcon::kTriangleDown,
            StandardIcon::kTriangleLeftCircle,
            StandardIcon::kTriangleRightCircle,
            StandardIcon::kTriangleUpCircle,
            StandardIcon::kTriangleDownCircle,
            StandardIcon::kRefresh,
            StandardIcon::kArrowLeft,
            StandardIcon::kArrowRight,
            StandardIcon::kArrowUp,
            StandardIcon::kArrowDown,
            StandardIcon::kArrowLeftCircle,
            StandardIcon::kArrowRightCircle,
            StandardIcon::kArrowUpCircle,
            StandardIcon::kArrowDownCircle,
            StandardIcon::kMacCmd,
            StandardIcon::kMacShift,
            StandardIcon::kMacOption,

            StandardIcon::kNewFile,
            StandardIcon::kOpenFile,
            StandardIcon::kSaveFile,
            StandardIcon::kPrint,
            StandardIcon::kExport,
            StandardIcon::kExternal,
            StandardIcon::kBoldStyle,
            StandardIcon::kItalicStyle,
            StandardIcon::kUnderlineStyle,
            StandardIcon::kAlignLeft,
            StandardIcon::kAlignCenter,
            StandardIcon::kAlignRight,
            StandardIcon::kAlignJustify,
            StandardIcon::kBulletList,
            StandardIcon::kNumericList,
            StandardIcon::kPlay,
            StandardIcon::kPause,
            StandardIcon::kStop,
            StandardIcon::kFastForward,
            StandardIcon::kFastReverse,
            StandardIcon::kSkipForward,
            StandardIcon::kSkipBackward,
            StandardIcon::kShuffle,
            StandardIcon::kLoop,
            StandardIcon::kVolumeMute,
            StandardIcon::kVolumeSoft,
            StandardIcon::kVolumeMedium,
            StandardIcon::kVolumeLoud,
            StandardIcon::kZoomIn,
            StandardIcon::kZoomOut,
            StandardIcon::kRecordAudio,
            StandardIcon::kRecordVideo,
            StandardIcon::kNoAudio,
            StandardIcon::kNoVideo,
            StandardIcon::kCamera,

            StandardIcon::kFolder,
            StandardIcon::kFile,
            StandardIcon::kTrash,
            StandardIcon::kEdit,
            StandardIcon::kHome,
            StandardIcon::kPicture,
            StandardIcon::kDocument,
            StandardIcon::kUser,
            StandardIcon::kColor,
            StandardIcon::kStar,
            StandardIcon::kHeart,
            StandardIcon::kMail,
            StandardIcon::kAttachment,
            StandardIcon::kCalendar,
            StandardIcon::kChat,
            StandardIcon::kConversation
        };

        for (auto &id : icons) {
            auto *icon = new Icon(id);
            addChild(icon);
            mIcons.push_back(icon);
        }
    }

    void setIconSize(int px)
    {
        mIconSizePx = px;
        setNeedsLayout();
    }

    void setIconColors(const Color& fg, const Color& bg)
    {
        for (auto *icon : mIcons) {
            icon->setBackgroundColor(bg);
            icon->setColor(fg);
        }
    }

    void layout(const LayoutContext& context) override
    {
        auto em = context.theme.params().labelFont.pointSize();
        auto spacing = 0.5f * em;
        auto width = PicaPt::fromPixels(float(mIconSizePx), context.dc.dpi());
        auto height = width;
        const int nCols = int(frame().width / (width + spacing));
        int row = 0, col = 0;
        for (auto *icon : mIcons) {
            icon->setFrame(Rect(float(col) * (width + spacing), float(row) * (height + spacing),
                                width, height));
            col++;
            if (col >= nCols) {
                row++;
                col = 0;
            }
        }
        setBounds(Rect(PicaPt::kZero, PicaPt::kZero,
                       mIcons.back()->frame().maxX(), mIcons.back()->frame().maxY()));

        Super::layout(context);
    }

private:
    // We could use children() to iterate over them, but children() will
    // also have scrollbar widgets; this is just easier.
    std::vector<Icon*> mIcons;  // we do not own these, Super does
    int mIconSizePx = 0;
};

class Panel : public Widget
{
    using Super = Widget;
public:
    Panel()
    {
        mSizeLabel = new Label("Icon size");
        addChild(mSizeLabel);
        mSizeSlider = new Slider();
        mSizeSlider->setValue(0);
        addChild(mSizeSlider);
        mSizeEdit = new NumberEdit();
        addChild(mSizeEdit);

        mDebug = new Checkbox("Use debug colors");
        addChild(mDebug);

        mIcons = new AllIcons();
        addChild(mIcons);

        mSizeSlider->setOnValueChanged([this](SliderLogic *slider) {
            this->mSizeEdit->setValue(slider->intValue());
            this->mIcons->setIconSize(slider->intValue());
        });
        mSizeEdit->setOnValueChanged([this](NumberEdit *edit) {
            this->mSizeSlider->setValue(edit->intValue());
            this->mIcons->setIconSize(edit->intValue());
        });

        mDebug->setOnClicked([this](Button *b) {
            if (b->isOn()) {
                this->mIcons->setBackgroundColor(Color::kBlack);
                this->mIcons->setIconColors(Color::kWhite, Color(0.0f, 0.4f, 0.2f));
            } else {
                this->mIcons->setBackgroundColor(Color::kTransparent);
                this->mIcons->setIconColors(Color::kTransparent, Color::kTransparent);
            }
        });
    }

    void layout(const LayoutContext& context) override
    {
        auto em = context.dc.roundToNearestPixel(context.theme.params().labelFont.pointSize());

        // TODO: this is a hack, we really need an onInit() or onDpiChanged() or something
        if (mSizeEdit->intValue() <= 0) {
            int maxPx = PicaPt(72.0f) / context.dc.onePixel();
            int defaultPx = em / context.dc.onePixel();
            mSizeEdit->setLimits(9, maxPx, 1);
            mSizeSlider->setLimits(9, maxPx, 1);
            mSizeEdit->setValue(defaultPx);
            mSizeSlider->setValue(defaultPx);
            mIcons->setIconSize(defaultPx);
        }

        auto marginX = em;
        auto y = em;

        auto pref = mSizeLabel->preferredSize(context);
        mSizeLabel->setFrame(Rect(marginX, y, pref.width, pref.height));
        pref = mSizeSlider->preferredSize(context);
        mSizeSlider->setFrame(Rect(mSizeLabel->frame().maxX() + em, y, 15 * em, pref.height));
        pref = mSizeEdit->preferredSize(context);
        mSizeEdit->setFrame(Rect(mSizeSlider->frame().maxX() + em, y, pref.width, pref.height));
        y = mSizeEdit->frame().maxY() + em;

        pref = mDebug->preferredSize(context);
        mDebug->setFrame(Rect(marginX, y, pref.width, pref.height));
        y = mDebug->frame().maxY() + em;
        
        mIcons->setFrame(Rect(marginX, y, 21.0f * em, 31.0f * em));

        Super::layout(context);
    }

private:
    Label *mSizeLabel;
    Slider *mSizeSlider;
    NumberEdit *mSizeEdit;
    Checkbox *mDebug;
    AllIcons *mIcons;
};

} // namespace icons
