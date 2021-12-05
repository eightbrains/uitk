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

#ifndef UITK_SHORTCUT_KEY_H
#define UITK_SHORTCUT_KEY_H

#include "Events.h"

#include <memory>
#include <string>

namespace uitk {

struct ShortcutKey
{
    KeyModifier::Values modifier;
    Key key;

    static const ShortcutKey kNone;
    
    ShortcutKey() : modifier(KeyModifier::kNone), key(Key::kNone) {}
    ShortcutKey(KeyModifier::Values m, Key k) : modifier(m), key(k) {}

    std::size_t hash() const;
    bool operator==(const ShortcutKey& rhs) const
        { return (this->modifier == rhs.modifier && this->key == rhs.key); }

    std::string displayText() const;
};

class Shortcuts
{
public:
    Shortcuts();
    ~Shortcuts();

    void add(int menuId, const ShortcutKey& shortcut);
    void remove(int menuId);

    bool hasShortcut(const KeyEvent& e, int *menuId);

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk

#endif // UITK_SHORTCUT_KEY_H
