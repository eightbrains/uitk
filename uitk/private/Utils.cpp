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
