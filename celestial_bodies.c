#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "celestial_bodies.h"
#include "tools/file_io.h"

struct Body *sun;
struct Body *earth;

struct Body * all_celestial_bodies[20];

struct System **available_systems;

int num_available_systems = 0;



struct Body * new_body() {
	struct Body *new_body = (struct Body*) malloc(sizeof(struct Body));
	sprintf(new_body->name, "BODY");
	new_body->color[0] = 0.5;
	new_body->color[1] = 0.5;
	new_body->color[2] = 0.5;
	new_body->id = 0;
	new_body->mu = 0;
	new_body->radius = 0;
	new_body->rotation_period = 86400;
	new_body->sl_atmo_p = 0;
	new_body->scale_height = 1000;
	new_body->atmo_alt = 0;
	new_body->ephem = NULL;

	new_body->orbit.a = 150e9;
	new_body->orbit.e = 0;
	new_body->orbit.inclination = 0;
	new_body->orbit.raan = 0;
	new_body->orbit.arg_peri = 0;
	new_body->orbit.theta = 0;
	new_body->orbit.body = NULL;

	return new_body;
}

void set_body_color(struct Body *body, double red, double green, double blue) {
	body->color[0] = red;
	body->color[1] = green;
	body->color[2] = blue;
}

struct System * new_system() {
	struct System *system = (struct System*) malloc(sizeof(struct System));
	sprintf(system->name,"CELESTIAL SYSTEM");
	system->num_bodies = 0;
	system->cb = NULL;
	system->bodies = NULL;
	system->calc_method = EPHEMS;
	system->ut0 = 0;
	return system;
}


void init_available_systems(const char *directory) {
	available_systems = (struct System**) malloc(10 * sizeof(struct System*));	// A maximum of 10 systems seems reasonable

	create_directory_if_not_exists(directory);
	GDir *dir = g_dir_open(directory, 0, NULL);
	if (!dir) {
		g_printerr("Unable to open directory: %s\n", directory);
		return;
	}

	const gchar *filename;
	while ((filename = g_dir_read_name(dir)) != NULL) {
		if (g_str_has_suffix(filename, ".cfg")) {
			char filepath[100];
			sprintf(filepath, "%s%s", directory, filename);
			struct System *system = load_system_from_config_file(filepath);
			if(system != NULL) {
				available_systems[num_available_systems] = system;
				num_available_systems++;
			}
		}
	}

	g_dir_close(dir);
}

int get_num_available_systems() {return num_available_systems;}
struct System ** get_available_systems() {return available_systems;}

int is_available_system(struct System *system) {
	if(system == NULL) return 0;
	for(int i = 0; i < num_available_systems; i++) {
		if(system == get_available_systems()[i]) return 1;
	}
	return 0;
}

void free_all_celestial_systems() {
	for(int i = 0; i < num_available_systems; i++) {
		free_system(get_available_systems()[i]);
	}
}

void free_system(struct System *system) {
	if(system == NULL) return;
	for(int i = 0; i < system->num_bodies; i++) {
		if(system->bodies[i]->ephem != NULL) free(system->bodies[i]->ephem);
		free(system->bodies[i]);
	}
	free(system->cb);
	free(system);
}

int get_body_system_id(struct Body *body, struct System *system) {
	for(int i = 0; i < system->num_bodies; i++) {
		if(system->bodies[i] == body) return i;
	}
	return -1;
}





// LEGACY CODE (to be removed in future) --------------------------------------------

struct Body ** all_celest_bodies() {
    all_celestial_bodies[0] = SUN();
    all_celestial_bodies[3] = EARTH();
    return all_celestial_bodies;
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
	sun->orbit.body = NULL;
    // sun's orbit not declared as it should not be used and can't give any meaningful information
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

	earth->ephem = NULL;
//	get_body_ephems(earth, SUN());
}

void init_celestial_bodies() {
    init_SUN();
    init_EARTH();
}

struct Body * SUN() {
    return sun;
}

struct Body * EARTH() {
    return earth;
}