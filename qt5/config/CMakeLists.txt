if(NOT QTC_QT5_ENABLE_KDE)
  return()
endif()

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)

set(kstyle_qtcurve5_config_PART_SRCS
  qtcurveconfig.cpp
  imagepropertiesdialog.cpp
  ../common/common.cpp
  ../common/config_file.cpp
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

target_link_libraries(kstyle_qtcurve5_config Qt5::Core Qt5::Gui Qt5::Widgets)
target_link_libraries(kstyle_qtcurve5_config
  KF5::Archive
  KF5::I18n
  KF5::KDELibs4Support
  KF5::KIOWidgets
  KF5::WidgetsAddons
  KF5::XmlGui)
target_link_libraries(kstyle_qtcurve5_config qtcurve-utils)

install(TARGETS kstyle_qtcurve5_config DESTINATION ${QT_PLUGIN_INSTALL_DIR})
