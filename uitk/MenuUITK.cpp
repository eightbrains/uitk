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

#include "MenuUITK.h"

#include "Application.h"
#include "Label.h"
#include "ListView.h"
#include "Menu.h"
#include "UIContext.h"
#include "Window.h"
#include "private/Utils.h"
#include "themes/Theme.h"

#include <chrono>
#include <functional>
#include <string>
#include <unordered_map>

/* Requirements:
   - Add and insert regular items, separator items, and submenu items
   - Regular items (and only regular items) can have a shortcut key.
   - If this is a context menu, can have an action (but no shortcuts for a
     context menu).
   - Items should always have space on the left for a checkmark.
   - Shortcuts should be left-justified in the right column.
   - Submenus should have a indicator icon right-justified in the right column.
   - Highlighting a submenu item should open it, preferably after a 200 ms delay
     so that quickly cutting across the upper right corner of the item below
     does not cancel the menu open.
     - the submenu item should remain highlighted while the submenu is open
       unless the mouse hovers back over the original menu in a different
       item (including disabled / separator items), in which case the submenu
       should be canceled.
   - Clicking on an enabled, regular item should blink quickly, the menu
     disappear, and the action be taken.
   - Mousing over an item in the menubar while a menubar menu is open should
     cancel the current menu and open the item under the cursor. Mousing over
     empty space (not including item margins) in the menubar should do nothing.
   - Enter, return, and space are the same as clicking on an item.
   - Up and down should move the highlight to the next enabled item. If it
     is a submenu, the menu should not open until left/enter/return/space are
     pressed. If the mouse moves after highlight was changed, the highlight
     should change to the item under the mouse.
   - Left should close a submenu, right should open it. If the item is a
     regular item (or no items are highlighted), the menu to the left/right
     in the menubar should open (unless this is a context menu, of course).
   - Escape should cancel the entire menu hierarchy.
   - Activating a shortcut should blink the menubar item, then take the
     action in the corresponding menu. If the menu item is disabled, then do
     not blink but beep.
   - On macOS, Ctrl-F2 should enter menubar navigation. On Windows/Linux,
     pressing and releasing Alt (without any other key) should enter menubar
     navigation.
   - Test: move mouse slowly from a separator item to a submenu item. Ensures
     that entering a submenu item on the top edge does not immediately cancel
     because it is also on the bottom edge of the disabled item.
 */
namespace uitk {

namespace {
class MenuItemWidget : public Widget
{
    using Super = Widget;
public:
    enum Separator { kSeparator };
    MenuItemWidget(const std::string& text, const std::string& shortcut) : mText(text), mShortcut(shortcut) {}
    MenuItemWidget(const std::string& text, Menu *menu) : mText(text), mSubmenu(menu) {}
    explicit MenuItemWidget(Separator s) : mIsSeparator(true) {}

    virtual ~MenuItemWidget() {}

    bool isSeparator() const { return mIsSeparator; }

    const std::string& text() const { return mText; }
    void setText(const std::string& text) { mText = text; }

    const std::string& shortcut() const { return mShortcut; }
    void setShortcut(const std::string& shortcut) { mShortcut = shortcut; }

    bool checked() const { return mChecked; }
    void setChecked(bool checked) {
        assert(!mIsSeparator);
        assert(!mSubmenu);
        mChecked = checked;
    }

    bool isClickable() const
    {
        return (enabled() && !isSeparator() && submenu() == nullptr);
    }

    Menu* submenu() const { return mSubmenu.get(); }

    Menu* removeSubmenu() { return mSubmenu.release(); }  // transfers ownership to caller

    virtual PicaPt preferredShortcutWidth(const LayoutContext& context) const = 0;

    void setShortcutWidth(const PicaPt& w) { mShortcutWidth = w; }

