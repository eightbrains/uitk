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

#ifndef UITK_TEXT_EDITOR_LOGIC_H
#define UITK_TEXT_EDITOR_LOGIC_H

#include "CutPasteable.h"

#include <functional>
#include <memory>
#include <string>

namespace uitk {

class Color;
class DrawContext;
class Font;
struct PicaPt;
struct Point;
struct Rect;
class TextLayout;
class Window;

struct KeyEvent;
struct MouseEvent;
struct TextEvent;

class TextEditorLogic : public CutPasteable
{
public:
    /// Note that it is not possible to advance one character by simply adding
    /// one to an index. Index is a byte offset into the UTF-8 text, and one
    /// glyph may be multiple bytes. Instead, used nextChar(), nextWord(), etc.
    using Index = int;

    static constexpr int kInvalidIndex = -1;

    struct Selection
    {
        Index start;  /// this is a byte index into the UTF-8 text
        Index end;    /// this is a byte index into the UTF-8 text
                      /// (and may go beyond the last byte if the selection
                      /// end is at the end of the text, in which case
                      /// end = utf8.size())

        enum class CursorLocation {
            kStart,        /// cursor movement is based off the start index
            kEnd,          /// cursor movement is based off the end index
            kUndetermined  /// We do not know where the user expects the cursor
                           /// to be yet. This should be used when a non-empty
                           /// selection is created in one instant (for instance,
                           /// double-clicking to select a word).
        };
        CursorLocation cursorLoc;

        explicit Selection(Index idx)
            : start(idx), end(idx), cursorLoc(CursorLocation::kEnd)
        {}

        Selection(Index s, Index e, CursorLocation loc)
            : start(s), end(e), cursorLoc(loc)
        {}

        Index cursorIndex(int dir) const {
            switch(cursorLoc) {
                case CursorLocation::kStart:
                    return start;
                case CursorLocation::kEnd:
                    return end;
                case CursorLocation::kUndetermined:
                    if (dir < 0) {
                        return start;
                    }
                    return end;
            }
            return Index(-1);  // cannot happen but makes MSVC happy
        }
    };

    // This is text that is currently in its temporary (usually phonetic)
    // form while the user is converting it to its final form. It should
    // replace the selection (including if the selection is empty, as
    // it represents the caret in that case). However, it has not been
    // committed by the user, so it should not be part of the value of
    // the control.
    struct IMEConversion
    {
        Index start;  // this is a byte index
        std::string text;
        // The cursor should be displayed at start + cursorOffset
        int cursorOffset = 0;  // this is a byte offset

        IMEConversion() : start(kInvalidIndex) {}
        IMEConversion(Index start_, const std::string& text_)
            : start(start_), text(text_), cursorOffset(int(text.size()))
        {}

        bool isEmpty() const { return text.empty(); }
    };

    TextEditorLogic();
    virtual ~TextEditorLogic();

    virtual bool isEmpty() const = 0;
    virtual Index size() const = 0;

    /// Returns the text in the range [start, end).
    virtual std::string textForRange(Index start, Index end) const = 0;

    /// Inserts the text, does not alter the selection.
    /// Design note: this is to make things simple for the implementor.
    virtual void insertText(Index i, const std::string& utf8) = 0;
    /// Deletes the text in the range [start, end), does not alter the selection.
    /// Design note: this is to make things simple for the implementor.
    virtual void deleteText(Index start, Index end) = 0;

    virtual Index startOfText() const = 0;
    virtual Index endOfText() const = 0;
    virtual Index prevChar(Index i) const = 0;
    virtual Index nextChar(Index i) const = 0;
    virtual Index startOfWord(Index i) const = 0;
    virtual Index endOfWord(Index i) const = 0;
    virtual Index startOfLine(Index i) const = 0;
    /// Returns the index after the last character (or space) in the line.
    virtual Index endOfLine(Index i) const = 0;
    virtual Index lineAbove(Index i) const;
    virtual Index lineBelow(Index i) const;

    virtual bool needsLayout() const = 0;
    /// Marks the text as needing to be re-created. This is normally only called
    /// internally, and should be called externally only in situations
    /// where the text's color has changed, such as the theme changed.
    virtual void setNeedsLayout() const = 0;
    virtual void layoutText(const DrawContext& dc, const Font& font, const Color& color,
                            const Color& selectedColor, const PicaPt& width) = 0;
    virtual const TextLayout* layout() const = 0;
    virtual float layoutDPI() const = 0;

    virtual Index indexAtPoint(const Point& p) const = 0;
    virtual Point pointAtIndex(Index i) const = 0;
    virtual Rect glyphRectAtIndex(Index i) const = 0;

    virtual Selection selection() const = 0;
    virtual void setSelection(const Selection& sel) = 0;

    virtual IMEConversion imeConversion() const = 0;
    virtual void setIMEConversion(const IMEConversion& conv) = 0;

    virtual std::string textWithConversion() const = 0;
    // Text rect, in widget coordinates. Used to position the input method editor window.
    virtual Point textUpperLeft() const = 0;

    /// Handles mouse events, except for right-click for context menu.
    /// Returns true if consumed the event.
    virtual void handleMouseEntered(Window *w);
    virtual void handleMouseExited(Window *w);
    virtual bool handleMouseEvent(const MouseEvent& e, bool isInFrame);
    enum ReturnKeyMode { kNewline, kCommits };
    virtual bool handleKeyEvent(const KeyEvent& e, ReturnKeyMode rkMode);  // returns true if consumed event
    virtual void handleTextEvent(const TextEvent& e);
    enum class SelectionMode { kReplace, kExtend };
    virtual void insertText(const std::string& utf8);
    virtual void deleteSelection();
    virtual void deletePrevChar();
    virtual void deleteNextChar();
    virtual void deleteBackToWordStart();
    virtual void deleteForwardToWordEnd();
    virtual void deleteBackToLineStart();
    virtual void deleteForwardToLineEnd();
    virtual void deleteBackTo(Index i);
    virtual void deleteForwardTo(Index i);
    virtual void moveToStart(SelectionMode mode);
    virtual void moveToEnd(SelectionMode mode);
    virtual void moveToPrevChar(SelectionMode mode);
    virtual void moveToNextChar(SelectionMode mode);
    virtual void moveToPrevWord(SelectionMode mode);
    virtual void moveToNextWord(SelectionMode mode);
    virtual void moveToLineStart(SelectionMode mode);
    virtual void moveToLineEnd(SelectionMode mode);
    virtual void moveOneLineUp(SelectionMode mode);
    virtual void moveOneLineDown(SelectionMode mode);
    virtual void moveToLocation(Index i, SelectionMode mode);
    virtual void commit();

    bool canCopyNow() const override;
    void copyToClipboard() override;
    void cutToClipboard() override;
    void pasteFromClipboard() override;

    // Called in response to a change in text due to a mouse or keyboard event.
    // Is not called by insertText, deleteText, or other direct modification
    // of the underlying text storage.
    std::function<void()> onTextChanged;
    // Called in response to Enter/Return.
    std::function<void()> onTextCommitted;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_TEXT_EDITOR_LOGIC_H

