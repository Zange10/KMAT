#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "file_io.h"

void write_csv(char fields[], double data[]) {
    
    time_t currtime;
    time(&currtime);
    struct tm now = *gmtime(&currtime);
    char filename[19];  // 14 for date + 4 for .csv + 1 for string terminator
    sprintf(filename, "%04d%02d%02d%02d%02d%02d.csv", now.tm_year+1900, now.tm_mon, now.tm_mday,now.tm_hour, now.tm_min, now.tm_sec);

    // -------------------

    int n_fields = amt_of_fields(fields);

    FILE *file;
    file = fopen(filename,"w");

    fprintf(file,"%s\n", fields);

    for(int i = 1; i < data[0]; i+=n_fields) {   // data[0] amount of data points in data
        for(int j = 0; j < n_fields-1; j++) fprintf(file, "%f,",data[i+j]);
        fprintf(file, "%f\n", data[i+n_fields-1]);
    }

    fclose(file);
}

int amt_of_fields(char *fields) {
    int c = 1;
    int i = 0;
    do {
        if(fields[i] == ',') c++;
        i++;
    } while(fields[i] != 0);    // 0 = NULL Terminator for strings
    return c;
}





void store_body_in_config_file(FILE *file, struct Body *body, struct System *system) {
	fprintf(file, "[%s]\n", body->name);
	fprintf(file, "color = [%.3f, %.3f, %.3f]\n", body->color[0], body->color[1], body->color[2]);
	fprintf(file, "id = %d\n", body->id);
	fprintf(file, "gravitational_parameter = %.9g\n", body->mu);
	fprintf(file, "radius = %f\n", body->radius/1e3);
	fprintf(file, "rotational_period = %f\n", body->rotation_period);
	fprintf(file, "sea_level_pressure = %f\n", body->sl_atmo_p/1e3);
	fprintf(file, "scale_height = %.0f\n", body->scale_height);
	fprintf(file, "atmosphere_altitude = %f\n", body->atmo_alt/1e3);
	if(body->orbit.body != system->cb && system->calc_method != EPHEMS) {
		fprintf(file, "semi_major_axis = %f\n", body->orbit.a/1e3);
		fprintf(file, "eccentricity = %f\n", body->orbit.e);
		fprintf(file, "inclination = %f\n", rad2deg(body->orbit.inclination));
		fprintf(file, "raan = %f\n", rad2deg(body->orbit.raan));
		fprintf(file, "argument_of_periapsis = %f\n", rad2deg(body->orbit.arg_peri));
		fprintf(file, "true_anomaly_ut0 = %f\n", rad2deg(body->orbit.theta));
	}
}

void store_system_in_config_file(struct System *system) {
	char filename[50];
	sprintf(filename, "./Celestial_Systems/test.cfg");

	FILE *file;
	file = fopen(filename,"w");

	fprintf(file, "[%s]\n", system->name);
	fprintf(file, "propagation_method = %s\n", system->calc_method == ORB_ELEMENTS ? "ELEMENTS" : "EPHEMERIDES");
	fprintf(file, "ut0 = %f\n", system->ut0);
	fprintf(file, "number_of_bodies = %d\n", system->num_bodies);
	fprintf(file, "central_body = %s\n\n", system->cb->name);

	store_body_in_config_file(file, system->cb, system);

	for(int i = 0; i < system->num_bodies; i++) {
		fprintf(file, "\n");
		store_body_in_config_file(file, system->bodies[i], system);
	}


	fclose(file);
}


/*********************************************************************
 *          Celestial System Binary Storing and Loading
 *********************************************************************/
union CelestialSystemBin {
	struct CelestialSystemBinValid {
		int *ptr;
	} is_valid;

	struct CelestialSystemBinT2 {
		char name[50];
		int num_bodies;
		enum SystemCalcMethod calc_method;
		double ut0;
	} t2;
};

union CelestialBodyBin {
	struct CelestialBodyBinValid {
		int *ptr;
	} is_valid;

	struct CelestialBodyBinT2 {
		char name[32];
		double color[3];
		int id;  // ID given by JPL's Horizon API
		double mu;
		double radius;
		double a, e, incl, raan, arg_peri, theta;
	} t2;
};

