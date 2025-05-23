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

#include "X11Application.h"

#include "X11Clipboard.h"
#include "X11Window.h"
#include "../Application.h"
#include "../Events.h"
#include "../openal/OpenALSound.h"
#include "../themes/EmpireTheme.h"
#include "../private/PlatformUtils.h"

// For print dialog
#include "../io/File.h"
#include "../Button.h"
#include "../ComboBox.h"
#include "../Dialog.h"
#include "../FileDialog.h"
#include "../Label.h"
#include "../Layout.h"
#include "../NumberEdit.h"
#include "../Printing.h"
#include "../RadioButton.h"
#include "../StringEdit.h"
#include "../UIContext.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>

#include <sys/types.h>
#include <dirent.h>
#include <locale.h>
#include <string.h>  // for memset()
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <thread>
#include <unordered_map>

// How do we get the binary name? argv[0] may actually be null or incorrect
// (assuming someone is being nasty or ignoring the Posix convention with an
// execve(pathToBinary, NULL, NULL)). The BSDs have get getprogname() and
// GNU libc has the program_invocation_name global. However,
// program_invocation_name gives the full path (basically, argv[0]), which is
// actually not what we want for this. __progname gives just the binary name.
// This is non-standard, but seems well-supported.
extern char *__progname;
const char *gBinaryName = __progname;

namespace uitk {

namespace {
static const char *kDB_XftDPI = "Xft.dpi";
static const char *kDB_XftDPI_alt = "Xft.Dpi";

static const long kDoubleClickMaxMillisecs = 500;  // Windows' default
static const uitk::PicaPt kDoubleClickMaxRadiusPicaPt(2);  // 2/72 inch
}  // namespace

static std::unordered_map<KeySym, Key> gKeysym2key = {
    { XK_BackSpace, Key::kBackspace },
    { XK_Tab, Key::kTab },
    { XK_KP_Enter, Key::kEnter },
    { XK_Return, Key::kReturn },
    { XK_Escape, Key::kEscape },
    { XK_space, Key::kSpace },
    { XK_KP_Multiply, Key::kNumMultiply },
    { XK_KP_Add, Key::kNumPlus },
    { XK_KP_Separator, Key::kNumComma },
    { XK_KP_Subtract, Key::kNumMinus },
    { XK_KP_Decimal, Key::kNumPeriod },
    { XK_KP_Divide, Key::kNumSlash },
    { XK_Delete, Key::kDelete },
    { XK_Insert, Key::kInsert },
    { XK_Shift_L, Key::kShift },
    { XK_Shift_R, Key::kShift },
    { XK_Control_L, Key::kCtrl },
    { XK_Control_R, Key::kCtrl },
    { XK_Alt_L, Key::kAlt },
    { XK_Alt_R, Key::kAlt },
    { XK_Meta_L, Key::kMeta },
    { XK_Meta_R, Key::kMeta },
    { XK_Caps_Lock, Key::kCapsLock },
    { XK_Num_Lock, Key::kNumLock },
    { XK_Left, Key::kLeft },
    { XK_KP_Left, Key::kLeft },
    { XK_Right, Key::kRight },
    { XK_KP_Right, Key::kRight },
    { XK_Up, Key::kUp },
    { XK_KP_Up, Key::kUp },
    { XK_Down, Key::kDown },
    { XK_KP_Down, Key::kDown },
    { XK_Home, Key::kHome },
    { XK_KP_Home, Key::kHome },
    { XK_End, Key::kEnd },
    { XK_KP_End, Key::kEnd },
    { XK_Page_Up, Key::kPageUp },
    { XK_KP_Page_Up, Key::kPageUp },
    { XK_Page_Down, Key::kPageDown },
    { XK_KP_Page_Down, Key::kPageDown },
    { XK_F1, Key::kF1 },
    { XK_F2, Key::kF2 },
    { XK_F3, Key::kF3 }, 
    { XK_F4, Key::kF4 }, 
    { XK_F5, Key::kF5 }, 
    { XK_F6, Key::kF6 }, 
    { XK_F7, Key::kF7 }, 
    { XK_F8, Key::kF8 }, 
    { XK_F9, Key::kF9 }, 
    { XK_F10, Key::kF10 },
    { XK_F11, Key::kF11 },
    { XK_F12, Key::kF12 },
    { XK_Print, Key::kPrintScreen }
};

int toKeymods(unsigned int xstate) {
    int keymods = 0;
    if (xstate & ShiftMask) {
        keymods |= uitk::KeyModifier::kShift;
    }
    if (xstate & ControlMask) {
        keymods |= uitk::KeyModifier::kCtrl;
    }
    if (xstate & Mod1Mask) {
        keymods |= uitk::KeyModifier::kAlt;
    }
    if (xstate & Mod4Mask) {
        keymods |= uitk::KeyModifier::kMeta;
    }
    // Do not set numlock or capslock in the keymods, otherwise you have
    // to remember to mask them out when checking for other things, which
    // you are almost sure to forget to do.
    // (Mod2Mask is numlock and LockMask is capslock)
    return keymods;
}

// Adjusted from n-click detection in Win32Window.cpp.
// See https://devblogs.microsoft.com/oldnewthing/20041018-00/?p=37543 for
// pitfalls in detecting double-clicks, triple-clicks, etc.
class ClickCounter
{
public:
    ClickCounter()
    {
        reset();
    }

