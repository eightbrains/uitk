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
    void addItem(const std::string& text, MenuId id, const ShortcutKey& shortcut) override;
    void addMenu(const std::string& text, Menu *menu) override;
    void addSeparator() override;

    void insertItem(int index, const std::string& text, MenuId id, const ShortcutKey& shortcut) override;
    void insertMenu(int index, const std::string& text, Menu *menu) override;
    void insertSeparator(int index) override;

    void removeItem(MenuId id) override;
    Menu* removeMenu(const std::string& text) override;  /// return ownership

    Menu* menu(const std::string& text) const override;  /// retains ownership

    bool isSeparator(MenuId id) const override;

    bool itemChecked(MenuId id) const override;
    ItemFound setItemChecked(MenuId id, bool checked) override;

    bool itemEnabled(MenuId id) const override;
    ItemFound setItemEnabled(MenuId id, bool enabled) override;

    std::string itemText(MenuId id) const override;
    ItemFound setItemText(MenuId id, const std::string& text) override;

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
