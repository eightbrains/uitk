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

#include <unordered_map>

namespace uitk {

namespace {

struct Item
{
    std::string title;
    int shortcutKeymods;
    Key shortcutKey;
};

struct StandardItemInfo
{
    std::unordered_map<Menu::StandardItem, Item> item2item;


    void add(Menu::StandardItem item, const std::string& title, int keymods = 0, Key key = Key::kNone)
    {
        item2item[item] = Item{title, keymods, key};
    }

    StandardItemInfo()
    {
        add(Menu::StandardItem::kCopy, "Copy", KeyModifier::kCtrl, Key::kC);
        add(Menu::StandardItem::kCut, "Cut", KeyModifier::kCtrl, Key::kX);
        add(Menu::StandardItem::kPaste, "Paste", KeyModifier::kCtrl, Key::kV);
        add(Menu::StandardItem::kUndo, "Undo", KeyModifier::kCtrl, Key::kZ);
        add(Menu::StandardItem::kRedo, "Redo", KeyModifier::kCtrl | KeyModifier::kShift, Key::kZ);
        add(Menu::StandardItem::kAbout, "About...");
        add(Menu::StandardItem::kPreferences, "Preferences...");
#if defined(__APPLE__)
        add(Menu::StandardItem::kQuit, "Quit", KeyModifier::kCtrl, Key::kQ);
#elif defined(_WIN32) || defined(_WIN64)  // _WIN32 covers everything except 64-bit ARM
        add(Menu::StandardItem::kQuit, "Exit", KeyModifier::kCtrl, Key::kQ);
#else
        add(Menu::StandardItem::kQuit, "Quit", KeyModifier::kCtrl, Key::kQ);
#endif
    }
};
StandardItemInfo gStandardItems;

} // namespace

//-----------------------------------------------------------------------------

bool Menu::isShortcutFor(StandardItem item, const KeyEvent& e)
{
    auto info = gStandardItems.item2item[item];
    return (e.keymods == info.shortcutKeymods && e.key == info.shortcutKey);
}

MenuId Menu::kInvalidId = OSMenu::kInvalidId;

struct Menu::Impl
{
    std::shared_ptr<OSMenu> menu;  // use this for most things
    std::shared_ptr<MenuUITK> menuUitk;  // will be null if using platform menus
};

Menu::Menu()
    : mImpl(new Menu::Impl())
{
    mImpl->menuUitk = std::make_shared<MenuUITK>();
    mImpl->menu = mImpl->menuUitk;
}

Menu::~Menu()
{
}

MenuUITK* Menu::menuUitk() const { return mImpl->menuUitk.get(); }

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

Menu* Menu::insertSeparator(MenuId index)
{
    mImpl->menu->insertSeparator(index);
    return this;
}

void Menu::removeItem(MenuId id)
{
    mImpl->menu->removeItem(id);
}

bool Menu::isSeparator(MenuId id) const
{
    return mImpl->menu->isSeparator(id);
}

bool Menu::itemChecked(MenuId id) const
{
    return mImpl->menu->itemChecked(id);
}

Menu::ItemFound Menu::setItemChecked(MenuId id, bool checked)
{
    return mImpl->menu->setItemChecked(id, checked);
}

bool Menu::itemEnabled(MenuId id) const
{
    return mImpl->menu->itemEnabled(id);
}

Menu::ItemFound Menu::setItemEnabled(MenuId id, bool enabled)
{
    return mImpl->menu->setItemEnabled(id, enabled);
}

const std::string& Menu::itemText(MenuId id) const
{
    return mImpl->menu->itemText(id);
}

Menu::ItemFound Menu::setItemText(MenuId id, const std::string& text)
{
    return mImpl->menu->setItemText(id, text);
}

Menu::ItemFound Menu::activateItem(MenuId id) const
{
    auto *win = Application::instance().activeWindow();
    return mImpl->menu->activateItem(id, win);
}

} // namespace uitk
