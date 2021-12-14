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

/// This is passed to the callback given to Window::setOnMenuItemNeedsUpdate()
/// Design notes: ideally this would be Menu::Item, but it is not possible to
/// forward-declare inner classes. We prefer the slight awkwardness of an
/// individual class (less discoverability in the documentation, slight
/// namespace pollution) to including Menu.h efectively everywhere just to so
/// that things like Window.h can get the defintion of Menu::Item.
class MenuItem
{
public:
    virtual ~MenuItem() {}

    virtual MenuId id() const = 0;
    virtual std::string text() const = 0;

    virtual void setEnabled(bool enabled) = 0;
    virtual void setChecked(bool checked) = 0;
    virtual void setText(const std::string& text) = 0;
};

/// This class represents a menu.
/// - Menu items that are selected will trigger the onMenuActivated callback
/// on Window; use Window::setOnMenuActivated() FOR EACH WINDOW to set a
/// callback that for each menu items. This callback is also triggered if
/// the menu item's shortcut key is pressed; the shortcut is exactly like
/// clicking through the menus.
/// - To set an item dis/en-abled or (un)checked, set Window's
/// onMenuItemNeedsUpdate callback with Window::setOnMenuItemNeedsUpdate().
/// Like macOS, but unlike Windows and Linux, a menu is the same for all
/// (normal) windows. This means that the menu cannot be used to store state
/// (checked, unchecked), since different windows may have different states,
/// but there is only one menu. To prevent people from going down that road,
/// there is no way to get or set a menu item's enabled and checked states
/// from the menu.
/// - Put simply, the ordering and type (item, separator, submenu) are
/// part of the <i>structure</i> of the menu and can be changed from
/// a Menu pointer. Enabled, checked, and text are part of the item's
/// <i>state</i> and can only be changed from the onMenuItemNeedsUpdate
/// callback.
///
/// Design notes:
/// Q: Why not register a callback with each menu item instead of this
///    onMenuActivated callback?
/// A: This gets inconvenient in the case where there are multiple windows:
///    how does the callback know what object it should be operating on?
///    The best it can know is the Window, but then it will need to look
///    up the object from a Window -> object mapping.
/// Q: Why not use something like QAction, then?
/// A: We dislike spooky action at a distance: it is hard to debug, and it
///    it is hard to reason about at the destination code (when is this
///    code called?). You would still need to register a different Action
///    per Window anyway.
/// Q: Why enforce macOS' global menubar on all the other operating systems?
/// A: Support needs to be there for macOS, and it is very rare that two
///    main windows of an application need different menus. For that case,
///    you can override Window::onMenuWillShow() and change out the menus.
/// Q: Having to case out in the onMenuItemNeedsUpdate seems kind of clunky.
/// A: The original design was to call e.g. Menu::setItemChecked(itemId) as
///    necessary in an onMenuWillShow() callback. However, that involves a
///    linear search through the menu hierarchy for each call to
///    setItemChecked() and setItemEnabled(), which would be O(n^2) in the
///    worst case. Not great if your menus include a list of 100 fonts,
///    or 195 countries, or different language/encoding settings (such as
///    is in a web browser), and the natural approach for, say a list of
///    fonts, would be to iterate over all the items and set unchecked
///    unless that font is current; this would be an instant O(100^2).
///    The menu could keep a id -> item mapping, but updating that is error
///    prone and clunky itself. Cocoa/NextStep (macOS) has been doing it
///    this way for decades, and it at least is a simple and understandable
///    approach. It also is simple to explain what goes on underneath--the
///    callback gets called for each menu item before a menu is shown--which
///    means it is easy to for people to understand.
class Menu
{
    friend class Menubar;
public:
    using ItemFound = OSMenu::ItemFound;

    static MenuId kInvalidId;

    Menu();
    virtual ~Menu();
    
    void clear();
    /// Adds item with the given string. When using native menus on Windows, an ampersand
    /// marks the key navigation for the menu item; on all other platform ampersands are
    /// removed.
    Menu* addItem(const std::string& text, MenuId id, const ShortcutKey& shortcut);
    /// Takes ownership of menu
    Menu* addMenu(const std::string& text, Menu *menu);
    Menu* addSeparator();

    /// Inserts item with the given string at the index. When using native menus on Windows,
    /// an ampersand marks the key navigation for the menu item; on all other platform
    /// ampersands are removed.
    Menu* insertItem(int index, const std::string& text, MenuId id, const ShortcutKey& shortcut);
    /// Takes ownership of menu
    Menu* insertMenu(int index, const std::string& text, Menu *menu);
    Menu* insertSeparator(int index);

    /// Removes and destroys the item at index, including any submenus,
    /// if applicable.
    void removeItem(int index);

    /// Removes the submenu and returns ownership to the caller.
    Menu* removeMenu(int index);

    bool isSeparator(int index) const;

    /// Activates the item if it exists in the menu tree and is enabled.
    /// This is mostly for internal use; if you need this you should
    /// probably call Application::instance().menubar().activateItemId() instead.
    ItemFound activateItem(MenuId id) const;

    /// This is for internal use
    MenuUITK* menuUitk() const;
    /// This is for internal use
    OSMenu* nativeMenu() const;

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
