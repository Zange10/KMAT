#include <stdio.h>
#include <math.h>
#include "orbit_calculator.h"

#define MU_EARTH 3.98574405e14
#define EARTHRADIUS 6371000

struct Orbit {
    double apoapsis;
    double periapsis;
    double a;
};

void print_orbit_info(struct Orbit o) {
    printf("\n______________________\nVESSEL:\n\n");
    printf("Apoapsis:\t\t%g km\n", (o.apoapsis-EARTHRADIUS)/1000);
    printf("Periapsis:\t\t%g km\n", (o.periapsis-EARTHRADIUS)/1000);
    printf("Semi-major axis:\t%g km\n", o.a /1000);
    printf("______________________\n\n");
}

void choose_calculation() {
    int selection = 0;
    printf("Choose Calculation (1=change Apsis): ");
    scanf("%d", &selection);

    double dV = 0;

    switch (selection)
    {
    case 1:
        dV = change_apsis();
        break;
    default:
        break;
    }

    printf("\n________\n\nNeeded Delta-V: %g\n__________\n\n\n", dV);
}

double change_apsis() {
    double initial_apsis = 0;
    double new_apsis = 0;
    double static_apsis = 0;

    printf("Set parameters (opposing Apsis, initial value of apsis, new value of apsis): ");
    scanf("%lf %lf %lf", &static_apsis, &initial_apsis, &new_apsis);

    struct Orbit initial_orbit;
    struct Orbit new_orbit;

    if(static_apsis > initial_apsis) {
        initial_orbit.apoapsis = static_apsis*1000+EARTHRADIUS;
        initial_orbit.periapsis = initial_apsis*1000+EARTHRADIUS;
    } else {
        initial_orbit.apoapsis = initial_apsis*1000+EARTHRADIUS;
        initial_orbit.periapsis = static_apsis*1000+EARTHRADIUS;
    }
    initial_orbit.a = calc_semimajor_axis(initial_orbit);

    if(static_apsis > new_apsis) {
        new_orbit.apoapsis = static_apsis*1000+EARTHRADIUS;
        new_orbit.periapsis = new_apsis*1000+EARTHRADIUS;
    } else {
        new_orbit.apoapsis = new_apsis*1000+EARTHRADIUS;
        new_orbit.periapsis = static_apsis*1000+EARTHRADIUS;
    }
    new_orbit.a = calc_semimajor_axis(new_orbit);

    print_orbit_info(initial_orbit);
    print_orbit_info(new_orbit);

    double v0 = calc_orbital_speed(static_apsis, initial_orbit.a);
    double v1 = calc_orbital_speed(static_apsis, new_orbit.a);

    return fabs(v1-v0);
}

double calc_semimajor_axis(struct Orbit o) {
    return (o.apoapsis+o.periapsis)/2;
}

double calc_orbital_speed(double altitude, double a) {
    double r = altitude + EARTHRADIUS;
    double v2 = MU_EARTH * (2/r - 1/a);
    return sqrt(v2);
}