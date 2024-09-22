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
        tools/csv_writer.c
        tools/csv_writer.h
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
        ephem.c
        ephem.h
        tools/thread_pool.c
        tools/thread_pool.h
        gui/transfer_app.c
        gui/transfer_app.h
        gui/drawing.c
        gui/drawing.h
        tools/thread_pool.h
        orbit_calculator/double_swing_by.c
        orbit_calculator/double_swing_by.h
        database/database.c
        database/database.h
        gui/database_app.c
        gui/database_app.h
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
        gui/transfer_app/transfer_calculator.c
        gui/transfer_app/transfer_calculator.h
        launch_calculator/launch_sim.c
        launch_calculator/launch_sim.h
        launch_calculator/launch_state.c
        launch_calculator/launch_state.h
        gui/launch_app.c
        gui/launch_app.h
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
)

target_link_libraries(KSP m;
        ${GTK3_LIBRARIES};
        sqlite3)

target_link_options(KSP PRIVATE -rdynamic)