    void mouseEntered() override
    {
        Super::mouseEntered();
        if (auto *w = window()) {
            if (auto *popup = w->popupMenu()) {
                popup->cancel();
            }
        }
    }

protected:
    std::string mText;
    std::string mShortcut;
    std::unique_ptr<Menu> mSubmenu;
    bool mIsSeparator = false;
    bool mChecked = false;
    PicaPt mShortcutWidth;
};

class StringMenuItem : public MenuItemWidget
{
    using Super = MenuItemWidget;
public:
    explicit StringMenuItem(const std::string& text, const ShortcutKey& shortcut)
        : MenuItemWidget(text, shortcut.displayText())
    {}
    ~StringMenuItem() {}

    PicaPt preferredShortcutWidth(const LayoutContext& context) const override
    {
        return mPreferredShortcutWidth;
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        auto attr = (checked() ? Theme::MenuItemAttribute::kChecked
                                   : Theme::MenuItemAttribute::kNormal);
        return context.theme.calcPreferredMenuItemSize(context.dc, mText, mShortcut, attr,
                                                       &mPreferredShortcutWidth);
    }

    void draw(UIContext& context) override
    {
        auto attr = (checked() ? Theme::MenuItemAttribute::kChecked
                               : Theme::MenuItemAttribute::kNormal);
        auto s = themeState();
        context.theme.drawMenuItem(context, bounds(), mShortcutWidth, text(), shortcut(), attr, style(s), s);
    }

private:
    mutable PicaPt mPreferredShortcutWidth;
};

class SeparatorMenuItem : public MenuItemWidget
{
public:
    SeparatorMenuItem()
        : MenuItemWidget(MenuItemWidget::kSeparator)
    {
        setEnabled(false);
    }

    PicaPt preferredShortcutWidth(const LayoutContext& context) const override { return PicaPt::kZero; }

    Size preferredSize(const LayoutContext& context) const override
    {
        auto h = context.theme.calcPreferredMenuItemSize(context.dc, "Ag", "",
                                                         Theme::MenuItemAttribute::kNormal, nullptr).height;
        return Size(3.0f * h, h);
    }

    void draw(UIContext& context) override
    {
        context.theme.drawMenuSeparatorItem(context, bounds());
    }
};

class SubmenuItem : public MenuItemWidget
{
    using Super = MenuItemWidget;
public:
    explicit SubmenuItem(const std::string& text, Menu *submenu)
        : MenuItemWidget(text, submenu)
    {
    }

    void openSubmenu() const
    {
        if (auto *w = window()) {
            auto *popup = w->popupMenu();
            auto *menu = (submenu() ? submenu()->menuUitk() : nullptr);
            if (popup != menu) {
                if (popup) {
                    popup->cancel();
                }
                if (menu) {
                    menu->show(w, convertToWindowFromLocal(bounds().upperRight()));
                }
            }
        }
    }

    PicaPt preferredShortcutWidth(const LayoutContext& context) const override
    {
        return frame().height;
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        return context.theme.calcPreferredMenuItemSize(context.dc, mText, mShortcut,
                                                       Theme::MenuItemAttribute::kSubmenu, nullptr);
    }

    void mouseEntered() override
    {
        Widget::mouseEntered();  // skip parent: don't necessary want to close an open menu
        openSubmenu();
    }

    void draw(UIContext& context) override
    {
        auto s = themeState();
        context.theme.drawMenuItem(context, bounds(), mShortcutWidth, text(), shortcut(),
                                   Theme::MenuItemAttribute::kSubmenu, style(s), s);
    }
};

class MenuListView : public ListView
{
    using Super = ListView;
public:
    MenuListView(MenuUITK *m) : mMenuUitk(m) {}
    ~MenuListView()
    {
    }

