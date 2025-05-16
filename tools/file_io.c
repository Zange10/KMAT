#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <gtk/gtk.h>

#include "file_io.h"
#include "orbit_calculator/transfer_tools.h"

const int current_bin_file_type = 3;

int get_current_bin_file_type() {
	return current_bin_file_type;
}


void create_directory_if_not_exists(const char *path) {
	if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
		GError *error = NULL;
		if (g_mkdir_with_parents(path, 0755) == -1) {
			g_warning("Failed to create directory: %s", error->message);
			g_error_free(error);
		}
	}
}


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





/*********************************************************************
 *          Celestial System Config Storing and Loading
 *********************************************************************/

void store_body_in_config_file(FILE *file, struct Body *body, struct System *system) {
	fprintf(file, "[%s]\n", body->name);
	fprintf(file, "color = [%.3f, %.3f, %.3f]\n", body->color[0], body->color[1], body->color[2]);
	fprintf(file, "id = %d\n", body->id);
	fprintf(file, "gravitational_parameter = %.9g\n", body->mu);
	fprintf(file, "radius = %f\n", body->radius/1e3); // Convert from m to km
	fprintf(file, "rotational_period = %f\n", body->rotation_period);
	fprintf(file, "sea_level_pressure = %f\n", body->sl_atmo_p/1e3); // Convert from Pa to kPa
	fprintf(file, "scale_height = %.0f\n", body->scale_height);
	fprintf(file, "atmosphere_altitude = %f\n", body->atmo_alt/1e3); // Convert from m to km
	if(body != system->cb) {
		fprintf(file, "semi_major_axis = %f\n", body->orbit.a/1e3); // Convert from m to km
		fprintf(file, "eccentricity = %f\n", body->orbit.e);
		fprintf(file, "inclination = %f\n", rad2deg(body->orbit.inclination));
		fprintf(file, "raan = %f\n", rad2deg(body->orbit.raan));
		fprintf(file, "argument_of_periapsis = %f\n", rad2deg(body->orbit.arg_peri));
		fprintf(file, "true_anomaly_ut0 = %f\n", rad2deg(body->orbit.theta));
		fprintf(file, "parent_body = %s", body->orbit.body->name);
	}
}

