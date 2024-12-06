//-----------------------------------------------------------------------------
// Copyright 2021 - 2025 Eight Brains Studios, LLC
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

namespace sound {

class PianoKey : public Widget
{
    using Super = Widget;
public:
    PianoKey(const Color& color, float freqHz)
    {
        mBG = color;
        mFreqHz = freqHz;
        setBackgroundColor(color);
    }

    float freqHz() const { return mFreqHz; }

    EventResult mouse(const MouseEvent& e) override
    {
        if (e.type == MouseEvent::Type::kButtonDown) {
            setBackgroundColor(Color::kGrey);
            play();
            return EventResult::kConsumed;
        } else if (e.type == MouseEvent::Type::kButtonUp) {
            setBackgroundColor(mBG);
            return EventResult::kConsumed;
        } else {
            return Super::mouse(e);
        }
    }

    void play() const
    {
        const float twoPi = 2.0f * 3.141592f;
        float rate = 44000.0f;
        float lengthSec = 1.0f;
        float masterVolume = 0.5f;
        std::vector<float> volumes = { 1.0f, 0.5f, 0.25f, 0.125f, 0.0625f, 0.03125f };

        lengthSec = std::ceil(lengthSec * mFreqHz) / mFreqHz;  // force an integer number of cycles
        std::vector<int16_t> samples((int)std::round(rate * lengthSec));
        for (size_t i = 0;  i < samples.size();  ++i) {
            const float sec = float(i) / rate;
            //float decay = (lengthSec - sec) / lengthSec;
            float decay = std::exp(-7.0f * sec / lengthSec);
            float v = 0.0f;
            for (size_t i = 0;  i < volumes.size();  ++i) {
                v += volumes[i] * decay * std::sin(float(i) * mFreqHz * twoPi * sec);
            }
            v *= masterVolume;
            samples[i] = int16_t(std::round(float(INT16_MAX) * v));
        }

        Application::instance().sound().play(samples.data(), samples.size(), int(rate), 1);
    }

private:
    Color mBG;
    float mFreqHz;
};

class Piano : public Widget
{
    using Super = Widget;
public:
    Piano()
    {
        setBorderColor(Color::kBlack);
        setBorderWidth(PicaPt::fromStandardPixels(1.0f));

        addChild(new PianoKey(Color::kWhite, 261.6256f));  // C4
        addChild(new PianoKey(Color::kWhite, 293.6648f));  // D4
        addChild(new PianoKey(Color::kWhite, 329.6276f));  // E4
        addChild(new PianoKey(Color::kWhite, 349.2282f));  // F4
        addChild(new PianoKey(Color::kWhite, 391.9954f));  // G4
        addChild(new PianoKey(Color::kWhite, 440.0f));     // A4
        addChild(new PianoKey(Color::kWhite, 493.8833f));  // B4
        addChild(new PianoKey(Color::kBlack, 277.1826f));  // C#4
        addChild(new PianoKey(Color::kBlack, 311.1270f));  // D#4
        addChild(new PianoKey(Color::kBlack, 369.9944f));  // F#4
        addChild(new PianoKey(Color::kBlack, 415.3047f));  // G#4
        addChild(new PianoKey(Color::kBlack, 466.1638f));  // A#4
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        const auto em = context.dc.roundToNearestPixel(context.theme.params().labelFont.pointSize());
        return Size(7 * 3 * em, 10 * em);
    }

    void layout(const LayoutContext& context) override
    {
        auto r = bounds();
        auto whiteWidth = r.width / 7.0f;
        auto blackWidth = whiteWidth * 0.666f;
        auto halfBlack = blackWidth / 2.0f;
        auto blackHeight = context.dc.roundToNearestPixel(0.666f * r.height);

        auto &childs = children();
        for (int i = 0;  i < 7;  ++i) {
            auto *key = childs[i];
            key->setBorderColor(Color::kBlack);
            key->setBorderWidth(PicaPt::fromStandardPixels(0.5f));
            key->setFrame(Rect(i * whiteWidth, PicaPt::kZero, whiteWidth, r.height));
        }

        childs[7]->setFrame(Rect(1.0f * whiteWidth - halfBlack, PicaPt::kZero, blackWidth, blackHeight));
        childs[8]->setFrame(Rect(2.0f * whiteWidth - halfBlack, PicaPt::kZero, blackWidth, blackHeight));
        childs[9]->setFrame(Rect(4.0f * whiteWidth - halfBlack, PicaPt::kZero, blackWidth, blackHeight));
        childs[10]->setFrame(Rect(5.0f * whiteWidth - halfBlack, PicaPt::kZero, blackWidth, blackHeight));
        childs[11]->setFrame(Rect(6.0f * whiteWidth - halfBlack, PicaPt::kZero, blackWidth, blackHeight));

        Super::layout(context);
    }

private:
};

class Panel : public VLayout
{
    using Super = VLayout;

public:
    Panel()
    {
        setMarginsEm(1.0f);
        setSpacingEm(1.0f);

        auto *beepButton = new Button("System beep");
        addChild(new HLayout({ beepButton, new Layout::Stretch(Dir::kHoriz) }));
        beepButton->setOnClicked([](Button *b) { Application::instance().beep(); });

        mPiano = new Piano();
        addChild(new HLayout({ mPiano, new Layout::Stretch(Dir::kHoriz) }));

        addStretch();
    }

    void layout(const LayoutContext& context) override
    {
        Super::layout(context);
    }

private:
    Piano *mPiano;
};

} // namespace sound