    EventResult mouse(const MouseEvent& e) override
    {
        auto retval = Super::mouse(e);

        // Cancel any submenu if we mouseover a disabled item (which does not
        // get mouseover events itself). For example, when a user opens a
        // submenu, then mouses over a disabled item (or separator), especially
        // if they first mouse over the submenu, then return to the parent menu.
        auto *w = window();
        if (w && w->popupMenu()) {
            auto row = calcRowIndex(e.pos);
            auto *cell = cellAtIndex(row);
            // Note that calcRowIndex() uses mathematical rects, but the mouse
            // uses pixels. So if the mouse is exactly on the border, the
            // frame rects of two cells will test true for contains(e).
            // Treat the mathematical edge of the bottom of the frame as in
            // the next cell. (To test, move mouse slowly from a separator
            // item to a submenu item in Linux/X11.)
            auto f = cell->frame();
            if (cell && !cell->enabled() && e.pos.y > f.y && e.pos.y < f.maxY()) {
                w->popupMenu()->cancel();
            }
        }

        // We should not have a selection while moving the mouse. However, we
        // might have one if we opened a submenu with Key::kRight, then closed
        // with kLeft, then moved the mouse.
        if (selectedIndex() >= 0) {
            clearSelection();
        }

        return retval;
    }

    void mouseExited() override
    {
        // Since the appearance of the submenu being selected is done via
        // the mouseover code, when a submenu is active, we do not want to set
        // the state away from mouse over if the mouse exits the window,
        // otherwise the appearance of the submenu item will become unselected,
        // which looks pretty odd.
        auto *w = window();
        if (w && w->popupMenu()) {
            ;  // don't super
        } else {
            Super::mouseExited();
        }
    }

    void key(const KeyEvent& e) override
    {
        bool handled = false;
        if (e.type == KeyEvent::Type::kKeyDown) {
            switch (e.key) {
                case Key::kUp:
                case Key::kDown:
                    // if opened submenu with right, then closed with left, then move up or down
                    // we don't want the selection created with right to persist (and show two
                    // highlighted items), so clear.
                    clearSelection();
                    handled = false;  // want to call super!
                    break;
                case Key::kLeft:
                    if (auto *win = window()) {
                        mMenuUitk->cancel();
                    }
                    handled = true;
                    break;
                case Key::kRight:
                    if (auto *item = dynamic_cast<MenuItemWidget*>(cellAtIndex(highlightedIndex()))) {
                        if (!item->submenu()) {
                            break;  // not a submenu item, ignore
                        }
                    }
                    // this is a submenu item: fall through
                case Key::kEnter:
                case Key::kReturn:
                case Key::kSpace:
                    clearSelection();  // in case already selected (right to open menu, left to close, right)
                    // transform to Enter, in case this was kRight
                    Super::key({ KeyEvent::Type::kKeyDown, Key::kEnter, 0, 0, false });
                    handled = false;  // want to call super!
                    break;
                case Key::kEscape:
                    mMenuUitk->cancelHierarchy();
                    handled = true;
                    break;
                default:
                    handled = false;
                    break;
            }
        }
        if (!handled) {
            Super::key(e);
        }
    }

    void blinkSelection(int index, std::function<void()> onDone)
    {
        mBlinkState = BlinkState::kStart;
        mBlinkIndex = index;
        mBlinkStartTime = std::chrono::steady_clock::now();
        mOnBlinkDone = onDone;
        // We can't just have a nested set of calls to Application::scheduleLater(),
        // because they may happen before the all the draws finish. So we update
        // the blink state in draw().
        setNeedsDraw();
    }

    bool isBlinking() const { return (mBlinkState != BlinkState::kNone); }

