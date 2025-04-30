#ifndef ORBIT
#define ORBIT

// needs to be before include ephem (or anything that includes celestial bodies),
// because struct Orbit needs to be defined before defining struct Body in celestial_bodies.h
struct Orbit {
    struct Body * body; // central body
    double e;           // eccentricity
    double a;           // semi-major axis
    double inclination; // inclination
    double raan;        // longitude of ascending node
    double arg_peri; 	// argument of periapsis
    double theta;   	// true anomaly
	double t;			// time since periapsis
    double period;      // orbital period
    double apoapsis;    // highest point in orbit
    double periapsis;   // lowest point in orbit
};

#include "tools/analytic_geometry.h"
#include "tools/ephem.h"

// constructs orbit using orbital elements
struct Orbit constr_orbit(double a, double e, double i, double raan, double arg_of_peri, double theta, struct Body *body);

// constructs orbit using apsides and inclination
struct Orbit constr_orbit_w_apsides(double apsis1, double apsis2, double inclination, struct Body * body);

// constructs orbit from Orbital State Vector
struct Orbit constr_orbit_from_osv(struct Vector r, struct Vector v, struct Body *attractor);


// calculate the tangential speed at point in orbit (altitude)
double calc_orbital_speed(struct Orbit orbit, double r);


double calc_dtheta_from_dt(struct Orbit orbit, double dt);

double calc_dt_from_dtheta(struct Orbit orbit, double dtheta);

double calc_orbit_apoapsis(struct Orbit orbit);

double calc_orbit_periapsis(struct Orbit orbit);

double calc_true_anomaly_from_mean_anomaly(struct Orbit orbit, double mean_anomaly);

// Prints parameters specific to the orbit
void print_orbit_info(struct Orbit orbit);
// Prints apsides (no line-break): "Apoapsis - Periapsis"
void print_orbit_apsides(struct Orbit orbit);

#endif
