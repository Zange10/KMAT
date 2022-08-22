#include <math.h>
#include "celestial_bodies.h"

// scale_heights are not ksp-ro-true, as it is handled with key points in the atmosphere to create pressure curve. Still used for easier calculations

struct Body VENUS() {
    struct Body venus;
    venus.mu = 3.24859e14;
    venus.radius = 6049e3;
    venus.rotation_period = -20996797.016381; // rotates in opposite direction
    venus.sl_atmo_p = 10905200;
    venus.scale_height = 15900;
    venus.atmo_alt = 145e3;
    return venus;
}

struct Body EARTH() {
    struct Body earth;
    earth.mu = 3.986004418e14;
    earth.radius = 6371e3;
    earth.rotation_period = 86164.098903691;
    earth.sl_atmo_p = 101325;
    earth.scale_height = 8500;
    earth.atmo_alt = 140e3;
    return earth;
}

struct Body MOON() {
    struct Body moon;
    moon.mu = 0.049038958e14;
    moon.radius = 1737.1e3;
    moon.rotation_period = 2360584.68479999;
    moon.sl_atmo_p = 0;
    moon.scale_height = 0;
    moon.atmo_alt = 0;
    return moon;
}

struct Body MARS() {
    struct Body mars;
    mars.mu = 0.4282831e14;
    mars.radius = 3375.8e3;
    mars.rotation_period = 88642.6848;
    mars.sl_atmo_p = 1144.97;
    mars.scale_height = 11100;
    mars.atmo_alt = 125e3;
    return mars;
}

struct Body JUPITER() {
    struct Body jupiter;
    jupiter.mu = 1266.86534e14;
    jupiter.radius = 69373e3;
    jupiter.rotation_period = 35730
    jupiter.sl_atmo_p = 101325000;
    jupiter.scale_height = 27000;
    jupiter.atmo_alt = 1550e3;
    return jupiter;
}

struct Body SATURN() {
    struct Body saturn;
    saturn.mu = 379.31187e14;
    saturn.radius = 57216e3;
    saturn.rotation_period = 38052;
    saturn.sl_atmo_p = 101325000;
    saturn.scale_height = 59500;
    saturn.atmo_alt = 2000e3;
    return saturn;
}