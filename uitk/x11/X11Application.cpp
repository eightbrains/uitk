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

#include "X11Clipboard.h"
#include "X11Window.h"
#include "../Application.h"
#include "../Events.h"
#include "../themes/EmpireTheme.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>

#include <locale.h>

#include <list>
#include <map>
#include <mutex>
#include <unordered_map>

namespace {
static const char *kDB_XftDPI = "Xft.dpi";
static const char *kDB_XftDPI_alt = "Xft.Dpi";

static const long kDoubleClickMaxMillisecs = 500;  // Windows' default
static const uitk::PicaPt kDoubleClickMaxRadiusPicaPt(2);  // 2/72 inch
}

namespace uitk {

static std::unordered_map<KeySym, Key> gKeysym2key = {
    { XK_BackSpace, Key::kBackspace },
    { XK_Tab, Key::kTab },
    { XK_KP_Enter, Key::kEnter },
    { XK_Return, Key::kReturn },
    { XK_Escape, Key::kEscape },
    { XK_space, Key::kSpace },
    { XK_KP_Multiply, Key::kNumMultiply },
    { XK_KP_Add, Key::kNumPlus },
    { XK_KP_Separator, Key::kNumComma },
    { XK_KP_Subtract, Key::kNumMinus },
    { XK_KP_Decimal, Key::kNumPeriod },
    { XK_KP_Divide, Key::kNumSlash },
    { XK_Delete, Key::kDelete },
    { XK_Insert, Key::kInsert },
    { XK_Shift_L, Key::kShift },
    { XK_Shift_R, Key::kShift },
    { XK_Control_L, Key::kCtrl },
    { XK_Control_R, Key::kCtrl },
    { XK_Alt_L, Key::kAlt },
    { XK_Alt_R, Key::kAlt },
    { XK_Meta_L, Key::kMeta },
    { XK_Meta_R, Key::kMeta },
    { XK_Caps_Lock, Key::kCapsLock },
    { XK_Num_Lock, Key::kNumLock },
    { XK_Left, Key::kLeft },
    { XK_KP_Left, Key::kLeft },
    { XK_Right, Key::kRight },
    { XK_KP_Right, Key::kRight },
    { XK_Up, Key::kUp },
    { XK_KP_Up, Key::kUp },
    { XK_Down, Key::kDown },
    { XK_KP_Down, Key::kDown },
    { XK_Home, Key::kHome },
    { XK_KP_Home, Key::kHome },
    { XK_End, Key::kEnd },
    { XK_KP_End, Key::kEnd },
    { XK_Page_Up, Key::kPageUp },
    { XK_KP_Page_Up, Key::kPageUp },
    { XK_Page_Down, Key::kPageDown },
    { XK_KP_Page_Down, Key::kPageDown },
    { XK_F1, Key::kF1 },
    { XK_F2, Key::kF2 },
    { XK_F3, Key::kF3 }, 
    { XK_F4, Key::kF4 }, 
    { XK_F5, Key::kF5 }, 
    { XK_F6, Key::kF6 }, 
    { XK_F7, Key::kF7 }, 
    { XK_F8, Key::kF8 }, 
    { XK_F9, Key::kF9 }, 
    { XK_F10, Key::kF10 },
    { XK_F11, Key::kF11 },
    { XK_F12, Key::kF12 },
    { XK_Print, Key::kPrintScreen }
};

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
    if (xstate & Mod4Mask) {
        keymods |= uitk::KeyModifier::kMeta;
    }
    // Do not set numlock or capslock in the keymods, otherwise you have
    // to remember to mask them out when checking for other things, which
    // you are almost sure to forget to do.
    // (Mod2Mask is numlock and LockMask is capslock)
    return keymods;
}

// Adjusted from n-click detection in Win32Window.cpp.
// See https://devblogs.microsoft.com/oldnewthing/20041018-00/?p=37543 for
// pitfalls in detecting double-clicks, triple-clicks, etc.
class ClickCounter
{
public:
    ClickCounter()
    {
        reset();
    }

    int nClicks() const { return mNClicks;  }

