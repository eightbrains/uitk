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

#include "Win32Menubar.h"

#include "../Application.h"
#include "../Menu.h"
#include "../private/Utils.h"
#include "Win32Menu.h"
#include "Win32Utils.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace uitk {

struct Win32Menubar::Impl {
	struct Item
	{
		std::string title;
		std::string titleNoAmpersands;
		std::unique_ptr<Menu> menu;
	};
	std::vector<Item*> menus;
};

Win32Menubar::Win32Menubar()
	: mImpl(new Impl())
{
}

Win32Menubar::~Win32Menubar()
{
	for (auto it : mImpl->menus) {
		delete it;
	}
}

Menu* Win32Menubar::newMenu(const std::string& name)
{
	Menu *m = new Menu();
	addMenu(m, name);
	return m;
}

void Win32Menubar::addMenu(Menu* menu, const std::string& name)
{
	mImpl->menus.emplace_back(new Impl::Item{ name, removeMenuItemMnemonics(name), std::unique_ptr<Menu>(menu) });
}

Menu* Win32Menubar::removeMenu(const std::string& name)
{
	Menu *retval = nullptr;
	for (auto it = mImpl->menus.begin(); it != mImpl->menus.end(); ++it) {
		if ((*it)->title == name || (*it)->titleNoAmpersands == name) {
			retval = (*it)->menu.release();
			delete *it;
			mImpl->menus.erase(it);
			break;
		}
	}
	return retval;
}

Menu* Win32Menubar::menu(const std::string& name) const
{
	for (auto *item : mImpl->menus) {
		if (item->title == name || item->titleNoAmpersands == name) {
			return item->menu.get();
		}
	}
	return nullptr;
}

Menu* Win32Menubar::macosApplicationMenu() const { return nullptr; }

std::vector<Menu*> Win32Menubar::menus() const
{
	std::vector<Menu*> mm;
	mm.reserve(mImpl->menus.size());
	for (auto *item : mImpl->menus) {
		mm.push_back(item->menu.get());
	}
	return mm;
}

/*void Win32Menubar::setItemEnabled(MenuId itemId, bool enabled)
{
	for (auto& item : mImpl->menus) {
		if (item->menu->setItemEnabled(itemId, enabled) == OSMenu::ItemFound::kYes) {
			return;
		}
	}
}

void Win32Menubar::setItemChecked(MenuId itemId, bool checked)
{
	for (auto& item : mImpl->menus) {
		if (item->menu->setItemChecked(itemId, checked) == OSMenu::ItemFound::kYes) {
			return;
		}
	}
}
*/
void Win32Menubar::activateItemId(MenuId itemId) const
{
	for (auto& item : mImpl->menus) {
		if (item->menu->activateItem(itemId) == OSMenu::ItemFound::kYes) {
			return;
		}
	}
}

void* Win32Menubar::createNativeMenubar()
{
	HMENU hmenubar = CreateMenu();
	for (auto& item : mImpl->menus) {
		if (auto *win32menu = dynamic_cast<Win32Menu*>(item->menu->nativeMenu())) {
			AppendMenuW(hmenubar, MF_POPUP, (UINT_PTR)win32menu->createNativeMenu(),
						win32UnicodeFromUTF8(item->title).c_str());
		}
	}
	return hmenubar;
}

}  // namespace uitk