void store_system_in_config_file(struct System *system) {
	char filename[50];
	sprintf(filename, "./Celestial_Systems/%s.cfg", system->name);

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

int get_key_and_value_from_config(char *key, char *value, char *line) {
	char *equalSign = strchr(line, '=');  // Find '='
	if (equalSign) {
		size_t keyLen = equalSign - line;
		strncpy(key, line, keyLen);  // Copy key
		key[keyLen] = '\0';  // Null-terminate
		if(isspace(key[keyLen-1])) key[keyLen-1] = '\0';	// remove space

		strcpy(value,  equalSign + (isspace(*(equalSign + 1)) ? 2 : 1));  // Copy value and skip space

		// remove line breaks
		size_t len = strlen(value);
		if (len > 0 && value[len - 1] == '\n') {
			value[len - 1] = '\0';  // Replace newline with null terminator
		}
		return 1;
	}
	return 0;
}

struct Body * load_body_from_config_file(FILE *file, struct System *system) {
	struct Body *body = new_body();
	double mean_anomaly = 0;
	double g_asl = 0;
	int has_mean_anomaly = 0;
	int has_g_asl = 0;
	int has_central_body_name = 0;
	char central_body_name[50];

	char line[256];  // Buffer for each line
	while (fgets(line, sizeof(line), file)) {
		if(strncmp(line, "[", 1) == 0) {
			sscanf(line, "[%50[^]]]", body->name);
		} else if(strcmp(line, "\n") == 0){
			break;
		} else {
			char key[50], value[50];
			if(get_key_and_value_from_config(key, value, line)) {
				if (strcmp(key, "color") == 0) {
					sscanf(value, "[%lf, %lf, %lf]", &body->color[0], &body->color[1], &body->color[2]);
				} else if (strcmp(key, "id") == 0) {
					sscanf(value, "%d", &body->id);
				} else if (strcmp(key, "gravitational_parameter") == 0) {
					sscanf(value, "%lg", &body->mu);
				} else if (strcmp(key, "g_asl") == 0) {
					sscanf(value, "%lg", &g_asl); has_g_asl = 1;
				} else if (strcmp(key, "radius") == 0) {
					sscanf(value, "%lf", &body->radius);
					body->radius *= 1e3;  // Convert from km to m
				} else if (strcmp(key, "rotational_period") == 0) {
					sscanf(value, "%lf", &body->rotation_period);
				} else if (strcmp(key, "sea_level_pressure") == 0) {
					sscanf(value, "%lf", &body->sl_atmo_p);
					body->sl_atmo_p *= 1e3;  // Convert from kPa to Pa
				} else if (strcmp(key, "scale_height") == 0) {
					sscanf(value, "%lf", &body->scale_height);
				} else if (strcmp(key, "atmosphere_altitude") == 0) {
					sscanf(value, "%lf", &body->atmo_alt);
					body->atmo_alt *= 1e3;  // Convert from km to m
				} else if (strcmp(key, "semi_major_axis") == 0) {
					sscanf(value, "%lf", &body->orbit.a);
					body->orbit.a *= 1e3;  // Convert from km to m
				} else if (strcmp(key, "eccentricity") == 0) {
					sscanf(value, "%lf", &body->orbit.e);
				} else if (strcmp(key, "inclination") == 0) {
					sscanf(value, "%lf", &body->orbit.inclination);
					body->orbit.inclination = deg2rad(body->orbit.inclination);
				} else if (strcmp(key, "raan") == 0) {
					sscanf(value, "%lf", &body->orbit.raan);
					body->orbit.raan = deg2rad(body->orbit.raan);
				} else if (strcmp(key, "argument_of_periapsis") == 0) {
					sscanf(value, "%lf", &body->orbit.arg_peri);
					body->orbit.arg_peri = deg2rad(body->orbit.arg_peri);
				} else if (strcmp(key, "true_anomaly_ut0") == 0) {
					sscanf(value, "%lf", &body->orbit.theta);
					body->orbit.theta = deg2rad(body->orbit.theta);
				} else if (strcmp(key, "mean_anomaly_ut0") == 0) {
					sscanf(value, "%lf", &mean_anomaly);
					has_mean_anomaly = 1;
				} else if (strcmp(key, "parent_body") == 0) {
					sscanf(value, "%s", central_body_name);
					has_central_body_name = 1;
				}
			}
		}
	}
	if(has_g_asl) body->mu = 9.81*g_asl * body->radius*body->radius;
	
	if(system != NULL) {
		struct Body *attractor = system->cb;
		if(has_central_body_name) {
			struct Body *attr_temp = get_body_by_name(central_body_name, system);
			if(attr_temp != NULL) attractor = attr_temp;
		}
		
		body->orbit = constr_orbit(
				body->orbit.a,
				body->orbit.e,
				body->orbit.inclination,
				body->orbit.raan,
				body->orbit.arg_peri,
				has_mean_anomaly ? calc_true_anomaly_from_mean_anomaly(body->orbit, mean_anomaly) : body->orbit.theta,
				attractor
		);
	}
	return body;
}

struct System * load_system_from_config_file(char *filename) {
	struct System *system = new_system();
	system->num_bodies = 0;
	system->calc_method = ORB_ELEMENTS;
	char central_body_name[50];

	FILE *file = fopen(filename, "r");
	if (!file) {
		perror("Failed to open file");
		free(system);
		return NULL;
	}

	char line[256];  // Buffer for each line
	while (fgets(line, sizeof(line), file)) {
		if(strncmp(line, "[", 1) == 0) {
			sscanf(line, "[%50[^]]]", system->name);
		} else if(strcmp(line, "\n") == 0){
			break;
		} else {
			char key[50], value[50];
			if(get_key_and_value_from_config(key, value, line)) {
				if (strcmp(key, "propagation_method") == 0) {
					if(strcmp(value, "EPHEMERIDES") == 0) system->calc_method = EPHEMS;
				} else if (strcmp(key, "ut0") == 0) {
					sscanf(value, "%lf", &system->ut0);
				} else if (strcmp(key, "number_of_bodies") == 0) {
					sscanf(value, "%d", &system->num_bodies);
				} else if (strcmp(key, "central_body") == 0) {
					sprintf(central_body_name, "%s", value);
				}
			}
		}
	}

	struct Body *cb = load_body_from_config_file(file, NULL);

	if(cb == NULL) {printf("Couldn't load Central Body!\n"); free(system); return NULL;}
	if(strcmp(central_body_name, cb->name) != 0) {printf("Central Body not in first position!\n"); free(system); free(cb); return NULL;}

	system->cb = cb;
	system->bodies = (struct Body**) calloc(system->num_bodies, sizeof(struct Body*));
	for(int i = 0; i < system->num_bodies; i++) system->bodies[i] = load_body_from_config_file(file, system);
	
	if(system->calc_method == EPHEMS) {
		for(int i = 0; i < system->num_bodies; i++) {
			get_body_ephems(system->bodies[i], system->bodies[i]->orbit.body);
			// Needed for orbit visualization scale
			struct OSV osv = osv_from_ephem(system->bodies[i]->ephem, system->ut0, system->bodies[i]->orbit.body);
			system->bodies[i]->orbit = constr_orbit_from_osv(osv.r, osv.v, system->bodies[i]->orbit.body);
		}
	}
	
	parse_and_sort_into_celestial_subsystems(system);

	fclose(file);

	return system;
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
		double atmo_alt;
		double a, e, incl, raan, arg_peri, theta;
	} t2;
};

