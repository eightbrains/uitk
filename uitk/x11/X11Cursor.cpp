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

#include "X11Cursor.h"

#include <assert.h>
#include <X11/Xcursor/Xcursor.h>

#include <vector>

namespace uitk {

namespace {

::Cursor loadCursor(::Display *display, const std::vector<const char *>& names)
{
    ::Cursor cursor = 0;
    for (auto *name : names) {
        cursor = XcursorLibraryLoadCursor(display, name);
        if (cursor != 0) {
            break;
        }
    }
    return cursor;
}

}  // namespace

struct X11Cursor::Impl
{
    OSCursor::System cursorId;
    ::Display *display = nullptr;
    ::Cursor cursor;
};

X11Cursor::X11Cursor(OSCursor::System id)
    : mImpl(new Impl())
{
    mImpl->cursorId = id;
}

X11Cursor::~X11Cursor()
{
    if (mImpl->display) {
        // It is not clear from the "documentation" (consisting entirely of the
        // header file) if a Cursor needs to be freed. The fact that there is
        // not an obvious function to do it suggests not.
        mImpl->display = nullptr;
    }
}

void X11Cursor::set(void *window /*= nullptr*/, void *windowSystem /*= nullptr*/) const
{
    Display *display = (Display*)windowSystem;
    assert(mImpl->display == nullptr || mImpl->display == display);

    if (!mImpl->display) {
        
        mImpl->display = display;
        // There is no documentation on what cursors are available, although
        // there is an expectation that the legacy cursors named
        // in /usr/include/X11/cursorfont.h are available. Freedesktop has a
        // draft specification which in no way corresponds to what Ubuntu is
        // shipping (for example see /usr/share/icons/DMZ-White for Ubuntu 18.04)
        // so we fallback to those (which may not be very appropriate) if the
        // better ones fail.
        switch (mImpl->cursorId) {
            case OSCursor::System::kLast:
            case OSCursor::System::kArrow:
                mImpl->cursor = XcursorLibraryLoadCursor(display, "left_ptr");
                break;
            case OSCursor::System::kIBeam:
                // "vertical-text" appears to be for vertical i-beam
                mImpl->cursor = loadCursor(display, {"text", "ibeam", "xterm"});
                break;
            case OSCursor::System::kCrosshair:
                mImpl->cursor = XcursorLibraryLoadCursor(display, "crosshair");
                break;
            case OSCursor::System::kOpenHand:
                mImpl->cursor = loadCursor(display, {"openhand", "fleur"});
                break;
            case OSCursor::System::kClosedHand:
                mImpl->cursor = loadCursor(display,
                                           {"grabbing", "closedhand", "fleur"});
                break;
            case OSCursor::System::kPointingHand:
                mImpl->cursor = loadCursor(display, {"pointing_hand", "hand2"});
                break;
            case OSCursor::System::kResizeLeftRight:
                mImpl->cursor = XcursorLibraryLoadCursor(display, "sb_h_double_arrow");
                break;
            case OSCursor::System::kResizeUpDown:
                mImpl->cursor = XcursorLibraryLoadCursor(display, "sb_v_double_arrow");
                break;
            case OSCursor::System::kResizeNWSE:
                mImpl->cursor = loadCursor(display,
                                    {"bd_double_arrow", "size_bdiag", "fleur"});
                break;
            case OSCursor::System::kResizeNESW:
                mImpl->cursor = loadCursor(display,
                                    {"fd_double_arrow", "size_fdiag", "fleur"});
                break;
            case OSCursor::System::kForbidden:
                mImpl->cursor = XcursorLibraryLoadCursor(display, "circle");
                break;
        }
    }
    XDefineCursor(display, (::Window)window, mImpl->cursor);
}

void X11Cursor::getHotspotPx(float *x, float *y) const
{
}

void X11Cursor::getSizePx(float *width, float *height) const
{
}

Rect X11Cursor::rectForPosition(void *window, const Point& pos) const
{
}

}  // namespace uitk
