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

#include "Menu.h"

#include "Application.h"
#include "Events.h"
#include "MenuUITK.h"

#if defined(__APPLE__)
#include "macos/MacOSMenu.h"
#elif defined(_WIN32) || defined(_WIN64)
#include "win32/Win32Menu.h"
#endif

#include <unordered_map>

namespace uitk {

MenuId Menu::kInvalidId = OSMenu::kInvalidId;

struct Menu::Impl
{
    std::shared_ptr<OSMenu> menu;  // use this for most things
    std::shared_ptr<MenuUITK> menuUitk;  // will be null if using platform menus
};

Menu::Menu()
    : mImpl(new Menu::Impl())
{
    if (Application::instance().supportsNativeMenus()) {
#if defined(__APPLE__)
        mImpl->menu = std::make_shared<MacOSMenu>();
#elif defined(_WIN32) || defined(_WIN64)
        mImpl->menu = std::make_shared<Win32Menu>();
#endif
    } else {
        mImpl->menuUitk = std::make_shared<MenuUITK>();
        mImpl->menu = mImpl->menuUitk;
    }
}

Menu::~Menu()
{
}

OSMenu* Menu::nativeMenu() const
{
    if (mImpl->menuUitk) {
        return nullptr;  // mImpl->menu always exists, but if menuUitk exists, menu is not a native menu
    } else {
        return mImpl->menu.get();
    }
}

MenuUITK* Menu::menuUitk() const { return mImpl->menuUitk.get(); }

int Menu::size() const { return mImpl->menu->size(); }

void Menu::clear()
{
    mImpl->menu->clear();
}

Menu* Menu::addItem(const std::string& text, MenuId id, const ShortcutKey& shortcut)
{
    mImpl->menu->addItem(text, id, shortcut);
    return this;
}

Menu* Menu::addMenu(const std::string& text, Menu *menu)
{
    mImpl->menu->addMenu(text, menu);
    return this;
}

//Menu* Menu::addItem(MenuItem *item, int id, std::function<void()> onItem)

Menu* Menu::addSeparator()
{
    mImpl->menu->addSeparator();
    return this;
}

Menu* Menu::insertItem(int index, const std::string& text, MenuId id, const ShortcutKey& shortcut)
{
    mImpl->menu->insertItem(index, text, id, shortcut);
    return this;
}

Menu* Menu::insertMenu(int index, const std::string& text, Menu *menu)
{
    mImpl->menu->insertMenu(index, text, menu);
    return this;
}

//Menu* insertItem(int index, MenuItem *item, MenuId id, std::function<void()> onItem)

Menu* Menu::insertSeparator(int index)
{
    mImpl->menu->insertSeparator(index);
    return this;
}

void Menu::removeItem(int index)
{
    mImpl->menu->removeItem(index);
}

Menu* Menu::removeMenu(int index)
{
    return mImpl->menu->removeMenu(index);
}

MenuId Menu::menuId(int index) const
{
    return mImpl->menu->itemIdAt(index);
}

bool Menu::isSeparator(int index) const
{
    return mImpl->menu->isSeparatorAt(index);
}

Menu::ItemFound Menu::activateItem(MenuId id) const
{
    auto *win = Application::instance().activeWindow();
    return mImpl->menu->activateItem(id, win);
}

} // namespace uitk
