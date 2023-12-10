#ifndef KSP_TRANSFER_TOOLS_H
#define KSP_TRANSFER_TOOLS_H

#include "orbit.h"
#include "analytic_geometry.h"
#include "ephem.h"

// 2-dimensional transfer orbit with orbital elements
struct Transfer2D {
    struct Orbit orbit;
    double theta1, theta2;
};

enum Transfer_Type {
    capcap,
    circcap,
    capcirc,
    circcirc,
    capfb,
    circfb
};

// 3-dimensional transfer from point r0 to point r1
struct Transfer {
    struct Vector r0, v0, r1, v1;
};

// orbital state vector (position vector and velocity vector)
struct OSV {
    struct Vector r, v;
};

// calculate the 2-dimensional vector with given magnitude, location, true anomaly and flight path angle
struct Vector2D calc_v_2d(double r_mag, double v_mag, double theta, double gamma);

// calculate 3D-Vector from vector in orbital plane with given RAAN, Argument of Periapsis and Inclination
struct Vector heliocentric_rot(struct Vector2D v, double RAAN, double w, double inc);

// calculate orbital elements in 2D orbit using the radii of the two transfer points, the target transfer duration and the delta in true anomaly between them
struct Transfer2D calc_2d_transfer_orbit(double r1, double r2, double target_dt, double dtheta, struct Body *attractor);

// calculate starting velocity and end velocity for transfer between r1 and r2 with given 2D orbital elements
struct Transfer calc_transfer_dv(struct Transfer2D transfer2d, struct Vector r1, struct Vector r2);

// calculate transfer between two different points in a given amount of time
struct Transfer calc_transfer(enum Transfer_Type tt, struct Body *dep_body, struct Body *arr_body, struct Vector r1, struct Vector v1, struct Vector r2, struct Vector v2, double dt, double *data);

// calculate the delta-v between circular orbit at given Periapsis and speed at given Periapsis for given excess speed
double dv_circ(struct Body *body, double rp, double vinf);

// calculate the delta-v between speed at given Periapsis for excess speed of 0m/s at given Periapsis and speed at given Periapsis for given excess speed
double dv_capture(struct Body *body, double rp, double vinf);

// propagate elliptical orbit by time
struct OSV propagate_orbit(struct Vector r, struct Vector v, double dt, struct Body *attractor);

// calculate the orbital state vector at the given date for a given ephemeris list
struct OSV osv_from_ephem(struct Ephem *ephem_list, double date, struct Body *attractor);

void calc_double_swing_by(struct OSV s0, struct OSV p0, struct OSV s1, struct OSV p1, double transfer_duration, struct Body *body, double **xs, int **ints);

#endif //KSP_TRANSFER_TOOLS_H
