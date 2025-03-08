#include "celestial_bodies.h"
#include "launch_calculator/launch_calculator.h"
#include "orbit_calculator/orbit_calculator.h"
#include "tools/tool_funcs.h"
#include "gui/gui_manager.h"
#include "database/database.h"


// ------------------------------------------------------------

int main() {
    init_celestial_bodies();
	init_db();

	start_gui();

    int selection;
    char title[] = "CHOOSE PROGRAM:";
    char options[] = "Exit; Launch Calculator; Orbit Calculator; Transfer Calculator; Launch App; DB Test";
    char question[] = "Program: ";

    do {
        selection = user_selection(title, options, question);

        switch (selection) {
        case 1:
            launch_calculator();
            break;
        case 2:
            orbit_calculator();
            break;
		case 3:
//			start_transfer_app();
			break;
		case 4:
//			start_launch_app();
			break;
		case 5:
			start_gui();
			break;
        default:
            break;
        }
    } while(selection != 0);
	close_db();
    return 0;
}