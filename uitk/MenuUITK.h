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

#ifndef UITK_POPUP_MENU_H
#define UITK_POPUP_MENU_H

#include "OSMenu.h"
#include "Widget.h"

#include <functional>
#include <string>

namespace uitk {

class Menu;

class MenuUITK : public OSMenu
{
public:
    MenuUITK();
    ~MenuUITK();

    void clear() override;
    int size() const override;
    /// Adds item with the given string. When using native menus on Windows, an underscore
    /// marks the key navigation for the menu item; on all other platform underscores are
    /// removed.
    void addItem(const std::string& text, MenuId id, const ShortcutKey& shortcut) override;
    /// Adds an item with a callback function. This should only be used for popup menus.
    void addItem(const std::string& text, MenuId id, std::function<void()> onSelected);
    /// Takes ownership of menu.
    void addMenu(const std::string& text, Menu *menu) override;
    void addSeparator() override;

    /// Inserts item with the given string at the index. When using native menus on Windows,
    /// an underscore marks the key navigation for the menu item; on all other platform
    /// underscores are removed.
    void insertItem(int index, const std::string& text, MenuId id,
                    const ShortcutKey& shortcut) override;
    /// Inserts an item with a callback function at the index.
    /// This should only be used for popup menus.
    void insertItem(int index, const std::string& text, MenuId id, std::function<void()> onSelected);
    /// Takes ownership of menu.
    void insertMenu(int index, const std::string& text, Menu *menu) override;
    void insertSeparator(int index) override;

    /// Removes item at index and destroys all memory, including submenus if applicable.
    void removeItem(int index) override;
    /// Removes the submenu, but returns ownership to the caller.
    Menu* removeMenu(int index) override;

    MenuId itemIdAt(int index) const override;

    bool isSubmenuAt(int index) const override;
    bool isSeparatorAt(int index) const override;
    Menu* itemMenuAt(int index) const override;

    bool itemCheckedAt(int index) const override;
    void setItemCheckedAt(int index, bool checked) override;

    bool itemEnabledAt(int index) const override;
    void setItemEnabledAt(int index, bool enabled) override;

    std::string itemTextAt(int index) const override;
    void setItemTextAt(int index, const std::string& text) override;

    void removeItem(MenuId id);
    // Returns ownership of the menu (if it exists)
    Menu* removeMenu(const std::string& text);

    // Returns the menu associated with the text, otherwise nullptr. Retains ownership.
    Menu* itemMenu(const std::string& text) const;

    bool itemChecked(MenuId id) const;
    ItemFound setItemChecked(MenuId id, bool checked);

    bool itemEnabled(MenuId id) const;
    ItemFound setItemEnabled(MenuId id, bool enabled);

    /// Returns the text of the item with the requested index, or "" if the
    /// index is invalid.
    std::string itemText(MenuId id) const;
    ItemFound setItemText(MenuId id, const std::string& text);

    ItemFound activateItem(MenuId id, Window *activeWindow) const override;

    Size preferredSize(const LayoutContext& context) const;

    bool isShowing() const;
    void show(Window *w, const Point& upperLeftWindowCoord, MenuId id = OSMenu::kInvalidId,
              int extraWindowFlags = 0);
    void cancel();
    void cancelHierarchy(); /// Cancels the menu and any parent menus

    /// Sets callback function for when the menu closes, which will be called
    /// whether a menu item is selected or the menu is cancelled.
    void setOnClose(std::function<void()> onClose);

    /// Returns the popup's window. Note that the window may not exist unless
    /// the window is showing.
    Window* window();
    
    /// Draws the item with ID with the upper left at (0, 0).
    /// This is mostly internal, used by ComboBox.
    void drawItem(UIContext& context, const Rect& frame, MenuId id, Theme::WidgetState itemState);

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_POPUP_MENU_H
