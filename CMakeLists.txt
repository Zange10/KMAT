cmake_minimum_required(VERSION 3.26)
project(KSP C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

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
        orbit_calculator/transfer_tools.h)

target_link_libraries(KSP m)
