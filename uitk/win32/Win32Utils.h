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

#ifndef UITK_WIN32_UTILS_H
#define UITK_WIN32_UTILS_H

#include <string>

namespace uitk {

std::wstring win32UnicodeFromUTF8(const std::string& utf8);
std::string utf8FromWin32Unicode(wchar_t *wstr);

// Use like:  DPrint() << "text " << i << ", " << ptr;
// Destructor will add a newline before printing.
class DPrint
{
public:
    DPrint();
    ~DPrint();

    DPrint& operator<<(bool b);
    DPrint& operator<<(char c);
    DPrint& operator<<(unsigned char c);
    DPrint& operator<<(short i);
    DPrint& operator<<(unsigned short i);
    DPrint& operator<<(int i);
    DPrint& operator<<(unsigned int i);
    DPrint& operator<<(long i);
    DPrint& operator<<(unsigned long i);
    DPrint& operator<<(long long i);
    DPrint& operator<<(unsigned long long i);
    DPrint& operator<<(float f);
    DPrint& operator<<(double d);
    DPrint& operator<<(const std::string& s);
    DPrint& operator<<(const char *str);
    DPrint& operator<<(void *p);

protected:
    std::string mLine;
};

} // namespace uitk
#endif // UITK_WIN32_UTILS_H
