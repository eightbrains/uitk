//-----------------------------------------------------------------------------
// Copyright 2021-2022 Eight Brains Studios, LLC
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

#include "FileDialog.h"

#include "Application.h"
#include "Button.h"
#include "Checkbox.h"
#include "ComboBox.h"
#include "Label.h"
#include "ListView.h"
#include "StringEdit.h"
#include "UIContext.h"
#include "private/Utils.h"

#if defined(__APPLE__)
#include <dirent.h>  // for readdir, etc.
#include "macos/MacOSDialog.h"
#elif defined(_WIN32) || defined(_WIN64)
#include "win32/Win32Dialog.h"
#else
#endif

#if defined(_WIN32) || defined(_WIN64)
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#endif

#include <filesystem>
#include <set>

// Using TaskDialogIndirect requires included comctl32.lib, but that leads to
// "The ordinal 345 is missing from ... .exe".
// This is from https://stackoverflow.com/a/43215416
#if defined _M_IX86
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

namespace uitk {

namespace {

std::vector<std::string> pathToComponents(const std::string& path)
{
    assert(!path.empty());

    std::vector<std::string> components;
    size_t start = 0;
    size_t i = 0;
    while (i < path.size()) {
        if (path[i] == '/') {
            components.push_back(path.substr(start, i - start));
            start = i + 1;
        }
        i++;
    }
    if (i > start) {
        components.push_back(path.substr(start, i - start));
    }
#if defined(_WIN32) || defined(_WIN64)
    if (!components.empty() && components[0].size() == 2 && components[0][1] == ':') {
        components[0] += "/";
    }
#else
    if (components.empty() || components[0].empty()) {
        components[0] = "/";
    }
#endif // windows
    return components;
}

}  // namespace

struct FileDialog::Impl
{
    struct FileType {
        std::vector<std::string> extensions;
        std::string description;
    };

    // Makes these static (aka global) so that changes persist between
    // dialogs. It is really annoying to have to change directories from
    // Documents or My Documents every. single. time.
    static std::string dirPath;
    static bool showDotFiles;

    Type type;
    std::vector<FileType> allowedTypes;
    std::vector<std::set<std::string>> allowedExts;
    bool canSelectDirectory;
    bool canSelectMultipleFiles;

    std::vector<std::string> results;

    struct {
        ComboBox *pathComponents;
        Checkbox *showHidden;
        ListView *files;
        ComboBox *fileTypes;
        Label *filenameLabel = nullptr;
        StringEdit *filename = nullptr;
        Button *ok;
        Button *cancel;
    } panel;

    struct DirEntry {
        std::string name;
        bool isDir;
    };

    struct {
        std::vector<DirEntry> entries;
    } model;

    void updateDirectoryListing(const std::string& path)
    {
        std::vector<std::string> dirs;
        std::vector<std::string> files;

#if __APPLE__
        // macOS 10.14 doesn't support std::filesystem; remove this path when
        // Mojave (10.14) drops below 2% market share (it is the last 32-bit OS,
        // so some people may stay a while).
        struct dirent *entry;
        DIR *d = opendir(path.c_str());
        if (d) {
            do {
                entry = readdir(d);
                if (entry) {
                    std::string entryName(entry->d_name);
                    bool isHidden = (entry->d_name[0] == '.' && entryName != "..");  // .. is parent dir, so not hidden
                    if (!isHidden || FileDialog::Impl::showDotFiles) {
                        if (entry->d_type & DT_DIR) {
                            std::string dirName(entryName);
                            if (entryName != ".") {
                                dirs.push_back(entryName);
                            }
                        } else if (entry->d_type & (DT_REG | DT_LNK)) {
                            if (isValidExt(entryName)) {
                                files.push_back(entryName);
                            }
                        }
                    }
                }
            } while (entry);
            closedir(d);
        }
#else
        for (auto const& entry : std::filesystem::directory_iterator(path)) {
            auto name = entry.path().filename().u8string();  // always UTF-8
            // (Unix file names cannot be empty; presumably Win32 is the same, so name[0] is safe)
            bool isHidden = (name[0] == '.' && name != "..");  // .. is parent dir, so not hidden
            if (!isHidden || FileDialog::Impl::showDotFiles) {
                if (entry.is_directory()) {
                    std::string dirName(name);
                    if (name != ".") {
                        dirs.push_back(name);
                    }
                }
                else if (entry.is_regular_file() || entry.is_symlink()) {
                    if (isValidExt(name)) {
                        files.push_back(name);
                    }
                }
            }
        }
#endif

        std::sort(dirs.begin(), dirs.end());
        std::sort(files.begin(), files.end());

        model.entries.clear();
        model.entries.reserve(dirs.size() + files.size());
        for (auto &d : dirs) {
            model.entries.push_back({d, true});
        }
        for (auto &f : files) {
            model.entries.push_back({f, false});
        }

        this->panel.files->clearCells();
        for (auto &e : model.entries) {
            if (e.isDir) {
                this->panel.files->addStringCell(e.name + "/");
            } else {
                this->panel.files->addStringCell(e.name);
            }
        }

        this->panel.ok->setEnabled(false);
        updateFilenameFromSelection();
    }

