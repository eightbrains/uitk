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

#include "MenubarUITK.h"

#include "Application.h"
#include "Events.h"
#include "Menu.h"
#include "MenuUITK.h"
#include "UIContext.h"
#include "Widget.h"
#include "Window.h"
#include "private/Utils.h"
#include "themes/Theme.h"

#include <chrono>
#include <map>
#include <vector>

namespace uitk {

//-----------------------------------------------------------------------------
namespace {

struct MenubarModel
{
    static constexpr int kNoActiveMenu = -1;

    struct MenubarItem
    {
        Menu *menu;
        std::string name;
    };
    std::vector<MenubarItem> menus;
    int activeIndex = kNoActiveMenu;
    int transientShortcutActivatedIndex = kNoActiveMenu;
    int justClosedIndex = kNoActiveMenu;
    std::chrono::steady_clock::time_point transientStartTime;

    bool isActive() const { return (this->activeIndex >= 0); }
};

} // namespace

//-----------------------------------------------------------------------------
class MenubarWidget : public Widget
{
    using Super = Widget;
public:
    explicit MenubarWidget(MenubarModel& modelRef)
        : mModel(modelRef)
    {
    }

    ~MenubarWidget() {}

    Size preferredSize(const LayoutContext& context) const override
    {
        if (mModel.menus.empty()) {
            return Size(PicaPt::kZero, PicaPt::kZero);
        }
        auto font = context.theme.params().nonNativeMenubarFont;
        auto h = context.theme.calcPreferredButtonSize(context.dc, font, "Ag").height;
        return Size(Widget::kDimGrow, h);
    }

    void layout(const LayoutContext& context) override
    {
        mMargin = context.theme.calcPreferredMenubarItemHorizMargin(context.dc, frame().height);
        if (mTheme != &context.theme) {
            mTextWidthCache.clear();
        }
        mTheme = &context.theme;
        // Note that we cannot cache the DrawContext, because on some platforms
        // (macOS) the context is only valid during the draw.
    }

    // It would be convenient to use a Button for the menus, but then we
    // would need to either always be verifying if anything changed with
    // the menus, or Menubar would need to know all the instances of
    // MenubarWidget and update them when something changes.

    virtual EventResult mouse(const MouseEvent& e) override
    {
        auto result = Super::mouse(e);

        // This is more complicated than you'd expect, because on platforms
        // that use a separate window for the menu, the menubar will may not
        // receive a click if the user clicks on the menubar while the menu
        // is active (to either toggle the menu off or select a different menu).
        // So Menubar::addMenu() adds an onClosed callback to the menu to
        // set mModel.justClosedIndex, so we can tell what had the menu open.
        // But if the user clicked on the menubar, we will get a mouse up message.
        // Note that it is important to clear justClosedIndex on any mouse action.
        // Test:
        // - click on menu and then click on it again: menu should close and
        //   menubar item unhighlight.
        // - click on menu, then click on a different menu in the menubar: menu
        //   should close and the new menu highlight and open.
        // - click on menu, then click in window to dismiss, then click on
        //   a menu. Menu should dismiss when clicked in window, menubar item
        //   should unhighlight, and then appropriate menu should open when clicked.
        // - click on menu, then select menu entry, then click on a menu. Should
        //   be same as previous.
        bool changed = false;
        const bool isClick = (e.type == MouseEvent::Type::kButtonDown);
        const bool isMove = (e.type == MouseEvent::Type::kDrag || e.type == MouseEvent::Type::kMove);
        const bool isMoveWithOpenMenu = (isMove && mModel.activeIndex >= 0);
        if (isClick || isMoveWithOpenMenu) {
            auto h = bounds().height;
            PicaPt x = PicaPt::kZero;
            for (size_t i = 0;  i < mModel.menus.size();  ++i) {
                auto &item = mModel.menus[i];
                auto w = itemWidth(nullptr, item.name);
                if (e.pos.y <= h && e.pos.x >= x && e.pos.x <= x + w) {
                    auto *menuUitk = item.menu->menuUitk();
                    if (mModel.activeIndex == i) {
                        // Set justClosedIndex *before* cancel(), not after.
                        // Usually cancel() will request a close which will
                        // eventually call the onClose callback--after this
                        // function has completed, so that when it sets
                        // justClosedIndex, everything is fine. But in the case
                        // of an empty window, cancel() calls onClose
                        // immediately. setting justClosedIndex after the cancel
                        // is incorrect behavior, it just happens to usually
                        // work because of timing.
                        mModel.justClosedIndex = MenubarModel::kNoActiveMenu;
                        if (isClick) {
                            if (menuUitk) {
                                menuUitk->cancel();
                            }
                            mModel.activeIndex = MenubarModel::kNoActiveMenu;
                        }
                    } else if (mModel.justClosedIndex != i || isMoveWithOpenMenu) {
                        auto* win = window();
                        if (win->popupMenu()) {
                            win->popupMenu()->cancel();
                        }
                        mModel.activeIndex = int(i);
                        if (menuUitk) {
                            win->onMenuWillShow();
                            menuUitk->show(win, Point(frame().x + x, frame().maxY()),
                                           0, Window::Flags::kMenuEdges);
                        }
                        mModel.justClosedIndex = MenubarModel::kNoActiveMenu;
                    } else {
                        mModel.justClosedIndex = MenubarModel::kNoActiveMenu;
                    }
                    changed = true;
                    break;
                }
                x += w;
            }
        } else if (mModel.justClosedIndex >= 0) {
            mModel.justClosedIndex = MenubarModel::kNoActiveMenu;
            changed = true;
        }
        if (changed) {
            setNeedsDraw();
        }
        if (isClick || isMove) {
            result = EventResult::kConsumed;
        }
        return result;
    }

