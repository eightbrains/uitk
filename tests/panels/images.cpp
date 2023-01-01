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

// No includes, this is included directly as a .cpp file, which avoids the
// hassle of a bunch of mostly identical header files.

namespace images {

class Panel : public Widget
{
    using Super = Widget;
public:
    Panel()
    {
        struct Format {
            ImageFormat format;
            std::string text;
        };
        std::vector<Format> formats = {
            { kImageGreyscale8,           "Grey" },
            { kImageGreyscaleAlpha16,     "Grey+alpha" },
            { kImageRGB24,                "RGB" },
            { kImageBGR24,                "BGR" },
            { kImageRGBX32,               "RGBX" },
            { kImageBGRX32,               "BGRX" },
            { kImageRGBA32,               "RGBA" },
            { kImageRGBA32_Premultiplied, "RGBA pre" },
            { kImageBGRA32,               "BGRA" },
            { kImageBGRA32_Premultiplied, "BGRA pre" },
            { kImageARGB32,               "ARGB" },
            { kImageARGB32_Premultiplied, "ARGB pre" },
            { kImageABGR32,               "ABGR" },
            { kImageABGR32_Premultiplied, "ABGR pre" },
        };

        mBasicTests.reserve(formats.size());
        for (auto &fmt : formats) {
            mBasicTests.push_back(BasicTest{ fmt.format,
                                             new ImageView(),
                                             (new Label(fmt.text))
                                                 ->setWordWrapEnabled(true)
                                                 ->setAlignment(Alignment::kHCenter)

                                           });
            addChild(mBasicTests.back().view);
            addChild(mBasicTests.back().label);
        }

        mWideWide = new ImageView();
        mWideWide->setBorderColor(Color(0.5f, 0.5f, 0.5f));
        mWideWide->setBorderWidth(PicaPt::fromStandardPixels(1));
        addChild(mWideWide);

        mWideHigh = new ImageView();
        mWideHigh->setBorderColor(Color(0.5f, 0.5f, 0.5f));
        mWideHigh->setBorderWidth(PicaPt::fromStandardPixels(1));
        addChild(mWideHigh);

        mHighWide = new ImageView();
        mHighWide->setBorderColor(Color(0.5f, 0.5f, 0.5f));
        mHighWide->setBorderWidth(PicaPt::fromStandardPixels(1));
        addChild(mHighWide);

        mHighHigh = new ImageView();
        mHighHigh->setBorderColor(Color(0.5f, 0.5f, 0.5f));
        mHighHigh->setBorderWidth(PicaPt::fromStandardPixels(1));
        addChild(mHighHigh);

        mStretch = new ImageView();
        mStretch->setMode(ImageView::Mode::kStretch);
        mStretch->setBorderColor(Color(0.5f, 0.5f, 0.5f));
        mStretch->setBorderWidth(PicaPt::fromStandardPixels(1));
        addChild(mStretch);

        mFixed = new ImageView();
        mFixed->setMode(ImageView::Mode::kFixed);
        mFixed->setBorderColor(Color(0.5f, 0.5f, 0.5f));
        mFixed->setBorderWidth(PicaPt::fromStandardPixels(1));
        addChild(mFixed);

        mSmall = new ImageView();
        mSmall->setBorderColor(Color(0.5f, 0.5f, 0.5f));
        mSmall->setBorderWidth(PicaPt::fromStandardPixels(1));
        addChild(mSmall);

        mUser = new ImageView();
        mUser->setBorderColor(Color(0.5f, 0.5f, 0.5f));
        mUser->setBorderWidth(PicaPt::fromStandardPixels(1));
        addChild(mUser);

        mChoose = new Button("Select Image...");
        addChild(mChoose);

        mNewFractal = new Button("New Fractal");
        addChild(mNewFractal);

        mNewFractal->setOnClicked([this](Button*) { mUser->setImage(Image()); });
        
        mChoose->setOnClicked([this](Button *b) {
            auto *w = b->window();
            auto *dlg = new FileDialog(FileDialog::Type::kOpen);
            // std::string has a constructor that takes a pair of iterators, so we need to
            // specify that we want to make a vector.
            dlg->addAllowedType(std::vector<std::string>({"jpg", "jpeg", "png", "gif"}), "Images");
            dlg->addAllowedType("gif", "GIF Image");
            dlg->addAllowedType(std::vector<std::string>({"jpg", "jpeg"}), "JPEG Image");
            dlg->addAllowedType("png", "PNG Image");
            dlg->addAllowedType("", "All files");
            dlg->showModal(w, [dlg, w, this](Dialog::Result result, int) {
                if (result == Dialog::Result::kFinished) {
                    auto path = dlg->selectedPath();
                    IOError::Error err;
                    auto data = File(path).readContents(&err);
                    if (err == IOError::kNone) {
                        mUser->setImage(Image::fromEncodedData((uint8_t*)data.data(), data.size()));
                    } else {
                        Dialog::showAlert(w, "Could not read file", "Could not read file: error " + std::to_string(int(err)), "");
                    }
                    this->setNeedsDraw();
                }
                delete dlg;
            });
        });
    }

