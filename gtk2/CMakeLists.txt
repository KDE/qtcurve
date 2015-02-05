add_definitions(-DQTC_UTILS_GTK2 -DQTC_UTILS_GTK)

if(ENABLE_GTK2)
  include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}")
endif()

add_subdirectory(common)
add_subdirectory(style)
add_subdirectory(mozilla)
