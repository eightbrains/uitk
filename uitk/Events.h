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

#ifndef UITK_EVENTS_H
#define UITK_EVENTS_H

#define ND_NAMESPACE uitk
#include <nativedraw.h>

#include <string>

namespace uitk {

struct KeyModifier {
    enum Values {
        kNone = 0,
        kShift = (1 << 0),
        kCtrl = (1 << 1),  // this is the Command key on macOS
        kAlt = (1 << 2),   // this is the Option key on macOS
        kMeta = (1 << 3),  // this is the Control key on macOS
        // Note: caps lock and num lock are not included in key modifiers,
        //       because num lock on and Ctrl + V would fail if you checked
        //       using (mods & kCtrl) && (key == Key::kV), which is
        //       rather counter-intuitive
    };
};

enum class MouseButton {
    kNone = 0,
    kLeft =    1 << 0,
    kRight =   1 << 1,
    kMiddle =  1 << 2,
    kButton4 = 1 << 3,
    kButton5 = 1 << 4
};

struct MouseEvent
{
    enum class Type { kMove, kButtonDown, kDrag, kButtonUp, kScroll };

    Type type;
    Point pos;
    int keymods;
    union {
        struct {
            MouseButton button;
            int nClicks;
        } button;
        struct {
            int buttons;
        } drag;
        struct {
            PicaPt dx;
            PicaPt dy;
        } scroll;
    };

    // scroll has non-trivial member, so need a constructor.
    // But PicaPt has no invariants, so we do not need to initialize
    // anything.
    MouseEvent() {}
};

/// These are the major keys, but neither macOS nor Windows provides
/// definitions for much more than this. International keyboards provide
/// a wide variety of keys, and then there is the question of what a
/// key actually is. On a US keyboard the comma key is less-than when
/// shift is pressed, but on a European keyboard, shift + comma is
/// question mark. What key should shift + comma generate? Or if the
/// key should always be the unshifted version, what to do with slash,
/// which is unshifted on a US keyboard, but is shift + u-umlaut on
/// a European keyboard. So we have provided the special keys and the
/// keys that are fairly unambiguous.
enum class Key
{
    kNone = 0,
    kUnknown = 1,
    kBackspace = 8,
    kTab = 9,
    kEnter = 10,
    kReturn = 13,
    kEscape = 27,
    kSpace = 32,
    kNumMultiply = 42,
    kNumPlus = 43,
    kNumComma = 44,
    kNumMinus = 45,
    kNumPeriod = 46,
    kNumSlash = 47,
    k0 = 48, k1, k2, k3, k4, k5, k6, k7, k8, k9,
    kA = 97, kB, kC, kD, kE, kF, kG, kH, kI, kJ, kK, kL, kM,
    kN, kO, kP, kQ, kR, kS, kT, kU, kV, kW, kX, kY, kZ,
    kDelete = 127,
    kInsert,
    kShift = 512,
    kCtrl,
    kAlt,
    kMeta,
    kCapsLock,
    kNumLock,
    kLeft,
    kRight,
    kUp,
    kDown,
    kHome,
    kEnd,
    kPageUp,
    kPageDown,
    kF1, kF2, kF3, kF4, kF5, kF6, kF7, kF8, kF9, kF10, kF11, kF12,
    kPrintScreen
};

/// It is not possible to provide a key definition for keys that
/// represent text. As a result, key events are expected to be used
/// mainly for detecting special / non-text keys (e.g. left, home, F2).
/// They may also be used for games, although since the Key enum cannot
/// provide an exhaustive list, the nativeKey field can be used to
/// detect other keys. Key events should NOT be used for text input;
/// use TextEvent instead.
struct KeyEvent
{
    enum class Type { kKeyDown, kKeyUp };

    Type type;
    Key key;
    int nativeKey;  /// this is passed through from native events
    int keymods;
    bool isRepeat;
};

struct TextEvent
{
    std::string utf8;
};

}  // namespace uitk
#endif // UITK_EVENTS_H
