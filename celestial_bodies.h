#ifndef CELESTIAL_BODIES
#define CELESTIAL_BODIES

struct Body {
    double mu;              // gravitational parameter of body [m³/s²]
    double radius;          // radius of body [m]
    double rotation_period; // the time period, in which the body rotates around its axis [s]
    double sl_atmo_p;       // atmospheric pressure at sea level [Pa]
    double scale_height;    // the height at which the atmospheric pressure decreases by the factor e [m]
    double atmo_alt;        // highest altitude with atmosphere (ksp-specific) [m]
};

struct Body VENUS();
struct Body EARTH();
struct Body MOON();
struct Body MARS();
struct Body JUPITER();
struct Body SATURN();

struct Body KERBIN();

#endif