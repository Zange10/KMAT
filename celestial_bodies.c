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
struct Body *moho;
struct Body *eve;
struct Body *kerbin;
struct Body *duna;
struct Body *jool;

struct Body * all_celestial_bodies[20];

struct System *curr_system;
struct System *solar_system_ephem, *solar_system, *stock_system;



struct System * get_current_system() {
	return curr_system;
}

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

void set_body_color(struct Body *body, double red, double green, double blue) {
	body->color[0] = red;
	body->color[1] = green;
	body->color[2] = blue;
}

void init_SUN() {
    sun = (struct Body*)malloc(sizeof(struct Body));
    strcpy(sun->name, "SUN");
	set_body_color(sun, 1.0, 1.0, 0.3);
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
	set_body_color(mercury, 0.3, 0.3, 0.3);
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
        /*  i  */ deg2rad(7.00487),
        /* raan */ deg2rad(48.33167),
        /*  ω  */ deg2rad((77.45645-48.33167)),   // longitude of perihelion - longitude of ascending node
		/*  θ  */ deg2rad(0),
        /*pbody*/ SUN()
    );

	int num_body_ephems = 12*100;	// 12 months for 100 years (1950-2050)
	mercury->ephem = (struct Ephem*) malloc(num_body_ephems*sizeof(struct Ephem));
	get_body_ephem(mercury->ephem, mercury->id);
}

void init_VENUS() {
    venus = (struct Body*)malloc(sizeof(struct Body));
    strcpy(venus->name, "VENUS");
	set_body_color(venus, 0.6, 0.6, 0.2);
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
        /*  i  */ deg2rad(3.39471),
        /* raan */ deg2rad(76.68069),
		/*  θ  */ deg2rad(0),
        /*  ω  */ deg2rad((131.53298-76.68069)),   // longitude of perihelion - longitude of ascending node
        /*pbody*/ SUN()
    );

	int num_body_ephems = 12*100;	// 12 months for 100 years (1950-2050)
	venus->ephem = (struct Ephem*) malloc(num_body_ephems*sizeof(struct Ephem));
	get_body_ephem(venus->ephem, venus->id);
}

void init_EARTH() {
    earth = (struct Body*)malloc(sizeof(struct Body));
    strcpy(earth->name, "EARTH");
	set_body_color(earth, 0.2, 0.2, 1.0);
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
        /*  i  */ deg2rad(0.00005),
        /* raan */ deg2rad(-11.26064),
        /*  w  */ deg2rad((102.94719-(-11.26064))),
		/*  θ  */ deg2rad(0),
        /*pbody*/ SUN()
    );

	int num_body_ephems = 12*100;	// 12 months for 100 years (1950-2050)
	earth->ephem = (struct Ephem*) malloc(num_body_ephems*sizeof(struct Ephem));
	get_body_ephem(earth->ephem, earth->id);
}

void init_MOON() {
    moon = (struct Body*)malloc(sizeof(struct Body));
    strcpy(moon->name, "MOON");
	set_body_color(moon, 0.3, 0.3, 0.3);
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
        /*  i  */ deg2rad(5.145),
        /* raan */ deg2rad(0),    // not static
        /*  w  */ deg2rad(0),    // not static
		/*  θ  */ deg2rad(0),
        /*pbody*/ EARTH()
    );
}

void init_MARS() {
    mars = (struct Body*)malloc(sizeof(struct Body));
    strcpy(mars->name, "MARS");
	set_body_color(mars, 1.0, 0.2, 0.0);
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
        /*  i  */ deg2rad(1.85061),
        /* raan */ deg2rad(49.57854),
        /*  w  */ deg2rad((336.04084-49.57854)),
		/*  θ  */ deg2rad(0),
        /*pbody*/ SUN()
    );

	int num_body_ephems = 12*100;	// 12 months for 100 years (1950-2050)
	mars->ephem = (struct Ephem*) malloc(num_body_ephems*sizeof(struct Ephem));
	get_body_ephem(mars->ephem, mars->id);
}

void init_JUPITER() {
    jupiter = (struct Body*)malloc(sizeof(struct Body));
    strcpy(jupiter->name, "JUPITER");
	set_body_color(jupiter, 0.6, 0.4, 0.2);
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
        /*  i  */ deg2rad(1.30530),
        /* raan */ deg2rad(100.55615),
        /*  w  */ deg2rad((14.75385-100.55615)),
		/*  θ  */ deg2rad(0),
        /*pbody*/ SUN()
    );

	int num_body_ephems = 12*100;	// 12 months for 100 years (1950-2050)
	jupiter->ephem = (struct Ephem*) malloc(num_body_ephems*sizeof(struct Ephem));
	get_body_ephem(jupiter->ephem, jupiter->id);
}