    int nClicks() const { return mNClicks;  }

    void reset()
    {
        mLastClickTime = Time(-1);  // also will exercise rollover code path!
        mLastClickWindow = 0;
        mButton = -1;
        mNClicks = 0;
    }

    int click(X11Window *w, const XButtonEvent& e)
    {
        if (!w) {  // should never happen, but prevents a crash with maxDistPx
            reset();
            return 0;
        }

        int maxRadiusPx = std::max(1, int(std::round(kDoubleClickMaxRadiusPicaPt.toPixels(w->dpi()))));

        // 'Time' turns out to be unsigned long, so once every 49.7 days
        // a double click can get missed. Since it is unsigned, we cannot use
        // Raymond Chen's rollover trick, and have to detect the rollover.
        Time dt;
        if (e.time >= mLastClickTime) {
            dt = e.time - mLastClickTime;
        } else {
            dt = (Time(-1) - mLastClickTime) + e.time;
        }

        if (w != mLastClickWindow || e.button != mButton
            || std::abs(e.x - mLastClickX) > maxRadiusPx
            || std::abs(e.y - mLastClickY) > maxRadiusPx
            || dt > kDoubleClickMaxMillisecs) {
            mButton = e.button;
            mNClicks = 0;
        }
        mNClicks++;

        mLastClickTime = e.time;
        mLastClickWindow = w;
        mLastClickX = e.x;
        mLastClickY = e.y;

        return mNClicks;
    }

private:
    unsigned int mButton;
    int mNClicks;
    Time mLastClickTime;
    X11Window *mLastClickWindow;  // we do not own this
    int mLastClickX = 0;
    int mLastClickY = 0;
};

class PrintDialog : public Dialog
{
    using Super = Dialog;
public:
    explicit PrintDialog(const PrintSettings& settings)
    {
        mKnownSizes = PaperSize::knownSizes(); // copies
        auto paperSize = settings.paperSize;
        if (paperSize.width < PicaPt(1.0f) || paperSize.height < PicaPt(1.0f)) {
            paperSize = Application::instance().defaultPaperSize();
        }

        VLayout *layout = new VLayout();
        GridLayout *grid = new GridLayout();
        int row = 0;

        HLayout *sizeLayout = new HLayout();
        mPaperWidthEdit = new NumberEdit();
        mPaperWidthEdit->setFixedWidthEm(4.0f);
        mPaperHeightEdit = new NumberEdit();
        mPaperHeightEdit->setFixedWidthEm(4.0f);

        // What should the max be? Could be a long sheet of paper that is
        // a poster or something, so just use a big number.
        const double kMaxPaperSize = 1e6;

        std::string units;
        float widthInches = paperSize.width.asFloat() / 72.0f;
        float heightInches = paperSize.height.asFloat() / 72.0f;
        if ((std::floor(widthInches * 4.0f) == widthInches * 4.0f) && 
            (std::floor(heightInches * 4.0f) == heightInches * 4.0f))
        {
            mPtsToUnits = 1.0f / 72.0f;
            units = "in.";
            mPaperWidthEdit->setLimits(0.0, kMaxPaperSize, 0.01);  // 2 digits, for 0.25
            mPaperHeightEdit->setLimits(0.0, kMaxPaperSize, 0.01);
        } else {
            mPtsToUnits = 25.4f / 72.0f;
            units = "mm";
            mPaperWidthEdit->setLimits(0.0, kMaxPaperSize, 1.0);
            mPaperHeightEdit->setLimits(0.0, kMaxPaperSize, 1.0);
        }

        sizeLayout->setAlignment(Alignment::kRight);
        sizeLayout->addChild(mPaperWidthEdit);
        sizeLayout->addChild(new Label(units + " x "));
        sizeLayout->addChild(mPaperHeightEdit);
        sizeLayout->addChild(new Label(units));

        const int kCustomPaperValue = -1;
        int paperSelectionValue = kCustomPaperValue;
        mPaperSizes = new ComboBox();
        for (size_t i = 0;  i < mKnownSizes.size();  ++i) {
            mPaperSizes->addItem(mKnownSizes[i].name, i);
            if (std::abs(mKnownSizes[i].width.asFloat() - paperSize.width.asFloat()) < 1e-4f &&
                std::abs(mKnownSizes[i].height.asFloat() - paperSize.height.asFloat()) < 1e-4f)
            {
                paperSelectionValue = i;
            }
        }
        mPaperSizes->addItem("Custom", kCustomPaperValue);
        mPaperSizes->setSelectedValue(paperSelectionValue);
        mPaperSizes->setOnSelectionChanged([this](ComboBox *) {
            this->updateUI();
        });

        if (paperSelectionValue == kCustomPaperValue) {
            mPaperWidthEdit->setValue(paperSize.width.asFloat() * mPtsToUnits);
            mPaperHeightEdit->setValue(paperSize.height.asFloat() * mPtsToUnits);
        }

        grid->addChild(new Label("Paper size"), row, 0);
        grid->addChild(mPaperSizes, row, 1);
        ++row;
        grid->addChild(sizeLayout, row, 1);
        ++row;

        mOrientations = new ComboBox();
        mOrientations->addItem("Portrait", int(PaperOrientation::kPortrait));
        mOrientations->addItem("Landscape", int(PaperOrientation::kLandscape));
        mOrientations->setSelectedValue(int(settings.orientation));

        grid->addChild(new Label("Paper orientation"), row, 0);
        grid->addChild(mOrientations, row, 1);
        ++row;

        mFilename = new StringEdit();
        mFilename->setOnTextChanged([this](const std::string&) { this->updateUI(); });

        auto *fileDlgButton = new Button("...");
        fileDlgButton->setOnClicked([this](Button*) {
            FileDialog *fd = new FileDialog(FileDialog::Type::kSave);
            fd->addAllowedType("pdf", "PDF");
            fd->addAllowedType("", "All files");
            fd->showModal(nullptr, [this, fd](Dialog::Result result, int i) {
                if (result == Dialog::Result::kFinished) {
                    this->mFilename->setText(fd->selectedPath());
                    this->updateUI();
                }
                delete fd;
            });
        });
        
        mAllPages = new RadioButton("All pages");
        mRangePages = new RadioButton("Range");
        mAllPages->setOn(true);
        mAllPages->setOnClicked([this](Button *) {
            mRangePages->setOn(false);
            updateUI();
        });
        mRangePages->setOnClicked([this](Button *) {
            mAllPages->setOn(false);
            updateUI();
        });
        mStartPage = new StringEdit();
        mStartPage->setAlignment(Alignment::kRight);
        mStartPage->setFixedWidthEm(4);
        mStartPage->setOnValueChanged([this](StringEdit*) { updateUI(); });
        mEndPage = new StringEdit();  // StringEdit, so can be empty, not 1e9
        mEndPage->setAlignment(Alignment::kRight);
        mEndPage->setFixedWidthEm(4);
        mEndPage->setOnValueChanged([this](StringEdit*) { updateUI(); });

        grid->addChild(new Label(" "), row, 0);  // blank line
        ++row;
        grid->addChild(mAllPages, row, 0);
        ++row;
        grid->addChild(mRangePages, row, 0);
        grid->addChild((new HLayout({ mStartPage, new Label("to"), mEndPage }))->setAlignment(Alignment::kLeft),
                       row, 1);
        ++row;

        grid->addChild(new Label(" "), row, 0);  // blank line
        ++row;
        grid->addChild(new Label("Filename"), row, 0);
        grid->addChild(new HLayout({ mFilename, fileDlgButton }), row, 1);
        ++row;

        mOkButton = new Button("Ok");
        auto *cancelButton = new Button("Cancel");
        mOkButton->setOnClicked([this](Button *b) { this->finish(1); });
        cancelButton->setOnClicked([this](Button *b) { this->cancel(); });
        auto *buttonRow = new HLayout();
        buttonRow->addStretch();
        buttonRow->addChild(mOkButton);
        buttonRow->addChild(cancelButton);

        layout->setMargins(Application::instance().theme()->params().dialogMargins);
        layout->addChild(grid);
        layout->addSpacingEm(1.0f);
        layout->addStretch();  // so buttons do not expand
        layout->addChild(buttonRow);

        addChild(layout);
        setTitle("Print to PDF");

        updateUI();
    }

