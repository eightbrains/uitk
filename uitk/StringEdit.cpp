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

#include "StringEdit.h"

#include "Button.h"
#include "Cursor.h"
#include "Events.h"
#include "MenuUITK.h"
#include "StringEditorLogic.h"
#include "UIContext.h"
#include "Window.h"
#include "private/Utils.h"
#include "themes/Theme.h"

namespace uitk {

namespace {

class ButtonThatSetsCursor : public Button
{
    using Super = Button;
public:
    ButtonThatSetsCursor(Theme::StandardIcon icon)
        : Button(icon)
    {}
    ~ButtonThatSetsCursor() {}

    void mouseEntered() override
    {
        Super::mouseEntered();
        window()->pushCursor(Cursor::arrow());
    }

    void mouseExited() override
    {
        Super::mouseExited();
        window()->popCursor();
    }
};

}  // namespace

Point calcScrollOffset(const DrawContext& dc, const Font& font, const StringEditorLogic& editor,
                       const Size& viewSize, int horizAlign, const Size& margins, const Point& currentScroll)
{
    auto sel = editor.selection();
    auto idx = sel.cursorIndex(0);
    if (!editor.imeConversion().isEmpty()) {
        idx += editor.imeConversion().cursorOffset;
    }
    auto r = Rect(PicaPt::kZero, PicaPt::kZero, viewSize.width, viewSize.height);
    auto textMetrics = editor.layout()->metrics();
    auto textWidth = textMetrics.width;
    auto textHeight = textMetrics.height;
    PicaPt textStartX;
    PicaPt textStartY = margins.height;
    switch (horizAlign) {
        default:
        case Alignment::kLeft:
            textStartX = PicaPt::kZero;
            break;
        case Alignment::kHCenter:
            textStartX = r.midX() - 0.5f * textWidth;
            break;
        case Alignment::kRight:
            textStartX = r.maxX() - textWidth;
            break;
    }
    auto glyphRect = editor.glyphRectAtIndex(idx);
    // Add an extra line to the bottom if there is a trailing empty line
    if (!editor.isEmpty() && editor.string().back() == '\n') {
        auto ag = dc.createTextLayout(Text("Ag\nAg", font, Color::kGrey),
                                      Size(Button::kDimGrow, Button::kDimGrow));
        auto &glyph3 = ag->glyphs()[3];
        auto extraHeight = glyph3.frame.y - ag->glyphs()[0].frame.y;
        textHeight += extraHeight;
        if (idx == editor.size()) {
            glyphRect = Rect(glyph3.frame.x, textHeight - extraHeight, PicaPt::kZero, glyph3.frame.height);
        }
    }

    auto cursorPt = Point(textStartX + glyphRect.x, textStartY + glyphRect.y);
    Point offset = currentScroll;
    // Horizontal offset
    if (textWidth > r.width) {
        if (r.contains(Point(cursorPt.x + currentScroll.x, PicaPt::kZero))) {
            // If we deleted characters from the right (subtract off 1 pt in case of roundoff errors)
            if (textWidth > r.width && textStartX + textWidth + currentScroll.x < r.maxX() - PicaPt(1)) {
                offset.x = r.width - textWidth;
            }
            // If we deleted characters from the left (subtract off 1 pt in case of roundoff errors)
            else if (textWidth > r.width && textStartX + currentScroll.x > r.minX() + PicaPt(1)) {
                offset.x = PicaPt::kZero;
            }
            // Otherwise, no change is needed
            else {
                offset.x = currentScroll.x;
            }
        } else {
            if (sel.start == sel.end) {
                if (cursorPt.x + currentScroll.x < r.x) {
                    offset.x = -cursorPt.x;
                } else {
                    offset.x = -(cursorPt.x - viewSize.width);
                }
            } else {
                if (sel.cursorLoc == TextEditorLogic::Selection::CursorLocation::kStart) {
                    offset.x = -cursorPt.x;
                } else {
                    offset.x = -(cursorPt.x - viewSize.width);
                }
            }
        }
    }
    // Vertical offset
    if (textHeight > r.height) {
        if (r.contains(Point(PicaPt::kZero, cursorPt.y + currentScroll.y)) &&
            r.contains(Point(PicaPt::kZero, cursorPt.y + currentScroll.y + glyphRect.height)))
        {
            // If we deleted characters from the bottom
            if (textStartY + textHeight + currentScroll.y < r.maxY()) {
//                offset.y = r.height - textHeight;
                // TODO: we need to figure out where the top is, then scroll up by a line
                // until we get to the right y value.
            } else {
                offset.y = currentScroll.y;
            }
        } else {
            if (sel.start == sel.end) {
                if (cursorPt.y + currentScroll.y < r.y) {
                    offset.y = -cursorPt.y;
                } else {
                    offset.y = -(cursorPt.y + glyphRect.height - viewSize.height);
                }
            } else {
                if (sel.cursorLoc == TextEditorLogic::Selection::CursorLocation::kStart) {
                    offset.y = -cursorPt.y;
                } else {
                    offset.y = -(cursorPt.y + glyphRect.height - viewSize.height);
                }
            }
        }
    } else {
        offset.y = PicaPt::kZero;  // may have deleted text up from bottom
    }

    return offset;
}

Point calcAlignmentOffset(const StringEditorLogic& editor, const Rect& textEditRect, int horizAlign)
{
    switch (horizAlign & Alignment::kHorizMask) {
        default:
        case Alignment::kLeft:
            return Point(PicaPt::kZero, PicaPt::kZero);
        case Alignment::kHCenter:
            return (Point(textEditRect.midX(), textEditRect.minY()) -
                    Point(0.5f * editor.layout()->metrics().width, PicaPt::kZero)) -
                   textEditRect.upperLeft();
        case Alignment::kRight:
            return (textEditRect.upperRight() - Point(editor.layout()->metrics().width, PicaPt::kZero)) -
                   textEditRect.upperLeft();
    }
}

// Design note:
// Password mode is more logically done, from a code standpoint, as a PasswordEdit
// subclass since it requires keeping a display editor and an actual string editor.
// However, from the standpoint of a library user, all it does it draw the characters
// as bullets instead of the actual characters. Toolkits diverge on which approach
// they take. Qt uses a mode (QLineEdit::setEchoMode()); Cocoa uses a class (NSSecureTextField).
// The advantage of having it be a mode is that supporting show/hide password is obviously
// just toggling the mode, whereas with a PasswordEdit class there would have to be
// a mode to undo the effect of the class. Hence, we go with a mode for the interface,
// and accept the complication of the code (which would come in the PasswordEdit
// otherwise).
struct StringEdit::Impl
{
    StringEditorLogic editor;
    std::unique_ptr<StringEditorLogic> passwordDisplay;
    std::string placeholder;
    int alignment = Alignment::kLeft | Alignment::kVCenter;
    Rect editorTextRect;
    Point scrollOffset = Point::kZero;
    UseClearButton useClearButton = UseClearButton::kTheme;
    Button *clearButton = nullptr;  // we do not own this; this will be a child
    MenuUITK *popup = nullptr;  // we own this
    std::function<void(const std::string&)> onTextChanged;
    std::function<void(StringEdit*)> onValueChanged;
    bool isSingleLine = true;
    bool textHasChanged = false;
    bool themeWantsClearButton = false;
    bool windowWasActiveLastDraw = false;

