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

#include <assert.h>
#include <errno.h>
#include <fcntl.h> // for open()
#include <sys/types.h>

// For fopen(), ftell(), fseeko()
#if defined(_WIN32) || defined(_WIN64)
#define ftello _ftelli64
#define fseeko _fseeki64
#endif

// for unlink()
#if defined(_WIN32) || defined(_WIN64)
#include <io.h>  // also requires stdio.h which is already included
#else
#include <unistd.h>
#endif

// for mmap()
#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <map>
#include <windows.h>
#include "../win32/Win32Utils.h"
#else
#include <sys/mman.h>
#endif

namespace uitk {

#if defined(_WIN32) || defined(_WIN64)
struct MappingInfo
{
    HANDLE hFile;
    HANDLE hFileMap;
};
// Keep a global mapping table keyed on the mapped address (which should be unique!),
// so that we can keep the implementation details (i.e. the OS handles) out of the
// File::MappingAddress object definition.
std::map<char*, MappingInfo> gWin32MappingInfo;

IOError::Error win32ErrorToIOError(int win32err)
{
    switch (win32err) {
        case ERROR_SUCCESS:
            return IOError::kNone;
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            return IOError::kPathDoesNotExist;
        case ERROR_ACCESS_DENIED:
            return IOError::kNoPermission;
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_OUTOFMEMORY:
            return IOError::kNoMemory;
        default:
            return IOError::kOther;
    }
}
#endif // win32

template <class T>
void readFileContents(const std::string& path, T *contents, IOError::Error *err)
{
#if defined(_WIN32) || defined(_WIN64)
    FILE *in;
    errno_t e = fopen_s(&in, path.c_str(), "rb");   // with no "b" windows converts \n -> \r\n
    if (e) {
        if (err) {
            *err = IOError::fromErrno(e);
        }
        return;
    }
#else
    FILE *in = fopen(path.c_str(), "rb");  // with no "b" windows converts \n -> \r\n
    if (!in) {
        if (err) {
            *err = IOError::fromErrno(errno);
        }
        return;
    }
#endif
    if (in) {
        fseeko(in, 0, SEEK_END);
        auto size = ftello(in);
        contents->resize(size);
        fseeko(in, 0, SEEK_SET);
        fread(contents->data(), size, 1, in);
        fclose(in);
        if (err) {
            *err = IOError::kNone;
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
#if defined(_WIN32) || defined(_WIN64)
    FILE *out;
    errno_t e = fopen_s(&out, mPath.c_str(), "wb");   // with no "b" windows converts \n -> \r\n
    if (e) {
        return IOError::fromErrno(e);
    }
#else
    FILE *out = fopen(mPath.c_str(), "wb");  // with no "b" windows converts \n -> \r\n
    if (!out) {
        return IOError::fromErrno(errno);
    }
#endif
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

    // Newer mmap (macOS, Linux >= 2.6.12) return EINVAL for empty files.
    // Win32 also fails mapping an empty file.
    if (fileLen == 0) {
        if (err) {
            *err = IOError::kNone;
        }
        return addr;
    }

#if defined(_WIN32) || defined(_WIN64)
    auto path = win32UnicodeFromUTF8(calcWindowsPath());
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (!hFile) {
        if (err) {
            *err = win32ErrorToIOError(GetLastError());
        }
        return addr;
    }
    HANDLE hFileMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!hFileMap) {
        if (err) {
            *err = win32ErrorToIOError(GetLastError());
        }
        return addr;
    }
    addr.addr = (char *)MapViewOfFile(hFileMap, FILE_MAP_READ,
                                      0, 0, // high and low bytes of offset (i.e 0)
                                      0);   // map to end of file
    if (!addr.addr) {
        if (err) {
            *err = win32ErrorToIOError(GetLastError());
        }
        CloseHandle(hFile);
        return addr;
    }
    if (err) {
        *err = IOError::kNone;
    }
    addr.len = fileLen;
    assert(gWin32MappingInfo.find(addr.addr) == gWin32MappingInfo.end());
    gWin32MappingInfo[addr.addr] = { hFile, hFileMap };
#else
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
#endif

    return addr;
}

void File::munmap(const File::MappedAddress& mapping)
{
    if (!mapping.addr) {
        return;
    }
#if defined(_WIN32) || defined(_WIN64)
    UnmapViewOfFile(mapping.addr);
    auto infoIt = gWin32MappingInfo.find(mapping.addr);
    if (infoIt != gWin32MappingInfo.end()) {
        CloseHandle(infoIt->second.hFileMap);
        CloseHandle(infoIt->second.hFile);
        gWin32MappingInfo.erase(infoIt);
    }
#else
    ::munmap(mapping.addr, mapping.len);
#endif
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
#if defined(_WIN32) || defined(_WIN64)
    if (::_unlink(mPath.c_str()) < 0) {
#else
    if (::unlink(mPath.c_str()) < 0) {
#endif
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
