find_package(Qt6 COMPONENTS Widgets Pdf PrintSupport Network QUIET)

if(Qt6_FOUND)
  qt_standard_project_setup()

  set(app_icon_png "${CMAKE_SOURCE_DIR}/assets/icons/png/icon-256.png")
  set(app_icon_icns "${CMAKE_SOURCE_DIR}/assets/icons/help-me-print.icns")
  set(app_icon_windows_rc "${CMAKE_SOURCE_DIR}/packaging/windows/help-me-print.rc")
  set(app_sources
    main.cpp
    main_window.cpp
    main_window.hpp
    qt_document_service.cpp
    qt_document_service.hpp
    qt_easure_profile_repository.cpp
    qt_easure_profile_repository.hpp
    qt_pdf_service.cpp
    qt_pdf_service.hpp
    qt_print_service.cpp
    qt_print_service.hpp
    qt_printer_service.cpp
    qt_printer_service.hpp
  )

  if(WIN32)
    list(APPEND app_sources "${app_icon_windows_rc}")
  endif()
  if(APPLE)
    list(APPEND app_sources "${app_icon_icns}")
    set_source_files_properties("${app_icon_icns}" PROPERTIES
      MACOSX_PACKAGE_LOCATION "Resources"
    )
  endif()

  set_source_files_properties("${app_icon_png}" PROPERTIES
    QT_RESOURCE_ALIAS "app-icon-256.png"
  )

  qt_add_executable(duplexprint_qt
    MANUAL_FINALIZATION
    MACOSX_BUNDLE
    ${app_sources}
  )

  qt_add_resources(duplexprint_qt "duplexprint_icons"
    PREFIX "/icons"
    FILES
      "${app_icon_png}"
  )

  target_link_libraries(duplexprint_qt
    PRIVATE
      Qt6::Widgets
      Qt6::Pdf
      Qt6::PrintSupport
      Qt6::Network
      duplexprint_core
      duplexprint_cloud
      duplexprint_platform
  )

  set_target_properties(duplexprint_qt PROPERTIES
    OUTPUT_NAME "help-me-print"
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "Help Me Print"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.easure.helpmeprint"
    MACOSX_BUNDLE_ICON_FILE "help-me-print.icns"
  )

  install(TARGETS duplexprint_qt
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
  )

  if(UNIX AND NOT APPLE)
    install(FILES "${CMAKE_SOURCE_DIR}/packaging/linux/help-me-print.desktop"
      DESTINATION share/applications
    )
    install(FILES "${app_icon_png}"
      DESTINATION share/icons/hicolor/256x256/apps
      RENAME help-me-print.png
    )
  endif()

  if(DUPLEXPRINT_DEPLOY_QT_RUNTIME)
    include(GNUInstallDirs)
    qt_generate_deploy_app_script(
      TARGET duplexprint_qt
      OUTPUT_SCRIPT deploy_qt_app_script
      NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT "${deploy_qt_app_script}")
  endif()

  qt_finalize_executable(duplexprint_qt)
else()
  if(DUPLEXPRINT_REQUIRE_QT_APP)
    message(FATAL_ERROR "Qt6 Widgets/Pdf/PrintSupport/Network not found; cannot create duplexprint_qt target.")
  endif()
  message(STATUS "Qt6 Widgets/Pdf/PrintSupport/Network not found; skipping native UI target.")
endif()
