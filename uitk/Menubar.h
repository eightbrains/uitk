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

#ifndef UITK_MENUBAR_H
#define UITK_MENUBAR_H

#include "OSMenu.h"

#include <memory>
#include <string>

namespace uitk {

class Menu;
class Widget;

class Menubar
{
    friend class Application;
    friend class Window;
public:
    Menubar(const Menubar&) = delete;
    ~Menubar();

    /// Creates a new Menu and adds to the menubar. Retains ownership.
    Menu* newMenu(const std::string& name);

    /// Adds a Menu to the menubar; takes ownership. When using native menus
    /// on Windows, an underscore will be used for key navigation; on all
    /// other platforms underscores will be removed.
    void addMenu(Menu* menu, const std::string& name);

    /// Removes the first Menu that matches the name from the menubar.
    /// Gives ownership to the caller, or returns nullptr if no menu was found.
    Menu* removeMenu(const std::string& name);

    /// Returns the first Menu that matches the name from the menubar,
    /// or nullptr if no matching menu was found. Ownership of the menu remains
    /// with the menubar.
    Menu* menu(const std::string& name) const;

    /// Sets the item with the given ID enabled (or disabled). This will
    /// search through the entire menu tree to find the item. If the item is
    /// not found the request will be ignored.
    void setItemEnabled(MenuId itemId, bool enabled);

    /// Sets the item with the given ID checked (or unchecked). This will
    /// search through the entire menu tree to find the item. If the item is
    /// not found the request will be ignored.
    void setItemChecked(MenuId itemId, bool checked);

    /// This is the programmatic way of clicking on a menu item. If the itemId
    /// exists in one of the menus its callback function will be called the
    /// same as if the user navigated through the menus. In particular,
    /// if the item is disabled, nothing will happen.
    void activateItemId(MenuId itemId) const;

    /// Returns true if the menus are using the native platform menus,
    /// false otherwise. The default is true for platforms that have native
    /// menus. If false, the menubar and menus will be drawn using UITK
    /// code.
    bool isNative() const;

    /// Sets whether to the native platform menus. THIS MUST BE CALLED
    /// BEFORE THE FIRST MENU IS ADDED. If false, the menubar and menus
    /// will be drawn using UITK code. The default is true for platforms
    /// that have native menus. If the platform does not have native
    /// menus (e.g. X11, WebAssembly) the argument will be ignored.
    /// This is mostly useful for testing and this should not be called
    /// in production code unless absolutely necessary, as users (especially
    /// macOS users) prefer native menus.
    void setIsNative(bool isNative);

private:
    Menubar();
    std::unique_ptr<Widget> createWidget() const;

    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk

#endif // UITK_MENUBAR_H
