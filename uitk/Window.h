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

#ifndef UITK_WINDOW_H
#define UITK_WINDOW_H

#include <memory>
#include <string>

#include "Global.h"
#include "OSWindow.h"

namespace uitk {

class DrawContext;
struct Point;

struct KeyEvent;
struct MouseEvent;
struct LayoutContext;
class Cursor;
class Dialog;
class MenuItem;
class MenuUITK;
class OSMenubar;
class Widget;

class Window : public IWindowCallbacks
{
    friend class MenuUITK;
    
    // Design notes:
    // Q: Why not make creation a factory function on Application?
    // A: That would prevent users from inheriting from Window, which is
    //    useful.
    // Q: Well, that forces users to manually manage an object whose contents
    //    (the native window) belongs more to Application than Window.
    // A: True, although native windows need to be manually managed, too.
public:
    struct Flags {  // the struct provides scoping like enum class
        enum Value {
            kNormal = 0,
            kDialog = (1 << 0),
            kPopup = (2 << 0),
            kMenuEdges = (3 << 1),  // used internally for menus: makes the top corners square
        };
    };

    /// Creates a window with default (x, y) position.
    /// width, height are in operating-system coordinates. The window is not
    /// shown.
    Window(const std::string& title, int width, int height,
           Flags::Value flags = Flags::kNormal);
    /// Creates a window. x, y, width, height are in operating-system
    /// coordinates. The window is not shown.
    Window(const std::string& title, int x, int y, int width, int height,
           Flags::Value flags = Flags::kNormal);
    virtual ~Window();

    bool isShowing() const;
    Window* show(bool show);
    void toggleMinimize();
    void toggleMaximize();

    bool isActive() const;

    enum class CloseBehavior { kAllowCancel, /// (default) allows onWindowShouldClose to return false
                               kForceClose   /// forces the window to close
    };
    /// Returns true if the window will close, false if onWindowShouldClose
    /// or the setOnWindowShouldClose callback returned false.
    bool close(CloseBehavior ask = CloseBehavior::kAllowCancel);

    /// Schedules the window for deletion at a point in the event loop where
    /// it is safe. This function is safe to call in the setOnWindowWillClose
    /// callback. (Delete is not safe, as it may delete lambda that is executing.)
    void deleteLater();

    const std::string& title() const;
    Window* setTitle(const std::string& title);

    // Design note:
    // Q: Why not have the Widget own a cursor, and automatically set it
    //    in mouseEntered() and mouseExited()?
    // A: This works fine for simplet widgets, like a numeric control, but
    //    it does not work well for something like a vector graphics canvas,
    //    where the cursor changes to sizing controls at the edges and corners
    //    of an object. In this case, the canvas widget would need to set
    //    the cursor of the object, which would have to set it on the window
    //    anyway, so why not make it clearer? Besides, mouseEntered() and
    //    mouseExited() do other things besides setting the cursor, so if
    //    inherited classes wanted to override the behavior they would not
    //    be able to do so easily. This way offers more flexibility, and it
    //    not that difficult for the few widgets that need a different cursor.

    /// Pushes the cursor for the window. The usual pattern is for widgets that
    /// want a particular cursor to pushCursor() in Widget::mouseEntered()
    /// and popCursor() in Widget::mouseExited().
    void pushCursor(const Cursor& cursor);
    void popCursor();
    /// Changes the cursor at the top of the stack. Generally you should use
    /// pushCursor() and popCursor(), as they will handle child objects that
    /// change the cursor better, but setCursor() is useful if you need to
    /// change the cursor after the mouse is already in the widget but is
    /// not over a child. For instance, a vector graphics canvas might use this
    /// to change the cursor inside an object to indicate it can be selected,
    /// resized, rotated, etc. A canvas might not use child objects to
    /// represent each graphic element. A good rule of thumb is that
    /// pushCursor() belongs in mouseEntered(), popCursor() belongs in
    /// mouseExited(), and then if necessary setCursor() can be used in mouse().
    void setCursor(const Cursor& cursor);

    /// Resizes the window so that the content rect is of the specified size.
    /// The actual window may be larger due to the title bar (if the size of the
    /// window includes the title bar on the OS) and the menubar (if a menubar is
    /// specified and if the OS includes the menubar as part of the window).
    /// Use setOSFrame if you need to set the size of the actual window (this is
    /// not normally helpful).
    void resize(const Size& contentSize);
    /// Resizes the window to the largest preferred size of the children.
    void resizeToFit();
    /// Resizes the window to the size returned by the provided function.
    void resizeToFit(std::function<Size(const LayoutContext&)> calcSizeFunc);

    void move(const PicaPt& dx, const PicaPt& dy);

