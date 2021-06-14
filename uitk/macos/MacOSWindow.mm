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

#if ! __has_feature(objc_arc)
#error "ARC is off"
#endif

#include "MacOSWindow.h"

#include "../Application.h"
#include "../Events.h"
#include <nativedraw.h>

#import <Cocoa/Cocoa.h>

namespace {
// macOS (and iOS) uses a 72 dpi assumption, and units to the API are
// given in 72 dpi pixels. Underneath the hood, the pixels might be
// smaller, so the fonts are scaled, the images are scaled, etc.
const float kDPI = 72.0f;

int toKeymods(NSEventModifierFlags flags)
{
    int keymods = 0;
    if (flags & NSEventModifierFlagShift) {
        keymods |= uitk::KeyModifier::kShift;
    }
    if (flags & NSEventModifierFlagCommand) {
        keymods |= uitk::KeyModifier::kCtrl;
    }
    if (flags & NSEventModifierFlagControl) {
        keymods |= uitk::KeyModifier::kMeta;
    }
    if (flags & NSEventModifierFlagOption) {
        keymods |= uitk::KeyModifier::kAlt;
    }
    if (flags & NSEventModifierFlagCapsLock) {
        keymods |= uitk::KeyModifier::kCapsLock;
    }
    return keymods;
}

uitk::MouseButton toUITKMouseButton(NSInteger buttonNumber)
{
    switch (buttonNumber) {
        case 1:
            return uitk::MouseButton::kLeft;
        case 2:
            return uitk::MouseButton::kRight;
        case 3:
            return uitk::MouseButton::kMiddle;
        case 4:
            return uitk::MouseButton::kButton4;
        case 5:
            return uitk::MouseButton::kButton5;
        default:
            return uitk::MouseButton::kNone;
    }
}

}  // namespace

//-----------------------------------------------------------------------------
@interface ContentView : NSView
@end

@interface ContentView ()
{
    bool mAppearanceChanged;
}
@property uitk::IWindowCallbacks *callbacks;  // ObjC does not accept reference
@end

@implementation ContentView
- (id)initWithCallbacks:(uitk::IWindowCallbacks*)cb {
    if (self = [super init]) {
        mAppearanceChanged = false;
        self.callbacks = cb;
    }
    return self;
}

// Note:  on macOS, draw contexts are transitory
- (std::shared_ptr<uitk::DrawContext>)createContext {
    int width = int(std::ceil(self.bounds.size.width));
    int height = int(std::ceil(self.bounds.size.height));

    auto cgContext = NSGraphicsContext.currentContext.CGContext;
    return uitk::DrawContext::fromCoreGraphics(cgContext, width, height,
                                               kDPI * self.window.backingScaleFactor);
}

/*- (void)setFrame:(NSRect)frame
{
    [super setFrame:frame];
    self.callbacks->onLayout();
}
*/
- (void)setFrameSize:(NSSize)newSize
{
    [super setFrameSize:newSize];
    auto dc = [self createContext];
    self.callbacks->onResize(*dc);
}

- (void)layout
{
    [super layout];

    if (mAppearanceChanged) {
        uitk::Application::instance().onSystemThemeChanged();
        mAppearanceChanged = false;
    }

    self.callbacks->onLayout(*[self createContext]);
}

- (void)drawRect:(NSRect)dirtyRect
{
    auto dc = [self createContext];
    dc->beginDraw();
    self.callbacks->onDraw(*dc);
    dc->endDraw();
}

- (void)mouseMoved:(NSEvent *)e
{
    NSPoint pt = [self convertPoint:e.locationInWindow fromView:nil];

    uitk::MouseEvent me;
    me.type = uitk::MouseEvent::Type::kMove;
    me.pos = uitk::Point(uitk::PicaPt::fromPixels(pt.x, kDPI),
                         uitk::PicaPt::fromPixels(self.frame.size.height - pt.y, kDPI));
    me.keymods = toKeymods(e.modifierFlags);

    self.callbacks->onMouse(me);
}

