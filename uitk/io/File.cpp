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

#include "File.h"

#include <errno.h>
#include <fcntl.h> // for open()
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h> // for close() (seriously guys?!)

namespace uitk {

template <class T>
void readFileContents(const std::string& path, T *contents, IOError::Error *err)
{
    FILE *in = fopen(path.c_str(), "rb");  // with no "b" windows converts \n -> \r\n
    if (in) {
        fseeko(in, 0, SEEK_END);
        off_t size = ftello(in);
        contents->resize(size);
        fseeko(in, 0, SEEK_SET);
        fread(contents->data(), size, 1, in);
        fclose(in);
        if (err) {
            *err = IOError::kNone;
        }
    } else {
        if (err) {
            *err = IOError::fromErrno(errno);
        }
    }
}

File::File()
{
}

File::File(const std::string& path)
    : FileSystemNode(path)
{
}
/*
const std::string& File::path() const { return mPath; }

std::string File::directoryPath() const
{
    auto idx = mPath.rfind('/');
    if (idx != std::string::npos) {
        if (idx == 0) {
            return "/";
        } else {
            return mPath.substr(0, idx - 1);
        }
    } else {
        return "";
    }
}

std::string File::extension() const
{
    auto idx = mPath.rfind('.');
    if (idx != std::string::npos) {
        if (idx == 0) {
            return "";  // dotfiles have no extension
        } else {
            return mPath.substr(idx + 1, mPath.size() - (idx + 1));
        }
    } else {
        return "";
    }
}

bool File::exists() const
{
    struct stat info;
    if (stat(mPath.c_str(), &info) != -1) {
        return true;
    } else {
        return false;
    }
}

bool File::isFile() const
{
    struct stat info;
    if (stat(mPath.c_str(), &info) != -1) {
        return (info.st_mode & S_IFREG);
    } else {
        return false;
    }
}

bool File::isDir() const
{
    struct stat info;
    if (stat(mPath.c_str(), &info) != -1) {
        return (info.st_mode & S_IFDIR);
    } else {
        return false;
    }
}

uint64_t File::size(IOError *err) const
{
    struct stat info;
    if (stat(mPath.c_str(), &info) != -1) {
        if (err) {
            *err = IOError::kNone;
        }
        return info.st_size;
    }
    if (err) {
        *err = errnoToIOError(errno);
    }
    return 0;
}
*/
std::string File::readContentsAsString(IOError::Error *err) const
{
    std::string contents;
    readFileContents(mPath, &contents, err);
    return contents;
}

std::vector<char> File::readContents(IOError::Error *err) const
{
    std::vector<char> contents;
    readFileContents(mPath, &contents, err);
    return contents;
}

IOError::Error File::writeContents(const std::string& contents)
{
    return writeContents(contents.data(), contents.size());
}

IOError::Error File::writeContents(const std::vector<char>& contents)
{
    return writeContents(contents.data(), contents.size());
}

IOError::Error File::writeContents(const char *contents, uint64_t size)
{
    FILE *out = fopen(mPath.c_str(), "wb");  // with no "b" windows converts \n -> \r\n
    if (out) {
        fwrite(contents, size, 1, out);
        fclose(out);
        return IOError::kNone;
    }
    return IOError::fromErrno(errno);
}

File::MappedAddress File::mmap(IOError::Error *err) const
{
    MappedAddress addr = { nullptr, 0 };

    IOError::Error sizeErr;
    auto fileLen = size(&sizeErr);  // err might not exist, but file might be 0 bytes
    if (sizeErr != IOError::kNone) {
        if (err) {
            *err = sizeErr;
        }
        return addr;
    }

    // Newer mmap (macOS, Linux >= 2.6.12) return EINVAL for empty files
    if (fileLen == 0) {
        if (err) {
            *err = IOError::kNone;
        }
        return addr;
    }

    auto fd = ::open(mPath.c_str(), O_RDONLY);
    if (fd == -1) {
        if (err) {
            *err = IOError::fromErrno(errno);
        }
        return addr;
    }

    void *result = ::mmap(nullptr,  // let system pick an address
                          fileLen,
                          PROT_READ,
                          MAP_FILE | MAP_SHARED,
                          fd,
                          0);       // offset from beginning of file
    ::close(fd);  // POSIX requires mapped region is still valid
    if (result != MAP_FAILED) {
        addr.addr = (char*)result;
        addr.len = fileLen;
        if (err) {
            *err = IOError::kNone;
        }
    } else {
        if (err) {
            *err = IOError::fromErrno(errno);
        }
    }

    return addr;
}

void File::munmap(const File::MappedAddress& mapping)
{
    if (!mapping.addr) {
        return;
    }
    ::munmap(mapping.addr, mapping.len);
}

// ----
File::Lines::Lines()
{
    mBegin.addr = nullptr;
    mBegin.len = 0;
    mBegin.mNextLine = nullptr;
    mBegin.mFileEnd = nullptr;
}

File::Lines::Lines(const std::string& path, const File::MappedAddress& addr)
{
    mPath = path;
    mAddr = addr;
    mBegin.addr = mAddr.addr;
    mBegin.len = 0;
    mBegin.mNextLine = mAddr.addr;
    mBegin.mFileEnd = mAddr.addr + mAddr.len;
    // If first character is a newline, then we have a true length=0 line, so
    // we should not increment to get to the first line.
    if (mBegin.addr && *mBegin.addr != '\n') {
        ++mBegin;
    }
}

File::Lines::~Lines()
{
    if (!mPath.empty() && mAddr.addr) {
        File(mPath).munmap(mAddr);
    }
}

File::Lines::Iterator File::Lines::begin() { return mBegin; }
File::Lines::Iterator File::Lines::end() {
    File::Lines::Iterator it;
    it.addr = nullptr;
    it.len = 0;
    it.mNextLine = nullptr;
    it.mFileEnd = nullptr;
    return it;
}

std::vector<std::string> File::Lines::allLines()
{
    std::vector<std::string> retVal;
    for (auto line : *this) {
        retVal.push_back(line);
    }
    return retVal;
}
// ----

std::string File::Lines::Iterator::operator*()
{
    return line();
}

File::Lines::Iterator& File::Lines::Iterator::operator++()
{
    // We want to return an empty line if the last character in the
    // file is \n. That is easiest if mNextLine is actually on the
    // \n.

    if (!this->addr || this->addr >= mFileEnd || mNextLine >= mFileEnd) {
        this->addr = nullptr;
        this->len = 0;
        return *this;
    }

    if (*mNextLine == '\n') {  // this should always be the case except for the first call
        mNextLine += 1;
    }
    char *c = mNextLine;
    while (c < mFileEnd) {
        if (*c == '\n') {
            break;
        }
        ++c;
    }
    this->addr = mNextLine;
    this->len = c - mNextLine;
    if (this->len > 0 && *(this->addr + this->len - 1) == '\r') {
        this->len -= 1;
    }
    mNextLine = c;

    return *this;
}

File::Lines::Iterator File::Lines::Iterator::operator++(int)
{
    return operator++();
}

bool File::Lines::Iterator::operator==(const Iterator& rhs) const
{
    // This is really only needed for 'while (lines.begin() != lines.end())',
    // but there's no need to compare lengths, since they would always be
    // the same if the line address is the same.
    return (this->addr == rhs.addr);
}

bool File::Lines::Iterator::operator!=(const Iterator& rhs) const
{
    return (this->addr != rhs.addr);
}

std::string File::Lines::Iterator::line() const { return std::string(addr, len); }

/// Returns a structure to iterator over lines in a file. The \n (and \r
/// if on Windows) are NOT included in the line. Note that if an error
/// occurs, usually there will be no lines, but it is possible for an
/// I/O error to occur after a file was succesfully opened.
File::Lines File::readLines(IOError::Error *err)
{
    IOError::Error e;
    auto addr = mmap(&e);
    if (err) {
        *err = e;
    }
    if (e == IOError::kNone) {
        return File::Lines(mPath, addr);
    } else {
        return File::Lines();
    }
}
/*
IOError::Error File::rename(const std::string& newPath)
{
    if (::rename(mPath.c_str(), newPath.c_str()) < 0) {
        return errnoToIOError(errno);
    }
    mPath = newPath;
    return IOError::kNone;
}
*/
IOError::Error File::remove()
{
    if (::unlink(mPath.c_str()) < 0) {
        return IOError::fromErrno(errno);
    }
    return IOError::kNone;
}

// Tests (see tests/test.cpp)
// 1: ""         -> []
// 2: "\n\n"     -> ["", "", ""]
// 3: "a\nbb\n"  -> ["a", "bb", ""]
// 4: "a\nbb"    -> ["a", "bb"]
// 5: "a\n\ncc"    -> ["a", "", "cc"]
// 6: "\r\nbb"   -> ["", "bb" ]
// 7: "a\r\nbb"  -> ["a", "bb" ]

}  // namespace uitk
