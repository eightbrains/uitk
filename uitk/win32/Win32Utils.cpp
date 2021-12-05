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

#include "Win32Utils.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <sstream>

namespace uitk {

std::wstring win32UnicodeFromUTF8(const std::string& utf8)
{
    const int kNullTerminated = -1;
    int nCharsNeeded = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                                           kNullTerminated, NULL, 0);
    std::wstring wstr(nCharsNeeded + 1, wchar_t(0));  // nCharsNeeded includes \0, but +1 just in case
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), kNullTerminated, &wstr[0], nCharsNeeded);
    return wstr;
}

std::string utf8FromWin32Unicode(wchar_t *wstr)
{
    const int kNullTerminated = -1;
    int nCharsNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr, kNullTerminated,
                                           NULL, 0, NULL, NULL);
    char *str = new char[nCharsNeeded + 1];
    str[0] = '\0';
    WideCharToMultiByte(CP_UTF8, 0, wstr, kNullTerminated,
                        str, nCharsNeeded, NULL, NULL);
    std::string utf8(str);
    delete [] str;
    return utf8;
}

//-----------------------------------------------------------------------------
DPrint::DPrint()
{
}

DPrint::~DPrint()
{
    mLine += '\n';
    OutputDebugStringA(mLine.c_str());
}

DPrint& DPrint::operator<<(bool b)
{
    mLine += (b ? "true" : "false");
    return *this;
}

DPrint& DPrint::operator<<(char c)
{
    mLine += c;
    return *this;
}

DPrint& DPrint::operator<<(unsigned char c)
{
    mLine += c;
    return *this;
}

DPrint& DPrint::operator<<(short i)
{
    mLine += std::to_string(i);
    return *this;
}

DPrint& DPrint::operator<<(unsigned short i)
{
    mLine += std::to_string(i);
    return *this;
}

DPrint& DPrint::operator<<(int i)
{
    mLine += std::to_string(i);
    return *this;
}

DPrint& DPrint::operator<<(unsigned int i)
{
    mLine += std::to_string(i);
    return *this;
}

DPrint& DPrint::operator<<(long i)
{
    mLine += std::to_string(i);
    return *this;
}

DPrint& DPrint::operator<<(unsigned long i)
{
    mLine += std::to_string(i);
    return *this;
}

DPrint& DPrint::operator<<(long long i)
{
    mLine += std::to_string(i);
    return *this;
}

DPrint& DPrint::operator<<(unsigned long long i)
{
    mLine += std::to_string(i);
    return *this;
}

DPrint& DPrint::operator<<(float f)
{
    mLine += std::to_string(f);
    return *this;
}

DPrint& DPrint::operator<<(double d)
{
    mLine += std::to_string(d);
    return *this;
}

DPrint& DPrint::operator<<(const std::string& s)
{
    mLine += s;
    return *this;
}

DPrint& DPrint::operator<<(const char *str)
{
    mLine += str;
    return *this;
}

DPrint& DPrint::operator<<(void *p)
{
    std::stringstream ss;
    ss << p;  // prints as hex
    mLine += "0x";
    mLine += ss.str();
    return *this;
}


} // namespace uitk