union CelestialSystemBin convert_celestial_system_bin(struct System *system, int file_type) {
	union CelestialSystemBin bin_system;
	switch(file_type) {
		case 2:
			sprintf(bin_system.t2.name, "%s", system->name);
			bin_system.t2.num_bodies = system->num_bodies;
			bin_system.t2.calc_method = system->calc_method;
			bin_system.t2.ut0 = system->ut0;
			break;
		default:
			bin_system.is_valid.ptr = NULL;
	}
	return bin_system;
}

struct System * convert_bin_celestial_system(union CelestialSystemBin bin_system, int file_type) {
	struct System *system = (struct System*) malloc(sizeof(struct System));
	switch(file_type) {
		case 2:
			sprintf(system->name, "%s", bin_system.t2.name);
			system->num_bodies = bin_system.t2.num_bodies;
			system->calc_method = bin_system.t2.calc_method;
			system->ut0 = bin_system.t2.ut0;
			system->bodies = (struct Body**) malloc(system->num_bodies * sizeof(struct Body*));
			break;
	}
	return system;
}

union CelestialBodyBin convert_celestial_body_bin(struct Body *body, int file_type) {
	union CelestialBodyBin bin_body;
	switch(file_type) {
		case 2:
			sprintf(bin_body.t2.name, "%s", body->name);
			bin_body.t2.color[0] = body->color[0];
			bin_body.t2.color[1] = body->color[1];
			bin_body.t2.color[2] = body->color[2];
			bin_body.t2.id = body->id;
			bin_body.t2.mu = body->mu;
			bin_body.t2.radius = body->radius;
			bin_body.t2.a = body->orbit.a;
			bin_body.t2.e = body->orbit.e;
			bin_body.t2.incl = body->orbit.inclination;
			bin_body.t2.raan = body->orbit.raan;
			bin_body.t2.arg_peri = body->orbit.arg_peri;
			bin_body.t2.theta = body->orbit.theta;
			break;
		default:
			bin_body.is_valid.ptr = NULL;
	}
	return bin_body;
}

struct Body * convert_bin_celestial_body(union CelestialBodyBin bin_body, struct Body *attractor, int file_type) {
	struct Body *body = (struct Body*) malloc(sizeof(struct Body));
	switch(file_type) {
		case 2:
			sprintf(body->name, "%s", bin_body.t2.name);
			body->color[0] = bin_body.t2.color[0];
			body->color[1] = bin_body.t2.color[1];
			body->color[2] = bin_body.t2.color[2];
			body->id = bin_body.t2.id;
			body->mu = bin_body.t2.mu;
			body->radius = bin_body.t2.radius;
			if(attractor != NULL)
				body->orbit = constr_orbit(
						bin_body.t2.a,
						bin_body.t2.e,
						bin_body.t2.incl,
						bin_body.t2.raan,
						bin_body.t2.arg_peri,
						bin_body.t2.theta,
						attractor);
			break;
	}
	return body;
}

void store_celestial_system_in_bfile(struct System *system, FILE *file, int file_type) {
	if(file_type == 2) {
		union CelestialSystemBin system_bin = convert_celestial_system_bin(system, file_type);
		fwrite(&system_bin.t2, sizeof(struct CelestialSystemBinT2), 1, file);

		union CelestialBodyBin body_bin = convert_celestial_body_bin(system->cb, file_type);
		fwrite(&body_bin.t2, sizeof(struct CelestialBodyBinT2), 1, file);

		for(int i = 0; i < system->num_bodies; i++) {
			body_bin = convert_celestial_body_bin(system->bodies[i], file_type);
			fwrite(&body_bin.t2, sizeof(struct CelestialBodyBinT2), 1, file);
		}
	}
}

struct System * load_celestial_system_from_bfile(FILE *file, int file_type) {
	struct System *system;
	if(file_type == 2) {
		union CelestialSystemBin system_bin;
		fread(&system_bin.t2, sizeof(struct CelestialSystemBinT2), 1, file);
		system = convert_bin_celestial_system(system_bin, file_type);

		union CelestialBodyBin body_bin;
		fread(&body_bin.t2, sizeof(struct CelestialBodyBinT2), 1, file);
		system->cb = convert_bin_celestial_body(body_bin, NULL, file_type);

		for(int i = 0; i < system->num_bodies; i++) {
			fread(&body_bin.t2, sizeof(struct CelestialBodyBinT2), 1, file);
			system->bodies[i] = convert_bin_celestial_body(body_bin, system->cb, file_type);
		}
	}

