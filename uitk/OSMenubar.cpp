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

#include "OSMenubar.h"

#include "Application.h"
#include "Events.h"
#include "Menu.h"

#include <unordered_set>

namespace uitk {

// Design notes:
// This is an awkward place to put these, but it seems like the menubar is the
// place callers will expect to find these. We do not want these to be pure
// virtual since the functionality will be the same for all the platforms, so
// having them in the abstracted layer enables us to reuse them.

void OSMenubar::addStandardItems(Menu **file, Menu **edit, Menu **window, Menu **help,
                                 const std::vector<StandardItem>& excluded /*= {}*/)
{
    auto vItems = { StandardItem::kAbout,
                    StandardItem::kQuit,
                    StandardItem::kCopy, StandardItem::kCut, StandardItem::kPaste,
                    StandardItem::kUndo, StandardItem::kRedo,
                    StandardItem::kPreferences,
                    StandardItem::kWindowList,
#if __APPLE__
                    StandardItem::kMacOSHideApp,
                    StandardItem::kMacOSHideOtherApps,
                    StandardItem::kMacOSShowOtherApps,
#endif
                    StandardItem::kMacOSMinimize,
                    StandardItem::kMacOSZoom,
                    StandardItem::kMacOSBringAllToFront,
                  };
    std::unordered_set<StandardItem> items;
    items.insert(vItems.begin(), vItems.end());
    for (auto ex : excluded) {
        items.erase(ex);
    }

    auto itemsHas = [&items](StandardItem item) { return (items.find(item) != items.end()); };
    auto addItem = [this, &items](Menu *menu, StandardItem item, int *index) -> bool {
        if (items.find(item) != items.end()) {
            this->addStandardItem(menu, item, *index);
            *index += 1;
            return true;
        }
        return false;
    };
    auto addSeparator = [this](Menu *menu, int *index, bool add = true) {
        if (add) {
            menu->insertSeparator(*index);
            *index += 1;
        }
    };

#if defined(__APPLE__)
    Menu *app = macosApplicationMenu();
    int idx = 0;
    if (addItem(app, StandardItem::kAbout, &idx)) {
        addSeparator(app, &idx);
    }
    if (addItem(app, StandardItem::kPreferences, &idx)) {
        addSeparator(app, &idx);
    }
    int hideAppsIdx = idx;
    addItem(app, StandardItem::kMacOSHideApp, &idx);
    addItem(app, StandardItem::kMacOSHideOtherApps, &idx);
    addItem(app, StandardItem::kMacOSShowOtherApps, &idx);
    if (hideAppsIdx < idx) {
        addSeparator(app, &idx);
    }
    if (addItem(app, StandardItem::kQuit, &idx)) {
        addSeparator(app, &idx);
    }

    idx = 0;
    Menu *editMenu = nullptr;
    if (!edit) {
        edit = &editMenu;
    }
    if (!*edit) {
        *edit = newMenu("Edit");
    }
    addItem(*edit, StandardItem::kUndo, &idx);
    addItem(*edit, StandardItem::kRedo, &idx);
    addSeparator(*edit, &idx, (idx > 0));
    int ccpIdx = idx;
    addItem(*edit, StandardItem::kCut, &ccpIdx);
    addItem(*edit, StandardItem::kCopy, &ccpIdx);
    addItem(*edit, StandardItem::kPaste, &ccpIdx);
    addSeparator(*edit, &idx, (ccpIdx > idx));

    idx = 0;
    Menu *windowMenu = nullptr;
    if (!window) {
        window = &windowMenu;
    }
    if (!*window) {
        *window = newMenu("&Window");
    }
    bool origIsEmpty = ((*window)->size() == 0);
    addItem(*window, StandardItem::kMacOSMinimize, &idx);
    addItem(*window, StandardItem::kMacOSZoom, &idx);
    addSeparator(*window, &idx, (idx > 0));
    idx = (*window)->size();
    addSeparator(*window, &idx, !origIsEmpty);
    if (addItem(*window, StandardItem::kMacOSBringAllToFront, &idx)) {
        addSeparator(*window, &idx, (items.find(StandardItem::kWindowList) != items.end()));
    }
    addItem(*window, StandardItem::kWindowList, &idx);
#else
    int idx = 0;
    Menu *fileMenu = nullptr;
    if (!file) {
        file = &fileMenu;
    }
    if (!*file) {
        *file = newMenu("&File");
    }
    idx = (*file)->size();
    if (addItem(file, StandardItem::kQuit, &idx)) {
        --idx;
        addSeparator(file, &idx);
    }

    idx = 0;
    Menu *editMenu = nullptr;
    if (!edit) {
        edit = &editMenu;
    }
    if (!*edit) {
        *edit = newMenu("&Edit");
    }
    addItem(*edit, StandardItem::kUndo, &idx);
    addItem(*edit, StandardItem::kRedo, &idx);
    addSeparator(*edit, &idx, (idx > 0));
    int ccpIdx = idx;
    addItem(*edit, StandardItem::kCut, &ccpIdx);
    addItem(*edit, StandardItem::kCopy, &ccpIdx);
    addItem(*edit, StandardItem::kPaste, &ccpIdx);
    addSeparator(*edit, &idx, (ccpIdx > idx));

    idx = (*edit)->size();
    addSeparator(*edit, &idx, itemsHas(StandardItem::kPreferences));
    --idx;
    addItem(*edit, StandardItem::kPreferences, &idx);

    int idx = 0;
    Menu *helpMenu = nullptr;
    if (!file) {
        file = &helpMenu;
    }
    if (!*help) {
        *help = newMenu("&Help");
    }
    addItem(app, StandardItem::kAbout, &idx);
#endif
}

void OSMenubar::addStandardItem(Menu *menu, StandardItem item, int index)
{
    switch (item) {
        case StandardItem::kAbout:
#if defined(__APPLE__)
            menu->insertItem(index, "&About " + Application::instance().applicationName() + "...",
                             MenuId(item), ShortcutKey::kNone);
#else
            menu->insertItem(index, "&About...", MenuId(item), ShortcutKey::kNone);
#endif
            break;
        case StandardItem::kQuit:
#if defined(_WIN32) || defined(_WIN64)
            menu->insertItem(index, "E&xit", MenuId(item), ShortcutKey::kNone);
#elif defined(__APPLE__)
            menu->insertItem(index, "Quit " + Application::instance().applicationName(), MenuId(item),
                             ShortcutKey(KeyModifier::kCtrl, Key::kQ));
#else
            menu->insertItem(index, "Quit", MenuId(item), ShortcutKey(KeyModifier::kCtrl, Key::kQ));
#endif
            break;
        case StandardItem::kCut:
            menu->insertItem(index, "&Cut", MenuId(item), ShortcutKey(KeyModifier::kCtrl, Key::kX));
            break;
        case StandardItem::kCopy:
            menu->insertItem(index, "C&opy", MenuId(item), ShortcutKey(KeyModifier::kCtrl, Key::kC));
            break;
        case StandardItem::kPaste:
            menu->insertItem(index, "&Paste", MenuId(item), ShortcutKey(KeyModifier::kCtrl, Key::kV));
            break;
        case StandardItem::kUndo:
            menu->insertItem(index, "&Undo", MenuId(item), ShortcutKey(KeyModifier::kCtrl, Key::kZ));
            break;
        case StandardItem::kRedo:
#if defined(_WIN32) || defined(_WIN64)
            menu->insertItem(index, "&Redo", MenuId(item), ShortcutKey(KeyModifier::kCtrl, Key::kY));
#else
            menu->insertItem(index, "Redo", MenuId(item),
                             ShortcutKey(KeyModifier::Values(KeyModifier::kCtrl | KeyModifier::kShift),
                                         Key::kZ));
#endif
            break;
        case StandardItem::kPreferences:
            menu->insertItem(index, "&Preferences", MenuId(item),
                             ShortcutKey(KeyModifier::kCtrl, Key::kNumComma));
            break;
        case StandardItem::kWindowList:
            menu->insertItem(index, "Window List", MenuId(item), ShortcutKey::kNone);
            break;
        case StandardItem::kMacOSHideApp:
#if defined(__APPLE__)
            menu->insertItem(index, "Hide " + Application::instance().applicationName(), MenuId(item),
                             ShortcutKey(KeyModifier::kCtrl, Key::kH));
#endif // __APPLE_
            break;
        case StandardItem::kMacOSHideOtherApps:
#if defined(__APPLE__)
            menu->insertItem(index, "Hide Others", MenuId(item),
                             ShortcutKey(KeyModifier::Values(KeyModifier::kCtrl | KeyModifier::kAlt),
                                         Key::kH));
#endif // __APPLE_
            break;
        case StandardItem::kMacOSShowOtherApps:
#if defined(__APPLE__)
            menu->insertItem(index, "Show All", MenuId(item), ShortcutKey::kNone);
#endif // __APPLE_
            break;
        case StandardItem::kMacOSMinimize:
            menu->insertItem(index, "Minimize", MenuId(item), ShortcutKey(KeyModifier::kCtrl, Key::kM));
            break;
        case StandardItem::kMacOSZoom:
            menu->insertItem(index, "Zoom", MenuId(item), ShortcutKey::kNone);
            break;
        case StandardItem::kMacOSBringAllToFront:
            menu->insertItem(index, "Bring All To Front", MenuId(item), ShortcutKey::kNone);
            break;
        case StandardItem::kWindow1:
        case StandardItem::kWindow2:
        case StandardItem::kWindow3:
        case StandardItem::kWindow4:
        case StandardItem::kWindow5:
        case StandardItem::kWindow6:
        case StandardItem::kWindow7:
        case StandardItem::kWindow8:
        case StandardItem::kWindow9:
        case StandardItem::kWindow10:
            break;
    }
}

}  // namespace uitk
