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

#include "WASMClipboard.h"

#include <emscripten.h>
#include <emscripten/html5.h>

namespace uitk {

EM_ASYNC_JS(void, jsWriteClipboardText, (const char* str), {
    await navigator.clipboard.writeText(UTF8ToString(str));
});

EM_ASYNC_JS(char*, jsReadClipboardText, (), {
    const strJS = await navigator.clipboard.readText();
    const len = lengthBytesUTF8(strJS) + 1;
    const str = _malloc(len);
    stringToUTF8(strJS, str, len);
    return str;
});

struct WASMClipboard::Impl
{
};

WASMClipboard::WASMClipboard()
    : mImpl(new Impl())
{
}

WASMClipboard::~WASMClipboard()
{
}

bool WASMClipboard::hasString() const { return !this->string().empty(); }

std::string WASMClipboard::string() const
{
    return jsReadClipboardText();
}

void WASMClipboard::setString(const std::string& utf8)
{
    jsWriteClipboardText(utf8.c_str());
}

bool WASMClipboard::supportsX11SelectionString() const { return false; }
void WASMClipboard::setX11SelectionString(const std::string& utf8) {}
std::string WASMClipboard::x11SelectionString() const { return std::string(); }

} // namespace uitk
