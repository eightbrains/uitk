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

#include "MacOSMenu.h"

#include "../Application.h"
#include "../Menu.h"
#include "../Window.h"

#import <Cocoa/Cocoa.h>

#include <map>

//-----------------------------------------------------------------------------
// Ordinarily a menu item gets sent to the appropriate NSObject (the window
// controller, I believe), but we are not doing that, so we need our own
// object that routes to the active window.
@interface MenuActivatedReceiver : NSObject
- (void)menuActivated:(NSMenuItem*)sender;
@end

@implementation MenuActivatedReceiver
- (void)menuActivated:(NSMenuItem*)sender
{
    if (auto *w = uitk::Application::instance().activeWindow()) {
        w->onMenuActivated(uitk::MenuId(sender.tag));
    }
}
@end

//-----------------------------------------------------------------------------
namespace uitk {

namespace {

NSString* nsstringWithoutUnderscores(const std::string& s)
{
    auto noUnderscores = s;
    auto idx = noUnderscores.find("_");
    while (idx != std::string::npos) {
        noUnderscores = noUnderscores.replace(idx, 1, "");
        idx = noUnderscores.find("_");
    }
    return [NSString stringWithUTF8String:noUnderscores.c_str()];
}

static MenuActivatedReceiver* gMenuReceiver = [[MenuActivatedReceiver alloc] init];

NSString* nsstringFromChar(unichar c)
{
    return [NSString stringWithCharacters:&c length:1];
}

NSMenuItem* createMenuItem(const std::string& title, MenuId id, const ShortcutKey& shortcut)
{
    NSMenuItem* item = [[NSMenuItem alloc]
                        initWithTitle:nsstringWithoutUnderscores(title)
                        action:nil
                        keyEquivalent:@""];  // docs say @"" for no shortcut, NOT nil
    item.tag = id;
    item.enabled = YES;
    item.target = gMenuReceiver;
    item.action = @selector(menuActivated:);
    if (shortcut.key != Key::kNone) {
        int modmask = 0;
        if (shortcut.modifier & int(KeyModifier::kCtrl)) {
            modmask |= NSEventModifierFlagCommand;
        }
        if (shortcut.modifier & int(KeyModifier::kAlt)) {
            modmask |= NSEventModifierFlagOption;
        }
        if (shortcut.modifier & int(KeyModifier::kMeta)) {
            modmask |= NSEventModifierFlagControl;
        }
        if (shortcut.modifier & int(KeyModifier::kShift)) {
            modmask |= NSEventModifierFlagShift;
        }

        NSString* keyEquiv = @"";
        switch (shortcut.key) {
            // These are not valid shortcut keys
            case Key::kNone:
            case Key::kUnknown:
            case Key::kShift:
            case Key::kCtrl:
            case Key::kAlt:
            case Key::kMeta:
            case Key::kCapsLock:
            case Key::kNumLock:
            case Key::kEscape:
                keyEquiv = @"";
                break;
            // These are valid shortcut keys
            case Key::kBackspace:
                keyEquiv = nsstringFromChar(NSBackspaceCharacter); break; // docs specifically specify this
            case Key::kTab:         keyEquiv = @"\t"; break;
            case Key::kEnter:       keyEquiv = nsstringFromChar(NSEnterCharacter); break;
            case Key::kReturn:      keyEquiv = nsstringFromChar(NSReturnTextMovement); break;
            case Key::kSpace:       keyEquiv = @" "; break;
            case Key::kNumMultiply: keyEquiv = @"*"; break;
            case Key::kNumPlus:     keyEquiv = @"+"; break;
            case Key::kNumComma:    keyEquiv = @","; break;
            case Key::kNumMinus:    keyEquiv = @"-"; break;
            case Key::kNumPeriod:   keyEquiv = @"."; break;
            case Key::kNumSlash:    keyEquiv = @"/"; break;
            case Key::k0:           keyEquiv = @"0"; break;
            case Key::k1:           keyEquiv = @"1"; break;
            case Key::k2:           keyEquiv = @"2"; break;
            case Key::k3:           keyEquiv = @"3"; break;
            case Key::k4:           keyEquiv = @"4"; break;
            case Key::k5:           keyEquiv = @"5"; break;
            case Key::k6:           keyEquiv = @"6"; break;
            case Key::k7:           keyEquiv = @"7"; break;
            case Key::k8:           keyEquiv = @"8"; break;
            case Key::k9:           keyEquiv = @"9"; break;
            case Key::kA:           keyEquiv = @"a"; break;
            case Key::kB:           keyEquiv = @"b"; break;
            case Key::kC:           keyEquiv = @"c"; break;
            case Key::kD:           keyEquiv = @"d"; break;
            case Key::kE:           keyEquiv = @"e"; break;
            case Key::kF:           keyEquiv = @"f"; break;
            case Key::kG:           keyEquiv = @"g"; break;
            case Key::kH:           keyEquiv = @"h"; break;
            case Key::kI:           keyEquiv = @"i"; break;
            case Key::kJ:           keyEquiv = @"j"; break;
            case Key::kK:           keyEquiv = @"k"; break;
            case Key::kL:           keyEquiv = @"l"; break;
            case Key::kM:           keyEquiv = @"m"; break;
            case Key::kN:           keyEquiv = @"n"; break;
            case Key::kO:           keyEquiv = @"o"; break;
            case Key::kP:           keyEquiv = @"p"; break;
            case Key::kQ:           keyEquiv = @"q"; break;
            case Key::kR:           keyEquiv = @"r"; break;
            case Key::kS:           keyEquiv = @"s"; break;
            case Key::kT:           keyEquiv = @"t"; break;
            case Key::kU:           keyEquiv = @"u"; break;
            case Key::kV:           keyEquiv = @"v"; break;
            case Key::kW:           keyEquiv = @"w"; break;
            case Key::kX:           keyEquiv = @"x"; break;
            case Key::kY:           keyEquiv = @"y"; break;
            case Key::kZ:           keyEquiv = @"z"; break;
            case Key::kDelete:
                keyEquiv = nsstringFromChar(NSDeleteCharacter); break; // docs specifically specify this
            case Key::kInsert:      keyEquiv = nsstringFromChar(NSInsertFunctionKey); break;
            case Key::kLeft:        keyEquiv = nsstringFromChar(NSLeftArrowFunctionKey); break;
            case Key::kRight:       keyEquiv = nsstringFromChar(NSRightArrowFunctionKey); break;
            case Key::kUp:          keyEquiv = nsstringFromChar(NSUpArrowFunctionKey); break;
            case Key::kDown:        keyEquiv = nsstringFromChar(NSDownArrowFunctionKey); break;
            case Key::kHome:        keyEquiv = nsstringFromChar(NSHomeFunctionKey); break;
            case Key::kEnd:         keyEquiv = nsstringFromChar(NSEndFunctionKey); break;
            case Key::kPageUp:      keyEquiv = nsstringFromChar(NSPageUpFunctionKey); break;
            case Key::kPageDown:    keyEquiv = nsstringFromChar(NSPageDownFunctionKey); break;
            case Key::kF1:          keyEquiv = nsstringFromChar(NSF1FunctionKey); break;
            case Key::kF2:          keyEquiv = nsstringFromChar(NSF2FunctionKey); break;
            case Key::kF3:          keyEquiv = nsstringFromChar(NSF3FunctionKey); break;
            case Key::kF4:          keyEquiv = nsstringFromChar(NSF4FunctionKey); break;
            case Key::kF5:          keyEquiv = nsstringFromChar(NSF5FunctionKey); break;
            case Key::kF6:          keyEquiv = nsstringFromChar(NSF6FunctionKey); break;
            case Key::kF7:          keyEquiv = nsstringFromChar(NSF7FunctionKey); break;
            case Key::kF8:          keyEquiv = nsstringFromChar(NSF8FunctionKey); break;
            case Key::kF9:          keyEquiv = nsstringFromChar(NSF9FunctionKey); break;
            case Key::kF10:         keyEquiv = nsstringFromChar(NSF10FunctionKey); break;
            case Key::kF11:         keyEquiv = nsstringFromChar(NSF11FunctionKey); break;
            case Key::kF12:         keyEquiv = nsstringFromChar(NSF12FunctionKey); break;
            case Key::kPrintScreen: keyEquiv = nsstringFromChar(NSPrintScreenFunctionKey); break;
        }
        item.keyEquivalent = keyEquiv;
        item.keyEquivalentModifierMask = modmask;
    }
    return item;
}

} // namespace

struct MacOSMenu::Impl
{
    NSMenu* menu;
    // Since the OSMenu interface takes ownership of the submenu pointers we need
    // to keep them somewhere. Note that the text key does NOT contain any underscores.
    std::map<std::string, std::unique_ptr<Menu>> ownedSubmenus;

