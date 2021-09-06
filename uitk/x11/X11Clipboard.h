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

#ifndef UITK_X11_CLIPBOARD_H
#define UITK_X11_CLIPBOARD_H

#include "../Clipboard.h"

#include <memory>
#include <vector>

namespace uitk {

class X11Clipboard : public Clipboard
{
public:
    X11Clipboard(void *display);
    ~X11Clipboard();

    bool hasString() const override;

    std::string string() const override;

    void setString(const std::string& utf8) override;

    bool supportsX11SelectionString() const override;
    void setX11SelectionString(const std::string& utf8) override;
    std::string x11SelectionString() const override;

    // ---- internal usage ----
    enum class Selection { kClipboard, kTextSelection };
    void setActiveWindow(unsigned long w);
    void weAreNoLongerOwner(Selection sel);
    bool doWeHaveDataForTarget(Selection sel, unsigned int targetAtom);
    // These will copy, which is inefficient, but a) prevents us from needing
    // to store in X-native format, which would inhibit our own usage, and b)
    // pasting is infrequent, so not too much of a problem.
    std::vector<unsigned char> supportedTypes(Selection sel) const;
    std::vector<unsigned char> dataForTarget(Selection sel, unsigned int targetAtom) const;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace uitk
#endif // UITK_X11_CLIPBOARD_H