    void updatePathComponents(const std::string& path)
    {
        this->panel.pathComponents->clear();
        auto pathComponents = pathToComponents(path);
        if (!pathComponents.empty()) {
            for (auto &dir : pathComponents) {
                this->panel.pathComponents->addItem(dir);
            }
            this->panel.pathComponents->setSelectedIndex(int(pathComponents.size() - 1));
        }
    }

    void updateFilenameFromSelection()
    {
        if (this->panel.filename) {
            auto selectedIdx = this->panel.files->selectedIndex();
            if (selectedIdx >= 0) {
                this->panel.filename->setText(this->model.entries[selectedIdx].name);
            } else {
                this->panel.filename->setText("");

            }
        }
    }

    void goIntoSubdir(const std::string& dirName)
    {
        if (dirName == "..") {
            auto dirs = pathToComponents(this->selectedDir());
            std::string path = dirs[0];
            for (size_t i = 1;  i < dirs.size() - 1;  ++i) {
                if (i > 1) {
                    path += "/";
                }
                path += dirs[i];
            }
            updateDirectoryListing(path);
            updatePathComponents(path);
        } else {
            auto newPath = this->selectedDir() + "/" + dirName;
            updateDirectoryListing(newPath);
            updatePathComponents(newPath);
        }
    }

    std::string selectedDir()
    {
        std::string path;
        int sel = this->panel.pathComponents->selectedIndex();
        for (int i = 0;  i <= sel;  ++i) {
            if (i > 1) {  // i[0] ends in '/' (i.e. "/" or "c:/")
                path += "/";
            }
            path += this->panel.pathComponents->textAtIndex(i);
        }
        return path;
    }

    void onShowHiddenToggled()
    {
        FileDialog::Impl::showDotFiles = this->panel.showHidden->isOn();
        updateDirectoryListing(selectedDir());
    }

    bool isValidExt(const std::string& path)
    {
        auto allowedIdx = this->panel.fileTypes->selectedIndex();
        assert(allowedIdx >= 0);
        if (allowedIdx >= 0) {
            auto &allowed = this->allowedExts[allowedIdx];
            if (allowed.empty()) {  // empty set is all extensions
                return true;
            }
            auto i = path.rfind('.');
            if (i == std::string::npos) {  // no '.' means extension is empty, so not allowed
                return false;
            }
            auto ext = path.substr(i + 1);
            if (allowed.size() == 1) {
                return (*allowed.begin() == ext);
            } else {
                return (allowed.find(ext) != allowed.end());
            }
        }
        return false;
    }

