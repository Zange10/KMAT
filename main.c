#include "celestial_bodies.h"
#include "launch_calculator/launch_calculator.h"
#include "orbit_calculator/orbit_calculator.h"
#include "orbit_calculator/transfer_calculator.h"
#include "tool_funcs.h"
#include "gui/window.h"


// ------------------------------------------------------------

int main() {
    init_celestial_bodies();
	struct Window win = create_window(500, 500);
	struct Window win2 = create_window(500, 500);
	return 0;
    if(0)simple_transfer();
    else create_swing_by_transfer();
    return 0;

    int selection;
    char title[] = "CHOOSE PROGRAM:";
    char options[] = "Exit; Launch Calculator; Orbit Calculator; Transfer Calculator";
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
        default:
            break;
        }
    } while(selection != 0);
    return 0;
}