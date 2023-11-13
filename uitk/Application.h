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
#include <vector>

namespace uitk {

class Clipboard;
class IconPainter;
class OSMenubar;
class OSApplication;
class Shortcuts;
class Theme;
class Window;

class Application
{
    friend class Window;
public:
    using ScheduledId = unsigned long;
    static ScheduledId kInvalidScheduledId;

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

    enum class ScheduleMode { kOnce = 0, kRepeating = 1 };
    /// Posts a function that will be called on the main thread later.
    /// This function is safe to call on either the main thread or another thread.
    /// The actual delay amount is not very precise; Windows may only be accurate
    /// to within 10 ms, for instance, and macOS states that there is some
    /// inherent imprecision. The system may skip a repeat if the system is too
    /// busy to notice that it needs to call a scheduled function by the time the
    /// repeat comes around. There may be performance problems with large numbers;
    /// it is better to have one timer update many things rather than each having
    /// its own timer. (But, a huge do-everything timer is also a bad idea; normal
    /// code is hardly likely to have more than a handful of callbacks active at
    /// a time, anyway.) Due to the timing inaccuracy, this is not well-suited
    /// for animations.
    ScheduledId scheduleLater(Window* w, float delay, ScheduleMode mode,
                              std::function<void(ScheduledId)> f);

    /// Cancels a scheduled callback.
    void cancelScheduled(ScheduledId id);

    /// Returns the name of the application (used by some MacOS menus, the
    /// the About dialog, and can be useful for window titles).
    std::string applicationName() const;

    /// Returns the current working directory of the process.
    std::string currentPath() const;

    /// Returns a temp directory for writing in.
    std::string tempDir() const;

    /// Returns the available fonts registered with the operating system.
    std::vector<std::string> availableFontFamilies() const;

    /// Returns a value in seconds, with microsecond precision. The actual
    /// value is not useful, as 0.0 is undefined; only deltas are useful.
    /// This is a monotonically increasing value, so can be useful in
    /// animations or manual profiling.
    /// Note that the *accuracy* of may not be microseconds: this is
    /// currently a wrapper around std::chrono::steady_clock().
    /// Design note:  the name of the function comes from Java's nanoTime)(
    /// function. However, the 52 bits of the fraction allows durations of
    /// over 142 years before we lose microsecond precision due to lack of
    /// bits. Nanoseconds would only give us 52 days, and programs can
    /// easily run for more than 52 days. (For instance, Windows NT 4 had
    /// a bug where the OS needed to be rebooted every 52 days because of
    /// timer overflow)
    double microTime() const;

    /// Plays a beep, usually when a keypress is rejected. (This is used
    /// to produce the beep when a pressing a keyboard shortcut for a menu
    /// item, and we are not using native OS menus.)
    void beep();

    /// Prints the string to the debug output. Normally this is std::cout,
    /// but Win32 applications entering from WinMain() do not have std::cout
    /// connected, so this will print to the debug console. This is for
    /// DEBUGGING ONLY; do NOT use for user-visible error messages (at least
    /// not by itself), use Dialog::showAlert() instead!
    void debugPrint(const std::string& s);

    /// Returns true if the operating system's coordinate system has the
    /// origin in the upper left (Linux, Windows), otherwise false (macOS,
    /// which has the origin in the lower left).
    bool isOriginInUpperLeft() const;

    /// Returns true if the window's border is outside the frame of the window,
    /// that is, if a window position of (x, y) returns the upper left corner
    /// of the actual drawable area, or whether it is the upper left corner
    /// of the border. MacOS, for instance, draws the border inside the window
    /// frame (which has the side effect that you can draw over top of the
    /// border). This is useful for positioning popup windows.
    bool isWindowBorderInsideWindowFrame() const;

    /// Returns true if the platform does not have real windows and we are
    /// drawing into a framebuffer (for instance, HTML/Canvas with WebAssembly).
    /// Returns false otherwise. This is used by Window to properly clip to the
    /// window bounds and to restore the context after drawing, but should not
    /// be needed generally, as the drawing context is provided to the widget
    /// already configured.
    bool windowsMightUseSameDrawContext() const;

    /// Returns true if the operating system hides scrollbars when not
    /// scrolling (e.g. macOS), false otherwise.
    bool shouldHideScrollbars() const;

    /// If shouldHideScrollbars() == true, this is the time in seconds after
    /// a scroll event when the scrollbars should hide.
    double autoHideScrollbarDelaySecs() const;

    /// Returns the amount of time the mouse must hover in a widget before
    /// the tooltip is displayed, in seconds.
    double tooltipDelaySecs() const;

    enum class KeyFocusCandidates { kAll, kTextAndLists };
    KeyFocusCandidates keyFocusCandidates() const;

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

    /// Gets the application's icon painter.
    std::shared_ptr<IconPainter> iconPainter() const;

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