    void draw(UIContext& context) override
    {
        Super::draw(context);
        if (mBlinkState != BlinkState::kNone) {
            auto now = std::chrono::steady_clock::now();
            auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - mBlinkStartTime).count();
            auto postDraw = [this]() {
                // Need to post this, since the needs-draw flag will be cleared
                // at the end of the draw.
                Application::instance().scheduleLater(window(), [this]() {
                    setNeedsDraw();
                });
            };

            switch (mBlinkState) {
                case BlinkState::kNone:
                case BlinkState::kStart:
                    mBlinkState = BlinkState::kBlinkOff;
                    postDraw();
                    break;
                case BlinkState::kBlinkOff:
                    clearSelection();
                    setSelectionModel(SelectionMode::kNoItems);
                    mBlinkState = BlinkState::kBlinkOffWait;
                    postDraw();
                    break;
                case BlinkState::kBlinkOffWait:
                    if (dt > 100 /* ms */) {
                        mBlinkState = BlinkState::kBlinkOn;
                    }
                    postDraw();
                    break;
                case BlinkState::kBlinkOn:
                    setSelectionModel(SelectionMode::kSingleItem);
                    setSelectedIndex(mBlinkIndex);
                    mBlinkState = BlinkState::kBlinkOnWait;
                    postDraw();
                    break;
                case BlinkState::kBlinkOnWait:
                    if (dt > 150 /* ms */) {
                        mBlinkState = BlinkState::kEnd;
                    }
                    postDraw();
                    break;
                case BlinkState::kEnd:
                    if (mOnBlinkDone) {
                        mOnBlinkDone();
                    }
                    mBlinkState = BlinkState::kNone;
                    mBlinkIndex = -1;
                    mOnBlinkDone = nullptr;
                    break;
            }
        }
    }

private:
    MenuUITK *mMenuUitk;  // we do not own this (it is our parent window's owner)

    enum class BlinkState { kNone = 0, kStart = 1, kBlinkOff = 2, kBlinkOffWait = 3, kBlinkOn = 4, kBlinkOnWait = 5, kEnd = 6 };
    BlinkState mBlinkState = BlinkState::kNone;
    int mBlinkIndex = -1;
    std::chrono::steady_clock::time_point mBlinkStartTime;
    std::function<void()> mOnBlinkDone;
};

} // namspace

//-----------------------------------------------------------------------------
struct MenuUITK::Impl
{
    struct ItemData {
        MenuItemWidget* item;  // this acts as a reference (but we can't use a reference b/c no copy constructor)
        std::function<void()> onSelected;
    };

    std::vector<MenuItemWidget*> items;  // we own these
    std::unordered_map<MenuId, ItemData> id2item;
    std::function<void()> onClose;
    std::function<void()> onCancelParentMenu;  // this is not exposed
    Window *menuWindow = nullptr;  // we own this
    Window *parent = nullptr;  // we don't own this
    MenuListView *listView = nullptr;  // we don't own this

    bool isShowing = false;
    mutable PicaPt shortcutWidth;

    ItemData* itemForId(MenuId id)
    {
        auto it = this->id2item.find(id);
        if (it != this->id2item.end()) {
            return &it->second;
        }
        for (auto *item : items) {
            if (auto *menu = item->submenu()) {
                assert(menu->menuUitk());
                if (auto *menuUitk = menu->menuUitk()) {
                    if (auto subitem = menuUitk->mImpl->itemForId(id)) {
                        return subitem;
                    }
                }
            }
        }
        return nullptr;
    }

    void insertItem(int index, MenuItemWidget*item, MenuId id, std::function<void()> onItem)
    {
        index = std::min(index, int(this->items.size()));
        item->setText(removeMenuItemMnemonics(item->text()));
        this->id2item[id] = Impl::ItemData{item, onItem};
        this->items.insert(this->items.begin() + index, item);
    }

    void addItem(MenuItemWidget*item, MenuId id, std::function<void()> onItem)
    {
        insertItem(int(this->items.size()), item, id, onItem);
    }

    void onMouseEnteredNormalItem()
    {
        if (this->menuWindow) {
            this->menuWindow->setPopupMenu(nullptr);
        }
    }

    void onMouseEnteredSubmenuItem(Menu *menu)
    {
        if (this->menuWindow) {
            this->menuWindow->setPopupMenu(nullptr);
            if (menu) {
                this->menuWindow->setPopupMenu(menu->menuUitk());
            }
        }
    }
};

MenuUITK::MenuUITK()
    : mImpl(new Impl())
{
}

MenuUITK::~MenuUITK()
{
    if (mImpl->menuWindow) {
        mImpl->menuWindow->setPopupMenu(nullptr);
    }

    for (auto *item : mImpl->items) {
        delete item;
    }
    delete mImpl->menuWindow;
}

