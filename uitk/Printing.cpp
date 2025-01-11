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

#include "Printing.h"

#define INCH(x) PicaPt(float(x)*72.0f)
#define MM(x) PicaPt(float(x)*72.0f/25.4f)

namespace uitk {

// Note that we cannot use PicaPt::kZero because it may not have been constructed yet
PaperSize PaperSize::kDefault(PicaPt(0.0f), PicaPt(0.0f), "default");

PaperSize PaperSize::kUSLetter(INCH(8.5), INCH(11.0), "US Letter");
PaperSize PaperSize::kUSLegal(INCH(8.5), INCH(14.0), "US Legal");
PaperSize PaperSize::kUSLedger(INCH(11.0), INCH(17.0), "US Ledger/Tabloid");
PaperSize PaperSize::kA0(MM(841),  MM(1189), "A0");
PaperSize PaperSize::kA1(MM(594),  MM(841),  "A1");
PaperSize PaperSize::kA2(MM(420),  MM(594),  "A2");
PaperSize PaperSize::kA3(MM(297),  MM(420),  "A3");
PaperSize PaperSize::kA4(MM(210),  MM(297),  "A4");
PaperSize PaperSize::kA5(MM(148),  MM(210),  "A5");
PaperSize PaperSize::kA6(MM(105),  MM(148),  "A6");
PaperSize PaperSize::kA7(MM(74),   MM(105),  "A7");
PaperSize PaperSize::kB0(MM(1000), MM(1414), "B0");
PaperSize PaperSize::kB1(MM(707),  MM(1000), "B1");
PaperSize PaperSize::kB2(MM(500),  MM(707),  "B2");
PaperSize PaperSize::kB3(MM(353),  MM(500),  "B3");
PaperSize PaperSize::kB4(MM(250),  MM(353),  "B4");
PaperSize PaperSize::kB5(MM(176),  MM(250),  "B5");
PaperSize PaperSize::kB6(MM(125),  MM(176),  "B6");
PaperSize PaperSize::kB7(MM(88),   MM(125),  "B7");

std::vector<PaperSize> PaperSize::knownSizes()
{
    return {
        PaperSize::kUSLetter,
        PaperSize::kUSLegal,
        PaperSize::kUSLedger,
        PaperSize::kA0,
        PaperSize::kA1,
        PaperSize::kA2,
        PaperSize::kA3,
        PaperSize::kA4,
        PaperSize::kA5,
        PaperSize::kA6,
        PaperSize::kA7,
        PaperSize::kB0,
        PaperSize::kB1,
        PaperSize::kB2,
        PaperSize::kB3,
        PaperSize::kB4,
        PaperSize::kB5,
        PaperSize::kB6,
        PaperSize::kB7,
    };
}

PrintSettings::PrintSettings()
    : paperSize(PaperSize::kDefault)
    , orientation(PaperOrientation::kPortrait)
{
}

} // namespace uitk
