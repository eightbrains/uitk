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

#ifndef UITK_FILE_SYSTEM_NODE_H
#define UITK_FILE_SYSTEM_NODE_H

#include <string>
#include <vector>

#include "IOError.h"

namespace uitk {

class FileSystemNode
{
public:
    FileSystemNode();
    FileSystemNode(const std::string& path);
    virtual ~FileSystemNode();

    const std::string& path() const;
    /// Returns the parent directory path, not including a trailing "/" unless
    /// it is the root directory. Does not convert a relative path into an
    /// absolute path, so the directory of "file.txt" is "".
    std::string parentPath() const;
    /// Returns the name of this node, not including the parent path.
    /// So "/path/to/file.txt" returns "file.txt".
    std::string name() const;
    /// Returns the extension, not including the "."
    std::string extension() const;

    bool exists() const;
    bool isFile() const;  /// returns false if directory or special file, or does not exist
    bool isDir() const;
    uint64_t size(IOError::Error *err) const;

    /// Renames the node on disk (also changes the path of this object
    /// if successful).
    IOError::Error rename(const std::string& newPath);

    /// Removes the node from disk. If the node is a directory, it must
    /// be empty to succeed.
    virtual IOError::Error remove() = 0;

protected:
    std::string mPath;
};

}  // namespace uitk
#endif // UITK_FILE_SYSTEM_NODE_H

