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

#include "../Accessibility.h"
#include "../Widget.h"
#include "../Window.h"

#import "MacOSAccessibility.h"

#include <map>
#include <set>

@interface AccessibilityElement ()
{
    // We want to get a message whenever the user selects an element. This is not available.
    // Worse, VoiceOver initially iterates over all the elements, and none of the functions
    // are consistently called when the user selects an element.
    //   It seems that during initial iteration both title and value are called, then
    // the top-level element is called. After that, it seems like value is always called,
    // although sometimes title seems to be, too.
    VoiceOverState mVoiceOverState;
}
@end

@implementation AccessibilityElement

+ (AccessibilityElement*)updateAccessibleElements:(const std::vector<uitk::AccessibilityInfo>&)children
                                              for:(NSAccessibilityElement*)parent
                                         topLevel:(NSAccessibilityElement*)topLevel
                                    frameWinCoord:(const uitk::Rect&)frameWinCoord
                                         ulOffset:(NSPoint)ulOffset
                                        nsWinTopY:(CGFloat)nsWinTopY
                                              dpi:(float)dpi
                           returnElementForWidget:(uitk::Widget*)widget
                                    cache:(std::map<uitk::AccessibilityInfo::UID,
                                                    AccessibilityElement*>&)cache
{
    // Note that rectangles on Apple go from the lower left up towards the top of the screen

    AccessibilityElement* elementForWidget = nil;
    bool creating = cache.empty();

    auto parentFrame = NSMakeRect(frameWinCoord.x.toPixels(dpi) + ulOffset.x,
                                  nsWinTopY - frameWinCoord.y.toPixels(dpi) - frameWinCoord.height.toPixels(dpi),
                                  frameWinCoord.width.toPixels(dpi),
                                  frameWinCoord.height.toPixels(dpi));
    parent.accessibilityElement = YES;
    parent.accessibilityRole = NSAccessibilityGroupRole;
    parent.accessibilityFrame = parentFrame;
    if (parent == topLevel) {  // NSView isa NSAccessibilityElement
        parent.accessibilityLabel = @"Root element";
    }

    if (!children.empty()) {
        NSMutableArray *nselements = [[NSMutableArray alloc] initWithCapacity: children.size()];
        for (auto &e : children) {
            assert(e.type != uitk::AccessibilityInfo::Type::kNone);
            auto nsframe = NSMakeRect(e.frameWinCoord.x.toPixels(dpi) + ulOffset.x,
                                      nsWinTopY - e.frameWinCoord.y.toPixels(dpi) - e.frameWinCoord.height.toPixels(dpi),
                                      e.frameWinCoord.width.toPixels(dpi),
                                      e.frameWinCoord.height.toPixels(dpi));
            NSString *label = [NSString stringWithUTF8String:e.text.c_str()];
            AccessibilityElement *ae = nil;
            if (!creating) {
                auto it = cache.find(e.uniqueId());
                if (it != cache.end()) {
                    ae = it->second;
                } else {
                    assert(false);  // all elements should be in cache
                }
            }
            if (ae == nil) {  // either creating == true, or cache failed (unexpected, but let's not crash)
                ae = [[AccessibilityElement alloc] init];
                ae.accessibilityRole = NSAccessibilityStaticTextRole;
                ae.accessibilityParent = parent;
                ae.accessibilityTopLevelUIElement = topLevel;
            }
            if (e.isVisibleToUser) {
                ae.accessibilityElement = YES;
                ae.accessibilityFrame = nsframe;
                ae.accessibilityLabel = label;
            } else {
                ae.accessibilityElement = NO;
                ae.accessibilityFrame = CGRectZero;
                ae.accessibilityLabel = @"";
            }
            ae.accessibilityEnabled = (e.widget && e.widget->enabled()) ? YES : NO;
            ae.info = e;
            if (e.type == uitk::AccessibilityInfo::Type::kContainer ||
                e.type == uitk::AccessibilityInfo::Type::kRadioGroup ||
                e.type == uitk::AccessibilityInfo::Type::kList)
            {
                if (e.type == uitk::AccessibilityInfo::Type::kRadioGroup) {
                    ae.accessibilityRole = NSAccessibilityRadioGroupRole;
                } else if (e.type == uitk::AccessibilityInfo::Type::kList) {
                    ae.accessibilityRole = NSAccessibilityTableRole;
                } else {
                    ae.accessibilityRole = NSAccessibilityGroupRole;
                }
                AccessibilityElement *maybeForWidget =
                    [AccessibilityElement
                     updateAccessibleElements:e.children
                                          for:ae
                                     topLevel:topLevel
                                frameWinCoord:e.frameWinCoord
                                     ulOffset:ulOffset
                                    nsWinTopY:nsWinTopY
                                          dpi:dpi
                                   returnElementForWidget:widget
                                        cache:cache
                     ];
                if (!elementForWidget && maybeForWidget) {
                    elementForWidget = maybeForWidget;
                }
            } else if (e.type == uitk::AccessibilityInfo::Type::kCheckbox) {  // must come before kButton
                ae.accessibilityRole = NSAccessibilityCheckBoxRole;
            } else if (e.type == uitk::AccessibilityInfo::Type::kRadioButton) { // must come before kButton
                ae.accessibilityRole = NSAccessibilityRadioButtonRole;
            } else if (e.type == uitk::AccessibilityInfo::Type::kButton) {
                ae.accessibilityRole = NSAccessibilityButtonRole;
            } else if (e.type == uitk::AccessibilityInfo::Type::kIncDec) {
                ae.accessibilityRole = NSAccessibilityIncrementorRole;
            } else if (e.type == uitk::AccessibilityInfo::Type::kSlider) {
                ae.accessibilityRole = NSAccessibilitySliderRole;
            } else if (e.type == uitk::AccessibilityInfo::Type::kCombobox) {
                // macOS uses this instead of NSAccessibilityComboBoxRole
                ae.accessibilityRole = NSAccessibilityPopUpButtonRole;
            } else if (e.type == uitk::AccessibilityInfo::Type::kTextEdit ||
                       e.type == uitk::AccessibilityInfo::Type::kPassword) {
                ae.accessibilityRole = NSAccessibilityTextFieldRole;
                ae.accessibilityPlaceholderValue = [NSString stringWithUTF8String:e.placeholderText.c_str()];
                if (std::holds_alternative<std::string>(e.value)) {
                    // It would be nice to have VoiceOver speak the text / changes
                    // when they happen, but it is not at all clear how to do that,
                    // and a normal NSTextField does not do it, either.
                }
                if (e.type == uitk::AccessibilityInfo::Type::kPassword) {
                    ae.accessibilityProtectedContent = YES;
                }
            } else if (e.type == uitk::AccessibilityInfo::Type::kMenuItem) {
                ae.accessibilityRole = NSAccessibilityMenuItemRole;
            }

            // Either this is a custom menu item, or it's a clickable label, but either
            // way, ...MenuItemRole allows VO-Space to work and has no extra verbage (unlike a button role)
            if (ae.accessibilityRole == NSAccessibilityStaticTextRole && e.performLeftClick) {
                ae.accessibilityRole = NSAccessibilityMenuItemRole;
            }

            [nselements addObject:ae];
            if (!elementForWidget && e.widget == widget) {
                elementForWidget = ae;
            }
        }
        parent.accessibilityChildren = nselements;
    } else {
        parent.accessibilityChildren = nil;
    }

    return elementForWidget;
}