	return system;
}


/*********************************************************************
 *                 Multi-Itinerary Binary Storing
 *********************************************************************/


union ItinStepBinHeader {
	struct ItinStepBinHeaderValid {
		int *ptr;
	} is_valid;

	struct ItinStepBinHeaderT0 {
		int num_nodes, num_deps, num_layers;
	} t0;

	struct ItinStepBinHeaderT1 {
		int num_nodes, num_deps;
	} t1, t2;
};

union ItinStepBin {
	struct ItinStepBinValid {
		int *ptr;
	} is_valid;

	struct ItinStepBinT0 {
		struct Vector r;
		struct Vector v_dep, v_arr, v_body;
		double date;
		int num_next_nodes;
	} t0;

	struct ItinStepBinT1 {
		double date;
		struct Vector r;
		struct Vector v_dep, v_arr, v_body;
		int body_id;
		int num_next_nodes;
	} t1, t2;
};

union ItinStepBin convert_ItinStep_bin(struct ItinStep *step, struct System *system, int file_type) {
	union ItinStepBin bin_step;
	switch(file_type) {
		case 0:
			bin_step.t0.r = step->r;
			bin_step.t0.v_dep = step->v_dep;
			bin_step.t0.v_arr = step->v_arr;
			bin_step.t0.v_body = step->v_body;
			bin_step.t0.date = step->date;
			bin_step.t0.num_next_nodes = step->num_next_nodes;
			break;
		case 1:
			bin_step.t1.r = step->r;
			bin_step.t1.v_dep = step->v_dep;
			bin_step.t1.v_arr = step->v_arr;
			bin_step.t1.v_body = step->v_body;
			bin_step.t1.date = step->date;
			bin_step.t1.body_id = step->body->id;
			bin_step.t1.num_next_nodes = step->num_next_nodes;
		case 2:
			bin_step.t2.r = step->r;
			bin_step.t2.v_dep = step->v_dep;
			bin_step.t2.v_arr = step->v_arr;
			bin_step.t2.v_body = step->v_body;
			bin_step.t2.date = step->date;
			bin_step.t2.body_id = get_body_system_id(step->body, system);
			bin_step.t2.num_next_nodes = step->num_next_nodes;
			break;
		default:
			bin_step.is_valid.ptr = NULL;
			break;
	}

	return bin_step;
}

void store_step_in_bfile(struct ItinStep *step, struct System *system, FILE *file, int file_type) {
	if(file_type == 0) {
		union ItinStepBin bin_step = convert_ItinStep_bin(step, system, 0);
		fwrite(&bin_step.t0, sizeof(struct ItinStepBinT0), 1, file);
	} else if(file_type == 1) {
		union ItinStepBin bin_step = convert_ItinStep_bin(step, system, 1);
		fwrite(&bin_step.t2, sizeof(struct ItinStepBinT1), 1, file);
	} else if(file_type == 2) {
		union ItinStepBin bin_step = convert_ItinStep_bin(step, system, 1);
		fwrite(&bin_step.t2, sizeof(struct ItinStepBinT1), 1, file);
	}
	for(int i = 0; i < step->num_next_nodes; i++) {
		store_step_in_bfile(step->next[i], system, file, file_type);
	}
}

