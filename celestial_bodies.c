#include <math.h>
#include "celestial_bodies.h"

struct Body VENUS() {
    struct Body venus;
    venus.mu = 3.2486e14;
    venus.radius = 6051.8e3;
    venus.rotation_period = 5832.5 * pow(60,2);
    venus.sl_atmo_p = 9321900;
    venus.scale_height = 15900;
    venus.atmo_alt = 145e3;
    return venus;
}

struct Body EARTH() {
    struct Body earth;
    earth.mu = 3.9860e14;
    earth.radius = 6371.0e3;
    earth.rotation_period = 23.9345 * pow(60,2);
    earth.sl_atmo_p = 101325;
    earth.scale_height = 8500;
    earth.atmo_alt = 140e3;
    return earth;
}

struct Body MOON() {
    struct Body moon;
    moon.mu = 0.0490e14;
    moon.radius = 1737.4e3;
    moon.rotation_period = 655.72 * pow(60,2);
    moon.sl_atmo_p = 0;
    moon.scale_height = 0;
    moon.atmo_alt = 0;
    return moon;
}

struct Body MARS() {
    struct Body mars;
    mars.mu = 0.42828e14;
    mars.radius = 3389.5e3;
    mars.rotation_period = 24.6229 * pow(60,2);
    mars.sl_atmo_p = 644;
    mars.scale_height = 11100;
    mars.atmo_alt = 125e3;
    return mars;
}

struct Body JUPITER() {
    struct Body jupiter;
    jupiter.mu = 1266.87e14;
    jupiter.radius = 69911e3;
    jupiter.rotation_period = 9.9250 * pow(60,2);
    jupiter.sl_atmo_p = 101325;
    jupiter.scale_height = 27000;
    jupiter.atmo_alt = 140e3;
    return jupiter;
}

struct Body SATURN() {
    struct Body saturn;
    saturn.mu = 379.31e14;
    saturn.radius = 58232e3;
    saturn.rotation_period = 10.656 * pow(60,2);
    saturn.sl_atmo_p = 101325;
    saturn.scale_height = 59500;
    saturn.atmo_alt = 140e3;
    return saturn;
}