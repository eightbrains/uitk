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

#ifndef UITK_WIN32_MENU_H
#define UITK_WIN32_MENU_H

#include "../OSMenu.h"

namespace uitk {

class Win32Menu : public OSMenu
{
public:
    Win32Menu();
    virtual ~Win32Menu();

    void clear() override;
    int size() const override;
    void addItem(const std::string& text, MenuId id, const ShortcutKey& shortcut) override;
    /// Takes ownership of menu
    void addMenu(const std::string& text, Menu* menu) override;
    void addSeparator() override;

    void insertItem(int index, const std::string& text, MenuId id, const ShortcutKey& shortcut) override;
    /// Takes ownership of menu
    void insertMenu(int index, const std::string& text, Menu* menu) override;
    void insertSeparator(int index) override;

    void removeItem(int index) override;
    // Does NOT destroy the menu, returns ownership to caller
    Menu* removeMenu(int index) override;

    MenuId itemIdAt(int index) const override;
    Menu* itemMenuAt(int index) const override;

    bool isSubmenuAt(int index) const override;
    bool isSeparatorAt(int index) const override;

    bool itemCheckedAt(int index) const override;
    void setItemCheckedAt(int index, bool checked) override;

    bool itemEnabledAt(int index) const override;
    void setItemEnabledAt(int index, bool enabled) override;

    std::string itemTextAt(int index) const override;
    void setItemTextAt(int index, const std::string& text) override;

/*    void removeItem(MenuId id) override;
    // Returns ownership of the menu (if it exists)
    Menu* removeMenu(const std::string& text) override;

    // Returns the menu associated with
    Menu* menu(const std::string& text) const override;

    bool itemChecked(MenuId id) const override;
    ItemFound setItemChecked(MenuId id, bool checked) override;

    bool itemEnabled(MenuId id) const override;
    ItemFound setItemEnabled(MenuId id, bool enabled) override;

    std::string itemText(MenuId id) const override;
    ItemFound setItemText(MenuId id, const std::string& text) override;
*/
    ItemFound activateItem(MenuId id, Window* activeWindow) const override;

    //bool isShowing() const override;
    //void show(Window *w, const Point& upperLeft, MenuId id = kInvalidId) override;
    //void cancel() override;

    /// Creates a Win32 HMENU implementing this menu truee. The caller owns the HMENU.
    /// The return value is void* to avoid pulling in windows.h into this header file.
    void* createNativeMenu();

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_WIN32_MENU_H
