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

#include "Utils.h"

namespace uitk {

// Hack:
//   std::vector<int> utf8IndicesForUTF16Indices(const char *utf8)
//   std::vector<int> utf16IndicesForUTF8Indices(const char *utf8)
// already exist in libnativedraw with these signatures. Since this will be
// linked, we just won't provide a body for these, and the libnativedraw ones
// will be used.

std::vector<int> codePointIndicesForUTF8Indices(const char *utf8)
{
    std::vector<int> indices;
    int cpIdx = 0;
    int i = 0;
    while (utf8[i] != '\0') {
        int next = nextCodePointUtf8(utf8, i);
        for (int j = i;  j < next; ++j) {
            indices.push_back(cpIdx);
        }
        i = next;
        cpIdx += 1;
    }
    return indices;
}

std::vector<int> utf8IndicesForCodePointIndices(const char *utf8)
{
    std::vector<int> indices;
    int cpIdx = 0;
    int i = 0;
    while (utf8[i] != '\0') {
        int next = nextCodePointUtf8(utf8, i);
        indices.push_back(i);
        i = next;
        cpIdx += 1;
    }
    indices.push_back(i);
    return indices;
}

int nextCodePointUtf8(const char *utf8, int currentIdx)
{
    if (utf8[currentIdx] == '\0') {
        return currentIdx;
    }
    
    // UTF8 encoding is:
    // 0x0000 - 0x007f:  0xxxxxxx
    // 0x0080 - 0x07ff:  110xxxxx 10xxxxxx
    // 0x0800 - 0xffff:  1110xxxx 10xxxxxx 10xxxxxx
    // 0x0080 - 0x07ff:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    int len = 1;
    if (((unsigned char)utf8[currentIdx] & 0b11000000) == (unsigned char)0b11000000) { len += 1; }
    if (((unsigned char)utf8[currentIdx] & 0b11100000) == (unsigned char)0b11100000) { len += 1; }
    if (((unsigned char)utf8[currentIdx] & 0b11110000) == (unsigned char)0b11110000) { len += 1; }
    return currentIdx + len;
}

int prevCodePointUtf8(const char *utf8, int currentIdx)
{
    if (currentIdx <= 0) {
        return 0;
    }

    // See UTF8 encoding table in nextCodePoint(). If byte is 10xxxxxx it is
    // part of a multibyte code point, so keep going.
    currentIdx -= 1;
    while (currentIdx >= 0 && (unsigned char)utf8[currentIdx] >= 0x80 && (unsigned char)utf8[currentIdx] < 0xc0) {
        currentIdx -= 1;
    }
    return currentIdx;
}

std::string baseDirectoryOfPath(const std::string& path)
{
    auto idx = path.rfind('/');
    if (idx == 0 || idx == std::string::npos) {
        return "/";
    }
    return path.substr(0, idx);
}

std::string removeMenuItemMnemonics(const std::string& s)
{
	auto noAmpersands = s;
	auto idx = noAmpersands.find("&");
	while (idx != std::string::npos) {
		noAmpersands = noAmpersands.replace(idx, 1, "");
		idx = noAmpersands.find("&");
	}
	return noAmpersands;
}

} // namespace uitk
