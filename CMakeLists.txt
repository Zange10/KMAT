cmake_minimum_required(VERSION 3.26)
project(KSP C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})


add_executable(KSP
        celestial_bodies.c
        celestial_bodies.h
        csv_writer.c
        csv_writer.h
        launch_calculator/launch_calculator.c
        launch_calculator/launch_calculator.h
        launch_calculator/lv_profile.c
        launch_calculator/lv_profile.h
        main.c
        orbit_calculator/orbit.c
        orbit_calculator/orbit.h
        orbit_calculator/orbit_calculator.c
        orbit_calculator/orbit_calculator.h
        tool_funcs.c
        tool_funcs.h
        launch_calculator/launch_circularization.c
        launch_calculator/launch_circularization.h
        launch_calculator/lp_parameters.c
        launch_calculator/lp_parameters.h
        orbit_calculator/transfer_calculator.c
        orbit_calculator/transfer_calculator.h
        analytic_geometry.c
        analytic_geometry.h
        orbit_calculator/transfer_tools.c
        orbit_calculator/transfer_tools.h
        launch_calculator/lp_parameters.h
        ephem.c
        ephem.h
        orbit_calculator/porkchop_tools.c
        orbit_calculator/porkchop_tools.h
        thread_pool.c
        thread_pool.h
        gui/transfer_app.c
        gui/transfer_app.h
        gui/drawing.c
        gui/drawing.h
        thread_pool.h
        orbit_calculator/double_swing_by.c
        orbit_calculator/double_swing_by.h)

target_link_libraries(KSP m;
        ${GTK3_LIBRARIES})

target_link_options(KSP PRIVATE -rdynamic)
