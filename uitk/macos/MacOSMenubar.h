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

#ifndef UITK_MACOS_MENUBAR_H
#define UITK_MACOS_MENUBAR_H

#include "../OSMenubar.h"

#include <memory>
#include <string>

namespace uitk {

class MacOSMenubar : public OSMenubar
{
    friend class Application;
    friend class Window;
public:
    MacOSMenubar(const MacOSMenubar&) = delete;
    ~MacOSMenubar();

    Menu* newMenu(const std::string& name) override;
    void addMenu(Menu* menu, const std::string& name) override;
    Menu* removeMenu(const std::string& name) override;

    Menu* menu(const std::string& name) const override;
    Menu* macosApplicationMenu() const override;

    std::vector<Menu*> menus() const override;

    void activateItemId(MenuId itemId) const override;

    void onApplicationFinishedLaunching();

private:
    MacOSMenubar();

    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk

#endif // UITK_MACOS_MENUBAR_H
