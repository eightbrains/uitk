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

#include "Application.h"

#include "Clipboard.h"
#include "OSApplication.h"
#include "themes/EmpireTheme.h"

#if defined(__APPLE__)
#include "macos/MacOSApplication.h"
#elif defined(_WIN32) || defined(_WIN64)  // _WIN32 covers everything except 64-bit ARM
#include "win32/Win32Application.h"
#else
#include "x11/X11Application.h"
#endif

#include <assert.h>

namespace uitk {

struct Application::Impl
{
    static Application* instance;

    std::unique_ptr<OSApplication> osApp;
    std::unique_ptr<Clipboard> clipboard;
    std::shared_ptr<Theme> theme;
};
Application* Application::Impl::instance = nullptr;

Application& Application::instance()
{
    assert(Application::Impl::instance);  // make sure you created an Application
    return *Application::Impl::instance;
}

Application::Application()
    : mImpl(new Application::Impl())
{
#if defined(__APPLE__)
    mImpl->osApp = std::make_unique<MacOSApplication>();
#elif defined(_WIN32) || defined(_WIN64)
    mImpl->osApp = std::make_unique<Win32Application>();
#else
    mImpl->osApp = std::make_unique<X11Application>();
#endif

    assert(!Application::Impl::instance);
    Application::Impl::instance = this;
}

Application::~Application()
{
    Application::Impl::instance = nullptr;
}

OSApplication& Application::osApplication() { return *mImpl->osApp; }

void Application::setExitWhenLastWindowCloses(bool exits)
{
    mImpl->osApp->setExitWhenLastWindowCloses(exits);
}

int Application::run()
{
    return mImpl->osApp->run();
}

void Application::scheduleLater(Window* w, std::function<void()> f)
{
    return mImpl->osApp->scheduleLater(w, f);
}

bool Application::isOriginInUpperLeft() const
{
    return mImpl->osApp->isOriginInUpperLeft();
}

bool Application::shouldHideScrollbars() const
{
    return mImpl->osApp->shouldHideScrollbars();
}

std::shared_ptr<Theme> Application::theme() const
{
    if (!mImpl->theme) {
        auto params = mImpl->osApp->themeParams();
#if defined(__APPLE__)
        mImpl->theme = std::make_unique<EmpireTheme>(params);
#elif defined(_WIN32) || defined(_WIN64)
        mImpl->theme = std::make_unique<EmpireTheme>(params);
#else
        mImpl->theme = std::make_unique<EmpireTheme>(params);
#endif
    }
    return mImpl->theme;
}

Clipboard& Application::clipboard() const
{
    if (!mImpl->clipboard) {
        // Application is a friend of Clipboard, so we can construct one, but
        // std::make_unique() is not a friend, so that fails.
        mImpl->clipboard = std::unique_ptr<Clipboard>(new Clipboard());
    }
    return *mImpl->clipboard;
}

void Application::onSystemThemeChanged()
{
    if (mImpl->theme) {
        mImpl->theme->setParams(mImpl->osApp->themeParams());
        // OS should invalidate windows
    }
}

} // namespace uitk
