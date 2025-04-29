#include "celestial_bodies.h"
#include "launch_calculator/launch_calculator.h"
#include "orbit_calculator/orbit_calculator.h"
#include "tools/tool_funcs.h"
#include "gui/gui_manager.h"
#include "00_Testing/testing.h"
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
//	init_db();

//	test();
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