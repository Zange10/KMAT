cmake_minimum_required(VERSION 3.26)
project(KSP C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(KSP
        celestial_bodies.c
        celestial_bodies.h
        csv_writer.c
        csv_writer.h
        launch_calculator.c
        launch_calculator.h
        lv_profile.c
        lv_profile.h
        main.c
        orbit.c
        orbit.h
        orbit_calculator.c
        orbit_calculator.h
        tool_funcs.c
        tool_funcs.h
        launch_circularization.c
        launch_circularization.h
        lp_parameters.c
        lp_parameters.h)

target_link_libraries(KSP m)
