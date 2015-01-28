set(kwin3_qtcurve_PART_SRCS
  qtcurvehandler.cpp
  qtcurveclient.cpp
  qtcurvebutton.cpp
  qtcurvesizegrip.cpp
  qtcurveshadowcache.cpp
  qtcurveconfig.cpp
  qtcurveshadowconfiguration.cpp
  tileset.cpp
  qtcurvetogglebutton.cpp)
set(kwin3_qtcurve_PART_HDRS
  qtcurvehandler.h
  qtcurveclient.h
  qtcurvebutton.h
  qtcurvedbus.h
  qtcurvesizegrip.h
  qtcurveshadowcache.h
  qtcurveconfig.h
  qtcurveshadowconfiguration.h
  tileset.h
  qtcurvetogglebutton.h)

translate_add_sources(${kwin3_qtcurve_PART_SRCS}
  ${kwin3_qtcurve_PART_HDRS} qtcurve.desktop)

if(NOT QTC_QT4_ENABLE_KWIN)
  return()
endif()

include_directories(
  "${CMAKE_CURRENT_BINARY_DIR}"
  ${QT_QTGUI_INCLUDE_DIR}
  ${QT_QTCORE_INCLUDE_DIR}
  ${KDE4WORKSPACE_INCLUDE_DIR})
link_directories(${KDE4WORKSPACE_LIB_DIR})
kde4_add_plugin(kwin3_qtcurve_kde4 ${kwin3_qtcurve_PART_SRCS})
set_target_properties(kwin3_qtcurve_kde4 PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  OUTPUT_NAME "kwin3_qtcurve")
target_link_libraries(kwin3_qtcurve_kde4 kdecorations
  ${KDE4_KDEUI_LIBS} qtcurve-utils)
install(TARGETS kwin3_qtcurve_kde4 DESTINATION "${PLUGIN_INSTALL_DIR}")
install(FILES qtcurve.desktop DESTINATION "${DATA_INSTALL_DIR}/kwin/")
