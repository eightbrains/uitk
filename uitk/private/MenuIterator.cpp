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

#include "MenuIterator.h"

#include "../Menu.h"
#include "../MenuUITK.h"
#include "../OSMenu.h"

namespace uitk {

namespace {

struct MenuItemWrapper : public MenuItem
{
	MenuItemWrapper() : mMenu(nullptr), mIndex(-1) {}
	MenuItemWrapper(OSMenu* m, int i) : mMenu(m), mIndex(i) {}
	~MenuItemWrapper() {}

	MenuId id() const override { return mMenu->itemIdAt(mIndex); }
	std::string text() const override { return mMenu->itemTextAt(mIndex); }
	void setEnabled(bool enabled) override { mMenu->setItemEnabledAt(mIndex, enabled); }
	void setChecked(bool checked) override { mMenu->setItemCheckedAt(mIndex, checked); }
	void setText(const std::string& text) override { mMenu->setItemTextAt(mIndex, text); }

private:
	OSMenu* mMenu;
	int mIndex;
};

} // namespace

//-----------------------------------------------------------------------------
// We want to keep the definition of MenuItemWrapper private, but that forces
// us to return a MenuItem pointer, which means that we need to a keep the
// object around so that it can live past the call to menuItem().
struct MenuIterator::Impl
{
	MenuItemWrapper item;
};

MenuIterator::MenuIterator(Menu *menu)
	: mImpl(new Impl())
{
	push(menu);
}

MenuIterator::MenuIterator(OSMenu* osmenu)
{
	mStack.push_back(Iterator{ osmenu, 0, osmenu->size() });
}

MenuIterator::~MenuIterator()
{
}

void MenuIterator::next()
{
	mStack.back().index++;
	// Skip separators
	while (mStack.back().index < mStack.back().maxIndex
		   && mStack.back().menu->isSeparatorAt(mStack.back().index)) {
		mStack.back().index++;
	}
	// If we have finished with a menu, pop it and resume (unless this is the top-level)
	if (mStack.back().index >= mStack.back().maxIndex) {
		if (mStack.size() == 1) {
			return;
		}
		mStack.pop_back();
		next();
	// If this is a submenu, step into it
	} else if (mStack.back().menu->isSubmenuAt(mStack.back().index)) {
		push(mStack.back().menu->itemMenuAt(mStack.back().index));
		mStack.back().index = -1;
		next();  // in case first item is a separator (unlikely, craftsmanship consider things like that!)
	}
}

bool MenuIterator::done()
{
	return (mStack.size() == 1 && mStack[0].index >= mStack[0].maxIndex);
}

MenuItem& MenuIterator::menuItem()
{
	mImpl->item = MenuItemWrapper(mStack.back().menu, mStack.back().index);
	return mImpl->item;
}

void MenuIterator::push(Menu* menu)
{
	OSMenu* osmenu = menu->nativeMenu();
	if (!osmenu) {
		osmenu = menu->menuUitk();
	}
	mStack.push_back(Iterator{ osmenu, 0, osmenu->size() });
}

}  // namespace uitk
