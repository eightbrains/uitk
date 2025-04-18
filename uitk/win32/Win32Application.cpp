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

#include "Win32Application.h"

#include "Win32Clipboard.h"
#include "Win32Sound.h"
#include "Win32Utils.h"
#include "../Application.h"
#include "../Printing.h"
#include "../UIContext.h"
#include "../Window.h"
#include "../themes/EmpireTheme.h"

#include <algorithm>
#include <list>
#include <mutex>
#include <set>
#include <sstream>
#include <unordered_map>

#include <windows.h>  // cannot use WIN32_LEAN_AND_MEAN because it excludes some printing functions
#include <commdlg.h>
#include <prntvpt.h>  // from PT* functions (printing)
#include <psapi.h>  // for GetProcessImageFileName()
#include <shellscalingapi.h>  // for SetProcessDpiAwareness()
#include <wrl.h>
#include <windows.ui.viewmanagement.h>

namespace uitk {

namespace {
static const int kCheckPostedFunctionsMsg = WM_USER + 1534;

static OSApplication::SchedulingId gNextTimerId = OSApplication::kInvalidSchedulingId;
} // namespace;

struct Win32Application::Impl {
    std::unique_ptr<Win32Clipboard> clipboard;
    std::unique_ptr<Win32Sound> sound;
    std::unordered_map<HWND, Win32Window*> hwnd2window;
    bool needsToUnitializeCOM;

    std::mutex postedFunctionsLock;
    // This is a linked list because adding and removing does not invalidate
    // iterators.
    std::list<std::function<void()>> postedFunctions;
    struct DelayedFunc {
        std::function<void(SchedulingId)> callback;
        SchedulingId id;
        HWND hwnd;  // so we can remove if window is destroyed without stopping timer
        bool repeats;
    };
    // Index on UINT_PTR so that repeating functions do not need to linear search,
    // only canceling does (and realistically there's probably only going to be one,
    // maybe two or three at a time).
    // NOTE: unordered_map does not invalidate iterators when erasing, if you need to change
    //       the data structure make sure to update clearPostedFunctionsForHwnd()
    std::unordered_map<UINT_PTR, DelayedFunc> postedLaterFunctions;

    static void TimerCallback(HWND hwnd, UINT arg2, UINT_PTR timerId, DWORD arg4)
    {
        auto &self = static_cast<Win32Application&>(Application::instance().osApplication()).mImpl;  // is ref to unique_ptr
        std::function<void(SchedulingId)> f;
        SchedulingId id = OSApplication::kInvalidSchedulingId;
        bool removeNow = false;

        {
            std::lock_guard<std::mutex> locker(self->postedFunctionsLock);
            auto it = self->postedLaterFunctions.find(timerId);
            if (it != self->postedLaterFunctions.end()) {
                f = it->second.callback;
                id = it->second.id;
                removeNow = !it->second.repeats;
            } else {
                // We could not find a callback, no point wasting time not finding it again in the future
                KillTimer(hwnd, timerId);
            }
        }

        // We must be unlocked during the function call, in case call schedules or cancels a callback
        f(id);

        if (removeNow) {
            std::lock_guard<std::mutex> locker(self->postedFunctionsLock);
            auto it = self->postedLaterFunctions.find(timerId);
            self->removePostedLater_locked(it);
        }
    }

    std::unordered_map<UINT_PTR, DelayedFunc>::iterator removePostedLater_locked(std::unordered_map<UINT_PTR, DelayedFunc>::iterator& plfIt)
    {
        // We should be locked here (since caller is holding an iterator)

        // (We take an iterator so that a single-shot callback does not need to linear search on the SchedulingId to cancel)
        KillTimer(NULL, plfIt->first);
        return this->postedLaterFunctions.erase(plfIt);  // returns the next iterator after the one erased
    }

