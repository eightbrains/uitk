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

#include "../themes/EmpireTheme.h"

#include <unordered_map>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellscalingapi.h>  // for SetProcessDpiAwareness()
#include <wrl.h>
#include <windows.ui.viewmanagement.h>

namespace uitk {

struct Win32Application::Impl {
    std::unordered_map<HWND, Win32Window*> hwnd2window;
    bool needsToUnitializeCOM;
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
}

Win32Application::~Win32Application()
{
    if (mImpl->needsToUnitializeCOM) {
        CoUninitialize();
    }
}

void Win32Application::setExitWhenLastWindowCloses(bool exits)
{
    // Do nothing, this is pretty much always true on Linux, as there would
    // be no way to open a new window after the last one closes.
}

int Win32Application::run()
{
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // msg is WM_QUIT, and WinMain return value should be wParam
    // (unless it exits before running the message loop, then it
    // it is zero by convention).
    return int(msg.wParam);
}

bool Win32Application::shouldHideScrollbars() const { return false; }

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
    if (mImpl->hwnd2window.empty()) {
        PostQuitMessage(0);
    }
}

} // namespace uitk
