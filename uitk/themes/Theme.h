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

#ifndef UITK_THEME_H
#define UITK_THEME_H

#include "../Global.h"

#include <nativedraw.h>

#include <functional>

namespace uitk {

struct LayoutContext;
class TextEditorLogic;
struct UIContext;

class Theme {
public:
    enum class WidgetState {
        kNormal = 0, // normal state (mouse not in widget)
        kDisabled,   // widget is disabled
        kMouseOver,  // mouse is over widget (widget is highlighted)
        kMouseDown,  // mouse is clicking on widget
        kSelected    // widget is drawn selected (e.g. in list view)
    };

    struct WidgetStyle {
        enum Flags { kNoneSet = 0,
                     kBGColorSet = (1 << 0),
                     kFGColorSet = (1 << 1),
                     kBorderColorSet = (1 << 2),
                     kBorderWidthSet = (1 << 3),
                     kBorderRadiusSet = (1 << 4),
        };

        int flags = kNoneSet;
        Color bgColor;
        Color fgColor;
        Color borderColor;
        PicaPt borderWidth = PicaPt(0);
        PicaPt borderRadius = PicaPt(0);

        WidgetStyle merge(const WidgetStyle& s) const;
    };

    struct Params
    {
        Color windowBackgroundColor;
        Color nonEditableBackgroundColor;
        Color editableBackgroundColor;
        Color disabledBackgroundColor;
        Color textColor;
        Color disabledTextColor;
        Color accentedBackgroundTextColor;  // for when accentColor is bg of text
        Color accentColor;
        Color selectionColor;
        Color keyFocusColor;
        Color nonNativeMenuSeparatorColor;
        Color nonNativeMenuBackgroundColor;
        Color nonNativeMenubarBackgroundColor;
        Font labelFont;
        Font nonNativeMenubarFont;

        bool useClearTextButton = false;
        bool useClearTextButtonForSearch = false;
    };

    /// Draws an icon in the given color. Function need not save/restore the
    /// DrawContext unless clipping is used. The design of the icon is assumed
    /// to fill the rectangle, although generally icons are square and should
    /// center themselves (aligned to a pixel boundary!) if size is not
    /// square. Margins will taken care of at a higher level. Function should
    /// good at multiple DPIs and with both odd and even numbers of pixels.
    using Icon = std::function<void(DrawContext&, const Theme&, const Rect&, const Color& fg)>;

    enum class StandardIcon {
        kNone = 0,  // no icon at all
        kEmpty = 1,  // an icon that draws nothing (useful for layout)

        kCloseX = 2,
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
        kCheckmark,
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

public:
    Theme() {}
    virtual ~Theme() {}

    virtual const Params& params() const = 0;
    virtual void setParams(const Params& params) = 0;

