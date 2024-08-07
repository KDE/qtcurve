set(qtcurve_utils_SRCS
  color.cpp
  dirs.cpp
  log.cpp
  utils.cpp
  strs.cpp
  shadow.cpp
  timer.cpp
  options.cpp
  fd_utils.cpp
  process.cpp
  # DO NOT condition on QTC_ENABLE_X11 !!!
  # These provides dummy API functions so that x and non-x version are abi
  # compatible. There's no X11 linkage when QTC_ENABLE_X11 is off even though
  # these files are compiled
  x11utils.cpp
  x11helpers.cpp
  x11wrap.cpp)

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
  "${CMAKE_BINARY_DIR}/version.cpp")
set_source_files_properties("${CMAKE_BINARY_DIR}/version.cpp"
  PROPERTIES GENERATED 1)

add_dependencies(qtcurve-utils gen-git-ver)

target_link_libraries(qtcurve-utils ${qtcurve_utils_LINKS})

set_target_properties(qtcurve-utils PROPERTIES
  VERSION 2.0
  SOVERSION 2
  COMPILE_FLAGS "-fvisibility=hidden"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  OUTPUT_NAME "qtcurve-utils${QTC_UTILSLIB_INFIX}")
install(TARGETS qtcurve-utils LIBRARY
  DESTINATION "${CMAKE_INSTALL_LIBDIR}")
