#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <gtk/gtk.h>

#include "file_io.h"
#include "orbitlib_fileio.h"


typedef struct BinTypes {
	int file_type, header_type, body_type, system_type, itin_step_type;
} BinTypes;

BinTypes all_itins_file_types[] = {{4, 3, 3, 2, 2}, {3, 3, 2, 2, 2}, {2, 2, 2, 2, 2}};
const int num_all_itins_file_types = 3;
const BinTypes *current_itins_file_types = &(all_itins_file_types[0]);

BinTypes all_itin_file_types[] = {{1, 0, 3, 2, 0}, {0, 0, 2, 2, 0}};
const int num_all_itin_file_types = 2;
const BinTypes *current_itin_file_types = &(all_itin_file_types[0]);

int get_current_bin_file_type() {
	return current_itins_file_types->file_type;
}

BinTypes get_itins_file_bin_types_from_file_type(int file_type) {
	for(int i = 0; i < num_all_itins_file_types; i++) {
		if(file_type == all_itins_file_types[i].file_type) return all_itins_file_types[i];
	}
	return (BinTypes) {-1};
}

BinTypes get_itin_file_bin_types_from_file_type(int file_type) {
	for(int i = 0; i < num_all_itin_file_types; i++) {
		if(file_type == all_itin_file_types[i].file_type) return all_itin_file_types[i];
	}
	return (BinTypes) {-1};
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
 *          Celestial System Binary Storing and Loading
 *********************************************************************/
union CelestialSystemBin {
	struct CelestialSystemBinValid {
		int *ptr;
	} is_valid;

	struct CelestialSystemBinT2 {
		char name[50];
		int num_bodies;
		enum CelestSystemPropMethod calc_method;
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
	
	struct CelestialBodyBinT3 {
		char name[32];
		bool is_home_body;
		double color[3];
		int id;  // ID given by JPL's Horizon API
		double mu;
		double radius;
		double atmo_alt;
		double north_pole_ra, north_pole_decl, rot_ut0;
		double a, e, incl, raan, arg_peri, theta;
	} t3;
};

union CelestialSystemBin convert_celestial_system_bin(CelestSystem *system, int system_bin_type) {
	union CelestialSystemBin bin_system = {0};
	if(system_bin_type == 2) {
		sprintf(bin_system.t2.name, "%s", system->name);
		bin_system.t2.num_bodies = system->num_bodies;
		bin_system.t2.calc_method = system->prop_method;
		bin_system.t2.ut0 = system->ut0;
	}
	return bin_system;
}

CelestSystem * convert_bin_celestial_system(union CelestialSystemBin bin_system, int system_bin_type) {
	CelestSystem *system = new_system();
	if(system_bin_type == 2) {
		sprintf(system->name, "%s", bin_system.t2.name);
		system->num_bodies = bin_system.t2.num_bodies;
		if(system->num_bodies > 1e3) { free(system); return NULL; }    // avoid overflows
		system->prop_method = bin_system.t2.calc_method;
		system->ut0 = bin_system.t2.ut0;
	}
	system->bodies = (Body **) malloc(system->num_bodies*sizeof(Body *));
	return system;
}

union CelestialBodyBin convert_celestial_body_bin(Body *body, CelestSystem *system, int body_bin_type) {
	union CelestialBodyBin bin_body;
	if(body_bin_type == 2) {
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
		bin_body.t2.incl = body->orbit.i;
		bin_body.t2.raan = body->orbit.raan;
		bin_body.t2.arg_peri = body->orbit.arg_peri;
		bin_body.t2.theta = body->orbit.ta;
	} else if(body_bin_type == 3) {
		sprintf(bin_body.t3.name, "%s", body->name);
		bin_body.t3.color[0] = body->color[0];
		bin_body.t3.color[1] = body->color[1];
		bin_body.t3.color[2] = body->color[2];
		bin_body.t3.id = body->id;
		bin_body.t3.mu = body->mu;
		bin_body.t3.radius = body->radius;
		bin_body.t3.atmo_alt = body->atmo_alt;
		bin_body.t3.north_pole_ra = body->north_pole_ra;
		bin_body.t3.north_pole_decl = body->north_pole_decl;
		bin_body.t3.rot_ut0 = body->rot_ut0;
		bin_body.t3.a = body->orbit.a;
		bin_body.t3.e = body->orbit.e;
		bin_body.t3.incl = body->orbit.i;
		bin_body.t3.raan = body->orbit.raan;
		bin_body.t3.arg_peri = body->orbit.arg_peri;
		bin_body.t3.theta = body->orbit.ta;
		bin_body.t3.is_home_body = system->home_body == body;
	}
	return bin_body;
}

struct Body * convert_bin_celestial_body(union CelestialBodyBin bin_body, CelestSystem *system, int body_bin_type) {
	struct Body *body = new_body();
	if(body_bin_type == 2) {
		sprintf(body->name, "%s", bin_body.t2.name);
		body->color[0] = bin_body.t2.color[0];
		body->color[1] = bin_body.t2.color[1];
		body->color[2] = bin_body.t2.color[2];
		body->id = bin_body.t2.id;
		body->mu = bin_body.t2.mu;
		body->radius = bin_body.t2.radius;
		body->atmo_alt = bin_body.t2.atmo_alt;
		if(system->cb != NULL)
			body->orbit = constr_orbit_from_elements(
					bin_body.t2.a,
					bin_body.t2.e,
					bin_body.t2.incl,
					bin_body.t2.raan,
					bin_body.t2.arg_peri,
					bin_body.t2.theta,
					system->cb);
	} else if(body_bin_type == 3) {
		sprintf(body->name, "%s", bin_body.t3.name);
		body->color[0] = bin_body.t3.color[0];
		body->color[1] = bin_body.t3.color[1];
		body->color[2] = bin_body.t3.color[2];
		body->id = bin_body.t3.id;
		body->mu = bin_body.t3.mu;
		body->radius = bin_body.t3.radius;
		body->atmo_alt = bin_body.t3.atmo_alt;
		body->north_pole_ra = bin_body.t3.north_pole_ra;
		body->north_pole_decl = bin_body.t3.north_pole_decl;
		body->rot_ut0 = bin_body.t3.rot_ut0;
		if(bin_body.t3.is_home_body) system->home_body = body;
		if(system->cb != NULL)
			body->orbit = constr_orbit_from_elements(
					bin_body.t3.a,
					bin_body.t3.e,
					bin_body.t3.incl,
					bin_body.t3.raan,
					bin_body.t3.arg_peri,
					bin_body.t3.theta,
					system->cb);
	}
	return body;
}

void store_celestial_system_in_bfile(CelestSystem *system, FILE *file, BinTypes types) {
	union CelestialSystemBin system_bin = convert_celestial_system_bin(system, types.system_type);
	switch(types.system_type) {
		case 2: fwrite(&system_bin.t2, sizeof(struct CelestialSystemBinT2), 1, file); break;
	}

	union CelestialBodyBin body_bin = convert_celestial_body_bin(system->cb, system, types.body_type);
	switch(types.body_type) {
		case 2: fwrite(&body_bin.t2, sizeof(struct CelestialBodyBinT2), 1, file); break;
		case 3: fwrite(&body_bin.t3, sizeof(struct CelestialBodyBinT3), 1, file); break;
	}

	for(int i = 0; i < system->num_bodies; i++) {
		body_bin = convert_celestial_body_bin(system->bodies[i], system, types.body_type);
		switch(types.body_type) {
			case 2: fwrite(&body_bin.t2, sizeof(struct CelestialBodyBinT2), 1, file); break;
			case 3: fwrite(&body_bin.t3, sizeof(struct CelestialBodyBinT3), 1, file); break;
		}
	}
}

CelestSystem * load_celestial_system_from_bfile(FILE *file, BinTypes types) {
	CelestSystem *system = 0;
	union CelestialSystemBin system_bin;
	switch(types.system_type) {
		case 2: fread(&system_bin.t2, sizeof(struct CelestialSystemBinT2), 1, file); break;
	}
	system = convert_bin_celestial_system(system_bin, types.system_type);
	if(system == NULL) return NULL;

	union CelestialBodyBin body_bin;
	switch(types.body_type) {
		case 2: fread(&body_bin.t2, sizeof(struct CelestialBodyBinT2), 1, file); break;
		case 3: fread(&body_bin.t3, sizeof(struct CelestialBodyBinT3), 1, file); break;
	}
	system->cb = convert_bin_celestial_body(body_bin, system, types.body_type);

	for(int i = 0; i < system->num_bodies; i++) {
		switch(types.body_type) {
			case 2: fread(&body_bin.t2, sizeof(struct CelestialBodyBinT2), 1, file); break;
			case 3: fread(&body_bin.t3, sizeof(struct CelestialBodyBinT3), 1, file); break;
		}
		system->bodies[i] = convert_bin_celestial_body(body_bin, system, types.body_type);
	}

	if(system->prop_method == EPHEMS) {
		for(int i = 0; i < system->num_bodies; i++) {
			get_body_ephems(system->bodies[i], (Datetime){1950, 1, 1},(Datetime){1950, 1, 1}, (Datetime){0,1}, "../Ephemerides");
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
		int num_nodes, num_deps, num_itins;
		CalcDataBin calc_data;
	} t3;
};

union ItinStepBin {
	struct ItinStepBinValid {
		int *ptr;
	} is_valid;

	struct ItinStepBinT0 {
		Vector3 r;
		Vector3 v_dep, v_arr, v_body;
		double date;
		int num_next_nodes;
	} t0;

	struct ItinStepBinT2 {
		double date;
		Vector3 r;
		Vector3 v_dep, v_arr, v_body;
		int body_id;
		int num_next_nodes;
	} t2;
};

union ItinStepBin convert_ItinStep_bin(struct ItinStep *step, CelestSystem *system, int step_type) {
	union ItinStepBin bin_step;
	switch(step_type) {
		case 0:	bin_step.t0.r = step->r;
				bin_step.t0.v_dep = step->v_dep;
				bin_step.t0.v_arr = step->v_arr;
				bin_step.t0.v_body = step->v_body;
				bin_step.t0.date = step->date;
				bin_step.t0.num_next_nodes = step->num_next_nodes;
				break;
		case 2:	bin_step.t2.r = step->r;
				bin_step.t2.v_dep = step->v_dep;
				bin_step.t2.v_arr = step->v_arr;
				bin_step.t2.v_body = step->v_body;
				bin_step.t2.date = step->date;
				bin_step.t2.body_id = get_body_system_id(step->body, system);
				bin_step.t2.num_next_nodes = step->num_next_nodes;
				break;
		default: bin_step = (union ItinStepBin) {0}; break;
	}

	return bin_step;
}

void store_step_in_bfile(struct ItinStep *step, CelestSystem *system, FILE *file, int step_type) {
	union ItinStepBin bin_step;
	switch(step_type) {
		case 0:	bin_step = convert_ItinStep_bin(step, system, step_type);
				fwrite(&bin_step.t0, sizeof(struct ItinStepBinT0), 1, file);
				break;
		case 2: bin_step = convert_ItinStep_bin(step, system, step_type);
				fwrite(&bin_step.t2, sizeof(struct ItinStepBinT2), 1, file);
				break;
		default: break;
	}
	for(int i = 0; i < step->num_next_nodes; i++) {
		store_step_in_bfile(step->next[i], system, file, step_type);
	}
}

void store_itineraries_in_bfile(struct ItinStep **departures, int num_nodes, int num_deps, int num_itins, Itin_Calc_Data calc_data, CelestSystem *system, char *filepath, int file_type) {
	BinTypes bin_types = get_itins_file_bin_types_from_file_type(file_type);
	if(bin_types.file_type < 0) return;
	
	// Check if the string ends with ".itins"
	if (strlen(filepath) >= 6 && strcmp(filepath + strlen(filepath) - 6, ".itins") != 0) {
		// If not, append ".itins" to the string
		strcat(filepath, ".itins");
	}

	union ItinStepBinHeader bin_header;
	int end_of_header_designator = -1;
	FILE *file = fopen(filepath, "wb");
	fwrite(&file_type, sizeof(int), 1, file);

	switch(bin_types.header_type) {
		case 3:	bin_header.t3.num_nodes = num_nodes;
				bin_header.t3.num_deps = num_deps;
				bin_header.t3.num_itins = num_itins;
		
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
		
				store_celestial_system_in_bfile(system, file, bin_types);
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
					store_step_in_bfile(departures[i], system, file, bin_types.itin_step_type);
				}
				break;
		default: bin_header.is_valid.ptr = NULL;
	}
	// ---------------------------------------------------

	fclose(file);
}


/*********************************************************************
 *                 Multi-Itinerary Binary Loading
 *********************************************************************/

void convert_bin_ItinStep(union ItinStepBin bin_step, struct ItinStep *step, struct Body *body, CelestSystem *system, int step_type) {
	switch(step_type) {
		case 0:	step->body = body;
				step->r = bin_step.t0.r;
				step->v_arr = bin_step.t0.v_arr;
				step->v_body = bin_step.t0.v_body;
				step->v_dep = bin_step.t0.v_dep;
				step->date = bin_step.t0.date;
				step->num_next_nodes = bin_step.t0.num_next_nodes;
				break;
		case 2:	step->body = system->bodies[bin_step.t2.body_id];
				step->r = bin_step.t2.r;
				step->v_arr = bin_step.t2.v_arr;
				step->v_body = bin_step.t2.v_body;
				step->v_dep = bin_step.t2.v_dep;
				step->date = bin_step.t2.date;
				step->num_next_nodes = bin_step.t2.num_next_nodes;
				break;
		default: break;
	}
}

void load_step_from_bfile(struct ItinStep *step, FILE *file, struct Body **body, CelestSystem *system, int step_type) {
	union ItinStepBin bin_step;
	
	switch(step_type) {
		case 0:	fread(&bin_step.t0, sizeof(struct ItinStepBinT0), 1, file);
				convert_bin_ItinStep(bin_step, step, body[0], system, step_type);
				break;
		case 2:	fread(&bin_step.t2, sizeof(struct ItinStepBinT2), 1, file);
				convert_bin_ItinStep(bin_step, step, NULL, system, step_type);
				break;
		default: break;
	}

	if(step->num_next_nodes > 1e6) return;	// avoid overflows

	if(step->num_next_nodes > 0)
		step->next = (struct ItinStep **) malloc(step->num_next_nodes * sizeof(struct ItinStep *));
	else step->next = NULL;

	for(int i = 0; i < step->num_next_nodes; i++) {
		step->next[i] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
		step->next[i]->prev = step;
		load_step_from_bfile(step->next[i], file, body == NULL ? NULL : body + 1, system, step_type);
	}
}

ItinStepBinHeaderData get_itins_bfile_header(FILE *file) {
	int file_type;
	BinTypes bin_types;
	union ItinStepBinHeader bin_header;
	fread(&file_type, sizeof(int), 1, file);
	ItinStepBinHeaderData header_data = {.file_type = file_type, .num_deps = -1, .calc_data.seq_info.to_target.type = -1};
	
	bin_types = get_itins_file_bin_types_from_file_type(file_type);
	
	int buf;
	switch(bin_types.header_type) {
		case 2:	fread(&bin_header.t2, sizeof(struct ItinStepBinHeaderT2), 1, file);
				// avoid overflows
				if(bin_header.t2.num_deps > 1e10) return header_data;
				header_data.num_deps = bin_header.t2.num_deps;
				header_data.num_nodes = bin_header.t2.num_nodes;
		
				fread(&buf, sizeof(int), 1, file);
		
				if(buf != -1) {
					printf("Problems reading itinerary file (Body list or header wrong)\n");
					fclose(file);
					header_data.num_deps = -1;
					return header_data;
				}
		
				header_data.system = load_celestial_system_from_bfile(file, bin_types);
				break;
		case 3: fread(&bin_header.t3, sizeof(struct ItinStepBinHeaderT3), 1, file);
				// avoid overflows
				if(bin_header.t3.num_deps > 1e10)  return header_data;
				header_data.num_deps = bin_header.t3.num_deps;
				header_data.num_nodes = bin_header.t3.num_nodes;
				header_data.num_itins = bin_header.t3.num_itins;
				header_data.calc_data.jd_min_dep = bin_header.t3.calc_data.jd_min_dep;
				header_data.calc_data.jd_max_dep = bin_header.t3.calc_data.jd_max_dep;
				header_data.calc_data.jd_max_arr = bin_header.t3.calc_data.jd_max_arr;
				header_data.calc_data.max_duration = bin_header.t3.calc_data.max_duration;
				header_data.calc_data.dv_filter = bin_header.t3.calc_data.dv_filter;
				header_data.calc_data.max_num_waiting_orbits = bin_header.t3.calc_data.max_num_waiting_orbits;
				header_data.calc_data.step_dep_date = bin_header.t3.calc_data.step_dep_date;
				header_data.calc_data.num_deps_per_date = bin_header.t3.calc_data.num_deps_per_date;
		
				header_data.system = load_celestial_system_from_bfile(file, bin_types);
		
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
					free_celestial_system(header_data.system);
					header_data.num_deps = -1;
					return header_data;
				}
		
				fread(&buf, sizeof(int), 1, file);
		
				if(buf != -1) {
					printf("Problems reading itinerary file (Body list or header wrong)\n");
					fclose(file);
					free_celestial_system(header_data.system);
					if(header_data.calc_data.seq_info.to_target.type == ITIN_SEQ_INFO_TO_TARGET) free(header_data.calc_data.seq_info.to_target.flyby_bodies);
					if(header_data.calc_data.seq_info.spec_seq.type == ITIN_SEQ_INFO_SPEC_SEQ) free(header_data.calc_data.seq_info.spec_seq.bodies);
					header_data.num_deps = -1;
					return header_data;
				}
				break;
	}
	return header_data;
}

void print_header_data_to_string(ItinStepBinHeaderData header, char *string, enum DateType date_format) {
	sprintf(string, "Number of stored nodes: %d\n", header.num_nodes);
	sprintf(string, "%sNumber of Departures: %d\n", string, header.num_deps);
	if(header.file_type > 2) {
		sprintf(string, "%sNumber of Itineraries: %d\n", string, header.num_itins);
		char date_string[32];
		date_to_string(convert_JD_date(header.calc_data.jd_min_dep, date_format), date_string, 1);
		sprintf(string, "%sMin departure date: %s\n", string, date_string);
		date_to_string(convert_JD_date(header.calc_data.jd_max_dep, date_format), date_string, 1);
		sprintf(string, "%sMax departure date: %s\n", string, date_string);
		date_to_string(convert_JD_date(header.calc_data.jd_max_arr, date_format), date_string, 1);
		sprintf(string, "%sMax arrival date: %s\n", string, date_string);
		sprintf(string, "%sMax duration: %.2f days\n", string, header.calc_data.max_duration);
		sprintf(string, "%sDep date step: %.4f days\n", string, header.calc_data.step_dep_date);
		sprintf(string, "%sNum of deps per date: %d\n", string, header.calc_data.num_deps_per_date);
		sprintf(string, "%sMax num of waiting orbits: %d\n", string, header.calc_data.max_num_waiting_orbits);
		sprintf(string, "%sMax tot dv: %.0f m/s\n", string, header.calc_data.dv_filter.max_totdv);
		sprintf(string, "%sMax dep dv: %.0f m/s\n", string, header.calc_data.dv_filter.max_depdv);
		sprintf(string, "%sMax sat dv: %.0f m/s\n", string, header.calc_data.dv_filter.max_satdv);
		sprintf(string, "%sDeparture Periapsis: %.0fkm\n", string, header.calc_data.dv_filter.dep_periapsis / 1e3);
		sprintf(string, "%sArrival Periapsis: %.0fkm\n", string, header.calc_data.dv_filter.arr_periapsis / 1e3);
		if(header.calc_data.dv_filter.last_transfer_type == TF_FLYBY)
			sprintf(string, "%sLast transfer type: Fly-By\n", string);
		if(header.calc_data.dv_filter.last_transfer_type == TF_CAPTURE)
			sprintf(string, "%sLast transfer type: Capture\n", string);
		if(header.calc_data.dv_filter.last_transfer_type == TF_CIRC)
			sprintf(string, "%sLast transfer type: Circularization\n", string);

		if(header.calc_data.seq_info.to_target.type == ITIN_SEQ_INFO_TO_TARGET) {
			sprintf(string, "%sDeparture body: %s\n", string, header.calc_data.seq_info.to_target.dep_body->name);
			sprintf(string, "%sArrival body: %s\n", string, header.calc_data.seq_info.to_target.arr_body->name);
			sprintf(string, "%sAllowed Fly-By bodies:\n", string);
			for(int i = 0; i < header.calc_data.seq_info.to_target.num_flyby_bodies; i++) sprintf(string, "%s  - %s\n", string, header.calc_data.seq_info.to_target.flyby_bodies[i]->name);
		} else if(header.calc_data.seq_info.spec_seq.type == ITIN_SEQ_INFO_SPEC_SEQ) {
			sprintf(string, "%sSequence:\n", string);
			for(int i = 0; i < header.calc_data.seq_info.spec_seq.num_steps; i++) sprintf(string, "%s  %d. %s\n", string, i+1, header.calc_data.seq_info.spec_seq.bodies[i]->name);
		}
	} else {
		sprintf(string, "%s\nCan't find further analysis parameters because the analysis file\nwas created in version 1.2.2 or before\n", string);
	}
}

struct ItinsLoadFileResults load_itineraries_from_bfile(char *filepath) {
	struct ItinStep **departures = NULL;

	FILE *file;
	file = fopen(filepath,"rb");

	ItinStepBinHeaderData header_data = get_itins_bfile_header(file);
	BinTypes bin_types = get_itins_file_bin_types_from_file_type(header_data.file_type);

	char header_string[1024];
	print_header_data_to_string(header_data, header_string, DATE_ISO);
	printf("\n--\n%s--\n", header_string);

	if(header_data.num_deps < 0) return (struct ItinsLoadFileResults){header_data, NULL};

	departures = (struct ItinStep **) malloc(header_data.num_deps * sizeof(struct ItinStep *));

	for(int i = 0; i < header_data.num_deps; i++) {
		departures[i] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
		departures[i]->prev = NULL;
		load_step_from_bfile(departures[i], file, NULL, header_data.system, bin_types.itin_step_type);
	}
	// ---------------------------------------------------

	fclose(file);
	return (struct ItinsLoadFileResults){header_data, departures};
}


/*********************************************************************
 *                 Single Itinerary Binary Storing
 *********************************************************************/

void store_single_itinerary_in_bfile(struct ItinStep *itin, CelestSystem *system, char *filepath) {
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
	
	int file_start = -1;	// num_nodes cannot get negative, therefore (to keep backwards-compatible) set -1 at start to show that next number will be version
	fwrite(&file_start, sizeof(int), 1, file);
	int file_type = current_itin_file_types->file_type;
	fwrite(&file_type, sizeof(int), 1, file);

	fwrite(&num_nodes, sizeof(int), 1, file);

	store_celestial_system_in_bfile(system, file, *current_itin_file_types);

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
		union ItinStepBin bin_step = convert_ItinStep_bin(ptr, system, current_itin_file_types->itin_step_type);
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
	CelestSystem *system;

	printf("Loading Itinerary: %s\n", filepath);

	FILE *file;
	file = fopen(filepath,"rb");
	
	int buf;
	fread(&buf, sizeof(int), 1, file);
	int file_type = 0;
	
	if(buf == -1) {
		fread(&file_type, sizeof(int), 1, file);
		fread(&num_nodes, sizeof(int), 1, file);
	} else num_nodes = buf;
	
	BinTypes bin_types = get_itin_file_bin_types_from_file_type(file_type);
	
	system = load_celestial_system_from_bfile(file, bin_types);

	int *bodies_id = (int*) malloc(num_nodes * sizeof(int));
	fread(bodies_id, sizeof(int), num_nodes, file);

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
		convert_bin_ItinStep(bin_step, itin, system->bodies[bodies_id[i]], system, bin_types.itin_step_type);
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
