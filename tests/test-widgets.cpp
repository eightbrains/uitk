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
#include <uitk/Events.h>

#include <array>
#include <random>
#include <sstream>

#include "fractal.h"

using namespace uitk;

// Include the .cpp files for the panels here. Since all the panels just
// inherit from Widget, there is no point in creating a bunch of virtually
// identical .h files.
#include "panels/widgets.cpp"
#include "panels/text.cpp"
#include "panels/icons.cpp"
#include "panels/cursors.cpp"
#include "panels/dialogs.cpp"
#include "panels/images.cpp"
#include "panels/gradients.cpp"
#include "panels/layouts.cpp"
#include "panels/sound.cpp"

class RootWidget : public Widget
{
    using Super = Widget;
public:
    RootWidget()
    {
        mSplitter = new Splitter(Dir::kHoriz);
        mSplitter->setPanelLengthsEm({ 10.0f });
        addChild(mSplitter);

        mPanelChooser = new ListView();
        mPanelChooser->setBorderWidth(PicaPt::kZero);
        mPanelChooser->setKeyNavigationWraps(true);
        mPanelChooser->setOnSelectionChanged([this](ListView *lv) {
            mPanels->setIndexShowing(lv->selectedIndex());
        });
        mSplitter->addPanel(mPanelChooser);

        mPanels = new StackedWidget();
        mSplitter->addPanel(mPanels);
    }

    void addPanel(const std::string& title, Widget *panel)
    {
        mPanelChooser->addStringCell(title);
        mPanels->addPanel(panel);

        if (mPanelChooser->size() == 1) {
            mPanelChooser->setSelectedIndex(0);
            mPanels->setIndexShowing(0);
        }
    }

    void layout(const LayoutContext& context) override
    {
        mSplitter->setFrame(bounds());
        Super::layout(context);
    }

private:
    Splitter *mSplitter;
    ListView *mPanelChooser;
    StackedWidget *mPanels;
};

static const MenuId kMenuIdNew = 1;
static const MenuId kMenuIdQuit = 2;
static const MenuId kMenuIdPrint = 3;
static const MenuId kMenuIdDisabled = 10;
static const MenuId kMenuIdCheckable = 11;
static const MenuId kMenuIdAddItem = 12;
static const MenuId kMenuIdDirectCallback = 13;
static const MenuId kMenuIdAlpha = 30;
static const MenuId kMenuIdBeta = 31;
static const MenuId kMenuIdToggleAlpha = 32;

class Document : public Window
{
public:
    static Document* createNewDocument()
    {
        auto *doc = new Document();
        doc->setOnWindowWillClose([](Window& w) { w.deleteLater(); });
        return doc;
    }

    std::string calcTitle() const
    {
        static int gWindowNum = 0;
        ++gWindowNum;
        std::string suffix;
        if (gWindowNum > 1) {
            suffix = std::string(" (") + std::to_string(gWindowNum) + ")";
        }
        return std::string("UITK test widgets") + suffix;
    }

    Document()
        : Window(calcTitle(), PicaPt::fromStandardPixels(1024), PicaPt::fromStandardPixels(768))
    {
        setOnMenuItemNeedsUpdate([this](MenuItem& item) {
            switch (item.id()) {
                case kMenuIdDisabled:
                    item.setEnabled(false);
                    break;
                case kMenuIdCheckable:
                    item.setChecked(mModel.testChecked);
                    break;
                case kMenuIdAlpha:
                    item.setChecked(mModel.alphaChecked);
                    break;
                case kMenuIdAddItem:
                    if (mModel.itemAdded) {
                        item.setText("Remove &Item from Menu");
                    } else {
                        item.setText("Add &Item to Menu");
                    }
                    break;
                default: break;
            }
        });

        setOnMenuActivated(kMenuIdNew, [](){ Document::createNewDocument(); });
        setOnMenuActivated(kMenuIdQuit, [](){ Application::instance().quit(); });
        setOnMenuActivated(kMenuIdPrint, [this]() {
            PrintSettings settings;
            settings.calcPages = [](const PaperSize&, const LayoutContext&) { return 2; };  // two, in case any problems with second page
            settings.drawPage = [this](const PrintContext& c) { this->print(c); };
            Application::instance().printDocument(settings);
        });
        setOnMenuActivated(kMenuIdDisabled,
                           [](){ Application::instance().quit(); /* shouldn't get here b/c disabled */ });
        setOnMenuActivated(kMenuIdCheckable,
                           [this](){ mModel.testChecked = !mModel.testChecked; });
        setOnMenuActivated(kMenuIdToggleAlpha,
                           [this](){ mModel.alphaChecked = !mModel.alphaChecked; });
        setOnMenuActivated(kMenuIdAddItem,
                           [this]() {
                if (!mModel.itemAdded) {
                    auto* m = Application::instance().menubar().menu("Test");
                    m->insertItem(0, "[Added item]", OSMenu::kInvalidId, ShortcutKey::kNone);
                    mModel.itemAdded = true;
                } else {
                    auto* m = Application::instance().menubar().menu("Test");
                    m->removeItem(0);
                    mModel.itemAdded = false;
                }
            });

        auto *root = new RootWidget();
        root->addPanel("Widgets", new widgets::AllWidgetsPanel());
        root->addPanel("Text", new text::Panel());
        root->addPanel("Icons", new icons::Panel());
        root->addPanel("Cursors", new cursor::Panel());
        root->addPanel("Layouts", new layouts::Panel());
        root->addPanel("Dialogs", new dialogs::Panel());
        root->addPanel("Images", new images::Panel());
        root->addPanel("Gradients", new gradients::Panel());
        root->addPanel("Sound", new sound::Panel());
        addChild(root);

        show(true);
    }

