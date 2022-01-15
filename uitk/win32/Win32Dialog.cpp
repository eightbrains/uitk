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

#include "Win32Dialog.h"

#include "Win32Window.h"
#include "Win32Utils.h"

#include <windows.h>
#include <commctrl.h>

namespace uitk {

std::vector<wchar_t> makeFilterString(const std::vector<Win32Dialog::FileType>& extensions)
{
    std::vector<wchar_t> utf16filter;

    std::vector<std::wstring> filterPieces;
    assert(!extensions.empty());
    for (auto& e : extensions) {
        filterPieces.push_back(win32UnicodeFromUTF8(e.description));
        if (e.exts.empty() || (e.exts.size() == 1 && e.exts[0].empty())) {
            filterPieces.push_back(win32UnicodeFromUTF8("*.*"));
        }
        else {
            std::string exts;
            for (auto& ee : e.exts) {
                if (!exts.empty()) {
                    exts += ";";
                }
                exts += "*.";
                exts += ee;
            }
            filterPieces.push_back(win32UnicodeFromUTF8(exts.c_str()));
        }
    }
    for (auto& piece : filterPieces) {
        utf16filter.insert(utf16filter.end(), piece.begin(), piece.end());  // includes the null terminator
    }
    utf16filter.push_back(wchar_t(0));  // indicate the end

    return utf16filter;
}

void Win32Dialog::showAlert(Window *w,
                            const std::string& title,
                            const std::string& message,
                            const std::string& info,
                            const std::vector<std::string>& buttons,
                            std::function<void(Dialog::Result, int)> onDone)
{
    if (w) {
        auto* win32win = dynamic_cast<Win32Window*>(w->nativeWindow());
        assert(win32win);
    }

    HWND hwnd = (w ? (HWND)w->nativeHandle() : (HWND)NULL);
    auto utf16title = win32UnicodeFromUTF8(title);
    auto utf16message = win32UnicodeFromUTF8(message);
    auto utf16info = win32UnicodeFromUTF8(info);
    std::vector<std::wstring> utf16ButtonNames;
    std::vector<TASKDIALOG_BUTTON> dlgButtons;
    // Button IDs are 1000+idx because IDCANCEL is 2
    const int kBaseId = 1000;
    for (int i = 0;  i < int(buttons.size());  ++i) {
        utf16ButtonNames.push_back(win32UnicodeFromUTF8(buttons[i]));
    }
    // Do this after all the button names have been generated, so that
    // c_str() is valid. (The array might have to reallocate on the append,
    // which would invalidate the pointer from c_str().
    for (int i = int(buttons.size()) - 1;  i >= 0;  --i) {
        dlgButtons.push_back({ kBaseId + i, utf16ButtonNames[i].c_str() });
    }

    TASKDIALOGCONFIG tdc;
    ZeroMemory(&tdc, sizeof(tdc));
    tdc.cbSize = sizeof(tdc);
    tdc.hwndParent = hwnd;
    tdc.cxWidth = 0;  // auto-size
    tdc.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION |
                  TDF_POSITION_RELATIVE_TO_WINDOW |
                  TDF_SIZE_TO_CONTENT;
    if (title.empty()) {
        tdc.pszWindowTitle = NULL;  // will display exe name
    } else {
        tdc.pszWindowTitle = utf16title.c_str();
    }
    //tdc.pszMainInstruction is a hideous huge blue font; don't use
    if (!message.empty()) {
        tdc.pszContent = utf16message.c_str();
    } else if (!info.empty()) {
        tdc.pszContent = utf16info.c_str();
    }
    if (!message.empty() && !info.empty()) {
        tdc.pszExpandedInformation = utf16info.c_str();
    }
    tdc.cButtons = (UINT)dlgButtons.size();
    tdc.pButtons = dlgButtons.data();
    tdc.nDefaultButton = kBaseId;

    int buttonId;
    TaskDialogIndirect(&tdc, &buttonId, NULL, NULL);
    if (buttonId == IDCANCEL || buttonId == kBaseId + 1) {
        onDone(Dialog::Result::kCancelled, 1);
    } else {
        onDone(Dialog::Result::kFinished, buttonId - kBaseId);
    }
}

void Win32Dialog::showSave(Window *w,
                         const std::string& title,
                         const std::string& dir,
                         const std::vector<FileType>& extensions,
                         std::function<void(Dialog::Result, const std::string&)> onDone)
{
    if (w) {
        auto* win32win = dynamic_cast<Win32Window*>(w->nativeWindow());
        assert(win32win);
    }

    HWND hwnd = (w ? (HWND)w->nativeHandle() : (HWND)NULL);
    // Note that thes strings need to all live long enough that c_str() / data()
    // are valid until the dialog box is finished.
    std::wstring utf16title;
    std::wstring utf16dir;
    std::vector<wchar_t> utf16filter = makeFilterString(extensions);
    WCHAR utf16result[32768] = { 0 };

    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = utf16result;
    ofn.nMaxFile = sizeof(utf16result) / sizeof(WCHAR) - 1;
    ofn.lpstrFilter = utf16filter.data();
    ofn.nFilterIndex = 1; // 1-based index (0 indicates ofn.lpstrCustomFilter)
    if (title.empty()) {
        ofn.lpstrTitle = NULL;
    } else {
        utf16title = win32UnicodeFromUTF8(title);
        ofn.lpstrTitle = utf16title.c_str();
    }
    if (dir.empty()) {
        ofn.lpstrInitialDir = NULL;
    } else {
        utf16dir = win32UnicodeFromUTF8(dir);
        ofn.lpstrInitialDir = utf16dir.c_str();
    }
    ofn.Flags = OFN_PATHMUSTEXIST;

    if (GetSaveFileNameW(&ofn) == TRUE) {
        auto selected = utf8FromWin32Unicode(utf16result);
        onDone(Dialog::Result::kFinished, normalizeWin32Path(selected));
    } else {
        onDone(Dialog::Result::kCancelled, {});
    }
}

void Win32Dialog::showOpen(Window *w,
                         const std::string& title,
                         const std::string& dir,
                         const std::vector<FileType>& extensions,
                         bool canSelectDirectories,
                         bool canSelectMultipleFiles,
                         std::function<void(Dialog::Result, const std::vector<std::string>&)> onDone)
{
    if (w) {
        auto* win32win = dynamic_cast<Win32Window*>(w->nativeWindow());
        assert(win32win);
    }

    HWND hwnd = (w ? (HWND)w->nativeHandle() : (HWND)NULL);
    // Note that thes strings need to all live long enough that c_str() / data()
    // are valid until the dialog box is finished.
    std::wstring utf16title;
    std::wstring utf16dir;
    std::vector<wchar_t> utf16filter = makeFilterString(extensions);
    WCHAR utf16result[32768] = { 0 };
    
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = utf16result; // holds the results (including if multiple files are selected)
    ofn.nMaxFile = sizeof(utf16result) / sizeof(WCHAR) - 1;
    ofn.lpstrFilter = utf16filter.data();
    ofn.nFilterIndex = 1; // 1-based index (0 indicates ofn.lpstrCustomFilter)
    if (title.empty()) {
        ofn.lpstrTitle = NULL;
    } else {
        utf16title = win32UnicodeFromUTF8(title);
        ofn.lpstrTitle = utf16title.c_str();
    }
    if (dir.empty()) {
        ofn.lpstrInitialDir = NULL;
    } else {
        utf16dir = win32UnicodeFromUTF8(dir);
        ofn.lpstrInitialDir = utf16dir.c_str();
    }
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (canSelectMultipleFiles) {
        // Explorer mode is used when selecting one file, but if you want multiple
        // files, you have to know to set it, otherwise you get a really old dialog.
        // Really intuitive, Microsoft.
        ofn.Flags |= OFN_ALLOWMULTISELECT | OFN_EXPLORER;
    }
    if (canSelectDirectories) {
        // TODO: what to do here?
    }

    if (GetOpenFileNameW(&ofn) == TRUE) {
        std::vector<std::string> results;
        if (canSelectMultipleFiles) {
            // Results are either:
            //   1 full path
            //   dir \0 name1 \0 name2 \0 ... \0 nameN \0 \0
            std::vector<std::string> rawResults;
            auto *start = &utf16result[0];
            while (*start != '\0') {
                auto *p = start;
                while (*p != '\0') {
                    ++p;
                }
                ++p;  // increment past null terminator
                auto selected = utf8FromWin32Unicode(start);
                rawResults.push_back(normalizeWin32Path(selected));
                start = p;
            }
            if (rawResults.size() == 1) {
                results = rawResults;
            } else {
                for (size_t i = 1;  i < rawResults.size();  ++i) {
                    results.push_back(rawResults[0] + "/" + rawResults[i]);
                }
            }
        } else {
            auto selected = utf8FromWin32Unicode(utf16result);
            results.push_back(normalizeWin32Path(selected));
        }
        onDone(Dialog::Result::kFinished, results);
    } else {
        onDone(Dialog::Result::kCancelled, {});
    }
}

}  // namespace uitk
