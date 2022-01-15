//-----------------------------------------------------------------------------
// Copyright 2021 - 2022 Eight Brains Studios, LLC
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

#ifndef UITK_WIN32_DIALOG_H
#define UITK_WIN32_DIALOG_H

#include "../Dialog.h"

namespace uitk {

class Win32Dialog
{
public:
    static void showAlert(Window *w,
                          const std::string& title,
                          const std::string& message,
                          const std::string& info,
                          const std::vector<std::string>& buttons,
                          std::function<void(Dialog::Result, int)> onDone);

    struct FileType
    {
        std::vector<std::string> exts;
        std::string description;
    };

    static void showSave(Window *w,
                         const std::string& title,
                         const std::string& dir,
                         const std::vector<FileType>& extensions,
                         std::function<void(Dialog::Result, const std::string&)> onDone);

    static void showOpen(Window *w,
                         const std::string& title,
                         const std::string& dir,
                         const std::vector<FileType>& extensions,
                         bool canSelectDirectories,
                         bool canSelectMultipleFiles,
                         std::function<void(Dialog::Result, const std::vector<std::string>&)> onDone);
};

}  // namespace uitk
#endif // UITK_WIN32_DIALOG_H
