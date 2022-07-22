#include <stdio.h>

#include "launch_calculator.h"
#include "orbit_calculator.h"

// ------------------------------------------------------------

int main() {
    int selection = 0;
    do {
    printf("Choose Program (0=Exit; 1=Launch Calculator; 2=Orbit Calculator): ");
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