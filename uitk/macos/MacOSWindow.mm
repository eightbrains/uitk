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
#include "../Cursor.h"
#include "../Events.h"
#include "../OSCursor.h"
#include <nativedraw.h>

#import <Cocoa/Cocoa.h>

#include <unordered_map>

namespace {
// macOS (and iOS) uses a 72 dpi assumption, and units to the API are
// given in 72 dpi pixels. Underneath the hood, the pixels might be
// smaller, so the fonts are scaled, the images are scaled, etc.
static const float kDPI = 72.0f;

static const float kPopopBorderWidth = 1;

static const std::unordered_map<unichar, uitk::Key> kKeychar2key = {
    { 3, uitk::Key::kEnter },
    { 127, uitk::Key::kBackspace },
    { 63232, uitk::Key::kUp },
    { 63233, uitk::Key::kDown },
    { 63234, uitk::Key::kLeft },
    { 63235, uitk::Key::kRight },
    { 63272, uitk::Key::kDelete },
    { 63273, uitk::Key::kHome },
    { 63275, uitk::Key::kEnd },
    { 63276, uitk::Key::kPageUp },
    { 63277, uitk::Key::kPageDown }
};

uitk::Key toKey(NSEvent *e)
{
    if (e.characters == nil || e.characters.length == 0) {
        return uitk::Key::kNone;
    }
    unichar keychar = [e.characters characterAtIndex:0];
    auto it = kKeychar2key.find(keychar);
    if (it != kKeychar2key.end()) {
        return it->second;
    }
    return uitk::Key(keychar);
}

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
    //if (flags & NSEventModifierFlagCapsLock) {
    //    keymods |= uitk::KeyModifier::kCapsLock;
    //}
    return keymods;
}

