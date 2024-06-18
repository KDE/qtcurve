set(CMAKE_AUTOMOC True)
set(CMAKE_AUTOUIC True)

include(CMakePrintHelpers)

if(QTC_QT6_ENABLE_QTQUICK2)
  set(QTC_QT6_MODULES Core Gui Widgets Svg Quick DBus)
else()
  set(QTC_QT6_MODULES Core Gui Widgets Svg DBus)
endif()
if(QTC_QT6_ENABLE_KDE)
  list(APPEND QTC_QT6_MODULES PrintSupport)
endif()

add_definitions(-DQTC_UTILS_QT6 -DQTC_UTILS_QT)

set(QTC_QT6_ENABLE_KWIN Off)

find_package(Qt6 REQUIRED COMPONENTS ${QTC_QT6_MODULES})
list(TRANSFORM QTC_QT6_MODULES PREPEND "Qt6::" OUTPUT_VARIABLE QTC_QT6_LINK_LIBS)
if(QTC_ENABLE_X11)
  list(APPEND QTC_QT6_LINK_LIBS Qt6::CorePrivate)
endif()
foreach(QtModule ${QTC_QT6_OPT_MODULES})
  find_package(Qt6${QtModule})
  if(Qt6${QtModule}_FOUND)
    list(APPEND QTC_QT6_LINK_LIBS "Qt6::${QtModule}")
  endif()
endforeach()

cmake_print_variables(QTC_QT6_MODULES QTC_QT6_LINK_LIBS)

get_target_property(REAL_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
execute_process(COMMAND "${REAL_QMAKE_EXECUTABLE}" -query QT_INSTALL_PLUGINS
  OUTPUT_VARIABLE _Qt6_PLUGIN_INSTALL_DIR
  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
set(QTCURVE_STYLE_DIR "${_Qt6_PLUGIN_INSTALL_DIR}/styles/")

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_BINARY_DIR}")
add_definitions(-DTRANSLATION_DOMAIN=\"qtcurve\")
configure_file(config-qt6.h.in config-qt6.h @ONLY)

if(QTC_QT6_ENABLE_KDE)
  find_package(ECM 1.2.0 REQUIRED NO_MODULE)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

  find_package(KF6 REQUIRED COMPONENTS
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

  add_definitions(-DQTC_QT6_ENABLE_KDE)
  include(KDEInstallDirs)
  include(KDECMakeSettings)
  include(KDECompilerSettings)
  if(NOT APPLE)
    set(QTC_QT6_ENABLE_KWIN ON)
  endif()
endif()

add_subdirectory(common)
add_subdirectory(config)
if(QTC_QT6_ENABLE_KWIN)
  add_subdirectory(kwin)
  add_subdirectory(kwinconfig)
endif()
add_subdirectory(style)
