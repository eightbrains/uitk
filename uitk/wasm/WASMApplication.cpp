//-----------------------------------------------------------------------------
// Copyright 2021 - 2023 Eight Brains Studios, LLC
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

#include "WASMApplication.h"

#include "WASMClipboard.h"
#include "WASMCursor.h"
#include "WASMSound.h"
#include "WASMWindow.h"
#include "../Application.h"
#include "../OSWindow.h"  // for OSScreen
#include "../TextEditorLogic.h"
#include "../themes/EmpireTheme.h"
#include "../private/PlatformUtils.h"

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/html5.h>

#include <chrono>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include <stdio.h>

using EmVal = emscripten::val;

namespace uitk {

namespace {
static const float kCSSDPI = 96.0f;
static const long kDoubleClickMaxMillisecs = 500;  // Windows' default
static const uitk::PicaPt kDoubleClickMaxRadiusPicaPt(2);  // 2/72 inch
static int gNextUnnamedCanvasId = 1;

enum class OS { kMacOS, kWindows, kLinux, kIOS, kAndroid, kOther };
static OS gOS = OS::kOther;

struct Keymapping
{
    const char *dom;
    Key key;
};
static const std::unordered_map<std::string, Key> kDOMKeymaps = {
    { "Backspace", Key::kBackspace },
    { "Tab", Key::kTab },
    { "NumpadEnter", Key::kEnter },
    { "Enter", Key::kReturn },
    { "Escape", Key::kEscape },
    { "Space", Key::kSpace },
    { "NumpadMultiply", Key::kNumMultiply },  // TODO: test
    { "NumpadPlus", Key::kNumPlus },  // TODO: test
    { "NumpadComma", Key::kNumComma },  // TODO: test
    { "NumpadMinus", Key::kNumMinus },  // TODO: test
    { "NumpadPeriod", Key::kNumPeriod },  // TODO: test
    { "NumpadSlash", Key::kNumSlash },  // TODO: test
    { "Digit0", Key::k0 },
    { "Digit1", Key::k1 },
    { "Digit2", Key::k2 },
    { "Digit3", Key::k3 },
    { "Digit4", Key::k4 },
    { "Digit5", Key::k5 },
    { "Digit6", Key::k6 },
    { "Digit7", Key::k7 },
    { "Digit8", Key::k8 },
    { "Digit9", Key::k9 },
    { "KeyA", Key::kA },
    { "KeyB", Key::kB },
    { "KeyC", Key::kC },
    { "KeyD", Key::kD },
    { "KeyE", Key::kE },
    { "KeyF", Key::kF },
    { "KeyG", Key::kG },
    { "KeyH", Key::kH },
    { "KeyI", Key::kI },
    { "KeyJ", Key::kJ },
    { "KeyK", Key::kK },
    { "KeyL", Key::kL },
    { "KeyM", Key::kM },
    { "KeyN", Key::kN },
    { "KeyO", Key::kO },
    { "KeyP", Key::kP },
    { "KeyQ", Key::kQ },
    { "KeyR", Key::kR },
    { "KeyS", Key::kS },
    { "KeyT", Key::kT },
    { "KeyU", Key::kU },
    { "KeyV", Key::kV },
    { "KeyW", Key::kW },
    { "KeyX", Key::kX },
    { "KeyY", Key::kY },
    { "KeyZ", Key::kZ },
    { "Delete", Key::kDelete },
    { "Insert", Key::kInsert },
    { "ShiftLeft", Key::kShift },
    { "ShiftRight", Key::kShift },
    { "ControlLeft", Key::kCtrl },
    { "ControlRight", Key::kCtrl },
    { "AltLeft", Key::kAlt },
    { "AltRight", Key::kAlt },
    { "MetaLeft", Key::kMeta },
    { "MetaRight", Key::kMeta },
    { "CapsLock", Key::kCapsLock },
    { "NumberLock", Key::kNumLock },  // TODO: test
    { "ArrowLeft", Key::kLeft },
    { "ArrowRight", Key::kRight },
    { "ArrowUp", Key::kUp },
    { "ArrowDown", Key::kDown },
    { "Home", Key::kHome },
    { "End", Key::kEnd },
    { "PageUp", Key::kPageUp },
    { "PageDown", Key::kPageDown },
    { "F1", Key::kF1 },
    { "F2", Key::kF2 },
    { "F3", Key::kF3 },
    { "F4", Key::kF4 },
    { "F5", Key::kF5 },
    { "F6", Key::kF6 },
    { "F7", Key::kF7 },
    { "F8", Key::kF8 },
    { "F9", Key::kF9 },
    { "F10", Key::kF10 },
    { "F11", Key::kF11 },
    { "F12", Key::kF12 },
    { "PrintScreen", Key::kPrintScreen }  // TODO: test
};

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
        mLastClickTime = std::chrono::time_point<std::chrono::steady_clock>();
        mLastClickWindow = 0;
        mButton = MouseButton::kNone;
        mNClicks = 0;
    }

    int click(WASMWindow *w, const MouseEvent& e)
    {
        if (!w) {  // should never happen, but prevents a crash with maxDistPx
            reset();
            return 0;
        }

        auto dpi = w->dpi();
        int maxRadiusPx = std::max(1, int(std::round(kDoubleClickMaxRadiusPicaPt.toPixels(dpi))));

        auto now = std::chrono::steady_clock::now();
        auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - mLastClickTime).count();

        if (w != mLastClickWindow || e.button.button != mButton
            || std::abs((e.pos.x - mLastClickX).toPixels(dpi)) > maxRadiusPx
            || std::abs((e.pos.y - mLastClickY).toPixels(dpi)) > maxRadiusPx
            || dt > kDoubleClickMaxMillisecs) {
            mButton = e.button.button;
            mNClicks = 0;
        }
        mNClicks++;

        mLastClickTime = now;
        mLastClickWindow = w;
        mLastClickX = e.pos.x;
        mLastClickY = e.pos.y;

        return mNClicks;
    }

private:
    MouseButton mButton;
    int mNClicks;
    std::chrono::time_point<std::chrono::steady_clock> mLastClickTime;
    WASMWindow *mLastClickWindow;  // we do not own this
    PicaPt mLastClickX;
    PicaPt mLastClickY;
};

} // namespace

