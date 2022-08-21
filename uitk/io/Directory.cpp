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

#include "Directory.h"

//#include <dirent.h>
#include <errno.h>
#include <sys/stat.h> // for mkdir()
// for rmdir() (seriously guys, why isn't this with mkdir()?!)
#if defined(_WIN32) || defined(_WIN64)
#include <direct.h>
#else
#include <unistd.h>
#endif

#if __unix__
#include <sys/types.h>
#include <dirent.h>
#endif  // __unix__

// Some macOS around Mojave (10.14) do not support std::filesystem, and
// Ubuntu 18.04 has GCC 7.5, which also does not support std::filesystem yet.
// (It may be supported with #include <experimental/filesystem> and linking
// with -lstdc++fs, but all that is too much hassle)
// Plus, it may not be very performant (see stdFilesystemEntries())
#if defined(_WIN32) || defined(_WIN64)
#define USE_STD_FILESYSTEM 1
#else
#define USE_STD_FILESYSTEM 0
#endif

#if USE_STD_FILESYSTEM
#include <filesystem>
#endif // USE_STD_FILESYSTEM

namespace uitk {

std::string Directory::Entry::extension() const
{
    auto idx = this->name.rfind('.');
    if (idx != std::string::npos) {
        if (idx == 0) {
            return "";  // dotfiles have no extension
        } else {
            return this->name.substr(idx + 1, this->name.size() - (idx + 1));
        }
    } else {
        return "";
    }
}

// I believe that std::filesystem::directory_entry::is_directory(), etc.
// make a system call, so we need an extra stat() call. Plus we have to
// extract the path. All this could make this function really slow, so I'm
// not sure using this is a good idea, particularly on Unix.
#if USE_STD_FILESYSTEM
std::vector<Directory::Entry> stdFilesystemGetEntries(const Directory& dir, IOError::Error *err)
{
    std::vector<Directory::Entry> entries;

    // Note that we want to use the error code versions to avoid exceptions 
    std::error_code ec, ec2;
    for (auto const& entry : std::filesystem::directory_iterator(dir.path(), ec)) {
        auto name = entry.path().filename().u8string();  // always UTF-8
        if (name != "." && name != "..") {
            auto stat = entry.status(ec2); // should not fail...
            auto fileType = stat.type();
            entries.push_back({ name,
                                fileType == std::filesystem::file_type::directory,
                                fileType == std::filesystem::file_type::regular,
                                fileType == std::filesystem::file_type::symlink
                              });
        }
    }
    if (ec.value() != 0) {
        // This is null if the path cannot be accessed, or if we are out of memory.
        // Do some checks to give a better error.
        if (!dir.exists()) {
            *err = IOError::kPathDoesNotExist;
        } else if (!dir.isDir()) {
            *err = IOError::kPathComponentIsNotDir;
        } else {
            *err = IOError::kNoMemory;
        }
    }
    return entries;
}
#endif // USE_STD_FILESYSTEM

#if defined(_WIN32) || defined(_WIN64)
std::vector<Directory::Entry> windowsGetEntries(const Directory& dir, IOError::Error *err)
{
    std::vector<Directory::Entry> entries;
    return entries;
}
#endif // isWindows

#if !(defined(_WIN32) || defined(_WIN64))
std::vector<Directory::Entry> posixGetEntries(const Directory& dir, IOError::Error *err)
{
    std::vector<Directory::Entry> entries;

    struct dirent *entry;
    DIR *d = opendir(dir.path().c_str());
    if (d) {
        do {
            entry = readdir(d);
            if (entry) {
                std::string entryName(entry->d_name);
                if (entryName != "." && entryName != "..") {
                    entries.push_back({ entryName,
                                        (entry->d_type & DT_DIR) != 0,
                                        (entry->d_type & DT_REG) != 0,
                                        (entry->d_type & DT_LNK) != 0,
                                      });
                }
            }
        } while (entry);

        closedir(d);
        if (err) {
            *err = IOError::kNone;
        }
    } else {
        if (err) {
            // This is null if the path cannot be accessed, or if we are out of memory.
            // Do some checks to give a better error.
            if (!dir.exists()) {
                *err = IOError::kPathDoesNotExist;
            } else if (!dir.isDir()) {
                *err = IOError::kPathComponentIsNotDir;
            } else {
                *err = IOError::kNoMemory;
            }
        }
    }
    return entries;
}
#endif  // !isWindows


//-----------------------------------------------------------------------------
Directory::Directory()
{
}

Directory::Directory(const std::string& path)
    : FileSystemNode(path)
{
}

IOError::Error Directory::mkdir()
{
#if defined(_WIN32) || defined(_WIN64)
    if (::_mkdir(mPath.c_str()) < 0) {
#else
    if (::mkdir(mPath.c_str(), 0777) < 0) {
#endif
        IOError::fromErrno(errno);
    }
    return IOError::kNone;
}

IOError::Error Directory::remove()
{
#if defined(_WIN32) || defined(_WIN64)
    if (::_rmdir(mPath.c_str()) < 0) {
#else
    if (::rmdir(mPath.c_str()) < 0) {
#endif
        return IOError::fromErrno(errno);
    }
    return IOError::kNone;
}

std::vector<Directory::Entry> Directory::entries(IOError::Error *err) const
{
#if USE_STD_FILESYSTEM
    return stdFilesystemGetEntries(*this, err);
#else
    return posixGetEntries(*this, err);
#endif

}

} // namespace uitk
