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

#include "MacOSApplication.h"

#include "MacOSClipboard.h"

#include "../Application.h"
#include "../Window.h"
#include "../themes/EmpireTheme.h"

#import <Cocoa/Cocoa.h>

#include <iostream>
#include <unordered_map>

@interface AppDelegate : NSObject <NSApplicationDelegate>
@end

@interface AppDelegate ()
{
    uitk::MacOSApplication *mApp;
}
@property bool exitsWhenLastWindowCloses;
@end

@implementation AppDelegate
- (id)init:(uitk::MacOSApplication*)app
{
    if (self = [super init]) {
        mApp = app;
        self.exitsWhenLastWindowCloses = false;  // macOS default
    }
    return self;
}

-(void)applicationDidFinishLaunching:(NSNotification *)notification
{
    // This must be called here, rather then before [NSApp run] is called,
    // otherwise the menu will not work correctly.
    [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    return (self.exitsWhenLastWindowCloses ? YES : NO);
}
@end

//-----------------------------------------------------------------------------
namespace uitk {

struct MacOSApplication::Impl
{
    struct Timer {
        NSTimer* timer;
        void* nswindow;  // void* because just used for an id: ARC won't retain
    };

    AppDelegate *delegate;
    std::unique_ptr<MacOSClipboard> clipboard;
    std::unordered_map<SchedulingId, Timer> timers;
    bool isHidingOtherApplications = false;

    std::unordered_map<SchedulingId, Timer>::iterator removeTimer(std::unordered_map<SchedulingId, Timer>::iterator it)
    {
        [it->second.timer invalidate];
        it->second.timer = nil;  // just in case erase() doesn't cause ARC to delete the NSTimer
        it->second.nswindow = nullptr;
        return this->timers.erase(it);
    }

};

MacOSApplication::MacOSApplication()
    : mImpl(new Impl())
{
    mImpl->delegate = [[AppDelegate alloc] init];
    mImpl->clipboard = std::make_unique<MacOSClipboard>();
}

MacOSApplication::~MacOSApplication()
{
    mImpl->delegate = nil;  // ARC releases
}

bool MacOSApplication::isOriginInUpperLeft() const { return false; }

bool MacOSApplication::isWindowBorderInsideWindowFrame() const { return true; }

bool MacOSApplication::windowsMightUseSameDrawContext() const { return false; }

bool MacOSApplication::shouldHideScrollbars() const { return true; }

bool MacOSApplication::canKeyFocusEverything() const
{
    return ([NSApp isFullKeyboardAccessEnabled] == YES);
}

bool MacOSApplication::platformHasMenubar() const { return true; }

Clipboard& MacOSApplication::clipboard() const { return *mImpl->clipboard; }

Theme::Params MacOSApplication::themeParams() const
{
    NSColorSpace *space = NSColorSpace.genericRGBColorSpace;
    auto toUITKColor = [space](NSColor *nscolor) {
        NSColor *c = [nscolor colorUsingColorSpace:space];
        return Color(float(c.redComponent),
                     float(c.greenComponent),
                     float(c.blueComponent),
                     float(c.alphaComponent));
    };

    auto windowBackground = toUITKColor(NSColor.windowBackgroundColor);
    auto textColor = toUITKColor(NSColor.controlTextColor);
    auto accentColor = toUITKColor(NSColor.controlAccentColor);
    bool isDarkMode = (textColor.toGrey().red() > 0.5f); // window bg may be transparent
    auto params = (isDarkMode ? EmpireTheme::darkModeParams(accentColor)
                              : EmpireTheme::lightModeParams(accentColor));
    params.accentColor = accentColor;
    params.keyFocusColor = toUITKColor(NSColor.keyboardFocusIndicatorColor);
    params.windowBackgroundColor = toUITKColor(NSColor.windowBackgroundColor);
    params.editableBackgroundColor = toUITKColor(NSColor.controlColor);
    params.nonEditableBackgroundColor = toUITKColor(NSColor.controlColor);
    params.textColor = textColor;
    params.disabledTextColor = toUITKColor(NSColor.disabledControlTextColor);
    params.disabledBackgroundColor = Color(params.nonEditableBackgroundColor.red(),
                                           params.nonEditableBackgroundColor.green(),
                                           params.nonEditableBackgroundColor.blue(), 0.1f);
    // selectedControlTextColor is white in dark mode, but black in light mode, even though
    // the system buttons have white text in light mode.
    params.accentedBackgroundTextColor = toUITKColor(NSColor.alternateSelectedControlTextColor);

    // Increased contrast (Settings >> Accessibility >> Display >> Increase contrast)
    // mostly draws a border around every control.
    if (NSWorkspace.sharedWorkspace.accessibilityDisplayShouldIncreaseContrast) {
        // In dark mode the control background is transparent if the window is
        // not active (which is the case currently).
        if (isDarkMode && params.editableBackgroundColor.alpha() < 0.0001f) {
            params.nonEditableBackgroundColor = Color(1.0f, 1.0f, 1.0f, 0.25f);
        }
        params.borderColor = textColor;
        params.useHighContrast = true;
    }

    // macOS gives the same font size no matter what the effective resolution
    // is (e.g. between native and scaled), so it handles the conversion to the
    // actual font size under the hood. Since we want the native resolution to
    // be the correct size, use the uiDPI. This will properly scale it if the
    // uses a different setting in Settings >> Display >> Resolution.
    float uiDPI;
    DrawContext::getScreenDPI(nullptr, &uiDPI, nullptr, nullptr);
    NSFont *nsfont = [NSTextField labelWithString:@"Ag"].font;
    params.labelFont = Font(nsfont.familyName.UTF8String,
                            PicaPt::fromPixels(nsfont.pointSize, uiDPI));
    NSFont *nsmenufont = [NSFont menuFontOfSize:0.0];
    params.nonNativeMenubarFont = Font(nsmenufont.familyName.UTF8String,
                                       PicaPt::fromPixels(nsmenufont.pointSize, uiDPI));
    return params;
}

void MacOSApplication::setExitWhenLastWindowCloses(bool exits)
{
    mImpl->delegate.exitsWhenLastWindowCloses = exits;
}

void MacOSApplication::scheduleLater(Window* w, std::function<void()> f)
{
    dispatch_async(dispatch_get_main_queue(), ^{ f(); });
}

OSApplication::SchedulingId MacOSApplication::scheduleLater(Window* w, float delay, bool repeat,
                                                            std::function<void(SchedulingId)> f)
{
    static unsigned long gId = kInvalidSchedulingId;

    unsigned long newId = ++gId;
    NSTimer *timer = [NSTimer scheduledTimerWithTimeInterval:(NSTimeInterval)delay
                                                     repeats:(repeat ? YES : NO)
                                                       block:^(NSTimer * _Nonnull timer) {
        f(newId);
    }];
    if (repeat) {
        mImpl->timers[newId] = { timer, w->nativeHandle() };
    }
    return newId;
}

void MacOSApplication::cancelScheduled(SchedulingId id)
{
    auto it = mImpl->timers.find(id);
    if (it != mImpl->timers.end()) {
        mImpl->removeTimer(it);
    }
}

void MacOSApplication::onWindowWillClose(void* nswindow)
{
    auto it = mImpl->timers.begin();
    while (it != mImpl->timers.end()) {
        if (it->second.nswindow == nswindow) {
            it = mImpl->removeTimer(it);
        } else {
            ++it;
        }
    }
}

std::string MacOSApplication::applicationName() const
{
    return std::string(NSRunningApplication.currentApplication.localizedName.UTF8String);
}

std::string MacOSApplication::tempDir() const
{
    return "/tmp";
}

std::vector<std::string> MacOSApplication::availableFontFamilies() const
{
    return Font::availableFontFamilies();
}

void MacOSApplication::beep()
{
    NSBeep();
}

void MacOSApplication::debugPrint(const std::string& s)
{
    std::cout << s << std::endl;
}

int MacOSApplication::run()
{
    @autoreleasepool {
        // Create NSApp object. (Note that this is likely to already be created
        // by accessing Application::instance().menubar(), not sure how that
        // affects the autoreleasepool at all.
        [NSApplication sharedApplication];
        NSApp.delegate = mImpl->delegate;
        // Allow apps without a bundle and Info.plist to get focus
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        // -activateIgnoringOtherApps: MUST be called in the delegate's
        //    -applicationDidFinishLaunching: otherwise menu items won't show up.
        //[NSApp activateIgnoringOtherApps:YES];
        [NSApp run];
    }
    return 0;
}

void MacOSApplication::exitRun()
{
    // Note: the docs say that -stop: only exits the run loop after an event is
    //       processed, and that a timer firing is not an event.
    [NSApp stop:nil];
}

void MacOSApplication::hideApplication()
{
    [NSApp hide:NSApp];
}

void MacOSApplication::hideOtherApplications()
{
    [NSApp hideOtherApplications:NSApp];
    mImpl->isHidingOtherApplications = true;
}

void MacOSApplication::showOtherApplications()
{
    [NSApp unhideAllApplications:NSApp];
    mImpl->isHidingOtherApplications = false;
}

bool MacOSApplication::isHidingOtherApplications() const
{
    return mImpl->isHidingOtherApplications;
}

} // namespace uitk