int OnJSResize(int eventType, const EmscriptenUiEvent *e, void *userData);
void OnJSEvent(EmVal e);

enum class JSTextEventType { kInput, kComposeStart, kComposeUpdate, kComposeEnd };

EMSCRIPTEN_BINDINGS(uitk) {
    emscripten::function("OnJSEvent", &OnJSEvent);
}

struct CallbackInfo
{
    std::function<void(const std::string&, EmVal, void*)> callback;
    void *obj;
};
std::map<std::string, std::map<std::string, CallbackInfo>> gJSEventCallbacks;

CallbackInfo* findCallback(const std::string& target, const std::string& event)
{
    auto it = gJSEventCallbacks.find(target);
    if (it != gJSEventCallbacks.end()) {
        auto evIt = it->second.find(event);
        if (evIt != it->second.end()) {
            return &evIt->second;
        }
    }
    return nullptr;
 }

void OnJSEvent(EmVal e)
{
    EmVal currentTarget = e["currentTarget"];
    if (!currentTarget.as<bool>()) {  // paranoia; I doubt this happens
        return;
    }
    auto target = currentTarget["id"].as<std::string>();
    auto eventType = e["type"].as<std::string>();
    auto info = findCallback(target, eventType);
    if (info) {
        info->callback(eventType, e, info->obj);
    } else {
        std::cerr << "[error] event '" << eventType << "' received for element id '" << target << "', but no callback has been registered" << std::endl;
    }
}

void removeEventListener(EmVal jsObj, const std::string& event)
{
    auto target = jsObj["id"].as<std::string>();
    auto it = gJSEventCallbacks.find(target);
    if (it != gJSEventCallbacks.end()) {
        jsObj.call<void>("removeEventListener", EmVal(event), EmVal::module_property("OnJSEvent"));
        it->second.erase(event);
    }
}

void addEventListener(EmVal jsObj, const std::string& event, std::function<void(const std::string&, EmVal, void*)> cb, void *cbObj)
{
    auto target = jsObj["id"].as<std::string>();
    auto it = gJSEventCallbacks.find(target);
    if (it == gJSEventCallbacks.end()) {
        gJSEventCallbacks[target] = std::map<std::string, CallbackInfo>();
        it = gJSEventCallbacks.find(target);
    }
    auto evIt = it->second.find(event);
    if (evIt != it->second.end()) {
        removeEventListener(jsObj, event);
    }
    jsObj.call<void>("addEventListener", EmVal(event), EmVal::module_property("OnJSEvent"));
    it->second[event] = { cb, cbObj };
}

class WASMScreen {
public:
    WASMScreen()
    {
        mScreen.desktopFrame = { 0.0f, 0.0f, 0.0f, 0.0f };
        mScreen.fullscreenFrame = { 0.0f, 0.0f, 0.0f, 0.0f };
        mScreen.dpi = 0.0f;
    }

    WASMScreen(EmVal canvasElement)
    {
        mCanvas = canvasElement;
        if (!mCanvas["id"].as<bool>()) {
            mCanvas.set("id", EmVal(std::string("uitk_canvas_") + std::to_string(gNextUnnamedCanvasId++)));
        }
        mCanvas["style"].set("padding", "none");  // so mouse coords are accurate
        mCanvasId = mCanvas["id"].as<std::string>();  // store so c_str() stays valid
        const char *canvasId =  mCanvasId.c_str();

        // Canvas needs "tabindex" attribute in order to get key events. There
        // does not appear to be a way to tell if it is set to -1 in the HTML
        // or if it is unset. But since -1 means you cannot tab to it (but you
        // can get key events, IF it is explicitly set), that is probably not
        // we want, so just always set to 0.
        mCanvas.set("tabIndex", 0);

        // Get the drawing information
        refreshDC();

        // Get the operating system
        gOS = OS::kOther;
        EmVal navigator = EmVal::global("navigator");
        if (navigator.as<bool>()) {
            EmVal userAgentData = navigator["userAgentData"];
            EmVal platform = navigator["platform"];
            std::string osStr;
            if (userAgentData.as<bool>()) {
                osStr = userAgentData["platform"].as<std::string>();
            } else if (platform.as<bool>()) {
                osStr = platform.as<std::string>();
            }
            for (auto &c : osStr) {
                c = std::tolower(c);
            }
            //std::cout << "[debug] os str: '" << osStr << "'" << std::endl;

            if (!osStr.empty()) {
                if (osStr.find("win") == 0) {
                    gOS = OS::kWindows;
                } else if (osStr.find("mac") == 0) {
                    gOS = OS::kMacOS;
                } else if (osStr[0] == 'i') {
                    gOS = OS::kIOS;
                } else if (osStr.find("and") == 0) {
                    gOS = OS::kAndroid;
                } else if (osStr.find("linux") == 0) {
                    gOS = OS::kLinux;
                }
            }
        }

        // Create a HTML element for text entry, since it does not appear
        // that we can get a text event, especially a compose event, otherwise.
        EmVal document = EmVal::global("document");
        if (document.as<bool>()) {
            mTextEntryId = "__" + mCanvasId + "_text";

            mTextEntry = document.call<EmVal>("createElement", EmVal("textarea"));
            mTextEntry.set("id", mTextEntryId);
            mTextEntry.set("autocomplete", false);
            mTextEntry.set("autocorrect", false);
            mTextEntry.set("disabled", false);
            mTextEntry.set("spellcheck", false);
            mTextEntry["style"].set("display", "none");

            EmVal body = document["body"];
            if (body.as<bool>()) {
                body.call<void>("appendChild", mTextEntry);
            } else {
                std::cerr << "[uitk] Could not find 'body' in document; text entry will not work" << std::endl;
            }
        } else {
            std::cerr << "[uitk] Could not find global 'document'; text entry will not work" << std::endl;
        }

        // Set event callbacks (note that resize is only guaranteed to work
        // on the window). We do not use the Emscripten callbacks because
        // they appear to only work for Canvas elements, and Emscripten does not
        // have callbacks for the text events (input, composition*), so we need
        // the framework anyway.
        emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, (void*)this, false, WASMScreen::OnJSResize);
        addEventListener(mCanvas, "focusin", WASMScreen::onJSFocus, (void*)this);
        addEventListener(mCanvas, "focusout", WASMScreen::onJSFocus, (void*)this);
        addEventListener(mCanvas, "mousedown", WASMScreen::onJSMouse, (void*)this);
        addEventListener(mCanvas, "mouseup", WASMScreen::onJSMouse, (void*)this);
        addEventListener(mCanvas, "mousemove", WASMScreen::onJSMouse, (void*)this);
        addEventListener(mCanvas, "wheel", WASMScreen::onJSMouse, (void*)this);

