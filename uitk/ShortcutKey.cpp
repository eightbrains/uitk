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

#include "ShortcutKey.h"

#include <unordered_map>

namespace uitk {

namespace {
struct ShortcutHash {
    std::size_t operator()(const ShortcutKey& s) const { return s.hash(); }
};
} // namespace

const ShortcutKey ShortcutKey::kNone;

std::size_t ShortcutKey::hash() const
{
    // From boost::hash_combine
    std::hash<int> hasher;
    std::size_t seed = hasher(int(modifier));
    seed ^= hasher(int(key)) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    return seed;
}

std::string ShortcutKey::displayText() const
{
    std::string text;
    if (int(modifier) & int(KeyModifier::kShift)) {
        text += "Shift+";
    }

    if (int(modifier) & int(KeyModifier::kCtrl)) {
#if __APPLE__
        text += "Cmd+";
#else
        text += "Ctrl+";
#endif // __APPLE__
    }

    if (int(modifier) & int(KeyModifier::kAlt)) {
#if __APPLE__
        text += "Opt+";
#else
        text += "Alt+";
#endif // __APPLE__
    }

    if (int(modifier) & int(KeyModifier::kMeta)) {
#if __APPLE__
        text += "Ctrl+";
#elif defined(_WIN32) || defined(_WIN64)
        text += "Win+";
#else
        text += "Meta+";
#endif // __APPLE__
    }

    switch (key) {
        default:                    text += "Unknown"; break;
        case Key::kNone:            text += ""; break;
        case Key::kBackspace:       text += "Back"; break;
        case Key::kTab:             text += "Tab"; break;
        case Key::kEnter:           text += "Enter"; break;
        case Key::kReturn:          text += "Return"; break;
        case Key::kSpace:           text += "Space"; break;
        case Key::kNumMultiply:     text += "NumMultiply"; break;
        case Key::kNumPlus:         text += "NumPlus"; break;
        case Key::kNumComma:        text += "NumComma"; break;
        case Key::kNumMinus:        text += "NumMinus"; break;
        case Key::kNumSlash:        text += "NumSlash"; break;
        case Key::kNumPeriod:       text += "NumPeriod"; break;
        case Key::k0:               text += "0"; break;
        case Key::k1:               text += "1"; break;
        case Key::k2:               text += "2"; break;
        case Key::k3:               text += "3"; break;
        case Key::k4:               text += "4"; break;
        case Key::k5:               text += "5"; break;
        case Key::k6:               text += "6"; break;
        case Key::k7:               text += "7"; break;
        case Key::k8:               text += "8"; break;
        case Key::k9:               text += "9"; break;
        case Key::kA:               text += "A"; break;
        case Key::kB:               text += "B"; break;
        case Key::kC:               text += "C"; break;
        case Key::kD:               text += "D"; break;
        case Key::kE:               text += "E"; break;
        case Key::kF:               text += "F"; break;
        case Key::kG:               text += "G"; break;
        case Key::kH:               text += "H"; break;
        case Key::kI:               text += "I"; break;
        case Key::kJ:               text += "J"; break;
        case Key::kK:               text += "K"; break;
        case Key::kL:               text += "L"; break;
        case Key::kM:               text += "M"; break;
        case Key::kN:               text += "N"; break;
        case Key::kO:               text += "O"; break;
        case Key::kP:               text += "P"; break;
        case Key::kQ:               text += "Q"; break;
        case Key::kR:               text += "R"; break;
        case Key::kS:               text += "S"; break;
        case Key::kT:               text += "T"; break;
        case Key::kU:               text += "U"; break;
        case Key::kV:               text += "V"; break;
        case Key::kW:               text += "W"; break;
        case Key::kX:               text += "X"; break;
        case Key::kY:               text += "Y"; break;
        case Key::kZ:               text += "Z"; break;
        case Key::kDelete:          text += "Del"; break;
        case Key::kInsert:          text += "Ins"; break;
        case Key::kLeft:            text += "Left"; break;
        case Key::kRight:           text += "Right"; break;
        case Key::kUp:              text += "Up"; break;
        case Key::kDown:            text += "Down"; break;
        case Key::kHome:            text += "Home"; break;
        case Key::kEnd:             text += "End"; break;
        case Key::kPageUp:          text += "PgUp"; break;
        case Key::kPageDown:        text += "PgDown"; break;
        case Key::kF1:              text += "F1"; break;
        case Key::kF2:              text += "F2"; break;
        case Key::kF3:              text += "F3"; break;
        case Key::kF4:              text += "F4"; break;
        case Key::kF5:              text += "F5"; break;
        case Key::kF6:              text += "F6"; break;
        case Key::kF7:              text += "F7"; break;
        case Key::kF8:              text += "F8"; break;
        case Key::kF9:              text += "F9"; break;
        case Key::kF10:             text += "F10"; break;
        case Key::kF11:             text += "F11"; break;
        case Key::kF12:             text += "F12"; break;
        case Key::kPrintScreen:     text += "PrintScn"; break;
    }

    return text;
}

//-----------------------------------------------------------------------------
struct Shortcuts::Impl
{
    std::unordered_map<ShortcutKey, int, ShortcutHash> key2id;
};

Shortcuts::Shortcuts()
: mImpl(new Shortcuts::Impl())
{
}

Shortcuts::~Shortcuts()
{
}

void Shortcuts::add(int menuId, const ShortcutKey& shortcut)
{
    mImpl->key2id[shortcut] = menuId;
}

void Shortcuts::remove(int menuId)
{
    for (auto it = mImpl->key2id.begin();  it != mImpl->key2id.end();  ++it) {
        if (it->second == menuId) {
            mImpl->key2id.erase(it);
            break;
        }
    }
}

bool Shortcuts::hasShortcut(const KeyEvent& e, int *menuId)
{
    auto it = mImpl->key2id.find(ShortcutKey(KeyModifier::Values(e.keymods), e.key));
    if (it != mImpl->key2id.end()) {
        if (menuId) {
            *menuId = it->second;
        }
        return true;
    }
    return false;
}

}  // namespace uitk
