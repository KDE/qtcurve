if(NOT QTC_QT6_ENABLE_KDE)
  return()
endif()

set(kstyle_qtcurve6_config_PART_SRCS
  qtcurveconfig.cpp
  imagepropertiesdialog.cpp
  ../common/common.cpp
  ../common/config_file.cpp
  ../common/kf5_utils.cpp
  ../kwinconfig/qtcurvekwinconfig.cpp
  ../kwin/qtcurveconfig.cpp
  ../kwin/qtcurveshadowconfiguration.cpp)

set(kstyle_qtcurve6_config_export_SRCS
  exportthemedialog.cpp)

set(kstyle_qtcurve6_config_PART_HDRS
  qtcurveconfig.h
  imagepropertiesdialog.h)

set(kstyle_qtcurve6_config_export_HDRS
  exportthemedialog.h)

set(kstyle_qtcurve6_config_UIS
  qtcurveconfigbase.ui
  stylepreview.ui
  imageproperties.ui
  ../kwinconfig/qtcurvekwinconfigwidget.ui)

if(QTC_QT6_STYLE_SUPPORT)
  set(kstyle_qtcurve6_config_PART_SRCS
    ${kstyle_qtcurve6_config_PART_SRCS}
    ${kstyle_qtcurve6_config_export_SRCS})
endif()

translate_add_sources(
  ${kstyle_qtcurve6_config_PART_SRCS}
  ${kstyle_qtcurve6_config_export_SRCS}
  ${kstyle_qtcurve6_config_PART_HDRS}
  ${kstyle_qtcurve6_config_export_HDRS}
  ${kstyle_qtcurve6_config_UIS})

add_definitions(-DCONFIG_WRITE -DCONFIG_DIALOG)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

ki18n_wrap_ui(kstyle_qtcurve6_config_PART_SRCS ${kstyle_qtcurve6_config_UIS})

add_library(kstyle_qtcurve6_config MODULE ${kstyle_qtcurve6_config_PART_SRCS})

target_link_libraries(kstyle_qtcurve6_config Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Svg)
target_link_libraries(kstyle_qtcurve6_config
  KF6::Archive
  KF6::ConfigCore
  KF6::ConfigWidgets
  KF6::I18n
  KF6::GuiAddons
  KF6::IconThemes
  KF6::KIOWidgets
  KF6::WidgetsAddons
  KF6::XmlGui
  KF6::Style
  KF6::WindowSystem)
target_link_libraries(kstyle_qtcurve6_config qtcurve-utils)

install(TARGETS kstyle_qtcurve6_config DESTINATION ${_Qt6_PLUGIN_INSTALL_DIR}/kstyle_config)
install(FILES QtCurveui.rc  DESTINATION ${KDE_INSTALL_KXMLGUIDIR}/QtCurve)
