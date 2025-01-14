UI Toolkit
----------
The Eight Brains UI Toolkit is a cross-platform user-interface toolkit currently supporting Windows, macOS, and Linux/X11. Future support is planned for WebAssembly, iOS, Android, offscreen-rendering, and possibly OpenGL/WebGL, Vulkan, Linux/Wayland. Current development is focused on basic UI functionality for 80% of native apps. Future development aims to be a reference user-interface implementation, including a detailed specification and test suite. Future development also includes wrappers for at least Python and Swift.

### Building

Uses CMake 3.10 or later.

#### macOS and Linux
```
mkdir build
cd build
cmake ..
make
```
You can get an Xcode project for the library with `cmake -G Xcode ..`.

#### Windows
```
mkdir build
cd build
cmake -G "Visual Studio 16 2019" ..
cmake --build .
```

### DPI
On macOS lengths are physically accurate (e.g. PicaPt(72) measures one inch with a ruler) when using the native resolution of the display, set in Settings >> Display >> Resolution. Note that the "Default" setting may _not_ be the native resolution! For instance, large MacBook Pros, commonly used by developers, use a scaled resolution by default. Since the resolution setting is clearly intended to be a UI scale factor, UITK applies this to the UI.

On Windows, the UI scales as expected; increasing the scale factor increases the effective DPI of the screen.

On Linux/X11, the `Xft.dpi` Xresources setting will be used. If that is not set, the X server's DPI will be used (which is probably incorrect for high DPI screens).

### Design philosphy
The interface design is strongly influenced by Cocoa (aka NextStep) with some influence by Qt. The default theme is aesthetically similar to macOS because, of the major desktop operating systems, only macOS has a strong and consistent aesthetic. Linux has no official toolkit and both GTK and Qt are commonly used, with the result that apps are a hodgepodge of styles. Windows has gone through many incarnations of UI, each with a different look. The closest to an official style is the old Win32 widgets, which are outdated. Windows apps also have a very hodgepodge look about them, including the core operating system, which has some panels that have the most recent look, which some older, more advanced-user panels look like they are unchanged from NT 4.0. Also--unlike Linux--I have not heard complaints about the visual differences of Windows apps, which macOS user value a native look-and-feel much more highly, so it make sense to target macOS by default. (And if the library gets strong uptake, at least some integration with GTK themes is probably fairly easy to support.)

UITK uses a limited MVC approach. The `Widget` class joins the model and controller, while the theme implementation does the drawing and hit-testing. The controller controls layout of child widgets, where a widget requires them, but most layout is done by the view class. For reliability, the drawing code and the hit-test code need to be in the same module, in this case, the view. This is because drawing code essentially converts model (e.g. 3.141592f) into pixels, and the hit-test code needs to convert pixels into a location in an editable model (e.g. placing the cursor after "4").

### Currently implemented
- Platforms: macOS, Windows, Linux/X11 (might work with other Unixen), WebAssembly
- Basic widgets one would expect
- Internationalization:
  - Displays text in all languages, but right-to-left may not work properly
  - Supports VoiceOver on macOS
- Basic sound playing (use OpenAL or similar if need something more thorough)
- Printing (Linux prints to PDF, and WebAssembly has no printing facility)

### Not yet implemented
- Gesture support
- Multi-monitor may have bugs, probably mostly in moving between monitors
- Drag-n-drop
- Verify/implement emoji support, including correct delete / cursor moving especially if the DPI is different
- Verify/implement newer macOS features like voice dictation
- Full right-to-left support
- Drawing drop shadows
- Tab control (use SegmentedWidget + StackedWidget instead, like Mac apps do)
- Native widget container (would allow using native OpenGL, video, WebView, etc.)
- Swift, Python, Rust? bindings

### Out of scope (at least for now)
- Networking (use cURL or a C++ library)
- WebView (infeasible to implement, and can't abstract native widget because Linux/WASM don't have one)
- Video playback (not sure how support on Linux [especially] and WebAssembly would work)
- OpenGL/Vulkan backend
- OpenGL/Vulkan widget
- VoiceOver equivalent in Windows (the library supports it, but crashes in kernel32.dll with no information, and I really hate spending time on Windows; if someone opens an issue with information on how to resolve the crash I will implement the rest).

### Why a new toolkit?
I am not aware of a toolkit that covers all of the following:

1. Supports Windows, macOS, Linux, WebAssembly, iOS, and Android
2. Allows the use of multiple languages
3. Permissive license
4. Good platform integration
5. Ease of deployment
6. Teaching tool (design decision explanation, widget requirements specification)

Major toolkits that are available are:

* [Qt](https://www.qt.io/): this is very full-featured and is the professional choice for cross-platform development. It supports Windows, macOS, Linux, iOS, Android, and can be built for WebAssembly. At the time of initial development, the license was expensive for small companies, and the free license (LGPL) is unclear and requires hosting a 750 MB source file for download for an indeterminate time to comply with the LGPL license. (Some of this may no longer be the case.) The macOS implementation is clunky and not very native. Also, new features seem to be restricted to Qt Quick, which uses a sort of JavaScript. However, Qt is historically a C++ library, and C++ has quite opposite design values than JavaScript. Finally, the library is quite large, and while shipping a Qt app is not difficult, the packaging process has a few gotchas on some platforms that are time-consuming to discover.
* [Flutter](https://flutter.dev/): developed by Google, supports all the major platforms. However, it requires programming in the Dart language.
* [Gtk](https://www.gtk.org/): supports Linux, macOS, and Windows, and possibly WebAssembly. However, Windows and macOS are functional, but do not look or function at all native. Also, the library is written in C, which can be used by many programming languages, but this makes for a fairly low-level experience in languages like C++. GTK is functionally the "native" UI toolkit for Linux, so is a good choice for apps that are primarily targeted to Linux but which need to run on other platforms.
* [wxWidgets](https://www.wxwidgets.org/): implemented for Windows, Linux, and macOS, plus others. Descended from MFC 1.0, so the interface is not very modern and very Microsoft-oriented. My experience was that the widgets were fairly inelegant visually, some features were spotty, and that MFC is not the best interface. (Hopefully it has improved since then, although Qt is mentioned as a platform solution over wxWidgets by a wide margin.)
* [Dear ImGUI](https://github.com/ocornut/imgui): immediate-mode GUI for OpenGL, mostly intended for internal game debugging UI. It is good at what it intends. However, it has programmer-aesthetics visually, and immediate mode GUI is not really sustainable. In fact, Dear ImGUI is really retained mode under the hood: the name of the widget is used as an index to store properties such as layout information. While it could be (and has been) used as a game UI, it is not really intended or suitable for normal applications.
* [JUCE](https://github.com/juce-framework/JUCE): used by the JUCE audio app, and supports all the major platforms except WebAssembly. Listed here for completeness, I have not used it, nor is it commonly used, compared to the above. 
