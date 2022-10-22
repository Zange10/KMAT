#include <math.h>
#include "orbit.h"
#include "celestial_bodies.h"



struct Orbit constr_orbit_w_apsides(double apsis1, double apsis2, double inclination, struct Body body) {
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

    // not given, therefore zero
    new_orbit.lan = 0;
    new_orbit.arg_of_peri = 0;
    new_orbit.true_anom = 0;

    return new_orbit;
}


double calc_orbital_speed(double r, double a, struct Body body) {
    double v2 = body.mu * (2/r - 1/a);
    return sqrt(v2);
}

// Printing info #######################################################

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