void init_SATURN() {
    saturn = (struct Body*)malloc(sizeof(struct Body));
    strcpy(saturn->name, "SATURN");
	set_body_color(saturn, 0.8, 0.8, 0.6);
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
        /*  i  */ deg2rad(2.48446),
        /* raan */ deg2rad(113.71504),
        /*  w  */ deg2rad((92.43194-113.71504)),
		/*  θ  */ deg2rad(0),
        /*pbody*/ SUN()
    );

	int num_body_ephems = 12*100;	// 12 months for 100 years (1950-2050)
	saturn->ephem = (struct Ephem*) malloc(num_body_ephems*sizeof(struct Ephem));
	get_body_ephem(saturn->ephem, saturn->id);
}

void init_URANUS() {
    uranus = (struct Body*)malloc(sizeof(struct Body));
    strcpy(uranus->name, "URANUS");
	set_body_color(uranus, 0.2, 0.6, 1.0);
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
        /*  i  */ deg2rad(0.76986),
        /* raan */ deg2rad(74.22988),
        /*  ω  */ deg2rad((170.96424-74.22988)),   // longitude of perihelion - longitude of ascending node
		/*  θ  */ deg2rad(0),
        /*pbody*/ SUN()
    );

	int num_body_ephems = 12*100;	// 12 months for 100 years (1950-2050)
	uranus->ephem = (struct Ephem*) malloc(num_body_ephems*sizeof(struct Ephem));
	get_body_ephem(uranus->ephem, uranus->id);
}

void init_NEPTUNE() {
    neptune = (struct Body*)malloc(sizeof(struct Body));
    strcpy(neptune->name, "NEPTUNE");
	set_body_color(neptune, 0.0, 0.0, 1.0);
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
        /*  i  */ deg2rad(1.76917),
        /* raan */ deg2rad(131.72169),
        /*  ω  */ deg2rad((44.97135-131.72169)),   // longitude of perihelion - longitude of ascending node
		/*  θ  */ deg2rad(0),
        /*pbody*/ SUN()
    );

	int num_body_ephems = 12*100;	// 12 months for 100 years (1950-2050)
	neptune->ephem = (struct Ephem*) malloc(num_body_ephems*sizeof(struct Ephem));
	get_body_ephem(neptune->ephem, neptune->id);
}

void init_PLUTO() {
    pluto = (struct Body*)malloc(sizeof(struct Body));
    strcpy(pluto->name, "PLUTO");
	set_body_color(pluto, 0.7, 0.7, 0.7);
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
        /*  i  */ deg2rad(17.14175),
        /* raan */ deg2rad(110.30347),
        /*  ω  */ deg2rad((224.06676-110.30347)),   // longitude of perihelion - longitude of ascending node
		/*  θ  */ deg2rad(0),
        /*pbody*/ SUN()
    );

	int num_body_ephems = 12*100;	// 12 months for 100 years (1950-2050)
	pluto->ephem = (struct Ephem*) malloc(num_body_ephems*sizeof(struct Ephem));
	get_body_ephem(pluto->ephem, pluto->id);
}


// Kerbol system

void init_KERBOL() {
    kerbol = (struct Body*)malloc(sizeof(struct Body));
    strcpy(kerbol->name, "KERBOL");
	set_body_color(kerbol, 1.0, 1.0, 0.3);
    kerbol->mu = 11723.328e14;
    kerbol->radius = 261600e3;
    kerbol->rotation_period = 432000;
    kerbol->sl_atmo_p = 0;          // has atmosphere but not really interesting for this calculator
    kerbol->scale_height = 0;
    kerbol->atmo_alt = 0;
    // kerbol's orbit not declared as it should not be used and can't give any meaningful information
}

void init_EVE() {
	eve = (struct Body*)malloc(sizeof(struct Body));
	strcpy(eve->name, "EVE");
	set_body_color(eve, 0.5, 0.0, 1.0);
	eve->mu = 8.1717302e12;
	eve->radius = 700e3;
	eve->rotation_period = 80500.0;
	eve->sl_atmo_p = 506625;
	eve->scale_height = 7000;
	eve->atmo_alt = 90e3;
	eve->orbit = constr_orbit(
			/*  a  */ 9.832684544e9,
			/*  e  */ 0.01,
			/*  i  */ deg2rad(2.1),
			/* raan */ deg2rad(15),
			/*  w  */ deg2rad(0),
			/*  θ  */ deg2rad(0),
			/*pbody*/ KERBOL()
	);
}

void init_KERBIN() {
    kerbin = (struct Body*)malloc(sizeof(struct Body));
    strcpy(kerbin->name, "KERBIN");
	set_body_color(kerbin, 0.2, 0.2, 1.0);
    kerbin->mu = 3.5316e12;
    kerbin->radius = 600e3;
    kerbin->rotation_period = 21549.452;
    kerbin->sl_atmo_p = 101325;
    kerbin->scale_height = 5600;
    kerbin->atmo_alt = 70e3;
    kerbin->orbit = constr_orbit(
        /*  a  */ 13.599840256e9,
        /*  e  */ 0,
        /*  i  */ deg2rad(0),
        /* raan */ deg2rad(0),
        /*  w  */ deg2rad(0),
		/*  θ  */ deg2rad(0),
        /*pbody*/ KERBOL()
    );
}

