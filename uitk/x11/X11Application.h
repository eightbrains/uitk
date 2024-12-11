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

#ifndef UITK_X11_APPLICATION_H
#define UITK_X11_APPLICATION_H

#include "../OSApplication.h"

#include <memory>

namespace uitk {

class Sound;
class X11Window;

class X11Application : public OSApplication
{
public:
    X11Application();
    ~X11Application();

    void setExitWhenLastWindowCloses(bool exits) override;
    int run() override;
    void exitRun() override;

    void scheduleLater(Window* w, std::function<void()> f) override;
    SchedulingId scheduleLater(Window* w, float delay, bool repeat,
                               std::function<void(SchedulingId)> f) override;
    void cancelScheduled(SchedulingId id) override;

    std::string applicationName() const override;
    std::string tempDir() const override;
    std::vector<std::string> availableFontFamilies() const override;

    void beep() override;
    Sound& sound() const override;
    void debugPrint(const std::string& s) override;

    bool isOriginInUpperLeft() const override;
    bool isWindowBorderInsideWindowFrame() const override;
    bool windowsMightUseSameDrawContext() const override;
    bool shouldHideScrollbars() const override;
    bool canKeyFocusEverything() const override;
    bool platformHasMenubar() const override;

    Clipboard& clipboard() const override;

    Theme::Params themeParams() const override;

public:
    // This returns the Display. It is void* to avoid including Xlib.h 
    // which pollutes the namespace badly (in particular, Font).
    void* display() const;
    void* xim() const;

    void registerWindow(long unsigned int xwindow, X11Window *window);
    void unregisterWindow(long unsigned int xwindow);

    float dpiForScreen(int screen);

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace uitk
#endif // UITK_X11_APPLICATION_H
