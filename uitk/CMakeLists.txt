set(ND_NAMESPACE "uitk")

set(UITK_PUBLIC_HEADERS
                 Accessibility.h
                 Application.h
                 Button.h
                 CellWidget.h
                 Checkbox.h
                 Clipboard.h
                 ColorEdit.h
                 ComboBox.h
                 Cursor.h
                 CutPasteable.h
                 Dialog.h
                 Events.h
                 FileDialog.h
                 FontListComboBox.h
                 Global.h
                 Icon.h
                 IconAndText.h
                 ImageView.h
                 IncDecWidget.h
                 IPopupWindow.h
                 Label.h
                 Layout.h
                 Length.h
                 ListView.h
                 Menu.h
                 MenuUITK.h
                 MenubarUITK.h
                 NumberEdit.h
                 NumericModel.h
                 OSApplication.h
                 OSCursor.h
                 OSMenu.h
                 OSMenubar.h
                 OSWindow.h
                 PopupWindow.h
                 ProgressBar.h
                 Screen.h
                 ScrollBar.h
                 ScrollView.h
                 SearchBar.h
                 SegmentedControl.h
                 ShortcutKey.h
                 Slider.h
                 SliderLogic.h
                 Sound.h
                 Splitter.h
                 StackedWidget.h
                 StringEdit.h
                 StringEditorLogic.h
                 TextEditorLogic.h
                 UIContext.h
                 Waiting.h
                 Widget.h
                 Window.h
                 themes/Theme.h
                 themes/EmpireTheme.h
                 themes/GetBorderTheme.h
                 themes/VectorBaseTheme.h
                 themes/IconPainter.h
                 themes/StandardIconPainter.h
                 io/Directory.h
                 io/File.h
                 io/FileSystemNode.h
                 io/IOError.h
                 )
set(UITK_HEADERS ${UITK_PUBLIC_HEADERS}
                 private/MenuIterator.h
                 private/Utils.h)
set(UITK_SOURCES Accessibility.cpp
                 Application.cpp
                 Button.cpp
                 Checkbox.cpp
                 ColorEdit.cpp
                 ComboBox.cpp
                 Cursor.cpp
                 Dialog.cpp
                 FileDialog.cpp
                 FontListComboBox.cpp
                 Icon.cpp
                 IconAndText.cpp
                 ImageView.cpp
                 IncDecWidget.cpp
                 Label.cpp
                 Layout.cpp
                 Length.cpp
                 ListView.cpp
                 Menu.cpp
                 MenubarUITK.cpp
                 MenuUITK.cpp
                 NumberEdit.cpp
                 NumericModel.cpp
                 OSMenubar.cpp
                 PopupWindow.cpp
                 ProgressBar.cpp
                 Screen.cpp
                 ScrollBar.cpp
                 ScrollView.cpp
                 SearchBar.cpp
                 SegmentedControl.cpp
                 ShortcutKey.cpp
                 Slider.cpp
                 SliderLogic.cpp
                 Splitter.cpp
                 StackedWidget.cpp
                 StringEdit.cpp
                 StringEditorLogic.cpp
                 TextEditorLogic.cpp
                 Waiting.cpp
                 Widget.cpp
                 Window.cpp
                 private/MenuIterator.cpp
                 private/Utils.cpp
                 themes/Theme.cpp
                 themes/EmpireTheme.cpp
                 themes/GetBorderTheme.cpp
                 themes/VectorBaseTheme.cpp
                 themes/StandardIconPainter.cpp
                 io/Directory.cpp
                 io/File.cpp
                 io/FileSystemNode.cpp
                 io/IOError.cpp
                 )
set(UITK_LIBS "")

set(LIBNATIVEDRAW_PREFIX "${CMAKE_BINARY_DIR}/libnativedraw")
ExternalProject_Add(libnativedraw
                    GIT_REPOSITORY "https://github.com/eightbrains/libnativedraw"
                    PREFIX ${LIBNATIVEDRAW_PREFIX}
                    #UPDATE_DISCONNECTED true # not necessary to check github each build
                    CMAKE_COMMAND "${CMAKE_CMD_FOR_EXTERNAL_PROJECT}"  # Emscripten needs emcmake
                    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${LIBNATIVEDRAW_PREFIX} "-DCMAKE_CXX_FLAGS:STRING=-DND_NAMESPACE=${ND_NAMESPACE}"
                   )
