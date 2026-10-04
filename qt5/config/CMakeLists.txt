if(NOT QTC_QT5_ENABLE_KDE)
  return()
endif()

# By default the module stays off the plugin path of a Qt 6 session.
set(_Qt5_CONFIG_INSTALL_DIR "${_Qt5_PLUGIN_INSTALL_DIR}")
if(NOT QTC_QT5_PLUGIN_DIR_DEFINED AND
    NOT _Qt5_QUERY_PREFIX STREQUAL CMAKE_INSTALL_PREFIX)
  set(_Qt5_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/qt5/plugins")
endif()
if(_QTC_QT6_KDE)
  get_filename_component(_QTC_QT6_DIR "${_Qt6_PLUGIN_INSTALL_DIR}" ABSOLUTE
    BASE_DIR "${CMAKE_INSTALL_PREFIX}")
  foreach(_QTC_DIR "${_Qt5_PLUGIN_INSTALL_DIR}" "${_Qt5_CONFIG_INSTALL_DIR}")
    get_filename_component(_QTC_DIR "${_QTC_DIR}" ABSOLUTE
      BASE_DIR "${CMAKE_INSTALL_PREFIX}")
    if(_QTC_DIR STREQUAL _QTC_QT6_DIR)
      return()
    endif()
  endforeach()
endif()

set(kstyle_qtcurve5_config_PART_SRCS
  qtcurveconfig.cpp
  imagepropertiesdialog.cpp
  ../common/common.cpp
  ../common/config_file.cpp
  ../common/kf5_utils.cpp
  ../kwinconfig/qtcurvekwinconfig.cpp
  ../kwin/qtcurveconfig.cpp
  ../kwin/qtcurveshadowconfiguration.cpp)

set(kstyle_qtcurve5_config_export_SRCS
  exportthemedialog.cpp)

set(kstyle_qtcurve5_config_PART_HDRS
  qtcurveconfig.h
  imagepropertiesdialog.h)

set(kstyle_qtcurve5_config_export_HDRS
  exportthemedialog.h)

set(kstyle_qtcurve5_config_UIS
  qtcurveconfigbase.ui
  stylepreview.ui
  imageproperties.ui
  ../kwinconfig/qtcurvekwinconfigwidget.ui)

if(QTC_QT5_STYLE_SUPPORT)
  set(kstyle_qtcurve5_config_PART_SRCS
    ${kstyle_qtcurve5_config_PART_SRCS}
    ${kstyle_qtcurve5_config_export_SRCS})
endif()

translate_add_sources(
  ${kstyle_qtcurve5_config_PART_SRCS}
  ${kstyle_qtcurve5_config_export_SRCS}
  ${kstyle_qtcurve5_config_PART_HDRS}
  ${kstyle_qtcurve5_config_export_HDRS}
  ${kstyle_qtcurve5_config_UIS})

add_definitions(-DCONFIG_WRITE -DCONFIG_DIALOG)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

ki18n_wrap_ui(kstyle_qtcurve5_config_PART_SRCS ${kstyle_qtcurve5_config_UIS})

add_library(kstyle_qtcurve5_config MODULE ${kstyle_qtcurve5_config_PART_SRCS})
# The themerc files of both dialects name this file.
set_target_properties(kstyle_qtcurve5_config PROPERTIES
  OUTPUT_NAME "kstyle_qtcurve_config")

target_link_libraries(kstyle_qtcurve5_config Qt5::Core Qt5::Gui Qt5::Widgets)
target_link_libraries(kstyle_qtcurve5_config
  KF5::Archive
  KF5::ConfigCore
  KF5::ConfigWidgets
  KF5::I18n
  KF5::GuiAddons
  KF5::IconThemes
  KF5::KIOWidgets
  KF5::WidgetsAddons
  KF5::XmlGui
  KF5::Style
  KF5::WindowSystem)
target_link_libraries(kstyle_qtcurve5_config qtcurve-utils)

install(TARGETS kstyle_qtcurve5_config DESTINATION ${_Qt5_CONFIG_INSTALL_DIR})
if(NOT _QTC_QT6_KDE)
  install(FILES QtCurveui.rc  DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/QtCurve)
endif()
