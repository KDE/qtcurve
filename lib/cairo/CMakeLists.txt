set(qtcurve_cairo_SRCS
  utils.cpp
  draw.cpp)

if(NOT ENABLE_GTK2 AND NOT ENABLE_GTK3)
  return()
endif()

find_package(PkgConfig REQUIRED)
unset(__pkg_config_checked_CAIRO CACHE)
pkg_check_modules(CAIRO
  REQUIRED cairo
  REQUIRED pangocairo)
include_directories(${CAIRO_INCLUDE_DIRS})
set(qtcurve_cairo_LINKS qtcurve-utils ${CAIRO_LDFLAGSF} ${CAIRO_LIBRARIES})
add_definitions("-DQTC_UTILS_INTERNAL" ${CAIRO_CFLAGS})

add_library(qtcurve-cairo SHARED ${qtcurve_cairo_SRCS})
target_link_libraries(qtcurve-cairo ${qtcurve_cairo_LINKS})

set_target_properties(qtcurve-cairo PROPERTIES
  VERSION 0.1
  SOVERSION 0
  COMPILE_FLAGS "-fvisibility=hidden"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
install(TARGETS qtcurve-cairo LIBRARY DESTINATION "${LIB_INSTALL_DIR}")
