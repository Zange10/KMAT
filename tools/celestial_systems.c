#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "celestial_systems.h"
#include "tools/file_io.h"

Body *sun;
Body *earth;

Body * all_celestial_bodies[20];

CelestSystem **available_systems;

int num_available_systems = 0;


void init_available_systems(const char *directory) {
	available_systems = init_available_systems_from_path(directory, &num_available_systems);
}

int get_num_available_systems() {return num_available_systems;}
CelestSystem ** get_available_systems() {return available_systems;}

int is_available_system(CelestSystem *system) {
	if(system == NULL) return 0;
	for(int i = 0; i < num_available_systems; i++) {
		if(system == get_available_systems()[i]) return 1;
	}
	return 0;
}

CelestSystem * get_subsystem_from_system_and_id_rec(CelestSystem *system, int *id) {
	if(*id == 0) return system;
	(*id)--;
	for(int i = 0; i < system->num_bodies; i++) {
		if(system->bodies[i]->system != NULL) {
			CelestSystem *subsystem = get_subsystem_from_system_and_id_rec(system->bodies[i]->system, id);
			if(subsystem != NULL) return subsystem;
		}
	}
	return NULL;
}

CelestSystem * get_subsystem_from_system_and_id(CelestSystem *system, int id) {
	return get_subsystem_from_system_and_id_rec(system, &id);
}

CelestSystem * get_system_by_name(char *name) {
	for(int i = 0; i < get_num_available_systems(); i++) {
		if(strcmp(get_available_systems()[i]->name, name) == 0) return get_available_systems()[i];
	}
	
	return NULL;
}

void free_all_celestial_systems() {
	free_celestial_systems(available_systems, num_available_systems);
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
	sun->orbit.cb = NULL;
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
    earth->orbit = constr_orbit_from_elements(
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