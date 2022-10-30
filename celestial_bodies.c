#include <math.h>
#include <cstring>
#include "celestial_bodies.h"

struct Body *sun;
struct Body *venus;
struct Body *earth;
struct Body *moon;
struct Body *mars;
struct Body *jupiter;
struct Body *saturn;

struct Body *kerbin;



struct Body ** all_celest_bodies() {
    static struct Body * all_celestial_bodies[] = {
        SUN(),
        VENUS(),
        EARTH(),
        MOON(),
        MARS(),
        JUPITER(),
        SATURN(),
        KERBIN()
    };
    return all_celestial_bodies;
}

void init_celestial_bodies() {
    sun = (struct Body*)malloc(sizeof(struct Body));
    strcpy(sun->name, "SUN");
    sun->mu = 1327124.40042e14;
    sun->radius = 695700e3;
    sun->rotation_period = 2192832;
    sun->sl_atmo_p = 0;
    sun->scale_height = 0;
    sun->atmo_alt = 0;
    // sun's orbit not declared as it should not be used and can't give any meaningful information


    venus = (struct Body*)malloc(sizeof(struct Body));
    strcpy(venus->name, "VENUS");
    venus->mu = 3.24859e14;
    venus->radius = 6049e3;
    venus->rotation_period = -20996797.016381; // rotates in opposite direction
    venus->sl_atmo_p = 10905200;
    venus->scale_height = 15900;
    venus->atmo_alt = 145e3;
    venus->orbit = constr_orbit(
        /*  a  */ 108.2089255e+9,
        /*  e  */ 0.00677323,
        /*  i  */ 3.39471,
        /* lan */ 76.68069,
        /*  ω  */ 131.53298-76.68069,   // longitude of perihelion - longitude of ascending node
        /*pbody*/ SUN()
    );


    earth = (struct Body*)malloc(sizeof(struct Body));
    strcpy(earth->name, "EARTH");
    earth->mu = 3.986004418e14;
    earth->radius = 6371e3;
    earth->rotation_period = 86164.098903691;
    earth->sl_atmo_p = 101325;
    earth->scale_height = 8500;
    earth->atmo_alt = 140e3;
    earth->orbit = constr_orbit(
        /*  a  */ 149.5978872e+9,
        /*  e  */ 0.01671022,
        /*  i  */ 0.00005,
        /* lan */ -11.26064,
        /*  w  */ 102.94719-(-11.26064),
        /*pbody*/ SUN()
    );


    moon = (struct Body*)malloc(sizeof(struct Body));
    strcpy(moon->name, "MOON");
    moon->mu = 0.049038958e14;
    moon->radius = 1737.1e3;
    moon->rotation_period = 2360584.68479999;
    moon->sl_atmo_p = 0;
    moon->scale_height = 0;
    moon->atmo_alt = 0;
    moon->orbit = constr_orbit(
        /*  a  */ 0.3844e9,
        /*  e  */ 0.0549,
        /*  i  */ 5.145,
        /* lan */ 0,    // not static
        /*  w  */ 0,    // not static
        /*pbody*/ EARTH()
    );


    mars = (struct Body*)malloc(sizeof(struct Body));
    strcpy(mars->name, "MARS");
    mars->mu = 0.4282831e14;
    mars->radius = 3375.8e3;
    mars->rotation_period = 88642.6848;
    mars->sl_atmo_p = 1144.97;
    mars->scale_height = 11100;
    mars->atmo_alt = 125e3;
    mars->orbit = constr_orbit(
        /*  a  */ 227.9366372e+9,
        /*  e  */ 0.09341233,
        /*  i  */ 1.85061,
        /* lan */ 49.57854,
        /*  w  */ 336.04084-49.57854,
        /*pbody*/ SUN()
    );


    jupiter = (struct Body*)malloc(sizeof(struct Body));
    strcpy(jupiter->name, "JUPITER");
    jupiter->mu = 1266.86534e14;
    jupiter->radius = 69373e3;
    jupiter->rotation_period = 35730;
    jupiter->sl_atmo_p = 101325000;
    jupiter->scale_height = 27000;
    jupiter->atmo_alt = 1550e3;
    jupiter->orbit = constr_orbit(
        /*  a  */ 778.4120268e+9,
        /*  e  */ 0.04839266,
        /*  i  */ 1.30530,
        /* lan */ 100.55615,
        /*  w  */ 14.75385-100.55615,
        /*pbody*/ SUN()
    );


    saturn = (struct Body*)malloc(sizeof(struct Body));
    strcpy(saturn->name, "SATURN");
    saturn->mu = 379.31187e14;
    saturn->radius = 57216e3;
    saturn->rotation_period = 38052;
    saturn->sl_atmo_p = 101325000;
    saturn->scale_height = 59500;
    saturn->atmo_alt = 2000e3;
    saturn->orbit = constr_orbit(
        /*  a  */ 1426.725413e+9,
        /*  e  */ 0.05415060,
        /*  i  */ 2.48446,
        /* lan */ 113.71504,
        /*  w  */ 92.43194-113.71504,
        /*pbody*/ SUN()
    );


    // Kerbol system
    kerbin = (struct Body*)malloc(sizeof(struct Body));
    strcpy(kerbin->name, "KERBIN");
    kerbin->mu = 3.5316e12;
    kerbin->radius = 600e3;
    kerbin->rotation_period = 21549.452;
    kerbin->sl_atmo_p = 101325000;
    kerbin->scale_height = 5600;
    kerbin->atmo_alt = 70e3;
    kerbin->orbit = constr_orbit(
        /*  a  */ 13.599840256e9,
        /*  e  */ 0,
        /*  i  */ 0,
        /* lan */ 0,
        /*  w  */ 0,
        /*pbody*/ NULL // Kerbol tbd
    );
}




// scale_heights are not ksp-ro-true, as it is handled with key points in the atmosphere to create pressure curve. Still used for easier calculations

// REAL SOLAR SYSTEM #########################################################################

struct Body * SUN() {
    return sun;
}

struct Body * VENUS() {
    return venus;
}

struct Body * EARTH() {
    return earth;
}

struct Body * MOON() {
    return moon;
}

struct Body * MARS() {
    return mars;
}

struct Body * JUPITER() {
    return jupiter;
}

struct Body * SATURN() {
    return saturn;
}



// KERBOL SYSTEM #########################################################################

struct Body * KERBIN() {
    return kerbin;
}