    // showNativeDialog() needs some setup; it must be called from
    // FileDialog::showModal().
#if defined(__APPLE__)
    bool showNativeDialog(Window *w, std::function<void(Dialog::Result, int)> onDone)
    {
        std::vector<MacOSDialog::FileType> ftypes;
        for (size_t i = 0;  i < this->allowedTypes.size();  ++i) {
            auto &desc = this->allowedTypes[i];
            for (auto &e : desc.extensions) {
                ftypes.push_back({ e, desc.description });
            }
        }

        if (this->type == kSave) {
            MacOSDialog::showSave(w, "", Impl::dirPath, ftypes,
                                  [this, onDone](Dialog::Result r, const std::string& path) {
                if (r != Dialog::Result::kCancelled) {
                    Impl::dirPath = baseDirectoryOfPath(path);
                }
                this->results.push_back(path);
                onDone(r, int(this->results.size()));
            });
        } else {
            MacOSDialog::showOpen(w, "", Impl::dirPath, ftypes,
                                  this->canSelectDirectory, this->canSelectMultipleFiles,
                                  [this, onDone](Dialog::Result r, const std::vector<std::string>& paths) {
                if (r != Dialog::Result::kCancelled && !paths.empty()) {
                    Impl::dirPath = baseDirectoryOfPath(paths[0]);
                }
                this->results = paths;
                onDone(r, int(this->results.size()));
            });
        }
        return true;
    }
#elif defined(_WIN32) || defined(_WIN64)
    bool showNativeDialog(Window *w, std::function<void(Dialog::Result, int)> onDone)
    {
        std::vector<Win32Dialog::FileType> ftypes;

        if (this->type == kSave) {
            for (size_t i = 0; i < this->allowedTypes.size(); ++i) {
                auto& desc = this->allowedTypes[i];
                for (auto& e : desc.extensions) {
                    ftypes.push_back({ { e }, desc.description });
                }
            }
            Win32Dialog::showSave(w, "", Impl::dirPath, ftypes,
                                  [this, onDone](Dialog::Result r, const std::string& path) {
                                      if (r != Dialog::Result::kCancelled) {
                                          Impl::dirPath = baseDirectoryOfPath(path);
                                      }
                                      this->results.push_back(path);
                                      onDone(r, int(this->results.size()));
            });
        } else {
            // Win32 does not support selecting directories, so fallback to
            // non-native dialog in this case.
            if (this->canSelectDirectory) {
                return false;
            }

            for (size_t i = 0; i < this->allowedTypes.size(); ++i) {
                auto& desc = this->allowedTypes[i];
                ftypes.push_back({ desc.extensions, desc.description });
            }
            Win32Dialog::showOpen(w, "", Impl::dirPath, ftypes,
                                  this->canSelectDirectory, this->canSelectMultipleFiles,
                                  [this, onDone](Dialog::Result r, const std::vector<std::string>& paths) {
                                      if (r != Dialog::Result::kCancelled && !paths.empty()) {
                                          Impl::dirPath = baseDirectoryOfPath(paths[0]);
                                      }
                                      this->results = paths;
                                      onDone(r, int(this->results.size()));
                                  });
        }
        return true;
    }
#else
    bool showNativeDialog(Window *w, std::function<void(Dialog::Result, int)> onDone)
    {
        assert(false);
        return false;
    }
#endif
};
std::string FileDialog::Impl::dirPath;
bool FileDialog::Impl::showDotFiles = false;

FileDialog::FileDialog(Type type)
    : mImpl(new Impl())
{
    assert(pathToComponents("/").size() == 1);
    assert(pathToComponents("/home").size() == 2);
    assert(pathToComponents("/home/").size() == 2);

    mImpl->type = type;
    mImpl->canSelectDirectory = false;
    mImpl->canSelectMultipleFiles = false;

    mImpl->panel.pathComponents = new ComboBox();
    mImpl->panel.pathComponents->setOnSelectionChanged([this](ComboBox*){
        mImpl->updateDirectoryListing(mImpl->selectedDir());
    });
    addChild(mImpl->panel.pathComponents);
    mImpl->panel.files = new ListView();
    addChild(mImpl->panel.files);
    mImpl->panel.fileTypes = new ComboBox();
    addChild(mImpl->panel.fileTypes);
    mImpl->panel.showHidden = new Checkbox("Show hidden files");
    mImpl->panel.showHidden->setOnClicked([this](Button*){ mImpl->onShowHiddenToggled(); });
    addChild(mImpl->panel.showHidden);
    if (type == Type::kSave) {
        mImpl->panel.filenameLabel = new Label("File name");
        addChild(mImpl->panel.filenameLabel);
        mImpl->panel.filename = new StringEdit();
        addChild(mImpl->panel.filename);
        mImpl->panel.filename->setOnTextChanged([this](const std::string &){
            if (mImpl->panel.files->selectedIndex() >= 0) {
                mImpl->panel.files->setSelectedIndex(-1);
            }
        });
    }
    mImpl->panel.cancel = new Button("Cancel");
    mImpl->panel.cancel->setOnClicked([this](Button*) { this->cancel(); });
    addChild(mImpl->panel.cancel);
    mImpl->panel.ok = new Button(type == kSave ? "Save" : "Open");
    mImpl->panel.ok->setEnabled(false);  // nothing selected at first, so ok is disabled
    mImpl->panel.ok->setOnClicked([this](Button*) {
        auto selectedIdx = mImpl->panel.files->selectedIndex();
        if (mImpl->model.entries[selectedIdx].isDir && !mImpl->canSelectDirectory) {
            mImpl->goIntoSubdir(mImpl->model.entries[selectedIdx].name);
        } else {
            this->finish(1);
        }
    });
    addChild(mImpl->panel.ok);
    setAsDefaultButton(mImpl->panel.ok);

    mImpl->panel.fileTypes->setOnSelectionChanged([this](ComboBox*) {
        mImpl->updateDirectoryListing(mImpl->selectedDir());
    });
    mImpl->panel.files->setOnSelectionChanged([this](ListView*) {
        auto selectedIdx = mImpl->panel.files->selectedIndex();
        if (selectedIdx >= 0 && (!mImpl->model.entries[selectedIdx].isDir || mImpl->canSelectDirectory)) {
            mImpl->updateFilenameFromSelection();
        }
        mImpl->panel.ok->setEnabled(selectedIdx >= 0);
    });
    mImpl->panel.files->setOnSelectionDoubleClicked([this](ListView*, int idx) {
        if (mImpl->model.entries[idx].isDir) {
            auto dirName = mImpl->model.entries[idx].name;
            mImpl->goIntoSubdir(dirName);
        } else {
            mImpl->panel.ok->performClick();
        }
    });
}

FileDialog::~FileDialog()
{
}

std::string FileDialog::selectedPath() const
{
    if (mImpl->results.empty()) {
        return "";
    }
    return mImpl->results[0];
}

std::vector<std::string> FileDialog::selectedPaths() const
{
    return mImpl->results;
}

const std::string& FileDialog::directory() const
{
    return FileDialog::Impl::dirPath;
}

void FileDialog::setDirectory(const std::string& dir)
{
    FileDialog::Impl::dirPath = dir;
    for (char &c : FileDialog::Impl::dirPath) {
        if (c == '\\') {
            c = '/';
        }
    }
}

void FileDialog::clearAllowedTypes()
{
    mImpl->allowedTypes.clear();
    mImpl->allowedExts.clear();
}

void FileDialog::addAllowedType(const std::string& extension, const std::string& description)
{
    // std::string has a constructor that takes a pair of iterators, so we need to
    // specify that we want to make a vector.
    addAllowedType(std::vector<std::string>({extension}), description);
}

void FileDialog::addAllowedType(const std::vector<std::string>& extensions,
                                const std::string& description)
{
    mImpl->allowedTypes.push_back({extensions, description});
    mImpl->allowedExts.emplace_back();
    // if extensions == {}, or extensions == {""}, want empty set
    if (!extensions.empty() && !(extensions.size() == 1 && extensions[0] == "")) {
        for (auto &ext : extensions) {
            mImpl->allowedExts.back().insert(ext);
        }
    }
    assert(mImpl->allowedTypes.size() == mImpl->allowedExts.size());
}

bool FileDialog::canSelectDirectory() const
{
    return mImpl->canSelectDirectory;
}

void FileDialog::setCanSelectDirectory(bool can)
{
    mImpl->canSelectDirectory = true;
}

bool FileDialog::canSelectMultipleFiles() const
{
    return mImpl->canSelectMultipleFiles;
}

void FileDialog::setCanSelectMultipleFiles(bool can)
{
    mImpl->canSelectMultipleFiles = can;
}

void FileDialog::showModal(Window *w, std::function<void(Result, int)> onDone)
{
    mImpl->results.clear();

    // Set defaults
    if (FileDialog::Impl::dirPath == "") {
        char *path = getcwd(NULL, 0);
        if (path) {
            setDirectory(path);
        }
        free(path);
    }
    if (mImpl->allowedTypes.empty()) {
        addAllowedType("", "All types");
    }

    bool showNonNative = true;
    if (Application::instance().supportsNativeDialogs()) {
        showNonNative = !mImpl->showNativeDialog(w, onDone);
    }
    if (showNonNative) {
        // Configure path component selector
        mImpl->updatePathComponents(FileDialog::Impl::dirPath);

        // Configure file types combobox
        mImpl->panel.fileTypes->clear();
        for (auto &t : mImpl->allowedTypes) {
            if (!t.extensions.empty() && !(t.extensions.size() == 1 && t.extensions[0] == "")) {
                std::string exts;
                for (int i = 0;  i < t.extensions.size();  ++i) {
                    if (i > 0) {
                        exts += ", ";
                    }
                    exts += "*.";
                    exts += t.extensions[i];
                }
                mImpl->panel.fileTypes->addItem(t.description + " (" + exts + ")");
            } else {
#if defined(_WIN32) || defined(_WIN64)
                mImpl->panel.fileTypes->addItem(t.description + " (*.*)");
#else
                mImpl->panel.fileTypes->addItem(t.description);
#endif // if windows
            }
        }

        // Configure list view
        if (mImpl->canSelectMultipleFiles) {
            mImpl->panel.files->setSelectionModel(ListView::SelectionMode::kMultipleItems);
        } else {
            mImpl->panel.files->setSelectionModel(ListView::SelectionMode::kSingleItem);
        }

        mImpl->updateDirectoryListing(FileDialog::Impl::dirPath);

        Super::showModal(w, [this, onDone](Dialog::Result r, int i) {
            if (r != Dialog::Result::kCancelled) {
                auto dir = mImpl->selectedDir();
                FileDialog::Impl::dirPath = dir;
                if (mImpl->canSelectMultipleFiles) {
                    assert(mImpl->type == kOpen);
                    for (auto i : mImpl->panel.files->selectedIndices()) {
                        if (!mImpl->model.entries[i].isDir || mImpl->canSelectDirectory) {
                            mImpl->results.push_back(dir + "/" + mImpl->model.entries[i].name);
                        }
                    }
                } else {
                    std::string filename;
                    if (mImpl->panel.files->selectedIndex() >= 0) {
                        filename = mImpl->model.entries[mImpl->panel.files->selectedIndex()].name;
                    } else if (mImpl->panel.filename) {
                        filename = mImpl->panel.filename->text();
                    }
                    mImpl->results.push_back(dir + "/" + filename);
                }
            }
            onDone(r, i);
        });
    }
}

Size FileDialog::preferredSize(const LayoutContext& context) const
{
    auto em = context.theme.params().labelFont.pointSize();
    return Size(40 * em, 40 * em);
}

void FileDialog::layout(const LayoutContext& context)
{
    auto em = context.theme.params().labelFont.pointSize();
    auto margin = 2.0f * em;
    Rect contentRect(margin, margin, bounds().width - 2.0f * margin, bounds().height - 2.0f * margin);

    Size pref;
    PicaPt y = contentRect.y;
    if (mImpl->panel.filename) {
        pref = mImpl->panel.filename->preferredSize(context);
        auto labelPref = mImpl->panel.filenameLabel->preferredSize(context);
        auto editWidth = 20.0f * em;
        auto w = labelPref.width + 0.5f * em + editWidth;
        mImpl->panel.filenameLabel->setAlignment(Alignment::kRight | Alignment::kVCenter);
        mImpl->panel.filenameLabel->setFrame(Rect(contentRect.midX() - 0.5f * w,
                                                  contentRect.y + 0.5f * (labelPref.height - pref.height),
                                                  labelPref.width, pref.height));
        pref = mImpl->panel.filename->preferredSize(context);
        mImpl->panel.filename->setFrame(Rect(mImpl->panel.filenameLabel->frame().maxX() + 0.5f * em,
                                             contentRect.y, editWidth, pref.height));
        y = mImpl->panel.filename->frame().maxY() + em;
    }

    pref = mImpl->panel.pathComponents->preferredSize(context);
    auto w = std::min(contentRect.width, std::max(5.0f * em, pref.width));
    mImpl->panel.pathComponents->setFrame(Rect(contentRect.midX() - 0.5f * w, y, w, pref.height));

    pref = mImpl->panel.ok->preferredSize(context);
    // Keep open/save and cancel buttons the same width so they look nice
    w = std::max(pref.width, mImpl->panel.cancel->preferredSize(context).width);
    mImpl->panel.ok->setFrame(Rect(contentRect.maxX() - w, contentRect.maxY() - pref.height,
                                   w, pref.height));
    mImpl->panel.cancel->setFrame(Rect(mImpl->panel.ok->frame().x - 2.0f * em - pref.width,
                                       mImpl->panel.ok->frame().y, w, pref.height));
    pref = mImpl->panel.fileTypes->preferredSize(context);
    w = std::min(contentRect.width, std::max(5.0f * em, pref.width));
    mImpl->panel.fileTypes->setFrame(Rect(contentRect.midX() - 0.5f * w,
                                          mImpl->panel.ok->frame().y - 2.0f * em - pref.height,
                                          w, pref.height));
    pref = mImpl->panel.showHidden->preferredSize(context);
    mImpl->panel.showHidden->setFrame(Rect(contentRect.x,
                                           mImpl->panel.fileTypes->frame().y - em - pref.height,
                                           pref.width, pref.height));

    y = mImpl->panel.pathComponents->frame().maxY() + em;
    mImpl->panel.files->setFrame(Rect(contentRect.x, y,
                                      contentRect.width, mImpl->panel.showHidden->frame().y - em - y));

    Super::layout(context);
}

}  // namespace uitk
