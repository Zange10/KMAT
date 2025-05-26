#include "celestial_bodies.h"
#include "launch_calculator/launch_calculator.h"
#include "orbit_calculator/orbit_calculator.h"
#include "tools/tool_funcs.h"
#include "gui/gui_manager.h"
#include "00_Testing/testing.h"
#include "_System_Config_Creator/ksp_connector/ksp_connector.h"
#include "orbit_calculator/transfer_tools.h"
//#include "database/database.h"
#ifdef _WIN32
#include <windows.h>  // for SetPriorityClass(), SetThreadPriority()
#endif


// ------------------------------------------------------------

void test();

void set_low_priority() {
	// Low thread priority
	#ifdef _WIN32
		if (!SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS)) {
			printf("Failed to set priority on Windows\n");
		}
	#else
		int current_nice = nice(10);  // Increase the nice value (lower priority)
		if (current_nice == -1) {
			perror("Failed to set nice value");
		}
	#endif
}

int main() {
	set_low_priority();

    init_celestial_bodies();
	init_available_systems("../Celestial_Systems/");


	if (!open_connection()) return 1;

	// double vec[3];
	// receive_data(vec);

	close_connection();


	for (int i = 0; i < 1e3; i++) {
		printf("%d\n", i);
	}

//	init_db();


	// struct System *system = get_system_by_name("SYSTEM");
	// struct System *system1 = get_system_by_name("SYSTEM1");
	// print_date(convert_JD_date(system->ut0, DATE_ISO), 1);
	//
	// struct OSV osv0 = osv_from_elements(system->bodies[2]->orbit, 2433647.500000, system);
	// struct Plane ecliptic = constr_plane(vec(0,0,0), osv0.r, osv0.v);
	//
	// for (int i = 0; i < system->num_bodies; i++) {
	// 	struct Body *body = system->bodies[i];
	// 	struct OSV osv_body = osv_from_elements(body->orbit, 0, system);
	// 	struct Plane orbital_plane = constr_plane(vec(0,0,0), osv_body.r, osv_body.v);
	// 	struct Plane eq_plane = constr_plane(vec(0,0,0), body->pm_ut0, cross_product(body->rot_axis, body->pm_ut0));
	//
	// 	double inclination = rad2deg(angle_plane_plane(ecliptic, orbital_plane));
	// 	double axial_tilt = rad2deg(angle_plane_plane(orbital_plane, eq_plane));
	//
	// 	double angle_pm = rad2deg(angle_vec_vec(body->pm_ut0, osv_body.r));
	//
	//
	// 	struct Body *body1 = system1->bodies[i];
	// 	double rot_axis_angle_diff = rad2deg(angle_vec_vec(body->rot_axis, body1->rot_axis));
	// 	double prime_meridian_angle_diff = rad2deg(angle_vec_vec(body->pm_ut0, body1->pm_ut0));
	//
	// 	printf("Inclination: %f\n", inclination);
	// }

	// test();

	start_gui("../GUI/GUI.glade");

//    int selection;
//    char title[] = "CHOOSE PROGRAM:";
//    char options[] = "Exit; Launch Calculator; Orbit Calculator; GUI; Test";
//    char question[] = "Program: ";
//
//    do {
//        selection = user_selection(title, options, question);
//
//        switch (selection) {
//        case 1:
//            launch_calculator();
//            break;
//        case 2:
//            orbit_calculator();
//            break;
//		case 3:
//			start_gui();
//			break;
//		case 4:
//			test();
//			break;
//        default:
//            break;
//        }
//    } while(selection != 0);
//	close_db();
	free_all_celestial_systems();
    return 0;
}

void test() {
	run_tests();
};