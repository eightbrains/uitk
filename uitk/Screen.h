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

#ifndef UITK_SCREEN_H
#define UITK_SCREEN_H

#include <nativedraw.h>

#include "OSWindow.h"

namespace uitk {

struct OSScreen;

class Screen
{
public:
    Screen();
    Screen(const OSScreen& osscreen);

    /// Returns the area usable by windows. This does not include the
    /// menubar and dock area on macOS, and the taskbar on Windows.
    /// You should use this rectangle for positioning windows.
    /// Note that this may NOT be the actual size of the monitor, especially
    /// on macOS if scaling is enabled (which it is by default on recent
    /// MacBook Pros). Also note that on macOS, a hidden dock does reserve
    /// a little bit of room.
    const Rect& desktopRect() const;

    /// Returns the monitor size, including non-desktop area.
    /// Note that this may NOT be the actual size of the monitor, especially
    /// on macOS if scaling is enabled (which it is by default on recent
    /// MacBook Pros).
    const Rect& monitorRect() const;

    const OSScreen& osScreen() const;

private:
    Rect mDesktop;
    Rect mMonitor;
    OSScreen mOSScreen;
};

}  // namespace uitk
#endif // UITK_SCREEN_H
