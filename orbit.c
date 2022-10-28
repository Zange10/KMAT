#include <math.h>
#include <stdio.h>
#include "orbit.h"




struct Orbit constr_orbit_w_apsides(double apsis1, double apsis2, double inclination, struct Body * body) {
    struct Orbit new_orbit;
    new_orbit.body = body;
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
    new_orbit.period = 2*M_PI*sqrt(pow(new_orbit.a,3)/body->mu);

    // not given, therefore zero
    new_orbit.lan = 0;
    new_orbit.arg_of_peri = 0;
    new_orbit.true_anom = 0;

    return new_orbit;
}


double calc_orbital_speed(struct Orbit orbit, double r) {
    double v2 = orbit.body->mu * (2/r - 1/orbit.a);
    return sqrt(v2);
}

// Printing info #######################################################

void print_orbit_info(struct Orbit orbit) {
    struct Body *body = orbit.body;
    printf("\n______________________\nORBIT:\n\n");
    printf("Orbiting: \t\t%s\n", body->name);
    printf("Apoapsis:\t\t%g km\n", (orbit.apoapsis-body->radius)/1000);
    printf("Periapsis:\t\t%g km\n", (orbit.periapsis-body->radius)/1000);
    printf("Semi-major axis:\t%g km\n", orbit.a /1000);
    printf("Inclination:\t\t%g°\n", orbit.inclination);
    printf("Eccentricity:\t\t%g\n", orbit.e);
    printf("Orbital Period:\t\t%gs\n", orbit.period);
    printf("______________________\n\n");
}

void print_orbit_apsides(struct Orbit orbit) {
    struct Body *body = orbit.body;
    double apo = orbit.apoapsis;
    double peri = orbit.periapsis;
    apo  -= body->radius;
    peri -= body->radius;
    apo  /= 1000;
    peri /= 1000;
    printf("%gkm - %gkm", apo, peri);
}