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

namespace uitk {

struct KeyModifier {
    enum Values {
        kNone = 0,
        kShift = (1 << 0),
        kCtrl = (1 << 1),  // this is the Command key on macOS
        kAlt = (1 << 2),   // this is the Option key on macOS
        kMeta = (1 << 3),  // this is the Control key on macOS
        kCapsLock = (1 << 4),
        kNumLock = (1 << 5)
    };
};

enum class MouseButton {
    kNone = 0, kLeft, kRight, kMiddle, kButton4, kButton5
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
            bool shouldHideScrollbars;
        } scroll;
    };

    // scroll has non-trivial member, so need a constructor.
    // But PicaPt has no invariants, so we do not need to initialize
    // anything.
    MouseEvent() {}
};

enum class Key
{
    kNone = 0,
    kBackspace = 8,
    kTab = 9,
    kEnter = 10,
    kReturn = 13,
    kEscape = 27,
    kSpace = 32,
    kExclamation,
    kDoubleQuote,
    kPound,
    kDollar,
    kPercent,
    kAmpersand,
    kQuote,
    kLeftParen,
    kRightParen,
    kAsterisk,
    kPlus,
    kComma,
    kMinus,
    kPeriod,
    kSlash,
    k0 = 48, k1, k2, k3, k4, k5, k6, k7, k8, k9,
    kColon = 58,
    kSemicolon,
    kLessThan,
    kEquals,
    kGreaterThan,
    kQuestionMark,
    kAt,
    kShiftA = 65, kShiftB, kShiftC, kShiftD, kShiftE, kShiftF, kShiftG,
    kShiftH, kShiftI, kShiftJ, kShiftK, kShiftL, kShiftM, kShiftN, kShiftO, kShiftP,
    kShiftQ, kShiftR, kShiftS, kShiftT, kShiftU, kShiftV, kShiftW, kShiftX,
    kShiftY, kShiftZ,
    kLeftBracket,
    kBackslash,
    kRightBracket,
    kCirconflex,
    kUnderscore,
    kBackquote,
    kA = 97, kB, kC, kD, kE, kF, kG, kH, kI, kJ, kK, kL, kM,
    kN, kO, kP, kQ, kR, kS, kT, kU, kV, kW, kX, kY, kZ,
    kLeftBrace = 123,
    kPipe,
    kRightBrace,
    kTilde,
    kDelete = 127,
    kLeftShift = 512,
    kRightShift,
    kCapsLock,
    kNumLock,
    kLeft,
    kRight,
    kUp,
    kDown,
    kHome,
    kEnd,
    kPageUp,
    kPageDown
};

struct KeyEvent
{
    enum class Type { kKeyDown, kKeyUp };

    Type type;
    Key key;
    int keymods;
    bool isRepeat;
};

}  // namespace uitk
#endif // UITK_EVENTS_H
