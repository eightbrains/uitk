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

#include "Clipboard.h"

#if defined(__APPLE__)
#include "macos/MacOSClipboard.h"
#elif defined(_WIN32) || defined(_WIN64)  // _WIN32 covers everything except 64-bit ARM
#include "win32/Win32Clipboard.h"
#else
#include "x11/X11Clipboard.h"
#endif

namespace uitk {

struct Clipboard::Impl
{
    std::unique_ptr<OSClipboard> osClipboard;
};

Clipboard::Clipboard()
    : mImpl(new Impl())
{
#if defined(__APPLE__)
    mImpl->osClipboard = std::make_unique<MacOSClipboard>();
#elif defined(_WIN32) || defined(_WIN64)  // _WIN32 covers everything except 64-bit ARM
    mImpl->osClipboard = std::make_unique<Win32Clipboard>();
#else
    mImpl->osClipboard = std::make_unique<X11Clipboard>();
#endif
}

Clipboard::~Clipboard()
{
}

bool Clipboard::hasString() const
{
    return mImpl->osClipboard->hasString();
}

std::string Clipboard::string() const
{
    return mImpl->osClipboard->string();
}

void Clipboard::setString(const std::string& utf8)
{
    mImpl->osClipboard->setString(utf8);
}

} // namespace uitk