    /// The text margin vertically is around capHeight, NOT ascent + descent.
    /// So in more traditional terms, the margin is (margin - descent) below the
    /// descender, and (margin) above (baseline + capHeight). Note that baseline + ascent
    /// is can be substantially above the top of the text, and seems to act like
    /// leading (which in these fonts is usually zero).
    virtual Size calcPreferredTextMargins(const DrawContext& dc, const Font& font) const = 0;
    /// Returns the standard height of a widget (button, single-line text edit,
    /// combobox, etc.). This should be used as the height if possible, which
    /// allows widgets to placed next to each other to have text baselines
    /// align nicely. This is always an integer number of pixels.
    virtual PicaPt calcStandardHeight(const DrawContext& dc, const Font& font) const = 0;
    /// Returns icon height when used in a standard-height
    virtual Size calcStandardIconSize(const DrawContext& dc, const Font& font) const = 0;
    virtual Rect calcStandardIconRect(const DrawContext& dc, const Rect& frame, const Font& font) const = 0;
    virtual PicaPt calcStandardIconSeparator(const DrawContext& dc, const Font& font) const = 0;
    virtual Size calcPreferredButtonMargins(const DrawContext& dc, const Font& font) const = 0;
    virtual Size calcPreferredCheckboxSize(const DrawContext& dc,
                                           const Font& font) const = 0;
    virtual Size calcPreferredSegmentMargins(const DrawContext& dc, const Font& font) const = 0;
    virtual Size calcPreferredComboBoxSize(const DrawContext& dc, const PicaPt& preferredMenuWidth) const = 0;
    virtual Size calcPreferredSliderThumbSize(const DrawContext& dc) const = 0;
    virtual Size calcPreferredProgressBarSize(const DrawContext& dc) const = 0;
    virtual Size calcPreferredTextEditSize(const DrawContext& dc, const Font& font) const = 0;
    virtual Rect calcTextEditRectForFrame(const Rect& frame, const DrawContext& dc,
                                          const Font& font) const = 0;
    virtual Size calcPreferredIncDecSize(const DrawContext& dc) const = 0;
    virtual PicaPt calcPreferredScrollbarThickness(const DrawContext& dc) const = 0;
    enum class MenuItemAttribute { kNormal, kChecked, kSubmenu };
    virtual Size calcPreferredMenuItemSize(const DrawContext& dc,
                                           const std::string& text, const std::string& shortcut,
                                           MenuItemAttribute itemAttr,
                                           PicaPt *shortcutWidth) const = 0;
    struct MenubarMetrics {
        PicaPt horizMargin;
        PicaPt checkboxWidth;
        PicaPt afterCheckboxSeparator;
        PicaPt afterTextSeparator;
        Size submenuIconSize;
    };
    virtual MenubarMetrics calcPreferredMenuItemMetrics(const DrawContext& dc, const PicaPt& height) const = 0;
    virtual PicaPt calcPreferredMenuVerticalMargin() const = 0;
    virtual PicaPt calcPreferredMenubarItemHorizMargin(const DrawContext& dc, const PicaPt& height) const = 0;

    virtual void drawCheckmark(UIContext& ui, const Rect& r, const WidgetStyle& style) const = 0;
    virtual void drawSubmenuIcon(UIContext& ui, const Rect& frame, const WidgetStyle& style) const = 0;