        // Requires that the canvas have `tabindex="-1"` in the HTML and also
        // that the canvas be focused in order to receive an event.
        addEventListener(mCanvas, "keydown", WASMScreen::onJSKey, (void*)this);
        addEventListener(mCanvas, "keyup", WASMScreen::onJSKey, (void*)this);

        if (!mTextEntryId.empty()) {
            addEventListener(mTextEntry, "focusin", WASMScreen::onJSFocus, (void*)this);
            addEventListener(mTextEntry, "focusout", WASMScreen::onJSFocus, (void*)this);
            addEventListener(mTextEntry, "mousedown", WASMScreen::onJSMouse, (void*)this);
            addEventListener(mTextEntry, "mouseup", WASMScreen::onJSMouse, (void*)this);
            addEventListener(mTextEntry, "mousemove", WASMScreen::onJSMouse, (void*)this);
            addEventListener(mTextEntry, "wheel", WASMScreen::onJSMouse, (void*)this);
            addEventListener(mTextEntry, "keydown", WASMScreen::onJSKey, (void*)this);
            addEventListener(mTextEntry, "keyup", WASMScreen::onJSKey, (void*)this);
            addEventListener(mTextEntry, "input", WASMScreen::onJSInput, (void*)this);
            addEventListener(mTextEntry, "compositionstart", WASMScreen::onJSComposeStart, (void*)this);
            addEventListener(mTextEntry, "compositionupdate", WASMScreen::onJSComposeUpdate, (void*)this);
            addEventListener(mTextEntry, "compositionend", WASMScreen::onJSComposeEnd, (void*)this);
        }
    }

    ~WASMScreen()
    {
        setTextEditing(nullptr, Rect::kZero);

        const char *canvasId =  mCanvasId.c_str();
        std::cout << "[debug] ~WASMScreen: " << canvasId << std::endl;

        // Clear the canvas, rather than leave defunct pixels. This is sort of
        // a waste, since it would only happen when the webpage exits, but it
        // will make any bugs clearer.
        auto &osr = mScreen.fullscreenFrame;
        mDC->beginDraw();
        mDC->clearRect(Rect::fromPixels(osr.x, osr.y, osr.width, osr.height, mScreen.dpi));
        mDC->endDraw();

        emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, (void*)this, false, nullptr);
        removeEventListener(mCanvas, "focusin");
        removeEventListener(mCanvas, "focusout");
        removeEventListener(mCanvas, "mousedown");
        removeEventListener(mCanvas, "mouseup");
        removeEventListener(mCanvas, "mousemove");
        removeEventListener(mCanvas, "wheel");
        removeEventListener(mCanvas, "keydown");
        removeEventListener(mCanvas, "keyup");

        if (!mTextEntryId.empty()) {
            removeEventListener(mTextEntry, "focusin");
            removeEventListener(mTextEntry, "focusout");
            removeEventListener(mTextEntry, "mousedown");
            removeEventListener(mTextEntry, "mouseup");
            removeEventListener(mTextEntry, "mousemove");
            removeEventListener(mTextEntry, "wheel");
            removeEventListener(mTextEntry, "keydown");
            removeEventListener(mTextEntry, "keyup");
            removeEventListener(mTextEntry, "input");
            removeEventListener(mTextEntry, "compositionstart");
            removeEventListener(mTextEntry, "compositionupdate");
            removeEventListener(mTextEntry, "compositionend");

            EmVal document = EmVal::global("document");
            if (document.as<bool>()) {
                EmVal body = document["body"];
                if (body.as<bool>()) {
                    body.call<void>("removeChild", mTextEntry);
                }
            }
            mTextEntry = EmVal();
        }
    }

    void refreshDC()
    {
        mDC = nullptr;  // force delete now, otherwise next line asserts
        mDC = DrawContext::fromHTMLCanvas(mCanvasId.c_str());
        mScreen.desktopFrame = { 0.0f, 0.0f, float(mDC->width()), float(mDC->height()) };
        mScreen.fullscreenFrame = mScreen.desktopFrame;
        mScreen.dpi = mDC->dpi();
        mNeedsDraw = true;
    }

    bool isValid() const { return mScreen.dpi > 0.0f; }

    const OSScreen& screen() const { return mScreen; }

    DrawContext* drawContext() { return mDC.get(); }

    const Point& currentMouseLocation() const { return mCurrentMousePos; }

    void addWindow(WASMWindow *w)
    {
        if (!mWindows.empty()) {
        }

        auto insertAt = mWindows.rbegin();
        if (!mWindows.empty()) {
            if (!(w->flags() & Window::Flags::kDialog)) {
                while (insertAt != mWindows.rend() && (insertAt->window->flags() & Window::Flags::kDialog)) {
                    ++insertAt;
                }
            }
        }
        mWindows.insert(insertAt.base(), WindowInfo{ w, { 0.0f, 0.0f, 0.0f, 0.0f }, w->isShowing() });
    }

    void removeWindow(WASMWindow *w)
    {
        auto it = findWindow(w);
        if (it != mWindows.end()) {
            // Do not call it->window->onDeactived: this is probably in
            // the window's destructor, and in any case, the window is
            // removed, so whatever it does is never going to be shown.
            if (it->isVisible) {
                postRedraw();
            }
            mWindows.erase(it);
        }
        if (!mWindows.empty()) {
            mWindows.back().window->onActivated();
        }
    }

    void showWindow(WASMWindow *w, bool show)
    {
        auto *prevActiveInfo = activeWindowInfo();

        auto it = findWindow(w);
        if (it != mWindows.end()) {
            bool wasVisible = it->isVisible;
            it->isVisible = show;
            if (wasVisible != show) {
                if (show) {
                    auto *currentActiveInfo = activeWindowInfo();
                    if (currentActiveInfo && currentActiveInfo->window == w) {
                        if (prevActiveInfo) {
                            prevActiveInfo->window->onDeactivated();
                        }
                        it->window->onActivated();
                    }
                } else {
                    it->window->onDeactivated();
                }
                postRedraw();
            }
        }
    }

    void setWindowFrame(WASMWindow *w, const Rect& frame)
    {
        auto it = findWindow(w);
        if (it != mWindows.end()) {
            it->frame.x = frame.x.toPixels(kCSSDPI);
            it->frame.y = frame.y.toPixels(kCSSDPI);
            it->frame.width = frame.width.toPixels(kCSSDPI);
            it->frame.height = frame.height.toPixels(kCSSDPI);
            postResize();
            postRedraw();
        }
    }

    enum FocusAction { kFocusCanvasWhenNotEditing, kDoNotChangeFocus };
    void setTextEditing(TextEditorLogic *te, const Rect& frame,
                        FocusAction focusAction = kFocusCanvasWhenNotEditing)
    {
        if (mTextEntryId.empty()) {
            return;
        }

        if (!te || frame.isEmpty()) {
            // This is called every mouse click, do not want to be refocusing
            // all the time.
            if (mActiveTextLogic) {
                gTextInputScreen = nullptr;
                mActiveTextLogic = nullptr;
                mActiveTextRect = Rect::kZero;
                mTextEntry["style"].set("display", "none");
                if (mIsActive && focusAction == kFocusCanvasWhenNotEditing) {
                    mCanvas.call<void>("focus");
                    mNeedsFocus = mCanvasId;  // the call to focus seems to get ignored
                }
            }
        } else {
            EmVal window = EmVal::global("window");
            EmVal canvasRect = mCanvas.call<EmVal>("getBoundingClientRect");
            float x = canvasRect["left"].as<float>() + window["scrollX"].as<float>();
            float y = canvasRect["top"].as<float>() + window["scrollY"].as<float>();
            EmVal style = mTextEntry["style"];
            style.set("display", "initial");
            style.set("padding", "none");  // so mouse coords are accurate
            style.set("resize", "none");
            style.set("position", "absolute");
            style.set("left", x);
            style.set("top", y);
            style.set("width", canvasRect["width"]);
            style.set("height", canvasRect["height"]);
            style.set("zIndex", 100000);
            style.set("background-color", "transparent");
            style.set("color", "transparent");
            style.set("cursor", "text");
            style.set("font", std::string("1px sans-serif"));  // so IME is in a predictable location
            mTextEntry.call<void>("focus");
            mNeedsFocus = mTextEntryId;
            gTextInputScreen = this;
            mActiveTextLogic = te;
            mActiveTextRect = frame;
        }
        mNeedsDraw = true;
    }

    void setCursor(const WASMCursor& cursor)
    {
        auto styleAttr = mCanvas["style"];
        if (styleAttr.as<bool>()) {
            if (cursor.isSystemCursor()) {
                auto *cssCursor = "default";
                switch (cursor.systemCursorId()) {
                    case OSCursor::System::kArrow:
                        cssCursor = "default";
                        break;
                    case OSCursor::System::kIBeam:
                        cssCursor = "text";
                        break;
                    case OSCursor::System::kCrosshair:
                        cssCursor = "crosshair";
                        break;
                    case OSCursor::System::kOpenHand:
                        cssCursor = "grab";
                        break;
                    case OSCursor::System::kClosedHand:
                        cssCursor = "grabbing";
                        break;
                    case OSCursor::System::kPointingHand:
                        cssCursor = "pointer";
                        break;
                    case OSCursor::System::kResizeLeftRight:
                        cssCursor = "col-resize";
                        break;
                    case OSCursor::System::kResizeUpDown:
                        cssCursor = "row-resize";
                        break;
                    case OSCursor::System::kResizeNWSE:
                        cssCursor = "nwse-resize";
                        break;
                    case OSCursor::System::kResizeNESW:
                        cssCursor = "nesw-resize";
                        break;
                    case OSCursor::System::kForbidden:
                        cssCursor = "not-allowed";
                        break;
                    case OSCursor::System::kLast:
                        std::cerr << "[error] Cursor::kLast is not a valid cursor" << std::endl;
                        cssCursor = "default";
                        break;
                }
                styleAttr.set("cursor", EmVal(std::string(cssCursor)));
                if (!mTextEntryId.empty()) {
                    mTextEntry["style"].set("cursor", EmVal(std::string(cssCursor)));
                }
            }
        }
    }

    void raiseWindow(const WASMWindow *w)
    {
        if (auto *active = activeWindowInfo()) {
            active->window->onDeactivated();
        }
        // HACK: cast away constness, but this is okay because we are not
        //       actually calling anything with the pointer, just using it
        //       for comparison. The const is only so that const functions
        //       can all this.
        removeWindow((WASMWindow*)w);
        addWindow((WASMWindow*)w);
        if (auto *active = activeWindowInfo()) {  // w might not be at back()
            active->window->onActivated();        // if dialog is showing
        }
    }

    void postResize()
    {
        mNeedsResize = true;
    }

    void postRedraw()
    {
        mNeedsDraw = true;
    }

    void tick()
    {
        // When we set focus within an event sometimes it seems to get ignored.
        // So check that on a tick and focus if necessary.
        if (!mNeedsFocus.empty()) {
            if (mNeedsFocus == mCanvasId) {
                mCanvas.call<void>("focus");
            } else if (mNeedsFocus == mTextEntryId) {
                mTextEntry.call<void>("focus");
            }
            mNeedsFocus = "";
        }

        if (mNeedsResize) {
            resize();
        }
        if (mNeedsDraw) {
            draw();
        }
    }

    void activated(const std::string& prevFocus, const std::string& newFocus)
    {
        auto *wi = activeWindowInfo();
        if (!wi) {
            return;
        }

        if (!mIsActive) {
            mIsActive = true;
            wi->window->onActivated();
        }
    }

    void deactivated(const std::string& prevFocus, const std::string& newFocus)
    {
        auto *wi = activeWindowInfo();
        if (!wi) {
            return;
        }

        if (newFocus == mCanvasId || newFocus == mTextEntryId) {
            return;
        }

        EmVal document = EmVal::global("document");
        if (document.as<bool>()) {
            mIsActive = false;

            // Note that we do not get a focusout event when the window becomes
            // deactive if we were not focused to begin with, in which case
            // we would still consider ourselves active and show the accent
            // color. But it is not clear what we can do about that.
            if (!document.call<bool>("hasFocus")) {
                wi->window->onDeactivated();
            }

            // Always end text editing when deactivated, since it looks confusing
            // to have a selection area and caret when the text is going
            // somewhere else. On macOS this is slightly different than the
            // native behavior, which returns the keyfocus when the window is
            // reactivated, but since this is within the browser's window, we'd
            // need to have a third, partially-active theme state, which would
            // keep the accent color but not draw the focus ring. This is easier.
            if (mActiveTextLogic) {
                mActiveTextLogic->commit();
                setTextEditing(nullptr, Rect(), kDoNotChangeFocus);
            }
        }
    }

    void resize()
    {
        if (!mIsResizing) {
            mIsResizing = true;
            for (const auto &wi : mWindows) {
                wi.window->onResize();
            }
            mIsResizing = false;
            mNeedsResize = false;
        }
    }

    void draw()
    {
        if (!mIsDrawing) {
            mIsDrawing = true;
            for (const auto &wi : mWindows) {
                if (wi.isVisible) {
                    wi.window->onDraw();
                }
            }
            mIsDrawing = false;
            mNeedsDraw = false;
        }
    }

    void mouse(MouseEvent e, long x, long y)
    {
        auto *wi = activeWindowInfo();
        if (!wi) {
            return;
        }

        if (e.type == MouseEvent::Type::kButtonDown) {
            mCanvas.call<void>("focus");
            // Update nClicks
            e.button.nClicks = mClickCounter.click(wi->window, e);
        }

        // If mouse down or move outside of window, send the event to the window
        // it hit. (Otherwise menus will not cancel properly: down outside the
        // menu to cancel, move on parent to cancel submenu.)
        if ((e.type == MouseEvent::Type::kButtonDown ||
             e.type == MouseEvent::Type::kMove) &&
            (float(x) < wi->frame.x || float(y) < wi->frame.y ||
             float(x) > wi->frame.x + wi->frame.width ||
             float(y) > wi->frame.y + wi->frame.height))
        {
            for (auto it = mWindows.rbegin();  it != mWindows.rend();  ++it) {
                if (it->isVisible && (it->window->flags() & Window::Flags::kDialog) == 0) {
                
                    if ((float(x) >= it->frame.x && float(y) >= it->frame.y &&
                         float(x) <= it->frame.x + it->frame.width &&
                         float(y) <= it->frame.y + it->frame.height))
                    {
                        wi = &(*it);
                        break;
                    }
                }
            }
        }

        // (x, y) is in CSS units
        mCurrentMousePos = Point::fromPixels(x, y, kCSSDPI);
        e.pos = Point::fromPixels(x - long(wi->frame.x), y - long(wi->frame.y), kCSSDPI);
        wi->window->onMouse(e);

        // If we just update the IME position on the compose start event it can
        // take several tries to actually get it to move. Setting the padding
        // here, before the IME is opened seems to fix that.
        if (mActiveTextLogic && e.type == MouseEvent::Type::kButtonDown) {
            updateIMEPosition();
        }
    }

    void wheel(MouseEvent e)
    {
        auto *wi = activeWindowInfo();
        if (!wi) {
            return;
        }

        e.pos = mCurrentMousePos;

        wi->window->onMouse(e);
    }

    void key(const KeyEvent& e)
    {
        mClickCounter.reset();

        auto *wi = activeWindowInfo();
        if (!wi) {
            return;
        }

        wi->window->onKey(e);
    }

    void text(JSTextEventType type, EmVal e)
    {
        auto *wi = activeWindowInfo();
        if (!wi) {
            return;
        }

        switch (type)
        {
            case JSTextEventType::kInput: {
                // .data is empty for backspace, enter, etc
                if (e["data"].as<bool>() && !e["isComposing"].as<bool>()) {
                    auto utf8 = e["data"].as<std::string>();
                    wi->window->onText(TextEvent{ utf8 });
                    mTextEntry.set("value", EmVal(""));
                }
                break;
            }
            case JSTextEventType::kComposeStart:
                updateIMEPosition();
                break;
            case JSTextEventType::kComposeUpdate:
                if (mActiveTextLogic && e["data"].as<bool>()) {
                    auto utf8 = e["data"].as<std::string>();
                    mActiveTextLogic->setIMEConversion(TextEditorLogic::IMEConversion(mActiveTextLogic->selection().start, e["data"].as<std::string>()));
                }
                break;
            case JSTextEventType::kComposeEnd:
                if (mActiveTextLogic) {
                    auto utf8 = e["data"].as<std::string>();
                    mActiveTextLogic->setIMEConversion(TextEditorLogic::IMEConversion());
                    mActiveTextLogic->insertText(utf8);
                }
                break;
        }
    }

