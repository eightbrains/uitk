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

#include "Win32Menu.h"

#include "../Application.h"
#include "../Menu.h"
#include "../Window.h"
#include "Win32Window.h"
#include "Win32Utils.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef min

#include <algorithm>

namespace uitk {

namespace {

void updateMenubars()
{
	for (auto *w : Application::instance().windows()) {
		if (auto* w32 = dynamic_cast<Win32Window*>(w->nativeWindow())) {
			w32->setNeedsUpdateMenubar();
		}
	}
}

}  // namespace

struct Win32Menu::Impl
{
	struct MenuItem
	{
		MenuId id;
		std::string text;
		ShortcutKey shortcut;
		std::unique_ptr<Menu> submenu;
		bool checked = false;
		bool enabled = true;
	};

	std::vector<MenuItem*> items;

	enum class Action { kFind, kDelete };

	MenuItem* findMenuItem(MenuId id, Action action)
	{
		for (auto it = this->items.begin();  it != this->items.end();  ++it) {
			auto *item = *it;
			if (item->id == id) {
				MenuItem *mi = item;
				if (action == Action::kDelete) {
					this->items.erase(it);
					delete item;
					mi = nullptr;
				}
				return mi;
			} else if (item->submenu) {
				if (auto *subitem = ((Win32Menu*)item->submenu->nativeMenu())->mImpl->findMenuItem(id, action)) {
					return subitem;
				}
			}
		}
		return nullptr;
	}

	Menu* findMenu(const std::string& text, Action action)
	{
		for (auto it = this->items.begin(); it != this->items.end(); ++it) {
			auto* item = *it;
			if (item->text == text) {
				Menu *m;
				if (action == Action::kDelete) {
					m = item->submenu.release();
					this->items.erase(it);
					delete item;
				} else {
					m = item->submenu.get();
				}
				return m;
			} else if (item->submenu) {
				if (auto* subm = ((Win32Menu*)item->submenu->nativeMenu())->mImpl->findMenu(text, action)) {
					return subm;
				}
			}
		}
		return nullptr;
	}

