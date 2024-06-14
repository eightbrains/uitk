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

#include "FileSystemNode.h"

#include <errno.h>
#include <sys/stat.h>
#include <stdio.h>  // for rename()

namespace uitk {

FileSystemNode::FileSystemNode()
{
}

FileSystemNode::FileSystemNode(const std::string& path)
{
    mPath = path;
#if defined(_WIN32) || defined(_WIN64)
    // std doesn't have a very useful replace function(). Fortunately, since we
    // are only replacing one character, we can do it conveniently with a loop.
    for (auto &c : mPath) {
        if (c == '\\') {
            c = '/';
        }
    }
#endif // windows
    while (!mPath.empty() && mPath[mPath.size() - 1] == '/') {
        mPath.pop_back();
    }
}

FileSystemNode::~FileSystemNode()
{
}

std::string FileSystemNode::calcWindowsPath() const
{
    auto mspath = mPath;  // copy
    for (auto& c : mspath) {
        if (c == '/') {
            c = '\\';
        }
    }
    return mspath;
}

const std::string& FileSystemNode::path() const { return mPath; }

std::string FileSystemNode::parentPath() const
{
    auto idx = mPath.rfind('/');
    if (idx != std::string::npos) {
        if (idx == 0) {
            return "/";
        } else {
            return mPath.substr(0, idx);
        }
    } else {
        return "";
    }
}

std::string FileSystemNode::name() const
{
    auto idx = mPath.rfind('/');
    if (idx != std::string::npos) {
        return mPath.substr(idx + 1);
    } else {
        return mPath;
    }
}

std::string FileSystemNode::extension() const
{
    auto slashIdx = mPath.rfind('/');
    auto idx = mPath.rfind('.');
    if (idx != std::string::npos && (idx > slashIdx || slashIdx == std::string::npos)) {
        if (idx == 0) {
            return "";  // dotfiles have no extension
        } else {
            return mPath.substr(idx + 1, mPath.size() - (idx + 1));
        }
    } else {
        return "";
    }
}

bool FileSystemNode::exists() const
{
    struct stat info;
    if (stat(mPath.c_str(), &info) != -1) {
        return true;
    } else {
        return false;
    }
}

bool FileSystemNode::isFile() const
{
    struct stat info;
    if (stat(mPath.c_str(), &info) != -1) {
        return (info.st_mode & S_IFREG);
    } else {
        return false;
    }
}

bool FileSystemNode::isDir() const
{
    struct stat info;
    if (stat(mPath.c_str(), &info) != -1) {
        return (info.st_mode & S_IFDIR);
    } else {
        return false;
    }
}

uint64_t FileSystemNode::size(IOError::Error *err) const
{
    struct stat info;
    if (stat(mPath.c_str(), &info) != -1) {
        if (err) {
            *err = IOError::kNone;
        }
        return info.st_size;
    }
    if (err) {
        *err = IOError::fromErrno(errno);
    }
    return 0;
}

IOError::Error FileSystemNode::rename(const std::string& newPath)
{
    if (::rename(mPath.c_str(), newPath.c_str()) < 0) {
        return IOError::fromErrno(errno);
    }
    mPath = newPath;
    return IOError::kNone;
}

} // namespace uitk
