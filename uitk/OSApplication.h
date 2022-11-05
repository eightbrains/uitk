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

#ifndef UITK_OS_APPLICATION_H
#define UITK_OS_APPLICATION_H

#include "themes/Theme.h"

#include <functional>
#include <string>
#include <vector>

namespace uitk {

class Clipboard;
class Window;

class OSApplication
{
public:
    virtual ~OSApplication() {};

    virtual void setExitWhenLastWindowCloses(bool exits) = 0;
    virtual int run() = 0;
    virtual void exitRun() = 0;

    virtual void scheduleLater(Window* w, std::function<void()> f) = 0;

    virtual std::string applicationName() const = 0;
    virtual std::string tempDir() const = 0;
    virtual std::vector<std::string> availableFontFamilies() const = 0;

    virtual void beep() = 0;

    virtual bool isOriginInUpperLeft() const = 0;
    virtual bool shouldHideScrollbars() const = 0;
    virtual bool canKeyFocusEverything() const = 0;
    virtual bool platformHasMenubar() const = 0;

    virtual Clipboard& clipboard() const = 0;

    virtual Theme::Params themeParams() const = 0;
};

} // namespace uitk
#endif // UITK_OS_APPLICATION_H