- (void)mouseDown:(NSEvent *)e
{
    [self otherMouseDown:e];
}

- (void)rightMouseDown:(NSEvent *)e
{
    [self otherMouseDown:e];
}

- (void)otherMouseDown:(NSEvent *)e
{
    NSPoint pt = [self convertPoint:e.locationInWindow fromView:nil];

    uitk::MouseEvent me;
    me.type = uitk::MouseEvent::Type::kButtonDown;
    me.pos = uitk::Point(uitk::PicaPt::fromPixels(pt.x, kDPI),
                         uitk::PicaPt::fromPixels(self.frame.size.height - pt.y, kDPI));
    me.keymods = toKeymods(e.modifierFlags);
    me.button.button = toUITKMouseButton(e.buttonNumber);

    self.callbacks->onMouse(me);
}

- (void)mouseDragged:(NSEvent *)e
{
    NSPoint pt = [self convertPoint:e.locationInWindow fromView:nil];

    uitk::MouseEvent me;
    me.type = uitk::MouseEvent::Type::kDrag;
    me.pos = uitk::Point(uitk::PicaPt::fromPixels(pt.x, kDPI),
                         uitk::PicaPt::fromPixels(self.frame.size.height - pt.y, kDPI));
    me.keymods = toKeymods(e.modifierFlags);
    me.drag.buttons = 0;
    if (NSEvent.pressedMouseButtons & (1 << 0)) {
        me.drag.buttons |= int(uitk::MouseButton::kLeft);
    }
    if (NSEvent.pressedMouseButtons & (1 << 1)) {
        me.drag.buttons |= int(uitk::MouseButton::kRight);
    }
    if (NSEvent.pressedMouseButtons & (1 << 2)) {
        me.drag.buttons |= int(uitk::MouseButton::kMiddle);
    }
    if (NSEvent.pressedMouseButtons & (1 << 4)) {
        me.drag.buttons |= int(uitk::MouseButton::kButton4);
    }
    if (NSEvent.pressedMouseButtons & (1 << 5)) {
        me.drag.buttons |= int(uitk::MouseButton::kButton5);
    }

    self.callbacks->onMouse(me);
}

- (void)mouseUp:(NSEvent *)e
{
    [self otherMouseUp:e];
}

- (void)rightMouseUp:(NSEvent *)e
{
    [self otherMouseUp:e];
}

- (void)otherMouseUp:(NSEvent *)e
{
    NSPoint pt = [self convertPoint:e.locationInWindow fromView:nil];

    uitk::MouseEvent me;
    me.type = uitk::MouseEvent::Type::kButtonUp;
    me.pos = uitk::Point(uitk::PicaPt::fromPixels(pt.x, kDPI),
                         uitk::PicaPt::fromPixels(self.frame.size.height - pt.y, kDPI));
    me.keymods = toKeymods(e.modifierFlags);
    me.button.button = toUITKMouseButton(e.buttonNumber);

    self.callbacks->onMouse(me);
}

- (void)viewDidChangeEffectiveAppearance
{
    [super viewDidChangeEffectiveAppearance];
    // Apple's documentation says that colors can only be properly read from
    // -layout, -drawRect:, -updateLayer, and -updateConstraints.
    // Empirically, calling
    //     [NSAppearance setCurrentAppearance:self.effectiveAppearance];
    // also works. But since -layout gets called when the appearance changes,
    // we will notify ourself that we need to update the theme in -layout.
    // See https://developer.apple.com/documentation/uikit/appearance_customization/supporting_dark_mode_in_your_interface
    mAppearanceChanged = true;
}

@end

//-----------------------------------------------------------------------------
@interface UITKWindowDelegate : NSObject <NSWindowDelegate>
@property (weak) NSWindow *window;
@property uitk::IWindowCallbacks *callbacks;  // ObjC does not accept reference
@end

