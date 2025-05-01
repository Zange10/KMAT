cmake_minimum_required(VERSION 3.26)
project(KSP C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(KSP-Tools
        celestial_bodies.c
        celestial_bodies.h
        tools/file_io.c
        tools/file_io.h
        launch_calculator/launch_calculator.c
        launch_calculator/launch_calculator.h
        launch_calculator/lv_profile.c
        launch_calculator/lv_profile.h
        main.c
        orbit_calculator/orbit.c
        orbit_calculator/orbit.h
        orbit_calculator/orbit_calculator.c
        orbit_calculator/orbit_calculator.h
        tools/tool_funcs.c
        tools/tool_funcs.h
        launch_calculator/launch_circularization.c
        launch_calculator/launch_circularization.h
        launch_calculator/lp_parameters.c
        launch_calculator/lp_parameters.h
        orbit_calculator/transfer_calc.c
        orbit_calculator/transfer_calc.h
        tools/analytic_geometry.c
        tools/analytic_geometry.h
        orbit_calculator/transfer_tools.c
        orbit_calculator/transfer_tools.h
        launch_calculator/lp_parameters.h
        tools/ephem.c
        tools/ephem.h
        tools/thread_pool.c
        tools/thread_pool.h
        gui/drawing.c
        gui/drawing.h
        tools/thread_pool.h
        orbit_calculator/double_swing_by.c
        orbit_calculator/double_swing_by.h
        database/database.c
        database/database.h
        orbit_calculator/double_swing_by.h
        tools/data_tool.c
        tools/data_tool.h
        orbit_calculator/itin_tool.c
        orbit_calculator/itin_tool.h
        gui/transfer_app/transfer_planner.c
        gui/transfer_app/transfer_planner.h
        gui/transfer_app/porkchop_analyzer.c
        gui/transfer_app/porkchop_analyzer.h
        gui/transfer_app/porkchop_analyzer_tools.c
        gui/transfer_app/porkchop_analyzer_tools.h
        gui/transfer_app/sequence_calculator.c
        gui/transfer_app/sequence_calculator.h
        launch_calculator/launch_sim.c
        launch_calculator/launch_sim.h
        launch_calculator/launch_state.c
        launch_calculator/launch_state.h
        gui/launch_app/launch_analyzer.c
        gui/launch_app/launch_analyzer.h
        database/lv_database.c
        database/lv_database.h
        gui/launch_app/capability_analyzer.c
        gui/launch_app/capability_analyzer.h
        launch_calculator/capability_calculator.c
        launch_calculator/capability_calculator.h
        gui/launch_app/launch_parameter_analyzer.c
        gui/launch_app/launch_parameter_analyzer.h
        gui/database_app/mission_db.c
        gui/database_app/mission_db.h
        database/mission_database.c
        database/mission_database.h
        gui/css_loader.c
        gui/css_loader.h
        gui/database_app/database_app_tools/mission_db_tools.c
        gui/database_app/database_app_tools/mission_db_tools.h
        gui/database_app/database_app_tools/mission_manager_tools.c
        gui/database_app/database_app_tools/mission_manager_tools.h
        tools/gmat_interface.c
        tools/gmat_interface.h
        orbit_calculator/itin_dsb_tool.c
        orbit_calculator/itin_dsb_tool.h
        gui/transfer_app/itinerary_calculator.c
        gui/transfer_app/itinerary_calculator.h
        gui/gui_manager.c
        gui/gui_manager.h
        gui/info_win_manager.c
        gui/info_win_manager.h
        tools/datetime.c
        tools/datetime.h
        gui/settings.c
        gui/settings.h
        00_Testing/testing.c
        00_Testing/testing.h
        00_Testing/testing_algorithms.c
        00_Testing/testing_algorithms.h
        projection_test/projection_test.c
        projection_test/projection_test.h
)

# Platform-specific setup
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
        message(STATUS "Configuring Cross-Compilation for Windows (MinGW-w64)")
        set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
        set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)

        # Ensure MinGW headers are used
        include_directories(/usr/x86_64-w64-mingw32/include)
        link_directories(/usr/x86_64-w64-mingw32/lib)

        # GTK3 installation directory (modify if needed)
        set(GTK3_ROOT "/usr/x86_64-w64-mingw32/gtk")  # Change to actual GTK3 install path
    else()
        message(STATUS "Configuring for Windows (native MinGW)")
        set(GTK3_ROOT "C:/msys64/ucrt64")
    endif ()

    include_directories(
            ${GTK3_ROOT}/include
            ${GTK3_ROOT}/include/gtk-3.0
            ${GTK3_ROOT}/include/glib-2.0
            ${GTK3_ROOT}/lib/glib-2.0/include
            ${GTK3_ROOT}/include/pango-1.0
            ${GTK3_ROOT}/include/cairo
            ${GTK3_ROOT}/include/gdk-pixbuf-2.0
            ${GTK3_ROOT}/include/atk-1.0
            ${GTK3_ROOT}/include/harfbuzz
    )

    # Explicit GTK3 library directories
    link_directories(
            ${GTK3_ROOT}/lib
            ${GTK3_ROOT}/lib/gtk-3.0
            ${GTK3_ROOT}/lib/glib-2.0
    )

    target_link_libraries(KSP-Tools
            gtk-3
            gdk-3
            gobject-2.0
            glib-2.0
            pango-1.0
            cairo
            gdk_pixbuf-2.0
            atk-1.0
            gio-2.0
            urlmon
            sqlite3
    )


    set_target_properties(KSP-Tools PROPERTIES WIN32_EXECUTABLE TRUE)

else(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Configuring for Linux (GCC)")

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

    include_directories(${GTK3_INCLUDE_DIRS})
    link_directories(${GTK3_LIBRARY_DIRS})
    add_definitions(${GTK3_CFLAGS_OTHER})
    target_link_libraries(KSP-Tools m ${GTK3_LIBRARIES} sqlite3)

    target_link_options(KSP-Tools PRIVATE -rdynamic)
endif()