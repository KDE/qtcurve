set(qtcurve_SRCS
  animation.cpp
  combobox.cpp
  drawing.cpp
  entry.cpp
  helpers.cpp
  menu.cpp
  pixcache.cpp
  qtcurve.c
  qt_settings.c
  scrollbar.c
  scrolledwindow.c
  shadowhelper.c
  tab.cpp
  treeview.cpp
  widgetmap.cpp
  window.cpp
  wmmove.cpp)
set(qtcurve_HDRS
  animation.h
  combobox.h
  compatability.h
  drawing.h
  entry.h
  helpers.h
  menu.h
  pixcache.h
  qt_settings.h
  qtcurve.h
  scrollbar.h
  scrolledwindow.h
  shadowhelper.h
  tab.h
  treeview.h
  widgetmap.h
  window.h
  wmmove.h)

translate_add_sources(${qtcurve_SRCS} ${qtcurve_HDRS})

if(NOT ENABLE_GTK2)
  return()
endif()

set(qtcurve_style_common_SRCS
  ../common/common.cpp
  ../common/config_file.cpp)

include_directories(
  ${GTK2_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR})

add_definitions(${GTK2_CFLAGS})

set(GDK_PIXBUF_CSRC)
find_program(GDK_PIXBUF_CSRC gdk-pixbuf-csource)
if(NOT GDK_PIXBUF_CSRC)
  message(FATAL_ERROR "Cannot find gdk-pixbuf-csource.")
endif()

foreach(pngname check_on check_x_on blank16x16)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${pngname}-png.h"
    COMMAND "${GDK_PIXBUF_CSRC}" --static --raw "--name=${pngname}"
    "${PROJECT_SOURCE_DIR}/data/${pngname}.png"
    > "${CMAKE_CURRENT_BINARY_DIR}/${pngname}-png.h"
    DEPENDS "${GDK_PIXBUF_CSRC}" "${PROJECT_SOURCE_DIR}/data/${pngname}.png")
  add_custom_target(qtc_gtk2_${pngname}_hdr
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${pngname}-png.h")
endforeach()

set(qtcurve_SRCS ${qtcurve_SRCS} ${qtcurve_style_common_SRCS})
add_library(qtcurve-gtk2 MODULE ${qtcurve_SRCS})

set_target_properties(qtcurve-gtk2 PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  OUTPUT_NAME "qtcurve")
add_dependencies(qtcurve-gtk2 qtc_gtk2_check_on_hdr qtc_gtk2_check_x_on_hdr
  qtc_gtk2_blank16x16_hdr)

# The following fixes odd segfaults with evolution under Fedora 14.
# See https://bugzilla.redhat.com/show_bug.cgi?id=676837
add_definitions(-Dopts=qtcurve_gtk2_opts)

target_link_libraries(qtcurve-gtk2
  ${GTK2_LDFLAGS}
  ${GTK2_LIBRARIES}
  qtcurve-utils qtcurve-cairo m)

install(TARGETS qtcurve-gtk2 LIBRARY DESTINATION
  ${GTK2_LIBDIR}/gtk-2.0/${GTK2_BIN_VERSION}/engines)
install(PROGRAMS map_kde_icons.pl DESTINATION ${GTK2_THEME_DIR}/gtk-2.0)
install(FILES gtkrc icons3 icons4 kdeglobals
  DESTINATION ${GTK2_THEME_DIR}/gtk-2.0)