public:
    static WASMScreen *gTextInputScreen;

private:
    ClickCounter mClickCounter;

    EmVal mCanvas;
    std::string mCanvasId;
    EmVal mTextEntry;
    std::string mTextEntryId;
    OSScreen mScreen;
    std::shared_ptr<DrawContext> mDC;
    // Q: Why vector instead of, say, list?
    // A: Iterating needs to be fast, because that is what we do most often.
    //    Occasionally we need an O(n) iteration to alter a window, but that
    //    happens in response to user interactions (e.g. slow), and there are
    //    not going to be very many windows, anyway.
    struct WindowInfo {
        WASMWindow *window;  // this is a ref
        OSRect frame;
        bool isVisible;
    };
    std::vector<WindowInfo> mWindows;
    Point mCurrentMousePos; // in screen coordinates
    std::unordered_map<Key, int> mCurrentKeymods;
    TextEditorLogic *mActiveTextLogic = nullptr;  // ref; we do not own
    Rect mActiveTextRect;
    std::string mNeedsFocus;
    bool mIsActive = false;
    bool mIsResizing = false;
    bool mIsDrawing = false;
    bool mNeedsResize = true;
    bool mNeedsDraw = true;

    std::vector<WindowInfo>::iterator findWindow(WASMWindow *w)
    {
        for (auto it = mWindows.begin();  it != mWindows.end();  ++it) {
            if (it->window == w) {
                return it;
            }
        }
        return mWindows.end();
    }

    WindowInfo* activeWindowInfo()
    {
        for (auto it = mWindows.rbegin();  it != mWindows.rend();  ++it) {
            if (it->isVisible) {
                return &(*it);
            }
        }
        return nullptr;
    }

    void updateIMEPosition()
    {
        if (mActiveTextLogic) {
            auto start = mActiveTextLogic->selection().start;
            auto r = mActiveTextLogic->glyphRectAtIndex(start);
            auto x = (r.x + mActiveTextRect.x).toPixels(kCSSDPI);
            auto y = (r.y + mActiveTextRect.maxY()).toPixels(kCSSDPI);
            // The IME of the <textarea> is shown at the bottom of the
            // glyph. When we created the textarea we gave it a font
            // size of 1px (CSS) so that the size is predictable
            // (a 10px font is not always 10px, but the error on a 1px
            // font will be acceptable).
            auto style = mTextEntry["style"];  // this is a JS ref
            style.set("paddingLeft", std::to_string(x) + "px");
            style.set("paddingTop", std::to_string(y - 1.0f) + "px");
        }
    }

    static int OnJSResize(int eventType, const EmscriptenUiEvent *e, void *userData)
    {
        if (eventType == EMSCRIPTEN_EVENT_RESIZE) {
            ((WASMScreen*)userData)->refreshDC();
        }
        return 1;  // true: consumed event
    }

    static void onJSFocus(const std::string& event, EmVal e, void *obj)
    {
        static const std::string kFocusIn = "focusin";
        static const std::string kFocusOut = "focusout";


        std::string related, target;
        EmVal relatedElem = e["relatedTarget"];
        if (relatedElem.as<bool>()) {
            related = relatedElem["id"].as<std::string>();
        }
        EmVal targetElem = e["target"];
        if (targetElem.as<bool>()) {
            target = targetElem["id"].as<std::string>();
        }

        if (event == kFocusIn) {
            ((WASMScreen*)obj)->activated(related, target);
        } else if (event == kFocusOut) {
            ((WASMScreen*)obj)->deactivated(target, related);
        }
    }

    static void onJSMouse(const std::string& event, EmVal e, void *obj)
    {
        // Avoid creating a new string each call just to compare each mouse event
        static const std::string kMouseMove = "mousemove";
        static const std::string kMouseDown = "mousedown";
        static const std::string kMouseUp = "mouseup";
        static const std::string kWheel = "wheel";


        MouseEvent me;
        auto dragButtons = e["buttons"].as<unsigned short>();
        auto t = e["type"].as<std::string>();
        if (t == kMouseMove) {
            if (dragButtons == 0) {
                me.type = MouseEvent::Type::kMove;
            } else {
                me.type = MouseEvent::Type::kDrag;
            }
        } else if (t == kMouseDown) {
            me.type = MouseEvent::Type::kButtonDown;
        } else if (t == kMouseUp) {
            me.type = MouseEvent::Type::kButtonUp;
        } else if (t == kWheel) {
            me.type = MouseEvent::Type::kScroll;
        } else {
            return;
        }

        me.keymods = 0;
        if (e["shiftKey"].as<bool>()) { me.keymods |= KeyModifier::kShift; }
        if (e["altKey"].as<bool>()) { me.keymods |= KeyModifier::kAlt; }
        if (gOS == OS::kMacOS || gOS == OS::kIOS) {  // iOS may have keyboard too
            if (e["ctrlKey"].as<bool>()) { me.keymods |= KeyModifier::kMeta; }
            if (e["metaKey"].as<bool>()) { me.keymods |= KeyModifier::kCtrl; }
        } else {
            if (e["ctrlKey"].as<bool>()) { me.keymods |= KeyModifier::kCtrl; }
            if (e["metaKey"].as<bool>()) { me.keymods |= KeyModifier::kMeta; }
        }

        if (me.type == MouseEvent::Type::kButtonDown || me.type == MouseEvent::Type::kButtonUp) {
            int b = e["button"].as<unsigned short>();
            switch (b) {
                case 0:  me.button.button = MouseButton::kLeft;  break;
                case 1:  me.button.button = MouseButton::kMiddle;  break;
                case 2:  me.button.button = MouseButton::kRight;  break;
                case 3:  me.button.button = MouseButton::kButton4;  break;
                case 4:  me.button.button = MouseButton::kButton5;  break;
                default: me.button.button = MouseButton::kNone;  break;
            }
            me.button.nClicks = 1; // mouse() will fix
        } else if (me.type == MouseEvent::Type::kDrag) {
            int b = 0;
            if (dragButtons & 0b00001) { b |= int(MouseButton::kLeft); }
            if (dragButtons & 0b00010) { // Note: different order than "button"!
                b |= int(MouseButton::kRight);
            }
            if (dragButtons & 0b00100) { b |= int(MouseButton::kMiddle); }
            if (dragButtons & 0b01000) { b |= int(MouseButton::kButton4); }
            if (dragButtons & 0b10000) { b |= int(MouseButton::kButton5); }
            me.drag.buttons = b;
        }
        auto x = e["offsetX"].as<long>();  // from edge of textarea frame 
        auto y = e["offsetY"].as<long>();

        // A WheelEvent isa MouseEvent, but we also need to process the WheelEvent part
        if (me.type == MouseEvent::Type::kScroll) {
            auto toPica = PicaPt::fromStandardPixels(96.0f);
            switch (e["deltaMode"].as<unsigned long>()) {
                default:  // fall through
                case DOM_DELTA_PIXEL:
                    toPica = PicaPt::fromPixels(1.0f, kCSSDPI);
                    break;
                case DOM_DELTA_LINE: {
                    auto em = Application::instance().theme()->params().labelFont.pointSize();
                    toPica = em;
                    break;
                }
                case DOM_DELTA_PAGE: {
                    auto em = Application::instance().theme()->params().labelFont.pointSize();
                    toPica = em * 5.0f;
                    break;
                }
            }

            me.scroll.dx = -e["deltaX"].as<float>() * toPica;
            me.scroll.dy = -e["deltaY"].as<float>() * toPica;
        }

        // Do not call preventDefault on non-wheel events if the target is a
        // textarea or IME composition will stop working. Definitely do call
        // it when it is a wheel event, otherwise the page will bounce on macOS.
        if (me.type == MouseEvent::Type::kScroll) {
            e.call<void>("preventDefault");
        }

        ((WASMScreen*)obj)->mouse(me, x, y);
    }

    static void onJSKey(const std::string& event, EmVal e, void *obj)
    {
        // Avoid creating a new string each call just to compare each mouse event
        static const std::string kKeyDown = "keydown";
        static const std::string kKeyUp = "keyup";

        KeyEvent ke;
        auto eventType = e["type"].as<std::string>();
        if (eventType == kKeyDown) {
            ke.type = KeyEvent::Type::kKeyDown;
        } else if (eventType == kKeyUp) {
            ke.type = KeyEvent::Type::kKeyUp;
        } else {
            std::cerr << "[error] Unknown Emscripten key event type " << eventType << std::endl;
            return;  // ignore
        }
        ke.isRepeat = (e["repeat"].as<bool>() != 0);
        ke.keymods = 0;
        if (e["shiftKey"].as<bool>()) { ke.keymods |= KeyModifier::kShift; }
        if (e["altKey"].as<bool>())   { ke.keymods |= KeyModifier::kAlt; }
        if (gOS == OS::kMacOS || gOS == OS::kIOS) {  // iOS may have keyboard too
            if (e["ctrlKey"].as<bool>()) { ke.keymods |= KeyModifier::kMeta; }
            if (e["metaKey"].as<bool>()) { ke.keymods |= KeyModifier::kCtrl; }
        } else {
            if (e["ctrlKey"].as<bool>()) { ke.keymods |= KeyModifier::kCtrl; }
            if (e["metaKey"].as<bool>()) { ke.keymods |= KeyModifier::kMeta; }
        }

        auto it = kDOMKeymaps.find(e["code"].as<std::string>());
        if (it != kDOMKeymaps.end()) {
            ke.key = it->second;
        } else {
            ke.key = Key::kUnknown;
        }

        ((WASMScreen*)obj)->key(ke);
    }

    static void onJSInput(const std::string& event, EmVal e, void *obj)
    {
        ((WASMScreen*)obj)->text(JSTextEventType::kInput, e);
    }

    static void onJSComposeStart(const std::string& event, EmVal e, void *obj)
    {
        ((WASMScreen*)obj)->text(JSTextEventType::kComposeStart, e);
    }

    static void onJSComposeUpdate(const std::string& event, EmVal e, void *obj)
    {
        ((WASMScreen*)obj)->text(JSTextEventType::kComposeUpdate, e);
    }

    static void onJSComposeEnd(const std::string& event, EmVal e, void *obj)
    {
        ((WASMScreen*)obj)->text(JSTextEventType::kComposeEnd, e);
    }
};
WASMScreen* WASMScreen::gTextInputScreen = nullptr;

