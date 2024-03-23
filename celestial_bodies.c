#include <string.h>
#include <stdlib.h>
#include "celestial_bodies.h"

struct Body *sun;
struct Body *mercury;
struct Body *venus;
struct Body *earth;
struct Body *moon;
struct Body *mars;
struct Body *jupiter;
struct Body *saturn;
struct Body *uranus;
struct Body *neptune;
struct Body *pluto;

struct Body *kerbol;
struct Body *kerbin;

struct Body * all_celestial_bodies[20];



struct Body ** all_celest_bodies() {
    all_celestial_bodies[0] = SUN();
    all_celestial_bodies[1] = MERCURY();
    all_celestial_bodies[2] = VENUS();
    all_celestial_bodies[3] = EARTH();
    all_celestial_bodies[4] = MOON();
    all_celestial_bodies[5] = MARS();
    all_celestial_bodies[6] = JUPITER();
    all_celestial_bodies[7] = SATURN();
    all_celestial_bodies[8] = URANUS();
    all_celestial_bodies[9] = NEPTUNE();
    all_celestial_bodies[10] = PLUTO();
    all_celestial_bodies[11] = KERBOL();
    all_celestial_bodies[12] = KERBIN();
    return all_celestial_bodies;
}

void init_SUN() {
    sun = (struct Body*)malloc(sizeof(struct Body));
    strcpy(sun->name, "SUN");
    sun->id = 10;
    sun->mu = 1327124.40042e14;
    sun->radius = 695700e3;
    sun->rotation_period = 2192832;
    sun->sl_atmo_p = 0;
    sun->scale_height = 0;
    sun->atmo_alt = 0;
    // sun's orbit not declared as it should not be used and can't give any meaningful information
}

void init_MERCURY() {
    mercury = (struct Body*)malloc(sizeof(struct Body));
    strcpy(mercury->name, "MERCURY");
    mercury->id = 1;
    mercury->mu = 0.22032e14;
    mercury->radius = 2439.7e3;
    mercury->rotation_period = 5067360; // rotates in opposite direction
    mercury->sl_atmo_p = 0;
    mercury->scale_height = 0;
    mercury->atmo_alt = 0;
    mercury->orbit = constr_orbit(
        /*  a  */ 57.90917568e+9,
        /*  e  */ 0.20563069,
        /*  i  */ 7.00487,
        /* raan */ 48.33167,
        /*  ω  */ 77.45645-48.33167,   // longitude of perihelion - longitude of ascending node
        /*pbody*/ SUN()
    );
}

void init_VENUS() {
    venus = (struct Body*)malloc(sizeof(struct Body));
    strcpy(venus->name, "VENUS");
    venus->id = 2;
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
        /* raan */ 76.68069,
        /*  ω  */ 131.53298-76.68069,   // longitude of perihelion - longitude of ascending node
        /*pbody*/ SUN()
    );
}

void init_EARTH() {
    earth = (struct Body*)malloc(sizeof(struct Body));
    strcpy(earth->name, "EARTH");
    earth->id = 3;
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
        /* raan */ -11.26064,
        /*  w  */ 102.94719-(-11.26064),
        /*pbody*/ SUN()
    );
}

void init_MOON() {
    moon = (struct Body*)malloc(sizeof(struct Body));
    strcpy(moon->name, "MOON");
    moon->id = 301;
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
        /* raan */ 0,    // not static
        /*  w  */ 0,    // not static
        /*pbody*/ EARTH()
    );
}

void init_MARS() {
    mars = (struct Body*)malloc(sizeof(struct Body));
    strcpy(mars->name, "MARS");
    mars->id = 4;
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
        /* raan */ 49.57854,
        /*  w  */ 336.04084-49.57854,
        /*pbody*/ SUN()
    );
}

void init_JUPITER() {
    jupiter = (struct Body*)malloc(sizeof(struct Body));
    strcpy(jupiter->name, "JUPITER");
    jupiter->id = 5;
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
        /* raan */ 100.55615,
        /*  w  */ 14.75385-100.55615,
        /*pbody*/ SUN()
    );
}

void init_SATURN() {
    saturn = (struct Body*)malloc(sizeof(struct Body));
    strcpy(saturn->name, "SATURN");
    saturn->id = 6;
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
        /* raan */ 113.71504,
        /*  w  */ 92.43194-113.71504,
        /*pbody*/ SUN()
    );
}

