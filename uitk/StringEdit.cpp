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

#include "StringEdit.h"

#include "Events.h"
#include "MenuUITK.h"
#include "StringEditorLogic.h"
#include "UIContext.h"
#include "Window.h"

namespace uitk {

PicaPt calcScrollOffset(const StringEditorLogic& editor, const Size& viewSize,
                        int horizAlign, const PicaPt& currentScrollX)
{
    auto sel = editor.selection();
    auto idx = sel.cursorIndex(0);
    if (!editor.imeConversion().isEmpty()) {
        idx += editor.imeConversion().cursorOffset;
    }
    auto r = Rect(PicaPt::kZero, PicaPt::kZero, viewSize.width, viewSize.height);
    auto textWidth = editor.layout()->metrics().width;
    if (textWidth <= r.width) {
        return PicaPt::kZero;
    }
    PicaPt textStartX;
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
    auto cursorPt = Point(textStartX, PicaPt::kZero) +
                    Point(editor.pointAtIndex(idx).x, 0.5f * viewSize.height);
    if (r.contains(cursorPt + Point(currentScrollX, PicaPt::kZero))) {
        // If we deleted characters from the right (subtract off 1 pt in case of roundoff errors)
        if (textWidth > r.width && textStartX + textWidth + currentScrollX < r.maxX() - PicaPt(1)) {
            return r.width - textWidth;
        }
        // If we deleted characters from the left (subtract off 1 pt in case of roundoff errors)
        if (textWidth > r.width && textStartX + currentScrollX > r.minX() + PicaPt(1)) {
            return PicaPt::kZero;
        }
        // Otherwise, no change is needed
        return currentScrollX;
    } else {
        if (sel.start == sel.end) {
            if (cursorPt.x + currentScrollX < r.x) {
                return -cursorPt.x;
            } else {
                return -(cursorPt.x - viewSize.width);
            }
        } else {
            if (sel.cursorLoc == TextEditorLogic::Selection::CursorLocation::kStart) {
                return -cursorPt.x;
            } else {
                return -(cursorPt.x - viewSize.width);
            }
        }
    }
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

struct StringEdit::Impl
{
    StringEditorLogic editor;
    std::string placeholder;
    int alignment = Alignment::kLeft | Alignment::kVCenter;
    Rect editorTextRect;
    PicaPt scrollOffset = PicaPt::kZero;
    MenuUITK *popup = nullptr;  // we own this
    std::function<void(const std::string&)> onTextChanged;
    std::function<void(StringEdit*)> onValueChanged;
    bool textHasChanged = false;
};

StringEdit::StringEdit()
    : mImpl(new Impl())
{
    mImpl->editor.onTextChanged = [this]() {
        mImpl->textHasChanged = true;
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

int StringEdit::alignment() const { return mImpl->alignment; }

StringEdit* StringEdit::setAlignment(int alignment)
{
    alignment = (alignment & Alignment::kHorizMask);
    mImpl->alignment = alignment | (mImpl->alignment & Alignment::kVertMask);
    setNeedsDraw();
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

CutPasteable * StringEdit::asCutPasteable()
{
    return &mImpl->editor;
}

TextEditorLogic* StringEdit::asTextEditorLogic()
{
    return &mImpl->editor;
}

Size StringEdit::preferredSize(const LayoutContext& context) const
{
    return context.theme.calcPreferredTextEditSize(context.dc, context.theme.params().labelFont);
}

void StringEdit::layout(const LayoutContext& context)
{
    Super::layout(context);
    mImpl->editorTextRect = context.theme.calcTextEditRectForFrame(bounds(), context.dc,
                                                                   context.theme.params().labelFont);
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
    bool consumed = false;

    auto me = e;
    me.pos = e.pos - mImpl->editorTextRect.upperLeft() -
             calcAlignmentOffset(mImpl->editor, mImpl->editorTextRect, mImpl->alignment) -
             Point(mImpl->scrollOffset, PicaPt::kZero);

    if (e.type == MouseEvent::Type::kButtonDown && e.button.button == MouseButton::kLeft) {
        if (e.button.nClicks == 1) {
            if (auto *w = window()) {
                if (w->focusWidget() != this) {
                    w->setFocusWidget(this);
                }
            }
        }
        consumed = mImpl->editor.handleMouseEvent(me);
    } else if (e.type == MouseEvent::Type::kDrag) {
        consumed = mImpl->editor.handleMouseEvent(me);
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
                mImpl->editor.setSelection(TextEditorLogic::Selection(
                                    mImpl->editor.startOfText(),
                                    mImpl->editor.endOfText(),
                                    TextEditorLogic::Selection::CursorLocation::kUndetermined));
            }

            // Show the popup
            auto sel = mImpl->editor.selection();
            if (mImpl->popup) {
                mImpl->popup->cancel();
                delete mImpl->popup;
            }
            mImpl->popup = new MenuUITK();
            mImpl->popup->addItem("Cut", 1,
                                  [this]() { mImpl->editor.cutToClipboard(); setNeedsDraw(); });
            mImpl->popup->addItem("Copy", 2, [this]() { mImpl->editor.copyToClipboard(); });
            mImpl->popup->addItem("Paste", 3,
                                  [this]() { mImpl->editor.pasteFromClipboard(); setNeedsDraw(); });
            bool canCopy = (sel.start < sel.end);
            mImpl->popup->setItemEnabled(1, canCopy);
            mImpl->popup->setItemEnabled(2, canCopy);
            mImpl->popup->show(w, convertToWindowFromLocal(e.pos));

            consumed = true;
        }
    } else if (e.type == MouseEvent::Type::kButtonDown && e.button.button == MouseButton::kMiddle && e.button.nClicks == 1) {
        // For middle-click paste on X11
        consumed = mImpl->editor.handleMouseEvent(me);
    }

    if (consumed) {
        setNeedsDraw();
        return Widget::EventResult::kConsumed;
    } else {
        return Super::mouse(e);
    }
}

void StringEdit::key(const KeyEvent& e)
{
    if (mImpl->editor.handleKeyEvent(e)) {
        setNeedsDraw();
    }
}


void StringEdit::text(const TextEvent& e)
{
    mImpl->editor.handleTextEvent(e);
    setNeedsDraw();
}

void StringEdit::keyFocusEnded()
{
    // Call editor.onTextCommitted() so that we do not duplicate code and do not
    // need to have the code be a function in the class declaration. We assigned
    // a callback to this in the constructor.
    if (mImpl->editor.onTextCommitted) {
        mImpl->editor.onTextCommitted();
    }
}

void StringEdit::draw(UIContext& context)
{
    // mouse() and key() do not have access to the DrawContext, so we need to postpone
    // layout until the draw.
    if (mImpl->editor.needsLayout() || mImpl->editor.layoutDPI() != context.dc.dpi()) {
        auto s = context.theme.textEditStyle(style(themeState()), themeState());
        mImpl->editor.layoutText(context.dc, context.theme.params().labelFont, s.fgColor, PicaPt(1e6));
    }
    // mouse() and key() will change the selection (since the caret is a selection)
    // if anything changes, but if the layout changed we cannot do the calculation
    // until afterwards. If we have focus, assume that any draw is because of a
    // change from user input.
    if (focused()) {
        mImpl->scrollOffset = calcScrollOffset(mImpl->editor, mImpl->editorTextRect.size(),
                                               (mImpl->alignment & Alignment::kHorizMask),
                                               mImpl->scrollOffset);
    }

    auto alignOffset = calcAlignmentOffset(mImpl->editor, mImpl->editorTextRect, mImpl->alignment).x;
    context.theme.drawTextEdit(context, bounds(), alignOffset + mImpl->scrollOffset,
                               mImpl->placeholder, mImpl->editor, mImpl->alignment,
                               style(themeState()), themeState(), focused());
    Super::draw(context);
}

}  // namespace uitk
