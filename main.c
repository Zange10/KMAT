#include "celestial_bodies.h"
#include "launch_calculator/launch_calculator.h"
#include "orbit_calculator/orbit_calculator.h"
#include "orbit_calculator/transfer_calculator.h"
#include "tools/tool_funcs.h"
#include "gui/transfer_app.h"


// ------------------------------------------------------------

int main() {
    init_celestial_bodies();
	create_itinerary();
	return 0;

    int selection;
    char title[] = "CHOOSE PROGRAM:";
    char options[] = "Exit; Launch Calculator; Orbit Calculator; Transfer Calculator; Transfer App";
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
            create_swing_by_transfer();
            break;
		case 4:
			start_transfer_app();
			break;
        default:
            break;
        }
    } while(selection != 0);
    return 0;
}