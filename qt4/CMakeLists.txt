if(ENABLE_QT4 AND NOT ENABLE_QT4_DEFINED)
  # ENABLE_QT4 is not defined on the command line
  find_package(Qt4 4.6.0)
  if(NOT Qt4_FOUND AND NOT QT4_FOUND)
    ## NB: this gets activated on OS X 10.9/MacPorts even when Qt is found,
    ## making it obligatory to define ENABLE_QT4 on the command line
    message(WARNING "Qt4 not found.")
    set(ENABLE_QT4 Off)
    set(ENABLE_QT4 Off PARENT_SCOPE)
  endif()
endif()

if(ENABLE_QT4)
  find_package(Qt4 4.6.0 REQUIRED)
  # search packages used by KDE
  if(QTC_QT4_ENABLE_KDE)
    find_package(KDE4 REQUIRED)
    include(KDE4Defaults)
    set(QTCURVE_STYLE_DIR "${PLUGIN_INSTALL_DIR}/plugins/styles/")
    if(NOT Q_WS_X11 OR NOT QTC_X11_FOUND)
      # Kwin headers uses a lot of xcb types and functions and we cannot
      # provide declarations for all of them even if they are not used.
      if(QTC_QT4_ENABLE_KWIN)
        message(WARNING "KWin4 support require Q_WS_X11 and libxcb headers.")
      endif()
      set(QTC_QT4_ENABLE_KWIN Off)
    elseif(NOT DEFINED QTC_QT4_ENABLE_KWIN)
      find_package(KDE4Workspace)
      set(QTC_QT4_ENABLE_KWIN "${KDE4Workspace_FOUND}")
    elseif(QTC_QT4_ENABLE_KWIN)
      find_package(KDE4Workspace REQUIRED)
    endif()
  else()
    set(QTC_QT4_ENABLE_KWIN Off)
    set(QTCURVE_STYLE_DIR "${QT_PLUGINS_DIR}/styles/")
  endif()
  add_definitions(${QT_DEFINITIONS} -DQTC_UTILS_QT4)
  include_directories(
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${QT_INCLUDE_DIR}")

  if(QTC_QT4_ENABLE_KDE)
    add_definitions(${KDE4_DEFINITIONS} -DQTC_QT4_ENABLE_KDE)
    include_directories(${KDE4_INCLUDES})
  endif()
else()
  set(QTC_QT4_ENABLE_KDE Off)
  set(QTC_QT4_ENABLE_KWIN Off)
endif()

add_subdirectory(colors)
add_subdirectory(common)
add_subdirectory(config)
add_subdirectory(themes)
add_subdirectory(kwin)
add_subdirectory(kwinconfig)
add_subdirectory(style)
