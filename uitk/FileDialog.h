//-----------------------------------------------------------------------------
// Copyright 2021-2022 Eight Brains Studios, LLC
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

#ifndef UITK_FILE_DIALOG_H
#define UITK_FILE_DIALOG_H

#include "Dialog.h"

namespace uitk {

class FileDialog : public Dialog
{
    using Super = Dialog;
public:
    enum Type { kOpen, kSave };
    FileDialog(Type type);
    virtual ~FileDialog();

    /// This returns the path the user selected, or "" if the dialog was
    /// canceled. This should be called after the dialog has finished, for example,
    /// in the onDone callback to showModal().
    std::string selectedPath() const;

    /// Returns the paths the user selected, or an empty vectory if the dialog
    /// was canceled. There will only be multiple paths if this is a kOpen dialog
    /// and setCanSelectMultipleFiles(true) was called. This should be called after
    /// the dialog has finished, for example, in the onDone callback to showModal().
    std::vector<std::string> selectedPaths() const;

    const std::string& directory() const;
    void setDirectory(const std::string& dir);

    void clearAllowedTypes();

    /// Adds an allowed type. Use extension "" to allow all types.
    void addAllowedType(const std::string& extension, const std::string& description);
    void addAllowedType(const std::vector<std::string>& extensions, const std::string& description);

    bool canSelectDirectory() const;
    void setCanSelectDirectory(bool can);

    /// Returns true if user can select multiple files. Default is false.
    bool canSelectMultipleFiles() const;
    /// Allows the user to select multiple files. Only valid for a dialog of type
    /// kOpen; has no effect for kSave. Default is false.
    void setCanSelectMultipleFiles(bool can);

    void showModal(Window *w, std::function<void(Result, int)> onDone) override;

    Size preferredSize(const LayoutContext& context) const override;
    void layout(const LayoutContext& context) override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_FILE_DIALOG_H