int OnJSResize(int eventType, const EmscriptenUiEvent *e, void *userData)
{
    if (eventType == EMSCRIPTEN_EVENT_RESIZE) {
        ((WASMScreen*)userData)->refreshDC();
    }
    return 1;  // true: consumed event (if no widget consumed it, the window did)
}
//-----------------------------------------------------------------------------
struct WASMApplication::Impl
{
    std::unique_ptr<WASMScreen> screen;
    std::unique_ptr<WASMClipboard> clipboard;
    std::unique_ptr<WASMSound> sound;
    DeferredFunctions<WASMWindow*> postedLater;
    bool inTick = false;

    Impl() : clipboard(std::make_unique<WASMClipboard>()) {}
};
    
void WASMApplication::onTick()  // static
{
    WASMApplication *app = (WASMApplication*)&Application::instance().osApplication();
    if (app->mImpl->inTick) {
        return;
    }

    app->mImpl->inTick = true;

    app->mImpl->postedLater.executeTick();

    auto &screen = *app->mImpl->screen.get();
    screen.tick();

    app->mImpl->inTick = false;
}

WASMApplication::WASMApplication()
    : mImpl(new Impl())
{
    // Need to set the canvas before the window is created (since it needs the
    // dpi).

    // TODO: Unless it would work to send a message to the window saying that
    //       has a new dpi? Like it might if it got moved to a new screen?
    //       But that seems a tad unnecessary, to get config messages about one
    //       dpi, and then almost immediately after get some for a potentially
    //       different dpi.

    auto document = EmVal::global("document");
    auto canvases = document.call<EmVal, std::string>("getElementsByTagName", "canvas");
    if (!canvases.as<bool>()) {
        std::cerr << "[error] could not find any canvas elements in document" << std::endl;
    }
    if (!canvases["length"].as<bool>() || canvases["length"].as<int>() == 0) {
        std::cerr << "[error] could not find any canvas elements in document (empty array)" << std::endl;
    }
    if (!canvases[0].as<bool>()) {
        std::cerr << "[error] first canvas element in document is not a valid object!" << std::endl;
    }
    mImpl->screen = std::make_unique<WASMScreen>(canvases[0]);
    mImpl->sound = std::make_unique<WASMSound>();
}