    PaperSize paperSize() const
    {
        auto idx = mPaperSizes->selectedValue();
        if (idx >= 0) {
            return mKnownSizes[idx];
        } else {
            return PaperSize(PicaPt(mPaperWidthEdit->doubleValue() / mPtsToUnits),
                             PicaPt(mPaperHeightEdit->doubleValue() / mPtsToUnits),
                             "Custom");
        }
    }

    PaperOrientation orientation() const {
        return (PaperOrientation)mOrientations->selectedValue();
    }

    std::string filename() const { return mFilename->text(); }

    int startPage() const
    {
        if (mAllPages->isOn()) {
            return 1;
        } else {
            int start = std::max(1, std::atoi(mStartPage->text().c_str()));
            return start;
        }
    }

    int endPage() const
    {
        if (mAllPages->isOn()) {
            return kMaxPage;
        } else {
            int end = std::max(1, std::atoi(mEndPage->text().c_str()));
            return end;
        }
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        auto em = context.theme.params().labelFont.pointSize();
        return Size(35.0f * em, 23.0f * em);
    }

protected:
    void updateUI()
    {
        auto sizeIdx = mPaperSizes->selectedValue();
        mPaperWidthEdit->setEnabled(sizeIdx < 0);
        mPaperHeightEdit->setEnabled(sizeIdx < 0);
        if (sizeIdx >= 0) {
            auto &paperSize = mKnownSizes[sizeIdx];
            mPaperWidthEdit->setValue(paperSize.width.asFloat() * mPtsToUnits);
            mPaperHeightEdit->setValue(paperSize.height.asFloat() * mPtsToUnits);
        }

        int start = 1;
        int end = kMaxPage;
        if (!mStartPage->text().empty()) {
            start = std::atoi(mStartPage->text().c_str());
            start = std::max(1, start);  // an error in atoi gives 0, which will
                                         // be conveniently maxned to 1
            mStartPage->setText(std::to_string(start));
        }
        if (!mEndPage->text().empty()) {
            end = std::atoi(mEndPage->text().c_str());
            end = std::max(start, end);
            mEndPage->setText(std::to_string(end));
        }
        mStartPage->setEnabled(mRangePages->isOn());
        mEndPage->setEnabled(mRangePages->isOn());

        bool rangeOk = (mAllPages->isOn() || (!mStartPage->text().empty() && !mEndPage->text().empty() && end >= start));
        bool isOk = (!mFilename->text().empty() && rangeOk);
        mOkButton->setEnabled(isOk);
    }

private:
    static constexpr int kMaxPage = 1000000000;

