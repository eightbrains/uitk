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

#include "PopupMenu.h"

#include "Label.h"
#include "ListView.h"
#include "UIContext.h"
#include "Window.h"
#include "themes/Theme.h"

#include <functional>
#include <string>
#include <unordered_map>

namespace uitk {

class StringMenuItem : public MenuItem
{
    using Super = MenuItem;
public:
    explicit StringMenuItem(const std::string& text)
    {
        mText = text;
    }

    ~StringMenuItem()
    {
    }

    bool isSeparator() const override { return false; }

    const std::string& text() const override { return mText; }
    void setText(const std::string& text) override { mText = text; }

    bool checked() const override { return mChecked; }
    void setChecked(bool checked) override { mChecked = checked; }

    Size preferredSize(const LayoutContext& context) const override
    {
        return context.theme.calcPreferredMenuItemSize(context.dc, mText);
    }

    void draw(UIContext& context) override
    {
        context.theme.drawMenuItem(context, bounds(), mText, mChecked, style(state()), state());
    }

private:
    std::string mText;
    bool mChecked = false;
};

class SeparatorMenuItem : public MenuItem
{
public:
    bool isSeparator() const override { return true; }

    const std::string& text() const override {
        static const std::string kNoText = "";
        return kNoText;
    }
    void setText(const std::string& text) override {}

    bool checked() const override { return false; }
    void setChecked(bool) override {}

    Size preferredSize(const LayoutContext& context) const override
    {
        return context.theme.calcPreferredMenuItemSize(context.dc, "Ag");
    }

    void draw(UIContext& context) override
    {
        context.theme.drawMenuSeparatorItem(context, bounds());
    }
};
//-----------------------------------------------------------------------------
class PopupMenuContent : public Widget
{
    using Super = Widget;
public:
    PopupMenuContent(ListView *list)
    {
        mList = list;
        addChild(mList);
    }

    void layout(const LayoutContext& context)
    {
        Super::layout(context);
    }

private:
    ListView *mList;
};

//-----------------------------------------------------------------------------
struct PopupMenu::Impl
{
    struct ItemData {
        MenuItem* item;  // this acts as a reference (but we can't use a reference b/c no copy constructor)
        std::function<void()> onSelected;
    };

    std::vector<MenuItem*> items;  // we own these
    std::unordered_map<int, ItemData> id2item;
    Window *menuWindow = nullptr;  // we own this
    Window *parent = nullptr;  // we don't own this

