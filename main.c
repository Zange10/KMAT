#include "celestial_bodies.h"
#include "launch_calculator/launch_calculator.h"
#include "orbit_calculator/orbit_calculator.h"
#include "orbit_calculator/transfer_calculator.h"
#include "tool_funcs.h"


// ------------------------------------------------------------

int main() {
    init_celestial_bodies();

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
            create_transfer();
            break;
        default:
            break;
        }
    } while(selection != 0);
    return 0;
}