void init_URANUS() {
    uranus = (struct Body*)malloc(sizeof(struct Body));
    strcpy(uranus->name, "URANUS");
    uranus->id = 7;
    uranus->mu = 57.940e14;
    uranus->radius = 25362e3;
    uranus->rotation_period = -62064; // rotates in tilted direction
    uranus->sl_atmo_p = 101325000;
    uranus->scale_height = 27700;
    uranus->atmo_alt = 1400e3;
    uranus->orbit = constr_orbit(
        /*  a  */ 2870.97222e+9,
        /*  e  */ 0.04716771,
        /*  i  */ 0.76986,
        /* raan */ 74.22988,
        /*  ω  */ 170.96424-74.22988,   // longitude of perihelion - longitude of ascending node
        /*pbody*/ SUN()
    );
}

void init_NEPTUNE() {
    neptune = (struct Body*)malloc(sizeof(struct Body));
    strcpy(neptune->name, "NEPTUNE");
    neptune->id = 8;
    neptune->mu = 68.351e14;
    neptune->radius = 24622e3;
    neptune->rotation_period = 57996;
    neptune->sl_atmo_p = 101325000;
    neptune->scale_height = 19700;
    neptune->atmo_alt = 1250e3;
    neptune->orbit = constr_orbit(
        /*  a  */ 4498.2529108e+9,
        /*  e  */ 0.00858587,
        /*  i  */ 1.76917,
        /* raan */ 131.72169,
        /*  ω  */ 44.97135-131.72169,   // longitude of perihelion - longitude of ascending node
        /*pbody*/ SUN()
    );
}

void init_PLUTO() {
    pluto = (struct Body*)malloc(sizeof(struct Body));
    strcpy(pluto->name, "PLUTO");
    pluto->id = 9;
    pluto->mu = 0.00870e14;
    pluto->radius = 1188e3;
    pluto->rotation_period = -551854.08; // rotates in opposite direction
    pluto->sl_atmo_p = 1320;
    pluto->scale_height = 50000;
    pluto->atmo_alt = 110e3;
    pluto->orbit = constr_orbit(
        /*  a  */ 5906.3762724e+9,
        /*  e  */ 0.24880766,
        /*  i  */ 17.14175,
        /* raan */ 110.30347,
        /*  ω  */ 224.06676-110.30347,   // longitude of perihelion - longitude of ascending node
        /*pbody*/ SUN()
    );
}


// Kerbol system

void init_KERBOL() {
    kerbol = (struct Body*)malloc(sizeof(struct Body));
    strcpy(kerbol->name, "KERBOL");
    kerbol->mu = 11723.328e14;
    kerbol->radius = 261600e3;
    kerbol->rotation_period = 432000;
    kerbol->sl_atmo_p = 0;          // has atmosphere but not really interesting for this calculator
    kerbol->scale_height = 0;
    kerbol->atmo_alt = 0;
    // kerbol's orbit not declared as it should not be used and can't give any meaningful information
}

void init_KERBIN() {
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
        /* raan */ 0,
        /*  w  */ 0,
        /*pbody*/ KERBOL()
    );
}





// ##############################################################################################

void init_celestial_bodies() {
    init_SUN();
    init_MERCURY();
    init_VENUS();
    init_EARTH();
    init_MOON();
    init_MARS();
    init_JUPITER();
    init_SATURN();
    init_URANUS();
    init_NEPTUNE();
    init_PLUTO();

    init_KERBOL();
    init_KERBIN();
}



struct Body * get_body_from_id(int id) {
	struct Body **all_bodies = all_celest_bodies();
	int i = 0;
	while(all_bodies[i] != NULL) {
		if(all_bodies[i]->id == id) return all_bodies[i];
		i++;
	}
	return NULL;
}



// scale_heights are not ksp-ro-true, as it is handled with key points in the atmosphere to create pressure curve. Still used for easier calculations

// REAL SOLAR SYSTEM #########################################################################

struct Body * SUN() {
    return sun;
}

struct Body * MERCURY() {
    return mercury;
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

struct Body * URANUS() {
    return uranus;
}

struct Body * NEPTUNE() {
    return neptune;
}

struct Body * PLUTO() {
    return pluto;
}



// KERBOL SYSTEM #########################################################################

struct Body * KERBOL() {
    return kerbol;
}

struct Body * KERBIN() {
    return kerbin;
}