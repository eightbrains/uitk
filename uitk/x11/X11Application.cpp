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

#include "X11Application.h"

#include "X11Window.h"
#include "../Events.h"
#include "../themes/EmpireTheme.h"

#include <X11/Xlib.h>
#include <X11/Xresource.h>

#include <map>
#include <unordered_map>

namespace {
static const char *kDB_XftDPI = "Xft.dpi";
static const char *kDB_XftDPI_alt = "Xft.Dpi";

int toKeymods(unsigned int xstate) {
    int keymods = 0;
    if (xstate & ShiftMask) {
        keymods |= uitk::KeyModifier::kShift;
    }
    if (xstate & ControlMask) {
        keymods |= uitk::KeyModifier::kCtrl;
    }
    if (xstate & Mod1Mask) {
        keymods |= uitk::KeyModifier::kAlt;
    }
    //if (xstate & Mod2Mask) {
    //    keymods |= uitk::KeyModifier::kNumLock;
    //}
    if (xstate & Mod4Mask) {
        keymods |= uitk::KeyModifier::kMeta;
    }
    if (xstate & LockMask) {
        keymods |= uitk::KeyModifier::kCapsLock;
    }
    return keymods;
}

}

namespace uitk {

struct X11Application::Impl
{
    Display *display;
    // The database and strings are per-screen, with 0 assumed to be default
    std::map<std::string, std::string> xrdbStrings;
    std::vector<std::map<std::string, std::string>> xrdbScreenStrings;
    std::unordered_map<::Window, X11Window*> xwin2window;
};

X11Application::X11Application()
    : mImpl(new Impl())
{
    mImpl->display = XOpenDisplay(NULL);

    // Read the resource databases from each screen.
    int nScreens = XScreenCount(mImpl->display);
    mImpl->xrdbScreenStrings.resize(nScreens);
    XrmInitialize();

    static const std::vector<std::string> kQueryStrings =
        { kDB_XftDPI, kDB_XftDPI_alt };

    auto readDatabase = [](char *resourceString,
                           std::map<std::string, std::string>& keyVal) {
        char *type = NULL;
        XrmValue value;

        auto db = XrmGetStringDatabase(resourceString);
        for (auto &key : kQueryStrings) {
            if (XrmGetResource(db, key.c_str(), "String", &type, &value) == True) {
                keyVal[key] = std::string(value.addr);
            }
        }
        XrmDestroyDatabase(db);
    };

    // Read the global resources. Docs say NOT to free the string.
    auto resourceString = XResourceManagerString(mImpl->display);
    readDatabase(resourceString, mImpl->xrdbStrings);

    // Read the per-screen resources, in case you can set Xft.dpi separately
    // per-screen (which seems like a good idea).
    for (int sn = 0; sn < nScreens; ++sn) {
        Screen *s = XScreenOfDisplay(mImpl->display, sn);
        auto resourceString = XScreenResourceString(s);
        if (resourceString) {
            readDatabase(resourceString, mImpl->xrdbScreenStrings[sn]);
            XFree(resourceString);  // docs say MUST free this string
        }
    }
}

X11Application::~X11Application()
{
    XCloseDisplay(mImpl->display);
}

void X11Application::setExitWhenLastWindowCloses(bool exits)
{
    // Do nothing, this is pretty much always true on Linux, as there would
    // be no way to open a new window after the last one closes.
}

int X11Application::run()
{
    Atom kWMDeleteMsg = XInternAtom(mImpl->display, "WM_DELETE_WINDOW", False);

    bool done = false;
    XEvent event;
    while (!done) {
        XNextEvent(mImpl->display, &event);

        X11Window *w;
        auto wit = mImpl->xwin2window.find(event.xany.window);
        if (wit != mImpl->xwin2window.end()) {
            w = wit->second;
        } else {
            continue;  // we know nothing about this window, ignore event
        }

        switch (event.type) {
            case Expose:  // GraphicsExpose only happens for XCopyArea/XCopyPlane
                w->onDraw();
                break;
            case ConfigureNotify:
                // This gets called when a window is moved, resized, raised,
                // lowered, or border width is changed. We only need to resize
                // on resize (and move if moved to a different screen),
                // but the others are rare enough.
                w->onResize();
                break;
            //case ResizeRequest:
            //    // Determine proper size here (e.g. enforce minimum size
            //    // or aspect ratio), then resize appropriately
            //    // XResizeWindow(mImpl->display, event.xresizerequest.window,
            //                  event.xresizerequest.width,
            //                  event.xresizerequest.height);
            //    w->onResize();
            //    break;
            case MotionNotify: {
                int buttons = 0;
                if (event.xmotion.state & Button1MotionMask) {
                    buttons |= int(MouseButton::kLeft);
                }
                if (event.xmotion.state & Button2MotionMask) {
                    buttons |= int(MouseButton::kRight);
                }
                if (event.xmotion.state & Button3MotionMask) {
                    buttons |= int(MouseButton::kMiddle);
                }
                if (event.xmotion.state & Button4MotionMask) {
                    buttons |= int(MouseButton::kButton4);
                }
                if (event.xmotion.state & Button5MotionMask) {
                    buttons |= int(MouseButton::kButton5);
                }
                MouseEvent me;
                if (buttons == 0) {
                    me.type = MouseEvent::Type::kMove;
                } else {
                    me.type = MouseEvent::Type::kDrag;
                    me.drag.buttons = buttons;
                }
                me.keymods = toKeymods(event.xmotion.state);
                w->onMouse(me, event.xmotion.x, event.xmotion.y);
                break;
            }
            case ButtonPress:  // fall through
            case ButtonRelease:
            {
                MouseEvent me;
                if (event.type == ButtonPress) {
                    me.type = MouseEvent::Type::kButtonDown;
                    me.button.nClicks = 1;
                } else {
                    me.type = MouseEvent::Type::kButtonUp;
                    me.button.nClicks = 0;
                }
                me.keymods = toKeymods(event.xmotion.state);
                switch (event.xbutton.button) {
                    default:
                    case Button1:
                        me.button.button = MouseButton::kLeft;
                        break;
                    case Button2:
                        me.button.button = MouseButton::kRight;
                        break;
                    case Button3:
                        me.button.button = MouseButton::kMiddle;
                        break;
                    case Button4:
                        me.button.button = MouseButton::kButton4;
                        break;
                    case Button5:
                        me.button.button = MouseButton::kButton5;
                        break;
                }
                w->onMouse(me, event.xbutton.x, event.xbutton.y);
                break;
            }
            case KeyPress:
                break;
            case KeyRelease:
                break;
            case ClientMessage:
                if (event.xclient.data.l[0] == kWMDeleteMsg) {
                    w->close();
                }
                break;
            case DestroyNotify:  // get with StructureNotifyMask
                mImpl->xwin2window.erase(wit);
                if (mImpl->xwin2window.empty()) {
                    done = true;
                }
                break;
            case FocusIn:
            case FocusOut:
                break;
            case KeymapNotify:
                // Update keyboard state
                break;
            default:
                break;
        }
    }
}

Theme::Params X11Application::themeParams() const
{
    return EmpireTheme::defaultParams();
}

void* X11Application::display() const
{
    return (void*)mImpl->display;
}

void X11Application::registerWindow(long unsigned int xwindow,
                                    X11Window *window)
{
    mImpl->xwin2window[(::Window)xwindow] = window;
}

float X11Application::dpiForScreen(int screen)
{
    if (screen >= mImpl->xrdbScreenStrings.size()) {
        screen = 0;
    }

    auto findXftDPI = [](const std::map<std::string, std::string>& db) {
        auto it = db.find(kDB_XftDPI);
        if (it == db.end()) {
            it = db.find(kDB_XftDPI_alt);
        }
        if (it != db.end()) {
            return it->second;
        }
        return std::string("");
    };

    // Check if Xft.dpi (or Xft.Dpi) was set on this screen, and if so, use that.
    // (Note: check if 'screen' is still valid, as xrdbStrings might be empty
    // if there was an error).
    std::string dpiStr;
    if (screen < mImpl->xrdbScreenStrings.size()) {
        dpiStr = findXftDPI(mImpl->xrdbScreenStrings[screen]);
    }
    // If there is not anything for this screen, check the global strings.
    if (dpiStr.empty()) {
        dpiStr = findXftDPI(mImpl->xrdbStrings);
    }

    // If we found Xft.dpi set on the screen or globally, return that value.
    if (!dpiStr.empty()) {
        return std::atof(dpiStr.c_str());
    }

    // Otherwise return what X reports.
    int heightPx = DisplayHeight(mImpl->display, screen);
    int heightMM = DisplayHeightMM(mImpl->display, screen);
    return float(heightPx) / (float(heightMM) / 25.4f);
}

} // namespace uitk
