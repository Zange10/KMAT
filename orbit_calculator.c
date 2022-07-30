#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include "orbit_calculator.h"
#include "celestial_bodies.h"
#include "tool_funcs.h"

struct Orbit {
    double apoapsis;    // highest point in orbit
    double periapsis;   // lowest point in orbit
    double a;           // semi-major axis
    double inclination; // inclination
    double e;           // eccentricity
    double period;      // orbital period
};

struct ManeuverPlan {
    double dV1;
    double dV2;
    bool first_raise_Apo;
};

struct Orbit construct_orbit(double apsis1, double apsis2, double inclination, struct Body body) {
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
    new_orbit.e = (new_orbit.apoapsis-new_orbit.periapsis)/(new_orbit.apoapsis+new_orbit.periapsis);
    new_orbit.period = 2*M_PI*sqrt(pow(new_orbit.a,3)/body.mu);
    return new_orbit;
}

void print_orbit_info(struct Orbit orbit, struct Body body) {
    printf("\n______________________\nORBIT:\n\n");
    printf("Apoapsis:\t\t%g km\n", (orbit.apoapsis-body.radius)/1000);
    printf("Periapsis:\t\t%g km\n", (orbit.periapsis-body.radius)/1000);
    printf("Semi-major axis:\t%g km\n", orbit.a /1000);
    printf("Inclination:\t\t%g°\n", orbit.inclination);
    printf("Eccentricity:\t\t%g\n", orbit.e);
    printf("Orbital Period:\t\t%gs\n", orbit.period);
    printf("______________________\n\n");
}

void print_orbit_apsides(double apsis1, double apsis2, struct Body body) {
    apsis1 -= body.radius;
    apsis2 -= body.radius;
    apsis1 /= 1000;
    apsis2 /= 1000;
    if(apsis1 > apsis2) printf("%gkm - %gkm", apsis1, apsis2);
    else printf("%gkm - %gkm", apsis2, apsis1);
}

void orbit_calculator() {
    char title[] = "CHOOSE CALCULATION:";
    char options[] = "Go Back; Get orbit info; Change Apsis from circular orbit; change Apsis; Hohmann Transfer";
    char question[] = "Calculation: ";
    int selection = 0;

    struct Body parent_body = EARTH();

    do {
        selection = user_selection(title, options, question);
        switch (selection)
        {
        case 1:
            calc_orbital_parameters(parent_body);
            break;
        case 2:
            change_apsis_circ(parent_body);
            break;
        case 3:
            change_apsis(parent_body);
            break;
        case 4:
            calc_hohmann_transfer(parent_body);
            break;
        default:
            break;
        }
    } while(selection != 0);
}

void calc_orbital_parameters(struct Body body) {
    double apsis1 = 0;
    double apsis2 = 0;
    double inclination = 0;

    printf("Enter apsis1, apsis2 and inclination: ");
    scanf("%lf %lf %lf", &apsis1, &apsis2, &inclination);

    apsis1 = apsis1*1000+body.radius;
    apsis2 = apsis2*1000+body.radius;

    struct Orbit orbit = construct_orbit(apsis1, apsis2, inclination,body);
    print_orbit_info(orbit,body);

    return;
}

void change_apsis_circ(struct Body body) {
    double initial_apsis = 0;
    double new_apsis = 0;

    printf("Enter parameters (altitude of initial circular orbit, new value of apsis): ");
    scanf("%lf %lf", &initial_apsis, &new_apsis);

    initial_apsis = initial_apsis*1000+body.radius;
    new_apsis = new_apsis*1000+body.radius;

    double dV = calc_maneuver_dV(initial_apsis, initial_apsis, new_apsis,body);

    printf("\n____________\n\nOrbit 1: "); print_orbit_apsides(initial_apsis,initial_apsis,body); printf("   -->   Orbit 2: "); print_orbit_apsides(new_apsis,initial_apsis,body);
    printf("\nNeeded dV: %g m/s\n____________\n\n", dV);

    return;
}

void change_apsis(struct Body body) {
    double initial_apsis = 0;
    double new_apsis = 0;
    double static_apsis = 0;

    printf("Enter parameters (static Apsis, initial value of apsis, new value of apsis): ");
    scanf("%lf %lf %lf", &static_apsis, &initial_apsis, &new_apsis);

    static_apsis = static_apsis*1000+body.radius;
    initial_apsis = initial_apsis*1000+body.radius;
    new_apsis = new_apsis*1000+body.radius;

    double dV = calc_maneuver_dV(static_apsis, initial_apsis, new_apsis,body);

    printf("\n____________\n\nOrbit 1: "); print_orbit_apsides(initial_apsis,static_apsis,body); printf("   -->   Orbit 2: "); print_orbit_apsides(new_apsis,static_apsis,body);
    printf("\nNeeded dV: %g m/s\n____________\n\n", dV);

    return;
}

void calc_hohmann_transfer(struct Body body) {
    double initial_apsis = 0;
    double new_apsis = 0;

    printf("Enter parameters (altitude of initial circular orbit, altitude of planned circular orbit): ");
    scanf("%lf %lf", &initial_apsis, &new_apsis);
    
    initial_apsis = initial_apsis*1000+body.radius;
    new_apsis = new_apsis*1000+body.radius;

    struct Orbit initial_orbit;
    struct Orbit new_orbit;

    initial_orbit = construct_orbit(initial_apsis, initial_apsis, 0, body);
    new_orbit = construct_orbit(new_apsis, new_apsis, 0, body);

    struct ManeuverPlan mp = calc_change_orbit_dV(initial_orbit, new_orbit, body);

    printf("\n____________\n\nOrbit 1: "); print_orbit_apsides(initial_apsis,initial_apsis, body); printf("   -->   Orbit 2: "); print_orbit_apsides(new_apsis,new_apsis, body);
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


struct ManeuverPlan calc_change_orbit_dV(struct Orbit initial_orbit, struct Orbit planned_orbit, struct Body body) {
    struct ManeuverPlan mp;
    if(planned_orbit.apoapsis > initial_orbit.apoapsis) {
        mp.first_raise_Apo = true;
        mp.dV1 = calc_maneuver_dV(initial_orbit.periapsis, initial_orbit.apoapsis, planned_orbit.apoapsis, body);
        mp.dV2 = calc_maneuver_dV(planned_orbit.apoapsis, initial_orbit.periapsis, planned_orbit.periapsis, body);
    } else {
        mp.first_raise_Apo = false;
        mp.dV1 = calc_maneuver_dV(initial_orbit.apoapsis, initial_orbit.periapsis, planned_orbit.periapsis, body);
        mp.dV2 = calc_maneuver_dV(planned_orbit.periapsis, initial_orbit.apoapsis, planned_orbit.apoapsis, body);
    }

    return mp;
}

double calc_maneuver_dV(double static_apsis, double initial_apsis, double new_apsis, struct Body body) {
    struct Orbit initial_orbit;
    struct Orbit new_orbit;

    initial_orbit = construct_orbit(static_apsis, initial_apsis, 0, body);
    new_orbit = construct_orbit(static_apsis, new_apsis, 0, body);

    double v0 = calc_orbital_speed(static_apsis, initial_orbit.a, body);
    double v1 = calc_orbital_speed(static_apsis, new_orbit.a, body);

    return fabs(v1-v0);
}

double calc_orbital_speed(double r, double a, struct Body body) {
    double v2 = body.mu * (2/r - 1/a);
    return sqrt(v2);
}