find_package(Qt6 COMPONENTS Widgets QUIET)

if(Qt6_FOUND)
  qt_standard_project_setup()

  qt_add_executable(duplexprint_qt
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
else()
  message(STATUS "Qt6 Widgets not found; skipping native UI target.")
endif()