    OSWindow::OSRect osFrame() const;
    /// Sets the window rectangle of the operating system's window, in operating
    /// system coordinates (pixels in Windows and X11, virtual-pixels in macOS).
    /// Note that this is NOT the content rect, as the operating system may include
    /// titlebars and/or menubars in the window area. resize() is probably a
    /// more convenient function if you need to change the window's size.
    void setOSFrame(float x, float y, float width, float height);

    /// Returns the point in the operating system's window manager of the
    /// point passed it. This is useful for positioning popup windows.
    OSWindow::OSPoint convertWindowToOSPoint(const Point& windowPt) const;

    /// Returns the content rect of the window, relative to the upper left of the
    /// drawable area. Usually this is the drawable area of the window, but on
    /// platforms where UITK draws the menus it is offset by the size of the menu.
    const Rect& contentRect() const;

    /// Takes ownership of the widget and adds as a child to the Window.
    /// Returns pointer the child added so that adding and assignment for later
    /// can be in one convenient step. If setOnWindowLayout() has set a
    /// callback function that will be called when the window resizes, otherwise
    /// all the children will be set to the visible area of the window (obviously
    /// this is most useful if there is only one child).
    Widget* addChild(Widget *child);

    /// Remove child (if it is a child), and returns ownership to the caller.
    Widget* removeChild(Widget *child);

    /// Schedules a redraw.
    void setNeedsDraw();

    /// Schedules a layout
    void setNeedsLayout();

    void raiseToTop() const;

    PicaPt borderWidth() const;
    
    OSWindow* nativeWindow();
    void* nativeHandle();

    /// Sets a callback that will be called whenever a menu item needs to
    /// update its checked or enabled state; currently this is right before
    /// the menu is opened, and is called for all menu items.
    /// This is where menu items should be enabled and disabled. Note that
    /// an item should either be ignored, or it should always set its status.
    /// Menus are global, so the current "state" of the menu should be
    /// considered to be incorrect, as it may have been set for a different
    /// window.
    /// The usual pattern for the callback is:
    ///   void itemNeedsUpdate(Menu::Item& item) {
    ///     switch (item.id()) {
    ///       case Feature1ActionItemId:
    ///         item.setEnabled(model.isFeature1Valid);
    ///         break;
    ///       case BoolFeature2Id:
    ///         item.setChecked(model.feature2State);
    ///         break;
    ///       default:
    ///         break;  // these items will always be enabled and unchecked
    ///     }
    ///   }
    void setOnMenuItemNeedsUpdate(std::function<void(MenuItem&)> onNeedsUpdate);

    /// Sets the callback when a menu item is activated/selected.
    /// This is a convenience instead of overriding onMenuActivated and putting
    /// in a big switch statement.
    void setOnMenuActivated(MenuId id, std::function<void()> onActivated);

    void setOnWindowWillShow(std::function<void(Window&)> onWillShow);
    void setOnWindowLayout(std::function<void(Window&, const LayoutContext&)> onLayout);
    void setOnWindowDidDeactivate(std::function<void(Window&)> onDidDeactivate);
    void setOnWindowShouldClose(std::function<bool(Window&)> onShouldClose);
    void setOnWindowWillClose(std::function<void(Window&)> onWillClose);

public:
    /// Directs mouse events directly to the widget specified until mouse up.
    /// Useful for when a widget needs to capture mouse drag events outside
    /// its frame.
    void setMouseGrab(Widget *w);
    Widget* mouseGrabWidget() const;

    /// Sets the widget that gets key events.
    void setFocusWidget(Widget *w);;
    Widget* focusWidget() const;

    // On macOS windows without a titlebar do not get activated/deactivated
    // messages, so we need to register the popup menu
    void setPopupMenu(MenuUITK *menu);
    /// Returns the active popup menu, or nullptr
    MenuUITK* popupMenu() const;

    // Takes ownership of the dialog and displays modal to this window,
    // returning true. If a dialog is already displaying, returns false and does
    // not take ownership. This is for use in implementing dialogs; use
    // Dialog::showModal() instead.
    bool beginModalDialog(Dialog *d);
    // Ends display of the dialog and returns ownership to the caller.
    Dialog* endModalDialog();

    void onResize(const DrawContext& dc) override;
    void onLayout(const DrawContext& dc) override;
    void onDraw(DrawContext& dc) override;
    void onMouse(const MouseEvent& e) override;
    void onKey(const KeyEvent& e) override;
    void onText(const TextEvent& e) override;
    void onActivated(const Point& currentMousePos) override;
    void onDeactivated() override;
    void onMenuWillShow() override;
    void onMenuActivated(MenuId id) override;
    bool onWindowShouldClose() override;
    void onWindowWillClose() override;

protected:
    /// Posts a redraw message to the event loop scheduling a redraw.
    /// Note that this does not immediately redraw the window, that only
    /// happens when the event loop handles the redraw event.
    void postRedraw() const;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace uitk

#endif // UITK_WINDOW_H
