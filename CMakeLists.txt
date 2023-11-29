## Setup
cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(QtCurve)

include(FeatureSummary)

## Temporary
function(translate_add_sources)
endfunction()

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CMakeCMacros)

## Version
set(QTCURVE_VERSION_MAJOR "1")
set(QTCURVE_VERSION_MINOR "9")
set(QTCURVE_VERSION_PATCH "0-rc1")
set(QTCURVE_VERSION "${QTCURVE_VERSION_MAJOR}.${QTCURVE_VERSION_MINOR}")
set(QTCURVE_VERSION_FULL "${QTCURVE_VERSION}.${QTCURVE_VERSION_PATCH}")

find_package(Git QUIET)
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/version.cpp"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen-version.sh"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
add_custom_target(gen-git-ver ALL
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen-version.sh"
  "${CMAKE_CURRENT_BINARY_DIR}/version.cpp" "${QTCURVE_VERSION_FULL}"
  "${GIT_EXECUTABLE}"
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/version.cpp")

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
qtc_option(ENABLE_QT6 "Building Qt6 theme." On)
qtc_option(ENABLE_GTK2 "Building Gtk2 theme." On)
qtc_option(QTC_QT6_ENABLE_KDE "Building Qt6 style with KDE Frameworks 6 support." Off)
# option(ENABLE_GTK3 "Building Gtk3 theme." On)
option(BUILD_TESTING "Enable testing." On)
option(QTC_ENABLE_X11 "Enable X11" On)
option(QTC_INSTALL_PO "Install translation files." On)

if(ENABLE_QT4)
    qtc_option(QTC_QT4_ENABLE_KDE "Building Qt4 style with KDE4 support." On)
else()
    qtc_option(QTC_QT4_ENABLE_KDE "Building Qt4 style with KDE4 support." Off)
endif()
if(ENABLE_QT5)
    qtc_option(QTC_QT5_ENABLE_KDE
      "Building Qt5 style with KDE Frameworks 5 support." On)
else()
    qtc_option(QTC_QT5_ENABLE_KDE
      "Building Qt5 style with KDE Frameworks 5 support." Off)
endif()

option(QTC_QT4_STYLE_SUPPORT
  "Qt4 support for creating a style for each QtCurve theme." Off)

## Common settings.
# support an infix argument to have different qtcurve-utils libraries, e.g. for
# GTk2, Qt4 and Qt5
qtc_option(QTC_UTILSLIB_INFIX "Infix for having multiple qtcurve-utils's" "")
qtc_option(QTC_KDE4_DEFAULT_HOME
  "Path of kde4 home (relative to user home), for reading old theme files only"
  ".kde4")

if((QTC_KDE4_DEFAULT_HOME STREQUAL ".kde4") OR
    (QTC_KDE4_DEFAULT_HOME STREQUAL ".kde") OR
    (QTC_KDE4_DEFAULT_HOME STREQUAL ".kde4/") OR
    (QTC_KDE4_DEFAULT_HOME STREQUAL ".kde/") OR
    (QTC_KDE4_DEFAULT_HOME STREQUAL ""))
  set(QTC_KDE4_DEFAULT_HOME_DEFAULT True)
else()
  set(QTC_KDE4_DEFAULT_HOME_DEFAULT False)
endif()

# set compiler flags earlier to prevent potential failure running find_package(KDE4)
# glibc 2.20 warns about _BSD_SOURCE if _DEFAULT_SOURCE is not defined.
cmake_c_add_flags(CMAKE_C_FLAGS -Wall -Wextra -Wno-cast-align
  -std=gnu99 -D_DEFAULT_SOURCE)
# Register storage class is deprecated in C++11 but is still used in Qt.
# Use compiler option (-Wno-deprecated-register) to suppress
# the warning in clang++.
# We use `override` in certain Qt Classes but the QObject macro does not use it
# Use compiler option (-Wno-inconsistent-missing-override) to suppress
# the warning in clang++ 3.6+.
cmake_c_add_flags(CMAKE_CXX_FLAGS -Wall -Wextra
  -Wno-deprecated-register -Wno-cast-align
  -Wno-inconsistent-missing-override)
if(NOT APPLE)
  # -std=c++0x is deprecated but gcc < 4.7 do not recognise c++11 ....
  cmake_c_add_flags(CMAKE_CXX_FLAGS -std=c++17 -D_DEFAULT_SOURCE)
  cmake_c_add_flags(CMAKE_SHARED_LINKER_FLAGS -Wl,--as-needed -Wl,--no-undefined)
  cmake_c_add_flags(CMAKE_MODULE_LINKER_FLAGS -Wl,--as-needed -Wl,--no-undefined)
else()
  cmake_c_add_flags(CMAKE_CXX_FLAGS -std=c++11)
endif()

if(QTC_QT4_ENABLE_KDE)
    find_package(KDE4 QUIET)
    set(KDE4_KDECONFIG_EXECUTABLE)
    find_program(KDE4_KDECONFIG_EXECUTABLE NAMES kde4-config
      PATHS "${CMAKE_INSTALL_PREFIX}/bin" ${_KDEDIRS} /opt/kde4/bin
      NO_DEFAULT_PATH)
    if(NOT KDE4_KDECONFIG_EXECUTABLE)
      find_program(KDE4_KDECONFIG_EXECUTABLE NAMES kde4-config)
    endif()
endif()

function(get_cmd_var varname def cmd)
  if(DEFINED ${varname})
    return()
  endif()
  if(cmd)
    execute_process(COMMAND "${cmd}" ${ARGN} OUTPUT_VARIABLE _cmd_out
      RESULT_VARIABLE _cmd_res)
    if("${_cmd_res}" EQUAL 0)
      string(STRIP "${_cmd_out}" _cmd_out)
    else()
      set(_cmd_out "${def}")
    endif()
  else()
    set(_cmd_out "${def}")
  endif()
  set("${varname}" "${_cmd_out}" PARENT_SCOPE)
endfunction()

function(get_kde_var var def)
  get_cmd_var(_output "${def}" "${KDE4_KDECONFIG_EXECUTABLE}" ${ARGN})
  set(${var} "${_output}" PARENT_SCOPE)
endfunction()

get_kde_var(QTC_KDE4_PREFIX "${CMAKE_INSTALL_PREFIX}" --prefix)
get_kde_var(QTC_KDE4_ICONS_PREFIX "${QTC_KDE4_PREFIX}/share/icons"
  --install icon)

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
add_definitions("-D_GNU_SOURCE -pthread")

if(NOT DEFINED LIB_INSTALL_DIR)
  set(LIB_INSTALL_DIR lib)
endif()

cmake_c_include_fix_path()
if(ENABLE_QT4 OR ENABLE_QT5 OR ENABLE_GTK2)
    add_subdirectory(lib)
    add_subdirectory(tools)
endif()
if(ENABLE_QT4)
    add_subdirectory(qt4)
endif()
if(ENABLE_QT5)
    add_subdirectory(qt5)
endif()
if(ENABLE_QT6)
    add_subdirectory(qt6)
endif()

if(ENABLE_GTK2)
    add_subdirectory(gtk2)
endif()
if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(test)
endif()

if(QTC_INSTALL_PO)
    find_package(ECM 1.2.0 REQUIRED NO_MODULE)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

    find_package(KF5 REQUIRED COMPONENTS
      I18n)
    ki18n_install(po)
endif()
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
