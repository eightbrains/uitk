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

#include "TextEditorLogic.h"

#include "Application.h"
#include "Clipboard.h"
#include "Cursor.h"
#include "Events.h"
#include "Menu.h"
#include "Window.h"

namespace uitk {

struct TextEditorLogic::Impl
{
    Point mouseDownPt;
    Index dragPivotIndex = Index(0);
};

TextEditorLogic::TextEditorLogic()
    : mImpl(new Impl())
{
}

TextEditorLogic::~TextEditorLogic()
{
}

void TextEditorLogic::handleMouseEntered(Window *w)
{
    w->pushCursor(Cursor::iBeam());
}

void TextEditorLogic::handleMouseExited(Window *w)
{
    w->popCursor();
}

bool TextEditorLogic::handleMouseEvent(const MouseEvent& e)
{
    auto calcIndex = [this](const Point& p) {
        auto idx = indexAtPoint(p);
        if (idx == kInvalidIndex) {
            idx = indexAtPoint(Point(p.x, PicaPt::kZero));
            if (idx == TextEditorLogic::kInvalidIndex) {
                if (p.x <= PicaPt::kZero) {
                    idx = startOfText();
                } else {
                    idx = endOfText();
                }
            }
        }
        return idx;
    };

    if (e.type == MouseEvent::Type::kButtonDown && e.button.button == MouseButton::kLeft) {
        bool hasShift = (e.keymods & int(KeyModifier::kShift));
        if (e.button.nClicks == 1) {
            mImpl->mouseDownPt = e.pos;
            auto idx = calcIndex(e.pos);
            mImpl->dragPivotIndex = idx;
            if (hasShift) {
                auto sel = selection();
                auto start = sel.start;
                auto end = sel.end;
                auto mid = sel.start + (sel.end - sel.start) / 2;
                if (idx < mid) {
                    start = idx;
                } else {
                    end = idx;
                }
                setSelection(Selection(start, end, (idx == start ? Selection::CursorLocation::kStart
                                                                 : Selection::CursorLocation::kEnd)));
            } else {
                setSelection(Selection(idx));
            }
        } else if (e.button.nClicks == 2 && !hasShift) {
            auto sel = selection();
            auto start = startOfWord(sel.start);
            auto end = endOfWord(sel.end);
            setSelection(Selection(start, end, Selection::CursorLocation::kUndetermined));
        } else if (e.button.nClicks == 3 && !hasShift) {
            auto sel = selection();
            auto start = startOfLine(sel.start);
            auto end = endOfLine(sel.end);
            setSelection(Selection(start, end, Selection::CursorLocation::kUndetermined));
        }
        return true;
    } else if (e.type == MouseEvent::Type::kDrag && e.drag.buttons == int(MouseButton::kLeft)) {
        if (std::abs((e.pos.x - mImpl->mouseDownPt.x).toPixels(72.0f)) > 1.0f ||
            std::abs((e.pos.y - mImpl->mouseDownPt.y).toPixels(72.0f)) > 1.0f)
        {
            auto idx = calcIndex(e.pos);
            if (idx >= mImpl->dragPivotIndex) {
                setSelection(Selection(mImpl->dragPivotIndex, idx, Selection::CursorLocation::kEnd));
            } else {
                setSelection(Selection(idx, mImpl->dragPivotIndex, Selection::CursorLocation::kStart));
            }
        }
        return true;
    } else if (e.type == MouseEvent::Type::kButtonDown && e.button.button == MouseButton::kMiddle) {
        if (e.keymods == 0 && e.button.nClicks == 1) {
            if (Application::instance().clipboard().supportsX11SelectionString()) {
                auto start = calcIndex(e.pos);
                auto selString = Application::instance().clipboard().x11SelectionString();
                insertText(start, selString);
                start += Index(selString.size());
                setSelection(Selection(start, start, Selection::CursorLocation::kEnd));
                if (onTextChanged) { onTextChanged(); }
            }
        }
    }

    // It seems like it would be nice to have right-click for a context menu
    // here, but we need to know the window for that, which we do not know.
    // Also, apart from copy/cut/paste and any system-specific items, we do
    // not know what else the widget might want to add to the menu. So we
    // leave this to the widget to handle.

    return false;
}

bool TextEditorLogic::handleKeyEvent(const KeyEvent& e)
{
    if (e.type != KeyEvent::Type::kKeyDown) {  // we do not need to process key up events
        return false;
    }

    auto selMode = ((e.keymods & KeyModifier::kShift) ? SelectionMode::kExtend : SelectionMode::kReplace);
    bool isWordMod = false;
    bool isLineMod = false;
    bool isCommandMod = (e.keymods & (int(KeyModifier::kCtrl) |
                                      int(KeyModifier::kAlt) |
                                      int(KeyModifier::kMeta)));
#if __APPLE__
    isWordMod = (!(e.keymods & KeyModifier::kCtrl) &&
                 ((e.keymods & KeyModifier::kMeta) || (e.keymods & KeyModifier::kAlt)));
    isLineMod = ((e.keymods & KeyModifier::kCtrl) && !isWordMod);
#else
    isWordMod = (e.keymods & KeyModifier::kCtrl);
    isLineMod = false;  // Windows/Linux uses home/end
#endif // __APPLE__

    if (imeConversion().isEmpty()) {  // OS will handle editing the IME text
        switch (e.key) {
            case Key::kBackspace:
                if (isWordMod) {
                    deleteBackToWordStart();
                } else if (isLineMod) {
                    deleteBackToLineStart();
                } else {
                    deletePrevChar();
                }
                if (onTextChanged) { onTextChanged(); }
                break;
            case Key::kDelete:
                if (isWordMod) {
                    deleteForwardToWordEnd();
                } else if (isLineMod) {
                    deleteForwardToLineEnd();
                } else {
                    deleteNextChar();
                }
                if (onTextChanged) { onTextChanged(); }
                break;
            case Key::kLeft:
                if (isWordMod) {
                    moveToPrevWord(selMode);
                } else if (isLineMod) {
                    moveToLineStart(selMode);
                } else {
                    moveToPrevChar(selMode);
                }
                break;
            case Key::kRight:
                if (isWordMod) {
                    moveToNextWord(selMode);
                } else if (isLineMod) {
                    moveToLineEnd(selMode);
                } else {
                    moveToNextChar(selMode);
                }
                break;
            case Key::kUp:
                if (e.keymods & KeyModifier::kCtrl) {
                    moveToStart(selMode);
                } else {
                    moveOneLineUp(selMode);
                }
                break;
            case Key::kDown:
                if (e.keymods & KeyModifier::kCtrl) {
                    moveToEnd(selMode);
                } else {
                    moveOneLineDown(selMode);
                }
                break;
            case Key::kHome:
                if (e.keymods & KeyModifier::kCtrl) {
                    moveToStart(selMode);
                } else {
                    moveToLineStart(selMode);
                }
                break;
            case Key::kEnd:
                if (e.keymods & KeyModifier::kCtrl) {
                    moveToEnd(selMode);
                } else {
                    moveToLineEnd(selMode);
                }
                break;
            case Key::kEnter:
            case Key::kReturn:
                if (onTextCommitted) {
                    onTextCommitted();
                }
                break;
            default:
                break;
        }
    }
    return true;
}

void TextEditorLogic::handleTextEvent(const TextEvent& e)
{
    insertText(e.utf8);
    if (onTextChanged) {
        onTextChanged();
    }
}

void TextEditorLogic::insertText(const std::string& utf8)
{
    auto sel = selection();
    if (sel.start != sel.end) {
        deleteSelection();
    }
    insertText(sel.start, utf8);
    setSelection(Selection(sel.start + Index(utf8.size())));
}

void TextEditorLogic::deleteSelection()
{
    auto sel = selection();
    if (sel.start < sel.end) {
        deleteText(sel.start, sel.end);
    }
    setSelection(Selection(sel.start));
}

void TextEditorLogic::deletePrevChar()
{
    deleteBackTo(prevChar(selection().cursorIndex(-1)));
}

void TextEditorLogic::deleteNextChar()
{
    deleteForwardTo(nextChar(selection().cursorIndex(1)));
}

void TextEditorLogic::deleteBackToWordStart()
{
    auto currIdx = selection().cursorIndex(-1);
    auto wordStartIdx = startOfWord(currIdx);
    if (wordStartIdx == currIdx) {
        wordStartIdx = endOfWord(prevChar(currIdx));
    }
    deleteBackTo(wordStartIdx);
}

void TextEditorLogic::deleteForwardToWordEnd()
{
    auto currIdx = selection().cursorIndex(1);
    auto wordEndIdx = endOfWord(currIdx);
    if (wordEndIdx == currIdx) {
        wordEndIdx = endOfWord(nextChar(currIdx));
    }
    deleteForwardTo(wordEndIdx);
}

void TextEditorLogic::deleteBackToLineStart()
{
    deleteBackTo(startOfLine(selection().cursorIndex(-1)));
}

void TextEditorLogic::deleteForwardToLineEnd()
{
    deleteForwardTo(endOfLine(selection().cursorIndex(1)));
}

void TextEditorLogic::deleteBackTo(Index i)
{
    auto sel = selection();
    if (sel.start == sel.end) {
        if (i < sel.start) {
            deleteText(i, sel.start);
            setSelection(Selection(i));
        }
    } else {
        deleteSelection();
    }
}

void TextEditorLogic::deleteForwardTo(Index i)
{
    auto sel = selection();
    if (sel.start == sel.end) {
        if (i > sel.end) {
            deleteText(sel.end, i);
            // Selection remains the same, we deleted forward
        }
    } else {
        deleteSelection();
    }
}

void TextEditorLogic::moveToStart(SelectionMode mode)
{
    Index idx = startOfText();
    if (mode == SelectionMode::kReplace) {
        setSelection(Selection(idx));
    } else {
        // Since we cannot move previous, the only useful place for the cursor
        // location is the end.
        setSelection(Selection(idx, selection().end, Selection::CursorLocation::kEnd));
    }
}

void TextEditorLogic::moveToEnd(SelectionMode mode)
{
    Index idx = endOfText();
    if (mode == SelectionMode::kReplace) {
        setSelection(Selection(idx));
    } else {
        // Since we cannot move next, the only useful place for the cursor
        // location is the start.
        setSelection(Selection(selection().start, idx, Selection::CursorLocation::kStart));
    }
}

void TextEditorLogic::moveToPrevChar(SelectionMode mode)
{
    if (mode == SelectionMode::kReplace && selection().end > selection().start) {
        moveToLocation(selection().start, mode);
    } else {
        moveToLocation(prevChar(selection().cursorIndex(-1)), mode);
    }
}

void TextEditorLogic::moveToNextChar(SelectionMode mode)
{
    if (mode == SelectionMode::kReplace && selection().end > selection().start) {
        moveToLocation(selection().end, mode);
    } else {
        moveToLocation(nextChar(selection().cursorIndex(1)), mode);
    }
}

void TextEditorLogic::moveToPrevWord(SelectionMode mode)
{
    auto currIdx = selection().cursorIndex(-1);
    if (currIdx == 0) {
        return;
    }
    auto wordStartIdx = startOfWord(currIdx);
    if (wordStartIdx == currIdx) {
        wordStartIdx = endOfWord(prevChar(currIdx));
    }
    moveToLocation(wordStartIdx, mode);
}

void TextEditorLogic::moveToNextWord(SelectionMode mode)
{
    auto currIdx = selection().cursorIndex(1);
    auto wordEndIdx = endOfWord(currIdx);
    if (wordEndIdx == currIdx) {
        wordEndIdx = endOfWord(nextChar(currIdx));
    }
    moveToLocation(wordEndIdx, mode);
}

void TextEditorLogic::moveToLineStart(SelectionMode mode)
{
    moveToLocation(startOfLine(selection().cursorIndex(-1)), mode);
}

void TextEditorLogic::moveToLineEnd(SelectionMode mode)
{
    moveToLocation(endOfLine(selection().cursorIndex(1)), mode);
}

void TextEditorLogic::moveOneLineUp(SelectionMode mode)
{
    moveToLocation(lineAbove(selection().cursorIndex(-1)), mode);
}

void TextEditorLogic::moveOneLineDown(SelectionMode mode)
{
    moveToLocation(lineBelow(selection().cursorIndex(1)), mode);
}

void TextEditorLogic::moveToLocation(Index i, SelectionMode mode)
{
    if (mode == SelectionMode::kReplace) {
        setSelection(Selection(i));
    } else {
        auto sel = selection();
        if (sel.cursorLoc == Selection::CursorLocation::kUndetermined) {
            if (i < sel.start) {
                sel.cursorLoc = Selection::CursorLocation::kStart;
            } else {
                sel.cursorLoc = Selection::CursorLocation::kEnd;
            }
        }
        if (sel.cursorLoc == Selection::CursorLocation::kStart) {
            if (i <= sel.end) {
                setSelection(Selection(i, sel.end, sel.cursorLoc));
            } else {
                setSelection(Selection(sel.end, i, Selection::CursorLocation::kEnd));
            }
        } else {
            if (i >= sel.start) {
                setSelection(Selection(sel.start, i, sel.cursorLoc));
            } else {
                setSelection(Selection(i, sel.start, Selection::CursorLocation::kStart));
            }
        }
    }
}

bool TextEditorLogic::canCopyNow() const
{
    auto sel = selection();
    return (sel.start < sel.end && sel.start >= 0);
}

void TextEditorLogic::copyToClipboard()
{
    auto sel = selection();
    if (sel.start < sel.end) {
        Application::instance().clipboard().setString(textForRange(sel.start, sel.end));
    }
}

void TextEditorLogic::cutToClipboard()
{
    auto sel = selection();
    if (sel.start < sel.end) {
        copyToClipboard();
        deleteSelection();
    }
    if (onTextChanged) {
        onTextChanged();
    }
}

void TextEditorLogic::pasteFromClipboard()
{
    auto &clipboard = Application::instance().clipboard();
    if (clipboard.hasString()) {
        auto clipString = clipboard.string();
        auto sel = selection();
        deleteSelection();
        insertText(sel.start, clipString);
        sel.start += Index(clipString.size());
        setSelection(Selection(sel.start, sel.start, Selection::CursorLocation::kEnd));
        if (onTextChanged) {
            onTextChanged();
        }
    }
}

}  // namespace uitk