    NSMenuItem* findMenuItem(MenuId id)
    {
        return findMenuItemInNSMenu(id, this->menu);
    }

    NSMenuItem* findMenuItemInNSMenu(MenuId id, NSMenu *nsmenu)
    {
        NSMenuItem* it = [nsmenu itemWithTag:id];
        if (it) {
            return it;
        }
        for (it in nsmenu.itemArray) {
            if (it.submenu != nil) {
                if (NSMenuItem* subIt = findMenuItemInNSMenu(id, it.submenu)) {
                    return subIt;
                }
            }
        }
        return nil;
    }
};

MacOSMenu::MacOSMenu()
    : mImpl(new Impl())
{
    mImpl->menu = [[NSMenu alloc] initWithTitle:@""];
    // We do not use the NSMenuValidation (informal) protocol; we use the NSMenu
    // delegate on menubar's menu to achieve the same effect.
    mImpl->menu.autoenablesItems = NO;
}

MacOSMenu::~MacOSMenu()
{
    mImpl->menu = nil;  // release
}

void* MacOSMenu::nsmenu() { return (__bridge void*)mImpl->menu; }

void MacOSMenu::clear()
{
    [mImpl->menu removeAllItems];
}

void MacOSMenu::addItem(const std::string& text, MenuId id, const ShortcutKey& shortcut)
{
    [mImpl->menu addItem:createMenuItem(text, id, shortcut)];
}

void MacOSMenu::addMenu(const std::string& text, Menu *menu)
{
    insertMenu(mImpl->menu.itemArray.count, text, menu);
}

void MacOSMenu::addSeparator()
{
    [mImpl->menu addItem:[NSMenuItem separatorItem]];
}

void MacOSMenu::insertItem(int index, const std::string& text, MenuId id, const ShortcutKey& shortcut)
{
    [mImpl->menu insertItem:createMenuItem(text, id, shortcut) atIndex:index];
}

void MacOSMenu::insertMenu(int index, const std::string& text, Menu *menu)
{
    if (!menu) {
        return;
    }

    assert(menu->nativeMenu());
    NSString* title = nsstringWithoutUnderscores(text);
    mImpl->ownedSubmenus[title.UTF8String] = std::move(std::unique_ptr<Menu>(menu));

    NSMenuItem* item = [[NSMenuItem alloc]
                        initWithTitle:title action:nil keyEquivalent:@""];
    item.submenu = ((MacOSMenu*)menu->nativeMenu())->mImpl->menu;
    // If this is getting inserted into the menubar's menu, the title is what
    // gets displayed (and if the title is blank, the menu just gets the margins
    // so it is hard to tell that there is actually a menu there).
    item.submenu.title = title;
    [mImpl->menu insertItem:item atIndex:index];
}

void MacOSMenu::insertSeparator(int index)
{
    [mImpl->menu insertItem:[NSMenuItem separatorItem] atIndex:index];
}

void MacOSMenu::removeItem(MenuId id)
{
    NSMenuItem* it = mImpl->findMenuItem(id);
    if (it != nil) {
        [it.menu removeItem:it];
    }
}

Menu* MacOSMenu::removeMenu(const std::string& text)
{
    NSString* title = nsstringWithoutUnderscores(text);
    for (NSMenuItem *it in mImpl->menu.itemArray) {
        if (it.submenu != nil && [it.title isEqualToString:title]) {
            [mImpl->menu removeItem:it];
            auto it = mImpl->ownedSubmenus.find(text);
            if (it != mImpl->ownedSubmenus.end()) {
                Menu *menu = it->second.release();
                mImpl->ownedSubmenus.erase(it);
            } else {
                return nullptr;
            }
        }
    }
    return nullptr;
}

Menu* MacOSMenu::menu(const std::string& text) const
{
    auto it = mImpl->ownedSubmenus.find(text);
    if (it != mImpl->ownedSubmenus.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool MacOSMenu::isSeparator(MenuId id) const
{
    // Note that sending a message to a nil object is okay, and returns 0
    return (mImpl->findMenuItem(id).isSeparatorItem == YES);
}

bool MacOSMenu::itemChecked(MenuId id) const
{
    return (mImpl->findMenuItem(id).state == NSControlStateValueOn);
}

OSMenu::ItemFound MacOSMenu::setItemChecked(MenuId id, bool checked)
{
    NSMenuItem* it = mImpl->findMenuItem(id);
    it.state = (checked ? NSControlStateValueOn : NSControlStateValueOff);
    return (it != nil ? ItemFound::kYes : ItemFound::kNo);
}

bool MacOSMenu::itemEnabled(MenuId id) const
{
    return (mImpl->findMenuItem(id).enabled == YES);
}

OSMenu::ItemFound MacOSMenu::setItemEnabled(MenuId id, bool enabled)
{
    NSMenuItem* it = mImpl->findMenuItem(id);
    it.enabled = (enabled ? YES : NO);
    return (it != nil ? ItemFound::kYes : ItemFound::kNo);
}

std::string MacOSMenu::itemText(MenuId id) const
{
    return mImpl->findMenuItem(id).title.UTF8String;
}

OSMenu::ItemFound MacOSMenu::setItemText(MenuId id, const std::string& text)
{
    NSMenuItem* it = mImpl->findMenuItem(id);
    it.title = nsstringWithoutUnderscores(text);
    return (it != nil ? ItemFound::kYes : ItemFound::kNo);
}

OSMenu::ItemFound MacOSMenu::activateItem(MenuId id, Window *activeWindow) const
{
    NSMenuItem* it = mImpl->findMenuItem(id);
    if (it != nil) {
        auto idx = [it.menu indexOfItemWithTag:id];
        if (idx >= 0) {
            [it.menu performActionForItemAtIndex:idx];
        }
    }
    return (it != nil ? ItemFound::kYes : ItemFound::kNo);
}

}  // namespace uitk