union CelestialSystemBin convert_celestial_system_bin(struct System *system, int file_type) {
	union CelestialSystemBin bin_system;
	sprintf(bin_system.t2.name, "%s", system->name);
	bin_system.t2.num_bodies = system->num_bodies;
	bin_system.t2.calc_method = system->calc_method;
	bin_system.t2.ut0 = system->ut0;
	return bin_system;
}

struct System * convert_bin_celestial_system(union CelestialSystemBin bin_system, int file_type) {
	struct System *system = new_system();
	sprintf(system->name, "%s", bin_system.t2.name);
	system->num_bodies = bin_system.t2.num_bodies;
	if(system->num_bodies > 1e3) {free(system); return NULL;}	// avoid overflows
	system->calc_method = bin_system.t2.calc_method;
	system->ut0 = bin_system.t2.ut0;
	system->bodies = (struct Body**) malloc(system->num_bodies * sizeof(struct Body*));
	return system;
}

union CelestialBodyBin convert_celestial_body_bin(struct Body *body, int file_type) {
	union CelestialBodyBin bin_body;
	sprintf(bin_body.t2.name, "%s", body->name);
	bin_body.t2.color[0] = body->color[0];
	bin_body.t2.color[1] = body->color[1];
	bin_body.t2.color[2] = body->color[2];
	bin_body.t2.id = body->id;
	bin_body.t2.mu = body->mu;
	bin_body.t2.radius = body->radius;
	bin_body.t2.atmo_alt = body->atmo_alt;
	bin_body.t2.a = body->orbit.a;
	bin_body.t2.e = body->orbit.e;
	bin_body.t2.incl = body->orbit.inclination;
	bin_body.t2.raan = body->orbit.raan;
	bin_body.t2.arg_peri = body->orbit.arg_peri;
	bin_body.t2.theta = body->orbit.theta;
	return bin_body;
}

struct Body * convert_bin_celestial_body(union CelestialBodyBin bin_body, struct Body *attractor, int file_type) {
	struct Body *body = new_body();
	sprintf(body->name, "%s", bin_body.t2.name);
	body->color[0] = bin_body.t2.color[0];
	body->color[1] = bin_body.t2.color[1];
	body->color[2] = bin_body.t2.color[2];
	body->id = bin_body.t2.id;
	body->mu = bin_body.t2.mu;
	body->radius = bin_body.t2.radius;
	body->atmo_alt = bin_body.t2.atmo_alt;
	if(attractor != NULL)
		body->orbit = constr_orbit(
				bin_body.t2.a,
				bin_body.t2.e,
				bin_body.t2.incl,
				bin_body.t2.raan,
				bin_body.t2.arg_peri,
				bin_body.t2.theta,
				attractor);
	return body;
}