add_library(nativedraw STATIC IMPORTED)
set_target_properties(nativedraw PROPERTIES IMPORTED_LOCATION "${LIBNATIVEDRAW_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}nativedraw${CMAKE_STATIC_LIBRARY_SUFFIX}")
list(APPEND UITK_LIBS nativedraw)

if (APPLE)
    list(APPEND UITK_HEADERS macos/MacOSAccessibility.h
                             macos/MacOSApplication.h
                             macos/MacOSClipboard.h
                             macos/MacOSCursor.h
                             macos/MacOSDialog.h
                             macos/MacOSMenu.h
                             macos/MacOSMenubar.h
                             macos/MacOSSound.h
                             macos/MacOSWindow.h)
    list(APPEND UITK_SOURCES macos/MacOSAccessibility.mm
                             macos/MacOSApplication.mm
                             macos/MacOSClipboard.mm
                             macos/MacOSCursor.mm
                             macos/MacOSDialog.mm
                             macos/MacOSMenu.mm
                             macos/MacOSMenubar.mm
                             macos/MacOSSound.mm
                             macos/MacOSWindow.mm)
    find_library(CORE_GRAPHICS CoreGraphics)
    find_library(APPKIT AppKit)
    list(APPEND UITK_LIBS ${APPKIT} ${CORE_GRAPHICS})
elseif (WIN32)
    list(APPEND UITK_HEADERS win32/Win32Application.h
                             win32/Win32Clipboard.h
                             win32/Win32Cursor.h
                             win32/Win32Dialog.h
                             win32/Win32Menu.h
                             win32/Win32Menubar.h
                             win32/Win32Sound.h
                             win32/Win32Utils.h
                             win32/Win32Window.h)
    list(APPEND UITK_SOURCES win32/Win32Application.cpp
                             win32/Win32Clipboard.cpp
                             win32/Win32Cursor.cpp
                             win32/Win32Dialog.cpp
                             win32/Win32Menu.cpp
                             win32/Win32Menubar.cpp
                             win32/Win32Sound.cpp
                             win32/Win32Utils.cpp
                             win32/Win32Window.cpp)
    list(APPEND UITK_LIBS imm32 d3d11 d2d1 dwrite shcore comctl32 winmm WindowsApp)
elseif (EMSCRIPTEN)
    list(APPEND UITK_HEADERS wasm/WASMApplication.h
                             wasm/WASMClipboard.h
                             wasm/WASMCursor.h
                             wasm/WASMWindow.h
                             openal/OpenALSound.h)
    list(APPEND UITK_SOURCES wasm/WASMApplication.cpp
                             wasm/WASMClipboard.cpp
                             wasm/WASMCursor.cpp
                             wasm/WASMWindow.cpp
                             openal/OpenALSound.cpp)
    list(APPEND UITK_LIBS embind)
