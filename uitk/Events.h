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

#ifndef UITK_EVENTS_H
#define UITK_EVENTS_H

#define ND_NAMESPACE uitk
#include <nativedraw.h>

namespace uitk {

struct KeyModifier {
    enum Values {
        kNone = 0,
        kShift = (1 << 0),
        kCtrl = (1 << 1),  // this is the Command key on macOS
        kAlt = (1 << 2),   // this is the Option key on macOS
        kMeta = (1 << 3),  // this is the Control key on macOS
        kCapsLock = (1 << 4),
        kNumLock = (1 << 5)
    };
};

enum class MouseButton {
    kNone = 0, kLeft, kRight, kMiddle, kButton4, kButton5
};

struct MouseEvent
{
    enum class Type { kMove, kButtonDown, kDrag, kButtonUp, kScroll };

    Type type;
    Point pos;
    int keymods;
    union {
        struct {
            MouseButton button;
            int nClicks;
        } button;
        struct {
            int buttons;
        } drag;
        struct {
            float dx;
            float dy;
        } scroll;
    };
};

}  // namespace uitk
#endif // UITK_EVENTS_H
