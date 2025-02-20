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

#include "StringEditorLogic.h"

#include "Application.h"
#include "Clipboard.h"
#include "Widget.h"
#include "private/Utils.h"

#include <nativedraw.h>

namespace uitk {

namespace {

bool isWordChar(char c)
{
    return ((c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z'));
}

} // namespace

struct StringEditorLogic::Impl
{
    std::string stringUTF8;
    Selection selection = Selection(0);
    IMEConversion imeConversion = IMEConversion();
    std::shared_ptr<TextLayout> layout;
    float layoutDPI = 0;
    PicaPt layoutLineHeight = PicaPt(12.0f);
    bool needsLayout = true;
};

StringEditorLogic::StringEditorLogic()
    : mImpl(new Impl())
{
}

StringEditorLogic::~StringEditorLogic()
{
}

const std::string& StringEditorLogic::string() const
{
    return mImpl->stringUTF8;
}

void StringEditorLogic::setString(const std::string& utf8)
{
    mImpl->stringUTF8 = utf8;
    setSelection(Selection(Index(mImpl->stringUTF8.size())));
    mImpl->needsLayout = true;
}

bool StringEditorLogic::isEmpty() const
{
    return mImpl->stringUTF8.empty();
}

StringEditorLogic::Index StringEditorLogic::size() const
{
    return Index(mImpl->stringUTF8.size());
}

std::string StringEditorLogic::textForRange(Index start, Index end) const
{
    return mImpl->stringUTF8.substr(size_t(start), size_t(end - start));
}

void StringEditorLogic::insertText(Index i, const std::string& utf8)
{
    mImpl->stringUTF8.insert(i, utf8);
    mImpl->needsLayout = true;
}

void StringEditorLogic::deleteText(Index start, Index end)
{
    mImpl->stringUTF8.erase(mImpl->stringUTF8.begin() + start,
                            mImpl->stringUTF8.begin() + end);
    mImpl->needsLayout = true;
}

TextEditorLogic::Index StringEditorLogic::startOfText() const
{
    return 0;
}

TextEditorLogic::Index StringEditorLogic::endOfText() const
{
    return Index(mImpl->stringUTF8.size());
}

TextEditorLogic::Index StringEditorLogic::prevChar(Index i) const
{
    return prevCodePointUtf8(mImpl->stringUTF8.c_str(), i);
}

TextEditorLogic::Index StringEditorLogic::nextChar(Index i) const
{
    if (i >= Index(mImpl->stringUTF8.size())) {
        return Index(mImpl->stringUTF8.size());
    }
    
    return nextCodePointUtf8(mImpl->stringUTF8.c_str(), i);
}

TextEditorLogic::Index StringEditorLogic::startOfWord(Index i) const
{
    if (i <= 0) {
        return 0;
    }

    // If we are in-between words, find the end of the previous one...
    while (i > 0 && !isWordChar(mImpl->stringUTF8[i - 1])) {
        i--;
    }
    // ...and find the start.
    while (i > 0 && isWordChar(mImpl->stringUTF8[i - 1])) {
        i--;
    }
    return i;
}

TextEditorLogic::Index StringEditorLogic::endOfWord(Index i) const
{
    auto end = Index(mImpl->stringUTF8.size());
    if (i >= end) {
        return end;
    }

    // If we are in-between words, find the start of the next one...
    while (i < end && !isWordChar(mImpl->stringUTF8[i])) {
        i++;
    }
    // ...and find the end.
    while (i < end && isWordChar(mImpl->stringUTF8[i])) {
        i++;
    }
    return i;
}

TextEditorLogic::Index StringEditorLogic::startOfLine(Index i) const
{
    if (i <= 0) {
        return 0;
    }

    PicaPt epsilon(0.001f);
    auto &glyphs = mImpl->layout->glyphs();
    auto glyphIdx = mImpl->layout->glyphIndexAtIndex(i);
    if (glyphIdx == 0) {  // i must be in middle of first glyph (i.e. invalid i), so SOL is 0
        return 0;
    }
    assert(glyphIdx != 0 && glyphIdx < long(glyphs.size()));
    PicaPt x;
    if (glyphIdx >= 0) {
        x = glyphs[glyphIdx].frame.x;
    } else {
        x = glyphs.back().frame.maxX();
        glyphIdx = long(glyphs.size());  // will always be decremented before using, so size() is okay
    }
    while (i > 0 && mImpl->stringUTF8[i - 1] != '\n' && (glyphs[glyphIdx - 1].frame.x - x) < epsilon) {
        i = int(glyphs[--glyphIdx].index);
        x = glyphs[glyphIdx].frame.x;
    }
    return i;
}

TextEditorLogic::Index StringEditorLogic::endOfLine(Index i) const
{
    auto end = Index(mImpl->stringUTF8.size());
    if (i >= end) {
        return end;
    }

    PicaPt epsilon(0.001f);
    auto &glyphs = mImpl->layout->glyphs();
    auto glyphIdx = mImpl->layout->glyphIndexAtIndex(i);
    assert(glyphIdx >= 0 && glyphIdx < long(glyphs.size()));
    auto x = glyphs[glyphIdx].frame.x;
    while (i < end && mImpl->stringUTF8[i] != '\n' && (glyphs[glyphIdx].frame.x - x) > -epsilon) {
        x = glyphs[glyphIdx].frame.x;
        i = int(glyphs[glyphIdx++].indexOfNext);
    }
    return i;
}

bool StringEditorLogic::needsLayout() const
{
    return mImpl->needsLayout;
}

void StringEditorLogic::setNeedsLayout() const
{
    mImpl->needsLayout = true;
}

void StringEditorLogic::layoutText(const DrawContext& dc, const Font& font,
                                   const Color& color, const Color& selectedColor,
                                   const PicaPt& width)
{
    if (mImpl->imeConversion.isEmpty()) {
        Text t(mImpl->stringUTF8, font, color);
        // Note: selection should be empty if there is IME text
        if (mImpl->selection.start != mImpl->selection.end && selectedColor.toRGBA() != color.toRGBA()) {
            t.setColor(selectedColor, mImpl->selection.start, mImpl->selection.end - mImpl->selection.start);
        }
        mImpl->layout = dc.createTextLayout(t, Size(width, Widget::kDimGrow));
    } else {
        Text t(textWithConversion(), font, color);
        t.setUnderlineStyle(kUnderlineSingle, mImpl->imeConversion.start, int(mImpl->imeConversion.text.size()));
        mImpl->layout = dc.createTextLayout(t, Size(width, Widget::kDimGrow));
    }
    mImpl->layoutDPI = dc.dpi();
    mImpl->layoutLineHeight = font.pointSize();
    mImpl->needsLayout = false;
}

const TextLayout* StringEditorLogic::layout() const { return mImpl->layout.get(); }

float StringEditorLogic::layoutDPI() const
{
    return mImpl->layoutDPI;
}

Rect StringEditorLogic::glyphRectAtIndex(Index i) const
{
    // Note that i >= mImpl->stringUTF8.size() is okay (and expected)

    if (mImpl->stringUTF8.empty()) {
        return Rect(PicaPt::kZero, PicaPt::kZero, PicaPt::kZero, mImpl->layoutLineHeight);
    }

    if (mImpl->layout) {
        auto *glyph = mImpl->layout->glyphAtIndex(long(i));
        if (glyph) {
            return glyph->frame;
        } else {
            auto &glyphs = mImpl->layout->glyphs();
            if (!glyphs.empty()) {
                return Rect(glyphs.back().frame.maxX(), glyphs.back().frame.y,
                            PicaPt::kZero, glyphs.back().frame.height);
            } else {
                return Rect(PicaPt::kZero, PicaPt::kZero, PicaPt::kZero, mImpl->layoutLineHeight);
            }
        }
    }

    return Rect(PicaPt::kZero, PicaPt::kZero, PicaPt::kZero, mImpl->layoutLineHeight);
}

Point StringEditorLogic::pointAtIndex(Index i) const
{
    // Note that i >= mImpl->stringUTF8.size() is okay (and expected); pointAtIndex()
    // will return the farthest side of the last glyph.
    if (mImpl->layout && i >= 0) {
        return mImpl->layout->pointAtIndex(long(i));
    }
    return Point(PicaPt::kZero, PicaPt::kZero);
}

TextEditorLogic::Index StringEditorLogic::indexAtPoint(const Point& p) const
{
    if (mImpl->layout) {
        auto *g = mImpl->layout->glyphAtPoint(p);
        if (g) {
            if (p.x < g->frame.midX()) {
                return Index(g->index);
            } else {
                return Index(g->indexOfNext);
            }
        }
    }
    return kInvalidIndex;
}

TextEditorLogic::Selection StringEditorLogic::selection() const
{
    return mImpl->selection;
}

void StringEditorLogic::setSelection(const Selection& sel)
{
    // The color of selected text might be different than unselected text, so if
    // re-layout if either the new or old selection length > 0. (If it is zero,
    // we are just drawing the caret, so no need to update anything, and it is
    // relatively expensive to recreate text)
    if (mImpl->selection.start < mImpl->selection.end || sel.start < sel.end) {
        mImpl->needsLayout = true;
    }

    mImpl->selection = sel;
    if (sel.start < sel.end) {
        auto &clip = Application::instance().clipboard();
        if (clip.supportsX11SelectionString()) {
            clip.setX11SelectionString(textForRange(sel.start, sel.end));
        }
    }
}

StringEditorLogic::IMEConversion StringEditorLogic::imeConversion() const
{
    return mImpl->imeConversion;
}

void StringEditorLogic::setIMEConversion(const IMEConversion& conv)
{
    assert(conv.text.empty() || conv.start >= 0);

    mImpl->imeConversion = conv;
    mImpl->needsLayout = true;
}

std::string StringEditorLogic::textWithConversion() const
{
    std::string s = mImpl->stringUTF8;  // copy
    auto sel = selection();
    s.replace(sel.start, sel.end - sel.start, mImpl->imeConversion.text);
    return s;
}

Point StringEditorLogic::textUpperLeft() const
{
    return Point::kZero;
}

}  // namespace uitk
