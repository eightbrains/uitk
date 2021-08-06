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

#include "Menu.h"

#include "Events.h"

#include <unordered_map>

namespace uitk {

namespace {

struct Item
{
    std::string title;
    int shortcutKeymods;
    Key shortcutKey;
};

struct StandardItemInfo
{
    std::unordered_map<Menu::StandardItem, Item> item2item;


    void add(Menu::StandardItem item, const std::string& title, int keymods = 0, Key key = Key::kNone)
    {
        item2item[item] = Item{title, keymods, key};
    }

    StandardItemInfo()
    {
        add(Menu::StandardItem::kCopy, "Copy", KeyModifier::kCtrl, Key::kC);
        add(Menu::StandardItem::kCut, "Cut", KeyModifier::kCtrl, Key::kX);
        add(Menu::StandardItem::kPaste, "Paste", KeyModifier::kCtrl, Key::kV);
        add(Menu::StandardItem::kUndo, "Undo", KeyModifier::kCtrl, Key::kZ);
        add(Menu::StandardItem::kRedo, "Redo", KeyModifier::kCtrl, Key::kShiftZ);
        add(Menu::StandardItem::kAbout, "About...");
        add(Menu::StandardItem::kPreferences, "Preferences...");
#if defined(__APPLE__)
        add(Menu::StandardItem::kQuit, "Quit", KeyModifier::kCtrl, Key::kQ);
#elif defined(_WIN32) || defined(_WIN64)  // _WIN32 covers everything except 64-bit ARM
        add(Menu::StandardItem::kQuit, "Exit", KeyModifier::kCtrl, Key::kQ);
#else
        add(Menu::StandardItem::kQuit, "Quit", KeyModifier::kCtrl, Key::kQ);
#endif
    }
};
StandardItemInfo gStandardItems;

} // namespace

bool Menu::isShortcutFor(StandardItem item, const KeyEvent& e)
{
    auto info = gStandardItems.item2item[item];
    return (e.keymods == info.shortcutKeymods && e.key == info.shortcutKey);
}

} // namespace uitk
