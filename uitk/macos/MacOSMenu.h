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

#ifndef UITK_MACOS_MENU_H
#define UITK_MACOS_MENU_H

#include "../OSMenu.h"

namespace uitk {

class MacOSMenu : public OSMenu
{
public:
    MacOSMenu();
    ~MacOSMenu();

    void clear() override;
    int size() const override;
    void addItem(const std::string& text, MenuId id, const ShortcutKey& shortcut) override;
    void addMenu(const std::string& text, Menu *menu) override;
    void addSeparator() override;

    void insertItem(int index, const std::string& text, MenuId id, const ShortcutKey& shortcut) override;
    void insertMenu(int index, const std::string& text, Menu *menu) override;
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

    /*void removeItem(MenuId id);
    Menu* removeMenu(const std::string& text);  /// return ownership

    Menu* menu(const std::string& text) const;  /// retains ownership

    bool isSeparator(MenuId id) const;

    bool itemChecked(MenuId id) const;
    ItemFound setItemChecked(MenuId id, bool checked);

    bool itemEnabled(MenuId id) const;
    ItemFound setItemEnabled(MenuId id, bool enabled);

    std::string itemText(MenuId id) const;
    ItemFound setItemText(MenuId id, const std::string& text); */

    ItemFound activateItem(MenuId id, Window *activeWindow) const override;

    void* nsmenu();

    //bool isShowing() const override;
    //void show(Window *w, const Point& upperLeft, MenuId id = kInvalidId) override;
    //void cancel() override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_MACOS_MENU_H
