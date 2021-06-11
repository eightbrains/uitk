//-----------------------------------------------------------------------------
// Copyright 2021 Eight Brains Studios, LLC
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

#ifndef UITK_APPLICATION_H
#define UITK_APPLICATION_H

#include <memory>

namespace uitk {

class OSApplication;
class Theme;

class Application
{
public:
    /// The Application constructor will set the instance, so that you can
    /// inherit from Application if you want.
    static Application& instance();

    /// Instantiating an Application must be the first thing you do in the
    /// program before calling any other function in the library, as some
    /// (such as window creation) will access the instance. The application
    /// instance must live the for the duration of the program. Therefore,
    /// it is usually placed in the main() function, such as:
    ///     int main(int argc, char *argv) {
    ///         Application app;
    ///         return app.run();
    ///     }
    Application();
    virtual ~Application();

    /// On macOS apps do not usually exit when all the windows close; this
    /// can be used to change that behavior. It is ignored on other platforms,
    /// since there would be no way to re-open a window without a menubar,
    /// on non-Mac platforms is tied to the window.
    void setExitWhenLastWindowCloses(bool exits);

    /// Runs the event loop
    int run();

    /// Gets the application's theme.
    std::shared_ptr<Theme> theme() const;

    void onSystemThemeChanged();

    OSApplication& osApplication();  // for internal use

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace uitk

#endif // UITK_APPLICATION_H
