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

#include "MacOSCursor.h"

#import <AppKit/AppKit.h>

namespace uitk {

struct MacOSCursor::Impl
{
    NSCursor *cursor;
};

MacOSCursor::MacOSCursor(OSCursor::System cursor)
: mImpl(new Impl())
{
    switch (cursor) {
        case OSCursor::System::kArrow:
            mImpl->cursor = NSCursor.arrowCursor;
            break;
        case OSCursor::System::kIBeam:
            mImpl->cursor = NSCursor.IBeamCursor;
            break;
        case OSCursor::System::kCrosshair:
            mImpl->cursor = NSCursor.crosshairCursor;
            break;
        case OSCursor::System::kOpenHand:
            mImpl->cursor = NSCursor.openHandCursor;
            break;
        case OSCursor::System::kClosedHand:
            mImpl->cursor = NSCursor.closedHandCursor;
            break;
        case OSCursor::System::kPointingHand:
            mImpl->cursor = NSCursor.pointingHandCursor;
            break;
        case OSCursor::System::kResizeLeftRight:
            mImpl->cursor = NSCursor.resizeLeftRightCursor;
            break;
        case OSCursor::System::kResizeUpDown:
            mImpl->cursor = NSCursor.resizeUpDownCursor;
            break;
        case OSCursor::System::kResizeNWSE:
            mImpl->cursor = [[NSCursor class]
                             performSelector:@selector(_windowResizeNorthWestSouthEastCursor)];
            break;
        case OSCursor::System::kResizeNESW:
            mImpl->cursor = [[NSCursor class]
                             performSelector:@selector(_windowResizeNorthEastSouthWestCursor)];
            break;
        case OSCursor::System::kForbidden:
            mImpl->cursor = NSCursor.operationNotAllowedCursor;
            break;
        case OSCursor::System::kLast:
            assert(false);
            mImpl->cursor = NSCursor.arrowCursor;
            break;
    }
}

MacOSCursor::~MacOSCursor()
{
    mImpl->cursor = nil;  // release
}

void MacOSCursor::set(void */*window = nullptr*/, void */*windowSystem = nullptr*/) const
{
    [mImpl->cursor set];
}

}  // namespace uitk