void store_celestial_system_in_bfile(struct System *system, FILE *file, int file_type) {
	union CelestialSystemBin system_bin = convert_celestial_system_bin(system, file_type);
	fwrite(&system_bin.t2, sizeof(struct CelestialSystemBinT2), 1, file);

	union CelestialBodyBin body_bin = convert_celestial_body_bin(system->cb, file_type);
	fwrite(&body_bin.t2, sizeof(struct CelestialBodyBinT2), 1, file);

	for(int i = 0; i < system->num_bodies; i++) {
		body_bin = convert_celestial_body_bin(system->bodies[i], file_type);
		fwrite(&body_bin.t2, sizeof(struct CelestialBodyBinT2), 1, file);
	}
}

struct System * load_celestial_system_from_bfile(FILE *file, int file_type) {
	struct System *system = 0;
	union CelestialSystemBin system_bin;
	fread(&system_bin.t2, sizeof(struct CelestialSystemBinT2), 1, file);
	system = convert_bin_celestial_system(system_bin, file_type);
	if(system == NULL) return NULL;

	union CelestialBodyBin body_bin;
	fread(&body_bin.t2, sizeof(struct CelestialBodyBinT2), 1, file);
	system->cb = convert_bin_celestial_body(body_bin, NULL, file_type);

	for(int i = 0; i < system->num_bodies; i++) {
		fread(&body_bin.t2, sizeof(struct CelestialBodyBinT2), 1, file);
		system->bodies[i] = convert_bin_celestial_body(body_bin, system->cb, file_type);
	}

	if(system->calc_method == EPHEMS) {
		for(int i = 0; i < system->num_bodies; i++) {
			get_body_ephems(system->bodies[i], system->cb);
		}
	}

	parse_and_sort_into_celestial_subsystems(system);

	return system;
}


/*********************************************************************
 *                 Multi-Itinerary Binary Storing
 *********************************************************************/

typedef struct {
	double jd_min_dep;
	double jd_max_dep;
	double jd_max_arr;
	double max_duration;
	double step_dep_date;
	int num_deps_per_date;
	int max_num_waiting_orbits;
	struct Dv_Filter dv_filter;
} CalcDataBin;


union ItinStepBinHeader {
	struct ItinStepBinHeaderValid {
		int *ptr;
	} is_valid;

	struct ItinStepBinHeaderT2 {
		int num_nodes, num_deps;
	} t2;

	struct ItinStepBinHeaderT3 {
		int num_nodes, num_deps;
		CalcDataBin calc_data;
	} t3;
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

	struct ItinStepBinT2 {
		double date;
		struct Vector r;
		struct Vector v_dep, v_arr, v_body;
		int body_id;
		int num_next_nodes;
	} t2;
};

union ItinStepBin convert_ItinStep_bin(struct ItinStep *step, struct System *system, int file_type) {
	union ItinStepBin bin_step;
	if(file_type == 0) {
		bin_step.t0.r = step->r;
		bin_step.t0.v_dep = step->v_dep;
		bin_step.t0.v_arr = step->v_arr;
		bin_step.t0.v_body = step->v_body;
		bin_step.t0.date = step->date;
		bin_step.t0.num_next_nodes = step->num_next_nodes;
	} else {
		bin_step.t2.r = step->r;
		bin_step.t2.v_dep = step->v_dep;
		bin_step.t2.v_arr = step->v_arr;
		bin_step.t2.v_body = step->v_body;
		bin_step.t2.date = step->date;
		bin_step.t2.body_id = get_body_system_id(step->body, system);
		bin_step.t2.num_next_nodes = step->num_next_nodes;
	}

	return bin_step;
}

void store_step_in_bfile(struct ItinStep *step, struct System *system, FILE *file, int file_type) {
	if(file_type == 0) {
		union ItinStepBin bin_step = convert_ItinStep_bin(step, system, file_type);
		fwrite(&bin_step.t0, sizeof(struct ItinStepBinT0), 1, file);
	} else {
		union ItinStepBin bin_step = convert_ItinStep_bin(step, system, file_type);
		fwrite(&bin_step.t2, sizeof(struct ItinStepBinT2), 1, file);
	}
	for(int i = 0; i < step->num_next_nodes; i++) {
		store_step_in_bfile(step->next[i], system, file, file_type);
	}
}