WASMApplication::~WASMApplication()
{
}

void WASMApplication::setExitWhenLastWindowCloses(bool exits)
{
    // No-op: cannot close last window in a web page (no way to get it back)
}

int WASMApplication::run()
{
    emscripten_set_main_loop(WASMApplication::onTick,
                             0,  // Let browser determine frame rate
                             1); // Simulate infinite loop (so main() does not
                                 //   exit and cause Application to destruct
    return 0;
}

void WASMApplication::exitRun()
{
    emscripten_cancel_main_loop();
}

void WASMApplication::scheduleLater(Window* w, std::function<void()> f)
{
    scheduleLater(w, 0, false, [f](SchedulingId) { f(); });
}

OSApplication::SchedulingId WASMApplication::scheduleLater(
                                            Window* w, float delay, bool repeat,
                                            std::function<void(SchedulingId)> f)
{
    WASMWindow *nh = nullptr;
    if (w) {
        nh = (WASMWindow*)(w->nativeHandle());
    }
    return mImpl->postedLater.add(nh, delay, repeat, f);
}

void WASMApplication::cancelScheduled(SchedulingId id)
{
    mImpl->postedLater.remove(id);
}

std::string WASMApplication::applicationName() const
{
    return "App";  // This is only used on menus on macOS
}

