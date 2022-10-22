#ifndef ORBIT
#define ORBIT

struct Orbit {
    double e;           // eccentricity
    double a;           // semi-major axis
    double inclination; // inclination
    double lan;         // longitude of ascending node
    double arg_of_peri; // argument of periapsis
    double true_anom;   // true anomaly
    double period;      // orbital period
    double apoapsis;    // highest point in orbit
    double periapsis;   // lowest point in orbit
};

// constructs orbit using apsides and inclination
struct Orbit constr_orbit_w_apsides(double apsis1, double apsis2, double inclination, struct Body body);


#endif
