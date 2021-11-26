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

#include "ComboBox.h"

#include "Events.h"
#include "Label.h"
#include "ListView.h"
#include "MenuUITK.h"
#include "ShortcutKey.h"
#include "UIContext.h"
#include "Window.h"
#include "themes/Theme.h"

namespace uitk {

struct ComboBox::Impl
{
    struct Item {
        int id;
        int value;
    };
    std::vector<Item> items;
    int selectedIndex = -1;
    MenuUITK *menu = nullptr; // we own this
    std::function<void(ComboBox*)> onSelectionChanged;
    PicaPt itemDrawOffset;

    int nextId = 1;
};

ComboBox::ComboBox()
    : mImpl(new Impl())
{
    mImpl->menu = new MenuUITK();
}

ComboBox::~ComboBox()
{
    mImpl->menu->cancel();  // in case menu is open
    delete mImpl->menu;
}

void ComboBox::clear()
{
    mImpl->menu->clear();
}

ComboBox* ComboBox::addItem(const std::string& text, int value /*= 0*/)
{
    auto id = mImpl->nextId++;
    int idx = int(mImpl->items.size());
    mImpl->items.push_back({id, value});
    mImpl->menu->addItem(text, id, [this, idx]() {
        setSelectedIndex(idx);
        if (mImpl->onSelectionChanged) {
            mImpl->onSelectionChanged(this);
        }
    });
    if (idx == 0) {
        setSelectedIndex(0);
    }
    return this;
}

ComboBox* ComboBox::addSeparator()
{
    mImpl->menu->addSeparator();
    return this;
}

const std::string& ComboBox::textAtIndex(int index) const
{
    static const std::string kBadIndexText = "";
    if (index >= 0 || index < int(mImpl->items.size())) {
        return mImpl->menu->itemText(mImpl->items[index].id);
    } else {
        return kBadIndexText;
    }
}

ComboBox* ComboBox::setTextAtIndex(int index, const std::string& text)
{
    if (index >= 0 || index < int(mImpl->items.size())) {
        mImpl->menu->setItemText(mImpl->items[index].value, text);
    }
    return this;
}

int ComboBox::valueAtIndex(int index) const
{
    if (index >= 0 || index < int(mImpl->items.size())) {
        return mImpl->items[index].value;
    } else {
        return 0;
    }
}

int ComboBox::selectedIndex() const
{
    return mImpl->selectedIndex;
}

ComboBox* ComboBox::setSelectedIndex(int index)
{
    int oldIdx = mImpl->selectedIndex;

    if (index >= 0 || index < int(mImpl->items.size())) {
        if (mImpl->menu->isSeparator(mImpl->items[index].id)) {
            return this;
        }
    }

    if (oldIdx >= 0 && oldIdx < int(mImpl->items.size())) {
        mImpl->menu->setItemChecked(mImpl->items[oldIdx].id, false);
    }

    mImpl->selectedIndex = index;  // could be -1
    setNeedsDraw();

    if (index >= 0 || index < int(mImpl->items.size())) {
        mImpl->menu->setItemChecked(mImpl->items[index].id, true);
    }

    return this;
}

ComboBox* ComboBox::setSelectedValue(int value)
{
    for (size_t i = 0;  i < mImpl->items.size();  ++i) {
        if (mImpl->items[i].value == value) {
            return setSelectedIndex(i);
        }
    }
    return this;
}

ComboBox* ComboBox::setSelectedText(const std::string& text)
{
    for (size_t i = 0;  i < mImpl->items.size();  ++i) {
        if (mImpl->menu->itemText(mImpl->items[i].id) == text) {
            return setSelectedIndex(i);
        }
    }
    return this;
}

ComboBox* ComboBox::setOnSelectionChanged(std::function<void(ComboBox*)> onChanged)
{
    mImpl->onSelectionChanged = onChanged;
    return this;
}

// We don't want grabbing because we are going to open a popup window.
// But it is not right to return kIgnored for the mouse click, either.
bool ComboBox::shouldAutoGrab() const { return false; }

Size ComboBox::preferredSize(const LayoutContext& context) const
{
    auto menuPref = mImpl->menu->preferredSize(context);
    return context.theme.calcPreferredComboBoxSize(context.dc, menuPref.width);
}

void ComboBox::layout(const LayoutContext& context)
{
    auto xMargin = context.theme.calcPreferredTextMargins(context.dc,
                                                          context.theme.params().labelFont).width;
    Rect textRectMenuCoord;
    context.theme.calcMenuItemFrames(context.dc, bounds(), PicaPt::kZero, nullptr, &textRectMenuCoord,
                                     nullptr);
    mImpl->itemDrawOffset = textRectMenuCoord.x - xMargin;

    Super::layout(context);
}

Widget::EventResult ComboBox::mouse(const MouseEvent& e)
{
    switch (e.type) {
        case MouseEvent::Type::kButtonDown: {
            // Don't Super::mouse() here, because we do not want to be set as the grab widget,
            // since we are opening a popup menu.

            int id = OSMenu::kInvalidId;
            if (mImpl->selectedIndex >= 0) {
                id = mImpl->items[mImpl->selectedIndex].id;
            }

            // The menu has a checkmark next to the currently selected item, and since
            // we offset the menu item when drawing the selected item in the combobox,
            // we also need to offset the menu similarly. This is Mac behavior.
            auto menuUL = frame().upperLeft();
            menuUL.x -= mImpl->itemDrawOffset;
            mImpl->menu->show(window(), convertToWindowFromLocal(menuUL), id);
#ifdef __APPLE__
            // macOS draws the window border inside the window, instead of decorating
            // the exterior of the window like Win32 and Xlib. show() outsets for this,
            // but since we are aligned with the frame of the control, we need to undo that.
            if (auto *menuWin = mImpl->menu->window()) {
                auto border = menuWin->borderWidth();
                menuWin->move(border, border);
            }
#endif // __APPLE__

            return EventResult::kConsumed;
        }
        default:
            return Super::mouse(e);
    }
}

void ComboBox::draw(UIContext& context)
{
    context.dc.save();
    context.theme.drawComboBoxAndClip(context, bounds(), style(state()), state());
    if (mImpl->selectedIndex >= 0 && (!mImpl->menu || !mImpl->menu->isShowing())) {
        auto itemState = (state() == Theme::WidgetState::kDisabled
                          ? Theme::WidgetState::kDisabled
                          : Theme::WidgetState::kNormal);
        int id = mImpl->items[mImpl->selectedIndex].id;
        context.dc.translate(-mImpl->itemDrawOffset, PicaPt::kZero);
        mImpl->menu->drawItem(context, bounds(), id, itemState);
        context.dc.translate(mImpl->itemDrawOffset, PicaPt::kZero);
    }
    context.dc.restore();
    Super::draw(context);
}

}  // namespace uitk
