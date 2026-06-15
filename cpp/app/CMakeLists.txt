find_package(Qt6 COMPONENTS Widgets QUIET)

if(Qt6_FOUND)
  qt_standard_project_setup()

  qt_add_executable(duplexprint_qt
    MANUAL_FINALIZATION
    MACOSX_BUNDLE
    main.cpp
    main_window.cpp
    main_window.hpp
  )

  target_link_libraries(duplexprint_qt
    PRIVATE
      Qt6::Widgets
      duplexprint_core
      duplexprint_cloud
      duplexprint_platform
  )

  set_target_properties(duplexprint_qt PROPERTIES
    OUTPUT_NAME "help-me-print"
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "Help Me Print"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.easure.helpmeprint"
  )

  install(TARGETS duplexprint_qt
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
  )

  include(GNUInstallDirs)
  qt_generate_deploy_app_script(
    TARGET duplexprint_qt
    OUTPUT_SCRIPT deploy_qt_app_script
    NO_UNSUPPORTED_PLATFORM_ERROR
  )
  install(SCRIPT "${deploy_qt_app_script}")

  qt_finalize_executable(duplexprint_qt)
else()
  message(STATUS "Qt6 Widgets not found; skipping native UI target.")
endif()
