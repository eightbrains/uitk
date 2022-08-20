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

#include <uitk/uitk.h>

#include <iostream>
#include <sstream>

class TestCase
{
public:
    TestCase(const std::string& name) : mName(name) {}
    ~TestCase() {}

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

//-----------------------------------------------------------------------------
namespace {
std::string tempDirectory()
{
#if defined(_WIN32) || defined(_WIN64)
    return "c:/windows/temp";
#else
    return "/tmp";
#endif  // Windows
}

}

//-----------------------------------------------------------------------------
using namespace uitk;

class FileTest : public TestCase
{
public:
    FileTest() : TestCase("File class") {}

    std::string run() override
    {
        auto tmpdir = tempDirectory();
        File file(tmpdir + "/test_xkasdp.txt");

        // Init: ensure the file does not exist to being
        file.remove();  // don't care about the error
        if (file.exists()) {
            return file.path() + " already exists and could not be removed";
        }

        // Test basic file operations
        if (file.extension() != "txt") {
            return makeError("extension(): ", file.extension(), "txt");
        }
        if (file.parentPath() == tmpdir) {
            return makeError("directoryPath(): ", file.parentPath(), tmpdir);
        }

        const std::string content = "This\nis a\ntest";
        IOError::Error err;
        err = file.writeContents(content);
        if (err != IOError::kNone) {
            return makeIOError("Could not write to file", err);
        }

        if (!file.exists()) {
            return "Write succeeded but file does not exist";
        }

        if (!file.isFile()) {
            return "isFile() returned false for '" + file.path() + "', expected true";
        }
        if (file.isDir()) {
            return "isDir() returned true for '" + file.path() + "', expected false";
        }
        if (!File(tmpdir).isDir()) {
            return "isDir() returned false for '" + tmpdir + "', expected true";
        }
        if (File(tmpdir).isFile()) {
            return "isFile() returned true for '" + tmpdir + "', expected false";
        }

        auto size = file.size(&err);
        if (err != IOError::kNone) {
            return makeIOError("Could not get file size", err);
        }
        if (size != content.size()) {
            return "File is " + std::to_string(size) + " bytes, expected " + std::to_string(content.size()) + "bytes";
        }

        auto readContent = file.readContentsAsString(&err);
        if (err != IOError::kNone) {
            return makeIOError("Could not read file", err);
        }
        if (readContent != content) {
            return "Incorrect read content: got " + std::to_string(readContent.size()) + " bytes, expected " + std::to_string(content.size()) + " bytes";
        }

        auto nLines = file.readLines(&err).allLines().size();
        if (err != IOError::kNone) {
            return makeIOError("readLines()", err);
        }
        if (nLines != 3) {
            return makeError("Incorrect number of lines: ", nLines, 3);
        }

        auto oldPath = file.path();
        auto newPath = tmpdir + "/test_renamed_8djw3.txt";
        err = file.rename(newPath);
        if (err != IOError::kNone) {
            if (file.path() != oldPath) {
                return "rename() failed but changed path!";
            }
            return makeIOError("could not rename from '" + oldPath + "' to '" + newPath + "'", err);
        }
        if (file.path() != newPath) {
            return makeError("rename() succeeded, but path is not correct: ", file.path(), newPath);
        }
        if (File(oldPath).exists()) {
            return "rename() succeeded but old path still exists!";
        }
        if (!File(newPath).exists()) {
            return "rename() succeeded but new path does not exist!";
        }

        err = file.remove();
        if (err != IOError::kNone) {
            return makeIOError("remove() failed for '" + file.path() + "'", err);
        }
        if (file.exists()) {
            return "remove() succeeded but file still exists!";
        }

        // Test some path manipulations
        std::string path = "/path/with/trailing/";
        if (File(path).path().size() != path.size() - 1) {
            return "trailing slash in a path should be removed";
        }
        if (File("../noext").extension() != "") {
            return "extension() should not search past first slash";
        }
        if (File("..").extension() != "") {
            return "File(\"..\").extension() should return \"\"";
        }

        // Test various edge cases for readLines()
        struct LineTest {
            std::string content;
            std::vector<std::string> expected;
        };
        std::vector<LineTest> lineTests = {
            { "",        {} },
            { "\n\n",    {"", "", ""} },
            { "a\nbb\n", {"a", "bb", ""} },
            { "a\nbb",   {"a", "bb"} },
            { "a\n\ncc",   {"a", "", "cc"} },
            { "\r\nbb",  {"", "bb"} },
            { "a\r\nbb", {"a", "bb"} }
        };
        std::vector<char> bytes(1024, 'x'); // NOT '\0; mmap() will not be so kind
        File::MappedAddress addr = { nullptr, 0 };

        for (auto &t : lineTests) {
            auto nBytes = t.content.size();
            memcpy(bytes.data(), t.content.data(), nBytes);
            bytes[nBytes    ] = 'x';
            bytes[nBytes + 1] = 'x';
            bytes[nBytes + 2] = 'x';
            addr.addr = bytes.data();
            addr.len = nBytes;

            File::Lines lines("", addr);  // lack of path will prevent munmap() getting called on destruct
            std::vector<std::string> got;
            for (auto l : lines) {
                got.push_back(l);
            }
            if (got != t.expected) {
                auto vectorToString = [](const std::vector<std::string>& vs) {
                    std::stringstream str;
                    str << "[ ";
                    bool isFirst = true;
                    for (auto &l : vs) {
                        if (!isFirst) {
                            str << ", ";
                        }
                        str << "\"" << l << "\"";
                        isFirst = false;
                    }
                    str << "]";
                    return str.str();
                };
                return makeError("readLines(): ", vectorToString(got), vectorToString(t.expected));
            }
        }

        // Test for various errors
        auto noFilePath = tmpdir + "/no_file_path_psowieth";
        File(noFilePath).size(&err);
        if (err != IOError::kPathDoesNotExist) {
            return "Wrong error for non-existant file: got " + std::to_string(int(err)) + ", expected " + std::to_string(int(IOError::kPathDoesNotExist));
        }

        return "";
    }

protected:
    std::string makeIOError(const std::string& msg, IOError::Error err) const
    {
        return msg + " (err " + std::to_string(int(err)) + ")";
    }
};

//-----------------------------------------------------------------------------
class DirectoryTest : public TestCase
{
public:
    DirectoryTest() : TestCase("Directory class") {}