void store_itineraries_in_bfile(struct ItinStep **departures, int num_nodes, int num_deps, struct System *system, char *filepath, int file_type) {
	// Check if the string ends with ".itins"
	if (strlen(filepath) >= 6 && strcmp(filepath + strlen(filepath) - 6, ".itins") != 0) {
		// If not, append ".itins" to the string
		strcat(filepath, ".itins");
	}

	union ItinStepBinHeader bin_header;
	int end_of_bodies_designator = -1;
	FILE *file = fopen(filepath, "wb");
	fwrite(&file_type, sizeof(int), 1, file);

	switch (file_type) {
		// TYPE 0 --------------------------------------------
		case 0:
			bin_header.t0.num_nodes = num_nodes;
			bin_header.t0.num_deps = num_deps;
			bin_header.t0.num_layers = get_num_of_itin_layers(departures[0]);

			printf("Filesize: ~%.3f MB\n", (double) num_nodes*sizeof(struct ItinStepBinT0)/1e6);

			printf("Number of stored nodes: %d\n", bin_header.t0.num_nodes);
			printf("Number of Departures: %d, Number of Steps: %d\n", num_deps, bin_header.t0.num_layers);

			fwrite(&bin_header.t0, sizeof(struct ItinStepBinHeaderT0), 1, file);

			struct ItinStep *ptr = departures[0];

			// same algorithm as layer counter (part of header)
			while(ptr != NULL) {
				int body_id = (ptr->body != NULL) ? ptr->body->id : 0;
				fwrite(&body_id, sizeof(int), 1, file);
				if(ptr->next != NULL) ptr = ptr->next[0];
				else break;
			}

			fwrite(&end_of_bodies_designator, sizeof(int), 1, file);

			for(int i = 0; i < num_deps; i++) {
				store_step_in_bfile(departures[i], system, file, file_type);
			}
			break;
		// TYPE 1 --------------------------------------------
		case 1:
			bin_header.t1.num_nodes = num_nodes;
			bin_header.t1.num_deps = num_deps;

			printf("Filesize: ~%.3f MB\n", (double) num_nodes*sizeof(struct ItinStepBinT1)/1e6);

			printf("Number of stored nodes: %d\n", bin_header.t1.num_nodes);
			printf("Number of Departures: %d\n", num_deps);

			fwrite(&bin_header.t1, sizeof(struct ItinStepBinHeaderT1), 1, file);

			fwrite(&end_of_bodies_designator, sizeof(int), 1, file);

			for(int i = 0; i < num_deps; i++) {
				store_step_in_bfile(departures[i], system, file, file_type);
			}
			break;
		// TYPE 2 --------------------------------------------
		case 2:
			bin_header.t2.num_nodes = num_nodes;
			bin_header.t2.num_deps = num_deps;

			printf("Filesize: ~%.3f MB\n", (double) num_nodes*sizeof(struct ItinStepBinT1)/1e6);

			printf("Number of stored nodes: %d\n", bin_header.t2.num_nodes);
			printf("Number of Departures: %d\n", num_deps);

			fwrite(&bin_header.t2, sizeof(struct ItinStepBinHeaderT1), 1, file);

			fwrite(&end_of_bodies_designator, sizeof(int), 1, file);

			store_celestial_system_in_bfile(system, file, file_type);

			for(int i = 0; i < num_deps; i++) {
				store_step_in_bfile(departures[i], system, file, file_type);
			}

		default:
			bin_header.is_valid.ptr = NULL;
	}
	// ---------------------------------------------------

	fclose(file);
}


/*********************************************************************
 *                 Multi-Itinerary Binary Loading
 *********************************************************************/

void convert_bin_ItinStep(union ItinStepBin bin_step, struct ItinStep *step, struct Body *body, struct System *system, int file_type) {
	switch(file_type) {
		case 0:
			step->body = body;
			step->r = bin_step.t0.r;
			step->v_arr = bin_step.t0.v_arr;
			step->v_body = bin_step.t0.v_body;
			step->v_dep = bin_step.t0.v_dep;
			step->date = bin_step.t0.date;
			step->num_next_nodes = bin_step.t0.num_next_nodes;
			break;
		case 1:
			step->body = get_body_from_id(bin_step.t1.body_id);
			step->r = bin_step.t1.r;
			step->v_arr = bin_step.t1.v_arr;
			step->v_body = bin_step.t1.v_body;
			step->v_dep = bin_step.t1.v_dep;
			step->date = bin_step.t1.date;
			step->num_next_nodes = bin_step.t1.num_next_nodes;
			break;
		case 2:
			step->body = system->bodies[bin_step.t2.body_id];
			step->r = bin_step.t2.r;
			step->v_arr = bin_step.t2.v_arr;
			step->v_body = bin_step.t2.v_body;
			step->v_dep = bin_step.t2.v_dep;
			step->date = bin_step.t2.date;
			step->num_next_nodes = bin_step.t2.num_next_nodes;
		default:
			return;
	}
}