    SchedulingId addPostedLater_unlocked(Window *w, float delay, bool repeat, std::function<void(SchedulingId)> f)
    {
        // The MS docs say that the auto-exception eating blocks should be disabled when using
        // timers. This is done in the constructor.

        auto id = ++gNextTimerId;
        UINT delayMilliSec = UINT(std::round(delay * 1000.0f));

        auto win32id = SetTimer(NULL, 0, delayMilliSec, Impl::TimerCallback);
        assert(win32id != 0);  // SetTimer should not fail

        {
            std::lock_guard<std::mutex> locker(this->postedFunctionsLock);
            this->postedLaterFunctions[win32id] = { f, id, (HWND)w->nativeHandle(), repeat };
        }

        return id;
    }

    void clearPostedFunctionsForHwnd(HWND hwnd)
    {
        std::lock_guard<std::mutex> locker(this->postedFunctionsLock);
        // NOTE: std::unordered_map does not invalidate iterators on erase (except the one erased),
        //       so we can erase and continue iterating;
        auto it = this->postedLaterFunctions.begin();
        while (it != this->postedLaterFunctions.end()) {
            if (it->second.hwnd == hwnd) {
                it = removePostedLater_locked(it);
            } else {
                ++it;
            }
        }
    }
};

Win32Application::Win32Application()
    : mImpl(new Impl())
{
    // Getting color themes requires COM initialized
    HRESULT coinitResult = CoInitialize(0);
    mImpl->needsToUnitializeCOM = (coinitResult == S_OK);  // S_FALSE means already called

    // Other settings will get scaled automatically by windows, but this
    // way we can have thinner lines because we will know what our DPI
    // really is.
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    mImpl->clipboard = std::make_unique<Win32Clipboard>();
    mImpl->sound = std::make_unique<Win32Sound>();
}

Win32Application::~Win32Application()
{
    if (mImpl->needsToUnitializeCOM) {
        CoUninitialize();
    }
}

void Win32Application::setExitWhenLastWindowCloses(bool exits)
{
    // Do nothing, this is pretty much always true on Windows, as there would
    // be no way to open a new window after the last one closes.
}

Clipboard& Win32Application::clipboard() const { return *mImpl->clipboard; }

int Win32Application::run()
{
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        if (msg.message != kCheckPostedFunctionsMsg) {
            DispatchMessage(&msg);
        } else {
            // The posted function might generate another posted function
            // (for example, an animation), so we only run the functions
            // that we have right now. Also, we need to not be holding the
            // lock when we run the function, otherwise posting a function
            // will deadlock.
            size_t n = 0;
            mImpl->postedFunctionsLock.lock();
            n = mImpl->postedFunctions.size();
            mImpl->postedFunctionsLock.unlock();

            for (size_t i = 0; i < n; ++i) {
                std::function<void()> f;
                mImpl->postedFunctionsLock.lock();
                f = *mImpl->postedFunctions.begin();
                mImpl->postedFunctionsLock.unlock();

                f();

                mImpl->postedFunctionsLock.lock();
                mImpl->postedFunctions.erase(mImpl->postedFunctions.begin());
                mImpl->postedFunctionsLock.unlock();
            }
        }
    }

    // msg is WM_QUIT, and WinMain return value should be wParam
    // (unless it exits before running the message loop, then it
    // it is zero by convention).
    return int(msg.wParam);
}


void Win32Application::exitRun()
{
    // We do not need to do anything here because this should only be called from
    // Application::quit(), which will have closed all the windows, causing us to
    // exit.
}

void Win32Application::scheduleLater(Window *w, std::function<void()> f)
{
    {
    std::lock_guard<std::mutex> locker(mImpl->postedFunctionsLock);
    mImpl->postedFunctions.push_back(f);
    }

    if (w) {
        PostMessage((HWND)w->nativeHandle(), kCheckPostedFunctionsMsg, 0, 0);
    } else {
        // Docs say this acts like PostThreadMessage(), which uses GetMessage()
        // the same as our run loop, so this should be okay.
        PostMessage(NULL, kCheckPostedFunctionsMsg, 0, 0);
    }
}

