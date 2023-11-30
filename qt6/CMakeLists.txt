if(QTC_QT6_ENABLE_QTQUICK2)
  set(QTC_QT6_MODULES Qt6Core Qt6Gui Qt6Widgets Qt6Svg Qt6Quick Qt6DBus)
else()
  set(QTC_QT6_MODULES Qt6Core Qt6Gui Qt6Widgets Qt6Svg Qt6DBus)
endif()
if(QTC_QT6_ENABLE_KDE)
  set(QTC_QT6_MODULES ${QTC_QT6_MODULES} Qt6PrintSupport)
endif()

if(ENABLE_QT6 AND NOT ENABLE_QT6_DEFINED)
  set(QT_MIN_VERSION "5.3.2")
  # ENABLE_QT6 is not defined on the command line
  foreach(QtModule ${QTC_QT6_MODULES})
    find_package(${QtModule} ${QT_MIN_VERSION} CONFIG)
    if(NOT ${QtModule}_FOUND)
      message(WARNING "Required Qt6 Module ${QtModule} not found."
        "Disabling Qt6 build")
      set(ENABLE_QT6 Off)
      set(ENABLE_QT6 Off PARENT_SCOPE)
      break()
    endif()
  endforeach()
endif()

add_definitions(-DQTC_UTILS_QT6 -DQTC_UTILS_QT)

set(QTC_QT6_ENABLE_KWIN Off)

if(ENABLE_QT6)
  set(QTC_QT6_LINK_LIBS)
  set(QTC_QT6_DEFS)
  set(QTC_QT6_INCLUDES)
  foreach(QtModule ${QTC_QT6_MODULES})
    find_package(${QtModule} REQUIRED)
    set(QTC_QT6_LINK_LIBS ${QTC_QT6_LINK_LIBS} ${${QtModule}_LIBRARIES})
    set(QTC_QT6_DEFS ${QTC_QT6_DEFS} ${${QtModule}_DEFINITIONS})
    set(QTC_QT6_INCLUDES ${QTC_QT6_INCLUDES} ${${QtModule}_INCLUDE_DIRS})
  endforeach()
  if(QTC_ENABLE_X11)
    set(QTC_QT6_OPT_MODULES Qt::CorePrivate)
  endif()
  foreach(QtModule ${QTC_QT6_OPT_MODULES})
    find_package(${QtModule})
    if(${QtModule}_FOUND)
      set(QTC_QT6_LINK_LIBS ${QTC_QT6_LINK_LIBS} ${${QtModule}_LIBRARIES})
      set(QTC_QT6_DEFS ${QTC_QT6_DEFS} ${${QtModule}_DEFINITIONS})
      set(QTC_QT6_INCLUDES ${QTC_QT6_INCLUDES} ${${QtModule}_INCLUDE_DIRS})
    endif()
  endforeach()
  get_target_property(REAL_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
  execute_process(COMMAND "${REAL_QMAKE_EXECUTABLE}" -query QT_INSTALL_PLUGINS
    OUTPUT_VARIABLE _Qt6_PLUGIN_INSTALL_DIR
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(QTCURVE_STYLE_DIR "${_Qt6_PLUGIN_INSTALL_DIR}/styles/")
  include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}")
  add_definitions(-DTRANSLATION_DOMAIN=\"qtcurve\" ${QTC_QT6_DEFS})
  include_directories(${QTC_QT6_INCLUDES})
  configure_file(config-qt6.h.in config-qt6.h @ONLY)

  if(QTC_QT6_ENABLE_KDE)
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

    add_definitions(-DQTC_QT6_ENABLE_KDE)
    include(KDEInstallDirs)
    include(KDECMakeSettings)
    include(KDECompilerSettings)
    if(NOT APPLE)
        set(QTC_QT6_ENABLE_KWIN ON)
    endif()
  endif()
else()
  set(QTC_QT6_ENABLE_KDE Off)
endif()

add_subdirectory(common)
add_subdirectory(config)
if(QTC_QT6_ENABLE_KWIN)
    add_subdirectory(kwin)
    add_subdirectory(kwinconfig)
endif()
add_subdirectory(style)
