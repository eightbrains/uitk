//-----------------------------------------------------------------------------
// Copyright 2021 - 2022 Eight Brains Studios, LLC
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

#include "Window.h"

#include "Application.h"
#include "Cursor.h"
#include "Dialog.h"
#include "Events.h"
#include "OSMenubar.h"
#include "OSWindow.h"
#include "Menu.h"
#include "MenuUITK.h"
#include "MenubarUITK.h"
#include "UIContext.h"
#include "Widget.h"
#include "private/MenuIterator.h"
#include "themes/Theme.h"
#include "themes/EmpireTheme.h"

#if defined(__APPLE__)
#include "macos/MacOSApplication.h"
#include "macos/MacOSWindow.h"
#elif defined(_WIN32) || defined(_WIN64)  // _WIN32 covers everything except 64-bit ARM
#include "win32/Win32Window.h"
#else
#include "x11/X11Window.h"
#endif

#include <nativedraw.h>

#include <algorithm>
#include <unordered_map>

namespace uitk {

// Standard menu handlers
namespace {

std::vector<Window*> sortedWindowList();

void onMenuRaiseWindow(int n)
{
    auto windowList = sortedWindowList();
    if (int(windowList.size()) >= n) {  // n is [1, ...)
        windowList[n - 1]->raiseToTop();
    }
}

void addStandardMenuHandlers(Window &w)
{
    auto onAbout = [](){};
    auto onQuit = [](){ Application::instance().quit(); };
    auto onCut = [&w](){
        if (auto *focus = w.focusWidget()) {
            if (auto *copyable = focus->asCutPasteable()) {
                if (copyable->canCopyNow()) {
                    copyable->cutToClipboard();
                    focus->setNeedsDraw();
                }
            }
        }
    };
    auto onCopy = [&w](){
        if (auto *focus = w.focusWidget()) {
            if (auto *copyable = focus->asCutPasteable()) {
                if (copyable->canCopyNow()) {
                    copyable->copyToClipboard();
                }
            }
        }
    };
    auto onPaste = [&w](){
        if (auto *focus = w.focusWidget()) {
            if (auto *copyable = focus->asCutPasteable()) {
                copyable->pasteFromClipboard();
                focus->setNeedsDraw();
            }
        }
    };
    auto onUndo = [](){};
    auto onRedo = [](){};
    auto onPreferences = [](){};

    // We can set all the handlers; if they are not in the menu then their
    // identifiers will never get referenced.
    w.setOnMenuActivated(MenuId(OSMenubar::StandardItem::kAbout), onAbout);
    w.setOnMenuActivated(MenuId(OSMenubar::StandardItem::kQuit), onQuit);
    w.setOnMenuActivated(MenuId(OSMenubar::StandardItem::kCut), onCut);
    w.setOnMenuActivated(MenuId(OSMenubar::StandardItem::kCopy), onCopy);
    w.setOnMenuActivated(MenuId(OSMenubar::StandardItem::kPaste), onPaste);
    w.setOnMenuActivated(MenuId(OSMenubar::StandardItem::kUndo), onUndo);
    w.setOnMenuActivated(MenuId(OSMenubar::StandardItem::kRedo), onRedo);
    w.setOnMenuActivated(MenuId(OSMenubar::StandardItem::kPreferences), onPreferences);

    // We set all 10 callbacks for windows. Most of them will never be called
    // because those IDs will not even be in the menu, but this way we can
    // guarantee that if they *are* in the menu, it will work.
    w.setOnMenuActivated(MenuId(OSMenubar::StandardItem::kWindow1), [](){ onMenuRaiseWindow(1); });
    w.setOnMenuActivated(MenuId(OSMenubar::StandardItem::kWindow2), [](){ onMenuRaiseWindow(2); });
    w.setOnMenuActivated(MenuId(OSMenubar::StandardItem::kWindow3), [](){ onMenuRaiseWindow(3); });
    w.setOnMenuActivated(MenuId(OSMenubar::StandardItem::kWindow4), [](){ onMenuRaiseWindow(4); });
    w.setOnMenuActivated(MenuId(OSMenubar::StandardItem::kWindow5), [](){ onMenuRaiseWindow(5); });
    w.setOnMenuActivated(MenuId(OSMenubar::StandardItem::kWindow6), [](){ onMenuRaiseWindow(6); });
    w.setOnMenuActivated(MenuId(OSMenubar::StandardItem::kWindow7), [](){ onMenuRaiseWindow(7); });
    w.setOnMenuActivated(MenuId(OSMenubar::StandardItem::kWindow8), [](){ onMenuRaiseWindow(8); });
    w.setOnMenuActivated(MenuId(OSMenubar::StandardItem::kWindow9), [](){ onMenuRaiseWindow(9); });
    w.setOnMenuActivated(MenuId(OSMenubar::StandardItem::kWindow10), [](){ onMenuRaiseWindow(10); });

    auto onMacMinimize = [](){
        if (auto *w = Application::instance().activeWindow()) {
            w->toggleMinimize();
        }
    };
    auto onMacMaximize = [](){
        if (auto *w = Application::instance().activeWindow()) {
            w->toggleMaximize();
        }
    };
    w.setOnMenuActivated(MenuId(OSMenubar::StandardItem::kMacOSMinimize), onMacMinimize);
    w.setOnMenuActivated(MenuId(OSMenubar::StandardItem::kMacOSZoom), onMacMaximize);

#if defined(__APPLE__)
    // Note that we might not have an actual MacOSApplication: might be iOS,
    // might be a non-windowed app.
    auto onMacHideApp = [](){
        if (auto *macApp = dynamic_cast<MacOSApplication*>(&Application::instance().osApplication())) {
            macApp->hideApplication();
        }
    };
    auto onMacHideOtherApps = [](){
        if (auto *macApp = dynamic_cast<MacOSApplication*>(&Application::instance().osApplication())) {
            macApp->hideOtherApplications();
        }
    };
    auto onMacShowOtherApps = [](){
        if (auto *macApp = dynamic_cast<MacOSApplication*>(&Application::instance().osApplication())) {
            macApp->showOtherApplications();
        }

    };
    auto onMacAllToFront = [](){
        auto *w = Application::instance().activeWindow();
        for (auto *w : Application::instance().windows()) {
            w->raiseToTop();
        }
        if (w) {
            w->raiseToTop();
        }
    };

    w.setOnMenuActivated(MenuId(OSMenubar::StandardItem::kMacOSHideApp), onMacHideApp);
    w.setOnMenuActivated(MenuId(OSMenubar::StandardItem::kMacOSHideOtherApps), onMacHideOtherApps);
    w.setOnMenuActivated(MenuId(OSMenubar::StandardItem::kMacOSShowOtherApps), onMacShowOtherApps);
    w.setOnMenuActivated(MenuId(OSMenubar::StandardItem::kMacOSBringAllToFront), onMacAllToFront);
#endif // __APPLE__
}

// This needs to sort the windows consistently, so that window #3 is always #3.
// See more detailed comment in Window::onMenuWillShow().
std::vector<Window*> sortedWindowList()
{
    auto titleLess = [](Window* x, Window* y) -> bool {
        auto &xTitle = x->title();
        auto &yTitle = y->title();
        // Don't just use std::string::operator<, we need some way to keep
        // windows with identical names consistently sorted. The address is not
        // guaranteed to work, as a new window could show up at an earlier
        // address, but at least it will be consistent until afterwards.
        if (xTitle == yTitle) {
            return &xTitle < &yTitle;
        }
        return (xTitle < yTitle);
    };
    auto windows = Application::instance().windows();  // returns ref, so we need to copy
    std::sort(windows.begin(), windows.end(), titleLess);
    return windows;
}

void updateWindowList()
{
    // Find the Window menu. We do not know its name (it might be
    // internationalized), so search for evidence of the window list.
    Menu *windowMenu = nullptr;
    int startIdx = -1;
    auto menus = Application::instance().menubar().menus();  // returns vector, not vector&
    // Search from back, since the Window menu is usually the last or second to last
    for (auto it = menus.rbegin();  it != menus.rend();  ++it) {
        int nItems = (*it)->size();
        for (int idx = nItems - 1;  idx >= 0;  --idx) {
            auto id = (*it)->menuId(idx);
            if (id == MenuId(OSMenubar::StandardItem::kWindowList)
                || id == MenuId(OSMenubar::StandardItem::kWindow1)) {
                startIdx = idx;
                break;
            }
        }
        if (startIdx >= 0) {
            windowMenu = *it;
            break;
        }
    }

    // If we have a Window menu, update the window list.
    // On macOS this is alphabetized, Linux has no native menus so we might as
    // well use the macOS behavior there. Windows is unclear; I've always thought
    // it is either stacking order / most recently used, but not only is this
    // less usable, but it's not clear what a good way of associating the window
    // pointer with the menu id, as it has to be done for all windows,
    // so alphabetized is easier here.
    if (windowMenu) {
        for (int idx = windowMenu->size() - 1;  idx >= startIdx;  --idx) {
            windowMenu->removeItem(idx);
        }
        auto windows = sortedWindowList();  // copy
        for (int idx = 0;  idx < 10 && idx < int(windows.size());  ++idx) {
            auto* w = windows[idx];
            windowMenu->addItem(w->title(), MenuId(int(OSMenubar::StandardItem::kWindow1) + idx),
                                ShortcutKey::kNone);
        }
    }
}

void updateStandardItem(Window &w, MenuItem *item, MenuId activeWindowId)
{
    switch (item->id()) {
        case (MenuId)OSMenubar::StandardItem::kCopy:
            item->setEnabled(w.focusWidget() && w.focusWidget()->asCutPasteable() && w.focusWidget()->asCutPasteable()->canCopyNow());
            break;
        case (MenuId)OSMenubar::StandardItem::kCut:
            item->setEnabled(w.focusWidget() && w.focusWidget()->asCutPasteable() && w.focusWidget()->asCutPasteable()->canCopyNow());
            break;
        case (MenuId)OSMenubar::StandardItem::kPaste:
            // can always paste if a text item is focused
            item->setEnabled(w.focusWidget() && w.focusWidget()->asCutPasteable());
            break;
        case (MenuId)OSMenubar::StandardItem::kUndo:
            item->setEnabled(false);  // TODO: implement undo
            break;
        case (MenuId)OSMenubar::StandardItem::kRedo:
            item->setEnabled(false);  // TODO: implement redo
            break;
        case (MenuId)OSMenubar::StandardItem::kAbout:
            item->setEnabled(false);  // TODO: implement about dialog
            break;
        case (MenuId)OSMenubar::StandardItem::kPreferences:
            item->setEnabled(false);  // TODO: implement preferences dialog
            break;
        case (MenuId)OSMenubar::StandardItem::kWindow1:
        case (MenuId)OSMenubar::StandardItem::kWindow2:
        case (MenuId)OSMenubar::StandardItem::kWindow3:
        case (MenuId)OSMenubar::StandardItem::kWindow4:
        case (MenuId)OSMenubar::StandardItem::kWindow5:
        case (MenuId)OSMenubar::StandardItem::kWindow6:
        case (MenuId)OSMenubar::StandardItem::kWindow7:
        case (MenuId)OSMenubar::StandardItem::kWindow8:
        case (MenuId)OSMenubar::StandardItem::kWindow9:
        case (MenuId)OSMenubar::StandardItem::kWindow10:
            item->setChecked(item->id() == activeWindowId);
            break;
        case (MenuId)OSMenubar::StandardItem::kMacOSHideOtherApps:
#if defined(__APPLE__)
            if (auto *macApp = dynamic_cast<MacOSApplication*>(&Application::instance().osApplication())) {
                item->setEnabled(!macApp->isHidingOtherApplications());
            }
#endif // __APPLE__
            break;
        case (MenuId)OSMenubar::StandardItem::kMacOSShowOtherApps:
#if defined(__APPLE__)
            if (auto *macApp = dynamic_cast<MacOSApplication*>(&Application::instance().osApplication())) {
                item->setEnabled(macApp->isHidingOtherApplications());
            }
#endif // __APPLE__
            break;
        default:
            break;
    }
}

}  // namespace

//-----------------------------------------------------------------------------
enum class PopupState { kNone, kShowing, kCancelling };

struct Window::Impl
{
    std::shared_ptr<Theme> theme;
    std::unique_ptr<OSWindow> window;
    std::string title;
    std::vector<Cursor> cursorStack;
    int flags;
    std::unique_ptr<Widget> menubarWidget;
    std::unique_ptr<Widget> rootWidget;
    Widget *grabbedWidget = nullptr;
    Widget *focusedWidget = nullptr;
    MenuUITK *activePopup = nullptr;
    PopupState popupState = PopupState::kNone;
    struct {
        Dialog* dialog = nullptr;
        std::unique_ptr<Window> window;
    } dialog;
    std::function<void(MenuItem&)> onMenuItemNeedsUpdate;
    std::unordered_map<MenuId, std::function<void()>> onMenuActivatedCallbacks;
    std::function<void(Window&)> onWillShow;
    std::function<void(Window&, const LayoutContext&)> onLayout;
    std::function<void(Window&)> onDidDeactivate;
    std::function<bool(Window&)> onShouldClose;
    std::function<void(Window&)> onWillClose;
    bool isActive = false;
    bool inResize = false;
    bool inMouse = false;
    bool inKey = false;
    bool inDraw = false;
    bool needsDraw = false;
    bool needsLayout = false;

