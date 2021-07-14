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

#ifndef UITK_POPUP_MENU_H
#define UITK_POPUP_MENU_H

#include "Widget.h"

#include <functional>
#include <string>

namespace uitk {

class MenuItem : public Widget
{
    using Super = Widget;
public:
    virtual bool isSeparator() const = 0;

    virtual const std::string& text() const = 0;
    virtual void setText(const std::string& text) = 0;

    virtual bool checked() const = 0;
    virtual void setChecked(bool checked) = 0;
};

class PopupMenu
{
public:
    static constexpr int kInvalidId = 0xffffff;

    PopupMenu();
    ~PopupMenu();

    void clear();
    PopupMenu* addItem(const std::string& text, int id, std::function<void()> onItem);
    /// Takes ownership of item.
    PopupMenu* addItem(MenuItem *item, int id, std::function<void()> onItem);
    PopupMenu* addSeparator(int id = kInvalidId);

    PopupMenu* insertItem(int index, const std::string& text, int id, std::function<void()> onItem);
    /// Takes ownership of item.
    PopupMenu* insertItem(int index, MenuItem *item, int id, std::function<void()> onItem);
    PopupMenu* insertSeparator(int index, int id = kInvalidId);

    void removeItem(int id);

    bool isSeparator(int id) const;

    bool itemChecked(int id) const;
    PopupMenu* setItemChecked(int id, bool checked);

    bool itemEnabled(int id) const;
    PopupMenu* setItemEnabled(int id, bool enabled);

    /// Returns the text of the item with the requested index, or "" if the
    /// index is invalid.
    const std::string& itemText(int id) const;
    PopupMenu* setItemText(int id, const std::string& text);

    Size preferredSize(const LayoutContext& context) const;

    bool isShowing() const;
    void show(Window *w, const Point& upperLeft, int id = kInvalidId);
    void cancel();

    /// Draws the item with ID with the upper left at (0, 0).
    /// This is mostly internal, used by ComboBox.
    void drawItem(UIContext& context, const Rect& frame, int id, Theme::WidgetState itemState);

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_POPUP_MENU_H