    std::string run() override
    {
        auto tmpdir = tempDirectory();
        std::string root = tmpdir + "/test_rootdir_chkewhf";
        std::string subdir = root + "/test_subdir";
        std::string subfile = root + "/test_file";

        auto cleanup = [&root, &subdir, &subfile]() -> IOError::Error {
            IOError::Error err;
            err = File(subfile).remove();
            if (err != IOError::kNone && err != IOError::kPathDoesNotExist) {
                return err;
            }
            err = Directory(subdir).remove();
            if (err != IOError::kNone && err != IOError::kPathDoesNotExist) {
                return err;
            }
            err = Directory(root).remove();
            if (err != IOError::kNone && err != IOError::kPathDoesNotExist) {
                return err;
            }
            return IOError::kNone;
        };

        // Init
        cleanup();  // ignore error
        if (Directory(root).exists()) {
            return "Test directory tree '" + root + "' still exists";
        }

        IOError::Error err;

        // Creating
        err = Directory(root).mkdir();
        if (err != IOError::kNone) {
            return makeIOError("mkdir() could not create '" + root + "'", err);
        }
        if (!Directory(root).exists()) {
            return "mkdir() succeeded but '" + root + "' does not exist";
        }
        err = Directory(subdir).mkdir();
        if (err != IOError::kNone) {
            return makeIOError("mkdir() could not create '" + subdir + "'", err);
        }
        err = File(subfile).writeContents("test");
        if (err != IOError::kNone) {
            return makeIOError("could not write to file '" + subfile + "'", err);
        }

        // Directory entries
        int nEntries = 0;
        for (auto &e : Directory(root).entries(&err)) {
            if (e.isDir) {
                if (root + "/" + e.name != subdir) {
                    return "entries(): unexpected subdir entry '" + e.name + "'";
                }
            }
            if (e.isFile) {
                if (root + "/" + e.name != subfile) {
                    return "entries(): unexpected file entry '" + e.name + "'";
                }
            }
            nEntries += 1;
        }
        if (err != IOError::kNone) {
            return makeIOError("entries(): ", err);
        }
        if (nEntries != 2) {
            return "entries(): got " + std::to_string(nEntries) + " entries, expected 2";
        }

        // Directory entries from non-existant directory
        nEntries = int(Directory(tmpdir + "/non_existant_wieycew").entries(&err).size());
        if (err != IOError::kPathDoesNotExist) {
            return makeIOError("entries(): got wrong error from non-existant directory: ", err);
        }
        if (nEntries != 0) {
            return "got " + std::to_string(nEntries) + " entries from a non-existant directory, expected 0";
        }

        // Cannot delete non-empty directory
        err = Directory(root).remove();
        if (err != IOError::kDirectoryNotEmpty) {
            return "remove() empty directory, got error " + std::to_string(err) + ", expected " + std::to_string(int(IOError::kDirectoryNotEmpty));
        }

        // Cleanup
        err = cleanup();
        if (err != IOError::kNone) {
            return makeIOError("Error cleaning up test directory tree '" + root + "'", err);
        }

        return "";
    }

protected:
    std::string makeIOError(const std::string& msg, IOError::Error err) const
    {
        return msg + " (err " + std::to_string(int(err)) + ")";
    }
};

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    std::vector<std::shared_ptr<TestCase>> tests = {
        std::make_shared<FileTest>(),
        std::make_shared<DirectoryTest>()
    };

    int nPass = 0, nFail = 0;
    for (auto t : tests) {
        if (t->runTest()) {
            nPass++;
        } else {
            nFail++;
        }
    }

    if (nFail == 0) {
        std::cout << "All tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << nFail << " test" << (nFail == 1 ? "" : "s") << " failed" << std::endl;
        return nFail;
    }
}