elseif (UNIX)  # note that APPLE and EMSCRIPTEN are UNIX, so check them first
    list(APPEND UITK_HEADERS x11/X11Application.h
                             x11/X11Clipboard.h
                             x11/X11Cursor.h
                             x11/X11Window.h
                             openal/OpenALSound.h)
    list(APPEND UITK_SOURCES x11/X11Application.cpp
                             x11/X11Clipboard.cpp
                             x11/X11Cursor.cpp
                             x11/X11Window.cpp
                             openal/OpenALSound.cpp)
    find_package(X11)
    find_path(CAIRO_INCLUDE_DIRS
              NAMES cairo.h
              PATH_SUFFIXES cairo)
    if ("${CAIRO_INCLUDE_DIRS}" MATCHES "NOTFOUND")
        message(FATAL_ERROR "Cannot find cairo.h")
    endif()
    find_library(CAIRO_LIBRARIES NAMES cairo)
    if ("${CAIRO_LIBRARIES}" MATCHES "NOTFOUND")
        message(FATAL_ERROR "Cannot find Cairo library")
    endif()

    # Find Pango (and GLib which it requires)
    find_path(PANGO_INCLUDE_DIRS
              NAME pango/pango.h
              PATH_SUFFIXES pango
                            pango-1.0 # Ubuntu 18.04
             )
    if ("${PANGO_INCLUDE_DIRS}" MATCHES "NOTFOUND")
        message(FATAL_ERROR "Cannot find pango.h")
    endif()
    find_path(GLIB_INCLUDE_DIR
              NAME glib.h
              PATH_SUFFIXES glib
                            glib-2.0 # Ubuntu 18.04
             )
    if ("${GLIB_INCLUDE_DIR}" MATCHES "NOTFOUND")
         message(FATAL_ERROR "Cannot find glib.h")
    endif()
    find_path(GLIBCONFIG_INCLUDE_DIR
              NAME glibconfig.h
              PATHS /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/glib/include
                    /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/glib-2.0/include)
    if ("${GLIBCONFIG_INCLUDE_DIR}" MATCHES "NOTFOUND")
         message(FATAL_ERROR "Cannot find glibconfig.h")
    endif()
    list(APPEND PANGO_INCLUDE_DIRS "${GLIB_INCLUDE_DIR}" "${GLIBCONFIG_INCLUDE_DIR}")
    find_library(PANGO_LIB NAMES pango pango-1.0)
    if ("${PANGO_LIB}" MATCHES "NOTFOUND")
        message(FATAL_ERROR "Cannot find Pango library")
    endif()
    find_library(PANGOCAIRO_LIB NAMES pangocairo pangocairo-1.0)
    if ("${PANGOCAIRO_LIB}" MATCHES "NOTFOUND")
        message(FATAL_ERROR "Cannot find Pango-cairo library")
    endif()
    find_library(GLIB_LIB NAMES glib glib-2.0)
    if ("${GLIB_LIB}" MATCHES "NOTFOUND")
        message(FATAL_ERROR "Cannot find GLib library")
    endif()
    find_library(GOBJECT_LIB NAMES gobject gobject-2.0)
    if ("${GOBJECT_LIB}" MATCHES "NOTFOUND")
        message(FATAL_ERROR "Cannot find GObject library")
    endif()
    set(PANGO_LIBRARIES ${PANGO_LIB} ${PANGOCAIRO_LIB} ${GLIB_LIB} ${GOBJECT_LIB})
    find_package(JPEG)
    if ("${JPEG_INCLUDE_DIRS}" MATCHES "NOTFOUND")
        message(FATAL_ERROR "Cannot find JPEG library. (We recommend libturbo-jpeg)")
    endif()
    find_package(PNG)
    if ("${PNG_INCLUDE_DIRS}" MATCHES "NOTFOUND")
        message(FATAL_ERROR "Cannot find PNG library.")
    endif()
    find_package(GIF)
    if ("${GIF_INCLUDE_DIRS}" MATCHES "NOTFOUND")
        message(FATAL_ERROR "Cannot find GIF library. (Install libgif7, libgif-dev on Ubuntu)")
    endif()

    include_directories(${X11_INCLUDE_DIRS} ${CAIRO_INCLUDE_DIRS} ${OPENAL_INCLUDE_DIR})
    # Do not link directly to OpenAL; we will dynamically open the library
    # So that applications that do not require sound will not need to have
    # OpenAL installed.
    list(APPEND UITK_LIBS ${CAIRO_LIBRARIES}
                          ${PANGO_LIBRARIES}
                          ${X11_Xcursor_LIB}
                          ${X11_Xrender_LIB}
                          ${X11_Xfixes_LIB}
                          ${X11_LIBRARIES}
                          ${JPEG_LIBRARIES}
                          ${PNG_LIBRARIES}
                          ${GIF_LIBRARIES}
                          -ldl)
endif()

add_library(uitk ${UITK_HEADERS} ${UITK_SOURCES})
add_dependencies(uitk libnativedraw)
target_compile_definitions(uitk PRIVATE ND_NAMESPACE=${ND_NAMESPACE})
target_include_directories(uitk SYSTEM PRIVATE
                           "${LIBNATIVEDRAW_PREFIX}/include")
target_link_libraries(uitk ${UITK_LIBS})

if (APPLE)
    # We have to enable ARC automatically. All ObjC objects are internal, so
    # callers do not need to have ARC enabled, hence PRIVATE is sufficient.
    target_compile_options(uitk PRIVATE "-fobjc-arc")
endif()

# set_target_properties(uitk PROPERTIES PUBLIC_HEADER "${UITK_PUBLIC_HEADERS}")
# would install the headers automatically (without an install() command) along
# with the exectuable. However, it uses the top-level source and not the
# current source. So it's easier just to install them as files.
foreach (file ${UITK_PUBLIC_HEADERS})  # this way keeps directory structure
    get_filename_component(dir ${file} DIRECTORY)
    install(FILES ${file} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${dir})
endforeach()
install(FILES uitk.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
install(FILES ${LIBNATIVEDRAW_PREFIX}/include/nativedraw.h
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
install(TARGETS uitk LIBRARY)
