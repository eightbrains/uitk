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

#include "../themes/EmpireTheme.h"

#import <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate>
@end

@interface AppDelegate ()
{
}
@property bool exitsWhenLastWindowCloses;
@end

@implementation AppDelegate
- (id)init {
    if (self = [super init]) {
        self.exitsWhenLastWindowCloses = false;  // macOS default
    }
    return self;
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
    AppDelegate *delegate;
};

MacOSApplication::MacOSApplication()
    : mImpl(new Impl())
{
    mImpl->delegate = [[AppDelegate alloc] init];
}

MacOSApplication::~MacOSApplication()
{
    mImpl->delegate = nil;  // ARC releases
}

bool MacOSApplication::shouldHideScrollbars() const
{
    return true;
}

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
    auto params = EmpireTheme::defaultParams();
    params.accentColor = toUITKColor(NSColor.controlAccentColor);
    params.windowBackgroundColor = Color(0.0f, 0.0f, 0.0f, 0.0f); // draw nothing
    params.nonEditableBackgroundColor = toUITKColor(NSColor.controlColor);
    params.textColor = toUITKColor(NSColor.controlTextColor);
    params.disabledTextColor = toUITKColor(NSColor.disabledControlTextColor);
    params.disabledBackgroundColor = Color(params.nonEditableBackgroundColor.red(),
                                           params.nonEditableBackgroundColor.green(),
                                           params.nonEditableBackgroundColor.blue(), 0.1f);
    // selectedControlTextColor is white in dark mode, but black in light mode, even though
    // the system buttons have white text in light mode.
    params.accentedBackgroundTextColor = toUITKColor(NSColor.alternateSelectedControlTextColor);
    NSFont *nsfont = [NSTextField labelWithString:@"Ag"].font;
    params.labelFont = Font(nsfont.familyName.UTF8String,
                            PicaPt::fromPixels(nsfont.pointSize, 72.0f));
    return params;
}

void MacOSApplication::setExitWhenLastWindowCloses(bool exits)
{
    mImpl->delegate.exitsWhenLastWindowCloses = exits;
}

int MacOSApplication::run()
{
    @autoreleasepool {
        [NSApplication sharedApplication];  // create NSApp object
        NSApp.delegate = mImpl->delegate;
        // Allow apps without a bundle and Info.plist to get focus
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        [NSApp activateIgnoringOtherApps:YES];
        [NSApp run];
    }
    return 0;
}

} // namespace uitk
