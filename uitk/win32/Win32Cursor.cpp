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

#include "Win32Cursor.h"

#define WIN32_LEAN_AND_MEAN		1
#include <windows.h>

namespace uitk {

struct Win32Cursor::Impl
{
	HCURSOR cursor;
	bool needsDestroy;
};

Win32Cursor::Win32Cursor(OSCursor::System id)
	: mImpl(new Impl())
{
	switch (id) {
		case OSCursor::System::kLast:
		case OSCursor::System::kArrow:
			mImpl->cursor = LoadCursor(NULL, IDI_APPLICATION);
			break;
		case OSCursor::System::kIBeam:
			mImpl->cursor = LoadCursor(NULL, IDC_IBEAM);
			break;
		case OSCursor::System::kCrosshair:
			mImpl->cursor = LoadCursor(NULL, IDC_CROSS);
			break;
		case OSCursor::System::kOpenHand:
			mImpl->cursor = LoadCursor(NULL, IDC_SIZEALL);  // Windows has no open hand cursor
			break;
		case OSCursor::System::kClosedHand:
			mImpl->cursor = LoadCursor(NULL, IDC_SIZEALL);  // Windows has no closed hand cursor
			break;
		case OSCursor::System::kPointingHand:
			mImpl->cursor = LoadCursor(NULL, IDC_HAND);
			break;
		case OSCursor::System::kResizeLeftRight:
			mImpl->cursor = LoadCursor(NULL, IDC_SIZEWE);
			break;
		case OSCursor::System::kResizeUpDown:
			mImpl->cursor = LoadCursor(NULL, IDC_SIZENS);
			break;
		case OSCursor::System::kResizeNWSE:
			mImpl->cursor = LoadCursor(NULL, IDC_SIZENWSE);
			break;
		case OSCursor::System::kResizeNESW:
			mImpl->cursor = LoadCursor(NULL, IDC_SIZENESW);
			break;
		case OSCursor::System::kForbidden:
			mImpl->cursor = LoadCursor(NULL, IDC_NO);
			break;
	}
	mImpl->needsDestroy = false;
}

Win32Cursor::~Win32Cursor()
{
	if (mImpl->needsDestroy) {
		DestroyCursor(mImpl->cursor);
	}
}

void Win32Cursor::set(void */*window = nullptr*/, void */*windowSystem = nullptr*/) const
{
	SetCursor(mImpl->cursor);
}

}  // namespace uitk
