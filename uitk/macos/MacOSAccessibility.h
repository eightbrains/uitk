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

#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>

#include <map>

enum class VoiceOverState
{
    kNone = 0,
    kIterating,
    kUserAction
};

@protocol RootAccessibilityElement
@required
@property NSAccessibilityElement* currentlyActiveElement;
@end

@interface AccessibilityElement : NSAccessibilityElement
@property uitk::AccessibilityInfo info;

+ (AccessibilityElement*)updateAccessibleElements:(const std::vector<uitk::AccessibilityInfo>&)children
                                              for:(NSAccessibilityElement*)parent
                                         topLevel:(NSAccessibilityElement*)topLevel
                                    frameWinCoord:(const uitk::Rect&)frameWinCoord
                                         ulOffset:(NSPoint)ulOffset
                                        nsWinTopY:(CGFloat)nsWinTopY
                                              dpi:(float)dpi
                           returnElementForWidget:(uitk::Widget*)widget
                                            cache:(std::map<uitk::AccessibilityInfo::UID,
                                                            AccessibilityElement*>&)cache;
- (id)init;
- (NSArray*)accessibilityChildrenPure;
@end
