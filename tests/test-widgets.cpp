//-----------------------------------------------------------------------------
// Copyright 2021 - 2023 Eight Brains Studios, LLC
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

class RootWidget : public Widget
{
    using Super = Widget;
public:
    RootWidget()
    {
        mPanelChooser = new ListView();
        mPanelChooser->setKeyNavigationWraps(true);
        mPanelChooser->setOnSelectionChanged([this](ListView *lv) {
            mPanels->setIndexShowing(lv->selectedIndex());
        });
        addChild(mPanelChooser);

        mPanels = new StackedWidget();
        addChild(mPanels);
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
        auto em = context.theme.params().labelFont.pointSize();
        auto chooserWidth = context.dc.roundToNearestPixel(10.0f * em);
        auto &r = bounds();
        mPanelChooser->setFrame(Rect(PicaPt::kZero, PicaPt::kZero, chooserWidth, r.height));
        mPanels->setFrame(Rect(chooserWidth, PicaPt::kZero, r.maxX() - chooserWidth, r.height));
        Super::layout(context);
    }

private:
    ListView *mPanelChooser;
    StackedWidget *mPanels;
};

static const MenuId kMenuIdNew = 1;
static const MenuId kMenuIdQuit = 2;
static const MenuId kMenuIdDisabled = 10;
static const MenuId kMenuIdCheckable = 11;
static const MenuId kMenuIdAddItem = 12;
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
        addChild(root);

        show(true);
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
    //     0.0:  worse than microsecond accuracy (should take a dozen or to usec)
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
        ->addItem("New", kMenuIdNew, ShortcutKey(KeyModifier::kCtrl, Key::kN));
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
            ->addItem("Add Item to Menu", kMenuIdAddItem, ShortcutKey::kNone);
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