void load_step_from_bfile(struct ItinStep *step, FILE *file, struct Body **body, struct System *system, int file_type) {
	union ItinStepBin bin_step;
	// TYPE 0 --------------------------------------------
	if(file_type == 0) {
		fread(&bin_step.t0, sizeof(struct ItinStepBinT0), 1, file);
		convert_bin_ItinStep(bin_step, step, body[0], system, 0);
		// TYPE 1 --------------------------------------------
	} else if(file_type == 1) {
		fread(&bin_step.t1, sizeof(struct ItinStepBinT1), 1, file);
		convert_bin_ItinStep(bin_step, step, NULL, system, 1);
	} else if(file_type == 2) {
		fread(&bin_step.t2, sizeof(struct ItinStepBinT1), 1, file);
		convert_bin_ItinStep(bin_step, step, NULL, system, 2);
	}
	// ---------------------------------------------------

	if(step->num_next_nodes > 0)
		step->next = (struct ItinStep **) malloc(step->num_next_nodes * sizeof(struct ItinStep *));
	else step->next = NULL;

	for(int i = 0; i < step->num_next_nodes; i++) {
		step->next[i] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
		step->next[i]->prev = step;
		load_step_from_bfile(step->next[i], file, body == NULL ? NULL : body + 1, system, file_type);
	}
}

struct ItinsLoadFileResults load_itineraries_from_bfile(char *filepath) {
	struct ItinStep **departures = NULL;
	union ItinStepBinHeader bin_header;
	struct System *system;
	int num_deps;

	FILE *file;
	file = fopen(filepath,"rb");

	int type;
	fread(&type, sizeof(int), 1, file);

	// TYPE 0 --------------------------------------------
	if(type == 0 || type > 3) {
		system = SOLAR_SYSTEM_EPHEM();

		if(type > 3) {fclose(file); file = fopen(filepath,"rb");}	// no type specified in header
		fread(&bin_header.t0, sizeof(struct ItinStepBinHeaderT0), 1, file);
		num_deps = bin_header.t0.num_deps;

		int *bodies_id = (int *) malloc(bin_header.t0.num_layers * sizeof(int));
		fread(bodies_id, sizeof(int), bin_header.t0.num_layers, file);

		int buf;
		fread(&buf, sizeof(int), 1, file);

		if(buf != -1) {
			printf("Problems reading itinerary file (Body list or header wrong)\n");
			fclose(file);
			return (struct ItinsLoadFileResults){NULL, NULL, 0};
		}

		departures = (struct ItinStep **) malloc(bin_header.t0.num_deps * sizeof(struct ItinStep *));

		struct Body **bodies = (struct Body **) malloc(bin_header.t0.num_layers * sizeof(struct Body *));
		for(int i = 0; i < bin_header.t0.num_layers; i++)
			bodies[i] = (bodies_id[i] > 0) ? get_body_from_id(bodies_id[i]) : NULL;
		free(bodies_id);

		for(int i = 0; i < bin_header.t0.num_deps; i++) {
			departures[i] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
			departures[i]->prev = NULL;
			load_step_from_bfile(departures[i], file, bodies, system, type);
		}
		free(bodies);
		// TYPE 1 --------------------------------------------
	} else if(type == 1) {
		system = SOLAR_SYSTEM_EPHEM();

		fread(&bin_header.t1, sizeof(struct ItinStepBinHeaderT1), 1, file);
		num_deps = bin_header.t1.num_deps;

		int buf;
		fread(&buf, sizeof(int), 1, file);

		if(buf != -1) {
			printf("Problems reading itinerary file (Body list or header wrong)\n");
			fclose(file);
			return (struct ItinsLoadFileResults){NULL, NULL, 0};
		}

		departures = (struct ItinStep **) malloc(bin_header.t1.num_deps * sizeof(struct ItinStep *));

		for(int i = 0; i < bin_header.t1.num_deps; i++) {
			departures[i] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
			departures[i]->prev = NULL;
			load_step_from_bfile(departures[i], file, NULL, system, type);
		}
	} else if(type == 2) {
		fread(&bin_header.t2, sizeof(struct ItinStepBinHeaderT1), 1, file);
		num_deps = bin_header.t2.num_deps;

		int buf;
		fread(&buf, sizeof(int), 1, file);

		system = load_celestial_system_from_bfile(file, type);

		if(buf != -1) {
			printf("Problems reading itinerary file (Body list or header wrong)\n");
			fclose(file);
			return (struct ItinsLoadFileResults){NULL, NULL, 0};
		}

		departures = (struct ItinStep **) malloc(bin_header.t2.num_deps * sizeof(struct ItinStep *));

		for(int i = 0; i < bin_header.t2.num_deps; i++) {
			departures[i] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
			departures[i]->prev = NULL;
			load_step_from_bfile(departures[i], file, NULL, system, type);
		}
	}
	// ---------------------------------------------------

