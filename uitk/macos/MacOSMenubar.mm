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

#include "MacOSMenubar.h"

#include "../Application.h"
#include "../Menu.h"
#include "../Window.h"
#include "../private/Utils.h"
#include "MacOSMenu.h"

#import <Cocoa/Cocoa.h>

#include <map>

@interface MenubarDelegate : NSObject <NSMenuDelegate>
- (void)menuNeedsUpdate:(NSMenu *)menu;
@end

@implementation MenubarDelegate
- (void)menuNeedsUpdate:(NSMenu *)menu
{
    if (auto *w = uitk::Application::instance().activeWindow()) {
        w->onMenuWillShow();
    }
}
@end

//-----------------------------------------------------------------------------
namespace uitk {

// NSMenu only keeps a weak reference to the delegate, so we need to manage
// it ourselves, but since the delegate does not really do anything, just make
// it global.
static MenubarDelegate* gDelegate = [[MenubarDelegate alloc] init];

struct MacOSMenubar::Impl
{
    struct MenuInfo
    {
        Menu *menu;  // we do not own this
        std::string title;
    };

    std::unique_ptr<Menu> menubar;
    std::vector<MenuInfo> menuInfo;
    MenubarDelegate* delegate = nil;
    Menu *appMenu = nullptr; // menubar owns this
};

MacOSMenubar::MacOSMenubar()
    : mImpl(new Impl())
{
    mImpl->menubar = std::make_unique<Menu>();
    mImpl->delegate = [[MenubarDelegate alloc] init];

    mImpl->appMenu = newMenu("app"); // first menu is displayed with process name

    auto *menubar = dynamic_cast<MacOSMenu*>(mImpl->menubar->nativeMenu());
    assert(menubar);
    if (menubar) {
        NSApplication.sharedApplication.mainMenu = (__bridge NSMenu*)(menubar->nsmenu());
    }
}

MacOSMenubar::~MacOSMenubar()
{
    NSApplication.sharedApplication.mainMenu = nil;
    // Manually release so we can release the delegate after the menu(s) that are using it,
    // otherwise we would have a dangling pointer to the delegate in the NSMenu.delegate property,
    // which is weak.
    mImpl->menubar.reset();
    // Now that Menu (and its NSMenu which references the delegate) are gone, we can
    // delete the delegate.
    mImpl->delegate = nil;
}

Menu* MacOSMenubar::newMenu(const std::string& name)
{
    Menu *m = new Menu();
    addMenu(m, name);
    return m;
}

void MacOSMenubar::addMenu(Menu* menu, const std::string& name)
{
    // Since we are not using the NSMenuValidation (informal) protocol, we need
    // to use the NSMenu delegate to know when we need to query the menu item
    // status. It would be nice if we could set this on mImpl->menubar, but
    // unfortunately the delegate never gets called. This results in a call
    // to Window::onMenuWillShow() every time any menu is open, which is excessive
    // because onMenuWillShow() updates all the items, but that's the way it is.
    if (auto *macMenu = dynamic_cast<MacOSMenu*>(menu->nativeMenu())) {
        NSMenu* nsmenu = (__bridge NSMenu*)macMenu->nsmenu();
        nsmenu.delegate = mImpl->delegate;
    }

    // Note that it is important that the 'title' property of the menu is set.
    // At this point it is set to "", since the constructor does not know what the
    // name should be. We could dig out the NSMenu from 'menu' here, but addMenu()
    // will take care of setting the 'title' property for us.
    mImpl->menubar->addMenu(name, menu);

    mImpl->menuInfo.push_back({ menu, removeMenuItemMnemonics(name) });
}

Menu* MacOSMenubar::removeMenu(const std::string& name)
{
    auto nameNoMnemonics = removeMenuItemMnemonics(name);
    for (size_t i = 0;  i < mImpl->menuInfo.size();  ++i) {
        if (mImpl->menuInfo[i].title == nameNoMnemonics) {
            return mImpl->menubar->removeMenu(int(i));
        }
    }
    return nullptr;
}

Menu* MacOSMenubar::menu(const std::string& name) const
{
    for (auto &info : mImpl->menuInfo) {
        if (info.title == name) {
            return info.menu;
        }
    }
    return nullptr;
}

Menu* MacOSMenubar::macosApplicationMenu() const
{
    return mImpl->appMenu;
}

std::vector<Menu*> MacOSMenubar::menus() const
{
    std::vector<Menu*> mm;
    mm.reserve(mImpl->menuInfo.size());
    for (auto &info : mImpl->menuInfo) {
        mm.push_back(info.menu);
    }
    return mm;
}

void MacOSMenubar::activateItemId(MenuId itemId) const
{
    mImpl->menubar->activateItem(itemId);
}

}  // namespace uitk
