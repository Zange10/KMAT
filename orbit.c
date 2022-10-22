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