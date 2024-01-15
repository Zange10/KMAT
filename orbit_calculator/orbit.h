#ifndef ORBIT
#define ORBIT

#include "celestial_bodies.h"
#include "analytic_geometry.h"

struct Orbit {
    struct Body * body; // parent body
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



struct Body {
    char name[10];
    int id;                 // ID issued by JPL's Horizon API
    double mu;              // gravitational parameter of body [m³/s²]
    double radius;          // radius of body [m]
    double rotation_period; // the time period, in which the body rotates around its axis [s]
    double sl_atmo_p;       // atmospheric pressure at sea level [Pa]
    double scale_height;    // the height at which the atmospheric pressure decreases by the factor e [m]
    double atmo_alt;        // highest altitude with atmosphere (ksp-specific) [m]
    struct Orbit orbit;     // orbit of body
};

// constructs orbit using orbital elements
struct Orbit constr_orbit(double a, double e, double i, double lan, double arg_of_peri, struct Body *body);

// constructs orbit using apsides and inclination
struct Orbit constr_orbit_w_apsides(double apsis1, double apsis2, double inclination, struct Body * body);

// constructs orbit from Orbital State Vector
struct Orbit constr_orbit_from_osv(struct Vector r, struct Vector v, struct Body *attractor);


// calculate the tangential speed at point in orbit (altitude)
double calc_orbital_speed(struct Orbit orbit, double r);


// Prints parameters specific to the orbit
void print_orbit_info(struct Orbit orbit);
// Prints apsides (no line-break): "Apoapsis - Periapsis"
void print_orbit_apsides(struct Orbit orbit);

#endif
