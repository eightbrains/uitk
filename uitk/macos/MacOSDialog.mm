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

#include "MacOSDialog.h"

#include "MacOSWindow.h"
#include "../Window.h"

#include <typeinfo>

#import <AppKit/AppKit.h>

@interface FileTypeChooser : NSPopUpButton
@end

@interface FileTypeChooser ()
{
    std::vector<std::string> mExtensions;
}
@property(weak) NSSavePanel* dialog;
@end

@implementation FileTypeChooser
- (id)initWithDialog:(NSSavePanel*)save types:(std::vector<uitk::MacOSDialog::FileType>)types
{
    self = [super init];
    if (self) {
        self.dialog = save;
        mExtensions.reserve(types.size());
        for (auto &e : types) {
            mExtensions.push_back(e.ext);

            std::string text;
            if (!e.ext.empty()) {
                text = e.description + " (*." + e.ext + ")";
            } else {
                text = e.description;
            }
            [self addItemWithTitle:[NSString stringWithUTF8String:text.c_str()]];
        }

        [self sizeToFit];
        // We want some vertical spacing; conveniently, Cocoa widgets will center themselves
        // vertically if they have more vertical space than they want.
        self.frame = NSMakeRect(0, 0, self.frame.size.width, 2.0 * self.frame.size.height);

        self.target = self;
        self.action = @selector(onChanged);
    }
    return self;
}