- (id)init
{
    if (self = [super init]) {
        mVoiceOverState = VoiceOverState::kNone;
    }
    return self;
}

// This is our function, called when the user navigates to an element.
// Unfortunately, macOS does not actually have a message for this, so
// we have to synthesize it ourselves. This may be called multiple times
// in a row.
- (void)userDidSelect
{
    // Ignore containers; they are not interactive
    if (self.info.type == uitk::AccessibilityInfo::Type::kContainer) {
        return;
    }

    //std::cout << "[debug] ACTIVATED " << (__bridge void *)self << " [" << self.info.text << "] " << uitk::Application::instance().microTime() << std::endl;

    // Special case for text: select the entire text on focus; this is what Apple does
    if (self.info.type == uitk::AccessibilityInfo::Type::kTextEdit) {
        if (auto *win = self.info.widget->window()) {
             if (self.info.performSelectAll && win->focusWidget() != self.info.widget) {
                 self.info.performSelectAll();
             }
         }
    }

    // Set key focus
    if (self.info.widget) {
        if (auto *win = self.info.widget->window()) {
            if (win->focusWidget() != self.info.widget) {
                win->setFocusWidget(self.info.widget);
                win->setNeedsDraw();
            }
        }
        ((id<RootAccessibilityElement>)super.accessibilityTopLevelUIElement).currentlyActiveElement = self;
    }
}
// This seems to get called once (or twice) when a new element is first selected.
// It is sometimes called when an element is re-selected.
- (id)accessibilityTopLevelUIElement
{
    //std::cout << "[debug] accessibilityTopLevelUIElement [" << self.info.text << "]" << std::endl;
    mVoiceOverState = VoiceOverState::kUserAction;
    [self userDidSelect];

    return [super accessibilityTopLevelUIElement];
}

