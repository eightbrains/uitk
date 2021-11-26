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

#ifndef UITK_MENU_H
#define UITK_MENU_H

#include "OSMenu.h"

#include <memory>
#include <string>

namespace uitk {

struct KeyEvent;
class MenuUITK;

class Menu
{
    friend class Menubar;
public:
    using ItemFound = OSMenu::ItemFound;

    static MenuId kInvalidId;

    Menu();
    virtual ~Menu();
    
    void clear();
    Menu* addItem(const std::string& text, MenuId id, const ShortcutKey& shortcut);
    /// Takes ownership of menu
    Menu* addMenu(const std::string& text, Menu *menu);
    Menu* addSeparator();

    Menu* insertItem(int index, const std::string& text, MenuId id, const ShortcutKey& shortcut);
    /// Takes ownership of menu
    Menu* insertMenu(int index, const std::string& text, Menu *menu);
    Menu* insertSeparator(int index);

    void removeItem(MenuId id);

    bool isSeparator(MenuId id) const;

    bool itemChecked(MenuId id) const;
    /// This does not return the this pointer to remind you that you should
    /// use Window::setOnMenuWillShow() to set an item's state.
    ItemFound setItemChecked(MenuId id, bool checked);

    bool itemEnabled(MenuId id) const;
    /// This does not return the this pointer to remind you that you should
    /// use Window::setOnMenuWillShow() to set an item's state.
    ItemFound setItemEnabled(MenuId id, bool enabled);

    /// Returns the text of the item with the requested index, or "" if the
    /// index is invalid.
    const std::string& itemText(MenuId id) const;
    ItemFound setItemText(MenuId id, const std::string& text);

    /// Activates the item if it exists in the menu tree and is enabled.
    /// This is mostly for internal use; if you need this you should
    /// probably call Application::instance().menubar().activateItemId() instead.
    ItemFound activateItem(MenuId id) const;

    /// This is for internal use
    MenuUITK* menuUitk() const;

public:
    enum class StandardItem {
        kAbout = 1,
        kQuit,
        kCopy, kCut, kPaste,
        kUndo, kRedo,
        kPreferences
    };

    /// Returns true if the key event is the shortcut key for the given item
    /// type (assuming a standard shortcut exists). You should
    /// not need to call this function if you are using the menus.
    static bool isShortcutFor(StandardItem item, const KeyEvent& e);

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace uitk
#endif // UITK_MENU_H