void init_DUNA() {
	duna = (struct Body*)malloc(sizeof(struct Body));
	strcpy(duna->name, "DUNA");
	set_body_color(duna, 1.0, 0.2, 0.0);
	duna->mu = 3.0136321e11;
	duna->radius = 320e3;
	duna->rotation_period = 65517.859;
	duna->sl_atmo_p = 6755;
	duna->scale_height = 5600;
	duna->atmo_alt = 50e3;
	duna->orbit = constr_orbit(
			/*  a  */ 20.726155264e9,
			/*  e  */ 0.051,
			/*  i  */ deg2rad(0.06),
			/* raan */ deg2rad(135.5),
			/*  w  */ deg2rad(0),
			/*  θ  */ deg2rad(0),
			/*pbody*/ KERBOL()
	);
}

void init_JOOL() {
	jool = (struct Body*)malloc(sizeof(struct Body));
	strcpy(jool->name, "JOOL");
	set_body_color(jool, 0.0, 0.4, 0.0);
	jool->mu = 2.8252800e14;
	jool->radius = 6000e3;
	jool->rotation_period = 36000.0;
	jool->sl_atmo_p = 1519880;
	jool->scale_height = 22000;
	jool->atmo_alt = 200e3;
	jool->orbit = constr_orbit(
			/*  a  */ 68.773560320e9,
			/*  e  */ 0.05,
			/*  i  */ deg2rad(1.304),
			/* raan */ deg2rad(52),
			/*  w  */ deg2rad(0),
			/*  θ  */ deg2rad(0),
			/*pbody*/ KERBOL()
	);
}


// ##############################################################################################


void init_solar_system() {
	solar_system = (struct System*)malloc(sizeof(struct System));
	strcpy(solar_system->name, "Solar System (RSS)");
	solar_system->cb = SUN();
	solar_system->calc_method = ORB_ELEMENTS;
	solar_system->num_bodies = 9;
	solar_system->ut0 = 2451545.0;

	solar_system->bodies = (struct Body**)malloc(solar_system->num_bodies*sizeof(struct Body*));
	solar_system->bodies[0] = MERCURY();
	solar_system->bodies[1] = VENUS();
	solar_system->bodies[2] = EARTH();
	solar_system->bodies[3] = MARS();
	solar_system->bodies[4] = JUPITER();
	solar_system->bodies[5] = SATURN();
	solar_system->bodies[6] = URANUS();
	solar_system->bodies[7] = NEPTUNE();
	solar_system->bodies[8] = PLUTO();
}


void init_solar_system_ephem() {
	solar_system_ephem = (struct System*)malloc(sizeof(struct System));
	strcpy(solar_system_ephem->name, "Solar System (RSS Ephem)");
	solar_system_ephem->cb = SUN();
	solar_system_ephem->calc_method = EPHEMS;
	solar_system_ephem->num_bodies = 9;

	solar_system_ephem->bodies = (struct Body**)malloc(solar_system_ephem->num_bodies * sizeof(struct Body*));
	solar_system_ephem->bodies[0] = MERCURY();
	solar_system_ephem->bodies[1] = VENUS();
	solar_system_ephem->bodies[2] = EARTH();
	solar_system_ephem->bodies[3] = MARS();
	solar_system_ephem->bodies[4] = JUPITER();
	solar_system_ephem->bodies[5] = SATURN();
	solar_system_ephem->bodies[6] = URANUS();
	solar_system_ephem->bodies[7] = NEPTUNE();
	solar_system_ephem->bodies[8] = PLUTO();
}

void init_stock_system() {
	stock_system = (struct System*)malloc(sizeof(struct System));
	strcpy(stock_system->name, "Stock System");
	stock_system->cb = kerbol;
	stock_system->calc_method = ORB_ELEMENTS;
	stock_system->num_bodies = 4;
	solar_system->ut0 = 0.0;

	stock_system->bodies = (struct Body**)malloc(stock_system->num_bodies*sizeof(struct Body*));
	stock_system->bodies[0] = eve;
	stock_system->bodies[1] = kerbin;
	stock_system->bodies[2] = duna;
	stock_system->bodies[3] = jool;
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
	init_EVE();
    init_KERBIN();
	init_DUNA();
	init_JOOL();

	init_solar_system_ephem();
	init_solar_system();
	init_stock_system();
//	curr_system = solar_system;
//	curr_system = solar_system_ephem;
	curr_system = stock_system;
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