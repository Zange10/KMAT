#include "celestial_bodies.h"
#include "launch_calculator/launch_calculator.h"
#include "orbit_calculator/orbit_calculator.h"
#include "tools/tool_funcs.h"
#include "gui/gui_manager.h"
#include "database/database.h"


// ------------------------------------------------------------

void test();

int main() {
    init_celestial_bodies();
	init_db();
//	test();
	start_gui();

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
	close_db();
    return 0;
}



void test() {
//	store_system_in_config_file(get_current_system());
}