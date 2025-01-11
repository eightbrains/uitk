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

#ifndef UITK_PRINTING_H
#define UITK_PRINTING_H

#include <nativedraw.h>

#include <functional>
#include <string>

namespace uitk {

struct PrintContext;
struct LayoutContext;

enum class PaperOrientation {
    kPortrait = 0,
    kLandscape,
};

#define INCH(x) PicaPt(float(x)*72.0f)
#define MM(x) PicaPt(float(x)*72.0f/25.4f)

struct PaperSize
{
    static PaperSize kDefault;

    static PaperSize kUSLetter;
    static PaperSize kUSLegal;
    static PaperSize kUSLedger;
    static PaperSize kA0;
    static PaperSize kA1;
    static PaperSize kA2;
    static PaperSize kA3;
    static PaperSize kA4;
    static PaperSize kA5;
    static PaperSize kA6;
    static PaperSize kA7;
    static PaperSize kB0;
    static PaperSize kB1;
    static PaperSize kB2;
    static PaperSize kB3;
    static PaperSize kB4;
    static PaperSize kB5;
    static PaperSize kB6;
    static PaperSize kB7;

    static std::vector<PaperSize> knownSizes();

    PicaPt width;
    PicaPt height;
    std::string name;

    PaperSize() : PaperSize(PicaPt(0.0f), PicaPt(0.0f), "") {}

    PaperSize(const PicaPt& w, const PicaPt& h, const std::string& n)
        : width(w), height(h), name(n)
    {}
};

#undef INCH
#undef MM

struct PrintSettings
{
    /// Set if document size is known, otherwise the size will be taken
    /// from the print dialog. Default is PaperSize::kDefault.
    PaperSize paperSize;

    /// Default is Orientation::kPortrait.
    PaperOrientation orientation;

    /// Called after the print dialog completes, must return the number of
    /// pages in the document.
    std::function<int(const PaperSize&, const LayoutContext&)> calcPages;

    /// Called to draw each page.
    std::function<void(const PrintContext&)> drawPage;
};

} // namespace uitk
#endif // UITK_PRINTING_H
