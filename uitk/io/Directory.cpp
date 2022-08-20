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

#include <dirent.h>
#include <errno.h>
#include <sys/stat.h> // for mkdir()
#include <unistd.h>   // for rmdir()  (seriously?!)

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

Directory::Directory()
{
}

Directory::Directory(const std::string& path)
    : FileSystemNode(path)
{
}

IOError::Error Directory::mkdir()
{
    if (::mkdir(mPath.c_str(), 0777) < 0) {
        IOError::fromErrno(errno);
    }
    return IOError::kNone;
}

IOError::Error Directory::remove()
{
    if (::rmdir(mPath.c_str()) < 0) {
        return IOError::fromErrno(errno);
    }
    return IOError::kNone;
}

std::vector<Directory::Entry> Directory::entries(IOError::Error *err) const
{
    std::vector<Entry> entries;

    // std::filesystem is not supported everywhere. macOS 10.14 (Mojave)
    // and Ubuntu 18.04, for instance, do not support it.
    struct dirent *entry;
    DIR *d = opendir(mPath.c_str());
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
            if (!exists()) {
                *err = IOError::kPathDoesNotExist;
            } else if (!isDir()) {
                *err = IOError::kPathComponentIsNotDir;
            } else {
                *err = IOError::kNoMemory;
            }
        }
    }
    return entries;
}

} // namespace uitk
