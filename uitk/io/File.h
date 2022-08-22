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

#ifndef UITK_FILE_H
#define UITK_FILE_H

#include <string>
#include <vector>

#include "FileSystemNode.h"

namespace uitk {

class File : public FileSystemNode
{
public:
    File();
    File(const std::string& path);

    std::string readContentsAsString(IOError::Error *err) const;
    std::vector<char> readContents(IOError::Error *err) const;
    IOError::Error writeContents(const std::string& contents);
    IOError::Error writeContents(const std::vector<char>& contents);
    IOError::Error writeContents(const char *contents, uint64_t size);

    IOError::Error remove() override;
    
    struct MappedAddress {
        char *addr = nullptr;
        uint64_t len = 0;  // do not modify this, it is needed by munmap()
    };

    /// Maps the file into memory as read-only. The destructor does NOT call
    /// munmap(). MappedAddress.addr will be nullptr on failure. Note that
    /// mapping a file larger than 4 GB on a 32-bit system may not work.
    /// Attempting to map an empty file will not fail, but will return
    /// the default address { nullptr, 0 }.
    MappedAddress mmap(IOError::Error *err) const;
    /// This will be a no-op if mapping.addr == nullptr (so always calling
    /// munmap() is safe, and recommended).
    void munmap(const MappedAddress& mapping);

    class Lines
    {
    public:
        struct Iterator {
            friend Lines;

            char *addr;
            uint64_t len;

            /// Copies the line into a string. Access the addr pointer
            /// directly if you do not need/want a copy, or if a string
            /// is inappropriate.
            std::string line() const;

            std::string operator*();
            Iterator& operator++();
            Iterator operator++(int);

            bool operator==(const Iterator& rhs) const;
            bool operator!=(const Iterator& rhs) const;

        private:
            char *mFileEnd;
            char *mNextLine;
        };

        Lines();
        Lines(const std::string& path, const File::MappedAddress& addr);
        ~Lines();

        Iterator begin();
        Iterator end();

        /// Equivalent to:
        ///    std::vector<std::string> retVal;
        ///    for (auto line : lines) {
        ///        retVal.push_back(line);
        ///    }
        std::vector<std::string> allLines();

    private:
        std::string mPath;
        MappedAddress mAddr;
        Iterator mBegin;
    };

    /// Returns a structure to iterator over lines in a file. The \n (and
    /// trailing \r, because, Windows) are NOT included in the line. If an
    /// error occurs there will be no lines. The assumed use for this
    /// function is:
    ///    for (auto line : f.readLines(&err)) {
    ///        ...
    ///    }
    ///    if (err != IOError::kNone) { ... }
    Lines readLines(IOError::Error *err);
};

}  // namespace uitk
#endif // UITK_FILE_H
