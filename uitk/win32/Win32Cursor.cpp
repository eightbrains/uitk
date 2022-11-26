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

#include "Win32Cursor.h"

#include "../OSWindow.h"

#include <unordered_map>

#define WIN32_LEAN_AND_MEAN		1
#include <windows.h>
#include <ShellScalingAPI.h>

namespace uitk {

namespace {

struct CursorInfo {
	float hotspotX;
	float hotspotY;
	float width;
	float height;
};
static std::unordered_map<HCURSOR, CursorInfo> gCursorInfoCache;

}  // namespace

struct Win32Cursor::Impl
{
	HCURSOR cursor;
	bool needsDestroy;

	const CursorInfo& getInfo()
	{
		auto it = gCursorInfoCache.find(this->cursor);
		if (it == gCursorInfoCache.end()) {
			CursorInfo info;
			ICONINFO win32info;
			if (GetIconInfo(this->cursor, &win32info)) {  // turns out the icon can be either an icon or standard cursor
				info.hotspotX= float(win32info.xHotspot);
				info.hotspotY = float(win32info.yHotspot);

				bool isBW = (win32info.hbmColor == NULL);
				BITMAP bitmapInfo = { 0 };
				if (GetObject(win32info.hbmMask, sizeof(bitmapInfo), &bitmapInfo)) {
					info.width = float(bitmapInfo.bmWidth);
					info.height = float(std::abs(bitmapInfo.bmHeight) / (isBW ? 2 : 1));
				} else {
					info.width = 0.0f;
					info.height = 0.0f;
				}

				DeleteObject(win32info.hbmMask);
				if (!isBW) {
					DeleteObject(win32info.hbmColor);
				}
			}
			else {
				info.hotspotX = 0.0f;
				info.hotspotY = 0.0f;
				info.width = 0.0f;
				info.height = 0.0f;
			}

			gCursorInfoCache[this->cursor] = info;
			it = gCursorInfoCache.find(this->cursor);
		}
		return it->second;
	}
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
		auto it = gCursorInfoCache.find(mImpl->cursor);
		if (it != gCursorInfoCache.end()) {
			gCursorInfoCache.erase(it);
		}
		DestroyCursor(mImpl->cursor);
	}
}

void Win32Cursor::set(OSWindow* /*oswindow = nullptr*/, void* /*windowSystem = nullptr*/) const
{
	SetCursor(mImpl->cursor);
}

void Win32Cursor::getHotspotPx(float *x, float *y) const
{
	auto info = mImpl->getInfo();
	*x = info.hotspotX;
	*y = info.hotspotY;
}

void Win32Cursor::getSizePx(float *width, float *height) const
{
	auto info = mImpl->getInfo();
	*width = info.width;
	*height = info.height;
}

Rect Win32Cursor::rectForPosition(OSWindow *oswindow, const Point& pos) const
{
	// Windows 10 seems to use a constant scaling factor for the size of the cursor,
	// regardless of the resolution or scale factor set in Settings. I cannot figure
	// out where the value comes from. The DPI for the window is based on the values
	// in Settings, so scaling 100% gives 96 dpi, 200% gives 192 dpi, etc. This is
	// also the value for GetDpiMonitor() with MDT_EFFECTIVE_DPI. My monitor gives
	// a raw DPI of 325, 210. It does not seem like Windows is using the y value
	// (which I do not think is correct, either). The value that makes things look
	// correct is about 1.7, and the only way I can think to get that value is if
	// Windows is scaling by the monitor amount divided by a maximum value of 2.
	// There is a GetScaleFactorForMonitor(), which returns 180% for me, which is
	// close, but does not produce the correct results (as evaluated visually), and in
	// any case, it reports something related to the settings in Settings; it is not
	// settings invariant like this constant.
	auto hmonitor = MonitorFromWindow((HWND)oswindow->nativeHandle(), MONITOR_DEFAULTTONEAREST);
	UINT rawDPI, rawDPIY;
	GetDpiForMonitor(hmonitor, MDT_RAW_DPI, &rawDPI, &rawDPIY);
	const float kBaseDPI = 96.0f;
	auto rawMultiplier = float(rawDPI) / kBaseDPI;
	auto integerMultiplier = (rawDPI > 1.5f * kBaseDPI ? 2.0f : 1.0f);
	auto scaleFactor = max(1.0f, rawMultiplier / integerMultiplier);

	auto dpi = oswindow->dpi();
	auto info = mImpl->getInfo();
	Rect r(pos.x, pos.y,
		PicaPt::fromPixels(info.width / scaleFactor, dpi),
		PicaPt::fromPixels(info.height / scaleFactor, dpi));
	r.translate(PicaPt::fromPixels(-info.hotspotX, dpi * scaleFactor),
		PicaPt::fromPixels(-info.hotspotY, dpi * scaleFactor));
	return r;
}

}  // namespace uitk