    void reset()
    {
        mLastClickTime = Time(-1);  // also will exercise rollover code path!
        mLastClickWindow = 0;
        mButton = -1;
        mNClicks = 0;
    }

    int click(X11Window *w, const XButtonEvent& e)
    {
        if (!w) {  // should never happen, but prevents a crash with maxDistPx
            reset();
            return 0;
        }

        int maxRadiusPx = std::max(1, int(std::round(kDoubleClickMaxRadiusPicaPt.toPixels(w->dpi()))));

        // 'Time' turns out to be unsigned long, so once every 49.7 days
        // a double click can get missed. Since it is unsigned, we cannot use
        // Raymond Chen's rollover trick, and have to detect the rollover.
        Time dt;
        if (e.time >= mLastClickTime) {
            dt = e.time - mLastClickTime;
        } else {
            dt = (Time(-1) - mLastClickTime) + e.time;
        }

        if (w != mLastClickWindow || e.button != mButton
            || std::abs(e.x - mLastClickX) > maxRadiusPx
            || std::abs(e.y - mLastClickY) > maxRadiusPx
            || dt > kDoubleClickMaxMillisecs) {
            mButton = e.button;
            mNClicks = 0;
        }
        mNClicks++;

        mLastClickTime = e.time;
        mLastClickWindow = w;
        mLastClickX = e.x;
        mLastClickY = e.y;

        return mNClicks;
    }

private:
    unsigned int mButton;
    int mNClicks;
    Time mLastClickTime;
    X11Window *mLastClickWindow;  // we do not own this
    int mLastClickX = 0;
    int mLastClickY = 0;
};

//-----------------------------------------------------------------------------
struct X11Application::Impl
{
    Display *display;
    // The database and strings are per-screen, with 0 assumed to be default
    std::map<std::string, std::string> xrdbStrings;
    std::vector<std::map<std::string, std::string>> xrdbScreenStrings;
    std::unordered_map<::Window, X11Window*> xwin2window;
    ClickCounter clickCounter;
    std::unique_ptr<X11Clipboard> clipboard;

    Atom postedFuncAtom;
    std::mutex postedFunctionsLock;
    // This is a linked list because adding and removing does not invalidate
    // iterators.
    std::list<std::function<void()>> postedFunctions;
};

