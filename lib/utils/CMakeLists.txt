set(qtcurve_utils_SRCS
  color.c
  dirs.c
  log.c
  utils.c
  strs.c
  shadow.c
  timer.c
  map.c
  fd_utils.c
  process.c
  x11utils.c
  x11helpers.c
  x11wrap.c)
if(APPLE)
set(qtcurve_utils_LINKS
  ${LIBEXECINFO_LIBRARIES} pthread)
else()
set(qtcurve_utils_LINKS
  m rt dl ${LIBEXECINFO_LIBRARIES} pthread)
endif(APPLE)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

if(QTC_ENABLE_X11)
  set(qtcurve_utils_LINKS
    ${qtcurve_utils_LINKS}
    ${QTC_X11_LIBRARIES})
endif()

add_definitions("-DQTC_UTILS_INTERNAL -pthread")

add_library(qtcurve-utils SHARED
  ${qtcurve_utils_SRCS})

target_link_libraries(qtcurve-utils ${qtcurve_utils_LINKS})

set_target_properties(qtcurve-utils PROPERTIES
  VERSION 1.0
  SOVERSION 1
  COMPILE_FLAGS "-fvisibility=hidden"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
install(TARGETS qtcurve-utils LIBRARY
  DESTINATION "${LIB_INSTALL_DIR}")
