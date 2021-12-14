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

#ifndef UITK_MENU_ITERATOR_H
#define UITK_MENU_ITERATOR_H

#include <memory>
#include <vector>

namespace uitk {

class Menu;
class MenuItem;
class OSMenu;

// Cannot use a C++, STL-style iterator, because we do not want to expose
// begin() and end() functions on Menu.
class MenuIterator
{
public:
    explicit MenuIterator(Menu *menu);
    explicit MenuIterator(OSMenu *osmenu);
    ~MenuIterator();

    void next();
    bool done();
    // returned object is valid until the next call to an iterator function
    MenuItem& menuItem();

protected:
    struct Iterator
    {
        OSMenu *menu = nullptr;
        int index = 0;
        int maxIndex = 0;
    };

    std::vector<Iterator> mStack;

    void push(Menu *menu);

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_MENU_ITERATOR_H