X11Application::X11Application()
    : mImpl(new Impl())
{
    // Required to read in user values of the LC_* variables. These influence
    // the default fonts that Pango chooses.
    setlocale(LC_ALL, "");

    mImpl->display = XOpenDisplay(NULL);

    mImpl->clipboard = std::make_unique<X11Clipboard>(mImpl->display);

    mImpl->postedFuncAtom = XInternAtom(mImpl->display, "PostedFunction", False);

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

void X11Application::beep()
{
    if (mImpl->display) {
        XBell(mImpl->display, 0 /* base volume, ranges [-100, 100]*/);
    }
}

bool X11Application::isOriginInUpperLeft() const { return true; }

bool X11Application::shouldHideScrollbars() const { return false; }

bool X11Application::platformHasMenubar() const { return true; }

Clipboard& X11Application::clipboard() const { return *mImpl->clipboard; }

void X11Application::scheduleLater(uitk::Window* w, std::function<void()> f)
{
    {
    std::lock_guard<std::mutex> locker(mImpl->postedFunctionsLock);
    mImpl->postedFunctions.push_back(f);
    }

    XEvent xe;
    xe.type = ClientMessage;
    xe.xclient.type = ClientMessage;  // maybe X server sets this?
    xe.xclient.window = (::Window)w->nativeHandle();
    xe.xclient.message_type = mImpl->postedFuncAtom;
    xe.xclient.format = 32;  // 8, 16, 32 (size of xclient.data), we do not
                             // use that, so this does not matter
    XSendEvent(mImpl->display, xe.xclient.window, False, NoEventMask, &xe);
}

int X11Application::run()
{
    Atom kWMProtocolType = XInternAtom(mImpl->display, "WM_PROTOCOLS", True);
    Atom kWMDeleteMsg = XInternAtom(mImpl->display, "WM_DELETE_WINDOW", False);
    Atom kClipboard = XInternAtom(mImpl->display, "CLIPBOARD", False);
    Atom kPrimary = XInternAtom(mImpl->display, "PRIMARY", False);
    Atom kClipboardTargets = XInternAtom(mImpl->display, "TARGETS", False);

    XIM xim = XOpenIM(mImpl->display, 0, 0, 0);
    XIC xic = XCreateIC(xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                        NULL);

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
                    me.button.nClicks = mImpl->clickCounter.click(w, event.xbutton);
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
                        me.button.button = MouseButton::kMiddle;
                        break;
                    case Button3:
                        me.button.button = MouseButton::kRight;
                        break;
                    case Button4:
                        me.type = MouseEvent::Type::kScroll;
                        me.scroll.dx = PicaPt::kZero;
                        me.scroll.dy = PicaPt(1.0f);
                        break;
                    case Button5:
                        me.type = MouseEvent::Type::kScroll;
                        me.scroll.dx = PicaPt::kZero;
                        me.scroll.dy = PicaPt(-1.0f);
                        break;
                    case Button5 + 1:
                        me.type = MouseEvent::Type::kScroll;
                        me.scroll.dx = PicaPt(1.0f);
                        me.scroll.dy = PicaPt::kZero;
                        break;
                    case Button5 + 2:
                        me.type = MouseEvent::Type::kScroll;
                        me.scroll.dx = PicaPt(-1.0f);
                        me.scroll.dy = PicaPt::kZero;
                        break;
                    case Button5 + 3:
                        me.button.button = MouseButton::kButton4;
                        break;
                    case Button5 + 4:
                        me.button.button = MouseButton::kButton5;
                        break;
                }
                
                bool ignore = (event.type == ButtonRelease &&
                               event.xbutton.button >= Button4 &&
                               event.xbutton.button <= Button5 + 2);
                if (!ignore) {
                    w->onMouse(me, event.xbutton.x, event.xbutton.y);
                }
                break;
            }
            case KeyPress:  // fall through
            case KeyRelease: {
                mImpl->clickCounter.reset();

                KeySym ksym = XLookupKeysym(&event.xkey, 0);
                Key key;
                if (ksym >= XK_A && ksym <= XK_Z) {
                    key = Key(int(Key::kA) + (ksym - XK_A));
                } else if (ksym >= XK_a && ksym <= XK_z) {
                    key = Key(int(Key::kA) + (ksym - XK_a));
                } else if (ksym >= XK_0 && ksym <= XK_9) {
                    key = Key(int(Key::k0) + (ksym - XK_0));
                } else {
                    auto kit = gKeysym2key.find(ksym);
                    if (kit != gKeysym2key.end()) {
                        key = kit->second;
                    } else {
                        key = Key::kUnknown;
                    }
                }

                KeyEvent ke;
                ke.type = (event.type == KeyPress ? KeyEvent::Type::kKeyDown
                                                  : KeyEvent::Type::kKeyUp);
                ke.key = key;
                ke.nativeKey = ksym;
                ke.keymods = toKeymods(event.xkey.state);
                ke.isRepeat = false;  // TODO: figure this out

                w->onKey(ke);

                if (event.type == KeyPress && ke.keymods == 0 &&
                    ((ksym >= 0x0020 && ksym <= 0xfdff) || // most languages
                     (ksym >= XK_braille_dot_1 && ksym <= XK_braille_dot_10) ||
                     (ksym >= 0x10000000 && ksym < 0x11000000))) {
                    char utf8[32];
                    Status status;
                    int len = Xutf8LookupString(xic, &event.xkey, utf8, 32,
                                                nullptr, &status);
                    utf8[len] = '\0';  // just in case

                    TextEvent te;
                    te.utf8 = utf8;
                    w->onText(te);
                }

                break;
            }
            case DestroyNotify:  // get with StructureNotifyMask
                w->onWindowWillClose();
                mImpl->xwin2window.erase(wit);
                if (mImpl->xwin2window.empty()) {
                    done = true;
                }
                break;
            case FocusIn:
                mImpl->clickCounter.reset();
                // X11 makes the clipboard owned by the window, instead of
                // globally, which is how it functions (and how macOS and Win32
                // expose it). To avoid exporting this lousy interface, we
                // keep track of the active window so that the clipboard class
                // can do a copy at any time without the caller needing to
                // be aware of this mess.
                mImpl->clipboard->setActiveWindow(event.xany.window);
                break;
            case FocusOut:
                mImpl->clickCounter.reset();
                break;
            case KeymapNotify:
                // Update keyboard state
                break;
            case SelectionClear: {  // lost clipboard ownership
                // We consider the clipboard to be global to us, so if the
                // new window is still our window, we do not consider this
                // losing ownership. (Also, this prevents us from incorrectly
                // clearing our knowledge of ownership if we cut/copy from a
                // different window of ours.)
                auto newOwner = XGetSelectionOwner(mImpl->display,
                                                event.xselectionclear.selection);
                auto clipWinIt = mImpl->xwin2window.find(newOwner);
                if (clipWinIt == mImpl->xwin2window.end()) {
                    auto which = (event.xselectionclear.selection == kClipboard
                              ? X11Clipboard::Selection::kClipboard
                              : X11Clipboard::Selection::kTextSelection);
                    mImpl->clipboard->weAreNoLongerOwner(which);
                }
                break;
            }
            case SelectionRequest: {  // someone wants to paste
                if (event.xselectionrequest.selection != kClipboard &&
                    event.xselectionrequest.selection != kPrimary) {
                    break;
                }
                auto which = (event.xselectionrequest.selection == kClipboard
                              ? X11Clipboard::Selection::kClipboard
                              : X11Clipboard::Selection::kTextSelection);

                XSelectionEvent e = {0};
                e.type = SelectionNotify;
                e.display = event.xselectionrequest.display;
                e.requestor = event.xselectionrequest.requestor;
                e.selection = event.xselectionrequest.selection;
                e.time = event.xselectionrequest.time;
                e.target = event.xselectionrequest.target;
                e.property = event.xselectionrequest.property;

                //char* name = XGetAtomName(instance.display, ev.target);
                //std::cout << "target " << name << std::endl;
                //XFree(name);

                if (e.target == kClipboardTargets) {
                    auto targets = mImpl->clipboard->supportedTypes(which);
                    XChangeProperty(mImpl->display, e.requestor, e.property,
                                    XA_ATOM, 32, PropModeReplace,
                                    targets.data(), targets.size());
                } else if (mImpl->clipboard->doWeHaveDataForTarget(which, e.target)) {
                    auto data = mImpl->clipboard->dataForTarget(which, e.target);
                    XChangeProperty(mImpl->display, e.requestor,
                                    e.property, e.target, 8, PropModeReplace,
                                    data.data(), data.size());
                } else {
                    e.property = None;
                }

                XSendEvent(mImpl->display, e.requestor, 0, 0, (XEvent *)&e);
                break;
            }
            case ClientMessage:
                if (event.xclient.message_type == kWMProtocolType) {
                    if (event.xclient.data.l[0] == kWMDeleteMsg) {
                        w->close();
                    }
                } else if (event.xclient.message_type == mImpl->postedFuncAtom) {
                    // The posted function might generate another posted function
                    // (for example, an animation), so we only run the functions
                    // that we have right now. Also, we need to not be holding
                    // the lock when we run the function, otherwise posting a
                    // function will deadlock.
                    size_t n = 0;
                    mImpl->postedFunctionsLock.lock();
                    n = mImpl->postedFunctions.size();
                    mImpl->postedFunctionsLock.unlock();

                    for (size_t i = 0; i < n; ++i) {
                        std::function<void()> f;
                        mImpl->postedFunctionsLock.lock();
                        f = *mImpl->postedFunctions.begin();
                        mImpl->postedFunctionsLock.unlock();

                        f();

                        mImpl->postedFunctionsLock.lock();
                        mImpl->postedFunctions.erase(mImpl->postedFunctions.begin());
                        mImpl->postedFunctionsLock.unlock();
                    }
                }
                break;
            default:
                break;
        }
    }

    XDestroyIC(xic);
    XCloseIM(xim);
}

void X11Application::exitRun()
{
    // We do not need to do anything here because this should only be called from
    // Application::quit(), which will have closed all the windows, causing us to
    // exit.
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