    void print(const PrintContext& context) const
    {
        if (context.pageIndex >= 2) {
            return;
        }

        auto margins = PicaPt(0.5f * 72.0f); // 1/2 inch
        auto r = Rect(PicaPt::kZero, PicaPt::kZero, context.paperSize.width, context.paperSize.height).insetted(margins, margins);

        auto &dc = context.dc;

        // --- debugging ---
        dc.drawText("UL", Point(PicaPt(-144.0f), PicaPt(-144.0f)),
                    Font("Georgia", PicaPt(72.0f)), kPaintFill);
        dc.drawText("LL", Point(PicaPt(-144.0f), PicaPt(32.0f)),
            Font("Georgia", PicaPt(72.0f)), kPaintFill);
        dc.drawText("UR", Point(PicaPt(0.0f), PicaPt(-144.0f)),
            Font("Georgia", PicaPt(72.0f)), kPaintFill);
        // ----

        // Draw the unimageable margins with light grey, to see if the rect is really correct.
        // If the grey is visible in the printout, then the imageableRect is not correct.
        // This may be the OS's fault: macOS 10.14 uses the wrong imageable bounds for
        // the Brother HL-L2370DW.
        dc.setFillColor(Color(0.75f, 0.75f, 0.75f));
        dc.drawRect(context.drawRect, kPaintFill);
        dc.setFillColor(Color(0.9f, 0.9f, 0.9f));
        dc.drawRect(context.imageableRect, kPaintFill);
        dc.setFillColor(Color::kWhite);
        dc.drawRect(r, kPaintFill);  // clearRect() may not work on p rint context in the OS (e.g. macOS)
        dc.setFillColor(Color::kBlack);

        // Draw the page rect (shouldn't be visible printed, but will be in a PDF)
        dc.setStrokeWidth(PicaPt(1));
        dc.setStrokeDashes({ PicaPt(1), PicaPt(2), PicaPt(2), PicaPt(2) }, PicaPt::kZero);
        // stroked rect will have the line at the outside on the right/bottom
        auto half = PicaPt(0.5f);
        dc.drawRect(context.drawRect.insetted(half, half), kPaintStroke);
        dc.setStrokeDashes({}, PicaPt::kZero);

        // Draw the imageable rect
        dc.setStrokeDashes({ PicaPt(1), PicaPt(1) }, PicaPt::kZero);
        dc.drawRect(context.imageableRect.insetted(half, half), kPaintStroke);
        dc.setStrokeDashes({}, PicaPt::kZero);

        // Draw ruler
        auto drawRuler = [](DrawContext& dc, const Point& origin, const PicaPt& length, const PicaPt& tickLength) {
            auto inch = PicaPt(72.0f);
            Font rulerFont("Georgia", 0.125f * inch);
            auto minorTickHeight = 0.5f * tickLength;
            auto majorTickHeight = tickLength;
            int i = 0;
            PicaPt x = origin.x, y = origin.y;
            while (x <= origin.x + length) {
                if (i % 2 == 0) {
                    dc.drawLines({ Point(x, y), Point(x, y + majorTickHeight )});
                    auto text = std::to_string(i / 2);
                    auto width = dc.textMetrics(text.c_str(), rulerFont).width;
                    if (i == 2) {
                        text += " in.";
                    }
                    auto textX = x - 0.5f * width;
                    if (i == 0) {
                        textX = x;
                    } else if (textX + width > origin.x + length) {
                        textX = origin.x + length - width;
                    }
                    dc.drawText(text.c_str(), Point(textX, y + majorTickHeight + 0.125f), rulerFont, kPaintFill);
                } else {
                    dc.drawLines({ Point(x, y), Point(x, y + minorTickHeight )});
                }
                x += 0.5f * inch;
                ++i;
            }
        };
        drawRuler(dc, Point::kZero, context.drawRect.maxX(), PicaPt(0.125f * 72.0f));
        drawRuler(dc, r.upperLeft(), r.width, PicaPt(0.0875f * 72.0f));
        dc.drawLines({ Point(r.x, r.y), Point(r.maxX(), r.y) });

        Font font("Georgia", PicaPt(12));
        auto lineHeight = font.metrics(dc).lineHeight;
        auto y = r.y + PicaPt(24.0f);
        dc.drawText(("Page " + std::to_string(context.pageIndex + 1) + " (" + std::to_string(dc.dpi()) + " dpi)").c_str(),
                    Rect(r.x, y, r.width, lineHeight),
                    Alignment::kTop | Alignment::kVCenter, TextWrapping::kWrapNone, font, kPaintFill);
        y += lineHeight;

        std::stringstream s;
        s << "Paper size (pts): " << context.paperSize.width.asFloat() << " x " << context.paperSize.height.asFloat()
          << ";  imageableRect: (" << context.imageableRect.x.asFloat() << ", " << context.imageableRect.y.asFloat() << ") "
          << context.imageableRect.width.asFloat() << " x " << context.imageableRect.height.asFloat();
        auto text = dc.createTextLayout(s.str().c_str(), Font("Georgia", PicaPt(8)), Color::kBlack, Size(r.width, PicaPt::kZero), Alignment::kTop | Alignment::kLeft);
        dc.drawText(*text, Point(r.x, y));
        y += lineHeight;

        text = dc.createTextLayout("If ruler is slightly missized, check physical paper size.\nSome printers change the print size to keep the L/R margins equal.", Font("Georgia", PicaPt(8)), Color::kBlack, Size(r.width, PicaPt::kZero), Alignment::kTop | Alignment::kRight);
        dc.drawText(*text, Point(r.x, r.y + PicaPt(24.0f)));

        auto styledFont = font.fontWithPointSize(PicaPt(9.0f));
        Text styled("Egypt Egypt Egypt Egypt C6H12O6 r2 = x2 + y2 single double dotted wavy under6 under7 under8 under9 under12 under16", styledFont, Color::kBlack);
        styled.setBold(6, 5);
        styled.setItalic(12, 5);
        styled.setBold(18, 5);
        styled.setItalic(18, 5);
        styled.setSubscript(25, 1);
        styled.setSubscript(27, 2);
        styled.setSubscript(30, 1);
        styled.setItalic(32, 1);
        styled.setSuperscript(33, 1);
        styled.setItalic(37, 1);
        styled.setSuperscript(38, 1);
        styled.setItalic(42, 1);
        styled.setSuperscript(43, 1);
        styled.setUnderlineStyle(kUnderlineSingle, 45, 6);
        styled.setUnderlineStyle(kUnderlineDouble, 52, 6);
        styled.setUnderlineStyle(kUnderlineDotted, 59, 6);
        styled.setUnderlineStyle(kUnderlineDotted, 66, 4);
        styled.setFont(styledFont.setPointSize(PicaPt(6.0f)), 71, 6);
        styled.setUnderlineStyle(kUnderlineSingle, 71, 6);
        styled.setFont(styledFont.setPointSize(PicaPt(7.0f)), 78, 6);
        styled.setUnderlineStyle(kUnderlineSingle, 78, 6);
        styled.setFont(styledFont.setPointSize(PicaPt(8.0f)), 85, 6);
        styled.setUnderlineStyle(kUnderlineSingle, 85, 6);
        styled.setUnderlineStyle(kUnderlineSingle, 92, 6);  // same font; 9pt
        styled.setFont(styledFont.setPointSize(PicaPt(12.0f)), 99, 7);
        styled.setUnderlineStyle(kUnderlineSingle, 99, 7);
        styled.setFont(styledFont.setPointSize(PicaPt(16.0f)), 107, 7);
        styled.setUnderlineStyle(kUnderlineSingle, 107, 7);
        dc.drawText(*dc.createTextLayout(styled, styledFont, Color::kBlack), Point(r.x, y));
        y += lineHeight;
        y += lineHeight;  // extra line

        dc.drawText("Fonts (may not space evenly due to font metrics)",
                    Point(r.x, y), font, kPaintFill);
        y += lineHeight;

        auto y0 = y;
        auto x = r.x;
        auto allFonts = Application::instance().availableFontFamilies();
        auto approxNRowsPerCol = (r.maxY() - y) / lineHeight;  // lineHeight so estimates high
        auto approxNCols = std::ceil(allFonts.size() / approxNRowsPerCol);
        auto colWidth = r.width / approxNCols;
        dc.save();
        dc.clipToRect(Rect(x, y, colWidth, r.maxY() - y));
        for (auto &fname : allFonts) {
            Font f(fname, PicaPt(9));
            auto h = dc.fontMetrics(f).lineHeight;
            if (y + h > r.maxY()) {
                y = y0;
                x += colWidth;
                dc.restore();
                dc.save();
                dc.clipToRect(Rect(x, y, colWidth, r.maxY() - y));
            }
            dc.drawText(fname.c_str(),
                        Rect(x, y, r.width, h),
                        Alignment::kTop | Alignment::kVCenter, TextWrapping::kWrapNone, f, kPaintFill);
            y += h;
        }
        dc.restore();
    }

private:
    struct MenuModel {
        static bool itemAdded; // this is a global setting, since it alters the menu structure
        bool testChecked = true;
        bool alphaChecked = true;
    } mModel;
};
bool Document::MenuModel::itemAdded = false;

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
#else
int main(int argc, char *argv[])
#endif
{
    Application app;
    //app.setExitWhenLastWindowCloses(true);

    // Since this is a test app, print out the time for something very simple, since
    // just the timer measurement takes less than a microsecond.
    // This will give us a hint what the actual accuracy of the timer is
    // for this platform:
    //     0.0:  worse than microsecond accuracy (should take one or two dozen usec)
    //    nn.0:  microsecong accuracy
    //  nn.nnn:  nanosecond accuracy
    auto t0 = app.microTime();
    app.setExitWhenLastWindowCloses(true); // we need to do this, even if we aren't timing
    auto t1 = app.microTime();
    app.debugPrint("[debug] measured time was " + std::to_string((t1 - t0) * 1e6) + " usec");

    auto *submenu = new Menu();
    submenu->addItem("Item 1", 20, ShortcutKey::kNone);
    submenu->addItem("Item 2", 21, ShortcutKey::kNone);
    submenu->addItem("Item 3", 22, ShortcutKey::kNone);
    auto subsubmenu = new Menu();
    subsubmenu->addItem("Alpha", kMenuIdAlpha, ShortcutKey::kNone);
    subsubmenu->addItem("Beta", kMenuIdBeta, ShortcutKey::kNone);
    subsubmenu->addItem("Toggle alpha action", kMenuIdToggleAlpha,
                        ShortcutKey(KeyModifier::kCtrl, Key::kA));
    submenu->addMenu("Subsubmenu", subsubmenu);
    submenu->addItem("Item 4", 23, ShortcutKey::kNone);

    auto *submenu2 = new Menu();
    submenu2->addItem("First", 40, ShortcutKey::kNone);
    submenu2->addItem("Second", 41, ShortcutKey::kNone);
    submenu2->addItem("Third", 42, ShortcutKey::kNone);

    auto *fileMenu =
        app.menubar().newMenu("File")
            ->addItem("New", kMenuIdNew, ShortcutKey(KeyModifier::kCtrl, Key::kN))
#ifndef __EMSCRIPTEN__
            ->addSeparator()
            ->addItem("Print...", kMenuIdPrint, ShortcutKey(KeyModifier::kCtrl, Key::kP))
#endif // __EMSCRIPTEN
        ;
//            ->addSeparator()
//            ->addItem("Quit", kMenuIdQuit, ShortcutKey(KeyModifier::kCtrl, Key::kQ));
    auto *editMenu =
        app.menubar().newMenu("Edit");
    auto *testMenu =
        app.menubar().newMenu("&Test")
            ->addItem("&Disabled", kMenuIdDisabled, ShortcutKey(KeyModifier::kCtrl, Key::kD))
            ->addItem("&Checkable", kMenuIdCheckable, ShortcutKey::kNone)
            ->addSeparator()
            ->addItem("这是一个 UTF8 标题", -1, ShortcutKey::kNone)
            ->addSeparator()
            ->addMenu("Submenu", submenu)
            ->addMenu("Submenu 2", submenu2)
            ->addSeparator()
            ->addItem("Add Item to Menu", kMenuIdAddItem, ShortcutKey::kNone)
            ->addItem("Direct callback", kMenuIdDirectCallback, ShortcutKey::kNone,
                      [](Window *w) { Dialog::showAlert(w, "Test", "Test >> Direct callback menu item activated", ""); });
    auto *emptyMenu =
        app.menubar().newMenu("Empty");
    auto *windowMenu =
        app.menubar().newMenu("Window");
    //app.menubar().newMenu("Help");
    app.menubar().addStandardItems(&fileMenu, &editMenu, &windowMenu, nullptr,
                                   { OSMenubar::StandardItem::kUndo,
                                     OSMenubar::StandardItem::kRedo });

    Document::createNewDocument();

    return app.run();
}