    virtual void draw(UIContext& context) override
    {
        auto r = bounds();
        context.theme.drawMenubarBackground(context, r);
        PicaPt x = PicaPt::kZero;
        for (size_t i = 0;  i < mModel.menus.size();  ++i) {
            auto &item = mModel.menus[i];
            auto w = itemWidth(&context.dc, item.name);
            auto itemState = ((i == mModel.activeIndex || i == mModel.transientShortcutActivatedIndex)
                              && context.isWindowActive
                                ? Theme::WidgetState::kMouseDown
                                : Theme::WidgetState::kNormal);
            context.theme.drawMenubarItem(context, Rect(x, PicaPt::kZero, w, r.height),
                                          item.name, itemState);
            x += w;
        }
        if (mModel.transientShortcutActivatedIndex != MenubarModel::kNoActiveMenu) {
            auto now = std::chrono::steady_clock::now();
            auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - mModel.transientStartTime).count();
            if (dt >= 67 /* ms */) {
                mModel.transientShortcutActivatedIndex = MenubarModel::kNoActiveMenu;
            }
            if (auto *win = window()) {
                Application::instance().scheduleLater(win, [this]() { setNeedsDraw(); });
            }
        }
    }

protected:
    MenubarModel& mModel;
    mutable std::map<std::string, PicaPt> mTextWidthCache;

    PicaPt mMargin;
    const Theme* mTheme = nullptr;

    PicaPt itemWidth(const DrawContext* dc, const std::string& name) const
    {
        auto it = mTextWidthCache.find(name);
        if (it == mTextWidthCache.end() && dc) {
            mTextWidthCache[name] = 2.0f * mMargin + dc->textMetrics(name.c_str(), mTheme->params().nonNativeMenubarFont).width;
            it = mTextWidthCache.find(name);
        }
        if (it == mTextWidthCache.end()) {
            return PicaPt::kZero;
        }
        return it->second;
    }
};

//-----------------------------------------------------------------------------
struct MenubarUITK::Impl
{
    bool isNative;
    MenubarModel model;
};

MenubarUITK::MenubarUITK()
    : mImpl(new Impl())
{
    mImpl->isNative = false;
}

std::unique_ptr<Widget> MenubarUITK::createWidget() const
{
    return std::make_unique<MenubarWidget>(mImpl->model);
}

MenubarUITK::~MenubarUITK()
{
    for (auto item : mImpl->model.menus) {
        delete item.menu;
    }
    mImpl->model.menus.clear();
}

Menu* MenubarUITK::newMenu(const std::string& name)
{
    auto menu = new Menu();
    addMenu(menu, name);
    return menu;
}

void MenubarUITK::addMenu(Menu* menu, const std::string& name)
{
    if (menu->menuUitk()) {
        menu->menuUitk()->setOnClose([this](){
            mImpl->model.justClosedIndex = mImpl->model.activeIndex;
            mImpl->model.activeIndex = MenubarModel::kNoActiveMenu;
        });
    }
    mImpl->model.menus.push_back({menu, removeMenuItemMnemonics(name)});
}

Menu* MenubarUITK::removeMenu(const std::string& name)
{
    auto it = mImpl->model.menus.begin();
    while (it != mImpl->model.menus.end()) {
        if (it->name == name) {
            auto *menu = it->menu;
            mImpl->model.menus.erase(it);
            return menu;
        }
        ++it;
    }
    return nullptr;
}

Menu* MenubarUITK::menu(const std::string& name) const
{
    auto it = mImpl->model.menus.begin();
    while (it != mImpl->model.menus.end()) {
        if (it->name == name) {
            return it->menu;
        }
        ++it;
    }
    return nullptr;
}

Menu* MenubarUITK::macosApplicationMenu() const
{
    return nullptr;
}

std::vector<Menu*> MenubarUITK::menus() const
{
    std::vector<Menu*> mm;
    mm.reserve(mImpl->model.menus.size());
    for (auto &item : mImpl->model.menus) {
        mm.push_back(item.menu);
    }
    return mm;
}

void MenubarUITK::activateItemId(MenuId itemId) const
{
    auto *win = Application::instance().activeWindow();
    if (win) {
        win->onMenuWillShow();  // so items get properly enabled/disabled and (un)checked

        for (size_t i = 0;  i < mImpl->model.menus.size();  ++i) {
            auto &item = mImpl->model.menus[i];
            if (item.menu->activateItem(itemId) == OSMenu::ItemFound::kYes) {
                mImpl->model.transientShortcutActivatedIndex = int(i);
                mImpl->model.transientStartTime = std::chrono::steady_clock::now();
                // caller needs to call setNeedsDisplay(), since we do not know
                // which one of our menubars was actually activated.
                break;
            }
        }
    }
}

}  // namespace uitk
