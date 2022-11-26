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

#ifndef UITK_OS_CURSOR_H
#define UITK_OS_CURSOR_H

#include "Cursor.h"

#include <nativedraw.h>

namespace uitk {

class OSWindow;

class OSCursor
{
public:
    enum class System {
        kArrow = 0,
        kIBeam,
        kCrosshair,
        kOpenHand,
        kClosedHand,
        kPointingHand,
        kResizeLeftRight,
        kResizeUpDown,
        kResizeNWSE,
        kResizeNESW,
        kForbidden,
        kLast
    };

    virtual ~OSCursor() {}

    // The cursor functions on Windows and macOS do not require a window or
    // window system, but some platforms like X11 do. Systems that do not
    // require these parameters should use the defaults.
    virtual void set(OSWindow *oswindow = nullptr, void *windowSystem = nullptr) const = 0;

    virtual void getHotspotPx(float *x, float *y) const = 0;
    virtual void getSizePx(float *width, float *height) const = 0;
    virtual Rect rectForPosition(OSWindow *oswindow, const Point& pos) const = 0;
};

}  // namespace uitk
#endif // UITK_OS_CURSOR_H
