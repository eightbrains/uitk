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

#ifndef UITK_CURSOR_H
#define UITK_CURSOR_H

namespace uitk {

class OSCursor;

/// This is lightweight representation of a cursor, and can be copied
/// quickly and with no additional memory allocations. To get a system
/// cursor you should use one of the static functions. Use Window::setCursor()
/// to set the cursor; usually you will do this in Widget::mouseEntered()
/// and Widget::mouseExited().
class Cursor {
public:
    static Cursor arrow();          /// default cursor
    static Cursor iBeam();          /// for text
    static Cursor crosshair();
    static Cursor openHand();       /// usually indicates object or canvas can be grabbed
    static Cursor closedHand();     /// object or canvas is grabbed
    static Cursor pointingHand();
    static Cursor resizeUpDown();
    static Cursor resizeLeftRight();
    static Cursor resizeNWSE();
    static Cursor resizeNESW();
    static Cursor forbidden();

    /// Prefer using the static functions to get a system cursor to using
    /// the default constructor. This is a noop cursor, and exists to allow
    /// declaring members or stack variables without needing to create a
    /// cursor at construction time, which may be unknown (in the case of a
    /// stack variable) or happen before the graphics system is initialized,
    /// or the language used cannot handle C++ constructors at allocation
    /// time (e.g an Objective-C++ class with a Cursor member). This cursor
    /// MUST be assigned a real cursor before use. In particular, this
    /// is NOT the default constructor:  Cursor() == Cursor::arrow() will
    /// return false. It is preferable to construct your code so that
    /// the default constructor is not needed.
    Cursor();
    Cursor(const Cursor& c);
    ~Cursor();

    Cursor& operator=(const Cursor& rhs);
    bool operator==(const Cursor& rhs) const;
    bool operator!=(const Cursor& rhs) const;

public:
    Cursor(OSCursor *osc);  /// this is internal

    /// This is internal, you probably Window::setCursor().
    /// Note that this may return nullptr;
    OSCursor* osCursor() const;

private:
    // Q: Why is this not using the PImpl pattern used everywhere else?
    // A: Every widget will have one of these, almost all of which will be
    //    the default cursor. The PImpl pattern would produce a number of
    //    allocations, and this reduces to none for the common case, where
    //    it is essentially an embedded struct in the Widget.
    OSCursor *mCursor;
};

}  // namespace uitk
#endif // UITK_CURSOR_H


