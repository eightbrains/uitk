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
    }

    void layout(const LayoutContext& context)
    {
        auto em = context.theme.params().labelFont.pointSize();
        auto pref = mOkAlert->preferredSize(context);
        mOkAlert->setFrame(Rect(em, em, pref.width, pref.height));
        pref = mAlert->preferredSize(context);
        mAlert->setFrame(Rect(mOkAlert->frame().maxX() + em, em, pref.width, pref.height));

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
        Dialog::showAlert(w, "Alert", "We need to ask for confirmation or a decision about something.",
                          "Please click a button", {"Ok", "Cancel", "I'm feeling lucky!"},
                          [w](Dialog::Result r, int idx) {
            if (r == Dialog::Result::kFinished) {
                Dialog::showAlert(w, "Result", std::string("Pressed button ") + std::to_string(idx + 1), "");
            } else {
                Dialog::showAlert(w, "Result", "Pressed cancel", "");
            }
        });
    }

private:
    Button *mOkAlert;
    Button *mAlert;
};

}  // dialogs
