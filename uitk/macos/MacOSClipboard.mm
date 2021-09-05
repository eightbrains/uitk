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

#include "MacOSClipboard.h"

#import <AppKit/AppKit.h>

namespace uitk {

namespace {
NSString* bestTypeForString()
{
    return [NSPasteboard.generalPasteboard availableTypeFromArray:@[
        NSPasteboardTypeString, NSPasteboardTypeHTML, NSPasteboardTypeURL, NSPasteboardTypeFileURL ] ];
}

} // namespace

MacOSClipboard::MacOSClipboard()
{
}

MacOSClipboard::~MacOSClipboard()
{
}

bool MacOSClipboard::supportsX11SelectionString() const { return false; }
void MacOSClipboard::setX11SelectionString(const std::string& utf8) { }
std::string MacOSClipboard::x11SelectionString() const { return ""; }

bool MacOSClipboard::hasString() const
{
    NSString *best = bestTypeForString();
    return (best != nil);
}

std::string MacOSClipboard::string() const
{
    NSString *best = bestTypeForString();
    NSString *str = nil;
    if (best != nil) {
        str = [NSPasteboard.generalPasteboard stringForType:best];
    }
    if (str != nil) {
        return str.UTF8String;
    } else {
        return "";
    }
}

void MacOSClipboard::setString(const std::string& utf8)
{
    [NSPasteboard.generalPasteboard clearContents];
    if (!utf8.empty()) {
        NSString *s = [NSString stringWithUTF8String:utf8.c_str()];
        [NSPasteboard.generalPasteboard setString:s forType:NSPasteboardTypeString];
    }
}

} // namespace uitk



