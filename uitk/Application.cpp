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

#include "MenubarUITK.h"
#include "OSApplication.h"
#include "ShortcutKey.h"
#include "Window.h"
#include "themes/EmpireTheme.h"

#if defined(__APPLE__)
#include "macos/MacOSApplication.h"
#include "macos/MacOSMenubar.h"
#elif defined(_WIN32) || defined(_WIN64)  // _WIN32 covers everything except 64-bit ARM
#include "win32/Win32Application.h"
#include "win32/Win32Menubar.h"
#else
#include "x11/X11Application.h"
#endif

#include <assert.h>
#include <algorithm>

namespace uitk {

struct Application::Impl
{
    static Application* instance;

    std::unique_ptr<OSApplication> osApp;
    std::shared_ptr<Theme> theme;
    std::unique_ptr<OSMenubar> menubar;
    std::unique_ptr<Shortcuts> shortcuts;
    std::vector<Window*> windows;  // we do not own these
    Window* activeWindow = nullptr;  // we do not own this
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

    // Defer creation of menubar until it is requested, since menubar may
    // ask us (Application) if we support native menus. This is so that we
    // could potentially turn it off, e.g. for testing.

    mImpl->shortcuts = std::make_unique<Shortcuts>();

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

bool Application::quit()
{
    // Closing a window will cause it to remove itself from the set,
    // but we do not know exactly when that will happen, so to be
    // safe, make a copy and iterate over that.
    auto windows = mImpl->windows; // copy

    for (Window *w : windows) {
        if (!w->close(Window::CloseBehavior::kAllowCancel)) {
            return false;
        }
    }
    mImpl->osApp->exitRun();
    return true;
}

void Application::scheduleLater(Window* w, std::function<void()> f)
{
    return mImpl->osApp->scheduleLater(w, f);
}

std::string Application::applicationName() const
{
    return mImpl->osApp->applicationName();
}

void Application::beep()
{
    mImpl->osApp->beep();
}

bool Application::isOriginInUpperLeft() const
{
    return mImpl->osApp->isOriginInUpperLeft();
}

bool Application::shouldHideScrollbars() const
{
    return mImpl->osApp->shouldHideScrollbars();
}

bool Application::platformHasMenubar() const
{
    return mImpl->osApp->platformHasMenubar();
}

bool Application::supportsNativeMenus() const
{
#if defined(__APPLE__)
    return true;
#elif defined(_WIN32) || defined(_WIN64)
    return true;
#else
    return false;
#endif
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
    return mImpl->osApp->clipboard();
}

OSMenubar& Application::menubar() const
{
    if (!mImpl->menubar) {
        // Application is a friend, but std::make_unique<>() is not
        if (supportsNativeMenus()) {
#if defined(__APPLE__)
            mImpl->menubar = std::unique_ptr<MacOSMenubar>(new MacOSMenubar());
#elif defined(_WIN32) || defined(_WIN64)
            mImpl->menubar = std::unique_ptr<Win32Menubar>(new Win32Menubar());
#else
            assert(false);
#endif
        } else {
            mImpl->menubar = std::unique_ptr<MenubarUITK>(new MenubarUITK());
        }
    }
    return *mImpl->menubar;
}

Shortcuts& Application::keyboardShortcuts() const
{
    return *mImpl->shortcuts;
}

void Application::onSystemThemeChanged()
{
    if (mImpl->theme) {
        mImpl->theme->setParams(mImpl->osApp->themeParams());
        // OS should invalidate windows
    }
}

const std::vector<Window*>& Application::windows() const { return mImpl->windows; }

void Application::addWindow(Window *w)
{
    auto it = std::find(mImpl->windows.begin(), mImpl->windows.end(), w);
    if (it == mImpl->windows.end()) {
        mImpl->windows.push_back(w);
    }
}

void Application::removeWindow(Window *w)
{
    auto it = std::find(mImpl->windows.begin(), mImpl->windows.end(), w);
    if (it != mImpl->windows.end()) {
        mImpl->windows.erase(it);
    }
}

Window* Application::activeWindow() const { return mImpl->activeWindow; }

void Application::setActiveWindow(Window *w)
{
    mImpl->activeWindow = w;
    if (mImpl->windows.size() >= 2 && mImpl->windows.back() != w) {
        auto it = std::find(mImpl->windows.begin(), mImpl->windows.end(), w);
        if (it != mImpl->windows.end()) {
            mImpl->windows.erase(it);
            mImpl->windows.push_back(w);
        }
    }
}

} // namespace uitk
