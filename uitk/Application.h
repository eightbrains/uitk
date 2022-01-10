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

#ifndef UITK_APPLICATION_H
#define UITK_APPLICATION_H

#include <functional>
#include <memory>
#include <set>

namespace uitk {

class Clipboard;
class OSMenubar;
class OSApplication;
class Shortcuts;
class Theme;
class Window;

class Application
{
    friend class Window;
public:
    /// The Application constructor will set the instance, so that you can
    /// inherit from Application if you want.
    static Application& instance();

    /// Instantiating an Application must be the first thing you do in the
    /// program before calling any other function in the library, as some
    /// (such as window creation) will access the instance. The application
    /// instance must live the for the duration of the program. Therefore,
    /// it is usually placed in the main() function, such as:
    ///     int main(int argc, char *argv) {
    ///         Application app;
    ///         return app.run();
    ///     }
    Application();
    virtual ~Application();

    /// On macOS apps do not usually exit when all the windows close; this
    /// can be used to change that behavior. It is ignored on other platforms,
    /// since there would be no way to re-open a window without a menubar,
    /// on non-Mac platforms is tied to the window.
    void setExitWhenLastWindowCloses(bool exits);

    /// Runs the event loop
    int run();

    /// Closes all windows and quits the event loop. Returns true unless one
    /// of the windows canceled the close.
    bool quit();

    /// Posts a function that will be called on the main thread later.
    /// This function is safe to call on either the main thread or another thread.
    void scheduleLater(Window* w, std::function<void()> f);

    /// Returns the name of the application (used by some MacOS menus, the
    /// the About dialog, and can be useful for window titles).
    std::string applicationName() const;

    /// Plays a beep, usually when a keypress is rejected. (This is used
    /// to produce the beep when a pressing a keyboard shortcut for a menu
    /// item, and we are not using native OS menus.)
    void beep();

    /// Returns true if the operating system's coordinate system has the
    /// origin in the upper left (Linux, Windows), otherwise false (macOS,
    /// which has the origin in the lower left).
    bool isOriginInUpperLeft() const;

    /// Returns true if the operating system hides scrollbars when not
    /// scrolling (e.g. macOS), false otherwise.
    bool shouldHideScrollbars() const;

    /// Returns true if the platform uses a menubar, false otherwise.
    /// For instance, desktop platforms (macOS, Windows, Linux) will return
    /// true, and mobile platorms (Android, iOS) will return false;
    bool platformHasMenubar() const;

    /// Returns true if the platform supports using native menus.
    bool supportsNativeMenus() const;

    /// Returns true if the platform supports native alert and file dialogs.
    bool supportsNativeDialogs() const;
    
    /// Sets or unsets using native dialogs for alerts and file dialogs.
    /// The argument is ignored for platforms that do not support or do not
    /// have native dialogs. Since the default is true (if native dialogs
    /// can be supported), this is mostly useful for turning off native
    /// support for testing.
    void setSupportsNativeDialogs(bool supports);

    /// Returns the active window, or nullptr if no windows are active.
    Window* activeWindow() const;

    /// Returns all windows
    const std::vector<Window*>& windows() const;

    /// Gets the application's clipboard
    Clipboard& clipboard() const;

    /// Gets the applications menubar
    OSMenubar& menubar() const;

    /// Gets the applications keyboard shortcuts manager. Note that if native
    /// menus are enabled the keyboard shorts in menus will be processed by
    /// the native code path and will not be in the shortcuts manager.
    Shortcuts& keyboardShortcuts() const;

    /// Gets the application's theme.
    std::shared_ptr<Theme> theme() const;

    void onSystemThemeChanged();

    OSApplication& osApplication();  // for internal use

private:
    void addWindow(Window *w);  // does NOT take ownership
    void removeWindow(Window *w);  // does NOT take ownership
    void setActiveWindow(Window *w);  // does NOT take ownership

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace uitk

#endif // UITK_APPLICATION_H
