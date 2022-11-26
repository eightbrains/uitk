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
#include "Win32Utils.h"
#include "../Application.h"
#include "../Window.h"
#include "../themes/EmpireTheme.h"

#include <algorithm>
#include <list>
#include <mutex>
#include <set>
#include <unordered_map>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
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

void Win32Application::debugPrint(const std::string& s)
{
    DPrint() << s;  // includes \n
}

bool Win32Application::isOriginInUpperLeft() const { return true; }

bool Win32Application::isWindowBorderInsideWindowFrame() const { return false; }

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

    wrl::ComPtr<abi_vm::IUISettings3> settings;
    wf::ActivateInstance(wrl::Wrappers::HStringReference(
        RuntimeClass_Windows_UI_ViewManagement_UISettings).Get(), &settings);

    auto getColor = [&settings](abi_vm::UIColorType type) {
        ABI::Windows::UI::Color color;
        settings->GetColorValue(type, &color);
        return Color(color.R, color.G, color.B, color.A);
    };

    Color background = getColor(abi_vm::UIColorType::UIColorType_Background);
    //Color foreground = getColor(abi_vm::UIColorType::UIColorType_Foreground);
    Color accent = getColor(abi_vm::UIColorType::UIColorType_Accent);

    Theme::Params params;
    if (background.toGrey().red() < 0.5f) {
        params = EmpireTheme::darkModeParams(accent);
    } else {
        params = EmpireTheme::lightModeParams(accent);
    }
    params.labelFont = Font(fontFamily, fontSize);
    params.nonNativeMenubarFont = params.labelFont;

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