	fclose(file);
	return (struct ItinsLoadFileResults){departures, system, num_deps};
}

int get_num_of_deps_of_itinerary_from_bfile(char *filepath) {
	FILE *file;
	file = fopen(filepath,"rb");
	union ItinStepBinHeader bin_header;
	int num_deps = 0;

	int type;
	fread(&type, sizeof(int), 1, file);

	// TYPE 0 --------------------------------------------
	if(type == 0 || type > 3) {
		if(type > 3) {fclose(file); file = fopen(filepath,"rb");}	// no type specified in header
		fread(&bin_header.t0, sizeof(struct ItinStepBinHeaderT0), 1, file);
		num_deps = bin_header.t0.num_deps;
		// TYPE 1 --------------------------------------------
	} else if(type == 1) {
		fread(&bin_header.t1, sizeof(struct ItinStepBinHeaderT1), 1, file);
		num_deps = bin_header.t1.num_deps;
	}
	// ---------------------------------------------------

	fclose(file);
	return num_deps;
}


/*********************************************************************
 *                 Single Itinerary Binary Storing
 *********************************************************************/

void store_single_itinerary_in_bfile(struct ItinStep *itin, struct System *system, char *filepath) {
	if(itin == NULL) return;

	int num_nodes = get_num_of_itin_layers(itin);

	// Check if the string ends with ".itin"
	if (strlen(filepath) >= 5 && strcmp(filepath + strlen(filepath) - 5, ".itin") != 0) {
		// If not, append ".itin" to the string
		strcat(filepath, ".itin");
	}


	printf("Storing Itinerary: %s\n", filepath);

	FILE *file;
	file = fopen(filepath,"wb");

	fwrite(&num_nodes, sizeof(int), 1, file);

	struct ItinStep *ptr = itin;

	// same algorithm as layer counter (part of header)
	while(ptr != NULL) {
		int body_id = (ptr->body != NULL) ? ptr->body->id : 0;
		fwrite(&body_id, sizeof(int), 1, file);
		if(ptr->next != NULL) ptr = ptr->next[0];
		else break;
	}

	int end_of_bodies_designator = -1;
	fwrite(&end_of_bodies_designator, sizeof(int), 1, file);

	ptr = itin;
	while(ptr != NULL) {
		union ItinStepBin bin_step = convert_ItinStep_bin(ptr, system, 0);
		fwrite(&bin_step.t0, sizeof(struct ItinStepBinT0), 1, file);
		if(ptr->next != NULL) ptr = ptr->next[0];
		else break;
	}

	fclose(file);
}



/*********************************************************************
 *                 Single Itinerary Binary Loading
 *********************************************************************/


struct ItinStep * load_single_itinerary_from_bfile(char *filepath) {
	int num_nodes;
	struct System *system;

	printf("Loading Itinerary: %s\n", filepath);

	FILE *file;
	file = fopen(filepath,"rb");

	fread(&num_nodes, sizeof(int), 1, file);

	int *bodies_id = (int*) malloc(num_nodes * sizeof(int));
	fread(bodies_id, sizeof(int), num_nodes, file);

	int buf;
	fread(&buf, sizeof(int), 1, file);

	if(buf != -1) {
		printf("Problems reading itinerary file (Body list or header wrong)\n");
		fclose(file);
		return NULL;
	}


	struct Body **bodies = (struct Body**) malloc(num_nodes * sizeof(struct Body*));
	for(int i = 0; i < num_nodes; i++) bodies[i] = (bodies_id[i] > 0) ? get_body_from_id(bodies_id[i]) : NULL;
	free(bodies_id);

	struct ItinStep *itin;
	union ItinStepBin bin_step;
	struct ItinStep *last_step = NULL;

	for(int i = 0; i < num_nodes; i++) {
		itin = (struct ItinStep*) malloc(sizeof(struct ItinStep));
		fread(&bin_step.t0, sizeof(struct ItinStepBinT0), 1, file);
		convert_bin_ItinStep(bin_step, itin, bodies[i], system, 0);
		itin->prev = last_step;
		itin->next = NULL;
		if(last_step != NULL) {
			last_step->next = (struct ItinStep**) malloc(sizeof(struct ItinStep*));
			last_step->next[0] = itin;
		}
		last_step = itin;
	}

	fclose(file);
	free(bodies);
	return itin;
}