Win32Application::SchedulingId Win32Application::scheduleLater(Window* w, float delay, bool repeat,
                                                               std::function<void(SchedulingId)> f)
{
    return mImpl->addPostedLater_unlocked(w, delay, repeat, f);
}

void Win32Application::cancelScheduled(SchedulingId id)
{
    std::lock_guard<std::mutex> locker(mImpl->postedFunctionsLock);
    // postedLaterFunctions is indexed on the Win32 timer id not UITK's id, so we need
    // to do a linear search.
    for (auto it = mImpl->postedLaterFunctions.begin(); it != mImpl->postedLaterFunctions.end(); ++it) {
        if (it->second.id == id) {
            mImpl->removePostedLater_locked(it);
            break;
        }
    }
}

std::string Win32Application::applicationName() const
{
    WCHAR path[MAX_PATH];
    path[0] = 0;
    GetProcessImageFileNameW(NULL, path, MAX_PATH);
    return utf8FromWin32Unicode(path);
}

std::string Win32Application::appDataPath() const
{
    WCHAR exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    auto path = utf8FromWin32Unicode(exePath);
    for (auto& c : path) {  // sadly, this is more straightforward than replace()
        if (c == '\\') {
            c = '/';
        }
    }
    auto lastSlash = path.rfind('/');
    auto firstSlash = path.find('/');
    path = path.substr(0, lastSlash);
    if (lastSlash == firstSlash) {
        path += '/';
    }
    return path;
}

std::string Win32Application::tempDir() const
{
    wchar_t path[MAX_PATH + 2];
    GetTempPathW(MAX_PATH + 1, path);
    std::string spath(utf8FromWin32Unicode(path));
    if (spath.back() == wchar_t('\\')) {
        spath.pop_back();
    }
    return spath;
}

std::vector<std::string> Win32Application::availableFontFamilies() const
{
    return Font::availableFontFamilies();
}

void Win32Application::beep()
{
    MessageBeep(MB_OK);  // default beep
}

Sound& Win32Application::sound() const
{
    return *mImpl->sound;
}