    void cancelPopup()
    {
        if (this->activePopup) {
            this->popupState = PopupState::kCancelling;
            this->activePopup->cancel();
            // These are redundant, since cancel() should call setPopupMenu(nullptr)
            this->activePopup = nullptr;
            this->popupState = PopupState::kNone;
        }
    }
};

Window::Window(const std::string& title, int width, int height,
               Flags::Value flags /*= Flags::kNone*/)
    : Window(title, -1, -1, width, height, flags)
{
}

Window::Window(const std::string& title, int x, int y, int width, int height,
               Flags::Value flags /*= Flags::kNone*/)
    : mImpl(new Impl())
{
    // Create theme before the window, in case a draw is requested immediately
    // on creation (as it is on Win32).
    mImpl->theme = Application::instance().theme();
    mImpl->rootWidget = std::make_unique<Widget>();
    mImpl->rootWidget->setWindow(this);
    mImpl->flags = flags;
    mImpl->title = title;

    auto &menubar = Application::instance().menubar();
    if (!(flags & Flags::kPopup) &&
        !(flags & Flags::kDialog) &&
        !Application::instance().supportsNativeMenus()) {
        if (auto* uitkMenubar = dynamic_cast<MenubarUITK*>(&Application::instance().menubar())) {
            mImpl->menubarWidget = uitkMenubar->createWidget();
            mImpl->menubarWidget->setWindow(this);
        }
    }

#if defined(__APPLE__)
    mImpl->window = std::make_unique<MacOSWindow>(*this, title, x, y, width, height, flags);
#elif defined(_WIN32) || defined(_WIN64)
    mImpl->window = std::make_unique<Win32Window>(*this, title, x, y, width, height, flags);
#else
    mImpl->window = std::make_unique<X11Window>(*this, title, x, y, width, height, flags);
#endif

    pushCursor(Cursor::arrow());
    addStandardMenuHandlers(*this);

    if (!(flags & Flags::kPopup) && !(flags & Flags::kDialog)) {
        Application::instance().addWindow(this);
        updateWindowList();
    }

    mImpl->needsLayout = true;
}

Window::~Window()
{
    Application::instance().removeWindow(this);
    updateWindowList();
    mImpl->cancelPopup();
    mImpl->theme.reset();
    mImpl->window.reset();
    mImpl->menubarWidget.reset();
    mImpl->rootWidget.reset();
}

void Window::deleteLater()
{
    Application::instance().scheduleLater(this, [w = this]() {
        delete w;
    });
}

OSWindow* Window::nativeWindow() { return mImpl->window.get(); }

void* Window::nativeHandle() { return mImpl->window->nativeHandle(); }

bool Window::isShowing() const { return mImpl->window->isShowing(); }

Window* Window::show(bool show)
{
    auto onWillShow = [this](const DrawContext& dc) {
        if (mImpl->onWillShow) {
            mImpl->onWillShow(*this);
        }
    };
    mImpl->window->show(show, onWillShow);
    return this;
}

void Window::toggleMinimize()
{
    mImpl->window->toggleMinimize();
}

void Window::toggleMaximize()
{
    mImpl->window->toggleMaximize();
}

bool Window::isActive() const { return mImpl->isActive; }

bool Window::close(CloseBehavior ask /*= CloseBehavior::kAllowCancel*/)
{
    if (ask == CloseBehavior::kAllowCancel && !onWindowShouldClose()) {
        return false;
    }
    if (mImpl->inDraw) {
        Application::instance().scheduleLater(this, [this](){
                close(CloseBehavior::kForceClose);
        });
    } else {
        mImpl->cancelPopup();
        mImpl->window->close();
    }
    return true;
}

const std::string& Window::title() const { return mImpl->title; }

Window* Window::setTitle(const std::string& title)
{
    mImpl->title = title;
    mImpl->window->setTitle(title);
    updateWindowList();
    return this;
}

void Window::setCursor(const Cursor& cursor)
{
    // Interestingly, the three major operating systems set the cursor
    // completely differently. MacOS sets it by application, Windows
    // sets it for just this instant and resets to the cursor in the
    // window class when the mouse moves (unless you intercept WM_SETCURSOR,
    // and X11 sets it on the window.
    mImpl->window->setCursor(cursor);
}

void Window::pushCursor(const Cursor& cursor)
{
    mImpl->cursorStack.push_back(cursor);
    mImpl->window->setCursor(cursor);
}

void Window::popCursor()
{
    mImpl->cursorStack.pop_back();
    assert(!mImpl->cursorStack.empty());  // should always have arrow on bottom from Window()
    if (mImpl->cursorStack.empty()) {
        mImpl->cursorStack.push_back(Cursor::arrow());
    }
    mImpl->window->setCursor(mImpl->cursorStack.back());
}

void Window::resize(const Size& contentSize)
{
    Size menuSize;
    if (mImpl->menubarWidget) {
        menuSize = mImpl->menubarWidget->frame().size();
    }
    Size newContentSize(contentSize.width, contentSize.height + menuSize.height);
    
    mImpl->window->setContentSize(contentSize);
}

void Window::resizeToFit()
{
    mImpl->window->callWithLayoutContext([this](const DrawContext& dc) {
        LayoutContext context{ *mImpl->theme, dc };
        Size size(PicaPt::kZero, PicaPt::kZero);
        for (auto *child : mImpl->rootWidget->children()) {
            auto pref = child->preferredSize(context);
            size.width = std::max(size.width, pref.width);
            size.height = std::max(size.height, pref.height);
        }
        resize(size);
    });
}

void Window::resizeToFit(std::function<Size(const LayoutContext&)> calcSizeFunc)
{
    mImpl->window->callWithLayoutContext([this, calcSizeFunc](const DrawContext& dc) {
        LayoutContext context{ *mImpl->theme, dc };
        auto size = calcSizeFunc(context);
        resize(size);
    });
}

void Window::move(const PicaPt& dx, const PicaPt& dy)
{
    auto dpi = mImpl->window->dpi();
    auto f = mImpl->window->osFrame();
    if (Application::instance().isOriginInUpperLeft()) {
        setOSFrame(f.x + dx.toPixels(dpi), f.y + dy.toPixels(dpi), f.width, f.height);
    } else {
        setOSFrame(f.x + dx.toPixels(dpi), f.y - dy.toPixels(dpi), f.width, f.height);
    }
}

OSWindow::OSRect Window::osFrame() const
{
    return mImpl->window->osFrame();
}

void Window::setOSFrame(float x, float y, float width, float height)
{
    mImpl->window->setOSFrame(x, y, width, height);
}

OSWindow::OSPoint Window::convertWindowToOSPoint(const Point& windowPt) const
{
    auto contentRect = mImpl->window->contentRect();
    auto osContentRect = mImpl->window->osContentRect();
    auto dpi = mImpl->window->dpi();
    if (Application::instance().isOriginInUpperLeft()) {
        return { osContentRect.x + (contentRect.x + windowPt.x).toPixels(dpi),
                 osContentRect.y + (contentRect.y + windowPt.y).toPixels(dpi) };
    } else {
        return { osContentRect.x + (contentRect.x + windowPt.x).toPixels(dpi),
                 osContentRect.y + (contentRect.maxY() - windowPt.y).toPixels(dpi) };
    }
}

void Window::setNeedsDraw()
{
    // You'd think that we would never call setNeedsDraw() while drawing.
    // If we do, though, do not create an actual expose event (especially if
    // it sends an actual message, like on X11), or we may draw continuously.
    // Note that Button sets the color of the text, and that calls
    // setNeedsDraw().
    if (mImpl->inMouse || mImpl->inKey || mImpl->inResize || mImpl->inDraw) {
        mImpl->needsDraw = true;
    } else {
        postRedraw();
    }
}

void Window::setNeedsLayout()
{
    mImpl->needsLayout = true;
    setNeedsDraw();
}

void Window::postRedraw() const
{
    // We sometimes get deactivate messages from dialogs whose window has been
    // destroyed. The deactivate is reasonable, but we do not want post the
    // redraw!
    if (mImpl->window != NULL) {
        mImpl->window->postRedraw();
    }
}

void Window::raiseToTop() const { mImpl->window->raiseToTop(); }

const Rect& Window::contentRect() const { return mImpl->rootWidget->frame(); }

Widget* Window::addChild(Widget *child)
{
    mImpl->rootWidget->addChild(child);
    setNeedsDraw();
    return child;
}

Widget* Window::removeChild(Widget *child)
{
    mImpl->rootWidget->removeChild(child);
    setNeedsDraw();
    return child;
}

void Window::setOnMenuItemNeedsUpdate(std::function<void(MenuItem&)> onNeedsUpdate)
{
    mImpl->onMenuItemNeedsUpdate = onNeedsUpdate;
}

void Window::setOnMenuActivated(MenuId id, std::function<void()> onActivated)
{
    mImpl->onMenuActivatedCallbacks[id] = onActivated;
}

void Window::setOnWindowWillShow(std::function<void(Window&)> onWillShow)
{
    mImpl->onWillShow = onWillShow;
}

void Window::setOnWindowLayout(std::function<void(Window&, const LayoutContext&)> onLayout)
{
    mImpl->onLayout = onLayout;
}

void Window::setOnWindowDidDeactivate(std::function<void(Window&)> onDidDeactivate)
{
    mImpl->onDidDeactivate = onDidDeactivate;
}

void Window::setOnWindowShouldClose(std::function<bool(Window&)> onShouldClose)
{
    mImpl->onShouldClose = onShouldClose;
}

void Window::setOnWindowWillClose(std::function<void(Window&)> onWillClose)
{
    mImpl->onWillClose = onWillClose;
}

void Window::setMouseGrab(Widget *w)
{
    mImpl->grabbedWidget = w;
    if (mImpl->focusedWidget != w) {
        setFocusWidget(nullptr);
    }
}

Widget* Window::mouseGrabWidget() const { return mImpl->grabbedWidget; }

void Window::setFocusWidget(Widget *w)
{
    auto *oldFocusedWidget = mImpl->focusedWidget;
    bool isDifferent = (w != oldFocusedWidget);

    mImpl->focusedWidget = w;
    if (w) {
        auto origin = w->convertToWindowFromLocal(w->bounds().upperLeft());
        mImpl->window->setTextEditing(w->asTextEditorLogic(),
                                      Rect(origin.x, origin.y, w->bounds().width, w->bounds().height));
    } else {
        mImpl->window->setTextEditing(nullptr, Rect::kZero);
    }

    // Call keyFocusEnded after setting, to avoid an infinite loop
    // in case oldFocusedWidget calls resignKeyFocus().
    if (oldFocusedWidget && isDifferent) {
        oldFocusedWidget->keyFocusEnded();
    }

    if (isDifferent && w && mImpl->focusedWidget) {
        mImpl->focusedWidget->keyFocusStarted();
    }
}

Widget* Window::focusWidget() const { return mImpl->focusedWidget; }

PicaPt Window::borderWidth() const { return mImpl->window->borderWidth(); }

MenuUITK* Window::popupMenu() const { return mImpl->activePopup; }

void Window::setPopupMenu(MenuUITK *menu)
{
    // We clicked on the widget, so it will grab the mouse, but the unclick will
    // go to the menu, so it maintains the grab, which is obviously unwanted.
    setMouseGrab(nullptr);

    if (mImpl->activePopup != nullptr && menu == mImpl->activePopup
        && mImpl->popupState == PopupState::kShowing) {
        // Removing popup: need to call activate, in case mouse is over a widget
        // Q: How do we know this window is actually active?
        // A: Because if it weren't, onDeactivate would be called, cancelling the popup
        onActivated(mImpl->window->currentMouseLocation());
    }

    mImpl->activePopup = menu;

    if (menu) {
        mImpl->popupState = PopupState::kShowing;
        if (auto *w = menu->window()) {
            w->onActivated(w->mImpl->window->currentMouseLocation());
        }
    } else {
        mImpl->popupState = PopupState::kNone;
        if (mImpl->menubarWidget && !mImpl->menubarWidget->frame().isEmpty()) {
            setNeedsDraw();
        }
    }
}

bool Window::beginModalDialog(Dialog *d)
{
    if (mImpl->dialog.dialog) {
        return false;
    }

    mImpl->dialog.dialog = d;
    mImpl->dialog.window.reset(new Window(d->title(), 10, 10, Flags::kDialog));
    mImpl->dialog.window->addChild(d);
    mImpl->dialog.window->setOnWindowShouldClose([this](Window&) {
        Application::instance().scheduleLater(mImpl->dialog.window.get(), [this]() {
            mImpl->dialog.dialog->cancel();
        });
        return false;
    });
    mImpl->dialog.window->resizeToFit();
    mImpl->window->beginModalDialog(mImpl->dialog.window->nativeWindow());

    return true;
}

// Ends display of the dialog and returns ownership to the caller.
Dialog* Window::endModalDialog()
{
    auto *dialog = mImpl->dialog.dialog;
    mImpl->dialog.window->removeChild(dialog);
    mImpl->dialog.dialog = nullptr;
    // Makes sure we call this AFTER dialog.dialog is nulled, so that we can
    // tell between an onActivate() from someone trying to click away from the
    // dialog and the dialog is closing (but has not quite finished, e.g. on
    // Windows) and so this window is actually activating.
    mImpl->window->endModalDialog(mImpl->dialog.window->nativeWindow());
    // We are probably in an event, most likely a button event, so we cannot
    // delete the dialog window now.
    mImpl->dialog.window->deleteLater();
    mImpl->dialog.window.release();

    return dialog;
}

void Window::onMouse(const MouseEvent& eOrig)
{
    // macOS and Windows do not send events to a window under a dialog, but
    // X11 does.
    if (mImpl->dialog.dialog || mImpl->dialog.window) {
        return;
    }

    if (mImpl->activePopup) {
        if (eOrig.type == MouseEvent::Type::kButtonDown) {
            mImpl->cancelPopup();
        }
        // Some systems (at least macOS) send mouse events outside the window to
        // the parent window of a borderless window. If this happens, convert
        // move/drag events to the popup and send them on. Unless we are in the
        // menubar, in which case we need to pass the events in case the user
        // mouses over a different menu and we need to change the open menu.
        bool isMouseMoveOverMenubar = false;
        if (eOrig.type == MouseEvent::Type::kMove || eOrig.type == MouseEvent::Type::kDrag) {
            isMouseMoveOverMenubar = (mImpl->menubarWidget
                                      && mImpl->menubarWidget->frame().contains(eOrig.pos));
            auto *w = mImpl->activePopup->window();
            if (w) {
                auto thisWindowULinRoot = mImpl->rootWidget->frame().upperLeft();
                thisWindowULinRoot.y = -thisWindowULinRoot.y;
                auto thisWindowUL = convertWindowToOSPoint(mImpl->rootWidget->convertToWindowFromLocal(thisWindowULinRoot));
                auto popupUL = w->convertWindowToOSPoint(Point::kZero);
                auto dpi = mImpl->window->dpi();
                MouseEvent ePopup = eOrig;  // copy
                ePopup.pos.x -= PicaPt::fromPixels(popupUL.x - thisWindowUL.x, dpi);
                if (Application::instance().isOriginInUpperLeft()) {
                    ePopup.pos.y -= PicaPt::fromPixels(popupUL.y - thisWindowUL.y, dpi);
                } else {
                    ePopup.pos.y -= -PicaPt::fromPixels(popupUL.y - thisWindowUL.y, dpi);
                }
                w->onMouse(ePopup);
            }
        }
        // If we are a normal window we should not get the mouse event,
        // but if we are a menu displaying we should also get the event
        // so that we show/hide submenus, etc.
        if (!(mImpl->flags & Window::Flags::kPopup) && !isMouseMoveOverMenubar) {
            return;
        }
    }

    mImpl->inMouse = true;

    MouseEvent e = eOrig;  // copy
    e.pos.y = eOrig.pos.y - mImpl->rootWidget->frame().y;

#if !defined(__APPLE__)
    // X11 and Win32 treat scroll as lines, not pixels like macOS.
    // The event loop in X11 does not have access to the theme, so we need
    // to convert from lines to pixels here.
    if (e.type == MouseEvent::Type::kScroll) {
        e.scroll.dx *= 3.0f * mImpl->theme->params().labelFont.pointSize();
        e.scroll.dy *= 3.0f * mImpl->theme->params().labelFont.pointSize();
    }
#endif // !__APPLE__

    if (mImpl->grabbedWidget == nullptr) {
        if (mImpl->menubarWidget && mImpl->menubarWidget->frame().contains(eOrig.pos)) {
            mImpl->rootWidget->setState(Widget::MouseState::kNormal);
            mImpl->menubarWidget->setState(Widget::MouseState::kMouseOver);
            e.pos.y = eOrig.pos.y - mImpl->menubarWidget->frame().y;
            mImpl->menubarWidget->mouse(e);
        } else {
            if (mImpl->menubarWidget) {
                mImpl->menubarWidget->setState(Widget::MouseState::kNormal);
            }
            mImpl->rootWidget->setState(Widget::MouseState::kMouseOver);  // so onDeactivated works
            mImpl->rootWidget->mouse(e);
        }
    } else {
        auto *grabbed = mImpl->grabbedWidget;
        auto grabE = e;  // copy
        grabE.pos = grabbed->convertToLocalFromWindow(eOrig.pos);
        if (grabbed->bounds().contains(grabE.pos)) {
            // If we are inside the widget, we should send the event normally.
            // This handles two cases:
            //   1) the user dragged outside the widget and is now back in,
            //      so we need to update the highlighting,
            //   2) widgets like Button that use subwidgets. In Button's case,
            //      the text frame is the same size as the button, and the
            //      grab goes to the deepest. This way the Button will also
            //      get the event, not just the Label.
            mImpl->rootWidget->mouse(e);
        } else {
            // Send the event directly, unless it is a button up event.
            // Button up should be ignored outside the widget, because
            // otherwise every widget will need to remember to check if
            // the button up is inside the frame before assuming that this
            // is an actionable end-of-click.
            if (grabE.type != MouseEvent::Type::kButtonUp) {
                grabbed->mouse(grabE);
            } else {
                // Mouse up should be converted to a move and sent normally
                // so whatever it is over can handle that.
                // TODO: need to have e.button.buttons and check == 0
                auto moveE = e;
                moveE.type = MouseEvent::Type::kMove;
                mImpl->rootWidget->mouse(moveE);
            }

            // Handle the case where the mouse just left the frame
            auto currentState = grabbed->state();
            auto newState = Widget::MouseState::kNormal;
            if (newState != currentState) {
                Widget *w = grabbed;
                // Need to also set states of all the parents
                while (w) {
                    w->setState(newState);
                    w = w->parent();
                }
            }
        }
    }

    // TODO: need to have e.button.buttons and check == 0, otherwise grab will be
    // cancelled if one button is released, even though others are pressed
    //if (e.type == MouseEvent::Type::kButtonUp && e.button.buttons == 0) {
    if (e.type == MouseEvent::Type::kButtonUp && e.button.button == MouseButton::kLeft) {
        mImpl->grabbedWidget = nullptr;
    }

    mImpl->inMouse = false;
    if (mImpl->needsDraw) {
        postRedraw();
        mImpl->needsDraw = false;
    }
}

void Window::onKey(const KeyEvent &e)
{
    int menuId;
    if (!mImpl->dialog.dialog && e.type == KeyEvent::Type::kKeyDown && Application::instance().keyboardShortcuts().hasShortcut(e, &menuId)) {
        onMenuWillShow();  // make sure items are enabled/disabled for *this current* window
        Application::instance().menubar().activateItemId(menuId);
        // We need to flash the menu that got activated, but the menubar
        // does not know which menubar widget was actually activated, so we
        // need to call setNeedsDraw().
        if (mImpl->menubarWidget) {
            mImpl->menubarWidget->setNeedsDraw();
        }
        return;
    }

    // Key events may be sent to the main window, instead of the popup window,
    // in which case we need to forward the event on.
    if (mImpl->activePopup) {
        mImpl->activePopup->window()->onKey(e);
    } else {
        mImpl->inKey = true;
        // Send the key to the focused widget if there is one
        if (mImpl->focusedWidget) {
            mImpl->focusedWidget->key(e);
        // If no focused widget AND we are a dialog, send the key to the dialog
        // widget so that Esc, Enter, etc. can be handled.
        } else if (mImpl->flags & Flags::kDialog) {
            auto roots = mImpl->rootWidget->children();
            if (!roots.empty()) {
                roots[0]->key(e);
            }
        }
        mImpl->inKey = false;
        if (mImpl->needsDraw) {
            postRedraw();
            mImpl->needsDraw = false;
        }
    }
}

void Window::onText(const TextEvent& e)
{
    // Text events may be sent to the main window, instead of the popup window,
    // in which case we need to forward the event on.
    if (mImpl->activePopup) {
        mImpl->activePopup->window()->onText(e);
    } else {
        mImpl->inKey = true; // these are usually generated from key events
        if (mImpl->focusedWidget) {
            mImpl->focusedWidget->text(e);
        }
        mImpl->inKey = false;
        if (mImpl->needsDraw) {
            postRedraw();
            mImpl->needsDraw = false;
        }
    }
}

void Window::onResize(const DrawContext& dc)
{
    if (!mImpl->window) {
        return;
    }

    mImpl->inResize = true;
    onLayout(dc);
    mImpl->inResize = false;
}

void Window::onLayout(const DrawContext& dc)
{
    auto contentRect = mImpl->window->contentRect();
    LayoutContext context = { *mImpl->theme, dc };

    auto y = contentRect.y;
    auto menubarHeight = PicaPt::kZero;
    if (mImpl->menubarWidget) {
        menubarHeight = mImpl->menubarWidget->preferredSize(context).height;
        mImpl->menubarWidget->setFrame(Rect(contentRect.x, y, contentRect.width, menubarHeight));
        mImpl->menubarWidget->layout(context);
        y += menubarHeight;
    }

    assert(y == dc.roundToNearestPixel(y));
    mImpl->rootWidget->setFrame(Rect(contentRect.x, y,
                                     contentRect.width, contentRect.height - y));
    if (mImpl->onLayout) {
        mImpl->onLayout(*this, context);
    } else {
        auto &children = mImpl->rootWidget->children();
        for (auto *child : children) {
            child->setFrame(mImpl->rootWidget->bounds());
        }
    }
    mImpl->rootWidget->layout(context);
    mImpl->needsLayout = false;

    // Focus widget's frame may have changed; update so that IME position
    // will be correct.
    if (focusWidget()) {
        setFocusWidget(focusWidget());
    }
}

void Window::onDraw(DrawContext& dc)
{
    // It's not clear when to re-layout. We could send a user message for layout,
    // but it's still going to delay a draw (since it is all done by the same thread),
    // so it seems like it is simpler just to do it on a draw.
    if (mImpl->needsLayout) {
        onLayout(dc);
    }

    UIContext context { *mImpl->theme, dc, mImpl->isActive };
    auto size = Size(PicaPt::fromPixels(float(dc.width()), dc.dpi()),
                     PicaPt::fromPixels(float(dc.height()), dc.dpi()));
    auto rootUL = mImpl->rootWidget->frame().upperLeft();
    mImpl->inDraw = true;
    dc.beginDraw();
    if (mImpl->flags & Window::Flags::kPopup) {
        mImpl->theme->drawMenuBackground(context, size);
    } else {
        mImpl->theme->drawWindowBackground(context, size);
    }
    dc.translate(rootUL.x, rootUL.y);
    mImpl->rootWidget->draw(context);
    dc.translate(-rootUL.x, -rootUL.y);
    if (mImpl->menubarWidget) {
        mImpl->menubarWidget->draw(context);
    }
    dc.endDraw();
    mImpl->inDraw = false;

    mImpl->needsDraw = false;  // should be false anyway, just in case
}

void Window::onActivated(const Point& currentMousePos)
{
    // Some platforms, like Windows, do not allow a window as a dialog, so we
    // have to force the modality ourselves. Note that we need to check both
    // dialog.dialog and dialog.window, since we will get an onActivated() call
    // when the dialog's window is closing, but has not finished closing
    // (dialog.dialog == nullptr, dialog.window != nullptr). At least on
    // Windows, since there are some other messages that get sent before the
    // close message when DestroyWindow() is called.
    if (mImpl->dialog.dialog && mImpl->dialog.window) {
        mImpl->dialog.window->raiseToTop();
        Application::instance().beep();
        return;
    }

    mImpl->isActive = true;
    mImpl->cancelPopup();
    if (!(mImpl->flags & Flags::kPopup)) {
        Application::instance().setActiveWindow(this);
    }

    // If mouse is over the window when it is activated (especially by
    // Alt-Tab), send an artifical mouse move event, so that if the mouse
    // is over a control it will be properly highlighted.
    auto frame = mImpl->window->contentRect();
    if (frame.contains(currentMousePos)) {
        MouseEvent me;
        me.pos = currentMousePos;
        me.type = MouseEvent::Type::kMove;
        onMouse(me);
    }
}

void Window::onDeactivated()
{
    mImpl->isActive = false;
    mImpl->cancelPopup();

    mImpl->rootWidget->mouseExited();
    if (mImpl->onDidDeactivate) {
        mImpl->onDidDeactivate(*this);
    }
    postRedraw();
}

void Window::onMenuWillShow()
{
    assert(Application::instance().activeWindow() == this);

    // We cannot change the menus in Window::onMenuWillShow() on systems like
    // Windows, since it requires the menu to be recreated, but the menu will
    // already be tracking by the time we know we need to update it. So we can
    // only update the window list at other points. To avoid doing it too
    // frequently we sort the window list by title (which is macOS behavior) and
    // update the menus whenever the window title changes (limited to window
    // creation, destruction, and document needs-save changed in most
    // applications).
    auto windowList = sortedWindowList();
    MenuId activeWindowId = Menu::kInvalidId;
    for (int i = 0;  i < int(windowList.size());  ++i) {
        if (windowList[i] == this) {
            activeWindowId = MenuId(int(OSMenubar::StandardItem::kWindow1) + i);
        }
    }

    for (auto* menu : Application::instance().menubar().menus()) {
        MenuIterator it(menu);
        while (!it.done()) {
            // noop if not a standard item, also allows user to override standard items
            updateStandardItem(*this, &it.menuItem(), activeWindowId);

            if (mImpl->onMenuItemNeedsUpdate) {
                mImpl->onMenuItemNeedsUpdate(it.menuItem());
            }
            it.next();
        }
    }
}

void Window::onMenuActivated(MenuId id)
{
    assert((mImpl->flags & int(Flags::kPopup)) == 0);

    auto it = mImpl->onMenuActivatedCallbacks.find(id);
    if (it != mImpl->onMenuActivatedCallbacks.end() && it->second) {
        (it->second)();
    }
}

void Window::onThemeChanged()
{
    mImpl->rootWidget->themeChanged();
}

bool Window::onWindowShouldClose()
{
    // Some X11 window managers let you click the close button even if it has 
    // a transient, modal window.
    if (mImpl->dialog.dialog || mImpl->dialog.window) {
        return false;
    }

    if (mImpl->onShouldClose) {
        return mImpl->onShouldClose(*this);
    }
    return true;
}

void Window::onWindowWillClose()
{
    mImpl->cancelPopup();
    if (mImpl->onWillClose) {
        mImpl->onWillClose(*this);
    }
}

}  // namespace uitk