void MenuUITK::setOnClose(std::function<void()> onClose)
{
    mImpl->onClose = onClose;
}

void MenuUITK::clear()
{
    mImpl->items.clear();
    mImpl->id2item.clear();
}

int MenuUITK::size() const { return int(mImpl->items.size()); }

void MenuUITK::addItem(const std::string& text, MenuId id,
                       const ShortcutKey& shortcut)
{
    mImpl->addItem(new StringMenuItem(text, shortcut), id, nullptr);
    Application::instance().keyboardShortcuts().add(id, shortcut);
}

void MenuUITK::addItem(const std::string& text, MenuId id, std::function<void()> onSelected)
{
    mImpl->addItem(new StringMenuItem(text, ShortcutKey::kNone), id, onSelected);
}

void MenuUITK::addMenu(const std::string& text, Menu *menu)
{
    insertMenu(int(mImpl->items.size()), text, menu);
}

void MenuUITK::addSeparator()
{
    mImpl->addItem(new SeparatorMenuItem(), kInvalidId, nullptr);
}

void MenuUITK::insertItem(int index, const std::string& text, MenuId id,
                          const ShortcutKey& shortcut)
{
    mImpl->insertItem(index, new StringMenuItem(text, shortcut), id, nullptr);
    Application::instance().keyboardShortcuts().add(id, shortcut);
}

void MenuUITK::insertItem(int index, const std::string& text, MenuId id,
                         std::function<void()> onSelected)
{
    mImpl->insertItem(index, new StringMenuItem(text, ShortcutKey::kNone), id, onSelected);
}

void MenuUITK::insertMenu(int index, const std::string& text, Menu *menu)
{
    if (menu && menu->menuUitk()) {
        auto *menuUitk = menu->menuUitk();
        menuUitk->mImpl->onCancelParentMenu = [this]() {
            this->cancel();
            if (mImpl->onCancelParentMenu) {
                mImpl->onCancelParentMenu();
            }
        };
    }
    mImpl->insertItem(index, new SubmenuItem(text, menu), kInvalidId, nullptr);
}

void MenuUITK::insertSeparator(int index)
{
    mImpl->insertItem(index, new SeparatorMenuItem(), kInvalidId, nullptr);
}

void MenuUITK::removeItem(int index)
{
    if (index >= 0 && index < int(mImpl->items.size())) {
        if (mImpl->listView) {
            mImpl->listView->removeCellAtIndex(index);
        }
        delete mImpl->items[index];
        mImpl->items.erase(mImpl->items.begin() + index);
    }
}

void MenuUITK::removeItem(MenuId id)
{
    auto it = mImpl->id2item.find(id);
    if (it != mImpl->id2item.end()) {
        for (auto itemsIt = mImpl->items.begin();  itemsIt != mImpl->items.end();  ++itemsIt) {
            if (it->second.item == *itemsIt) {
                mImpl->id2item.erase(it);
                int index = int(itemsIt - mImpl->items.begin());
                removeItem(index); // changes mImpl->items, so invalidates itemsIt
                return;
            }
        }
    }
    Application::instance().keyboardShortcuts().remove(id);
}

Menu* MenuUITK::removeMenu(const std::string& text)
{
    for (auto it = mImpl->items.begin();  it != mImpl->items.end();  ++it) {
        if ((*it)->submenu() && (*it)->text() == text) {
            Menu *menu = (*it)->removeSubmenu();
            removeItem(int(it - mImpl->items.begin()));
        }
    }
    return nullptr;
}

MenuId MenuUITK::itemIdAt(int index) const
{
    if (index >= 0 && index < int(mImpl->items.size())) {
        auto *item = mImpl->items[index];
        for (auto it = mImpl->id2item.begin(); it != mImpl->id2item.end(); ++it) {
            if (it->second.item == item) {
                return it->first;
            }
        }
    }
    return kInvalidId;
}