    virtual void drawWindowBackground(UIContext& ui, const Size& size) const = 0;
    virtual void drawFrame(UIContext& ui, const Rect& frame,
                           const WidgetStyle& style) const = 0;
    virtual void clipFrame(UIContext& ui, const Rect& frame,
                           const WidgetStyle& style) const = 0;
    virtual void drawFocusFrame(UIContext& ui, const Rect& frame, const WidgetStyle& style) const = 0;
    virtual void drawIcon(UIContext& ui, const Rect& r, const Icon& icon, const Color& color) const;
    virtual void drawIcon(UIContext& ui, const Rect& r, StandardIcon icon, const Color& color) const;
    virtual WidgetStyle labelStyle(const WidgetStyle& style, WidgetState state) const = 0;
    enum class ButtonDrawStyle
    {
        kNormal,        /// normal button with a frame
        kDialogDefault, /// button indicating Enter will press it
        kNoDecoration,  /// icon button; no frame
        kAccessory,     /// pressable pieces of a widget, like the X to clear text in a search widget
    };
    virtual void drawButton(UIContext& ui, const Rect& frame, ButtonDrawStyle buttonStyle,
                            const WidgetStyle& style, WidgetState state,
                            bool isOn) const = 0;
    virtual const WidgetStyle& buttonTextStyle(WidgetState state, ButtonDrawStyle buttonStyle,
                                               bool isOn) const = 0;
    virtual void drawCheckbox(UIContext& ui, const Rect& frame,
                              const WidgetStyle& style, WidgetState state,
                              bool isOn) const = 0;
    enum class SegmentDrawStyle
    {
        kNormal, kNoDecoration
    };
    virtual void drawSegmentedControl(UIContext& ui, const Rect& frame, SegmentDrawStyle drawStyle,
                                      const WidgetStyle& style,
                                      WidgetState state) const = 0;
    virtual void drawSegment(UIContext& ui, const Rect& frame, SegmentDrawStyle drawStyle,
                             WidgetState state, bool isButton, bool isOn,
                             int segmentIndex, int nSegments) const = 0;
    virtual void drawSegmentDivider(UIContext& ui, const Point& top, const Point& bottom,
                                    SegmentDrawStyle drawStyle,
                                    const WidgetStyle& ctrlStyle, WidgetState ctrlState) const = 0;
    virtual const WidgetStyle& segmentTextStyle(WidgetState state, SegmentDrawStyle drawStyle,
                                                bool isOn) const = 0;
    virtual void drawComboBoxAndClip(UIContext& ui, const Rect& frame,
                                     const WidgetStyle& style, WidgetState state) const = 0;
    virtual void drawSliderTrack(UIContext& ui, SliderDir dir, const Rect& frame, const Point& thumbMid,
                                 const WidgetStyle& style, WidgetState state) const = 0;
    virtual void drawSliderThumb(UIContext& ui, const Rect& frame, const WidgetStyle& style,
                                 WidgetState state) const = 0;
    virtual void drawScrollbarTrack(UIContext& ui, SliderDir dir, const Rect& frame, const Point& thumbMid,
                                    const WidgetStyle& style, WidgetState state) const = 0;
    virtual void drawScrollbarThumb(UIContext& ui, const Rect& frame, const WidgetStyle& style,
                                    WidgetState state) const = 0;
    virtual void drawProgressBar(UIContext& ui, const Rect& frame, float value,
                                 const WidgetStyle& style, WidgetState state) const = 0;
    virtual void drawIncDec(UIContext& ui, const Rect& frame, WidgetState incState, WidgetState decState) const = 0;
    virtual WidgetStyle textEditStyle(const WidgetStyle& style, WidgetState state) const = 0;
    virtual void drawTextEdit(UIContext& ui, const Rect& frame, const PicaPt& scrollOffset,
                              const std::string& placeholder, TextEditorLogic& editor, int horizAlign, 
                              const WidgetStyle& style, WidgetState state, bool hasFocus) const = 0;
    // It's not clear if we should draw the search icon here, or have SearchBar have an
    // Icon object. The Icon object seems like it gives the user easier customizability.
    // Plus, it is more consistent with the clear-text button for text edit; that pretty
    // much needs to be a button order to have the same behavior
    virtual void drawSearchBar(UIContext& ui, const Rect& frame, const WidgetStyle& style,
                               WidgetState state) const = 0;
    virtual void clipScrollView(UIContext& ui, const Rect& frame,
                                const WidgetStyle& style, WidgetState state, bool drawsFrame) const = 0;
    virtual void drawScrollView(UIContext& ui, const Rect& frame,
                                const WidgetStyle& style, WidgetState state) const = 0;
    virtual void drawListView(UIContext& ui, const Rect& frame,
                              const WidgetStyle& style, WidgetState state) const = 0;
    virtual void clipListView(UIContext& ui, const Rect& frame,
                              const WidgetStyle& style, WidgetState state) const = 0;
    virtual void drawListViewSpecialRow(UIContext& ui, const Rect& frame,
                                        const WidgetStyle& style, WidgetState state) const = 0;
    virtual void drawMenuBackground(UIContext& ui, const Size& size) = 0;
    virtual void calcMenuItemFrames(const DrawContext& dc, const Rect& frame, const PicaPt& shortcutWidth,
                                    Rect *checkRect, Rect *textRect, Rect *shortcutRect) const = 0;
    virtual void drawMenuItem(UIContext& ui, const Rect& frame, const PicaPt& shortcutWidth,
                              const std::string& text, const std::string& shortcutKey,
                              MenuItemAttribute itemAttr,
                              const WidgetStyle& style, WidgetState state) const = 0;
    virtual void drawMenuSeparatorItem(UIContext& ui, const Rect& frame) const = 0;
    virtual void drawMenubarBackground(UIContext& ui, const Rect& frame) const = 0;
    virtual void drawMenubarItem(UIContext& ui, const Rect& frame, const std::string& text,
                                 WidgetState state) const = 0;
};

}  // namespace uitk
#endif // UITK_THEME_H