void store_itineraries_in_bfile(struct ItinStep **departures, int num_nodes, int num_deps, Itin_Calc_Data calc_data, struct System *system, char *filepath, int file_type) {
	// Check if the string ends with ".itins"
	if (strlen(filepath) >= 6 && strcmp(filepath + strlen(filepath) - 6, ".itins") != 0) {
		// If not, append ".itins" to the string
		strcat(filepath, ".itins");
	}

	union ItinStepBinHeader bin_header;
	int end_of_header_designator = -1;
	FILE *file = fopen(filepath, "wb");
	fwrite(&file_type, sizeof(int), 1, file);

	if(file_type == 2) {
		bin_header.t2.num_nodes = num_nodes;
		bin_header.t2.num_deps = num_deps;
		
		printf("Filesize: ~%.3f MB\n", (double) num_nodes*sizeof(struct ItinStepBinT2)/1e6);
		
		printf("Number of stored nodes: %d\n", bin_header.t2.num_nodes);
		printf("Number of Departures: %d\n", num_deps);
		
		fwrite(&bin_header.t2, sizeof(struct ItinStepBinHeaderT2), 1, file);
		
		fwrite(&end_of_header_designator, sizeof(int), 1, file);
		
		store_celestial_system_in_bfile(system, file, file_type);
		
		for(int i = 0; i < num_deps; i++) {
			store_step_in_bfile(departures[i], system, file, file_type);
		}
	} else if(file_type == 3) {
		bin_header.t3.num_nodes = num_nodes;
		bin_header.t3.num_deps = num_deps;

		bin_header.t3.calc_data = (CalcDataBin) {
			.jd_min_dep = calc_data.jd_min_dep,
			.jd_max_dep = calc_data.jd_max_dep,
			.jd_max_arr = calc_data.jd_max_arr,
			.max_duration = calc_data.max_duration,
			.step_dep_date = calc_data.step_dep_date,
			.num_deps_per_date = calc_data.num_deps_per_date,
			.max_num_waiting_orbits = calc_data.max_num_waiting_orbits,
			.dv_filter = calc_data.dv_filter
		};

		printf("Filesize: ~%.3f MB\n", (double) num_nodes*sizeof(struct ItinStepBinT2)/1e6);

		fwrite(&bin_header.t3, sizeof(struct ItinStepBinHeaderT3), 1, file);

		store_celestial_system_in_bfile(system, file, file_type);
		if(calc_data.seq_info.to_target.type == ITIN_SEQ_INFO_TO_TARGET) {
			fwrite(&calc_data.seq_info.to_target.type, sizeof(int), 1, file);
			fwrite(&calc_data.seq_info.to_target.num_flyby_bodies, sizeof(int), 1, file);
			int *body_ids = malloc((calc_data.seq_info.to_target.num_flyby_bodies + 2) * sizeof(int));
			body_ids[0] = get_body_system_id(calc_data.seq_info.to_target.dep_body, system);
			body_ids[1] = get_body_system_id(calc_data.seq_info.to_target.arr_body, system);
			for(int i = 0; i < calc_data.seq_info.to_target.num_flyby_bodies; i++) body_ids[i+2] = get_body_system_id(calc_data.seq_info.to_target.flyby_bodies[i], system);
			fwrite(body_ids, sizeof(int), calc_data.seq_info.to_target.num_flyby_bodies + 2, file);
			free(body_ids);
		} else {
			fwrite(&calc_data.seq_info.spec_seq.type, sizeof(int), 1, file);
			fwrite(&calc_data.seq_info.spec_seq.num_steps, sizeof(int), 1, file);
			int *body_ids = malloc((calc_data.seq_info.spec_seq.num_steps) * sizeof(int));
			for(int i = 0; i < calc_data.seq_info.spec_seq.num_steps; i++) body_ids[i] = get_body_system_id(calc_data.seq_info.spec_seq.bodies[i], system);
			fwrite(body_ids, sizeof(int), calc_data.seq_info.spec_seq.num_steps, file);
			free(body_ids);
		}

		fwrite(&end_of_header_designator, sizeof(int), 1, file);

		for(int i = 0; i < num_deps; i++) {
			store_step_in_bfile(departures[i], system, file, file_type);
		}
	} else {
		bin_header.is_valid.ptr = NULL;
	}
	// ---------------------------------------------------

	fclose(file);
}


/*********************************************************************
 *                 Multi-Itinerary Binary Loading
 *********************************************************************/