    bool isUsingClearButton()
    {
        switch (this->useClearButton) {
            case UseClearButton::kNo:
                return false;
            case UseClearButton::kYes:
                return true;
            case UseClearButton::kTheme:
                return this->themeWantsClearButton;
        }
        return this->themeWantsClearButton;  // for MSVC
    }

    void updateClearButton()
    {
        bool vis = false;
        if (isUsingClearButton()) {
            vis = !editor.isEmpty();
        }
        this->clearButton->setVisible(vis);
    }

    const Font& font(const Theme& theme)
    {
        return theme.params().labelFont;
    }
};

StringEdit::StringEdit()
    : mImpl(new Impl())
{
    mImpl->editor.onTextChanged = [this]() {
        mImpl->textHasChanged = true;
        mImpl->updateClearButton();
        if (mImpl->onTextChanged) {
            mImpl->onTextChanged(mImpl->editor.string());
        }
    };
    mImpl->editor.onTextCommitted = [this]() {
        if (mImpl->textHasChanged && mImpl->onValueChanged) {
            mImpl->onValueChanged(this);
        }
        mImpl->textHasChanged = false;
        resignKeyFocus();
    };

    mImpl->clearButton = new ButtonThatSetsCursor(Theme::StandardIcon::kCloseXCircle);
    mImpl->clearButton->setDrawStyle(Button::DrawStyle::kAccessory);
    mImpl->clearButton->setOnClicked([this](Button*) {
        setText("");
        if (mImpl->onTextChanged) {  // this was a user action, so do callback
            mImpl->onTextChanged(mImpl->editor.string());
        }
        if (auto *w = window()) {
            if (w->focusWidget() != this) {
                w->setFocusWidget(this);
            }
        }
    });
    addChild(mImpl->clearButton);  // we no longer own
    mImpl->updateClearButton();
}

StringEdit::~StringEdit()
{
    if (mImpl->popup) {
        mImpl->popup->cancel();  // in case menu is open
        delete mImpl->popup;
    }
}

const std::string& StringEdit::text() const { return mImpl->editor.string(); }

StringEdit* StringEdit::setText(const std::string& text)
{
    mImpl->editor.setString(text);
    mImpl->updateClearButton();
    setNeedsDraw();
    return this;
}

const std::string& StringEdit::placeholderText() const
{
    return mImpl->placeholder;
}

StringEdit* StringEdit::setPlaceholderText(const std::string& text)
{
    mImpl->placeholder = text;
    setNeedsDraw();
    return this;
}

bool StringEdit::isPassword() const { return (mImpl->passwordDisplay != nullptr); }

StringEdit* StringEdit::setIsPassword(bool is)
{
    if (mImpl->passwordDisplay && !is) {
        mImpl->passwordDisplay.reset();
    } else if (!mImpl->passwordDisplay && is) {
        mImpl->passwordDisplay.reset(new StringEditorLogic());
    }
    setNeedsDraw();
    return this;
}

int StringEdit::alignment() const { return mImpl->alignment; }

StringEdit* StringEdit::setAlignment(int alignment)
{
    alignment = (alignment & Alignment::kHorizMask);
    mImpl->alignment = alignment | (mImpl->alignment & Alignment::kVertMask);
    setNeedsDraw();
    return this;
}

bool StringEdit::isMultiline() const { return !mImpl->isSingleLine; }

StringEdit* StringEdit::setMultiline(bool multiline)
{
    mImpl->isSingleLine = !multiline;
    setNeedsDraw();
    return this;
}

StringEdit::UseClearButton StringEdit::useClearButton() const { return mImpl->useClearButton; }

StringEdit* StringEdit::setUseClearButton(UseClearButton use)
{
    mImpl->useClearButton = use;
    return this;
}

void StringEdit::setOnTextChanged(std::function<void(const std::string&)> onChanged)
{
    mImpl->onTextChanged = onChanged;
}

void StringEdit::setOnValueChanged(std::function<void(StringEdit*)> onChanged)
{
    mImpl->onValueChanged = onChanged;
}

bool StringEdit::acceptsKeyFocus() const { return true; }

CutPasteable* StringEdit::asCutPasteable()
{
    if (mImpl->passwordDisplay) {
        return nullptr;
    }
    return &mImpl->editor;
}

TextEditorLogic* StringEdit::asTextEditorLogic()
{
    return &mImpl->editor;
}

Widget* StringEdit::setEnabled(bool enabled)
{
    Super::setEnabled(enabled);
    // Need to recreate the TextLayout, because it probably changed color.
    mImpl->editor.setNeedsLayout();
    return this;
}

AccessibilityInfo StringEdit::accessibilityInfo()
{
    auto info = Super::accessibilityInfo();
    info.type = AccessibilityInfo::Type::kTextEdit;
    if (isPassword()) {
        info.type = AccessibilityInfo::Type::kPassword;
    } else {
        info.value = text();
    }
    info.placeholderText = placeholderText();
    info.performSelectAll = [this]() {
        mImpl->editor.setSelection(TextEditorLogic::Selection(0, mImpl->editor.endOfText(), TextEditorLogic::Selection::CursorLocation::kEnd));
        // setSelection() doesn't call setNeedsDraw(), since it is normally called within
        // a mouse/key event and the draw is scheduled there. But the accessibility callback
        // will not be called within an event, so we need to call it manually.
        setNeedsDraw();
    };
    return info;
}

Size StringEdit::preferredSize(const LayoutContext& context) const
{
    if (mImpl->isSingleLine) {
        return context.theme.calcPreferredTextEditSize(context.dc, mImpl->font(context.theme));
    } else {
        return Size(Widget::kDimGrow, Widget::kDimGrow);
    }
}

void StringEdit::layout(const LayoutContext& context)
{
    // We will not have access to this value when we need it, so cache it here.
    // Changing the theme will need to call layout, so it should always be correct.
    mImpl->themeWantsClearButton = context.theme.params().useClearTextButton;
    mImpl->updateClearButton();  // in case theme changed

    auto &r = bounds();
    if (mImpl->isUsingClearButton()) {
        mImpl->clearButton->setFrame(Rect(r.maxX() - r.height, r.y, r.height, r.height));
    } else {
        mImpl->clearButton->setFrame(Rect(r.maxX(), r.y, PicaPt::kZero, r.height));
    }
    mImpl->editorTextRect = context.theme.calcTextEditRectForFrame(
                                    Rect(r.x, r.y, mImpl->clearButton->frame().x, r.height), context.dc,
                                    mImpl->font(context.theme));

    Super::layout(context);
}

void StringEdit::mouseEntered()
{
    Super::mouseEntered();
    mImpl->editor.handleMouseEntered(window());
}

void StringEdit::mouseExited()
{
    Super::mouseExited();
    mImpl->editor.handleMouseExited(window());
}

Widget::EventResult StringEdit::mouse(const MouseEvent& e)
{
    if (mImpl->clearButton->visible() && mImpl->clearButton->frame().contains(e.pos)) {
        return Super::mouse(e);
    }

    bool consumed = false;
    bool copySelection = true;
    bool isInFrame = bounds().contains(e.pos);  // can be outside of frame on a drag and widget is grabbing
    auto me = e;
    me.pos = e.pos - mImpl->editorTextRect.upperLeft() -
             calcAlignmentOffset(mImpl->editor, mImpl->editorTextRect, mImpl->alignment) -
             mImpl->scrollOffset;
    auto *editor = &mImpl->editor;
    if (mImpl->passwordDisplay) {
        me.pos = e.pos - mImpl->editorTextRect.upperLeft() -
                 calcAlignmentOffset(*mImpl->passwordDisplay, mImpl->editorTextRect, mImpl->alignment) -
                 mImpl->scrollOffset;
        editor = mImpl->passwordDisplay.get();
    }

    if (e.type == MouseEvent::Type::kButtonDown && e.button.button == MouseButton::kLeft) {
        if (e.button.nClicks == 1) {
            if (auto *w = window()) {
                if (w->focusWidget() != this) {
                    w->setFocusWidget(this);
                }
            }
        }
        consumed = editor->handleMouseEvent(me, isInFrame);
    } else if (e.type == MouseEvent::Type::kDrag) {
        consumed = editor->handleMouseEvent(me, isInFrame);
    } else if (e.type == MouseEvent::Type::kButtonDown && e.button.button == MouseButton::kRight
               && e.button.nClicks == 1) {
        auto *w = window();
        if (w) {
            // If we do not have focus, select everything and give focus.
            // (This is what macOS does.)
            if (!focused()) {
                if (w->focusWidget() != this) {
                    w->setFocusWidget(this);
                }
                editor->setSelection(TextEditorLogic::Selection(
                                     editor->startOfText(),
                                     editor->endOfText(),
                                     TextEditorLogic::Selection::CursorLocation::kUndetermined));
            }

            // Show the popup
            auto sel = editor->selection();
            if (mImpl->popup) {
                mImpl->popup->cancel();
                delete mImpl->popup;
            }
            mImpl->popup = new MenuUITK();
            mImpl->popup->addItem("Cut", 1,
                                  [this](Window*) { mImpl->editor.cutToClipboard(); setNeedsDraw(); });
            mImpl->popup->addItem("Copy", 2, [this](Window*) { mImpl->editor.copyToClipboard(); });
            mImpl->popup->addItem("Paste", 3,
                                  [this](Window*) { mImpl->editor.pasteFromClipboard(); setNeedsDraw(); });
            bool canCopy = (sel.start < sel.end && !isPassword());
            mImpl->popup->setItemEnabled(1, canCopy);
            mImpl->popup->setItemEnabled(2, canCopy);
            mImpl->popup->show(w, convertToWindowFromLocal(e.pos));

            consumed = true;
        }
    } else if (e.type == MouseEvent::Type::kButtonDown && e.button.button == MouseButton::kMiddle && e.button.nClicks == 1) {
        // For middle-click paste on X11
        consumed = mImpl->editor.handleMouseEvent(me, isInFrame);  // paste: use real editor
        copySelection = false;
    }

    // If we are in password mode, update the selection from password -> editor
    if (mImpl->passwordDisplay && e.type != MouseEvent::Type::kMove && copySelection) {
        auto pwd2cp = codePointIndicesForUTF8Indices(mImpl->passwordDisplay->string().c_str());
        auto cp2idx = utf8IndicesForCodePointIndices(mImpl->editor.string().c_str());
        const int nCodePts = (pwd2cp.empty() ? 0 : int(pwd2cp.back()) + 1);  // since this is a vector index, size() > nCodePts
        pwd2cp.push_back(nCodePts);  // selection can go to nCodePt
        cp2idx.push_back(int(mImpl->editor.string().size()));
        auto sel = mImpl->passwordDisplay->selection();
        sel.start = cp2idx[pwd2cp[std::max(0, sel.start)]];
        sel.end = cp2idx[pwd2cp[std::max(0, sel.end)]];
        mImpl->editor.setSelection(sel);
    }

    if (consumed) {
        setNeedsDraw();
        return Widget::EventResult::kConsumed;
    } else {
        return Super::mouse(e);
    }
}

Widget::EventResult StringEdit::key(const KeyEvent& e)
{
    const auto rkMode = mImpl->isSingleLine ? TextEditorLogic::ReturnKeyMode::kCommits
                                            : TextEditorLogic::ReturnKeyMode::kNewline;
    if (mImpl->editor.handleKeyEvent(e, rkMode)) {
        setNeedsDraw();
        return EventResult::kConsumed;
    }
    return EventResult::kIgnored;
}


void StringEdit::text(const TextEvent& e)
{
    mImpl->editor.handleTextEvent(e);
    setNeedsDraw();
}

void StringEdit::keyFocusEnded()
{
    // Clear selection, since a visible selection is associated with editing text.
    // (Some programs, e.g. Firefox, keep their selection but do not show it until
    // the widget gets focus again, but it is not clear if this is better, or if
    // is consistent with macOS behavior)
    auto idx = mImpl->editor.selection().start;
    mImpl->editor.setSelection(TextEditorLogic::Selection(idx));

    // Call editor.onTextCommitted() so that we do not duplicate code and do not
    // need to have the code be a function in the class declaration. We assigned
    // a callback to this in the constructor.
    if (mImpl->editor.onTextCommitted) {
        mImpl->editor.onTextCommitted();
    }
}

void StringEdit::themeChanged(const Theme& theme)
{
    Super::themeChanged(theme);
    mImpl->editor.setNeedsLayout();
}

void StringEdit::draw(UIContext& context)
{
    // If the window becomes inactive or active AND there is a selection, then relayout so
    // that the selection color (which must be stored in the TextLayout) changes.
    if (mImpl->windowWasActiveLastDraw != context.isWindowActive) {
        auto sel = mImpl->editor.selection();
        if (sel.start != sel.end) {
            mImpl->editor.setNeedsLayout();
        }
    }
    mImpl->windowWasActiveLastDraw = context.isWindowActive;

    // mouse() and key() do not have access to the DrawContext, so we need to postpone
    // layout until the draw.
    if (mImpl->editor.needsLayout() || mImpl->editor.layoutDPI() != context.dc.dpi()) {
        auto s = context.theme.textEditStyle(context, style(themeState()), themeState());
        auto w = kDimGrow;
        if (!mImpl->isSingleLine) {
            auto margin = context.theme.calcPreferredTextMargins(context.dc,
                                                                 mImpl->font(context.theme)).width;
            w = bounds().width - 2.0f * margin;
        }
        mImpl->editor.layoutText(context.dc, mImpl->font(context.theme), s.fgColor,
                                 context.theme.params().accentedBackgroundTextColor, w);
    }

    // If we are in password mode, update the display StringEditorLogic to display
    // bullets instead of the actual characters.
    if (mImpl->passwordDisplay && (mImpl->passwordDisplay->needsLayout() ||
                                   mImpl->passwordDisplay->layoutDPI() != context.dc.dpi())) {
        // We need to replace each glyph with a bullet, not each byte; this is UTF-8!
        auto idx2cp = codePointIndicesForUTF8Indices(mImpl->editor.string().c_str());
        const int nCodePts = (idx2cp.empty() ? 0 : int(idx2cp.back()) + 1);  // since this is a vector index, size() > nCodePts
        std::string passwordStr;
        for (size_t i = 0;  i < nCodePts;  ++i) {
            passwordStr += char(0xe2);
            passwordStr += char(0x80);
            passwordStr += char(0xa2);
        }
        idx2cp.push_back(nCodePts);  // selection can go to nCodePt
        auto sel = mImpl->editor.selection();
        sel.start = 3 * idx2cp[std::max(0, sel.start)];
        sel.end = 3 * idx2cp[std::max(0, sel.end)];
        mImpl->passwordDisplay->setString(passwordStr);
        mImpl->passwordDisplay->setSelection(sel);

        auto s = context.theme.textEditStyle(context, style(themeState()), themeState());
        mImpl->passwordDisplay->layoutText(context.dc, mImpl->font(context.theme), s.fgColor,
                                           context.theme.params().accentedBackgroundTextColor, kDimGrow);
    }
    auto *editor = &mImpl->editor;
    if (mImpl->passwordDisplay) {
        editor = mImpl->passwordDisplay.get();
    }

    // mouse() and key() will change the selection (since the caret is a selection)
    // if anything changes, but if the layout changed we cannot do the calculation
    // until afterwards. If we have focus, assume that any draw is because of a
    // change from user input.
    if (focused()) {
        mImpl->scrollOffset = calcScrollOffset(context.dc, mImpl->font(context.theme),
                                               *editor, bounds().size(),
                                               (mImpl->alignment & Alignment::kHorizMask),
                                               context.theme.calcPreferredTextMargins(context.dc, mImpl->font(context.theme)),
                                               mImpl->scrollOffset);
    }

    auto alignOffset = calcAlignmentOffset(*editor, mImpl->editorTextRect, mImpl->alignment).x;
    context.theme.drawTextEdit(context, bounds(), Point(alignOffset, PicaPt::kZero) + mImpl->scrollOffset,
                               mImpl->placeholder, *editor, mImpl->alignment,
                               style(themeState()), themeState(), focused());

    Super::draw(context);
}

}  // namespace uitk
