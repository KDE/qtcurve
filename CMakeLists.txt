project(QtCurve)

## Temporary
function(translate_add_sources)
endfunction()

## Setup
cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CMakeCMacros)

## Version
set(QTCURVE_VERSION_MAJOR "1")
set(QTCURVE_VERSION_MINOR "8")
set(QTCURVE_VERSION_PATCH "18")
set(QTCURVE_VERSION "${QTCURVE_VERSION_MAJOR}.${QTCURVE_VERSION_MINOR}")
set(QTCURVE_VERSION_FULL "${QTCURVE_VERSION}.${QTCURVE_VERSION_PATCH}")

find_package(Git QUIET)
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/version.c"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen-version.sh"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
add_custom_target(gen-git-ver ALL
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen-version.sh"
  "${CMAKE_CURRENT_BINARY_DIR}/version.c" "${QTCURVE_VERSION_FULL}"
  "${GIT_EXECUTABLE}"
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/version.c")

## Options
function(qtc_option name doc def_val)
  if(DEFINED "${name}")
    set("${name}_DEFINED" On PARENT_SCOPE)
  else()
    set("${name}_DEFINED" Off PARENT_SCOPE)
    set("${name}" "${def_val}" PARENT_SCOPE)
  endif()
endfunction()
qtc_option(ENABLE_QT4 "Building Qt4 theme." On)
qtc_option(ENABLE_QT5 "Building Qt5 theme." On)
qtc_option(ENABLE_GTK2 "Building Gtk2 theme." On)
# option(ENABLE_GTK3 "Building Gtk3 theme." On)
option(ENABLE_TEST "Enable testing." Off)
option(QTC_ENABLE_X11 "Enable X11" On)
option(QTC_INSTALL_PO "Install translation files." On)

qtc_option(QTC_QT4_ENABLE_KDE "Building Qt4 style with KDE4 support." On)
qtc_option(QTC_QT5_ENABLE_KDE "Building Qt5 style with KDE Frameworks 5 support." On)

option(QTC_QT4_STYLE_SUPPORT
  "Qt4 support for creating a style for each QtCurve theme." Off)

option(QTC_GTK2_MODIFY_MOZILLA "Modify Mozilla Gtk2 with js and css." Off)

## Common settings.
# support an infix argument to have different qtcurve-utils libraries, e.g. for
# GTk2, Qt4 and Qt5
qtc_option(QTC_UTILSLIB_INFIX "an infix argument to have different qtcurve-utils libraries" "")

# Only try to figure out what kde prefix is if not specified at command line.
if(NOT DEFINED QTC_KDE4_PREFIX)
  find_package(KDE4)
  find_program(KDE4_KDECONFIG_EXECUTABLE NAMES kde4-config
    PATHS "${CMAKE_INSTALL_PREFIX}/bin" ${_KDEDIRS} /opt/kde4/bin
    NO_DEFAULT_PATH)
  if(NOT KDE4_KDECONFIG_EXECUTABLE)
    find_program(KDE4_KDECONFIG_EXECUTABLE NAMES kde4-config)
  endif()
  if(KDE4_KDECONFIG_EXECUTABLE)
    execute_process(COMMAND "${KDE4_KDECONFIG_EXECUTABLE}" --prefix
      OUTPUT_VARIABLE QTC_KDE4_PREFIX)
    string(STRIP "${QTC_KDE4_PREFIX}" QTC_KDE4_PREFIX)
    execute_process(COMMAND "${KDE4_KDECONFIG_EXECUTABLE}" --expandvars
      --install icon
      OUTPUT_VARIABLE QTC_KDE4_ICONS_PREFIX)
    string(STRIP "${QTC_KDE4_ICONS_PREFIX}" QTC_KDE4_ICONS_PREFIX)
  else()
    set(QTC_KDE4_PREFIX "${CMAKE_INSTALL_PREFIX}")
  endif()
endif()

if(NOT DEFINED QTC_KDE4_ICONS_PREFIX)
  set(QTC_KDE4_ICONS_PREFIX "${QTC_KDE4_PREFIX}/share/icons")
endif()

## Handle options
find_package(Libexecinfo)
if(LIBEXECINFO_LIB_FOUND)
  set(QTC_ENABLE_BACKTRACE On)
else()
  set(QTC_ENABLE_BACKTRACE Off)
endif()

if(QTC_ENABLE_X11)
  macro(qtc_check_x11)
    find_package(PkgConfig)
    if(NOT PKG_CONFIG_FOUND)
      message(WARNING "Cannot find pkg-config, disable x11 support.")
      set(QTC_ENABLE_X11 Off)
      set(QTC_X11_FOUND Off)
      return()
    endif()
    unset(__pkg_config_checked_QTC_X11 CACHE)
    # pkg_check_modules(QTC_X11 xcb x11-xcb xrender)
    if(QTC_ENABLE_X11)
      pkg_check_modules(QTC_X11 xcb x11-xcb)
    else()
      pkg_check_modules(QTC_X11 xcb)
    endif()
    if(NOT QTC_X11_FOUND AND QTC_ENABLE_X11)
      message(WARNING
        "Cannot find required modules for x11, disable x11 support.")
      set(QTC_ENABLE_X11 Off)
      return()
    endif()
  endmacro()
  qtc_check_x11()
else()
  set(QTC_X11_FOUND Off)
endif()

if(QTC_X11_FOUND)
  include_directories(${QTC_X11_INCLUDE_DIRS})
  add_definitions(${QTC_X11_CFLAGS})
else()
  include_directories(SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/lib/xcb-defs")
endif()

if(ENABLE_GTK2 AND NOT ENABLE_GTK2_DEFINED)
  find_package(PkgConfig)
  if(NOT PKG_CONFIG_FOUND)
    message(WARNING "pkg-config not found, disable Gtk2 build.")
    set(ENABLE_GTK2 Off)
  else()
    unset(__pkg_config_checked_GTK2 CACHE)
    pkg_check_modules(GTK2 gtk+-2.0)
    if(NOT GTK2_FOUND)
      message(WARNING "gtk+-2.0 not found, disable Gtk2 build.")
      set(ENABLE_GTK2 Off)
    endif()
  endif()
endif()

if(ENABLE_GTK2)
  find_package(PkgConfig REQUIRED)
  unset(__pkg_config_checked_GTK2 CACHE)
  pkg_check_modules(GTK2 REQUIRED gtk+-2.0)
  if(GTK2_THEME_PREFIX)
    set(GTK2_PREFIX "${GTK2_THEME_PREFIX}")
  elseif(NOT GTK2_PREFIX)
    set(GTK2_PREFIX "${CMAKE_INSTALL_PREFIX}")
  endif()
  if(NOT GTK2_LIBDIR)
    set(GTK2_LIBDIR "${GTK2_PREFIX}/lib")
  endif()
  _pkgconfig_invoke(gtk+-2.0 GTK2 BIN_VERSION "" --variable=gtk_binary_version)
  set(GTK2_THEME_DIR ${GTK2_PREFIX}/share/themes/QtCurve)
endif()

configure_file(config.h.in config.h @ONLY)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")
cmake_c_fix_include_path(lib/utils qtcurve-utils)
cmake_c_fix_include_path(lib/cairo qtcurve-cairo)
# glibc 2.20 warns about _BSD_SOURCE if _DEFAULT_SOURCE is not defined.
cmake_c_add_flags(CMAKE_C_FLAGS -Wall -Wextra -std=gnu99 -D_DEFAULT_SOURCE)
# Register storage class is deprecated in C++11 but is still used in Qt.
# Use compiler option (-Wno-deprecated-register) to suppress
# the warning in clang++.
if(NOT APPLE)
  # -std=c++0x is deprecated but gcc < 4.7 do not recognise c++11 ....
  cmake_c_add_flags(CMAKE_CXX_FLAGS -Wall -Wextra -std=c++0x -D_DEFAULT_SOURCE)
  cmake_c_add_flags(CMAKE_SHARED_LINKER_FLAGS -Wl,--as-needed -Wl,--no-undefined)
  cmake_c_add_flags(CMAKE_MODULE_LINKER_FLAGS -Wl,--as-needed -Wl,--no-undefined)
else()
  cmake_c_add_flags(CMAKE_CXX_FLAGS -Wall -Wextra
    -Wno-deprecated-register -std=c++11)
endif()
add_definitions("-D_GNU_SOURCE -pthread")

if(NOT DEFINED LIB_INSTALL_DIR)
  set(LIB_INSTALL_DIR lib)
endif()

cmake_c_include_fix_path()
add_subdirectory(lib)
add_subdirectory(qt4)
add_subdirectory(qt5)
add_subdirectory(gtk2)
add_subdirectory(tools)
if(ENABLE_TEST)
  add_subdirectory(test)
endif()

if(ENABLE_GTK2 AND QTC_GTK2_MODIFY_MOZILLA)
  message(WARNING
    "You have enabled the option for QtCurve to modify the configuration "
    "files of Mozilla applications (Firefox, Thunderbird, Seamonkey). "
    "Note: This sometimes causes issues with newer versions of these "
    "applications. Should such issues occur, you should recompile QtCurve "
    "without this option - and you will have to remove BY HAND the changes "
    "that QtCurve has made. This will entail removing lines from "
    "userChrom.css that indicate 'Added by QtCurve' userChrome.css is usually "
    "in a folder such as: \n"
    "`~/.mozilla/firefox/xxxxxxx.default/chrome/userChrome.css` or \n"
    "`~/.thunderbird/xxxxxxx.default/chrome/userChrome.css`. \n"
    "where xxxxxxx is a series of random characters.")
endif()
