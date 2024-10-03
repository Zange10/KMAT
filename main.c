#include "celestial_bodies.h"
#include "launch_calculator/launch_calculator.h"
#include "orbit_calculator/orbit_calculator.h"
#include "tools/tool_funcs.h"
#include "gui/transfer_app.h"
#include "gui/launch_app.h"
#include "database/database.h"
#include "tools/gmat_interface.h"


// ------------------------------------------------------------

int main() {
    init_celestial_bodies();
	write_gmat_script();

	return 0;

    int selection;
    char title[] = "CHOOSE PROGRAM:";
    char options[] = "Exit; Launch Calculator; Orbit Calculator; Transfer Calculator; Launch App";
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
			start_transfer_app();
			break;
		case 4:
			init_db();
			start_launch_app();
			close_db();
			break;
        default:
            break;
        }
    } while(selection != 0);
    return 0;
}