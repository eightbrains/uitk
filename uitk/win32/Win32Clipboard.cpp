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

#include "Win32Clipboard.h"

#include "Win32Utils.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace uitk {

Win32Clipboard::Win32Clipboard()
{
}

Win32Clipboard::~Win32Clipboard()
{
}

bool Win32Clipboard::hasString() const
{
    // CF_TEXT and CF_OEMTEXT are automatically converted to CF_UNICODETEXT.
    return IsClipboardFormatAvailable(CF_UNICODETEXT);
}

std::string Win32Clipboard::string() const
{
    std::string clipData;
    
    HWND hwnd = GetActiveWindow();
    OpenClipboard(hwnd);
    
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);  // clipboard owns the handle
    if (hData) {
        WCHAR *data = (WCHAR*)GlobalLock(hData);
        if (data) {
            clipData = utf8FromWin32Unicode(data);
        }
        GlobalUnlock(hData);
    }
    
    CloseClipboard();
    return clipData;
}

void Win32Clipboard::setString(const std::string& utf8)
{
    HWND hwnd = GetActiveWindow();
    OpenClipboard(hwnd);
    EmptyClipboard();  // this makes us the clipboard owner
    
    auto wstr = win32UnicodeFromUTF8(utf8);
    HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, (wstr.size() + 1) * sizeof(wchar_t));
    if (hData) {
        WCHAR* data = (WCHAR*)GlobalLock(hData);
        memcpy(data, wstr.data(), wstr.size() * sizeof(wchar_t));
        // No guarantee that std::wstring stores the \0, so set it manually.
        wchar_t wnull = '\0';
        memcpy(data + wstr.size() * sizeof(wchar_t), &wnull, sizeof(wchar_t));
        GlobalUnlock(hData);

        SetClipboardData(CF_UNICODETEXT, hData);  // takes ownership of handle
    }
    
    CloseClipboard();
}

} // namespace uitk
