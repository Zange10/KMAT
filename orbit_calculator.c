#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include "orbit_calculator.h"

#define MU_EARTH 3.986e14
#define EARTHRADIUS 6371000

struct Orbit {
    double apoapsis;
    double periapsis;
    double a;
    double inclination;
};

struct ManeuverPlan {
    double dV1;
    double dV2;
    bool first_raise_Apo;
};

struct Orbit construct_orbit(double apsis1, double apsis2, double inclination) {
    struct Orbit new_orbit;
    if(apsis1 > apsis2) {
        new_orbit.apoapsis = apsis1;
        new_orbit.periapsis = apsis2;
    } else {
        new_orbit.apoapsis = apsis2;
        new_orbit.periapsis = apsis1;
    }
    new_orbit.a = (new_orbit.apoapsis+new_orbit.periapsis)/2;
    new_orbit.inclination = inclination;
    return new_orbit;
}

void print_orbit_info(struct Orbit orbit) {
    printf("\n______________________\nORBIT:\n\n");
    printf("Apoapsis:\t\t%g km\n", (orbit.apoapsis-EARTHRADIUS)/1000);
    printf("Periapsis:\t\t%g km\n", (orbit.periapsis-EARTHRADIUS)/1000);
    printf("Semi-major axis:\t%g km\n", orbit.a /1000);
    printf("Inclination:\t\t%g°\n", orbit.inclination);
    printf("______________________\n\n");
}

void choose_calculation() {
    int selection = 0;
    printf("Choose Calculation (1=change Apsis from circular orbit; 2=change Apsis, 3=Hohmann transfer): ");
    scanf("%d", &selection);

    switch (selection)
    {
    case 1:
        change_apsis_circ();
        break;
    case 2:
        change_apsis();
        break;
    case 3:
        calc_hohmann_transfer();
        break;
    default:
        break;
    }
}

void change_apsis_circ() {
    double initial_apsis = 0;
    double new_apsis = 0;

    printf("Enter parameters (altitude of initial circular orbit, new value of apsis): ");
    scanf("%lf %lf", &initial_apsis, &new_apsis);

    initial_apsis = initial_apsis*1000+EARTHRADIUS;
    new_apsis = new_apsis*1000+EARTHRADIUS;

    double dV = calc_maneuver_dV(initial_apsis, initial_apsis, new_apsis);

    printf("\n____________________\n\nNeeded dV: %g m/s\n____________________\n\n", dV);

    return;
}

void change_apsis() {
    double initial_apsis = 0;
    double new_apsis = 0;
    double static_apsis = 0;

    printf("Enter parameters (static Apsis, initial value of apsis, new value of apsis): ");
    scanf("%lf %lf %lf", &static_apsis, &initial_apsis, &new_apsis);

    static_apsis = static_apsis*1000+EARTHRADIUS;
    initial_apsis = initial_apsis*1000+EARTHRADIUS;
    new_apsis = new_apsis*1000+EARTHRADIUS;

    double dV = calc_maneuver_dV(static_apsis, initial_apsis, new_apsis);

    printf("\n____________\n\nNeeded dV: %g m/s\n____________\n\n", dV);

    return;
}

void calc_hohmann_transfer() {
    double initial_apsis = 0;
    double new_apsis = 0;

    printf("Enter parameters (altitude of initial circular orbit, altitude of planned circular orbit): ");
    scanf("%lf %lf", &initial_apsis, &new_apsis);
    
    initial_apsis = initial_apsis*1000+EARTHRADIUS;
    new_apsis = new_apsis*1000+EARTHRADIUS;

    struct Orbit initial_orbit;
    struct Orbit new_orbit;

    initial_orbit = construct_orbit(initial_apsis, initial_apsis, 0);
    new_orbit = construct_orbit(new_apsis, new_apsis, 0);

    print_orbit_info(initial_orbit);
    print_orbit_info(new_orbit);

    struct ManeuverPlan mp = calc_change_orbit_dV(initial_orbit, new_orbit);

    if(mp.first_raise_Apo) {
        printf("\n____________\n\nNeeded Delta-V to raise Apoapsis: \t%g m/s\n", mp.dV1);
        printf("Needed Delta-V to raise Periapsis: \t%g m/s\n", mp.dV2);
        printf("Total Delta-V for Hohmann transfer: \t%g m/s\n____________\n\n", mp.dV1+mp.dV2);
    } else {
        printf("\n____________\n\nNeeded Delta-V to lower Periapsis: \t%g m/s\n", mp.dV1);
        printf("Needed Delta-V to lower Apoapsis: \t%g m/s\n", mp.dV2);
        printf("Total Delta-V for Hohmann transfer: \t%g m/s\n____________\n\n", mp.dV1+mp.dV2);
    }

    return;
}


struct ManeuverPlan calc_change_orbit_dV(struct Orbit initial_orbit, struct Orbit planned_orbit) {
    struct ManeuverPlan mp;
    if(planned_orbit.apoapsis > initial_orbit.apoapsis) {
        mp.first_raise_Apo = true;
        mp.dV1 = calc_maneuver_dV(initial_orbit.periapsis, initial_orbit.apoapsis, planned_orbit.apoapsis);
        mp.dV2 = calc_maneuver_dV(planned_orbit.apoapsis, initial_orbit.periapsis, planned_orbit.periapsis);
    } else {
        mp.first_raise_Apo = false;
        mp.dV1 = calc_maneuver_dV(initial_orbit.apoapsis, initial_orbit.periapsis, planned_orbit.periapsis);
        mp.dV2 = calc_maneuver_dV(planned_orbit.periapsis, initial_orbit.apoapsis, planned_orbit.apoapsis);
    }

    return mp;
}

double calc_maneuver_dV(double static_apsis, double initial_apsis, double new_apsis) {
    struct Orbit initial_orbit;
    struct Orbit new_orbit;

    initial_orbit = construct_orbit(static_apsis, initial_apsis, 0);
    new_orbit = construct_orbit(static_apsis, new_apsis, 0);

    double v0 = calc_orbital_speed(static_apsis, initial_orbit.a);
    double v1 = calc_orbital_speed(static_apsis, new_orbit.a);

    return fabs(v1-v0);
}

double calc_orbital_speed(double r, double a) {
    double v2 = MU_EARTH * (2/r - 1/a);
    return sqrt(v2);
}