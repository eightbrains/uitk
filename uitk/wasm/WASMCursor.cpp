//-----------------------------------------------------------------------------
// Copyright 2021 - 2023 Eight Brains Studios, LLC
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

#include "WASMCursor.h"

#include "WASMApplication.h"
#include "../Application.h"

#include <iostream>

namespace uitk {

namespace {
    WASMApplication& getWASMApplication()
    {
        return *(WASMApplication*)&Application::instance().osApplication();
    }
}  // namespace

// TODO: do we we even need an Impl?

struct WASMCursor::Impl
{
    OSCursor::System systemCursorId;
};

WASMCursor::WASMCursor(OSCursor::System id)
    : mImpl(new Impl)
{
    mImpl->systemCursorId = id;
}

WASMCursor::~WASMCursor()
{
}

bool WASMCursor::isSystemCursor() const { return true; }
    
OSCursor::System WASMCursor::systemCursorId() const
{
    return mImpl->systemCursorId;
}

void WASMCursor::set(OSWindow *oswindow /*= nullptr*/, void *windowSystem /*= nullptr*/) const
{
    getWASMApplication().setCursor((WASMWindow*)oswindow, *this);
}

void WASMCursor::getHotspotPx(float *x, float *y) const
{
    std::cerr << "[error] WASMCursor::getHotpostPx() not implemented" << std::endl;
    if (x) {
        *x = 0.0f;
    }
    if (y) {
        *y = 0.0f;
    }
}

void WASMCursor::getSizePx(float *width, float *height) const
{
    std::cerr << "[error] WASMCursor::getSizePx() not implemented" << std::endl;
    if (width) {
        *width = 0.0f;
    }
    if (height) {
        *height = 0.0f;
    }
}

Rect WASMCursor::rectForPosition(OSWindow *oswindow, const Point& pos) const
{
    // The browser has no utility for getting the cursor rectangle (or even
    // the cursor size).
    auto size = PicaPt::fromStandardPixels(32.0f);
    return Rect(pos.x - 0.5f * size, pos.y - 0.5f * size, size, size);
}


}  // namespace uitk