    MenuItem* itemForId(int id)
    {
        auto it = this->id2item.find(id);
        if (it != this->id2item.end()) {
            return it->second.item;
        }
        return nullptr;
    }
};

PopupMenu::PopupMenu()
    : mImpl(new Impl())
{
}

PopupMenu::~PopupMenu()
{
    for (auto *item : mImpl->items) {
        delete item;
    }
    delete mImpl->menuWindow;
}

void PopupMenu::clear()
{
    mImpl->items.clear();
    mImpl->id2item.clear();
}

PopupMenu* PopupMenu::addItem(const std::string& text, int id, std::function<void()> onItem)
{
    addItem(new StringMenuItem(text), id, onItem);
    return this;
}

PopupMenu* PopupMenu::addSeparator(int id /*= -1*/)
{
    addItem(new SeparatorMenuItem(), id, nullptr);
    return this;
}

PopupMenu* PopupMenu::addItem(MenuItem *item, int id, std::function<void()> onItem)
{
    mImpl->id2item[id] = Impl::ItemData{item, onItem};
    mImpl->items.push_back(item);
    return this;
}

PopupMenu* PopupMenu::insertItem(int index, const std::string& text, int id, std::function<void()> onItem)
{
    insertItem(index, new StringMenuItem(text), id, onItem);
    return this;
}

PopupMenu* PopupMenu::insertSeparator(int index, int id /*= -1*/)
{
    insertItem(index, new SeparatorMenuItem(), id, nullptr);
    return this;
}

PopupMenu* PopupMenu::insertItem(int index, MenuItem *item, int id, std::function<void()> onItem)
{
    mImpl->id2item[id] = Impl::ItemData{item, onItem};
    mImpl->items.insert(mImpl->items.begin() + index, item);
    return this;
}

void PopupMenu::removeItem(int id)
{
    auto it = mImpl->id2item.find(id);
    if (it != mImpl->id2item.end()) {
        for (auto itemsIt = mImpl->items.begin();  itemsIt != mImpl->items.end();  ++itemsIt) {
            if (it->second.item == *itemsIt) {
                mImpl->items.erase(itemsIt);
                mImpl->id2item.erase(it);
                return;
            }
        }
    }
}

bool PopupMenu::isSeparator(int id) const
{
    if (auto item = mImpl->itemForId(id)) {
        return item->isSeparator();
    }
    return false;
}

bool PopupMenu::itemChecked(int id) const
{
    if (auto item = mImpl->itemForId(id)) {
        return item->checked();
    }
    return false;
}

PopupMenu* PopupMenu::setItemChecked(int id, bool checked)
{
    if (auto item = mImpl->itemForId(id)) {
        item->setChecked(checked);
    }
    return this;
}

bool PopupMenu::itemEnabled(int id) const
{
    if (auto item = mImpl->itemForId(id)) {
        return item->enabled();
    }
    return false;
}

PopupMenu* PopupMenu::setItemEnabled(int id, bool enabled)
{
    if (auto item = mImpl->itemForId(id)) {
        item->setEnabled(enabled);
    }
    return this;
}

const std::string& PopupMenu::itemText(int id) const
{
    static const std::string kNoItemText = "";

    if (auto item = mImpl->itemForId(id)) {
        return item->text();
    }
    return kNoItemText;
}

PopupMenu* PopupMenu::setItemText(int id, const std::string& text)
{
    if (auto item = mImpl->itemForId(id)) {
        item->setText(text);
    }
    return this;
}

Size PopupMenu::preferredSize(const LayoutContext& context) const
{
    Size pref(PicaPt::kZero, PicaPt::kZero);
    for (auto item : mImpl->items) {
        auto itemPref = item->preferredSize(context);
        pref.width = std::max(pref.width, itemPref.width);
        pref.height += itemPref.height;
    }
    return pref;
}

void PopupMenu::drawItem(UIContext& context, const Rect& frame, int id, Theme::WidgetState itemState)
{
    auto *item = mImpl->itemForId(id);
    if (item) {
        if (item->frame().isEmpty()) {
            item->setFrame(frame);
        }
        item->draw(context);
    }
}

Window* PopupMenu::window() { return mImpl->menuWindow; }

bool PopupMenu::isShowing() const
{
    return (mImpl->menuWindow != nullptr);
}

void PopupMenu::show(Window *w, const Point& upperLeftWindowCoord, int id /*= kInvalidId*/)
{
    if (mImpl->menuWindow) {  // shouldn't happen, but handle it if it does
        cancel();
    }

    // Should we keep another mapping from id -> index? Seems unnecessary
    // since we are only going to use it in this function. This will be O(n), but
    // presumably menus are going to be reasonably sized.

    auto osUL = w->convertWindowToOSPoint(upperLeftWindowCoord);
    mImpl->menuWindow = new Window("", osUL.x, osUL.y, 0, 0, Window::Flags::kPopup);
#if __APPLE__
    // This is a hack
    auto popupBorder = mImpl->menuWindow->borderWidth();
    mImpl->menuWindow->move(PicaPt::kZero, -popupBorder);
#endif // __APPLE__
    mImpl->menuWindow->setOnWindowDidDeactivate([this](Window &w) {
        cancel();
    });

    auto *list = new ListView();  // will be owned by menuWindow
    list->setBorderWidth(PicaPt::kZero);
    list->setContentPadding(PicaPt::kZero, PicaPt::kZero);
    for (auto *item : mImpl->items) {
        list->addCell(item);
    }
    mImpl->menuWindow->addChild(list);

    mImpl->menuWindow->setOnWindowWillShow([this, list, id](Window& w, const LayoutContext& context) {
        auto contentSize = list->preferredContentSize(context);
        list->setFrame(Rect(PicaPt::kZero, PicaPt::kZero, contentSize.width, contentSize.height));
        w.resize(contentSize);

        if (id != kInvalidId) {
            auto osf = w.osFrame();
            for (auto &kv : mImpl->id2item) {
                if (kv.first == id) {
                    auto p = kv.second.item->frame().upperLeft();
                    w.move(PicaPt::kZero, -p.y);
                }
            }
        }
    });

    list->setOnSelectionChanged([this](ListView *lv) {
        int idx = lv->selectedIndex();  // lv will be going away
        // We do not want to call callback yet, as various operating systems
        // have different timing about when a redraw initiated by
        // setNeedsRedraw that the callback is sure to call. If the draw happens
        // immediately, then the window will not be closed, which may cause
        // problems (e.g. ComboBox on X11).
        mImpl->parent->setPopupMenu(nullptr);
        mImpl->menuWindow->close();

        if (idx >= 0 && idx < int(mImpl->items.size())) {
            for (auto &kv : mImpl->id2item) {
                if (kv.second.item == mImpl->items[idx]) {
                    if (kv.second.onSelected) {
                        kv.second.onSelected();
                    }
                    break;
                }
            }
        }
    });

    mImpl->menuWindow->setOnWindowWillClose([this, list](Window &w) {
        // Remove all the items from popup menu, or they will get deleted,
        // which would be bad.
        list->removeAllChildren();

        // We want to reset all the item widget states to normal (or disabled).
        // We cannot set directly, a mouseExited event should work. Arguably that
        // is actually correct/necessary, since the window is gone.
        for (auto *item : mImpl->items) {
            item->mouseExited();
        }
        
        mImpl->menuWindow->deleteLater();
        mImpl->menuWindow = nullptr;
    });

    mImpl->menuWindow->setMouseGrab(list);

    mImpl->parent = w;
    mImpl->menuWindow->show(true);
    w->setPopupMenu(this);
}

void PopupMenu::cancel()
{
    if (mImpl->menuWindow) {
        mImpl->menuWindow->close();
    }
    if (mImpl->parent) {
        mImpl->parent->setPopupMenu(nullptr);
    }
}

}  // namespace uitk