void Win32Application::printDocument(const PrintSettings& settings) const
{
    const float kPointsPerInch = 72.0f;

    auto* w = Application::instance().activeWindow();
    HWND hwnd = NULL;
    if (w) {
        hwnd = (HWND)w->nativeHandle();
    }

    PRINTDLGW printInfo;
    ZeroMemory(&printInfo, sizeof(printInfo));
    printInfo.lStructSize = sizeof(printInfo);
    printInfo.hwndOwner = NULL;  // we get PDERR_INITFAILURE if we use hwnd
    printInfo.hDC = NULL;
    printInfo.Flags = PD_USEDEVMODECOPIESANDCOLLATE | PD_NOCURRENTPAGE | PD_NOSELECTION | PD_HIDEPRINTTOFILE | PD_RETURNDC;
    printInfo.nCopies = 1;
    printInfo.nFromPage = 0xffff;
    printInfo.nToPage = 0xffff;
    printInfo.nMinPage = 1;
    printInfo.nMaxPage = 0xffff;
    printInfo.hDevMode = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(DEVMODEW));
    {
        DEVMODEW* devmode = (DEVMODEW*)GlobalLock(printInfo.hDevMode);
        devmode->dmSize = sizeof(DEVMODEW);
        devmode->dmFields = DM_ORIENTATION;
        devmode->dmOrientation = (settings.orientation == PaperOrientation::kPortrait
                                        ? DMORIENT_PORTRAIT : DMORIENT_LANDSCAPE);
        // It *seems* like Windows will detect what paper it is if you only specify the size,
        // but in case it does not work for all drivers, try to identify it first.
        if (settings.paperSize.width > PicaPt::kZero && settings.paperSize.height > PicaPt::kZero) {
            bool isKnownSize = false;
            for (auto &sz : PaperSize::knownSizes()) {
                if (std::abs((sz.width - settings.paperSize.width).asFloat()) <= 1e-4f &&
                    std::abs((sz.height - settings.paperSize.height).asFloat()) <= 1e-4f)
                {
                    isKnownSize = true;
                    devmode->dmFields |= DM_PAPERSIZE;
                    if (sz == PaperSize::kUSLetter) {
                        devmode->dmPaperSize = DMPAPER_LETTER;
                    } else if (sz == PaperSize::kUSLegal) {
                        devmode->dmPaperSize = DMPAPER_LEGAL;
                    } else if (sz == PaperSize::kUSLedger) {
                        devmode->dmPaperSize = DMPAPER_LEDGER;
                    // Windows does not have DMPAPER_A0 and DMPAPER_A1
                    } else if (sz == PaperSize::kA2) {
                        devmode->dmPaperSize = DMPAPER_A2;
                    } else if (sz == PaperSize::kA3) {
                        devmode->dmPaperSize = DMPAPER_A3;
                    } else if (sz == PaperSize::kA4) {
                        devmode->dmPaperSize = DMPAPER_A4;
                    } else if (sz == PaperSize::kA5) {
                        devmode->dmPaperSize = DMPAPER_A5;
                    } else if (sz == PaperSize::kA6) {
                        devmode->dmPaperSize = DMPAPER_A6;
                    // Windows does not DMPAPER_A7 and DMPAPER_B0 through DMPAPER_B3
                    } else if (sz == PaperSize::kB4) {
                        devmode->dmPaperSize = DMPAPER_B4;
                    } else if (sz == PaperSize::kB5) {
                        devmode->dmPaperSize = DMPAPER_B5;
                    // Windows does not DMPAPER_B6 and DMPAPER_B7
                    } else {
                        isKnownSize = false;
                        devmode->dmFields &= ~DM_PAPERSIZE;
                    }

                    if (isKnownSize) {
                        break;
                    }
                }

            }
            if (!isKnownSize) {  // Windows may coerce a custom size to a known paper size, but try anyway
                devmode->dmFields |= DM_PAPERWIDTH | DM_PAPERLENGTH;
                devmode->dmPaperWidth = settings.paperSize.width.asFloat() * 254.0f / 72.0f;  // 1/10 of mm
                devmode->dmPaperLength = settings.paperSize.height.asFloat() * 254.0f / 72.0f;
            }
        }
        GlobalUnlock(devmode);
    }

    HPTPROVIDER provider = nullptr;  // init these here, in case something fails
    LPSTREAM ptStream = nullptr;
    DEVNAMES *devnames = nullptr;
    DEVMODE *devmode = nullptr;

    if (PrintDlgW(&printInfo) == TRUE) {
        // The device caps provides good information, but hDevMode provides virtually nothing
        auto logx = float(GetDeviceCaps(printInfo.hDC, LOGPIXELSX));
        auto logy = float(GetDeviceCaps(printInfo.hDC, LOGPIXELSY));
        auto pageWidthInch = float(GetDeviceCaps(printInfo.hDC, PHYSICALWIDTH)) / logx;
        auto pageHeightInch = float(GetDeviceCaps(printInfo.hDC, PHYSICALHEIGHT)) / logy;
        auto imageableXInch = float(GetDeviceCaps(printInfo.hDC, PHYSICALOFFSETX)) / logx;
        auto imageableYInch = float(GetDeviceCaps(printInfo.hDC, PHYSICALOFFSETY)) / logy;
        auto imageableWidthInch = float(GetDeviceCaps(printInfo.hDC, HORZRES)) / logx;
        auto imageableHeightInch = float(GetDeviceCaps(printInfo.hDC, VERTRES)) / logy;
        auto dpi = min(logx, logy);

        float rotationDeg = 0.0f;
        auto translation = Point::kZero;

        // The "Microsoft Print to PDF" built-in printer seems to require scaling,
        // but there does not seem to be a way to detect this. Compared with a real
        // printer, GetDeviceCaps() and the DEVMODE structures give the same results.
        // The Direct2D initial transform and the GetWorldTransform(printInfo.hDC)
        // both have scaling of 1.0. So, since real printers cannot usually print
        // to the absolute edge of the paper, assume that any that can are printing
        // to a PDF. (This is probably wrong, but I cannot find a better way to do it.)
        bool isMSPrintToPDF = (imageableWidthInch >= pageWidthInch && imageableHeightInch >= pageHeightInch);

        XFORM transform;
        GetWorldTransform(printInfo.hDC, &transform);

        devmode = (DEVMODE*)GlobalLock(printInfo.hDevMode);
        if (!devmode) { goto err; }
        devnames = (DEVNAMES*)GlobalLock(printInfo.hDevNames);
        if (!devnames) { goto err; }

        // Handle landscape: my test printer prints blank pages in landscape, despite
        // everything being correct as far as I can tell. So change the devmode to be
        // back to portrait, and then rotate/translate so the drawing code acts like
        // it is printing landscape. However, this does not work for the Microsoft
        // Print to PDF driver.
        if (!isMSPrintToPDF && (devmode->dmDisplayFlags & DM_ORIENTATION) && devmode->dmOrientation != DMORIENT_PORTRAIT) {
            if (pageWidthInch < pageHeightInch) {
                std::swap(pageWidthInch, pageHeightInch);
                std::swap(imageableXInch, imageableYInch);
            }
            if (imageableWidthInch < imageableHeightInch) { // in case HORZRES/VERTRES act differently
                std::swap(imageableWidthInch, imageableHeightInch);
            }

            // Force portrait orientation
            devmode->dmOrientation = DMORIENT_PORTRAIT;
            if ((devmode->dmDisplayFlags & DM_PAPERWIDTH) && (devmode->dmDisplayFlags & DM_PAPERLENGTH) &&
                devmode->dmPaperWidth > devmode->dmPaperLength)
            {
                std::swap(devmode->dmPaperWidth, devmode->dmPaperLength);
            }
            rotationDeg = -90.0f;
            translation = Point(PicaPt::kZero, -PicaPt::fromPixels(pageHeightInch * dpi, dpi));
        }

        auto hr = CreateStreamOnHGlobal(nullptr, TRUE, &ptStream);
        if (hr != S_OK) { goto err; }
        auto* deviceName = (PCWSTR)devnames + devnames->wDeviceOffset;
        hr = PTOpenProvider(deviceName, 1, &provider);
        if (hr != S_OK) { goto err; }
        hr = PTConvertDevModeToPrintTicket(provider, devmode->dmSize + devmode->dmDriverExtra,
                                           devmode, kPTJobScope, ptStream);
        if (hr != S_OK) { goto err; }

        {
            int nPages = 0;
            {
            auto dc = DrawContext::createDirect2DBitmap(kBitmapGreyscale, int(std::ceil(pageWidthInch * 72.0f)),
                                                        int(std::ceil(pageHeightInch * 72.0f)), 72.0f);
            LayoutContext context = { *Application::instance().theme(), *dc };
            PaperSize paperSize(PicaPt(pageHeightInch * 72.0f), PicaPt(pageHeightInch * 72.0f), "");
            nPages = settings.calcPages(paperSize, context);
            }

            auto jobName = Application::instance().applicationName();
            auto dc = DrawContext::createPrinterContext(deviceName,
                                                        jobName.c_str(),
                                                        nullptr, // output to printer
                                                        ptStream,
                                                        int(std::ceil(pageWidthInch * dpi)),
                                                        int(std::ceil(pageHeightInch * dpi)),
                                                        dpi);
            PrintContext context = { *Application::instance().theme(),
                                     *dc,
                                     Rect(PicaPt::kZero, PicaPt::kZero, PicaPt(pageWidthInch * kPointsPerInch), PicaPt(pageHeightInch * kPointsPerInch)),
                                     true,  // window is active (in case that matters)
            };
            context.paperSize = Size(PicaPt(pageWidthInch * kPointsPerInch),
                                     PicaPt(pageHeightInch * kPointsPerInch));
            context.imageableRect = Rect(PicaPt(imageableXInch * kPointsPerInch),
                                         PicaPt(imageableYInch * kPointsPerInch),
                                         PicaPt(imageableWidthInch * kPointsPerInch),
                                         PicaPt(imageableHeightInch * kPointsPerInch));
            if (isMSPrintToPDF) {
                dc->scale(dpi / 96.0f, dpi / 96.0f);
            }
            dc->rotate(rotationDeg);
            dc->translate(translation.x, translation.y);

            int startPageIdx = 0;
            int endPageIdx = nPages - 1;
            if (printInfo.Flags & PD_PAGENUMS) {
                startPageIdx = printInfo.nFromPage - 1;
                endPageIdx = printInfo.nToPage - 1;
            } 
            for (int p = startPageIdx;  p <= endPageIdx;  ++p) {
                context.pageIndex = p;
                settings.drawPage(context);
                dc->addPage();
            }

            dc.reset();
        }

    err:
        if (provider) {
            PTCloseProvider(provider);
        }
        if (ptStream) {
            ptStream->Release();
        }
        if (devmode) {
            GlobalUnlock(printInfo.hDevMode);
        }
        if (devnames) {
            GlobalUnlock(printInfo.hDevNames);
        }
        if (printInfo.hDevMode != NULL) {
            GlobalFree(printInfo.hDevMode);
        }
        if (printInfo.hDevNames != NULL) {
            GlobalFree(printInfo.hDevNames);
        }
        DeleteDC(printInfo.hDC);
    } else {
        auto err = CommDlgExtendedError();
        if (err > 0) {  // err == 0 means dialog was cancelled
            std::string msg;
            switch (err) {
                case PDERR_NODEVICES:
                    msg = "No printer drivers were found";
                    break;
                case PDERR_NODEFAULTPRN:
                    msg = "There is no default printer";
                    break;
                default: {
                    std::stringstream s;
                    s << "Internal error: err " << std::hex << err;
                    msg = s.str();
                    break;
                }
            }
            MessageBoxA(hwnd, msg.c_str(), "Print error", MB_OK);
        }
    }
}

