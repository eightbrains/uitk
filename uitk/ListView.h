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

#ifndef UITK_LIST_VIEW_H
#define UITK_LIST_VIEW_H

#include "CellWidget.h"
#include "ScrollView.h"

#include <functional>
#include <string>
#include <unordered_set>

namespace uitk {

/// This is the base class for ListView cells. This class should do two things:
/// 1) calling preferredSize() should be quick:  in particular, do not create
///    a Text object or call a text measurement function unless it is
///    absolutely necessary. Doing so will cause ListView::layout() to be slow
///    for large data sets. Use the font metrics instead, as those are cached
///    and are quick to access.
/// 2) implement setForegroundColorNoRedraw(), which is used to set the text
///    color when the item is highlighted. In themes with light backgrounds and
///    dark text (e.g. macOS light mode), highlighted items need to draw their
///    text in a different color.
using ListViewCell = CellWidget;

class ListView : public ScrollView
{
    using Super = ScrollView;
public:
    ListView();
    ~ListView();

    enum class SelectionMode { kNoItems, kSingleItem, kMultipleItems };
    SelectionMode selectionMode() const;
    ListView* setSelectionModel(SelectionMode mode);

    /// Returns the number of cells in the list view
    int size() const;

    /// Deletes all the cells
    void clearCells();
    /// Removes all the cells and returns ownership of all cells to the caller.
    /// This is useful if you need to reuse the cells.
    void removeAllCells();
    /// Adds cell and takes owernship of the pointer. Note that the background
    /// color of the cell should be transparent or the selection will not be
    /// visible.
    ListView* addCell(ListViewCell *cell);
    /// Convenience function for addCell(new Label(text)).
    ListView* addStringCell(const std::string& text);

    /// Returns the cell or nullptr if there is no cell at the index.
    /// ListView retains ownership to the pointer.
    ListViewCell* cellAtIndex(int index) const;

    /// Removes the cell and transfers ownership to the caller.
    /// If index is out of range, returns nullptr;
    ListViewCell* removeCellAtIndex(int index);

    /// Returns the selected index or -1 if there is none.
    /// Should only be used in single item mode. Use selectedIndices() for
    /// multiple item mode.
    int selectedIndex() const;
    /// Returns the selected indices, if any. Can be used in all selection modes.
    std::vector<int> selectedIndices() const;

    ListView* setOnSelectionChanged(std::function<void(ListView*)> onChanged);
    ListView* setOnSelectionDoubleClicked(std::function<void(ListView*, int)> onDblClicked);

    void clearSelection();
    void setSelectedIndex(int index);
    void setSelectedIndices(const std::unordered_set<int> indices);

    bool isRowVisible(int index) const;

    /// Scrolls so that the requested row is visible. Note that this will not
    /// work until layout() has been called, since it requires the correct
    /// frames of the cell.
    void scrollRowVisible(int index);
    void scrollRowVisibleAtTop(int index);
    void scrollRowVisibleAtBottom(int index);

    /// Returns true if keyboard navigation will wrap from beginning to end and
    /// vice-versa.
    bool keyNavigationWraps() const;
    /// Sets keyboard navigation to wrap (or not wrap) from beginning to end and
    /// vice-versa. Default is false.
    void setKeyNavigationWraps(bool wraps);

    Size contentPadding() const;
    /// Sets the padding between the edge of the ListView widget and the
    /// content.
    ListView* setContentPadding(const PicaPt& xPadding, const PicaPt& yPadding);

    Size preferredContentSize(const LayoutContext& context) const;

    AccessibilityInfo accessibilityInfo() override;
    Size preferredSize(const LayoutContext& context) const override;
    void layout(const LayoutContext& context) override;
    EventResult mouse(const MouseEvent& e) override;
    void mouseExited() override;
    bool acceptsKeyFocus() const override;
    EventResult key(const KeyEvent& e) override;
    void draw(UIContext& context) override;

protected:
    int calcRowIndex(const Point& p) const;  // p is ListView widget coords
    int highlightedIndex() const;
    void setHighlightedIndex(int idx);  // in case key movement needs it
    void triggerOnSelectionChanged();

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_LIST_VIEW_H

