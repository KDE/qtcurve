set(qtcurve_utils_SRCS
  color.cpp
  dirs.c
  log.c
  utils.c
  strs.c
  shadow.cpp
  timer.cpp
  map.c
  fd_utils.c
  process.c
  # DO NOT condition on QTC_ENABLE_X11 !!!
  # These provides dummy API functions so that x and non-x version are abi
  # compatible. There's no X11 linkage when QTC_ENABLE_X11 is off even though
  # these files are compiled
  x11utils.cpp
  x11helpers.cpp
  x11wrap.c)

set(qtcurve_utils_LINKS ${LIBEXECINFO_LIBRARIES} pthread)

if(NOT APPLE)
  set(qtcurve_utils_LINKS ${qtcurve_utils_LINKS} m rt dl)
endif()

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

if(QTC_ENABLE_X11)
  set(qtcurve_utils_LINKS ${qtcurve_utils_LINKS} ${QTC_X11_LIBRARIES})
endif()

add_definitions("-DQTC_UTILS_INTERNAL -pthread")

add_library(qtcurve-utils SHARED ${qtcurve_utils_SRCS}
  "${CMAKE_BINARY_DIR}/version.c")
set_source_files_properties("${CMAKE_BINARY_DIR}/version.c"
  PROPERTIES GENERATED 1)

add_dependencies(qtcurve-utils gen-git-ver)

target_link_libraries(qtcurve-utils ${qtcurve_utils_LINKS})

set_target_properties(qtcurve-utils PROPERTIES
  VERSION 1.0
  SOVERSION 1
  COMPILE_FLAGS "-fvisibility=hidden"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  OUTPUT_NAME "qtcurve-utils${QTC_UTILSLIB_INFIX}")
install(TARGETS qtcurve-utils LIBRARY
  DESTINATION "${LIB_INSTALL_DIR}")
