if(QTC_QT5_ENABLE_QTQUICK2)
  set(QTC_QT5_MODULES Qt5Core Qt5Gui Qt5Widgets Qt5Svg Qt5Quick Qt5DBus)
else()
  set(QTC_QT5_MODULES Qt5Core Qt5Gui Qt5Widgets Qt5Svg Qt5DBus)
endif()
if(QTC_QT5_ENABLE_KDE)
  set(QTC_QT5_MODULES ${QTC_QT5_MODULES} Qt5PrintSupport)
endif()

if(ENABLE_QT5 AND NOT ENABLE_QT5_DEFINED)
  set(QT_MIN_VERSION "5.3.2")
  # ENABLE_QT5 is not defined on the command line
  foreach(QtModule ${QTC_QT5_MODULES})
    find_package(${QtModule} ${QT_MIN_VERSION} CONFIG)
    if(NOT ${QtModule}_FOUND)
      message(WARNING "Required Qt5 Module ${QtModule} not found."
        "Disabling Qt5 build")
      set(ENABLE_QT5 Off)
      set(ENABLE_QT5 Off PARENT_SCOPE)
      break()
    endif()
  endforeach()
endif()

add_definitions(-DQTC_UTILS_QT5 -DQTC_UTILS_QT)

set(QTC_QT5_ENABLE_KWIN Off)

if(ENABLE_QT5)
  set(QTC_QT5_LINK_LIBS)
  set(QTC_QT5_DEFS)
  set(QTC_QT5_INCLUDES)
  foreach(QtModule ${QTC_QT5_MODULES})
    find_package(${QtModule} REQUIRED)
    set(QTC_QT5_LINK_LIBS ${QTC_QT5_LINK_LIBS} ${${QtModule}_LIBRARIES})
    set(QTC_QT5_DEFS ${QTC_QT5_DEFS} ${${QtModule}_DEFINITIONS})
    set(QTC_QT5_INCLUDES ${QTC_QT5_INCLUDES} ${${QtModule}_INCLUDE_DIRS})
  endforeach()
  if(QTC_ENABLE_X11)
    set(QTC_QT5_OPT_MODULES Qt5X11Extras)
  endif()
  foreach(QtModule ${QTC_QT5_OPT_MODULES})
    find_package(${QtModule})
    if(${QtModule}_FOUND)
      set(QTC_QT5_LINK_LIBS ${QTC_QT5_LINK_LIBS} ${${QtModule}_LIBRARIES})
      set(QTC_QT5_DEFS ${QTC_QT5_DEFS} ${${QtModule}_DEFINITIONS})
      set(QTC_QT5_INCLUDES ${QTC_QT5_INCLUDES} ${${QtModule}_INCLUDE_DIRS})
    endif()
  endforeach()
  get_target_property(REAL_QMAKE_EXECUTABLE ${Qt5Core_QMAKE_EXECUTABLE}
    IMPORTED_LOCATION)
  execute_process(COMMAND "${REAL_QMAKE_EXECUTABLE}" -query QT_INSTALL_PLUGINS
    OUTPUT_VARIABLE _Qt5_PLUGIN_INSTALL_DIR
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(QTCURVE_STYLE_DIR "${_Qt5_PLUGIN_INSTALL_DIR}/styles/")
  include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}")
  add_definitions(-DTRANSLATION_DOMAIN=\"qtcurve\" ${QTC_QT5_DEFS})
  include_directories(${QTC_QT5_INCLUDES})
  configure_file(config-qt5.h.in config-qt5.h @ONLY)

  if(QTC_QT5_ENABLE_KDE)
    find_package(ECM 1.2.0 REQUIRED NO_MODULE)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

    find_package(KF5 REQUIRED COMPONENTS
      Archive
      Config
      ConfigWidgets
      FrameworkIntegration
      I18n
      GuiAddons
      KIO
      IconThemes
      WidgetsAddons
      WindowSystem
      XmlGui)

    add_definitions(-DQTC_QT5_ENABLE_KDE)
    include(KDEInstallDirs)
    include(KDECMakeSettings)
    include(KDECompilerSettings)
    if(NOT APPLE)
      set(QTC_QT5_ENABLE_KWIN ON)
    endif()
  endif()
else()
  set(QTC_QT5_ENABLE_KDE Off)
endif()

add_subdirectory(common)
add_subdirectory(config)
if(QTC_QT5_ENABLE_KWIN)
  add_subdirectory(kwin)
  add_subdirectory(kwinconfig)
endif()
add_subdirectory(style)
