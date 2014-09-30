set(kwin_qtcurve_config_PART_SRCS
  qtcurvekwinconfig.cpp)
set(kwin_qtcurve_config_PART_HDRS
  qtcurvekwinconfig.h)
set(kwin_qtcurve_config_PART_UIS
  qtcurvekwinconfigwidget.ui)

translate_add_sources(${kwin_qtcurve_config_PART_SRCS}
  ${kwin_qtcurve_config_PART_HDRS}
  ${kwin_qtcurve_config_PART_UIS})

if(NOT QTC_QT4_ENABLE_KWIN)
  return()
endif()

set(kwin_qtcurve_config_PART_SRCS
  ${kwin_qtcurve_config_PART_SRCS}
  ../kwin/qtcurveconfig.cpp
  ../kwin/qtcurveshadowconfiguration.cpp)
kde4_add_ui_files(kwin_qtcurve_config_PART_SRCS
  ${kwin_qtcurve_config_PART_UIS})
kde4_add_plugin(kwin_qtcurve_config_kde4 ${kwin_qtcurve_config_PART_SRCS})
set_target_properties(kwin_qtcurve_config_kde4 PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  OUTPUT_NAME "kwin_qtcurve_config")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(kwin_qtcurve_config_kde4
  ${KDE4_KDEUI_LIBS} ${QT_QTGUI_LIBRARY} qtcurve-utils)
install(TARGETS kwin_qtcurve_config_kde4 DESTINATION ${PLUGIN_INSTALL_DIR})
