set(kstyle_qtcurve_config_PART_SRCS
  qtcurveconfig.cpp
  imagepropertiesdialog.cpp)
set(kstyle_qtcurve_config_export_SRCS
  exportthemedialog.cpp)
set(kstyle_qtcurve_config_PART_HDRS
  qtcurveconfig.h
  imagepropertiesdialog.h)
set(kstyle_qtcurve_config_export_HDRS
  exportthemedialog.h)
set(kstyle_qtcurve_config_UIS
  qtcurveconfigbase.ui
  stylepreview.ui
  imageproperties.ui)
translate_add_sources(
  ${kstyle_qtcurve_config_PART_SRCS}
  ${kstyle_qtcurve_config_export_SRCS}
  ${kstyle_qtcurve_config_PART_HDRS}
  ${kstyle_qtcurve_config_export_HDRS}
  ${kstyle_qtcurve_config_UIS}
  QtCurveui.rc)

if(NOT QTC_QT4_ENABLE_KDE)
  return()
endif()

set(kstyle_qtcurve_config_common_SRCS
  ../common/common.cpp
  ../common/config_file.cpp)

set(kstyle_qtcurve_config_PART_SRCS
  ${kstyle_qtcurve_config_PART_SRCS}
  ../kwinconfig/qtcurvekwinconfig.cpp
  ../kwin/qtcurveconfig.cpp
  ../kwin/qtcurveshadowconfiguration.cpp
  ${kstyle_qtcurve_config_common_SRCS})

if(QTC_QT4_STYLE_SUPPORT)
  set(kstyle_qtcurve_config_PART_SRCS
    ${kstyle_qtcurve_config_PART_SRCS}
    ${kstyle_qtcurve_config_export_SRCS})
endif()

add_definitions(-DCONFIG_WRITE -DCONFIG_DIALOG)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")
kde4_add_ui_files(kstyle_qtcurve_config_PART_SRCS
  ${kstyle_qtcurve_config_UIS}
  ../kwinconfig/qtcurvekwinconfigwidget.ui)
kde4_add_plugin(kstyle_qtcurve_config_kde4
  ${kstyle_qtcurve_config_PART_SRCS})
set_target_properties(kstyle_qtcurve_config_kde4 PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  OUTPUT_NAME "kstyle_qtcurve_config")
target_link_libraries(kstyle_qtcurve_config_kde4
  ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS}
  ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY}
  qtcurve-utils)
install(TARGETS kstyle_qtcurve_config_kde4
  DESTINATION "${PLUGIN_INSTALL_DIR}")
install(FILES QtCurveui.rc DESTINATION "${DATA_INSTALL_DIR}/QtCurve")
