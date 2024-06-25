//-----------------------------------------------------------------------------
// Copyright 2021 - 2024 Eight Brains Studios, LLC
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
#include <uitk/uitk.h>

using namespace uitk;

// Class to manage one channel (e.g. red).
// This is structured in a model/view pattern. While this pattern is not
// necessary for a class of its complexity, it is a good discipline
// because it eliminates bugs in more complex UIs, particularly when
// some values need to be disabled, and similar situations. Basically there
// the model value(s), which are raw values, and an update function, which
// sets the UI to the proper state. (This does have one requirement, which
// is that the "set value" function not call any callbacks, otherwise you
// get callbacks calling callbacks or you have to figure out which UI object
// actually got set and not set that one. Fortunately, UITK uses this
// paradigm.
class ColorChannel : public HLayout
{
public:
    ColorChannel(int channelNum)
    {
        mChannelNum = channelNum;

        mLabel = new Label(" ");
        mSlider = new Slider();
        mSlider->setLimits(0, 255, 1);
        mIntValue = new NumberEdit();
        mIntValue->setLimits(0, 255, 1);
        mFloatValue = new NumberEdit();
        mFloatValue->setLimits(0.0, 1.0, 0.001);

        auto update = [this](double val) {
            setValue(float(val));
            if (mOnChanged) {
                mOnChanged(this);
            }
        };

        mSlider->setOnValueChanged([update](SliderLogic *s) {
            update(s->doubleValue() / 255.0);
        });
        mIntValue->setOnValueChanged([update](NumberEdit *ne) {
            update(ne->doubleValue() / 255.0);
        });
        mFloatValue->setOnValueChanged([update](NumberEdit *ne) {
            update(ne->doubleValue());
        });

        // addChild() takes ownership of the pointers and will delete
        // all its children, so we do not need to (and should not)
        // clean up these pointers. They effectively function as references.
        addChild(mLabel);
        addChild(mSlider);
        addChild(mIntValue);
        addChild(mFloatValue);
    }

    int intValue() const { return mModel.iValue; }
    float value() const { return float(mModel.value); }
    // UITK returns the calling object on setters so that callers can
    // create, set, and add a child all in one line.
    ColorChannel* setValue(float val)
    {
        int iVal = int(std::round(val * 255.0f));
        mModel.value = val;
        mModel.iValue = iVal;

        mSlider->setValue(iVal);
        mIntValue->setValue(iVal);
        mFloatValue->setValue(val);
        return this;
    }

    int channelNum() const { return mChannelNum; }

    Label* label() const { return mLabel; }

    ColorChannel* setOnValueChanged(std::function<void(ColorChannel *c)> onChanged)
    {
        mOnChanged = onChanged;
        return this;
    }

private:
    int mChannelNum;

    struct {
        float value = 0.0f;
        int iValue = 0;
    } mModel;

    std::function<void(ColorChannel *c)> mOnChanged;
    Label *mLabel;
    Slider *mSlider;
    NumberEdit *mIntValue;
    NumberEdit *mFloatValue;
};

class ColorPicker : public VLayout
{
public:
    ColorPicker()
    {
        // The default layout margin is PicaPt::kZero so that nested layouts work
        // like you expect. Since this is the outer layout, though, we want some
        // margins between the edge of the window and the content.
        setMarginsEm(1.0);  // 1.0 em

        addChild((mRGB[0] = new ColorChannel(0)));
        addChild((mRGB[1] = new ColorChannel(1)));
        addChild((mRGB[2] = new ColorChannel(2)));
        auto *h = new HLayout();
        h->addStretch();
        h->addChild(mHex = new StringEdit());
        addChild(h);
        addChild(mSwatch = new Widget());

        mRGB[0]->label()->setText("R:");
        mRGB[1]->label()->setText("G:");
        mRGB[2]->label()->setText("B:");

        mHex->setFixedWidthEm(5.0);
        mHex->setAlignment(Alignment::kRight);
        
        for (auto *channel : mRGB) {
            channel->setOnValueChanged([this](ColorChannel *cc){
                // Set the model value
                mColor.setRed(mRGB[0]->value());
                mColor.setGreen(mRGB[1]->value());
                mColor.setBlue(mRGB[2]->value());
                // Update the view
                update();
            });
        }

        mHex->setOnValueChanged([this](StringEdit *se) {
            auto hex = se->text();
            if (hex.empty()) {
                return;
            }
            if (hex[0] != '#') {
                hex = "#" + hex;
            }
            mColor = Color::fromCSS(hex.c_str());  // set the model value
            update();                              // update the view
        });

        update();
    }

    void update()
    {
        mRGB[0]->setValue(mColor.red());
        mRGB[1]->setValue(mColor.green());
        mRGB[2]->setValue(mColor.blue());
        mSwatch->setBackgroundColor(mColor);
        mHex->setText(mColor.toHexString().substr(0, 6));  // toHexString() includes alpha component
    }

private:
    // Model
    Color mColor = Color::kYellow;

    // View/UI (these pointers are effectively references and are owned by the parent)
    ColorChannel* mRGB[3];
    StringEdit *mHex;
    Widget *mSwatch;
};

int main(int argc, char *argv[])
{
    // We need to create the application before anything else.
    // This initializes the UITK library with the operating system.
    Application app;

    // Create the window. With the static create() function, memory cleanup will
    // be handled for us: the object will be automatically deleted when the window
    // closes, so we can just use the pointer and forget about it when we are done.
    Window &win = Window::create("Color picker", PicaPt::fromStandardPixels(640),
                                 PicaPt::fromStandardPixels(480));
    // addChild() takes ownership of the pointer, so we can just new this and
    // pass it in. If a window has only one child, that child will fill the entire
    // window, so layout will happen automatically.
    win.addChild(new ColorPicker());
    // Windows are created hidden initially, so we need to specifically show it.
    win.show(true);

    // app.run() runs the event loop.
    app.run();
}