void Win32Application::debugPrint(const std::string& s) const
{
    DPrint() << s;  // includes \n
}

bool Win32Application::isOriginInUpperLeft() const { return true; }

bool Win32Application::isWindowBorderInsideWindowFrame() const { return false; }

bool Win32Application::windowsMightUseSameDrawContext() const { return false; }

bool Win32Application::shouldHideScrollbars() const { return false; }

bool Win32Application::canKeyFocusEverything() const { return true; }

bool Win32Application::platformHasMenubar() const { return true; }

Theme::Params Win32Application::themeParams() const
{
    auto toColor = [](DWORD c) {
        return Color(GetRValue(c), GetGValue(c), GetBValue(c), 255);
    };

    // Get the font name. The GetStockObject(SYSTEM_FONT) documentation
    // says to use SystemParamtersInfo() with SPI_GETNONCLIENTMETRICS,
    // but does not say what the equivalents are. Based on the description
    // of SYSTEM_FONT, the message font seems to be the best fit.
    NONCLIENTMETRICSA ncm;
    ncm.cbSize = sizeof(ncm);

    // Use the A version, so that we get back the ASCII name so that we can
    // give the name to Font without needing to convert from WCHAR. The system
    // font is going to have a name with ASCII characters, so this should
    // be safe.
    SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
    std::string fontFamily = ncm.lfMessageFont.lfFaceName;
    // It is not clear what DPI ncm.lfMessageFont.lfHeight is in, which
    // makes it difficult to report the size back. Empirically, 12pt looks
    // nice, and consistent with everything else. (Note that if you use
    // lfHeight, it might be negative, meaning that it is in device units.)
    PicaPt fontSize = PicaPt::fromPixels(12.0f, 96.0f);

    // It appears to be impossible to get the accent color without using
    // UISettings. (Which requires COM to be initialized.)
    // GetSysColor(COLOR_HIGHLIGHT) always returns blue (and the other colors
    // are likewise fixed), as does GetThemeSysColor().
    namespace abi_vm = ABI::Windows::UI::ViewManagement;
    namespace wrl = Microsoft::WRL;
    namespace wf = Windows::Foundation;

    // Seriously, Microsoft?! IUISettings3 doesn't inherit from IUISettings??!
    wrl::ComPtr<abi_vm::IUISettings> settings;
    wrl::ComPtr<abi_vm::IUISettings3> settings3;
    wf::ActivateInstance(wrl::Wrappers::HStringReference(
        RuntimeClass_Windows_UI_ViewManagement_UISettings).Get(), &settings);
    wf::ActivateInstance(wrl::Wrappers::HStringReference(
        RuntimeClass_Windows_UI_ViewManagement_UISettings).Get(), &settings3);

    auto getColor = [&settings3](abi_vm::UIColorType type) {
        ABI::Windows::UI::Color color;
        settings3->GetColorValue(type, &color);
        return Color(color.R, color.G, color.B, color.A);
    };

    auto getElementColor = [&settings](abi_vm::UIElementType type) {
        ABI::Windows::UI::Color color;
        settings->UIElementColor(type, &color);
        return Color(color.R, color.G, color.B, color.A);
    };

    Color background = getColor(abi_vm::UIColorType::UIColorType_Background);
    Color accent = getColor(abi_vm::UIColorType::UIColorType_Accent);

    Theme::Params params;
    if (background.toGrey().red() < 0.5f) {
        params = EmpireTheme::darkModeParams(accent);
    } else {
        params = EmpireTheme::lightModeParams(accent);
    }
    // Window's light mode is white (like our default), but the dark mode is black, which
    // is too contrasty, so just use our background color, which looks better in dark mode.
    params.labelFont = Font(fontFamily, fontSize);
    params.nonNativeMenubarFont = params.labelFont;
    // Note: all of WindowText, CaptionText, ButtonText are always black, which is not
    //       desirable in dark mode (and is the case even for the OS's dark mode!)

    HIGHCONTRASTA hc;
    hc.cbSize = sizeof(hc);
    SystemParametersInfoA(SPI_GETHIGHCONTRAST, hc.cbSize, &hc, 0);
    params.useHighContrast = (hc.dwFlags & HCF_HIGHCONTRASTON);
    if (params.useHighContrast) {
        params.windowBackgroundColor = background; // want the black black in high contrast mode
        params.nonEditableBackgroundColor = background;
        params.editableBackgroundColor = background;
        params.borderColor = params.textColor;
        // Docs say _WindowText is for the title bar, but they appear to be wrong.
        // In High Contrast mode, only _WindowText, not _ButtonText or _CaptionText
        // have the green/yellow text color. Unlike dark mode, these *are* correct
        // in high contrast mode.
        params.textColor = getElementColor(abi_vm::UIElementType_WindowText);
        params.disabledTextColor = getElementColor(abi_vm::UIElementType_GrayText);  // not gray in high contrast
    }
    params.scrollbarColor = params.textColor;

    return params;
}

void Win32Application::registerWindow(void* hwnd, Win32Window* w)
{
    mImpl->hwnd2window[(HWND)hwnd] = w;
}

void Win32Application::unregisterWindow(void* hwnd)
{
    auto wit = mImpl->hwnd2window.find((HWND)hwnd);
    if (wit != mImpl->hwnd2window.end()) {
        mImpl->hwnd2window.erase(wit);
    }
    mImpl->clearPostedFunctionsForHwnd((HWND)hwnd);
    if (mImpl->hwnd2window.empty()) {
        PostQuitMessage(0);
    }
}

} // namespace uitk