@implementation UITKWindowDelegate
- (id)initWithWindow:(NSWindow*)w andCallbacks:(uitk::IWindowCallbacks*)cb {
    if (self = [super init]) {
        self.window = w;
        self.callbacks = cb;
    }
    return self;
}

// A "main" window is the window that is the active, top-level window. (A "key"
// window is the window that has the key focus; usually the main window also
// has key focus.)
- (void)windowDidBecomeMain:(NSNotification *)notification
{
    NSPoint p = self.window.mouseLocationOutsideOfEventStream;
    NSRect contentRect = [self.window contentRectForFrameRect:self.window.frame];
    uitk::Point currMouse(uitk::PicaPt::fromPixels(p.x, kDPI),
                          uitk::PicaPt::fromPixels(contentRect.size.height - p.y, kDPI));
    self.callbacks->onActivated(currMouse);
}

- (void)windowDidResignMain:(NSNotification *)notification
{
    self.callbacks->onDeactivated();
}
@end
//-----------------------------------------------------------------------------
namespace uitk {

struct MacOSWindow::Impl {
    IWindowCallbacks& callbacks;
    NSWindow* window;
    UITKWindowDelegate* winDelegate;  // NSWindow doesn't retain the delegate
    ContentView* contentView;
};

MacOSWindow::MacOSWindow(IWindowCallbacks& callbacks,
                         const std::string& title, int width, int height,
                         Window::Flags::Value flags)
    : MacOSWindow(callbacks, title, -1, -1, width, height, flags)
{
}

MacOSWindow::MacOSWindow(IWindowCallbacks& callbacks,
                         const std::string& title, int x, int y, int width, int height,
                         Window::Flags::Value flags)
    : mImpl(new Impl{callbacks})
{
    int nsflags = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                  NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    mImpl->window = [[NSWindow alloc]
                     initWithContentRect:NSMakeRect(x, y, width, height)
                     styleMask:nsflags backing:NSBackingStoreBuffered defer:NO];
    mImpl->winDelegate = [[UITKWindowDelegate alloc]
                          initWithWindow:mImpl->window andCallbacks:&callbacks];
    mImpl->window.delegate = mImpl->winDelegate;
    mImpl->window.acceptsMouseMovedEvents = YES;
    mImpl->contentView = [[ContentView alloc] initWithCallbacks:&callbacks];
    mImpl->window.contentView = mImpl->contentView;
    // Set view as first responder, otherwise we won't get mouse events
    [mImpl->window makeFirstResponder:mImpl->contentView];

    setTitle(title);
}

MacOSWindow::~MacOSWindow()
{
    [mImpl->window close];  // if obj is nil, ObjC message does nothing
    mImpl->winDelegate = nil;
    mImpl->window = nil;
}

void* MacOSWindow::nativeHandle() { return (__bridge void*)mImpl->window; }
IWindowCallbacks& MacOSWindow::callbacks() { return mImpl->callbacks; }

bool MacOSWindow::isShowing() const
{
    return (mImpl->window.visible == YES ? true : false);
}

void MacOSWindow::show(bool show)
{
    [mImpl->window setIsVisible:(show ? YES : NO)];
    [mImpl->window makeKeyAndOrderFront:nil];
}

void MacOSWindow::close()
{
    [mImpl->window performClose:nil];
}

void MacOSWindow::setTitle(const std::string& title)
{
    mImpl->window.title = [NSString stringWithUTF8String:title.c_str()];
}

Rect MacOSWindow::contentRect() const
{
    return Rect(PicaPt::kZero,
                PicaPt::kZero,
                PicaPt::fromPixels(mImpl->contentView.frame.size.width, kDPI),
                PicaPt::fromPixels(mImpl->contentView.frame.size.height, kDPI));
}

void MacOSWindow::postRedraw() const
{
    mImpl->contentView.needsDisplay = YES;
}

void MacOSWindow::raiseToTop() const
{
    [mImpl->window makeKeyAndOrderFront:nil];
}

}  // namespace uitk