bool MenuUITK::isSubmenuAt(int index) const
{
    if (index >= 0 && index < int(mImpl->items.size())) {
        return (mImpl->items[index]->submenu() != nullptr);
    }
    return false;
}

bool MenuUITK::isSeparatorAt(int index) const
{
    if (index >= 0 && index < int(mImpl->items.size())) {
        return mImpl->items[index]->isSeparator();
    }
    return false;
}

Menu* MenuUITK::itemMenuAt(int index) const
{
    if (index >= 0 && index < int(mImpl->items.size())) {
        return mImpl->items[index]->submenu();
    }
    return nullptr;
}

Menu* MenuUITK::itemMenu(const std::string& text) const
{
    for (auto it = mImpl->items.begin();  it != mImpl->items.end();  ++it) {
        if ((*it)->submenu() && (*it)->text() == text) {
            return (*it)->submenu();
        }
    }
    return nullptr;
}

bool MenuUITK::itemCheckedAt(int index) const
{
    if (index >= 0 && index < int(mImpl->items.size())) {
        return mImpl->items[index]->checked();
    }
    return false;
}

bool MenuUITK::itemChecked(MenuId id) const
{
    if (auto item = mImpl->itemForId(id)) {
        return item->item->checked();
    }
    return false;
}

void MenuUITK::setItemCheckedAt(int index, bool checked)
{
    if (index >= 0 && index < int(mImpl->items.size())) {
        mImpl->items[index]->setChecked(checked);
    }
}

MenuUITK::ItemFound MenuUITK::setItemChecked(MenuId id, bool checked)
{
    if (auto item = mImpl->itemForId(id)) {
        item->item->setChecked(checked);
        return ItemFound::kYes;
    }
    return ItemFound::kNo;
}

bool MenuUITK::itemEnabledAt(int index) const
{
    if (index >= 0 && index < int(mImpl->items.size())) {
        return mImpl->items[index]->enabled();
    }
    return false;
}


bool MenuUITK::itemEnabled(MenuId id) const
{
    if (auto item = mImpl->itemForId(id)) {
        return item->item->enabled();
    }
    return false;
}

void MenuUITK::setItemEnabledAt(int index, bool enabled)
{
    if (index >= 0 && index < int(mImpl->items.size())) {
        mImpl->items[index]->setEnabled(enabled);
    }
}

MenuUITK::ItemFound MenuUITK::setItemEnabled(MenuId id, bool enabled)
{
    if (auto item = mImpl->itemForId(id)) {
        item->item->setEnabled(enabled);
        return ItemFound::kYes;
    }
    return ItemFound::kNo;
}

std::string MenuUITK::itemTextAt(int index) const
{
    if (index >= 0 && index < int(mImpl->items.size())) {
        return mImpl->items[index]->text();
    }
    return false;
}

std::string MenuUITK::itemText(MenuId id) const
{
    static const std::string kNoItemText = "";

    if (auto item = mImpl->itemForId(id)) {
        return item->item->text();
    }
    return kNoItemText;
}

void MenuUITK::setItemTextAt(int index, const std::string& text)
{
    if (index >= 0 && index < int(mImpl->items.size())) {
        mImpl->items[index]->setText(removeMenuItemMnemonics(text));
    }
}

MenuUITK::ItemFound MenuUITK::setItemText(MenuId id, const std::string& text)
{
    if (auto item = mImpl->itemForId(id)) {
        item->item->setText(removeMenuItemMnemonics(text));
        return ItemFound::kYes;
    }
    return ItemFound::kNo;
}

MenuUITK::ItemFound MenuUITK::activateItem(MenuId id, Window *activeWindow) const
{
    if (auto item = mImpl->itemForId(id)) {
        if (item->item->enabled()) {
            activeWindow->onMenuActivated(id);
            return ItemFound::kYes;
        } else {
            Application::instance().beep();
            return ItemFound::kDisabled;
        }
    }
    return ItemFound::kNo;
}

