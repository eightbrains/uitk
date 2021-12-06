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

#ifndef UITK_OSMENUBAR_H
#define UITK_OSMENUBAR_H

#include "OSMenu.h"

#include <memory>
#include <string>

namespace uitk {

class Menu;
class Widget;

class OSMenubar
{
public:
    OSMenubar() {}
    OSMenubar(const OSMenubar&) = delete;
    virtual ~OSMenubar() {}

    /// Creates a new Menu and adds to the menubar. Retains ownership.
    virtual Menu* newMenu(const std::string& name) = 0;

    /// Adds a Menu to the menubar; takes ownership. When using native menus
    /// on Windows, an underscore will be used for key navigation; on all
    /// other platforms underscores will be removed.
    virtual void addMenu(Menu* menu, const std::string& name) = 0;

    /// Removes the first Menu that matches the name from the menubar.
    /// Gives ownership to the caller, or returns nullptr if no menu was found.
    virtual Menu* removeMenu(const std::string& name) = 0;

    /// Returns the first Menu that matches the name from the menubar,
    /// or nullptr if no matching menu was found. Ownership of the menu remains
    /// with the menubar.
    virtual Menu* menu(const std::string& name) const = 0;

    /// Returns the application menu on macOS, and nullptr on other platforms.
    /// The application menu is where macOS users expect to find "About...",
    /// "Preferences..." and "Quit".
    virtual Menu* macosApplicationMenu() const = 0;

    /// Sets the item with the given ID enabled (or disabled). This will
    /// search through the entire menu tree to find the item. If the item is
    /// not found the request will be ignored.
    virtual void setItemEnabled(MenuId itemId, bool enabled) = 0;

    /// Sets the item with the given ID checked (or unchecked). This will
    /// search through the entire menu tree to find the item. If the item is
    /// not found the request will be ignored.
    virtual void setItemChecked(MenuId itemId, bool checked) = 0;

    /// This is the programmatic way of clicking on a menu item. If the itemId
    /// exists in one of the menus its callback function will be called the
    /// same as if the user navigated through the menus. In particular,
    /// if the item is disabled, nothing will happen.
    virtual void activateItemId(MenuId itemId) const = 0;
};

}  // namespace uitk

#endif // UITK_OSMENUBAR_H
