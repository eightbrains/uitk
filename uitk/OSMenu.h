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

#ifndef UITK_OS_MENU_H
#define UITK_OS_MENU_H

#include <functional>
#include <string>

#include "Global.h"
// Include ShortcutKey.h so .cpp files everywhere do not need to include this;
// it is a little unexpected to include something and then find you need to
// track down all that include file's references.
#include "ShortcutKey.h"

namespace uitk {

class Menu;
class Window;

class OSMenu
{
public:
    static constexpr MenuId kInvalidId = 0xffff;

    enum class ItemFound {
        kNo = 0,
        kYes = 1,
        kDisabled = 2  /// item was found, but not activated because it is disabled;
                       /// only used by activateItem()
    };

    virtual ~OSMenu() {}

    virtual void clear() = 0;
    /// Returns the number of items in this menu; does not include items
    /// in submenus.
    virtual int size() const = 0;
    virtual void addItem(const std::string& text, MenuId id, const ShortcutKey& shortcut,
                         std::function<void(Window*)> onClicked = nullptr) = 0;
    /// Takes ownership of menu
    virtual void addMenu(const std::string& text, Menu *menu) = 0;
    virtual void addSeparator() = 0;

    virtual void insertItem(int index, const std::string& text, MenuId id, const ShortcutKey& shortcut,
                            std::function<void(Window*)> onClicked = nullptr) = 0;
    /// Takes ownership of menu
    virtual void insertMenu(int index, const std::string& text, Menu *menu) = 0;
    virtual void insertSeparator(int index) = 0;

    // Destroys the item, including any submenu it may have
    virtual void removeItem(int index) = 0;
    // Does NOT destroy the menu, returns ownership to caller
    virtual Menu* removeMenu(int index) = 0;

    virtual MenuId itemIdAt(int index) const = 0;
    virtual Menu* itemMenuAt(int index) const = 0;

    virtual bool isSubmenuAt(int index) const = 0;
    virtual bool isSeparatorAt(int index) const = 0;

    virtual bool itemCheckedAt(int index) const = 0;
    virtual void setItemCheckedAt(int index, bool checked) = 0;

    virtual bool itemEnabledAt(int index) const = 0;
    virtual void setItemEnabledAt(int index, bool enabled) = 0;

    /// Returns the text of the item with the requested index, or "" if the
    /// index is invalid.
    /// Design note: this cannot return a const reference since we may need to convert
    ///              from the OS text representation.
    virtual std::string itemTextAt(int index) const = 0;
    virtual void setItemTextAt(int index, const std::string& text) = 0;
/*
    virtual void removeItem(MenuId id) = 0;
    // Returns ownership of the menu (if it exists)
    virtual Menu* removeMenu(const std::string& text) = 0;

    // Returns the menu associated with the menu
    virtual Menu* menu(const std::string& text) const = 0;

    virtual bool itemChecked(MenuId id) const = 0;
    virtual ItemFound setItemChecked(MenuId id, bool checked) = 0;

    virtual bool itemEnabled(MenuId id) const = 0;
    virtual ItemFound setItemEnabled(MenuId id, bool enabled) = 0;

    virtual std::string itemText(MenuId id) const = 0;
    virtual ItemFound setItemText(MenuId id, const std::string& text) = 0;
*/
    // Activates the item if existing, and returns ItemFound::kYes if the item exists,
    // otherwise ItemFound::kNo. The item may not actually have been activated even
    // if it exists, if it was disabled, but this allows the caller to stop iterating
    // over menus.
    virtual ItemFound activateItem(MenuId id, Window *activeWindow) const = 0;

    //virtual bool isShowing() const = 0;
    //virtual void show(Window *w, const Point& upperLeft, MenuId id = kInvalidId) = 0;
    //virtual void cancel() = 0;

/*    // Cannot really use STL-style operators in a class hierarchy.
    // What does it mean to compare two iterators, which might be from
    // different derived classes? (This should not happen for us, but
    // the problem is still valid.) There's no good way of comparing a
    // Cat iterator with Dog iterator even though both inherit from Animal.
    // Java's iterator is much more suited for object hierarchies; since
    // we do not know which implementation of the menu is being used, we
    // cannot use templates, we need to use the iterator itself.
    class Iterator
    {
    public:
        virtual ~Iterator() {}
        virtual void next() = 0;
        virtual bool done() = 0;
        virtual MenuItem* menuItem() = 0;
    };
    virtual Iterator iterator() = 0;
    */
};

}  // namespace uitk
#endif // UITK_OS_MENU_H
