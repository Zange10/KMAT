#ifndef CELESTIAL_BODIES
#define CELESTIAL_BODIES

struct Body {
    char name[10];
    double mu;              // gravitational parameter of body [m³/s²]
    double radius;          // radius of body [m]
    double rotation_period; // the time period, in which the body rotates around its axis [s]
    double sl_atmo_p;       // atmospheric pressure at sea level [Pa]
    double scale_height;    // the height at which the atmospheric pressure decreases by the factor e [m]
    double atmo_alt;        // highest altitude with atmosphere (ksp-specific) [m]
};

// returns all stored celestial bodies in an array
struct Body* all_celest_bodies();

struct Body VENUS();
struct Body EARTH();
struct Body MOON();
struct Body MARS();
struct Body JUPITER();
struct Body SATURN();

struct Body KERBIN();

#endif