- (void)onChanged
{
    auto &ext = mExtensions[self.indexOfSelectedItem];
    if (!ext.empty()) {
        self.dialog.allowedFileTypes = @[ [NSString stringWithUTF8String:ext.c_str()] ];
        self.dialog.allowsOtherFileTypes = NO;
    } else {
        self.dialog.allowedFileTypes = nil;
        self.dialog.allowsOtherFileTypes = YES;
    }
}
@end
//-----------------------------------------------------------------------------
namespace uitk {

void MacOSDialog::showAlert(Window *w,
                            const std::string& title,
                            const std::string& message,
                            const std::string& info,
                            const std::vector<std::string>& buttons,
                            std::function<void(Dialog::Result, int)> onDone)
{
    NSAlert *dlg = [[NSAlert alloc] init];
    if (!message.empty()) {
        dlg.messageText = [NSString stringWithUTF8String:message.c_str()];
    }
    if (!info.empty()) {
        dlg.informativeText = [NSString stringWithUTF8String:info.c_str()];
    }
    for (auto &b : buttons) {
        [dlg addButtonWithTitle: [NSString stringWithUTF8String:b.c_str()]];
    }

    auto handleDone = [onDone](NSModalResponse returnCode) {
        switch (returnCode) {
            case NSModalResponseCancel:
                onDone(Dialog::Result::kCancelled, 0);
                break;
            case NSModalResponseOK:
            case NSAlertFirstButtonReturn:
                onDone(Dialog::Result::kFinished, 0);
                break;
            case NSAlertSecondButtonReturn:
                onDone(Dialog::Result::kFinished, 1);
                break;
            case NSAlertThirdButtonReturn:
                onDone(Dialog::Result::kFinished, 2);
                break;
            default:
                assert(returnCode > NSAlertThirdButtonReturn);
                onDone(Dialog::Result::kFinished, 2 + int(returnCode - NSAlertThirdButtonReturn));
                break;
        }
    };

    if (w) {
        auto *macWin = dynamic_cast<MacOSWindow*>(w->nativeWindow());
        assert(macWin);
        if (macWin) {
            [dlg beginSheetModalForWindow:(__bridge NSWindow*)w->nativeHandle()
                        completionHandler:^ (NSModalResponse returnCode) {
                handleDone(returnCode);
            } ];
        }
    } else {
        if (dlg.window != nil) {
            dlg.window.level = NSModalPanelWindowLevel;
        }
        [NSApplication.sharedApplication activateIgnoringOtherApps:YES];
        auto returnCode = [dlg runModal];
        handleDone(returnCode);
    }
}

void MacOSDialog::showSave(Window *w,
                           const std::string& title,
                           const std::string& dir,
                           const std::vector<FileType>& extensions,
                           std::function<void(Dialog::Result, const std::string&)> onDone)
{
    NSSavePanel *dlg = [[NSSavePanel alloc] init];
    // Don't set dlg.directoryURL, NSSavePanel already handles this itself.
    if (extensions.empty()) {
        dlg.allowsOtherFileTypes = YES;
    } else {
        NSMutableArray *nsexts = [[NSMutableArray alloc] init];
        for (auto &e : extensions) {
            if (!e.ext.empty()) {
                [nsexts addObject:[NSString stringWithUTF8String:e.ext.c_str()]];
            } else {
                dlg.allowsOtherFileTypes = YES;
            }
        }
        if (nsexts.count > 0) {  // exception if nsexts has nothing ({FileType{""}} passed in)
            dlg.allowedFileTypes = nsexts;

/*                NSPopUpButton *fileTypeChooser = [[NSPopUpButton alloc] init];
            for (auto &e : extensions) {
                std::string text;
                if (!e.ext.empty()) {
                    text = e.description + " (*." + e.ext + ")";
                } else {
                    text = e.description;
                }
                [fileTypeChooser addItemWithTitle:[NSString stringWithUTF8String:text.c_str()]];
            }
            [fileTypeChooser sizeToFit];
            // We want some vertical spacing; conveniently, Cocoa widgets will center themselves
            // vertically if they have more vertical space than they want.
            fileTypeChooser.frame = NSMakeRect(0, 0, fileTypeChooser.frame.size.width, 2.0 * fileTypeChooser.frame.size.height);
            dlg.accessoryView = fileTypeChooser; */
            dlg.accessoryView = [[FileTypeChooser alloc]
                                 initWithDialog:dlg types:extensions];
        }
    }
    NSWindow *parent = nil;
    if (w) {
        auto *macWin = dynamic_cast<MacOSWindow*>(w->nativeWindow());
        assert(macWin);
        if (macWin) {
            parent = (__bridge NSWindow*)w->nativeHandle();
        }
    }

    auto handleDone = [onDone, dlg](NSModalResponse result) {
        // NSModalResponseCancel == NSFileHandlingPanelCancelButton; docs say the latter,
        // but that turns out to be deprecated.
        if (result != NSModalResponseCancel) {
            onDone(Dialog::Result::kFinished, dlg.URL.filePathURL.absoluteString.UTF8String);
        } else {
            onDone(Dialog::Result::kCancelled, "");
        }
    };

    if (parent) {
        [dlg beginSheetModalForWindow:parent completionHandler:^(NSModalResponse result) {
            handleDone(result);
        }];
    } else {
        dlg.level = NSModalPanelWindowLevel;
        [NSApplication.sharedApplication activateIgnoringOtherApps:YES];
        auto result = [dlg runModal];
        handleDone(result);
    }
}

void MacOSDialog::showOpen(Window *w,
                           const std::string& title,
                           const std::string& dir,
                           const std::vector<FileType>& extensions,
                           bool canSelectDirectories,
                           bool canSelectMultipleFiles,
                           std::function<void(Dialog::Result, const std::vector<std::string>&)> onDone)
{
    NSOpenPanel *dlg = [[NSOpenPanel alloc] init];
    // Don't set dlg.directoryURL, NSOpenPanel already handles this itself.
    dlg.canChooseDirectories = (canSelectDirectories ? YES : NO);
    dlg.allowsMultipleSelection = (canSelectMultipleFiles ? YES : NO);
    if (extensions.empty()) {
        dlg.allowsOtherFileTypes = YES;
    } else {
        NSMutableArray *nsexts = [[NSMutableArray alloc] init];
        for (auto &e : extensions) {
            if (e.ext.empty()) {
                dlg.allowsOtherFileTypes = YES;
            } else {
                [nsexts addObject:[NSString stringWithUTF8String:e.ext.c_str()]];
            }
        }
        if (nsexts.count > 0) {  // exception if nsexts has nothing ({FileType{""}} passed in)
            dlg.allowedFileTypes = nsexts;
        }
    }

    NSWindow *parent = nil;
    if (w) {
        auto *macWin = dynamic_cast<MacOSWindow*>(w->nativeWindow());
        assert(macWin);
        if (macWin) {
            parent = (__bridge NSWindow*)w->nativeHandle();
        }
    }

    auto handleDone = [onDone, dlg](NSModalResponse result) {
        // NSModalResponseCancel == NSFileHandlingPanelCancelButton; docs say the latter,
        // but that turns out to be deprecated.
        if (result != NSModalResponseCancel) {
            std::vector<std::string> paths;
            paths.reserve(dlg.URLs.count);
            for (int i = 0;  i < dlg.URLs.count;  ++i) {
                paths.push_back([dlg.URLs objectAtIndex:i].fileSystemRepresentation);
            }
            onDone(Dialog::Result::kFinished, paths);
        } else {
            onDone(Dialog::Result::kCancelled, {});
        }
    };

    if (parent) {
        [dlg beginSheetModalForWindow:parent completionHandler:^(NSModalResponse result) {
            handleDone(result);
        }];
    } else {
        dlg.level = NSModalPanelWindowLevel;
        [NSApplication.sharedApplication activateIgnoringOtherApps:YES];
        auto result = [dlg runModal];
        handleDone(result);
    }
}

}  // namespace uitk