    void layout(const LayoutContext &context) override
    {
        auto& dc = context.dc;
        auto dpi = dc.dpi();
        auto em = dc.roundToNearestPixel(context.theme.params().labelFont.pointSize());
        auto margin = em;
        auto spacing = 2.0f * em;
        auto smallSpacing = em;

        auto aspectHeight = context.dc.roundToNearestPixel(10.0f * em);
        auto aspectWide = context.dc.roundToNearestPixel(1.333f * aspectHeight);
        auto aspectHigh = context.dc.roundToNearestPixel(0.75f * aspectHeight);
        int fractalHeightPx = int(1.5f * aspectHeight.toPixels(context.dc.dpi()));  // we want extra pixels

        // Update the images if necessary
        for (auto &test : mBasicTests) {
            if (!test.view->image().isValid() || test.view->image().dpi() != dpi) {
                test.view->setImage(createTestImage(dc, test.format, dpi));
            }
        }
        if (!mWideWide->image().isValid() || mWideWide->image().dpi() != dpi) {
            mWideWide->setImage(calcFractalImage(dc, 0x7a32d601, int(2.0f * fractalHeightPx), fractalHeightPx, dpi));
            mWideHigh->setImage(calcFractalImage(dc, 0x2f33c09d, int(1.0f * fractalHeightPx), fractalHeightPx, dpi));
            mHighWide->setImage(calcFractalImage(dc, 0x067b8821, int(2.0f * fractalHeightPx), fractalHeightPx, dpi));
            mHighHigh->setImage(calcFractalImage(dc, 0xbc690252, int(1.0f * fractalHeightPx), fractalHeightPx, dpi));
            mStretch->setImage(mHighWide->image());
            mFixed->setImage(calcFractalImage(dc, 0x33a8c416, int(1.0f * fractalHeightPx), fractalHeightPx, dpi));
            mSmall->setImage(calcFractalImage(dc, 0x5f02b002, int(0.5f * aspectHeight.toPixels(dpi)),
                                              int(0.5f * aspectHeight.toPixels(dpi)), dpi));
        }

        // Layout
        auto x = margin;
        auto y = margin;
        for (auto &test : mBasicTests) {
            auto pref = test.view->preferredSize(context);
            test.view->setFrame(Rect(x, y, pref.width, pref.height));
            test.label->setFrame(Rect(x - 0.5f * em, test.view->frame().maxY(), pref.width + em, 3.0f * em));
            x = test.view->frame().maxX() + spacing;
        }
        y = mBasicTests[0].label->frame().maxY() + spacing;

        x = margin;
        mWideWide->setFrame(Rect(x, y, aspectWide, aspectHeight));
        mWideHigh->setFrame(Rect(mWideWide->frame().maxX() + smallSpacing, y, aspectWide, aspectHeight));
        mHighWide->setFrame(Rect(mWideHigh->frame().maxX() + smallSpacing, y, aspectHigh, aspectHeight));
        mHighHigh->setFrame(Rect(mHighWide->frame().maxX() + smallSpacing, y, aspectHigh, aspectHeight));
        mStretch->setFrame(Rect(mHighHigh->frame().maxX() + 2.0f * spacing, y, aspectHigh, aspectHeight));
        mFixed->setFrame(Rect(mStretch->frame().maxX() + 2.0f * spacing, y, aspectHigh, aspectHeight));
        mSmall->setFrame(Rect(mFixed->frame().maxX() + 2.0f * spacing, y, aspectHigh, aspectHeight));
        y = mHighHigh->frame().maxY() + spacing;

        auto choosePref = mChoose->preferredSize(context);
        auto newPref = mNewFractal->preferredSize(context);
        mUser->setFrame(Rect(margin, y, 20 * em, 20 * em));
        mChoose->setFrame(Rect(mUser->frame().maxX() + spacing, y, std::max(choosePref.width, newPref.width), choosePref.height));
        mNewFractal->setFrame(mChoose->frame().translated(PicaPt::kZero, mChoose->frame().height + em));

        Super::layout(context);
    }

