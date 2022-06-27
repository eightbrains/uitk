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

#ifndef UITK_UTILS_H
#define UITK_UTILS_H

#include <string>
#include <vector>

namespace uitk {

// Returns an array such that out[i] where i is an index into a UTF-16 string,
// gives the index into utf8. Multibyte UTF-16 characters have the same index
// for each byte, which eliminates the need for error checking in the
// unfortunate event of a bug that results in lookup up in the middle of a
// character.
std::vector<int> utf8IndicesForUTF16Indices(const char *utf8);

std::vector<int> utf8IndicesForUTF16Indices(const char *utf8);
// Returns an array such that out[utf16idx] gives the utf8 index.
std::vector<int> utf16IndicesForUTF8Indices(const char *utf8);

// Returns an array of indices of code points (which usually correspond to
// a glyph, except in the case of emoji).
std::vector<int> codePointIndicesForUTF8Indices(const char *utf8);

// Returns an array such that utf8[out[nthCodePt]] is the start of the
// nthCodePt. The returned array will include an index to the null terminator,
// so that a look-up when the cursor is at the end of the text will work.
std::vector<int> utf8IndicesForCodePointIndices(const char *utf8);

// Returns the (byte) index into the utf8 string for the next code point,
// or the index to the null terminator if currentIdx is the start of the last
// code point (this allows the cursor to be at the end of the string).
int nextCodePointUtf8(const char *utf8, int currentIdx);

// Returns the (byte) index into the utf8 string for the previous code point,
// or 0 if already at the beginning.
int prevCodePointUtf8(const char *utf8, int currentIdx);

// Assumes forward slash for directory delimiter, returned path does not
// include a trailing slash (unless the result is the root dir, "/")
std::string baseDirectoryOfPath(const std::string& path);

std::string removeMenuItemMnemonics(const std::string& s);

} // namespace uitk
#endif // UITK_UTILS_H