uitk::MouseButton toUITKMouseButton(NSInteger buttonNumber)
{
    switch (buttonNumber) {
        case 0:
            return uitk::MouseButton::kLeft;
        case 1:
            return uitk::MouseButton::kRight;
        case 2:
            return uitk::MouseButton::kMiddle;
        case 3:
            return uitk::MouseButton::kButton4;
        case 4:
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
    std::vector<std::function<void()>> mDeferredCalls;
}
@property uitk::IWindowCallbacks *callbacks;  // ObjC does not accept reference
@property bool inEvent;
@end

@implementation ContentView
- (id)initWithCallbacks:(uitk::IWindowCallbacks*)cb {
    if (self = [super init]) {
        mAppearanceChanged = false;
        self.callbacks = cb;
        self.inEvent = false;
    }
    return self;
}

- (void)addDeferredCall:(std::function<void()>)f
{
    mDeferredCalls.push_back(f);
}

- (float)dpi
{
    return kDPI;
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
    // (Docs say not to call super for this method)

    NSPoint pt = [self convertPoint:e.locationInWindow fromView:nil];

    uitk::MouseEvent me;
    me.type = uitk::MouseEvent::Type::kMove;
    me.pos = uitk::Point(uitk::PicaPt::fromPixels(pt.x, kDPI),
                         uitk::PicaPt::fromPixels(self.frame.size.height - pt.y, kDPI));
    me.keymods = toKeymods(e.modifierFlags);

    [self doOnMouse:me];
}

- (void)mouseDown:(NSEvent *)e
{
    // (Docs say not to call super for this method)

    [self otherMouseDown:e];
}

- (void)rightMouseDown:(NSEvent *)e
{
    // (Docs say we should to call super for -rightMouseDown:)
    [super rightMouseDown:e];
    [self otherMouseDown:e];
}

- (void)otherMouseDown:(NSEvent *)e
{
    // (Docs say not to call super for this method)

    NSPoint pt = [self convertPoint:e.locationInWindow fromView:nil];

    uitk::MouseEvent me;
    me.type = uitk::MouseEvent::Type::kButtonDown;
    me.pos = uitk::Point(uitk::PicaPt::fromPixels(pt.x, kDPI),
                         uitk::PicaPt::fromPixels(self.frame.size.height - pt.y, kDPI));
    me.keymods = toKeymods(e.modifierFlags);
    me.button.button = toUITKMouseButton(e.buttonNumber);
    me.button.nClicks = e.clickCount;

    [self doOnMouse:me];
}

- (void)mouseDragged:(NSEvent *)e
{
    // (Docs say not to call super for this method)

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

    [self doOnMouse:me];
}

- (void)mouseUp:(NSEvent *)e
{
    // (Docs say not to call super for this method)

    [self otherMouseUp:e];
}

- (void)rightMouseUp:(NSEvent *)e
{
    // (Docs say not to call super for this method)

    [self otherMouseUp:e];
}

- (void)otherMouseUp:(NSEvent *)e
{
    // (Docs say not to call super for this method)

    NSPoint pt = [self convertPoint:e.locationInWindow fromView:nil];

    uitk::MouseEvent me;
    me.type = uitk::MouseEvent::Type::kButtonUp;
    me.pos = uitk::Point(uitk::PicaPt::fromPixels(pt.x, kDPI),
                         uitk::PicaPt::fromPixels(self.frame.size.height - pt.y, kDPI));
    me.keymods = toKeymods(e.modifierFlags);
    me.button.button = toUITKMouseButton(e.buttonNumber);

    [self doOnMouse:me];
}

- (void)scrollWheel:(NSEvent *)e
{
    // (Docs say not to call super for this method)

    NSPoint pt = [self convertPoint:e.locationInWindow fromView:nil];

    uitk::MouseEvent me;
    me.type = uitk::MouseEvent::Type::kScroll;
    me.pos = uitk::Point(uitk::PicaPt::fromPixels(pt.x, kDPI),
                         uitk::PicaPt::fromPixels(self.frame.size.height - pt.y, kDPI));
    me.keymods = toKeymods(e.modifierFlags);
    me.scroll.dx = uitk::PicaPt::fromPixels(e.deltaX, kDPI);
    me.scroll.dy = uitk::PicaPt::fromPixels(e.deltaY, kDPI);

    [self doOnMouse:me];
}

- (void)doOnMouse:(uitk::MouseEvent&)me
{
    self.inEvent = true;
    self.callbacks->onMouse(me);
    self.inEvent = false;

    if (!mDeferredCalls.empty()) {
        for (auto &call : mDeferredCalls) {
            call();
        }
        mDeferredCalls.clear();
    }
}

- (void)keyDown:(NSEvent *)e
{
    [self doOnKey:uitk::KeyEvent::Type::kKeyDown event:e];
}

- (void)keyUp:(NSEvent *)e
{
    [self doOnKey:uitk::KeyEvent::Type::kKeyUp event:e];
}

- (void)doOnKey:(uitk::KeyEvent::Type)type event:(NSEvent*)e
{
    uitk::KeyEvent ke;
    ke.type = type;
    ke.keymods = toKeymods(e.modifierFlags);
    ke.isRepeat = (e.isARepeat == YES ? true : false);
    ke.key = toKey(e);

    self.inEvent = true;
    self.callbacks->onKey(ke);
    if (type == uitk::KeyEvent::Type::kKeyDown
        && (ke.keymods == 0 && ((ke.key >= uitk::Key::kSpace && ke.key < uitk::Key::kDelete)
                                || ke.key == uitk::Key::kUnknown))) {
        uitk::TextEvent te{ e.characters.UTF8String };
        self.callbacks->onText(te);
    }
    self.inEvent = false;
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

- (BOOL)windowShouldClose:(NSWindow *)sender
{
    return self.callbacks->onWindowShouldClose();
}

- (void)windowWillClose:(NSNotification *)notification
{
    self.callbacks->onWindowWillClose();
}
@end
//-----------------------------------------------------------------------------
namespace uitk {

struct MacOSWindow::Impl {
    IWindowCallbacks& callbacks;
    bool userCanClose = true;
    NSWindow* window;
    UITKWindowDelegate* winDelegate;  // NSWindow doesn't retain the delegate
    ContentView* contentView;
    Window::Flags::Value flags;
    Cursor cursor;
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
    mImpl->flags = flags;

    int nsflags = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                  NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    if (flags & Window::Flags::kPopup) {
        nsflags = NSWindowStyleMaskBorderless;
        mImpl->userCanClose = false;
    }
    mImpl->window = [[NSWindow alloc]
                     initWithContentRect:NSMakeRect(x, y, width, height)
                     styleMask:nsflags backing:NSBackingStoreBuffered defer:NO];
    mImpl->winDelegate = [[UITKWindowDelegate alloc]
                          initWithWindow:mImpl->window andCallbacks:&callbacks];
    mImpl->window.delegate = mImpl->winDelegate;
    mImpl->window.acceptsMouseMovedEvents = YES;
    mImpl->window.releasedWhenClosed = NO;
    mImpl->contentView = [[ContentView alloc] initWithCallbacks:&callbacks];
    mImpl->window.contentView = mImpl->contentView;
    // Set view as first responder, otherwise we won't get mouse events
    [mImpl->window makeFirstResponder:mImpl->contentView];

    if (flags & Window::Flags::kPopup) {
        mImpl->contentView.wantsLayer = YES;
        mImpl->contentView.layer.cornerRadius = 5;
        if (flags & Window::Flags::kMenuEdges) {
            mImpl->contentView.layer.maskedCorners = kCALayerMinXMinYCorner | kCALayerMaxXMinYCorner;
        }
        mImpl->contentView.layer.masksToBounds = YES;
        mImpl->contentView.layer.backgroundColor = mImpl->window.backgroundColor.CGColor;
        mImpl->contentView.layer.borderWidth = CGFloat(kPopopBorderWidth);
        mImpl->contentView.layer.borderColor = NSColor.separatorColor.CGColor;
        mImpl->window.backgroundColor = NSColor.clearColor;
    }

    setTitle(title);
}

MacOSWindow::~MacOSWindow()
{
    // There may still be events queued, and apparently macOS does not fully
    // delete the object until they are all done. However, we cannot be calling
    // any callbacks, because officially this object is deleted.
    mImpl->contentView.callbacks = nullptr;
    mImpl->window.delegate = nil;  // avoid calling will close multiple times
    [mImpl->window close];  // if obj is nil, ObjC message does nothing
    mImpl->window = nil;
    mImpl->winDelegate = nil;
    mImpl->contentView = nil;
}

void* MacOSWindow::nativeHandle() { return (__bridge void*)mImpl->window; }
IWindowCallbacks& MacOSWindow::callbacks() { return mImpl->callbacks; }

bool MacOSWindow::isShowing() const
{
    return (mImpl->window.visible == YES ? true : false);
}

void MacOSWindow::show(bool show, std::function<void(const DrawContext&)> onWillShow)
{
    if (show && mImpl->window.visible == NO && onWillShow) {
        // This DrawContext will not actually be drawable, since it is only drawable
        // within -draw, but it will be used in a LayoutContext. Since onWillShow
        // takes a const DrawContext, nothing can actually be drawn.
        onWillShow(*[mImpl->contentView createContext]);
    }
    [mImpl->window setIsVisible:(show ? YES : NO)];
    if (mImpl->flags & Window::Flags::kPopup) {
        // If we use -makeKeyAndOrderFront:, it works except when click-dragging, in
        // which case the popup pops-behind (presumably because it cannot be made key),
        // which is bad.
        // Note that -orderFront: sometimes doesn't put it in front, but
        // -orderFrontRegardless seems to.
        [mImpl->window orderFrontRegardless];
    } else {
        [mImpl->window makeKeyAndOrderFront:nil];
    }
}

void MacOSWindow::toggleMinimize()
{
    [mImpl->window miniaturize:NSApp];
}

void MacOSWindow::toggleMaximize()
{
    [mImpl->window zoom:NSApp];
}

void MacOSWindow::close()
{
    if (mImpl->contentView.inEvent) {
        [mImpl->contentView addDeferredCall: [this]() { this->close(); }];
        return;
    }

    if (mImpl->userCanClose) {
        // Simulate clicking the close button, so that onWindowShouldClose can
        // be called (if one exists).
        [mImpl->window performClose:nil];
    } else {
        // If the close button does not exist, -performClose: beeps and does not
        // do anything, which is undesirable (especially since there is no other
        // way to close the window than close()!).
        [mImpl->window close];
    }
}

void MacOSWindow::raiseToTop() const
{
    [mImpl->window makeKeyAndOrderFront:nil];
}

void MacOSWindow::setTitle(const std::string& title)
{
    mImpl->window.title = [NSString stringWithUTF8String:title.c_str()];
}

void MacOSWindow::setCursor(const Cursor& cursor)
{
    mImpl->cursor = cursor;
    if (auto *osCursor = mImpl->cursor.osCursor()) {
        osCursor->set();
    }
}

Rect MacOSWindow::contentRect() const
{
    return Rect(PicaPt::kZero,
                PicaPt::kZero,
                PicaPt::fromPixels(mImpl->contentView.frame.size.width, kDPI),
                PicaPt::fromPixels(mImpl->contentView.frame.size.height, kDPI));
}

OSWindow::OSRect MacOSWindow::osContentRect() const
{
    // Since the lower left is at the bottom left of the window, oscontentrect.origin is
    // the same as the osframe.origin. The height, of osframe is higher, though, because
    // it includes the titlebar.
    OSRect f = osFrame();
    return OSRect{f.x,
                  f.y,
                  float(mImpl->contentView.frame.size.width),
                  float(mImpl->contentView.frame.size.height)};
}

void MacOSWindow::setContentSize(const Size& size)
{
    [mImpl->window setContentSize:NSMakeSize(size.width.toPixels(dpi()),
                                             size.height.toPixels(dpi()))];
}

float MacOSWindow::dpi() const
{
    return mImpl->contentView.dpi;
}

OSWindow::OSRect MacOSWindow::osFrame() const
{
    NSRect f = mImpl->window.frame;
    return OSRect{float(f.origin.x), float(f.origin.y),
                  float(f.size.width), float(f.size.height)};
}

void MacOSWindow::setOSFrame(float x, float y, float width, float height)
{
    [mImpl->window setFrame:NSMakeRect(x, y, width, height) display:YES];
}

PicaPt MacOSWindow::borderWidth() const
{
    if (mImpl->window.styleMask == 0) {  // NSWindowStyleMaskBorderless == 0
        return PicaPt::fromPixels(kPopopBorderWidth, dpi());
    } else {
        auto r = [mImpl->window frameRectForContentRect:NSMakeRect(0, 0, 100, 100)];
        return PicaPt::fromPixels(r.origin.x, dpi());
    }
}

void MacOSWindow::postRedraw() const
{
    mImpl->contentView.needsDisplay = YES;
}

void MacOSWindow::beginModalDialog(OSWindow *w)
{
    if (auto *macWindow = dynamic_cast<MacOSWindow*>(w)) {
        mImpl->callbacks.onDeactivated();  // -beginSheet doesn't seem to do this
        [mImpl->window beginSheet:macWindow->mImpl->window
                completionHandler:^(NSModalResponse returnCode) {}];
    } else {
        assert(false);
    }
}

void MacOSWindow::endModalDialog(OSWindow *w)
{
    if (auto *macWindow = dynamic_cast<MacOSWindow*>(w)) {
        [mImpl->window endSheet:macWindow->mImpl->window];
    } else {
        assert(false);
    }
}

Point MacOSWindow::currentMouseLocation() const
{
    NSPoint p = [mImpl->window mouseLocationOutsideOfEventStream];
    NSRect contentRect = [mImpl->window contentRectForFrameRect:mImpl->window.frame];
    return Point(PicaPt::fromPixels(p.x, dpi()),
                 PicaPt::fromPixels(contentRect.size.height - p.y, dpi()));
}

}  // namespace uitk