    void draw(UIContext& context) override
    {
        if (!mUser->image().isValid()) {
            uint32_t seed = std::random_device()();
            auto dpi = context.dc.dpi();
            mUser->setImage(calcFractalImage(context.dc, seed, int(mUser->frame().width.toPixels(dpi)),
                                             int(mUser->frame().height.toPixels(dpi)), dpi));
        }

        Super::draw(context);
    }

private:
    Image createTestImage(const DrawContext& dc, ImageFormat format, float dpi)
    {
        int nChannels = 0;
        std::array<int, 4> rgbaMap = { 0, 0, 0, 0 };
        bool isColor = (format != kImageGreyscaleAlpha16 && format != kImageGreyscale8);
        bool hasAlpha = false;
        bool premult = false;
        int allowedError = 0;
        switch (format) {
            case kImageRGBA32:
                nChannels = 4;  rgbaMap = { 0, 1, 2, 3 };  hasAlpha = true;  break;
            case kImageRGBA32_Premultiplied:
                nChannels = 4;  rgbaMap = { 0, 1, 2, 3 };  hasAlpha = true;  premult = true;  break;
            case kImageBGRA32:
                nChannels = 4;  rgbaMap = { 2, 1, 0, 3 };  hasAlpha = true;  break;
            case kImageBGRA32_Premultiplied:
                nChannels = 4;  rgbaMap = { 2, 1, 0, 3 };  hasAlpha = true;  premult = true;  break;
            case kImageARGB32:
                nChannels = 4;  rgbaMap = { 1, 2, 3, 0 };  hasAlpha = true;  break;
            case kImageARGB32_Premultiplied:
                nChannels = 4;  rgbaMap = { 1, 2, 3, 0 };  hasAlpha = true;  premult = true;  break;
            case kImageABGR32:
                nChannels = 4;  rgbaMap = { 3, 2, 1, 0 };  hasAlpha = true;  break;
            case kImageABGR32_Premultiplied:
                nChannels = 4;  rgbaMap = { 3, 2, 1, 0 };  hasAlpha = true;  premult = true;  break;
            case kImageRGBX32:
                nChannels = 4;  rgbaMap = { 0, 1, 2, 3 };  hasAlpha = false;  break;
            case kImageBGRX32:
                nChannels = 4;  rgbaMap = { 2, 1, 0, 3 };  hasAlpha = false;  break;
            case kImageRGB24:
                nChannels = 3;  rgbaMap = { 0, 1, 2, 3 };  hasAlpha = false;  break;
            case kImageBGR24:
                nChannels = 3;  rgbaMap = { 2, 1, 0, 3 };  hasAlpha = false;  break;
            case kImageGreyscaleAlpha16:
                nChannels = 2;  rgbaMap = { 0, 0, 0, 1 };  hasAlpha = true;  break;
            case kImageGreyscale8:
                nChannels = 1;  rgbaMap = { 0, 0, 0, 0 };  hasAlpha = false;  break;
            case kImageEncodedData_internal:
                assert(false);
                nChannels = 0;  rgbaMap = { 0, 0, 0, 0 };  break;
        }

        const int blockWidth = 20, blockHeight = 30;
        const int width = 2 * blockWidth, height = 2 * blockHeight;
        const int imgOffset = 1;
        const int RED = 0, GREEN = 1, BLUE = 2, ALPHA = 3;
        std::vector<uint8_t> imgData(nChannels * width * height, 0);

        auto setChannel = [&imgData, &rgbaMap, nChannels, width, hasAlpha, ALPHA, this](int x, int y, int channel, uint8_t val) {
            if (channel == ALPHA && !hasAlpha) {
                return;
            }
            imgData[y * nChannels * width + (x * nChannels) + rgbaMap[channel]] = val;
        };

        uint8_t red = (isColor ? 0xff : 0x80);
        for (int y = 0;  y < blockHeight;  ++y) {
            for (int x = 0;  x < blockWidth;  ++x) {
                setChannel(x, y, ALPHA, 0xff);
                setChannel(x, y, RED, red);
            }
        }

        uint8_t green = (isColor ? 0xff : 0xc0);
        for (int y = 0;  y < blockHeight;  ++y) {
            for (int x = blockWidth;  x < 2 * blockWidth;  ++x) {
                setChannel(x, y, ALPHA, 0xff);
                setChannel(x, y, GREEN, green);
            }
        }

        uint8_t blue = (isColor ? 0xff : 0x40);
        for (int y = blockHeight;  y < 2 * blockHeight;  ++y) {
            for (int x = 0;  x < blockWidth;  ++x) {
                setChannel(x, y, ALPHA, 0xff);
                setChannel(x, y, BLUE, blue);
            }
        }

        uint8_t white = (isColor ? 0xff : 0xff);
        for (int y = blockHeight;  y < 2 * blockHeight;  ++y) {
            for (int x = blockWidth;  x < 2 * blockWidth;  ++x) {
                setChannel(x, y, ALPHA, 0xff);
                setChannel(x, y, RED, white);
                setChannel(x, y, GREEN, white);
                setChannel(x, y, BLUE, white);
            }
        }

        if (hasAlpha) {  // overwrite lower corner of the white with alpha
            uint8_t alpha = 0x22;
            for (int y = blockHeight + blockHeight / 2;  y < 2 * blockHeight;  ++y) {
                for (int x = blockWidth + blockWidth / 2;  x < 2 * blockWidth;  ++x) {
                    if (premult) {  // premult is (white * alpha); otherwise white is already set from above
                        setChannel(x, y, RED, alpha);
                        setChannel(x, y, GREEN, alpha);
                        setChannel(x, y, BLUE, alpha);
                    }
                    setChannel(x, y, ALPHA, alpha);
                }
            }
        }

        return Image::fromCopyOfBytes(imgData.data(), width, height, format, dpi);
    }

private:
    struct BasicTest {
        ImageFormat format;
        ImageView *view;  // this is a ref; parent owns the actual widget
        Label *label;  // this is a ref
    };
    std::vector<BasicTest> mBasicTests;
    ImageView *mWideWide;
    ImageView *mWideHigh;
    ImageView *mHighWide;
    ImageView *mHighHigh;
    ImageView *mStretch;
    ImageView *mFixed;
    ImageView *mSmall;
    ImageView *mUser;
    Button *mChoose;
    Button *mNewFractal;
};

} // namespace images
