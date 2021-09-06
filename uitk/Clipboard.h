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

#ifndef UITK_CLIPBOARD_H
#define UITK_CLIPBOARD_H

#include <string>

namespace uitk {

class Clipboard
{
public:
    virtual ~Clipboard() {};

    virtual bool hasString() const = 0;

    virtual std::string string() const = 0;

    virtual void setString(const std::string& utf8) = 0;

    /// Returns true if this platform supports X11 primary selection (used
    /// by middle-click).
    virtual bool supportsX11SelectionString() const = 0;
    /// Sets the X11 primary selection (used by middle-click). Noop on other
    /// platforms.
    virtual void setX11SelectionString(const std::string& utf8) = 0;
    /// Reads the X11 primary selection (used by middle-click). Returns an
    /// empty string on other platforms.
    virtual std::string x11SelectionString() const = 0;

protected:
    Clipboard() {}
};

} // namespace uitk
#endif // UITK_CLIPBOARD_H
