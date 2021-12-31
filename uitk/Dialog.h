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

#ifndef UITK_DIALOG_H
#define UITK_DIALOG_H

#include "Widget.h"

namespace uitk {

// Design note
// Q: Why aren't finish()/cancel() protected? It does not make sense for
//    outsiders to exit the dialog.
// A: This allows the dialog to be constructed without needing to create
//    a new class and inherit from Dialog. This way you can simply connect
//    the callback function of the button to the appropriate exit and be
//    done.
class Dialog : public Widget
{
    using Super = Widget;
public:
    enum class Result { kCancelled = 0, kFinished = 1 };

    static void showAlert(Window *w,
                          const std::string& title,
                          const std::string& message,
                          const std::string& info);
    static void showAlert(Window *w,
                          const std::string& title,
                          const std::string& message,
                          const std::string& info,
                          const std::vector<std::string>& buttons,
                          std::function<void(Dialog::Result, int)> onDone);
public:
    Dialog();
    virtual ~Dialog();

    const std::string& title() const;
    Dialog* setTitle(const std::string& title);

    /// Dialogs should call this to indicate that they are finished
    void finish(int value);
    /// Dialogs should call this to indicate that they are cancelled
    void cancel();

    /// Shows the dialog asynchronously. When the dialog is finished, the callback
    /// is called with onDone(result, value), where value is what was passed
    /// to finish(), or if the dialog was cancelled, indeterminate.
    virtual void showModal(Window *w, std::function<void(Result, int)> onDone);

    void key(const KeyEvent& e) override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_DIALOG_H
