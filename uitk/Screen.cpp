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

#include "Screen.h"

#include "OSWindow.h"

namespace uitk {

Screen::Screen()
{
}

Screen::Screen(const OSScreen& osscreen)
    : mDesktop(PicaPt::fromPixels(osscreen.desktopFrame.x, osscreen.dpi),
               PicaPt::fromPixels(osscreen.desktopFrame.y, osscreen.dpi),
               PicaPt::fromPixels(osscreen.desktopFrame.width, osscreen.dpi),
               PicaPt::fromPixels(osscreen.desktopFrame.height, osscreen.dpi))
    , mMonitor(PicaPt::fromPixels(osscreen.fullscreenFrame.x, osscreen.dpi),
               PicaPt::fromPixels(osscreen.fullscreenFrame.y, osscreen.dpi),
               PicaPt::fromPixels(osscreen.fullscreenFrame.width, osscreen.dpi),
               PicaPt::fromPixels(osscreen.fullscreenFrame.height, osscreen.dpi))
    , mOSScreen(osscreen)
{
}

const Rect& Screen::desktopRect() const { return mDesktop; }

const Rect& Screen::monitorRect() const { return mMonitor; }

const OSScreen& Screen::osScreen() const { return mOSScreen; }

}  // namespace uitk
