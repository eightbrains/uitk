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

#include "Cursor.h"

#include "OSCursor.h"

#if defined(__APPLE__)
#include "macos/MacOSCursor.h"
#elif defined(_WIN32) || defined(_WIN64)
#include "win32/Win32Cursor.h"
#else
#include "x11/X11Cursor.h"
#endif

#include <memory>
#include <vector>

namespace uitk {

namespace {

class CursorImpl
{
private:
    std::vector<std::unique_ptr<OSCursor>> mOSSystemCursors;
    std::vector<std::unique_ptr<Cursor>> mSystemCursors;

public:
    CursorImpl()
    {
        mOSSystemCursors.resize(size_t(OSCursor::System::kLast));
        mSystemCursors.resize(size_t(OSCursor::System::kLast));
    }

    ~CursorImpl()
    {
        mSystemCursors.clear();
        mOSSystemCursors.clear();
    }

    Cursor& system(OSCursor::System id)
    {
        size_t idx = size_t(id);
        if (idx >= 0 && idx < mSystemCursors.size()) {
            if (!mSystemCursors[idx]) {
                OSCursor *newCursor = nullptr;
#if defined(__APPLE__)
                newCursor = new MacOSCursor(id);
#elif defined(_WIN32) || defined(_WIN64)
                newCursor = new Win32Cursor(id);
#else
                newCursor = new X11Cursor(id);
#endif
                mOSSystemCursors[idx].reset(newCursor);  // takes ownership of newCursor
                newCursor = nullptr;  // for clarity
                mSystemCursors[idx].reset(new Cursor(mOSSystemCursors[idx].get()));
            }
            return *mSystemCursors[idx];
        } else {
            return system(OSCursor::System::kArrow);
        }
    }

} gCursors;

}  // namespace

Cursor Cursor::arrow()
{
    return gCursors.system(OSCursor::System::kArrow);
}

Cursor Cursor::iBeam()
{
    return gCursors.system(OSCursor::System::kIBeam);
}

Cursor Cursor::crosshair()
{
    return gCursors.system(OSCursor::System::kCrosshair);
}

Cursor Cursor::openHand()
{
    return gCursors.system(OSCursor::System::kOpenHand);
}

Cursor Cursor::closedHand()
{
    return gCursors.system(OSCursor::System::kClosedHand);
}

Cursor Cursor::pointingHand()
{
    return gCursors.system(OSCursor::System::kPointingHand);
}

Cursor Cursor::resizeUpDown()
{
    return gCursors.system(OSCursor::System::kResizeUpDown);
}

Cursor Cursor::resizeLeftRight()
{
    return gCursors.system(OSCursor::System::kResizeLeftRight);
}

Cursor Cursor::resizeNWSE()
{
    return gCursors.system(OSCursor::System::kResizeNWSE);
}

Cursor Cursor::resizeNESW()
{
    return gCursors.system(OSCursor::System::kResizeNESW);
}

Cursor Cursor::forbidden()
{
    return gCursors.system(OSCursor::System::kForbidden);
}

Cursor::Cursor()
    : mCursor(nullptr)
{
}

Cursor::Cursor(OSCursor *osc)
    : mCursor(osc)
{
}

Cursor::Cursor(const Cursor& c)
    : mCursor(c.mCursor)
{
}

Cursor::~Cursor()
{
    // mCursor is a reference, so it should not be deleted. The only reason
    // it is a pointer is so that we can copy the object.
}

Cursor& Cursor::operator=(const Cursor& rhs)
{
    this->mCursor = rhs.mCursor;
    return *this;
}

bool Cursor::operator==(const Cursor& rhs) const
{
    return (this->mCursor == rhs.mCursor);
}

bool Cursor::operator!=(const Cursor& rhs) const
{
    return (this->mCursor != rhs.mCursor);
}

OSCursor* Cursor::osCursor() const { return mCursor; }

}  // namespace uitk
