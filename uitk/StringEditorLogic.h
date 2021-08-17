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

#ifndef UITK_STRING_EDITOR_LOGIC_H
#define UITK_STRING_EDITOR_LOGIC_H

#include "TextEditorLogic.h"

namespace uitk {

class StringEditorLogic : public TextEditorLogic
{
public:
    StringEditorLogic();
    virtual ~StringEditorLogic();

    const std::string& string() const;
    void setString(const std::string& utf8);

    bool isEmpty() const override;
    std::string textForRange(Index start, Index end) const override;

    void insertText(Index i, const std::string& utf8) override;
    void deleteText(Index start, Index end) override;

    Index startOfText() const override;
    Index endOfText() const override;
    Index prevChar(Index i) const override;
    Index nextChar(Index i) const override;
    Index startOfWord(Index i) const override;
    Index endOfWord(Index i) const override;
    Index startOfLine(Index i) const override;
    Index endOfLine(Index i) const override;
    Index lineAbove(Index i) const override;
    Index lineBelow(Index i) const override;

    bool needsLayout() const override;
    void layoutText(const DrawContext& dc, const Font& font, const Color& color, const PicaPt& width) override;
    const TextLayout* layout() const override;
    float layoutDPI() const override;

    Index indexAtPoint(const Point& p) const override;
    Point pointAtIndex(Index i) const override;

    Selection selection() const override;
    void setSelection(const Selection& sel) override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_STRING_EDITOR_LOGIC_H


