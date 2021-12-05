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

#include "Window.h"

#include "Application.h"
#include "Events.h"
#include "Menubar.h"
#include "OSWindow.h"
#include "MenuUITK.h"
#include "UIContext.h"
#include "Widget.h"
#include "themes/Theme.h"
#include "themes/EmpireTheme.h"

#if defined(__APPLE__)
#include "macos/MacOSWindow.h"
#elif defined(_WIN32) || defined(_WIN64)  // _WIN32 covers everything except 64-bit ARM
#include "win32/Win32Window.h"
#else
#include "x11/X11Window.h"
#endif

#include <nativedraw.h>

#include <unordered_map>

namespace uitk {

enum class PopupState { kNone, kShowing, kCancelling };

struct Window::Impl
{
    std::shared_ptr<Theme> theme;
    std::unique_ptr<OSWindow> window;
    std::string title;
    int flags;
    std::unique_ptr<Widget> menubarWidget;
    std::unique_ptr<Widget> rootWidget;
    Widget *grabbedWidget = nullptr;
    Widget *focusedWidget = nullptr;
    MenuUITK *activePopup = nullptr;
    void *dialog = nullptr;  // placeholder for later
    PopupState popupState = PopupState::kNone;
    std::function<void(Menubar&)> onMenuWillShow;
    std::unordered_map<MenuId, std::function<void()>> onMenuActivatedCallbacks;
    std::function<void(Window& w, const LayoutContext& context)> onWillShow;
    std::function<void(Window& w)> onDidDeactivate;
    std::function<bool(Window& w)> onShouldClose;
    std::function<void(Window& w)> onWillClose;
    bool isActive = false;
    bool inResize = false;
    bool inMouse = false;
    bool inKey = false;
    bool inDraw = false;
    bool needsDraw = false;

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

    auto &menubar = Application::instance().menubar();
    if (!(flags & Flags::kPopup) && !menubar.isNative()) {
        mImpl->menubarWidget = menubar.createWidget();
        mImpl->menubarWidget->setWindow(this);
    }

#if defined(__APPLE__)
    mImpl->window = std::make_unique<MacOSWindow>(*this, title, x, y, width, height, flags);
#elif defined(_WIN32) || defined(_WIN64)
    mImpl->window = std::make_unique<Win32Window>(*this, title, x, y, width, height, flags);
#else
    mImpl->window = std::make_unique<X11Window>(*this, title, x, y, width, height, flags);
#endif

    Application::instance().windowSet().insert(this);
}

Window::~Window()
{
    Application::instance().windowSet().erase(this);
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

void* Window::nativeHandle() { return mImpl->window->nativeHandle(); }

bool Window::isShowing() const { return mImpl->window->isShowing(); }

Window* Window::show(bool show)
{
    auto onWillShow = [this](const DrawContext& dc) {
        if (mImpl->onWillShow) {
            LayoutContext context = { *mImpl->theme, dc };
            mImpl->onWillShow(*this, context);
        }
    };
    mImpl->window->show(show, onWillShow);
    return this;
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

std::string Window::title() const { return mImpl->title; }

Window* Window::setTitle(const std::string& title)
{
    mImpl->title = title;
    mImpl->window->setTitle(title);
    return this;
}

void Window::resize(const Size& contentSize)
{
    auto dpi = mImpl->window->dpi();
    auto f = mImpl->window->osFrame();
    if (Application::instance().isOriginInUpperLeft()) {
        setOSFrame(f.x, f.y, contentSize.width.toPixels(dpi), contentSize.height.toPixels(dpi));
    } else {
        auto hPx = contentSize.height.toPixels(dpi);
        setOSFrame(f.x, f.y + f.height - hPx, contentSize.width.toPixels(dpi), hPx);
    }
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

void Window::postRedraw() const { mImpl->window->postRedraw(); }

void Window::raiseToTop() const { mImpl->window->raiseToTop(); }

const Rect& Window::contentRect() const { return mImpl->rootWidget->frame(); }

Window* Window::addChild(Widget *child)
{
    mImpl->rootWidget->addChild(child);
    setNeedsDraw();
    return this;
}

void Window::setOnMenuWillShow(std::function<void(Menubar&)> onWillShow)
{
    mImpl->onMenuWillShow = onWillShow;
}

void Window::setOnMenuActivated(MenuId id, std::function<void()> onActivated)
{
    mImpl->onMenuActivatedCallbacks[id] = onActivated;
}

void Window::setOnWindowWillShow(std::function<void(Window& w, const LayoutContext& context)> onWillShow)
{
    mImpl->onWillShow = onWillShow;
}

void Window::setOnWindowDidDeactivate(std::function<void(Window& w)> onDidDeactivate)
{
    mImpl->onDidDeactivate = onDidDeactivate;
}

void Window::setOnWindowShouldClose(std::function<bool(Window& w)> onShouldClose)
{
    mImpl->onShouldClose = onShouldClose;
}

void Window::setOnWindowWillClose(std::function<void(Window& w)> onWillClose)
{
    mImpl->onWillClose = onWillClose;
}

void Window::setMouseGrab(Widget *w)
{
    mImpl->grabbedWidget = w;
}

Widget* Window::mouseGrabWidget() const { return mImpl->grabbedWidget; }

void Window::setFocusWidget(Widget *w)
{
    auto *oldFocusedWidget = mImpl->focusedWidget;
    bool isDifferent = (w != oldFocusedWidget);

    mImpl->focusedWidget = w;

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

void Window::onMouse(const MouseEvent& eOrig)
{
    if (mImpl->activePopup) {
        if (eOrig.type == MouseEvent::Type::kButtonDown) {
            mImpl->cancelPopup();
        }
        // Some systems (at least macOS) send mouse events outside the window to
        // the parent window of a borderless window. If this happens, convert
        // move/drag events to the popup and send them on. Unless we are in the menubar,
        // in which case we need to pass the events in case the user mouses over a
        // different menu and we need to change the open menu.
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
    if (!mImpl->dialog && e.type == KeyEvent::Type::kKeyDown && Application::instance().keyboardShortcuts().hasShortcut(e, &menuId)) {
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
        if (mImpl->focusedWidget) {
            mImpl->focusedWidget->key(e);
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
    mImpl->rootWidget->layout(context);
}

void Window::onDraw(DrawContext& dc)
{
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
    // TODO: handle standard menu items here

    if (mImpl->onMenuWillShow) {
        mImpl->onMenuWillShow(Application::instance().menubar());
    }
}

void Window::onMenuActivated(MenuId id)
{
    assert((mImpl->flags & int(Flags::kPopup)) == 0);

    // TODO: handle standard menu items here

    auto it = mImpl->onMenuActivatedCallbacks.find(id);
    if (it != mImpl->onMenuActivatedCallbacks.end() && it->second) {
        (it->second)();
    }
}

bool Window::onWindowShouldClose()
{
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
