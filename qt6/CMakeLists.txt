set(CMAKE_AUTOMOC True)
set(CMAKE_AUTOUIC True)

include(CMakePrintHelpers)

set(QTC_QT6_KF6_COMPONENTS
  Archive
  Config
  ConfigWidgets
  FrameworkIntegration
  I18n
  GuiAddons
  KIO
  IconThemes
  WidgetsAddons
  WindowSystem
  XmlGui)

# Keep this block above the Qt 6 module list, because the PrintSupport append
# below reads the decision.
if(QTC_QT6_ENABLE_KDE AND NOT QTC_QT6_ENABLE_KDE_DEFINED)
  find_package(ECM 1.2.0 QUIET NO_MODULE)
  if(NOT ECM_FOUND)
    set(_QTC_KF6_MISSING ECM)
  else()
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})
    find_package(KF6 COMPONENTS ${QTC_QT6_KF6_COMPONENTS})
    set(_QTC_KF6_MISSING)
    foreach(_QTC_KF6_COMPONENT ${QTC_QT6_KF6_COMPONENTS})
      if(NOT KF6_${_QTC_KF6_COMPONENT}_FOUND)
        list(APPEND _QTC_KF6_MISSING KF6${_QTC_KF6_COMPONENT})
      endif()
    endforeach()
  endif()
  if(_QTC_KF6_MISSING)
    string(REPLACE ";" " " _QTC_KF6_MISSING "${_QTC_KF6_MISSING}")
    message(WARNING
      "${_QTC_KF6_MISSING} not found. Disabling KDE support for the Qt6 style")
    set(QTC_QT6_ENABLE_KDE Off)
  endif()
endif()

if(QTC_QT6_ENABLE_QTQUICK2)
  set(QTC_QT6_MODULES Core Gui GuiPrivate Widgets WidgetsPrivate Svg Quick DBus)
else()
  set(QTC_QT6_MODULES Core Gui GuiPrivate Widgets WidgetsPrivate Svg DBus)
endif()
if(QTC_QT6_ENABLE_KDE)
  list(APPEND QTC_QT6_MODULES PrintSupport)
endif()

add_definitions(-DQTC_UTILS_QT6 -DQTC_UTILS_QT)

set(QTC_QT6_ENABLE_KWIN Off)

find_package(Qt6 REQUIRED COMPONENTS ${QTC_QT6_MODULES})
list(TRANSFORM QTC_QT6_MODULES PREPEND "Qt6::" OUTPUT_VARIABLE QTC_QT6_LINK_LIBS)
foreach(QtModule ${QTC_QT6_OPT_MODULES})
  find_package(Qt6${QtModule})
  if(Qt6${QtModule}_FOUND)
    list(APPEND QTC_QT6_LINK_LIBS "Qt6::${QtModule}")
  endif()
endforeach()

cmake_print_variables(QTC_QT6_MODULES QTC_QT6_LINK_LIBS)

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_BINARY_DIR}")
add_definitions(-DTRANSLATION_DOMAIN=\"qtcurve\")
configure_file(config-qt6.h.in config-qt6.h @ONLY)

if(QTC_QT6_ENABLE_KDE)
  # Without a value of QTC_QT6_ENABLE_KDE from the configure, the block at the
  # top of this file found both packages.
  if(QTC_QT6_ENABLE_KDE_DEFINED)
    find_package(ECM 1.2.0 REQUIRED NO_MODULE)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

    find_package(KF6 REQUIRED COMPONENTS ${QTC_QT6_KF6_COMPONENTS})
  endif()

  add_definitions(-DQTC_QT6_ENABLE_KDE)
  include(KDEInstallDirs)
  include(KDECMakeSettings)
  include(KDECompilerSettings)
  if(NOT APPLE)
    set(QTC_QT6_ENABLE_KWIN ON)
  endif()
endif()

get_target_property(REAL_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
execute_process(COMMAND "${REAL_QMAKE_EXECUTABLE}" -query QT_INSTALL_PLUGINS
  OUTPUT_VARIABLE _Qt6_QUERY_PLUGIN_DIR
  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "${REAL_QMAKE_EXECUTABLE}" -query QT_INSTALL_PREFIX
  OUTPUT_VARIABLE _Qt6_QUERY_PREFIX
  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
# Keep this block below include(KDEInstallDirs), which writes
# KDE_INSTALL_FULL_QTPLUGINDIR and the libdir. A block above the include reads
# the libdir of GNUInstallDirs, which can name another directory. A build
# without KDE support leaves the variable empty, and qmake or the libdir then
# gives the default.
if(KDE_INSTALL_FULL_QTPLUGINDIR)
  set(QTC_QT6_PLUGIN_DEFAULT "${KDE_INSTALL_FULL_QTPLUGINDIR}")
elseif(_Qt6_QUERY_PREFIX STREQUAL CMAKE_INSTALL_PREFIX)
  set(QTC_QT6_PLUGIN_DEFAULT "${_Qt6_QUERY_PLUGIN_DIR}")
else()
  set(QTC_QT6_PLUGIN_DEFAULT "${CMAKE_INSTALL_FULL_LIBDIR}/plugins")
endif()
qtc_option(QTC_QT6_PLUGIN_DIR "Absolute directory of the Qt6 plugins."
  "${QTC_QT6_PLUGIN_DEFAULT}")
set(_Qt6_PLUGIN_INSTALL_DIR "${QTC_QT6_PLUGIN_DIR}")
set(QTCURVE_STYLE_DIR "${_Qt6_PLUGIN_INSTALL_DIR}/styles/")

add_subdirectory(common)
add_subdirectory(config)
if(QTC_QT6_ENABLE_KWIN)
  add_subdirectory(kwin)
  add_subdirectory(kwinconfig)
endif()
add_subdirectory(style)

# qt5 configures after this directory and reads the two final values.
set(QTC_QT6_ENABLE_KDE "${QTC_QT6_ENABLE_KDE}" PARENT_SCOPE)
set(_Qt6_PLUGIN_INSTALL_DIR "${_Qt6_PLUGIN_INSTALL_DIR}" PARENT_SCOPE)
