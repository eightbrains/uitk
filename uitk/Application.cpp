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
#include "Printing.h"
#include "ShortcutKey.h"
#include "Sound.h"
#include "Window.h"
#include "themes/EmpireTheme.h"
#include "themes/StandardIconPainter.h"

#if defined(__APPLE__)
#include "macos/MacOSApplication.h"
#include "macos/MacOSMenubar.h"
#elif defined(_WIN32) || defined(_WIN64)  // _WIN32 covers everything except 64-bit ARM
#include "win32/Win32Application.h"
#include "win32/Win32Menubar.h"
#elif defined(__EMSCRIPTEN__)
#include "wasm/WASMApplication.h"
#else
#include "x11/X11Application.h"
#endif

// getcwd
#if defined(_WIN32) || defined(_WIN64)
#include <direct.h>
#else
#include <unistd.h>
#endif

#include <stdlib.h>

#include <assert.h>
#include <algorithm>
#include <chrono>
#include <locale>

namespace uitk {

struct Application::Impl
{
    static Application* instance;

    std::unique_ptr<OSApplication> osApp;
    std::shared_ptr<Theme> theme;
    std::shared_ptr<StandardIconPainter> iconPainter;
    std::unique_ptr<OSMenubar> menubar;
    std::unique_ptr<Shortcuts> shortcuts;
    std::vector<Window*> windows;  // we do not own these
    Window* activeWindow = nullptr;  // we do not own this
    std::chrono::time_point<std::chrono::steady_clock> t0 = std::chrono::steady_clock::now();
    bool supportsNativeDialogs;
};
Application* Application::Impl::instance = nullptr;

Application::ScheduledId Application::kInvalidScheduledId = OSApplication::kInvalidSchedulingId;

Application& Application::instance()
{
    if (!Application::Impl::instance) {
        Application app;
        app.debugPrint("Need to create an instance of Application before doing");
        app.debugPrint("anything that interacts with the operating system");
        app.debugPrint("(e.g. creating a Window).");
        app.debugPrint("");
        app.debugPrint("int main(int argc, char *argv[])");
        app.debugPrint("{");
        app.debugPrint("    Application app;");
        app.debugPrint("    ...");
        app.debugPrint("    app.run();");
        app.debugPrint("}");
    }
    assert(Application::Impl::instance);  // assert so debugger goes somehwere useful
    return *Application::Impl::instance;
}

Application::Application()
    : mImpl(new Application::Impl())
{
#if defined(__APPLE__)
    mImpl->osApp = std::make_unique<MacOSApplication>();
#elif defined(_WIN32) || defined(_WIN64)
    mImpl->osApp = std::make_unique<Win32Application>();
#elif defined(__EMSCRIPTEN__)
    mImpl->osApp = std::make_unique<WASMApplication>();
#else
    mImpl->osApp = std::make_unique<X11Application>();
#endif

    setSupportsNativeDialogs(true);

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
#if defined(__APPLE__)
    // If we are on macOS and the app has not set any menus yet, we
    // have an empty menu (except for an empty Application menu).
    // This will behave strangely, so add the standard items.
    if (menubar().menus().size() == 1) {
#else
    if (menubar().menus().size() == 0) {
#endif // __APPLE__
        // Exclude undo/redo, since presumably that app is too simple
        // to handle that, too. We do want the edit menu, though,
        // in case there are text boxes or number editing (which handle
        // the menu internally).
        menubar().addStandardItems(nullptr, nullptr, nullptr, nullptr,
                                   { OSMenubar::StandardItem::kAbout,
                                     OSMenubar::StandardItem::kPreferences,
                                     OSMenubar::StandardItem::kUndo,
                                     OSMenubar::StandardItem::kRedo });
        // If the only way the code expects to quit is closing the
        // window, then we'd better exit after the last one closes.
        setExitWhenLastWindowCloses(true);
    }

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

Application::ScheduledId Application::scheduleLater(Window* w, float delay, ScheduleMode mode,
                          std::function<void(ScheduledId)> f)
{
    return mImpl->osApp->scheduleLater(w, delay, (mode == ScheduleMode::kRepeating), f);
}

void Application::cancelScheduled(ScheduledId id)
{
    mImpl->osApp->cancelScheduled(id);
}

std::string Application::applicationName() const
{
    return mImpl->osApp->applicationName();
}

std::string Application::tempDir() const
{
    return mImpl->osApp->tempDir();
}

std::string Application::currentPath() const
{
#if defined(_WIN32) || defined(_WIN64)
    // If buffer is nullptr, mallocs a buffer at least this big, more if necessary
    char *cwd = _getcwd(nullptr, 256);
#else
    // macOS: arg2 (size) is ignored if arg1 is nullptr
    // linux: if arg2 is 0, size automatically calculated
    char *cwd = getcwd(nullptr, 0);
#endif
    std::string path(cwd);
    free(cwd);
    return path;
}

std::vector<std::string> Application::availableFontFamilies() const
{
    return mImpl->osApp->availableFontFamilies();
}

double Application::microTime() const
{
    auto t1 = std::chrono::steady_clock::now();
    auto dt_usec = std::chrono::duration_cast<std::chrono::microseconds>(t1 - mImpl->t0).count() / 1e6;
    return dt_usec;
}

void Application::beep()
{
    mImpl->osApp->beep();
}

Sound& Application::sound() const
{
    return mImpl->osApp->sound();
}

const PaperSize& Application::defaultPaperSize() const
{
    std::string lc_paper;
    auto *e = getenv("LC_PAPER");  // (this is a GNU extension)
    if (e) {
        lc_paper = std::string(e);
    } else {
        lc_paper = std::locale("").name();
    }

    for (auto &c : lc_paper) {
        c = std::tolower(c);
    }

    if (lc_paper == "C") {
        return PaperSize::kUSLetter;
    } else if (lc_paper == "en" || lc_paper == "sp") {
        return PaperSize::kUSLetter;
    } else if (lc_paper.size() >=5 && lc_paper[2] == '_') {
        auto country = lc_paper.substr(3, 2);
        // The US, Canada (ca), Mexico (mx), Philippines (ph), Chile (cl),
        // Venezuela (ve), Costa Rica (cr), and Colombia (co) use US Letter
        // in practice, although some officially use A4.
        // (Note: "cr" does not conflict with Croatian, which is "hr")
        if (country == "us" || country == "ca" || country == "mx" ||
            country == "ph" || country == "cl" || country == "ve" ||
            country == "cr" || country == "co")
        {
            return PaperSize::kUSLetter;
        } else {
            return PaperSize::kA4;
        }
    } else {
        return PaperSize::kA4;
    }
}

void Application::printDocument(const PrintSettings& settings) const
{
    mImpl->osApp->printDocument(settings);
}

void Application::debugPrint(const std::string& s) const
{
    mImpl->osApp->debugPrint(s);
}

bool Application::isOriginInUpperLeft() const
{
    return mImpl->osApp->isOriginInUpperLeft();
}

bool Application::isWindowBorderInsideWindowFrame() const
{
    return mImpl->osApp->isWindowBorderInsideWindowFrame();
}

bool Application::windowsMightUseSameDrawContext() const
{
    return mImpl->osApp->windowsMightUseSameDrawContext();
}

bool Application::shouldHideScrollbars() const
{
    return mImpl->osApp->shouldHideScrollbars();
}

double Application::autoHideScrollbarDelaySecs() const { return 0.666f; }

double Application::tooltipDelaySecs() const { return 2.0f; }

Application::KeyFocusCandidates Application::keyFocusCandidates() const
{
    return (mImpl->osApp->canKeyFocusEverything()
            ? KeyFocusCandidates::kAll
            : KeyFocusCandidates::kTextAndLists);
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
#elif defined(__EMSCRIPTEN__)
    return false;
#else
    return false;
#endif
}

bool Application::supportsNativeDialogs() const
{
    return mImpl->supportsNativeDialogs;
}

void Application::setSupportsNativeDialogs(bool supports)
{
#if defined(__APPLE__)
    mImpl->supportsNativeDialogs = supports;
#elif defined(_WIN32) || defined(_WIN64)
    mImpl->supportsNativeDialogs = supports;
#elif defined(__EMSCRIPTEN__)
    mImpl->supportsNativeDialogs = false;
#else
    mImpl->supportsNativeDialogs = false;
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
#elif defined(__EMSCRIPTEN__)
        mImpl->theme = std::make_unique<EmpireTheme>(params);
#else
        mImpl->theme = std::make_unique<EmpireTheme>(params);
#endif
    }
    return mImpl->theme;
}

std::shared_ptr<IconPainter> Application::iconPainter() const
{
    if (!mImpl->iconPainter) {
        mImpl->iconPainter = std::make_unique<StandardIconPainter>();
    }
    return mImpl->iconPainter;
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
        for (auto &w : mImpl->windows) {
            w->onThemeChanged();
        }
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

    if (mImpl->activeWindow == w) {
        mImpl->activeWindow = nullptr;
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