    float mPtsToUnits;
    std::vector<PaperSize> mKnownSizes;
    ComboBox *mPaperSizes;
    ComboBox *mOrientations;
    NumberEdit *mPaperWidthEdit;
    NumberEdit *mPaperHeightEdit;
    RadioButton *mAllPages;
    RadioButton *mRangePages;
    StringEdit *mStartPage;
    StringEdit *mEndPage;
    StringEdit *mFilename;
    Button *mOkButton;
};

//-----------------------------------------------------------------------------
struct X11Application::Impl
{
    Display *display;
    XIM xim;
    // The database and strings are per-screen, with 0 assumed to be default
    std::map<std::string, std::string> xrdbStrings;
    std::vector<std::map<std::string, std::string>> xrdbScreenStrings;
    std::unordered_map<::Window, X11Window*> xwin2window;
    ClickCounter clickCounter;
    std::unique_ptr<X11Clipboard> clipboard;
    std::unique_ptr<OpenALSound> sound;

    Atom postedFuncAtom;
    std::mutex postedFunctionsLock;
    // This is a linked list because adding and removing does not invalidate
    // iterators.
    std::list<std::function<void()>> postedFunctions;

    DeferredFunctions<::Window> postedLater;  // note: has its own lock
};

X11Application::X11Application()
    : mImpl(new Impl())
{
    // Required to read in user values of the LC_* variables. These influence
    // the default fonts that Pango chooses.
    setlocale(LC_ALL, "");

    mImpl->display = XOpenDisplay(NULL);

    mImpl->clipboard = std::make_unique<X11Clipboard>(mImpl->display);
    mImpl->sound = std::make_unique<OpenALSound>();

    mImpl->postedFuncAtom = XInternAtom(mImpl->display, "PostedFunction", False);

    // Read the resource databases from each screen.
    int nScreens = XScreenCount(mImpl->display);
    mImpl->xrdbScreenStrings.resize(nScreens);
    XrmInitialize();

    static const std::vector<std::string> kQueryStrings =
        { kDB_XftDPI, kDB_XftDPI_alt };

    auto readDatabase = [](char *resourceString,
                           std::map<std::string, std::string>& keyVal) {
        char *type = NULL;
        XrmValue value;

        auto db = XrmGetStringDatabase(resourceString);
        for (auto &key : kQueryStrings) {
            if (XrmGetResource(db, key.c_str(), "String", &type, &value) == True) {
                keyVal[key] = std::string(value.addr);
            }
        }
        XrmDestroyDatabase(db);
    };

    // Read the global resources. Docs say to NOT free the string.
    auto resourceString = XResourceManagerString(mImpl->display);
    readDatabase(resourceString, mImpl->xrdbStrings);

    // Read the per-screen resources, in case you can set Xft.dpi separately
    // per-screen (which seems like a good idea).
    for (int sn = 0; sn < nScreens; ++sn) {
        ::Screen *s = XScreenOfDisplay(mImpl->display, sn);
        auto resourceString = XScreenResourceString(s);
        if (resourceString) {
            readDatabase(resourceString, mImpl->xrdbScreenStrings[sn]);
            XFree(resourceString);  // docs say MUST free this string
        }
    }

    // Create the input method
    char* modstr = XSetLocaleModifiers("");  // read from $XMODIFIERS env variable
    mImpl->xim = XOpenIM(mImpl->display, 0, 0, 0);
    if (!mImpl->xim) {
        std::cerr << "[uitk] Could not open input method in XMODIFIERS (" << modstr << ")" << std::endl;
        XSetLocaleModifiers("@im=none");
        mImpl->xim = XOpenIM(mImpl->display, 0, 0, 0);
    }
}

X11Application::~X11Application()
{
    XCloseIM(mImpl->xim);
    XCloseDisplay(mImpl->display);
}

void X11Application::setExitWhenLastWindowCloses(bool exits)
{
    // Do nothing, this is pretty much always true on Linux, as there would
    // be no way to open a new window after the last one closes.
}

std::string X11Application::applicationName() const
{
    return std::string(gBinaryName);
}

std::string X11Application::appDataPath() const
{
    char dest[PATH_MAX + 1];
    memset(dest,0,sizeof(dest)); // readlink() does not null terminate!
    if (readlink("/proc/self/exe", dest, PATH_MAX) == -1) {
        std::cerr << "[uitk] appDataPath(): could not read /proc/self/exe" << std::endl;
        strcpy(dest, "./");
    }
    std::string exePath(dest);
    auto lastSlash = exePath.rfind('/');
    auto firstSlash = exePath.find('/');
    if (lastSlash == firstSlash) {  // unlikely...
        return "/";
    }
    auto exeDir = exePath.substr(0, lastSlash);
    auto startExeDir = exeDir.rfind('/');
    auto dirname = exeDir.substr(startExeDir + 1);
    if (dirname == "bin") {
        auto shareDir = exeDir.substr(0, startExeDir) + "/share";
        // If shareDir exists, return it as the data dir
        DIR *dir = opendir(shareDir.c_str());
        if (dir) { // exists
            closedir(dir);
            return shareDir;
        }
        // does not exist
    }
    return exeDir;
}

std::string X11Application::tempDir() const
{
    return "/tmp";
}

std::vector<std::string> X11Application::availableFontFamilies() const
{
    return Font::availableFontFamilies();
}

void X11Application::beep()
{
    if (mImpl->display) {
        XBell(mImpl->display, 0 /* base volume, ranges [-100, 100]*/);
    }
}

Sound& X11Application::sound() const
{
    return *mImpl->sound;
}

void X11Application::debugPrint(const std::string& s) const
{
    std::cout << s << std::endl;
}

void X11Application::printDocument(const PrintSettings& settings) const
{
    assert(settings.calcPages);
    assert(settings.drawPage);

    auto *win = Application::instance().activeWindow();  // nullptr is okay

    auto *dlg = new PrintDialog(settings);
    dlg->showModal(win, [dlg, settings/*copy, orig is on stack*/](Dialog::Result r, int i) {
        if (r == Dialog::Result::kFinished) {
            auto theme = Application::instance().theme();
            auto paperSize = dlg->paperSize();  // copies
            if (dlg->orientation() == PaperOrientation::kLandscape) {
                std::swap(paperSize.width, paperSize.height);
            }
            int width = std::ceil(paperSize.width.asFloat());
            int height = std::ceil(paperSize.height.asFloat());
            int nPages = 0;
            {
            auto layoutDC = DrawContext::createCairoPDF(nullptr, width, height, 72.0f);
            LayoutContext layoutContext = { *theme, *layoutDC };
            nPages = settings.calcPages(paperSize, layoutContext);
            }  // releases layoutDC

            auto dc = DrawContext::createCairoPDF(dlg->filename().c_str(), width, height, 72.0f);
            Rect pageRect(PicaPt::kZero, PicaPt::kZero, paperSize.width, paperSize.height);
            PrintContext context {
                *theme,
                *dc,
                pageRect,
                true,  // window is active
                Size(paperSize.width, paperSize.height),
                pageRect,  // we do not know the imageable bounds
                0
            };

            int pagesFinished = 0;
            int startPageIdx = dlg->startPage() - 1;
            int endPageIdx = std::min(dlg->endPage() - 1, nPages - 1);
            dc->beginDraw();
            for (int i = startPageIdx;  i <= endPageIdx;  ++i) {
                if (pagesFinished > 0) {   // i might always be > 0!
                    dc->addPage();
                }
                context.pageIndex = i;
                settings.drawPage(context);
                ++pagesFinished;
            }
            dc->endDraw();
            dc.reset();  // force destruction, so file should be written
            
            // Note: if the file already exists, but we could not write to it,
            // we will not notify the user. However, to do that we would need
            // for nativedraw to return native errors.
            if (!File(dlg->filename()).exists()) {
                Dialog::showAlert(nullptr, "Print Error", "Could not print to file '" + dlg->filename() + "'", "Check that the path is writable and that the disk has enough space.");
            }
        }

        delete dlg;
    });
}

bool X11Application::isOriginInUpperLeft() const { return true; }

// The question is really whether (0, 0) is inside the border or not.
// If X11 draws the border outside the window, but the window manager positions
// the window such that the corner of the border is at (x, y) instead of the
// corner of the window, then it is effectively the same thing. Of course,
// window managers may do this differently, which will be a disaster for us.
bool X11Application::isWindowBorderInsideWindowFrame() const { return true; }

bool X11Application::windowsMightUseSameDrawContext() const { return false; }

bool X11Application::shouldHideScrollbars() const { return false; }

bool X11Application::canKeyFocusEverything() const { return true; }

bool X11Application::platformHasMenubar() const { return true; }

Clipboard& X11Application::clipboard() const { return *mImpl->clipboard; }

void X11Application::scheduleLater(uitk::Window* w, std::function<void()> f)
{
    {
    std::lock_guard<std::mutex> locker(mImpl->postedFunctionsLock);
    mImpl->postedFunctions.push_back(f);
    }

    bool canSend = true;
    ::Window to;
    if (w) {
        to = (::Window)w->nativeHandle();
    } else {
        // Any window will do, we just want an event so that the event loop
        // handles the message soon. We know that we have at least one
        // window, otherwise we would no longer be runing.
        auto it = mImpl->xwin2window.begin();
        if (it != mImpl->xwin2window.end()) {  // check, just in case...
            to = it->first;
        } else {
            canSend = false;
        }
    }

    if (canSend) {
        XEvent xe;
        xe.type = ClientMessage;
        xe.xclient.type = ClientMessage;  // maybe X server sets this?
        xe.xclient.window = to;
        xe.xclient.message_type = mImpl->postedFuncAtom;
        xe.xclient.format = 32;  // 8, 16, 32 (size of xclient.data), we do
                                 // not use that, so this does not matter
        XSendEvent(mImpl->display, xe.xclient.window, False, NoEventMask, &xe);
    }
    // Else: the function is still posted, we just have no way to send an event
}

OSApplication::SchedulingId X11Application::scheduleLater(
                               Window* w, float delay, bool repeat,
                               std::function<void(SchedulingId)> f)
{
    return mImpl->postedLater.add((::Window)w->nativeHandle(), delay, repeat, f);
}

void X11Application::cancelScheduled(SchedulingId id)
{
    mImpl->postedLater.remove(id);
}

int X11Application::run()
{
    Atom kWMProtocolType = XInternAtom(mImpl->display, "WM_PROTOCOLS", True);
    Atom kWMDeleteMsg = XInternAtom(mImpl->display, "WM_DELETE_WINDOW", False);
    Atom kClipboard = XInternAtom(mImpl->display, "CLIPBOARD", False);
    Atom kPrimary = XInternAtom(mImpl->display, "PRIMARY", False);
    Atom kClipboardTargets = XInternAtom(mImpl->display, "TARGETS", False);

    bool done = false;
    XEvent event;
    while (!done) {
        // There is a way to use select() to timeout on the file descriptors
        // Xlib uses under the hood, but small timeouts caused large latencies.
        // Anyway, this method is simpler and does not require arcane knowledge
        // gathered from dusty tomes.
        // (See https://www.linuxquestions.org/questions/programming-9/xnextevent-select-409355)
        while (!XPending(mImpl->display)) {
            mImpl->postedLater.executeTick();
            if (!XPending(mImpl->display)) {
                // std::this_thread::yield() produces fairly high CPU usage,
                // since multicore CPUs generally can reschedule the thread
                // immediately. Even 1 ms is enough to drop CPU usage down to
                // almost the level if this loop is eliminated. (Using a magic
                // number here because this is highly context dependent and
                // it we put it anywhere else we'd have to explain this whole
                // thing about the timers anyway.)
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
        }

        XNextEvent(mImpl->display, &event);

        X11Window *w;
        auto wit = mImpl->xwin2window.find(event.xany.window);
        if (wit != mImpl->xwin2window.end()) {
            w = wit->second;
        } else {
            // We only want to send IME events if a window is actually editing
            // text. However, this is not for any of our windows. But we need
            // to filter for the IME before continuing, as the IME does get
            // some messages not to our window. If we do not filter here, the
            // IME never becomes active and we do not filter below, either.
            XFilterEvent(&event, None);  // might be IME enabling/disabling
            continue;  // we know nothing about this window, ignore event
        }

        // Check with XIM to see if this is an IME event, and ignore if it is.
        // Note that if we are not editing, we do not want to send it to the
        // IME, since that will look odd to the user if a widget handles
        // alphabetical key presses. (The IME only gets the events if we
        // call XFilterEvent()).
        if (w->isEditing()) {
            if (XFilterEvent(&event, None)) {
                continue;
            }
        }

        switch (event.type) {
            case Expose:  // GraphicsExpose only happens for XCopyArea/XCopyPlane
                w->onDraw();
                break;
            case ConfigureNotify:
                // This gets called when a window is moved, resized, raised,
                // lowered, or border width is changed. We only need to resize
                // on resize (and move if moved to a different screen),
                // but the others are rare enough.
                w->onResize();
                break;
            //case ResizeRequest:
            //    // Determine proper size here (e.g. enforce minimum size
            //    // or aspect ratio), then resize appropriately
            //    // XResizeWindow(mImpl->display, event.xresizerequest.window,
            //                  event.xresizerequest.width,
            //                  event.xresizerequest.height);
            //    w->onResize();
            //    break;
            case MotionNotify: {
                int buttons = 0;
                if (event.xmotion.state & Button1MotionMask) {
                    buttons |= int(MouseButton::kLeft);
                }
                if (event.xmotion.state & Button2MotionMask) {
                    buttons |= int(MouseButton::kRight);
                }
                if (event.xmotion.state & Button3MotionMask) {
                    buttons |= int(MouseButton::kMiddle);
                }
                if (event.xmotion.state & Button4MotionMask) {
                    buttons |= int(MouseButton::kButton4);
                }
                if (event.xmotion.state & Button5MotionMask) {
                    buttons |= int(MouseButton::kButton5);
                }
                MouseEvent me;
                if (buttons == 0) {
                    me.type = MouseEvent::Type::kMove;
                } else {
                    me.type = MouseEvent::Type::kDrag;
                    me.drag.buttons = buttons;
                }
                me.keymods = toKeymods(event.xmotion.state);
                w->onMouse(me, event.xmotion.x, event.xmotion.y);
                break;
            }
            case ButtonPress:  // fall through
            case ButtonRelease:
            {
                MouseEvent me;
                if (event.type == ButtonPress) {
                    me.type = MouseEvent::Type::kButtonDown;
                    me.button.nClicks = mImpl->clickCounter.click(w, event.xbutton);
                } else {
                    me.type = MouseEvent::Type::kButtonUp;
                    me.button.nClicks = 0;
                }
                me.keymods = toKeymods(event.xmotion.state);
                switch (event.xbutton.button) {
                    default:
                    case Button1:
                        me.button.button = MouseButton::kLeft;
                        break;
                    case Button2:
                        me.button.button = MouseButton::kMiddle;
                        break;
                    case Button3:
                        me.button.button = MouseButton::kRight;
                        break;
                    case Button4:
                        me.type = MouseEvent::Type::kScroll;
                        me.scroll.dx = PicaPt::kZero;
                        me.scroll.dy = PicaPt(1.0f);
                        break;
                    case Button5:
                        me.type = MouseEvent::Type::kScroll;
                        me.scroll.dx = PicaPt::kZero;
                        me.scroll.dy = PicaPt(-1.0f);
                        break;
                    case Button5 + 1:
                        me.type = MouseEvent::Type::kScroll;
                        me.scroll.dx = PicaPt(1.0f);
                        me.scroll.dy = PicaPt::kZero;
                        break;
                    case Button5 + 2:
                        me.type = MouseEvent::Type::kScroll;
                        me.scroll.dx = PicaPt(-1.0f);
                        me.scroll.dy = PicaPt::kZero;
                        break;
                    case Button5 + 3:
                        me.button.button = MouseButton::kButton4;
                        break;
                    case Button5 + 4:
                        me.button.button = MouseButton::kButton5;
                        break;
                }
                
                bool ignore = (event.type == ButtonRelease &&
                               event.xbutton.button >= Button4 &&
                               event.xbutton.button <= Button5 + 2);
                if (!ignore) {
                    w->onMouse(me, event.xbutton.x, event.xbutton.y);
                }
                break;
            }
            case KeyPress:  // fall through
            case KeyRelease: {
                mImpl->clickCounter.reset();

                KeySym ksym = XLookupKeysym(&event.xkey, 0);
                Key key;
                if (ksym >= XK_A && ksym <= XK_Z) {
                    key = Key(int(Key::kA) + (ksym - XK_A));
                } else if (ksym >= XK_a && ksym <= XK_z) {
                    key = Key(int(Key::kA) + (ksym - XK_a));
                } else if (ksym >= XK_0 && ksym <= XK_9) {
                    key = Key(int(Key::k0) + (ksym - XK_0));
                } else {
                    auto kit = gKeysym2key.find(ksym);
                    if (kit != gKeysym2key.end()) {
                        key = kit->second;
                    } else {
                        key = Key::kUnknown;
                    }
                }

                // A KeyPress event with ksym 0x0 indicates a IME conversion
                // result, rather than a key press.  See
                // https://www.x.org/releases/X11R7.7/doc/libX11/libX11/libX11.html#Input_Method_Overview
                // under "Synchronization Convenstions"
                bool isIMEConversion = (event.type == KeyPress && ksym == 0x0);
                
                KeyEvent ke;
                ke.type = (event.type == KeyPress ? KeyEvent::Type::kKeyDown
                                                  : KeyEvent::Type::kKeyUp);
                ke.key = key;
                ke.nativeKey = ksym;
                ke.keymods = toKeymods(event.xkey.state);
                ke.isRepeat = false;  // TODO: figure this out
                if (!isIMEConversion) {
                    w->onKey(ke);
                }

                bool noMods = !(ke.keymods & (~uitk::KeyModifier::kShift));
                if (event.type == KeyPress && noMods &&
                    ((ksym >= 0x0020 && ksym <= 0xfdff) || // most languages
                     (ksym >= XK_braille_dot_1 && ksym <= XK_braille_dot_10) ||
                     (ksym >= 0x10000000 && ksym < 0x11000000) ||
                     ksym == 0x0 /* IME conversion */)) {
                    XIC xic = static_cast<XIC>(w->xic());
                    char utf8[1024];
                    Status status;
                    int len = Xutf8LookupString(xic, &event.xkey, utf8, 1024,
                                                NULL, &status);
                    utf8[len] = '\0';  // Xutf8LookupString does not add \0

                    TextEvent te;
                    te.utf8 = utf8;
                    w->onText(te);
                }

                break;
            }
            case DestroyNotify:  // get with StructureNotifyMask
                // Should not happen, window should be unregistered

                // Note that the window is destroyed, it is too late to
                // call w->onWindowWillClose(). Instead, this is done in
                // X11Window::close(), right before destruction.
                // Also note that X11Window::close() needs to unregister
                // the window, but we do it here just in case.
                unregisterWindow(wit->first);  // just in case
                break;
            case FocusIn:
                w->onActivated(w->currentMouseLocation());
                mImpl->clickCounter.reset();
                // X11 makes the clipboard owned by the window, instead of
                // globally, which is how it functions (and how macOS and Win32
                // expose it). To avoid exporting this lousy interface, we
                // keep track of the active window so that the clipboard class
                // can do a copy at any time without the caller needing to
                // be aware of this mess.
                mImpl->clipboard->setActiveWindow(event.xany.window);
                break;
            case FocusOut:
                w->onDeactivated();
                mImpl->clickCounter.reset();
                break;
            case KeymapNotify:
                // Update keyboard state
                break;
            case SelectionClear: {  // lost clipboard ownership
                // We consider the clipboard to be global to us, so if the
                // new window is still our window, we do not consider this
                // losing ownership. (Also, this prevents us from incorrectly
                // clearing our knowledge of ownership if we cut/copy from a
                // different window of ours.)
                auto newOwner = XGetSelectionOwner(mImpl->display,
                                                event.xselectionclear.selection);
                auto clipWinIt = mImpl->xwin2window.find(newOwner);
                if (clipWinIt == mImpl->xwin2window.end()) {
                    auto which = (event.xselectionclear.selection == kClipboard
                              ? X11Clipboard::Selection::kClipboard
                              : X11Clipboard::Selection::kTextSelection);
                    mImpl->clipboard->weAreNoLongerOwner(which);
                }
                break;
            }
            case SelectionRequest: {  // someone wants to paste
                if (event.xselectionrequest.selection != kClipboard &&
                    event.xselectionrequest.selection != kPrimary) {
                    break;
                }
                auto which = (event.xselectionrequest.selection == kClipboard
                              ? X11Clipboard::Selection::kClipboard
                              : X11Clipboard::Selection::kTextSelection);

                XSelectionEvent e = {0};
                e.type = SelectionNotify;
                e.display = event.xselectionrequest.display;
                e.requestor = event.xselectionrequest.requestor;
                e.selection = event.xselectionrequest.selection;
                e.time = event.xselectionrequest.time;
                e.target = event.xselectionrequest.target;
                e.property = event.xselectionrequest.property;

                //char* name = XGetAtomName(instance.display, ev.target);
                //std::cout << "target " << name << std::endl;
                //XFree(name);

                if (e.target == kClipboardTargets) {
                    auto targets = mImpl->clipboard->supportedTypes(which);
                    XChangeProperty(mImpl->display, e.requestor, e.property,
                                    XA_ATOM, 32, PropModeReplace,
                                    targets.data(), targets.size());
                } else if (mImpl->clipboard->doWeHaveDataForTarget(which, e.target)) {
                    auto data = mImpl->clipboard->dataForTarget(which, e.target);
                    XChangeProperty(mImpl->display, e.requestor,
                                    e.property, e.target, 8, PropModeReplace,
                                    data.data(), data.size());
                } else {
                    e.property = None;
                }

                XSendEvent(mImpl->display, e.requestor, 0, 0, (XEvent *)&e);
                break;
            }
            case ClientMessage:
                if (event.xclient.message_type == kWMProtocolType) {
                    if (event.xclient.data.l[0] == kWMDeleteMsg) {
                        w->close();
                    }
                } else if (event.xclient.message_type == mImpl->postedFuncAtom) {
                    // The posted function might generate another posted function
                    // (for example, an animation), so we only run the functions
                    // that we have right now. Also, we need to not be holding
                    // the lock when we run the function, otherwise posting a
                    // function will deadlock.
                    size_t n = 0;
                    mImpl->postedFunctionsLock.lock();
                    n = mImpl->postedFunctions.size();
                    mImpl->postedFunctionsLock.unlock();

                    for (size_t i = 0; i < n; ++i) {
                        std::function<void()> f;
                        mImpl->postedFunctionsLock.lock();
                        f = *mImpl->postedFunctions.begin();  // copy
                        mImpl->postedFunctionsLock.unlock();

                        f();

                        mImpl->postedFunctionsLock.lock();
                        mImpl->postedFunctions.erase(mImpl->postedFunctions.begin());
                        mImpl->postedFunctionsLock.unlock();
                    }
                }
                break;
            default:
                break;
        }

        if (mImpl->xwin2window.empty()) {
            done = true;
        }
    }
}

void X11Application::exitRun()
{
    // We do not need to do anything here because this should only be called
    // from Application::quit(), which will have closed all the windows, causing
    // us to exit.
}

Theme::Params X11Application::themeParams() const
{
    return EmpireTheme::defaultParams();
}

void* X11Application::display() const
{
    return (void*)mImpl->display;
}

void* X11Application::xim() const
{
    return (void*)mImpl->xim;
}

void X11Application::registerWindow(long unsigned int xwindow,
                                    X11Window *window)
{
    mImpl->xwin2window[(::Window)xwindow] = window;
}

void X11Application::unregisterWindow(long unsigned int xwindow)
{
    mImpl->xwin2window.erase((::Window)xwindow);
    mImpl->postedLater.removeForWindow(xwindow);
}

float X11Application::dpiForScreen(int screen)
{
    if (screen >= mImpl->xrdbScreenStrings.size()) {
        screen = 0;
    }

    auto findXftDPI = [](const std::map<std::string, std::string>& db) {
        auto it = db.find(kDB_XftDPI);
        if (it == db.end()) {
            it = db.find(kDB_XftDPI_alt);
        }
        if (it != db.end()) {
            return it->second;
        }
        return std::string("");
    };

    // Check if Xft.dpi (or Xft.Dpi) was set on this screen, and if so, use that.
    // (Note: check if 'screen' is still valid, as xrdbStrings might be empty
    // if there was an error).
    std::string dpiStr;
    if (screen < mImpl->xrdbScreenStrings.size()) {
        dpiStr = findXftDPI(mImpl->xrdbScreenStrings[screen]);
    }
    // If there is not anything for this screen, check the global strings.
    if (dpiStr.empty()) {
        dpiStr = findXftDPI(mImpl->xrdbStrings);
    }

    // If we found Xft.dpi set on the screen or globally, return that value.
    if (!dpiStr.empty()) {
        return std::atof(dpiStr.c_str());
    }

    // Otherwise return what X reports.
    int heightPx = DisplayHeight(mImpl->display, screen);
    int heightMM = DisplayHeightMM(mImpl->display, screen);
    return float(heightPx) / (float(heightMM) / 25.4f);
}

} // namespace uitk
