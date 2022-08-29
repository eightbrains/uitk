//-----------------------------------------------------------------------------
// Copyright 2021 - 2022 Eight Brains Studios, LLC
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
#include "../TextEditorLogic.h"
#include "../private/Utils.h"
#include <nativedraw.h>

#import <Cocoa/Cocoa.h>

#include <unordered_map>

namespace {

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
@interface ContentView : NSView<NSTextInputClient>
@end

@interface ContentView ()
{
    bool mAppearanceChanged;
    std::vector<std::function<void()>> mDeferredCalls;
}
@property uitk::IWindowCallbacks *callbacks;  // ObjC does not accept reference
@property bool inEvent;
@property float dpi;
@property float hiresDPI;
@property uitk::TextEditorLogic* textEditor;
@property uitk::Rect textEditorFrame;
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

- (void)updateDPI
{
    NSScreen *screen = nil;
    if (self.window != nil) {
        screen = self.window.screen;
    }
    if (!screen) {
        screen = NSScreen.deepestScreen;
    }

    // uiDPI is actually the native resolution of the screen. Setting this as the
    // DPI ensures that the PicaPt(72) is one inch in native resolution, but is
    // scaled appropriately if the user chooses something different in
    // Settings >> Display >> Resolution. Note that large, e.g. 15" MBPs do not use the
    // native resolution by default, since 2016! This means that, by default,
    // some screens--likely to be used by developers--will (correctly) display
    // using a scaled length and not the actual physical length specified. This
    // may surprise developers, but will match user expectations.
    float uiDPI, hiDPI;
    uitk::DrawContext::getScreenDPI((__bridge void*)screen, &uiDPI, nullptr, &hiDPI);
    self.dpi = uiDPI;
    self.hiresDPI = hiDPI;
}

- (void)addDeferredCall:(std::function<void()>)f
{
    mDeferredCalls.push_back(f);
}

// Note:  on macOS, draw contexts are transitory
- (std::shared_ptr<uitk::DrawContext>)createContext {
    int width = int(std::ceil(self.bounds.size.width));
    int height = int(std::ceil(self.bounds.size.height));

    auto cgContext = NSGraphicsContext.currentContext.CGContext;
    return uitk::DrawContext::fromCoreGraphics(cgContext, width, height, self.dpi);
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

    float dpi = self.dpi;
    uitk::MouseEvent me;
    me.type = uitk::MouseEvent::Type::kMove;
    me.pos = uitk::Point(uitk::PicaPt::fromPixels(pt.x, dpi),
                         uitk::PicaPt::fromPixels(self.frame.size.height - pt.y, dpi));
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

    float dpi = self.dpi;
    uitk::MouseEvent me;
    me.type = uitk::MouseEvent::Type::kButtonDown;
    me.pos = uitk::Point(uitk::PicaPt::fromPixels(pt.x, dpi),
                         uitk::PicaPt::fromPixels(self.frame.size.height - pt.y, dpi));
    me.keymods = toKeymods(e.modifierFlags);
    me.button.button = toUITKMouseButton(e.buttonNumber);
    me.button.nClicks = e.clickCount;

    [self doOnMouse:me];
}

- (void)mouseDragged:(NSEvent *)e
{
    // (Docs say not to call super for this method)

    NSPoint pt = [self convertPoint:e.locationInWindow fromView:nil];

    float dpi = self.dpi;
    uitk::MouseEvent me;
    me.type = uitk::MouseEvent::Type::kDrag;
    me.pos = uitk::Point(uitk::PicaPt::fromPixels(pt.x, dpi),
                         uitk::PicaPt::fromPixels(self.frame.size.height - pt.y, dpi));
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

    float dpi = self.dpi;
    uitk::MouseEvent me;
    me.type = uitk::MouseEvent::Type::kButtonUp;
    me.pos = uitk::Point(uitk::PicaPt::fromPixels(pt.x, dpi),
                         uitk::PicaPt::fromPixels(self.frame.size.height - pt.y, dpi));
    me.keymods = toKeymods(e.modifierFlags);
    me.button.button = toUITKMouseButton(e.buttonNumber);

    [self doOnMouse:me];
}

- (void)scrollWheel:(NSEvent *)e
{
    // (Docs say not to call super for this method)

    NSPoint pt = [self convertPoint:e.locationInWindow fromView:nil];

    float dpi = self.dpi;
    uitk::MouseEvent me;
    me.type = uitk::MouseEvent::Type::kScroll;
    me.pos = uitk::Point(uitk::PicaPt::fromPixels(pt.x, dpi),
                         uitk::PicaPt::fromPixels(self.frame.size.height - pt.y, dpi));
    me.keymods = toKeymods(e.modifierFlags);
    CGFloat dx, dy;
    if (e.hasPreciseScrollingDeltas) {  // trackpad
        dx = e.scrollingDeltaX;
        dy = e.scrollingDeltaY;
    } else {                            // discrete mouse wheel
        // What constant should we use? More complex controls seem to
        // multiply by the font height they are using, but we do not
        // have access to that information here (nor would it be a great
        // choice for Apple to use with, say, NSTableView). And in macOS
        // adjusts the scroll value depending on how quickly the events
        // comes, so it would be difficult to scroll by one line, anyway.
        // 12 seems to fit well with my system's behavior.
        dx = 12.0f * e.scrollingDeltaX;
        dy = 12.0f * e.scrollingDeltaY;
    }
    me.scroll.dx = uitk::PicaPt::fromPixels(dx, dpi);
    me.scroll.dy = uitk::PicaPt::fromPixels(dy, dpi);

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
    if (type == uitk::KeyEvent::Type::kKeyDown) {
        if (self.textEditor) {
            [self interpretKeyEvents:@[e] ];  // handles NSTextInputClient
        }
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

// ---- NSTextInputClient ----
// We store text in UTF8, as is right and proper, but macOS chose UTF16 before
// it became apparent that this was insufficient. So we need to convert back
// and forth. It is not clear how to do this efficiently: since it requires
// scanning the string up to the index (frequently the end), and the string
// will change frequently since it is being edited.
// On a 2019 MBP, 2.9 GHz i9:
// - utf16IndicesForUTF8Indices() takes about 10 ns
//   per character (140 ns in debug mode). Moby Dick is about 1215236 characters,
//   which would take under 6 ms (170 ms debug). One hopes an editor capable of
//   handling Moby Dick would break the text up a bit, but even if not, this
//   worst case should still be handleable.
// - utf8IndicesForUTF16Indices() takes about 12 ns (18 ns), so Moby Dick would
//   be about 15 ms (22 ms).
- (BOOL)hasMarkedText
{
    return (self.textEditor && !self.textEditor->imeConversion().isEmpty());
}

- (NSRange)markedRange
{
    if (self.textEditor) {
        auto utf8 = self.textEditor->textWithConversion();
        auto toUtf16 = uitk::utf16IndicesForUTF8Indices(utf8.c_str());
        int start8 = self.textEditor->imeConversion().start;
        int end8 = start8 + self.textEditor->imeConversion().text.length();
        return NSMakeRange(toUtf16[start8], toUtf16[end8] - toUtf16[start8]);
    } else {
        return NSMakeRange(NSNotFound, 0);
    }
}

// TODO: this might get called during draw, in which case we need to cache toUtf16 somehow
- (NSRange)selectedRange {
    if (self.textEditor) {
        auto utf8 = self.textEditor->textWithConversion();
        auto toUtf16 = uitk::utf16IndicesForUTF8Indices(utf8.c_str());
        int start8 = self.textEditor->selection().start;
        int end8 = self.textEditor->selection().end;
        return NSMakeRange(toUtf16[start8], toUtf16[end8] - toUtf16[start8]);
    } else {
        return NSMakeRange(NSNotFound, 0);
    }
}

- (void)setMarkedText:(nonnull id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange {
    if (self.textEditor) {
        // TODO: not sure what selectedRange is for
        std::string utf8;
        if ([string isKindOfClass:[NSAttributedString class]]) {
            utf8 = std::string(((NSAttributedString*)string).string.UTF8String);
        } else {
            utf8 = std::string(((NSString*)string).UTF8String);
        }
        auto sel = self.textEditor->selection();
        self.textEditor->setIMEConversion(uitk::TextEditorLogic::IMEConversion(sel.start, utf8));
        self.needsDisplay = YES;
    }
}

- (void)unmarkText {
    if (self.textEditor) {
        self.textEditor->setIMEConversion(uitk::TextEditorLogic::IMEConversion());
        self.needsDisplay = YES;
    }
}

- (nonnull NSArray<NSAttributedStringKey> *)validAttributesForMarkedText {
    return @[];
}

- (nullable NSAttributedString *)attributedSubstringForProposedRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange {
    if (self.textEditor) {
        auto utf8 = self.textEditor->textWithConversion();
        auto fromUtf16 = uitk::utf8IndicesForUTF16Indices(utf8.c_str());
        int start16 = std::min(int(range.location), int(fromUtf16.size() - 1));
        int end16 = std::min(start16 + int(range.length), int(fromUtf16.size() - 1));
        int start8 = fromUtf16[start16];
        int end8 = fromUtf16[end16];
        auto substr = utf8.substr(start8, start8 - end8);
        NSString *nsstr = [NSString stringWithUTF8String:substr.c_str()];
        if (actualRange) {
            *actualRange = NSMakeRange(start16, end16 - start16);
        }
        return [[NSAttributedString alloc] initWithString:nsstr];
    } else {
        return nil;
    }
}

- (void)insertText:(nonnull id)string replacementRange:(NSRange)replacementRange {
    if (self.textEditor) {
        auto sel = self.textEditor->selection();
        if (sel.end > sel.start) {
            self.textEditor->deleteText(sel.start, sel.end);
        } else if (replacementRange.length > 0) {
            // TODO: is the replacement range in the marked text or the committed text?
            // TODO: when does this happen? Doesn't seem to happen with Chinese, maybe Japanese?
            self.textEditor->deleteText(replacementRange.location,
                                        replacementRange.location + replacementRange.length);
        }
        std::string utf8;
        if ([string isKindOfClass:[NSAttributedString class]]) {
            utf8 = std::string(((NSAttributedString*)string).string.UTF8String);
        } else {
            utf8 = std::string(((NSString*)string).UTF8String);
        }
        self.textEditor->insertText(sel.start, utf8);
        self.textEditor->setIMEConversion(uitk::TextEditorLogic::IMEConversion());

        //std::cout << "[debug] insertText: '" << utf8 << "', range: (" << replacementRange.location << ", " << replacementRange.length << ")" << std::endl;

        int endIdx = sel.start + int(utf8.size());
            sel = uitk::TextEditorLogic::Selection(endIdx, endIdx,
                                                   uitk::TextEditorLogic::Selection::CursorLocation::kEnd);
        self.textEditor->setSelection(sel);
        if (self.textEditor->onTextChanged) {
            self.textEditor->onTextChanged();
        }

        self.needsDisplay = YES;
    }
}

- (NSUInteger)characterIndexForPoint:(NSPoint)p {
    // TODO: when is this called?
    if (self.textEditor) {
        p = [self.window convertPointFromScreen:p];  // screen -> window
        p = [self convertPoint:p fromView:nil]; // window -> view
        // TODO: this probably needs to be with the marked text
        int idx8 = self.textEditor->indexAtPoint(uitk::Point(uitk::PicaPt::fromPixels(p.x, self.dpi),
                                                             uitk::PicaPt::fromPixels(p.y, self.dpi)));
        auto toUtf16 = uitk::utf16IndicesForUTF8Indices(self.textEditor->textForRange(0, idx8).c_str());
        return toUtf16[idx8];
    }
    return 0;
}

// TODO: convert from UTF16 indices
- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange {
    if (self.textEditor) {
        auto &glyphs = self.textEditor->layout()->glyphs();
        const uitk::TextLayout::Glyph *g;
        int idx = range.location;
        if (idx < glyphs.size()) {
            g = &glyphs[idx];
        } else {
            if (glyphs.empty()) {
                g = nullptr;
            } else {
                g = &glyphs[glyphs.size() - 1];
            }
        }

        NSRect r;
        if (g) {
            r = NSMakeRect(g->frame.x.toPixels(self.dpi), g->frame.y.toPixels(self.dpi),
                           g->frame.maxX().toPixels(self.dpi), g->frame.maxY().toPixels(self.dpi));
        } else {
            r = NSMakeRect(0, 0, 0, 0);  // empty text (TODO: use appropriate height)
        }
        // The docs say the rect is the nearest suitable index in the range
        // (given composition characters, etc.), or to the end of the line
        // if the range extends beyond the first line.
        while (idx < range.location + range.length && idx < glyphs.size() && glyphs[idx].frame.y.toPixels(self.dpi) < r.origin.x + r.size.height) {
            r.size.width = glyphs[idx].frame.maxX().toPixels(self.dpi) - r.origin.x;
            idx++;
        }
        if (actualRange) {
            *actualRange = NSMakeRange(range.location, idx - range.location);
        }

        // The coordinates are in screen coordinates (presumably so that
        // the IME conversion window knows where to go). macOS wants to put
        // the window underneath the text.
        r.origin.x += self.textEditorFrame.x.toPixels(self.dpi);
        r.origin.y += self.frame.size.height - self.textEditorFrame.maxY().toPixels(self.dpi);
        r.origin = [self convertPoint:r.origin toView:nil]; // to window coord
        r.origin = [self.window convertPointToScreen:r.origin];  // to screen coord

        return r;
    }

    if (actualRange) {
        *actualRange = NSMakeRange(NSNotFound, 0);
    }
    return NSMakeRect(0, 0, 0, 0);
}

- (void)doCommandBySelector:(nonnull SEL)selector
{
    // Do nothing: TextEditorLogic handles these right now.
    // However, we should probably use NSStringFromSelector() and have a
    // bunch of ugly if's that call the appropriate functions. This would make
    // sure that things like emacs keybindings work, as well as any voice
    // editing commands that macOS may have.
}
// ----

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

- (void)windowDidChangeScreen:(NSNotification *)notification
{
    ContentView *cv = (ContentView*)self.window.contentView;
    if (cv != nil) {
        [cv updateDPI];
    }
}

// A "main" window is the window that is the active, top-level window. (A "key"
// window is the window that has the key focus; usually the main window also
// has key focus.)
- (void)windowDidBecomeMain:(NSNotification *)notification
{
    NSPoint p = self.window.mouseLocationOutsideOfEventStream;
    NSRect contentRect = [self.window contentRectForFrameRect:self.window.frame];
    ContentView *cv = (ContentView*)self.window.contentView;
    uitk::Point currMouse(uitk::PicaPt::fromPixels(p.x, cv.dpi),
                          uitk::PicaPt::fromPixels(contentRect.size.height - p.y, cv.dpi));
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
    // Everything else assumes dpi is properly set. (Note that we cannot call the delegate's
    // -windowDidChangeScreen: function because we cannot create an NSNotification. Also note
    // that -windowDidChangeScreen: is NOT called for the initial screen)
    [mImpl->contentView updateDPI];
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

void MacOSWindow::callWithLayoutContext(std::function<void(const DrawContext&)> f)
{
    f(*[mImpl->contentView createContext]);
}

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
    ContentView *cv = (ContentView*)mImpl->window.contentView;
    return Rect(PicaPt::kZero,
                PicaPt::kZero,
                PicaPt::fromPixels(mImpl->contentView.frame.size.width, cv.dpi),
                PicaPt::fromPixels(mImpl->contentView.frame.size.height, cv.dpi));
}

OSRect MacOSWindow::osContentRect() const
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
    // macOS apparently expands the window away from the origin, but since the
    // origin is in the lower left and our origin is in the upper right, this
    // is a problem. This means we cannot use -setContentSize:, but need to set
    // the frame.
    auto currentOSContentRect = osContentRect();
    auto newOSWidth = size.width.toPixels(dpi());
    auto newOSHeight = size.height.toPixels(dpi());
    auto dw = newOSWidth - currentOSContentRect.width;
    auto dh = newOSHeight - currentOSContentRect.height;
    auto f = osFrame();
    auto titlebarHeight = f.height - currentOSContentRect.height;
    f.y -= dh;  // minus, because this is the axis where the direction flips
    setOSFrame(f.x, f.y, newOSWidth, newOSHeight + titlebarHeight);
}

float MacOSWindow::dpi() const
{
    return mImpl->contentView.dpi;
}

OSRect MacOSWindow::osFrame() const
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

OSScreen MacOSWindow::osScreen() const
{
    NSScreen *screen = mImpl->window.screen;
    if (screen) {
        float uiDPI;
        DrawContext::getScreenDPI((__bridge void *)screen, &uiDPI, nullptr, nullptr);
        auto desktop = screen.visibleFrame;
        auto fullscreen = screen.frame;
        return {
            { float(desktop.origin.x), float(desktop.origin.y),
              float(desktop.size.width), float(desktop.size.height) },
            { float(fullscreen.origin.x), float(fullscreen.origin.y),
              float(fullscreen.size.width), float(fullscreen.size.height) },
            uiDPI
        };
    } else {
        return { OSRect{0, 0, 0, 0}, OSRect{0, 0, 0, 0}, 96.0f };
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

void MacOSWindow::setTextEditing(TextEditorLogic *te, const Rect& frame)
{
    mImpl->contentView.textEditor = te;
    mImpl->contentView.textEditorFrame = frame;
}

}  // namespace uitk
