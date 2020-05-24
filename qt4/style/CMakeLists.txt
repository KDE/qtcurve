set(qtcurve_SRCS
  qtcurve.cpp
  qtcurve_plugin.cpp
  windowmanager.cpp
  blurhelper.cpp
  prepolish.cpp
  utils.cpp
  shortcuthandler.cpp
  argbhelper.cpp
  macmenu.cpp
  shadowhelper.cpp)
set(qtcurve_MOC_HDRS
  qtcurve.h
  qtcurve_p.h
  qtcurve_plugin.h
  windowmanager.h
  blurhelper.h
  shortcuthandler.h
  argbhelper.h
  macmenu.h
  macmenu-dbus.h
  shadowhelper.h)

translate_add_sources(${qtcurve_SRCS} ${qtcurve_MOC_HDRS})

if(NOT ENABLE_QT4)
  return()
endif()

set(qtcurve_style_common_SRCS
  ../common/common.cpp
  ../common/config_file.cpp)

foreach(pngname check_on check_x_on dialog_error dialog_information
    dialog_warning)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${pngname}-png.h"
    COMMAND "${PROJECT_BINARY_DIR}/tools/qt-dump-png"
    "${PROJECT_SOURCE_DIR}/data/${pngname}.png" "qtc_${pngname}"
    "${CMAKE_CURRENT_BINARY_DIR}/${pngname}-png.h"
    DEPENDS qt-dump-png "${PROJECT_SOURCE_DIR}/data/${pngname}.png")
  add_custom_target(qtc_qt4_${pngname}_hdr
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${pngname}-png.h")
endforeach()

set(qtcurve_SRCS
  ${qtcurve_SRCS}
  ${qtcurve_style_common_SRCS})

add_definitions(-DQT_PLUGIN)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories(SYSTEM
  "${QT_QTGUI_INCLUDE_DIR}"
  "${QT_QTCORE_INCLUDE_DIR}"
  "${QT_QTDBUS_INCLUDE_DIR}"
  "${QT_QTSVG_INCLUDE_DIR}")
qt4_wrap_cpp(qtcurve_MOC_SRCS ${qtcurve_MOC_HDRS})
add_library(qtcurve-qt4 MODULE ${qtcurve_SRCS} ${qtcurve_MOC_SRCS})
add_dependencies(qtcurve-qt4 qtc_qt4_check_on_hdr qtc_qt4_check_x_on_hdr)
set_target_properties(qtcurve-qt4 PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  OUTPUT_NAME "qtcurve"
  PREFIX "")

if(QTC_QT4_ENABLE_KDE)
  install(FILES qtcurve.themerc DESTINATION ${DATA_INSTALL_DIR}/kstyle/themes)
  set(QTC_KDE_LIBS ${KDE4_KDEUI_LIBS})
  if(NOT QTC_DISABLE_KDEFILEDIALOG_CALLS)
    set(QTC_KFILEDIALOG_LIBS ${KDE4_KIO_LIBS} ${KDE4_KDECORE_LIBS})
  endif()
else()
  add_dependencies(qtcurve-qt4 qtc_qt4_dialog_error_hdr
    qtc_qt4_dialog_information_hdr
    qtc_qt4_dialog_warning_hdr)
endif()

target_link_libraries(qtcurve-qt4
  ${QTC_KDE_LIBS}
  ${QTC_KFILEDIALOG_LIBS}
  ${QT_QTGUI_LIBRARY}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTSVG_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  qtcurve-utils)

if(QTC_QT4_ENABLE_KDE)
  install(TARGETS qtcurve-qt4 DESTINATION ${QTCURVE_STYLE_DIR})
else()
  install(TARGETS qtcurve-qt4 LIBRARY DESTINATION ${QTCURVE_STYLE_DIR})
endif()