- (id)accessibilityValue
{
    //std::cout << "[debug] accessibilityValue [" << self.info.text << "] " << uitk::Application::instance().microTime() << std::endl;
    if (mVoiceOverState == VoiceOverState::kUserAction) {
        [self userDidSelect];
    }

    if (!self.info.widget ||
        self.info.type == uitk::AccessibilityInfo::Type::kNone ||
        self.info.type == uitk::AccessibilityInfo::Type::kContainer ||
        self.info.type == uitk::AccessibilityInfo::Type::kRadioGroup ||
        self.info.type == uitk::AccessibilityInfo::Type::kList)
    {
        return nil;
    }

    auto ae = self.info.widget->accessibilityInfo();
    if (std::holds_alternative<bool>(ae.value)) {
        return std::get<bool>(ae.value) ? @YES : @NO;
    } else if (std::holds_alternative<int>(ae.value)) {
        return @(std::get<int>(ae.value));
    } else if (std::holds_alternative<double>(ae.value)) {
        return @(std::get<double>(ae.value));
    } else if (std::holds_alternative<std::string>(ae.value)) {
        return [NSString stringWithUTF8String:std::get<std::string>(ae.value).c_str()];
    }
    return nil;
}

- (id)accessibilityTitle
{
    //std::cout << "[debug] accessibilityTitle [" << self.info.text << "] -> \"" << [super accessibilityTitle] << "\"" << std::endl;
    switch (mVoiceOverState) {
        case VoiceOverState::kNone:
            mVoiceOverState = VoiceOverState::kIterating;
            break;
        case VoiceOverState::kIterating:
            break;
        case VoiceOverState::kUserAction:
            break;
    }
    return [super accessibilityTitle];
}
/*
- (NSString*)accessibilityLabel
{
    std::cout << "[debug] accessibilityLabel [" << self.info.text << "]" << std::endl;
    return [super accessibilityLabel];
}

- (float)accessibilityLabelValue
{
    std::cout << "[debug] accessibilityLabelValue [" << self.info.text << "]" << std::endl;
    return [super accessibilityLabelValue];
}
*/
- (NSArray*)accessibilitySelectedChildren
{
    //std::cout << "[debug] accessibilitySelectedChildren [" << self.info.text << "]" << std::endl;
    return [super accessibilitySelectedChildren];
}

- (void)setAccessibilitySelectedChildren:(NSArray *)sel
{
/*    std::cout << "[debug] setAccessibilitySelectedChildren [" << self.info.text << "]" << std::endl;
    std::cout << "[debug]    [";
    for (int i = 0;  i < sel.count;  ++i) {
        std::cout << "\"" << ((AccessibilityElement*)[sel objectAtIndex:i]).info.text << "\"";
    }
    std::cout << "]" << std::endl; */
    NSArray *oldSelection = [super accessibilitySelectedChildren];
    [super setAccessibilitySelectedChildren:sel];

    // The 'if' is probably not necessary, but the root is a ContentView, so if that
    // ever gets focused for some reason, we could have problems.
    if ([self isKindOfClass:AccessibilityElement.class]) {
        AccessibilityElement* ae = (AccessibilityElement*)self;
        if (ae.info.type == uitk::AccessibilityInfo::Type::kList) {
            std::unordered_set<void*> off, on;
            for (int i = 0;  i < oldSelection.count;  ++i) {
                off.insert((__bridge void*)[oldSelection objectAtIndex:i]);
            }
            for (int i = 0;  i < sel.count;  ++i) {
                void* e = (__bridge void*)[sel objectAtIndex:i];
                if (off.find(e) != off.end()) {
                    off.erase(e);
                } else {
                    on.insert((void*)e);
                }
            }

            for (auto e : off) {
                AccessibilityElement* ae = (__bridge AccessibilityElement*)e;
                if (ae.info.performLeftClick) {
                    ae.info.performLeftClick();
                }
            }
            for (auto e : on) {
                AccessibilityElement* ae = (__bridge AccessibilityElement*)e;
                if (ae.info.performLeftClick) {
                    ae.info.performLeftClick();
                }
            }
        }
    }
}
/*
- (NSArray*)accessibilitySelectedRows
{
    std::cout << "[debug] accessibilitySelectedRows [" << self.info.text << "]" << std::endl;
    return [super accessibilitySelectedRows];
}
*/
- (NSArray*)accessibilityChildren
{
    //std::cout << "[debug] accessibilityChildren [" << self.info.text << "]" << std::endl;
    if ([super accessibilityChildren].count > 0 && mVoiceOverState == VoiceOverState::kUserAction) {
        [self userDidSelect];
    // Text fields do not always query value when changing the selected accessibility element,
    // but they do consistently ask for children (which seems strange) and title.
    } else if (mVoiceOverState == VoiceOverState::kUserAction && self.info.type == uitk::AccessibilityInfo::Type::kTextEdit) {
        [self userDidSelect];
    }
    return [super accessibilityChildren];
}