std::string WASMApplication::tempDir() const
{
    return "./";  // This will be in Emscripten's filesystem
}

std::vector<std::string> WASMApplication::availableFontFamilies() const
{
    return Font::availableFontFamilies();
}

void WASMApplication::beep()
{
    const float twoPi = 2.0f * 3.141592f;
    float rate = 44000.0f;
    float lengthSec = 1.0f;
    float masterVolume = 0.5f;

    // Produces a digital piano sound for the given frequency

    float freqHz = 220.0f;  // A3
    std::vector<float> volumes = { 1.0f, 0.5f, 0.25f, 0.125f, 0.0625f, 0.03125f };

    lengthSec = std::ceil(lengthSec * freqHz) / freqHz;  // force an integer number of cycles
    std::vector<int16_t> samples((int)std::round(rate * lengthSec));
    for (size_t i = 0;  i < samples.size();  ++i) {
        const float sec = float(i) / rate;
        float decay = std::exp(-7.0f * sec / lengthSec);
        float v = 0.0f;
        for (size_t i = 0;  i < volumes.size();  ++i) {
            v += volumes[i] * decay * std::sin(float(i) * freqHz * twoPi * sec);
        }
        v *= masterVolume;
        samples[i] = int16_t(std::round(float(INT16_MAX) * v));
    }

    sound().play(samples.data(), samples.size(), int(rate), 1);
}

