#include <stdio.h>

#include "launch_calculator.h"
#include "orbit_calculator.h"

// ------------------------------------------------------------

int main() {
    int selection = 0;
    do {
    printf("\n\nCHOOSE PROGRAM:\n _______________________________\n\n| - 0 = Exit\t\t\t|\n| - 1 = Launch Calculator\t|\n| - 2 = Orbit Calculator\t|\n _______________________________\n\nProgram: ");
    scanf("%d", &selection);

    switch (selection) {
    case 1:
        // Launch Calculator
        calculate_launch();
        break;
    case 2:
        // Orbit Calculator
        choose_calculation();
        break;
    default:
        break;
    }
    } while(selection != 0);
    return 0;
}