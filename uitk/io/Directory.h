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

#ifndef UITK_DIRECTORY_H
#define UITK_DIRECTORY_H

#include <string>
#include <vector>

#include "FileSystemNode.h"

namespace uitk {

class Directory : public FileSystemNode
{
public:
    Directory();
    Directory(const std::string& path);

    /// Creates the directory. If parentPath() does not exist, the
    /// the directory will not be created.
    IOError::Error mkdir();

    IOError::Error remove() override;

    struct Entry
    {
        std::string name;
        bool isDir;
        bool isFile;
        bool isLink;

        std::string extension() const;  /// does not include the "."
    };

    /// Returns the entries in the directory table. Note that this are only
    /// the filename (or subdirectory name); they do NOT include the path().
    /// This is to avoid duplicating all the parent's path for every entry,
    /// which could be rather large for large directory trees. Results do
    /// NOT include the special "." and ".." directories.
    std::vector<Entry> entries(IOError::Error *err) const;
};

} // namespace uitk
#endif // UITK_DIRECTORY_H