void convert_bin_ItinStep(union ItinStepBin bin_step, struct ItinStep *step, struct Body *body, struct System *system, int file_type) {
	if(file_type == 0) {
		step->body = body;
		step->r = bin_step.t0.r;
		step->v_arr = bin_step.t0.v_arr;
		step->v_body = bin_step.t0.v_body;
		step->v_dep = bin_step.t0.v_dep;
		step->date = bin_step.t0.date;
		step->num_next_nodes = bin_step.t0.num_next_nodes;
	} else {
		step->body = system->bodies[bin_step.t2.body_id];
		step->r = bin_step.t2.r;
		step->v_arr = bin_step.t2.v_arr;
		step->v_body = bin_step.t2.v_body;
		step->v_dep = bin_step.t2.v_dep;
		step->date = bin_step.t2.date;
		step->num_next_nodes = bin_step.t2.num_next_nodes;
	}
}

void load_step_from_bfile(struct ItinStep *step, FILE *file, struct Body **body, struct System *system, int file_type) {
	union ItinStepBin bin_step;
	
	if(file_type == 0) {
		fread(&bin_step.t0, sizeof(struct ItinStepBinT0), 1, file);
		convert_bin_ItinStep(bin_step, step, body[0], system, file_type);
	} else {
		fread(&bin_step.t2, sizeof(struct ItinStepBinT2), 1, file);
		convert_bin_ItinStep(bin_step, step, NULL, system, file_type);
	}

	if(step->num_next_nodes > 1e6) return;	// avoid overflows

	if(step->num_next_nodes > 0)
		step->next = (struct ItinStep **) malloc(step->num_next_nodes * sizeof(struct ItinStep *));
	else step->next = NULL;

	for(int i = 0; i < step->num_next_nodes; i++) {
		step->next[i] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
		step->next[i]->prev = step;
		load_step_from_bfile(step->next[i], file, body == NULL ? NULL : body + 1, system, file_type);
	}
}