OSSound& WASMApplication::sound() const
{
    return *mImpl->sound;
}

void WASMApplication::debugPrint(const std::string& s)
{
    std::cout << s << std::endl;
}

bool WASMApplication::isOriginInUpperLeft() const { return true; }

bool WASMApplication::isWindowBorderInsideWindowFrame() const { return true; }

bool WASMApplication::windowsMightUseSameDrawContext() const { return true; }

bool WASMApplication::shouldHideScrollbars() const { return false; }

bool WASMApplication::canKeyFocusEverything() const { return true; }

bool WASMApplication::platformHasMenubar() const { return false; }

Clipboard& WASMApplication::clipboard() const
{
    return *mImpl->clipboard;
}

Theme::Params WASMApplication::themeParams() const
{
    auto window = EmVal::global("window");
    auto document = EmVal::global("document");
    if (window.as<bool>() && document.as<bool>()) {
        auto body = document["body"];
        if (body.as<bool>()) {
            auto style = window.call<EmVal>("getComputedStyle", body);
            if (style.as<bool>()) {
                auto bgColor = Color::fromCSS(style["background-color"].as<std::string>().c_str());
                auto fgColor = Color::fromCSS(style["color"].as<std::string>().c_str());
                // To avoid browser fingerprinting, browsers do not return an
                // accurate value for accent-color if it is set to "highlight".
                auto accentColorStr = style["accent-color"].as<std::string>();
                bool success = false;
                auto accentColor = Color::fromCSS(accentColorStr.c_str(), &success);
                if (!success) {
                    accentColor = EmpireTheme::defaultParams().accentColor;
                }
                auto params = EmpireTheme::lightModeParams(accentColor);
                if (bgColor.alpha() > 0.001f) {
                    params = EmpireTheme::customParams(bgColor, fgColor, accentColor);
                }
                params.textColor = fgColor;
                params.labelFont = Font("system-ui", PicaPt(10.0f));
                params.nonNativeMenubarFont = params.labelFont;
                return params;
            }
        }
    }
    // If we failed to get a body style, assume the default of white background
    // and black text. (The default theme may not be light, but the default
    // window background is white.)
    return EmpireTheme::lightModeParams(EmpireTheme::defaultParams().accentColor);
}

void WASMApplication::registerWindow(WASMWindow *w)
{
    mImpl->screen->addWindow(w);
}

void WASMApplication::unregisterWindow(WASMWindow *w)
{
    mImpl->screen->removeWindow(w);
    mImpl->postedLater.removeForWindow(w);
}

void WASMApplication::setWindowFrame(WASMWindow *w, const Rect& frame)
{
    return mImpl->screen->setWindowFrame(w, frame);
}

void WASMApplication::showWindow(WASMWindow *w, bool show)
{
    mImpl->screen->showWindow(w, show);
}

void WASMApplication::raiseWindow(const WASMWindow *w)
{
    mImpl->screen->raiseWindow(w);
    postRedraw(w);
}

void WASMApplication::postRedraw(const WASMWindow *w)
{
    return mImpl->screen->postRedraw();
}

const OSScreen& WASMApplication::screenOfWindow(const WASMWindow *w)
{
    return mImpl->screen->screen();
}

DrawContext* WASMApplication::getDrawContext(WASMWindow *w)
{
    return mImpl->screen->drawContext();
}

const Point& WASMApplication::currentMouseLocation() const
{
    return mImpl->screen->currentMouseLocation();
}

void WASMApplication::setCursor(const WASMWindow *w, const WASMCursor& cursor)
{
    mImpl->screen->setCursor(cursor);
}

void WASMApplication::setTextEditing(TextEditorLogic *te, const Rect& frame)
{
    mImpl->screen->setTextEditing(te, frame);
}


} // namespace uitk