Size MenuUITK::preferredSize(const LayoutContext& context) const
{
    auto shortcutWidth = PicaPt::kZero;
    Size pref(PicaPt::kZero, PicaPt::kZero);
    for (auto item : mImpl->items) {
        auto itemPref = item->preferredSize(context);
        pref.width = std::max(pref.width, itemPref.width);
        pref.height += itemPref.height;
        shortcutWidth = std::max(shortcutWidth, item->preferredShortcutWidth(context));
    }
    mImpl->shortcutWidth = shortcutWidth;
    return pref;
}

void MenuUITK::drawItem(UIContext& context, const Rect& frame, MenuId id, Theme::WidgetState itemState)
{
    auto *item = mImpl->itemForId(id);
    if (item) {
        if (item->item->frame().isEmpty()) {
            item->item->setFrame(frame);
        }
        item->item->draw(context);
    }
}

Window* MenuUITK::window() { return mImpl->menuWindow; }

bool MenuUITK::isShowing() const
{
    // mImpl->menuWindow may not exist if the menu is empty, but if we
    // asked the menu to show(), then it should be showing.
    return mImpl->isShowing;
}

void MenuUITK::show(Window *w, const Point& upperLeftWindowCoord, MenuId id /*= OSMenu::kInvalidId*/,
                    int extraWindowFlags /*= 0*/)
{
    if (mImpl->menuWindow) {  // shouldn't happen, but handle it if it does
        cancel();
    }

    mImpl->isShowing = true;
    if (mImpl->items.empty()) {
        return;
    }

    // Should we keep another mapping from id -> index? Seems unnecessary
    // since we are only going to use it in this function. This will be O(n), but
    // presumably menus are going to be reasonably sized.

    auto osUL = w->convertWindowToOSPoint(upperLeftWindowCoord);
    mImpl->menuWindow = new Window("", int(osUL.x), int(osUL.y), 0, 0,
                                   (Window::Flags::Value)(Window::Flags::kPopup | extraWindowFlags));
#if __APPLE__
    // The window border is inside the window area on macOS
    auto border = mImpl->menuWindow->borderWidth();
    mImpl->menuWindow->move(-border, -border);
#endif // __APPLE__
    mImpl->menuWindow->setOnWindowDidDeactivate([this](Window &w) {
        cancel();
    });

    auto *list = new MenuListView(this);  // will be owned by menuWindow
    list->setBorderWidth(PicaPt::kZero);
    list->setContentPadding(PicaPt::kZero, PicaPt::kZero);
    // Highlight on mouseover, unlike normal ListView
    list->style(Theme::WidgetState::kMouseOver).fgColor = Application::instance().theme()->params().accentColor;
    list->style(Theme::WidgetState::kMouseOver).flags |= Theme::WidgetStyle::kFGColorSet;
    for (auto *item : mImpl->items) {
        list->addCell(item);
    }
    mImpl->listView = list;
    mImpl->menuWindow->addChild(list);

    mImpl->menuWindow->setOnWindowWillShow([this, list, id](Window& w, const LayoutContext& context) {
        auto contentSize = list->preferredContentSize(context);
        auto vertMargin = context.theme.calcPreferredMenuVerticalMargin();
        list->setFrame(Rect(PicaPt::kZero, vertMargin, contentSize.width, contentSize.height));
        w.resize(Size(contentSize.width, contentSize.height + 2.0f * vertMargin));

        if (id != kInvalidId) {
            auto osf = w.osFrame();
            for (auto &kv : mImpl->id2item) {
                if (kv.first == id) {
                    auto p = kv.second.item->frame().upperLeft();
                    w.move(PicaPt::kZero, -p.y);
                }
            }
        }

        w.setFocusWidget(list);
    });

    list->setOnSelectionChanged([this](ListView *lv) {
        auto *mlv = (MenuListView*)lv;
        int idx = mlv->selectedIndex();  // lv/mlv will be going away
        if (idx >= 0 && size_t(idx) < mImpl->items.size() && mImpl->items[idx]->isClickable()
            && !mlv->isBlinking()) {

            mlv->blinkSelection(idx, [this, idx]() {
                auto *parent = mImpl->parent;
                // We do not want to call callback yet, as various operating
                // systems have different timing about when a redraw initiated by
                // setNeedsRedraw that the callback is sure to call. If the draw
                // happens immediately, then the window will not be closed,
                // which may cause problems (e.g. ComboBox on X11).
                parent->setPopupMenu(nullptr);
                mImpl->menuWindow->close();
                mImpl->isShowing = false;
                // do not clear listView here, it exists until onCloseCallback.

                if (idx >= 0 && idx < int(mImpl->items.size())) {
                    for (auto &kv : mImpl->id2item) {
                        if (kv.second.item == mImpl->items[idx]) {
                            if (kv.second.onSelected) {
                                kv.second.onSelected();
                            } else {
                                auto *mainWindow = Application::instance().activeWindow();
                                mainWindow->onMenuActivated(kv.first);
                            }
                            break;
                        }
                    }
                }

                if (mImpl->onCancelParentMenu) {
                    mImpl->onCancelParentMenu();
                }
            });
        } else {
            // We need to have submenus enabled, but that means clicking on them
            // is possible, and we don't want clicking to select them in the
            // list view, so undo it here.
            if (idx >= 0 && idx < int(mImpl->items.size())) {
                auto *submenu = mImpl->items[idx]->submenu();

                // If the selected item was a submenu, either the user clicked
                // on it or pressed Enter/Return/Space via keyboard navigation.
                // In either case, toggle the open-ness of the submenu.
                if (submenu && submenu->menuUitk()) {
                    if (submenu->menuUitk()->isShowing()) {
                        submenu->menuUitk()->cancel();
                    } else {
                        ((SubmenuItem*)mImpl->items[idx])->openSubmenu();
                        // The menu will open with nothing as the mouseover, so
                        // fake a keystroke to highlight the first one
                        KeyEvent ke{ KeyEvent::Type::kKeyDown, Key::kDown, 0, 0, false };
                        submenu->menuUitk()->mImpl->listView->key(ke);
                    }
                } else {
                    lv->clearSelection();
                }
            } else {
                lv->clearSelection();
            }
        }
    });

    mImpl->menuWindow->setOnWindowWillClose([this, list](Window &w) {
        // Remove all the items from popup menu, or they will get deleted,
        // which would be bad.
        list->clearSelection();
        list->removeAllChildren();

        // We want to reset all the item widget states to normal (or disabled).
        // We cannot set directly, a mouseExited event should work. Arguably that
        // is actually correct/necessary, since the window is gone.
        list->mouseExited();

        mImpl->isShowing = false;  // also here (as well as above), in case cancel() was called
        mImpl->listView = nullptr;
        mImpl->menuWindow->deleteLater();
        mImpl->menuWindow = nullptr;

        if (mImpl->onClose) {
            mImpl->onClose();
        }

        // Menu may have changed something, so redraw the parent
        if (mImpl->parent) {
            mImpl->parent->postRedraw();
        }
    });

    mImpl->menuWindow->setMouseGrab(list);

    mImpl->parent = w;
    mImpl->menuWindow->show(true);
    w->setPopupMenu(this);
}

void MenuUITK::cancel()
{
    if (mImpl->isShowing) {
        if (mImpl->menuWindow) {
            mImpl->menuWindow->close();
        } else {
            // An empty menu will get no window on show(), but we do want
            // the onClose to run.
            if (mImpl->onClose) {
                mImpl->onClose();
            }
            mImpl->isShowing = false;
        }
    }
    if (mImpl->parent) {
        mImpl->parent->setPopupMenu(nullptr);
    }
}

void MenuUITK::cancelHierarchy()
{
    bool wasShowing = mImpl->isShowing;
    cancel();
    if (wasShowing && mImpl->onCancelParentMenu) {
        mImpl->onCancelParentMenu();
    }
}

}  // namespace uitk