ItinStepBinHeaderData get_itins_bfile_header(FILE *file) {
	int type;
	union ItinStepBinHeader bin_header;
	fread(&type, sizeof(int), 1, file);
	ItinStepBinHeaderData header_data = {.file_type = type, .num_deps = -1, .calc_data.seq_info.to_target.type = -1};

	// currently only type 2 available
	if(type == 2) {
		fread(&bin_header.t2, sizeof(struct ItinStepBinHeaderT2), 1, file);
		// avoid overflows
		if(bin_header.t2.num_deps > 1e10) return header_data;
		header_data.num_deps = bin_header.t2.num_deps;
		header_data.num_nodes = bin_header.t2.num_nodes;

		int buf;
		fread(&buf, sizeof(int), 1, file);

		if(buf != -1) {
			printf("Problems reading itinerary file (Body list or header wrong)\n");
			fclose(file);
			header_data.num_deps = -1;
			return header_data;
		}

		header_data.system = load_celestial_system_from_bfile(file, type);
	} else if(type == 3) {

		fread(&bin_header.t3, sizeof(struct ItinStepBinHeaderT3), 1, file);
		// avoid overflows
		if(bin_header.t3.num_deps > 1e10)  return header_data;
		header_data.num_deps = bin_header.t3.num_deps;
		header_data.num_nodes = bin_header.t3.num_nodes;
		header_data.calc_data.jd_min_dep = bin_header.t3.calc_data.jd_min_dep;
		header_data.calc_data.jd_max_dep = bin_header.t3.calc_data.jd_max_dep;
		header_data.calc_data.jd_max_arr = bin_header.t3.calc_data.jd_max_arr;
		header_data.calc_data.max_duration = bin_header.t3.calc_data.max_duration;
		header_data.calc_data.dv_filter = bin_header.t3.calc_data.dv_filter;
		header_data.calc_data.max_num_waiting_orbits = bin_header.t3.calc_data.max_num_waiting_orbits;
		header_data.calc_data.step_dep_date = bin_header.t3.calc_data.step_dep_date;
		header_data.calc_data.num_deps_per_date = bin_header.t3.calc_data.num_deps_per_date;

		header_data.system = load_celestial_system_from_bfile(file, type);

		int calc_type;
		fread(&calc_type, sizeof(int), 1, file);
		header_data.calc_data.seq_info.to_target.type = calc_type;

		if(calc_type == ITIN_SEQ_INFO_TO_TARGET) {
			int num_flyby_bodies;
			fread(&num_flyby_bodies, sizeof(int), 1, file);
			header_data.calc_data.seq_info.to_target.num_flyby_bodies = num_flyby_bodies;
			int *body_ids = malloc((num_flyby_bodies + 2) * sizeof(int));
			fread(body_ids, sizeof(int), num_flyby_bodies + 2, file);
			header_data.calc_data.seq_info.to_target.flyby_bodies = malloc((num_flyby_bodies) * sizeof(struct Body*));
			header_data.calc_data.seq_info.to_target.dep_body = header_data.system->bodies[body_ids[0]];
			header_data.calc_data.seq_info.to_target.arr_body = header_data.system->bodies[body_ids[1]];
			for(int i = 0; i < num_flyby_bodies; i++) header_data.calc_data.seq_info.to_target.flyby_bodies[i] = header_data.system->bodies[body_ids[i+2]];
			free(body_ids);
			header_data.calc_data.seq_info.to_target.system = header_data.system;
		} else if(calc_type == ITIN_SEQ_INFO_SPEC_SEQ){
			int num_steps;
			fread(&num_steps, sizeof(int), 1, file);
			header_data.calc_data.seq_info.spec_seq.num_steps = num_steps;
			int *body_ids = malloc(num_steps * sizeof(int));
			fread(body_ids, sizeof(int), num_steps, file);
			header_data.calc_data.seq_info.spec_seq.bodies = malloc((num_steps) * sizeof(struct Body*));
			for(int i = 0; i < num_steps; i++) header_data.calc_data.seq_info.spec_seq.bodies[i] = header_data.system->bodies[body_ids[i]];
			free(body_ids);
			header_data.calc_data.seq_info.spec_seq.system = header_data.system;
		} else {
			free_system(header_data.system);
			header_data.num_deps = -1;
			return header_data;
		}

		int buf;
		fread(&buf, sizeof(int), 1, file);

		if(buf != -1) {
			printf("Problems reading itinerary file (Body list or header wrong)\n");
			fclose(file);
			free_system(header_data.system);
			if(header_data.calc_data.seq_info.to_target.type == ITIN_SEQ_INFO_TO_TARGET) free(header_data.calc_data.seq_info.to_target.flyby_bodies);
			if(header_data.calc_data.seq_info.spec_seq.type == ITIN_SEQ_INFO_SPEC_SEQ) free(header_data.calc_data.seq_info.spec_seq.bodies);
			header_data.num_deps = -1;
			return header_data;
		}
	}
	return header_data;
}

void print_header_data_to_string(ItinStepBinHeaderData header, char *string, enum DateType date_format) {
	sprintf(string, "Number of stored nodes: %d\n", header.num_nodes);
	sprintf(string, "%sNumber of Departures: %d\n", string, header.num_deps);
	if(header.file_type > 2) {
		char date_string[32];
		date_to_string(convert_JD_date(header.calc_data.jd_min_dep, date_format), date_string, 1);
		sprintf(string, "%sMin departure date: %s\n", string, date_string);
		date_to_string(convert_JD_date(header.calc_data.jd_max_dep, date_format), date_string, 1);
		sprintf(string, "%sMax departure date: %s\n", string, date_string);
		date_to_string(convert_JD_date(header.calc_data.jd_max_arr, date_format), date_string, 1);
		sprintf(string, "%sMax arrival date: %s\n", string, date_string);
		sprintf(string, "%sMax duration: %f\n", string, header.calc_data.max_duration);
		sprintf(string, "%sDep date step: %f\n", string, header.calc_data.step_dep_date);
		sprintf(string, "%s# of deps per date: %d\n", string, header.calc_data.num_deps_per_date);
		sprintf(string, "%sMax # waiting orbits: %d\n", string, header.calc_data.max_num_waiting_orbits);
		sprintf(string, "%sMax tot dv: %.0f m/s\n", string, header.calc_data.dv_filter.max_totdv);
		sprintf(string, "%sMax dep dv: %.0f m/s\n", string, header.calc_data.dv_filter.max_depdv);
		sprintf(string, "%sMax sat dv: %.0f m/s\n", string, header.calc_data.dv_filter.max_satdv);
		sprintf(string, "%sDeparture Periapsis: %.0fkm\n", string, header.calc_data.dv_filter.dep_periapsis / 1e3);
		sprintf(string, "%sArrival Periapsis: %.0fkm\n", string, header.calc_data.dv_filter.arr_periapsis / 1e3);
		sprintf(string, "%sLast transfer type: %d\n", string, header.calc_data.dv_filter.last_transfer_type);

		if(header.calc_data.seq_info.to_target.type == ITIN_SEQ_INFO_TO_TARGET) {
			sprintf(string, "%sDeparture body: %s\n", string, header.calc_data.seq_info.to_target.dep_body->name);
			sprintf(string, "%sArrival body: %s\n", string, header.calc_data.seq_info.to_target.arr_body->name);
			sprintf(string, "%sAllowed Fly-By bodies:\n", string);
			for(int i = 0; i < header.calc_data.seq_info.to_target.num_flyby_bodies; i++) sprintf(string, "%s  - %s\n", string, header.calc_data.seq_info.to_target.flyby_bodies[i]->name);
		} else if(header.calc_data.seq_info.spec_seq.type == ITIN_SEQ_INFO_SPEC_SEQ) {
			sprintf(string, "%sSequence:\n", string);
			for(int i = 0; i < header.calc_data.seq_info.spec_seq.num_steps; i++) sprintf(string, "%s  %d. %s\n", string, i+1, header.calc_data.seq_info.spec_seq.bodies[i]->name);
		}
	}
}

