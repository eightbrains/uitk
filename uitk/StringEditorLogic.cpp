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

#include "StringEditorLogic.h"

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
    std::shared_ptr<TextLayout> layout;
    float layoutDPI = 0;
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
    if (i <= 0) {
        return 0;
    }

    // See UTF8 encoding table in nextChar(). If byte is 10xxxxxx it is part
    // of a multibyte code point, so keep going.
    i -= 1;
    while (i >= 0 && (unsigned char)mImpl->stringUTF8[i] >= 0x80 && (unsigned char)mImpl->stringUTF8[i] < 0xc0) {
        i -= 1;
    }
    return i;
}

TextEditorLogic::Index StringEditorLogic::nextChar(Index i) const
{
    if (i >= Index(mImpl->stringUTF8.size())) {
        return Index(mImpl->stringUTF8.size());
    }
    
    // UTF8 encoding is:
    // 0x0000 - 0x007f:  0xxxxxxx
    // 0x0080 - 0x07ff:  110xxxxx 10xxxxxx
    // 0x0800 - 0xffff:  1110xxxx 10xxxxxx 10xxxxxx
    // 0x0080 - 0x07ff:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    int len = 1;
    if (((unsigned char)mImpl->stringUTF8[i] & 0b11000000) == (unsigned char)0b11000000) { len += 1; }
    if (((unsigned char)mImpl->stringUTF8[i] & 0b11100000) == (unsigned char)0b11100000) { len += 1; }
    if (((unsigned char)mImpl->stringUTF8[i] & 0b11110000) == (unsigned char)0b11110000) { len += 1; }
    return i + len;
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

    while (i > 0 && mImpl->stringUTF8[i - 1] != '\n') {
        i--;
    }
    return i;
}

TextEditorLogic::Index StringEditorLogic::endOfLine(Index i) const
{
    auto end = Index(mImpl->stringUTF8.size());
    if (i >= end) {
        return end;
    }

    while (i < end && mImpl->stringUTF8[i + 1] != '\n') {
        i++;
    }
    return i;
}

TextEditorLogic::Index StringEditorLogic::lineAbove(Index i) const
{
    return startOfText();
}

TextEditorLogic::Index StringEditorLogic::lineBelow(Index i) const
{
    return endOfText();
}

bool StringEditorLogic::needsLayout() const
{
    return mImpl->needsLayout;
}

void StringEditorLogic::layoutText(const DrawContext& dc, const Font& font,
                               const Color& color, const PicaPt& width)
{
    mImpl->layoutDPI = dc.dpi();
    mImpl->layout = dc.createTextLayout(mImpl->stringUTF8.c_str(), font, color, width);
    mImpl->needsLayout = false;
}

const TextLayout* StringEditorLogic::layout() const { return mImpl->layout.get(); }

float StringEditorLogic::layoutDPI() const
{
    return mImpl->layoutDPI;
}

Point StringEditorLogic::pointAtIndex(Index i) const
{
    if (mImpl->layout && i >= 0 && i <= Index(mImpl->stringUTF8.size())) {
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
    mImpl->selection = sel;
}

}  // namespace uitk
