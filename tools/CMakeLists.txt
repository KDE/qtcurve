if(ENABLE_QT4 OR ENABLE_QT5)
  set(qt_dump_png_SRCS
    qt_dump_png.cpp)
  add_executable(qt-dump-png ${qt_dump_png_SRCS})
endif()