- (NSArray*)accessibilityChildrenPure
{
    return [super accessibilityChildren];
}

- (void)setAccessibilityFocused:(BOOL)focused
{
    //std::cout << "[debug] setAccessibilityFocused" << std::endl;
    return [super setAccessibilityFocused:focused];
}

- (void)setAccessibilitySelected:(BOOL)selected
{
    //std::cout << "[debug] setAccessibilitySelected" << std::endl;
    return [super setAccessibilitySelected:selected];
}

// ---- For lists ----
/*- (NSInteger)accessibilityIndex
{
    std::cout << "[debug] accessibilityIndex [" << self.info.text << "]" << std::endl;
    return [super accessibilityIndex];
} */
// ----

// ---- NSAccessibilityButton ---
- (BOOL)accessibilityPerformPress
{
    if (self.accessibilityEnabled == YES && self.info.performLeftClick) {
        self.info.performLeftClick();
        return YES;
    }
    return NO;
}
// ----

// ---- NSAccessibilityStepper, NSAccessibilitySlider ----
- (BOOL)accessibilityPerformIncrement
{
    if (self.accessibilityEnabled == YES && self.info.performIncrementNumeric) {
        self.info.performIncrementNumeric();
        return YES;
    }
    return NO;
}

- (BOOL)accessibilityPerformDecrement
{
    if (self.accessibilityEnabled == YES && self.info.performDecrementNumeric) {
        self.info.performDecrementNumeric();
        return YES;
    }
    return NO;
}
//---
//---
/*- (NSInteger)accessibilityInsertionPointLineNumber
{
    std::cout << "[debug] accessibilityInsertionPointLineNumber" << std::endl;
    return [super accessibilityInsertionPointLineNumber];
}

-(NSRange)accessibilityVisibleCharacterRange
{
    std::cout << "[debug] accessibilityVisibleCharacterRange" << std::endl;
    return [super accessibilityVisibleCharacterRange];
}

-(NSString*)accessibilityStringForRange:(NSRange)range
{
    std::cout << "[debug] accessibilityStringForRange" << std::endl;
//    return [super accessibilityStringForRange];
    return self.accessibilityValue;  // debugging: use the actual range
}

-(NSInteger)accessibilityNumberOfCharacters
{
    std::cout << "[debug] accessibilityNumberOfCharacters: " << [super accessibilityNumberOfCharacters] << std::endl;
    return [super accessibilityNumberOfCharacters];
}

-(NSString *)accessibilitySelectedText
{
    std::cout << "[debug] accessibilitySelectedText" << std::endl;
    return [super accessibilitySelectedText];
}

-(NSRange)accessibilitySelectedTextRange
{
    auto range = [super accessibilitySelectedTextRange];
    std::cout << "[debug] accessibilitySelectedTextRange: " << range.location << ", " << range.length << std::endl;
    return [super accessibilitySelectedTextRange];
}
*/
//---
/*
- (BOOL)accessibilityPerformCancel
{
    std::cout << "[debug] perform cancel" << std::endl;
    return [super accessibilityPerformCancel];
}

- (BOOL)accessibilityPerformConfirm
{
    std::cout << "[debug] perform confirm" << std::endl;
    return [super accessibilityPerformConfirm];
}

- (BOOL)accessibilityPerformDelete
{
    std::cout << "[debug] perform delete" << std::endl;
    return [super accessibilityPerformDelete];
}

- (BOOL)accessibilityPerformPick
{
    std::cout << "[debug] perform pick" << std::endl;
    return [super accessibilityPerformPick];
}
*/
@end