struct ItinsLoadFileResults load_itineraries_from_bfile(char *filepath) {
	struct ItinStep **departures = NULL;

	FILE *file;
	file = fopen(filepath,"rb");

	ItinStepBinHeaderData header_data = get_itins_bfile_header(file);

	char header_string[1024];
	print_header_data_to_string(header_data, header_string, DATE_ISO);
	printf("\n--\n%s--\n", header_string);

	if(header_data.num_deps < 0) return (struct ItinsLoadFileResults){header_data, NULL};

	departures = (struct ItinStep **) malloc(header_data.num_deps * sizeof(struct ItinStep *));

	for(int i = 0; i < header_data.num_deps; i++) {
		departures[i] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
		departures[i]->prev = NULL;
		load_step_from_bfile(departures[i], file, NULL, header_data.system, header_data.file_type);
	}
	// ---------------------------------------------------

	fclose(file);
	return (struct ItinsLoadFileResults){header_data, departures};
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

	store_celestial_system_in_bfile(system, file, 2);

	struct ItinStep *ptr = itin;

	// same algorithm as layer counter (part of header)
	while(ptr != NULL) {
		int body_id = (ptr->body != NULL) ? get_body_system_id(ptr->body,system) : -1;
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


struct ItinLoadFileResults load_single_itinerary_from_bfile(char *filepath) {
	int num_nodes;
	struct System *system;

	printf("Loading Itinerary: %s\n", filepath);

	FILE *file;
	file = fopen(filepath,"rb");

	fread(&num_nodes, sizeof(int), 1, file);

	system = load_celestial_system_from_bfile(file, 2);

	int *bodies_id = (int*) malloc(num_nodes * sizeof(int));
	fread(bodies_id, sizeof(int), num_nodes, file);

	int buf;
	fread(&buf, sizeof(int), 1, file);

	if(buf != -1) {
		printf("Problems reading itinerary file (Body list or header wrong)\n");
		fclose(file);
		return (struct ItinLoadFileResults) {.itin = NULL, .system = NULL};
	}

	struct ItinStep *itin;
	union ItinStepBin bin_step;
	struct ItinStep *last_step = NULL;

	for(int i = 0; i < num_nodes; i++) {
		itin = (struct ItinStep*) malloc(sizeof(struct ItinStep));
		fread(&bin_step.t0, sizeof(struct ItinStepBinT0), 1, file);
		convert_bin_ItinStep(bin_step, itin, system->bodies[bodies_id[i]], system, 0);
		itin->prev = last_step;
		itin->next = NULL;
		if(last_step != NULL) {
			last_step->next = (struct ItinStep**) malloc(sizeof(struct ItinStep*));
			last_step->next[0] = itin;
		}
		last_step = itin;
	}

	fclose(file);
	free(bodies_id);
	return (struct ItinLoadFileResults) {.itin = itin, .system = system};
}
