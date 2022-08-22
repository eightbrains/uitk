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

#include "IOError.h"

#include <errno.h>

namespace uitk {
namespace IOError {

Error fromErrno(int e)
{
    switch (e) {
        case 0:             return IOError::kNone;
        case EACCES:        return IOError::kNoPermission;
        case ENAMETOOLONG:  return IOError::kNameTooLong;
        case ELOOP:         return IOError::kSymlinkLoop;
        case ENOTDIR:       return IOError::kPathComponentIsNotDir;
        case ENOENT:        return IOError::kPathDoesNotExist;
        case ENOMEM:        return IOError::kNoMemory;
        case EIO:           return IOError::kIOError;
        case ENOTEMPTY:     return IOError::kDirectoryNotEmpty;
        case EMFILE: // fall through
        case ENFILE:
            return IOError::kOutOfSystemResources;
        default:            return IOError::kOther;
    }
}

} // namespace IOError
} // namespace uitk
