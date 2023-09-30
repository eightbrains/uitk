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

#ifndef UITK_WASM_CLIPBOARD_H
#define UITK_WASM_CLIPBOARD_H

#include "../Clipboard.h"

#include <memory>
#include <vector>

namespace uitk {

class WASMClipboard : public Clipboard
{
public:
    WASMClipboard();
    ~WASMClipboard();

    bool hasString() const override;

    std::string string() const override;

    void setString(const std::string& utf8) override;

    bool supportsX11SelectionString() const override;
    void setX11SelectionString(const std::string& utf8) override;
    std::string x11SelectionString() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace uitk
#endif // UITK_WASM_CLIPBOARD_H
