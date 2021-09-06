set(ND_NAMESPACE "uitk")

set(UITK_HEADERS Application.h
                 Button.h
                 Checkbox.h
                 Clipboard.h
                 ComboBox.h
                 Events.h
                 Global.h
                 Label.h
                 ListView.h
                 Menu.h
                 OSApplication.h
                 OSWindow.h
                 PopupMenu.h
                 ProgressBar.h
                 ScrollBar.h
                 ScrollView.h
                 SegmentedControl.h
                 Slider.h
                 SliderLogic.h
                 StringEdit.h
                 StringEditorLogic.h
                 TextEditorLogic.h
                 UIContext.h
                 Widget.h
                 Window.h
                 themes/Theme.h
                 themes/EmpireTheme.h
                 themes/VectorBaseTheme.h
                 )
set(UITK_SOURCES Application.cpp
                 Button.cpp
                 Checkbox.cpp
                 ComboBox.cpp
                 Label.cpp
                 ListView.cpp
                 Menu.cpp
                 PopupMenu.cpp
                 ProgressBar.cpp
                 ScrollBar.cpp
                 ScrollView.cpp
                 SegmentedControl.cpp
                 Slider.cpp
                 SliderLogic.cpp
                 StringEdit.cpp
                 StringEditorLogic.cpp
                 TextEditorLogic.cpp
                 Widget.cpp
                 Window.cpp
                 themes/EmpireTheme.cpp
                 themes/VectorBaseTheme.cpp
                 )
set(UITK_LIBS "")

set(LIBNATIVEDRAW_PREFIX "${CMAKE_BINARY_DIR}/libnativedraw")
ExternalProject_Add(libnativedraw
                    GIT_REPOSITORY "https://github.com/eightbrains/libnativedraw"
                    PREFIX ${LIBNATIVEDRAW_PREFIX}
                    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${LIBNATIVEDRAW_PREFIX} "-DCMAKE_CXX_FLAGS:STRING=-DND_NAMESPACE=${ND_NAMESPACE}"
                   )
add_library(nativedraw STATIC IMPORTED)
set_target_properties(nativedraw PROPERTIES IMPORTED_LOCATION "${LIBNATIVEDRAW_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}nativedraw${CMAKE_STATIC_LIBRARY_SUFFIX}")
list(APPEND UITK_LIBS nativedraw)

if (APPLE)
    list(APPEND UITK_HEADERS macos/MacOSApplication.h
                             macos/MacOSClipboard.h
                             macos/MacOSWindow.h)
    list(APPEND UITK_SOURCES macos/MacOSApplication.mm
                             macos/MacOSClipboard.mm
                             macos/MacOSWindow.mm)
    find_library(CORE_GRAPHICS CoreGraphics)
    find_library(APPKIT AppKit)
    list(APPEND UITK_LIBS ${APPKIT} ${CORE_GRAPHICS})
elseif (WIN32)
    list(APPEND UITK_HEADERS win32/Win32Application.h
                             win32/Win32Clipboard.h
                             win32/Win32Utils.h
                             win32/Win32Window.h)
    list(APPEND UITK_SOURCES win32/Win32Application.cpp
                             win32/Win32Clipboard.cpp
                             win32/Win32Utils.cpp
                             win32/Win32Window.cpp)
    list(APPEND UITK_LIBS d3d11 d2d1 dwrite shcore WindowsApp)
elseif (UNIX)  # note that APPLE is UNIX, so need to check that first
    list(APPEND UITK_SOURCES x11/X11Application.cpp
                             x11/X11Clipboard.cpp
                             x11/X11Window.cpp)
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

    include_directories(${X11_INCLUDE_DIRS} ${CAIRO_INCLUDE_DIRS})
    list(APPEND UITK_LIBS ${CAIRO_LIBRARIES}
                          ${PANGO_LIBRARIES}
                          ${X11_Xrender_LIB}
                          ${X11_LIBRARIES})
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
