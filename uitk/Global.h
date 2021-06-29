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

#ifndef UITK_GLOBAL_H
#define UITK_GLOBAL_H

namespace uitk {

struct Alignment {
    static const int kLeft = (1 << 0);
    static const int kHCenter = (1 << 1);
    static const int kRight = (1 << 2);
    static const int kTop = (1 << 4);
    static const int kVCenter = (1 << 5);
    static const int kBottom = (1 << 6);
    static const int kCenter = kHCenter | kVCenter;
    static const int kHorizMask = 0b00001111;
    static const int kVertMask =  0b11110000;
};

enum class Dir { kHoriz, kVert };

enum class SliderDir { kHoriz, kVertZeroAtTop, kVertZeroAtBottom };

}  // namespace uitk
#endif // UITK_GLOBAL_H
