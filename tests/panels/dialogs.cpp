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

// No includes, this is included directly as a .cpp file, which avoids the
// hassle of a bunch of mostly identical header files.

namespace dialogs {

class Panel : public Widget
{
    using Super = Widget;
public:
    Panel()
    {
        mOkAlert = new Button("Simple alert");
        mOkAlert->setOnClicked([this](Button *) { onOkAlert(); });
        addChild(mOkAlert);

        mAlert = new Button("Alert");
        mAlert->setOnClicked([this](Button *) { onAlert(); });
        addChild(mAlert);

        mOpen = new Button("Open file");
        mOpen->setOnClicked([this](Button *) { onFileDialog(FileDialog::kOpen, mOpenResult); });
        addChild(mOpen);

        mOpenResult = new Label("");
        addChild(mOpenResult);

        mSave = new Button("Save file");
        mSave->setOnClicked([this](Button *) { onFileDialog(FileDialog::kSave, mSaveResult); });
        addChild(mSave);

        mSaveResult = new Label("");
        addChild(mSaveResult);

        mMultiOpen = new Button("Multi-Open");
        mMultiOpen->setOnClicked([this](Button *) { onMultiOpen(); });
        addChild(mMultiOpen);

        mMultiOpenResults = new Label("");
        addChild(mMultiOpenResults);
    }

    void layout(const LayoutContext& context)
    {
        auto em = context.theme.params().labelFont.pointSize();
        auto pref = mOkAlert->preferredSize(context);
        mOkAlert->setFrame(Rect(em, em, pref.width, pref.height));
        pref = mAlert->preferredSize(context);
        mAlert->setFrame(Rect(mOkAlert->frame().maxX() + em, em, pref.width, pref.height));

        auto w = std::max(mOpen->preferredSize(context).width, mSave->preferredSize(context).width);
        auto labelPref = mOpenResult->preferredSize(context);
        mOpen->setFrame(Rect(em, mOkAlert->frame().maxY() + em, w, pref.height));
        mOpenResult->setFrame(Rect(mOpen->frame().maxX() + em, mOpen->frame().y, labelPref.width, labelPref.height));
        mSave->setFrame(Rect(em, mOpen->frame().maxY() + em, w, pref.height));
        labelPref = mSaveResult->preferredSize(context);
        mSaveResult->setFrame(Rect(mSave->frame().maxX() + em, mSave->frame().y, labelPref.width, labelPref.height));
        mMultiOpen->setFrame(Rect(em, mSave->frame().maxY() + em, w, pref.height));
        labelPref = mMultiOpenResults->preferredSize(context);
        mMultiOpenResults->setFrame(Rect(mMultiOpen->frame().maxX() + em, mMultiOpen->frame().y, labelPref.width, labelPref.height));

        Super::layout(context);
    }

private:
    void onOkAlert()
    {
        Dialog::showAlert(window(), "Ok alert", "This is the main message, probably an error. Please resolve any problems before trying again.", "This is more information or suggestions for resolving the problem");
    }

    void onAlert()
    {
        auto *w = window();
        Dialog::showAlert(w, "Alert", "This tests for multiple buttons and whether keys work properly.",
                          "Pressing the Ok button or the Return key should say '1'\n"
                          "Pressing the Cancel button, the Escape key, or clicking the close button should say 'cancelled'\n"
                          "The other button should say '3'",
                          {"Ok", "Cancel", "I'm feeling lucky!"},
                          [w](Dialog::Result r, int idx) {
            if (r == Dialog::Result::kFinished) {
                Dialog::showAlert(w, "Result", std::string("Pressed button ") + std::to_string(idx + 1), "");
            } else {
                Dialog::showAlert(w, "Result", "Pressed cancel", "");
            }
        });
    }

    void onFileDialog(FileDialog::Type type, Label *resultLabel)
    {
        auto *w = window();
        auto *dlg = new FileDialog(type);
        if (type == FileDialog::kOpen) {
            // std::string has a constructor that takes a pair of iterators, so we need to
            // specify that we want to make a vector.
            dlg->addAllowedType(std::vector<std::string>({"jpg", "jpeg", "png", "gif"}), "Images");
            dlg->addAllowedType("gif", "GIF Image");
            dlg->addAllowedType(std::vector<std::string>({"jpg", "jpeg"}), "JPEG Image");
            dlg->addAllowedType("png", "PNG Image");
            dlg->addAllowedType("", "All files");
        }
        dlg->showModal(w, [dlg, resultLabel](Dialog::Result, int) {
            resultLabel->setText(dlg->selectedPath());
            Application::instance().scheduleLater(nullptr, [dlg]() { delete dlg; });
        });
    }

    void onMultiOpen()
    {
        auto *w = window();
        auto *dlg = new FileDialog(FileDialog::kOpen);
        dlg->setCanSelectMultipleFiles(true);
        dlg->showModal(w, [dlg, this](Dialog::Result, int) {
            mMultiOpenResults->setText(std::to_string(dlg->selectedPaths().size()));
            Application::instance().scheduleLater(nullptr, [dlg]() { delete dlg; });
        });
    }

private:
    Button *mOkAlert;
    Button *mAlert;
    Button *mOpen;
    Label *mOpenResult;
    Button *mSave;
    Label *mSaveResult;
    Button *mMultiOpen;
    Label *mMultiOpenResults;
};

}  // dialogs
