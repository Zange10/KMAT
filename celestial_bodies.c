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
	new_body->system = NULL;
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

void parse_and_sort_into_celestial_subsystems(struct System *system) {
	system->cb->system = system;
	
	for(int i = 0; i < system->num_bodies; i++) {
		int num_child_bodies = 0;
		struct Body *body = system->bodies[i];
		for(int j = 0; j < system->num_bodies; j++) {
			if(system->bodies[j]->orbit.body == body) num_child_bodies++;
		}
		if(num_child_bodies > 0) {
			struct System *child_system = new_system();
			sprintf(child_system->name, "%s SYSTEM", body->name);
			child_system->num_bodies = num_child_bodies;
			child_system->bodies = malloc(num_child_bodies * sizeof(struct Body*));
			child_system->cb = body;
			child_system->calc_method = system->calc_method;
			child_system->ut0 = system->ut0;
			body->system = child_system;
		}
	}
	
	for(int i = 0; i < system->num_bodies; i++) {
		struct Body *body = system->bodies[i];
		struct Body *attractor = body->orbit.body;
		if(attractor != system->cb) {
			for(int j = 0; j < attractor->system->num_bodies-1; j++) {
				attractor->system->bodies[j] = attractor->system->bodies[j+1];
			}
			attractor->system->bodies[attractor->system->num_bodies-1] = body;
			for(int j = i; j < system->num_bodies-1; j++) {
				system->bodies[j] = system->bodies[j+1];
			}
			system->num_bodies--;
			i--;
		}
	}
	
	struct Body **temp = realloc(system->bodies, system->num_bodies*(sizeof(struct Body*)));
	if(temp == NULL) return;
	system->bodies = temp;
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

int get_number_of_subsystems(struct System *system) {
	int num_subsystems = 0;
	for(int i = 0; i < system->num_bodies; i++) {
		if(system->bodies[i]->system != NULL) {
			num_subsystems++;
			num_subsystems += get_number_of_subsystems(system->bodies[i]->system);
		}
	}
	return num_subsystems;
}

struct System * get_subsystem_from_system_and_id_rec(struct System *system, int *id) {
	if(*id == 0) return system;
	(*id)--;
	for(int i = 0; i < system->num_bodies; i++) {
		if(system->bodies[i]->system != NULL) {
			struct System *subsystem = get_subsystem_from_system_and_id_rec(system->bodies[i]->system, id);
			if(subsystem != NULL) return subsystem;
		}
	}
	return NULL;
}

struct System * get_subsystem_from_system_and_id(struct System *system, int id) {
	return get_subsystem_from_system_and_id_rec(system, &id);
}

struct System * get_top_level_system(struct System *system) {
	if(system == NULL) return NULL;
	while(system->cb->orbit.body != NULL) system = system->cb->orbit.body->system;
	return system;
}

struct System * get_system_by_name(char *name) {
	for(int i = 0; i < get_num_available_systems(); i++) {
		if(strcmp(get_available_systems()[i]->name, name) == 0) return get_available_systems()[i];
	}
	
	return NULL;
}

struct Body * get_body_by_name(char *name, struct System *system) {
	if(strcmp(system->cb->name, name) == 0) return system->cb;
	for(int i = 0; i < system->num_bodies; i++) {
		if(system->bodies[i] == NULL) return NULL;
		if(strcmp(system->bodies[i]->name, name) == 0) return system->bodies[i];
		if(system->bodies[i]->system != NULL) {
			struct Body *body = get_body_by_name(name, system->bodies[i]->system);
			if(body != NULL) return body;
		}
	}
	return NULL;
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

void print_celestial_system_layer(struct System *system, int layer) {
	for(int i = 0; i < system->num_bodies; i++) {
		for(int j = 0; j < layer-1; j++) printf("│  ");
		if(layer != 0 && i < system->num_bodies-1) printf("├─ ");
		else if(layer != 0) printf("└─ ");
		printf("%s\n", system->bodies[i]->name);
		if(system->bodies[i]->system != NULL) print_celestial_system_layer(system->bodies[i]->system, layer+1);
	}
}

void print_celestial_system(struct System *system) {
	printf("%s\n", system->cb->name);
	print_celestial_system_layer(system, 1);
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