	std::wstring win32UnicodeTextForItem(MenuItem *item)
	{
		// Windows cheaps out and makes us write our own accelerator key text.
		// (And register them, too)
		auto t = item->text;
		if (item->shortcut.key != Key::kNone) {
			t += "\t";  // Windows uses a tab to know there is accelerator text
			t += item->shortcut.displayText();
		}
		return win32UnicodeFromUTF8(t);
	}
};

Win32Menu::Win32Menu()
	: mImpl(new Impl())
{
}

Win32Menu::~Win32Menu()
{
	clear();  // so items are deleted
}

void Win32Menu::clear()
{
	for (auto* item : mImpl->items) {
		delete item;
	}
	mImpl->items.clear();
}

int Win32Menu::size() const { return int(mImpl->items.size()); }


void Win32Menu::addItem(const std::string& text, MenuId id, const ShortcutKey& shortcut)
{
	insertItem(int(mImpl->items.size()), text, id, shortcut);
}

void Win32Menu::addMenu(const std::string& text, Menu* menu)
{
	insertMenu(int(mImpl->items.size()), text, menu);
}

void Win32Menu::addSeparator()
{
	insertSeparator(int(mImpl->items.size()));
}

void Win32Menu::insertItem(int index, const std::string& text, MenuId id, const ShortcutKey& shortcut)
{
	index = std::min(index, int(mImpl->items.size()));
	mImpl->items.insert(mImpl->items.begin() + index, new Impl::MenuItem{ id, text, shortcut });
	updateMenubars();
	// Windows has keyboard accelerators, but it does not really offer us any benefit,
	// since they make us create an accelerator table and add each key, so we might
	// as well just use our existing accelerator.
	Application::instance().keyboardShortcuts().add(id, shortcut);
}

void Win32Menu::insertMenu(int index, const std::string& text, Menu* menu)
{
	index = std::min(index, int(mImpl->items.size()));
	mImpl->items.insert(mImpl->items.begin() + index,
						new Impl::MenuItem{ kInvalidId, text, ShortcutKey(), std::unique_ptr<Menu>(menu) });
	updateMenubars();
}

void Win32Menu::insertSeparator(int index)
{
	index = std::min(index, int(mImpl->items.size()));
	mImpl->items.insert(mImpl->items.begin() + index, new Impl::MenuItem{ kInvalidId, "", ShortcutKey() });
	updateMenubars();
}

void Win32Menu::removeItem(int index)
{
	if (index >= 0 && index <= int(mImpl->items.size())) {
		Application::instance().keyboardShortcuts().remove(mImpl->items[index]->id);
		delete mImpl->items[index];
		mImpl->items.erase(mImpl->items.begin() + index);
		updateMenubars();
	}
}

Menu* Win32Menu::removeMenu(int index)
{
	if (index >= 0 && index <= int(mImpl->items.size())) {
		Menu *m = mImpl->items[index]->submenu.release();
		mImpl->items.erase(mImpl->items.begin() + index);
		updateMenubars();
		return m;
	}
	return nullptr;
}

MenuId Win32Menu::itemIdAt(int index) const
{
	if (index >= 0 && index < int(mImpl->items.size())) {
		return mImpl->items[index]->id;
	}
	return kInvalidId;
}

Menu* Win32Menu::itemMenuAt(int index) const
{
	if (index >= 0 && index < int(mImpl->items.size())) {
		return mImpl->items[index]->submenu.get();
	}
	return nullptr;
}

bool Win32Menu::isSubmenuAt(int index) const
{
	if (index >= 0 && index < int(mImpl->items.size())) {
		return (mImpl->items[index]->submenu != nullptr);
	}
	return kInvalidId;
}

bool Win32Menu::isSeparatorAt(int index) const
{
	if (index >= 0 && index < int(mImpl->items.size())) {
		return (mImpl->items[index]->text.empty() && !mImpl->items[index]->submenu);
	}
	return false;
}

bool Win32Menu::itemCheckedAt(int index) const
{
	if (index >= 0 && index < int(mImpl->items.size())) {
		return mImpl->items[index]->checked;
	}
	return false;
}

void Win32Menu::setItemCheckedAt(int index, bool checked)
{
	if (index >= 0 && index < int(mImpl->items.size())) {
		mImpl->items[index]->checked = checked;

		HWND hwnd = (HWND)Application::instance().activeWindow()->nativeHandle();
		HMENU hmenu = GetMenu(hwnd);
		UINT id = (UINT)mImpl->items[index]->id;
		MENUITEMINFO info = { 0 };
		info.cbSize = sizeof(info);
		info.fMask = MIIM_STATE;  // only get/set state
		GetMenuItemInfo(hmenu, id, FALSE /* arg2 is id, not index */, &info);
		if (checked) {
			info.fState |= MFS_CHECKED;
		} else {
			info.fState &= ~MFS_CHECKED;  // MFS_UNCHECKED is 0x0
		}
		SetMenuItemInfo(hmenu, id, FALSE, &info);
	}
}

bool Win32Menu::itemEnabledAt(int index) const
{
	if (index >= 0 && index < int(mImpl->items.size())) {
		return mImpl->items[index]->enabled;
	}
	return false;
}

void Win32Menu::setItemEnabledAt(int index, bool enabled)
{
	if (index >= 0 && index < int(mImpl->items.size())) {
		mImpl->items[index]->enabled = enabled;

		HWND hwnd = (HWND)Application::instance().activeWindow()->nativeHandle();
		HMENU hmenu = GetMenu(hwnd);
		UINT id = (UINT)mImpl->items[index]->id;
		MENUITEMINFO info = { 0 };
		info.cbSize = sizeof(info);
		info.fMask = MIIM_STATE;  // only get/set state
		GetMenuItemInfo(hmenu, id, FALSE /* arg2 is id, not index */, &info);
		if (enabled) {
			info.fState &= ~MFS_DISABLED;  // MFS_ENABLED is 0x0
		} else {
			info.fState |= MFS_DISABLED;
		}
		SetMenuItemInfo(hmenu, id, FALSE, &info);
	}
}

std::string Win32Menu::itemTextAt(int index) const
{
	if (index >= 0 && index < int(mImpl->items.size())) {
		return mImpl->items[index]->text;
	}
	return "";
}

void Win32Menu::setItemTextAt(int index, const std::string& text)
{
	if (index >= 0 && index < int(mImpl->items.size())) {
		auto *item = mImpl->items[index];
		item->text = text;

		HWND hwnd = (HWND)Application::instance().activeWindow()->nativeHandle();
		HMENU hmenu = GetMenu(hwnd);
		UINT id = (UINT)item->id;
		MENUITEMINFOW info = { 0 };
		info.cbSize = sizeof(info);
		info.fMask = MIIM_TYPE | MIIM_DATA;  // only get/set item text
		info.fType = MFT_STRING;
		auto win32Text = mImpl->win32UnicodeTextForItem(item);  // must be assigned so that pointer from data() lives through the set call
		info.dwTypeData = win32Text.data();
		SetMenuItemInfoW(hmenu, id, FALSE /* arg2 is id, not index */, &info);
	}
}

OSMenu::ItemFound Win32Menu::activateItem(MenuId id, Window* activeWindow) const
{
	if (auto* item = mImpl->findMenuItem(id, Impl::Action::kFind)) {
		if (!item->enabled) {
			return ItemFound::kDisabled;
		}
		if (activeWindow) {
			activeWindow->onMenuActivated(id);
		}
		return ItemFound::kYes;
	}
	return ItemFound::kNo;
}

void* Win32Menu::createNativeMenu()
{
	HMENU hmenu = CreateMenu();
	for (auto* item : mImpl->items) {
		if (item->text.empty()) {
			AppendMenuW(hmenu, MF_SEPARATOR, 0, NULL);
		} else if (item->submenu) {
			if (auto* m = dynamic_cast<Win32Menu*>(item->submenu->nativeMenu())) {
				AppendMenuW(hmenu, MF_POPUP, (UINT_PTR)m->createNativeMenu(),
							mImpl->win32UnicodeTextForItem(item).c_str());
			} else {
				assert(false); // submenu of Win32Menu should also be a Win32Menu...
			}
		} else {
			AppendMenuW(hmenu, MF_STRING, item->id,
						mImpl->win32UnicodeTextForItem(item).c_str());
		}
	}
	return hmenu;
}

}  // namespace uitk
