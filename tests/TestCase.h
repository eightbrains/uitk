//-----------------------------------------------------------------------------
// Copyright 2021 - 2023 Eight Brains Studios, LLC
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

#include <iostream>
#include <sstream>

std::string getTempDir();

class TestCase
{
public:
    TestCase(const std::string& name) : mName(name) {}
    virtual ~TestCase() {}

    virtual std::string run() = 0;  // return "" on success

    bool runTest()
    {
        auto err = run();
        if (err.empty()) {
            std::cout << "[pass] " << mName << std::endl;
        } else {
            std::cout << "[FAIL] " << mName << std::endl;
            std::cout << "    " << err << std::endl;
        }
        return err.empty();
    }

protected:
    std::string makeError(const std::string& prefix, uint64_t got, uint64_t expected) const
    {
        return prefix + ": got " + std::to_string(got) + ", expected " + std::to_string(expected);
    }

    std::string makeError(const std::string& prefix, const std::string& got, const std::string& expected) const
    {
        return prefix + ": got " + got + ", expected " + expected;
    }

private:
    std::string mName;
};

