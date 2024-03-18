//
// Created by niklas on 18.03.24.
//

#include "itin_tool.h"
#include "orbit.h"
#include "transfer_tools.h"
#include "tools/data_tool.h"
#include "double_swing_by.h"
#include <math.h>
#include <stdlib.h>





void find_viable_flybys(struct ItinStep *tf, struct Ephem **ephems, struct Body *next_body, double min_dt, double max_dt) {
	struct OSV osv_dep = osv_from_ephem(ephems[tf->body->id - 1], tf->date, SUN());
	struct OSV osv_arr0 = osv_from_ephem(ephems[next_body->id - 1], tf->date, SUN());
	struct Vector proj_vec = proj_vec_plane(osv_dep.r, constr_plane(vec(0,0,0), osv_arr0.r, osv_arr0.v));
	double theta_conj_opp = angle_vec_vec(proj_vec, osv_arr0.r);
	if(cross_product(proj_vec, osv_arr0.r).z < 0) theta_conj_opp *= -1;
	else theta_conj_opp -= M_PI;

	int max_new_steps = 10;
	struct ItinStep *new_steps[max_new_steps];

	int counter = 0;

	struct OSV osv_arr1 = propagate_orbit_theta(osv_arr0.r, osv_arr0.v, -theta_conj_opp, SUN());
	struct Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, SUN());
	struct Orbit arr1 = constr_orbit_from_osv(osv_arr1.r, osv_arr1.v, SUN());
	double dt0 = arr1.t-arr0.t;

	osv_arr1 = propagate_orbit_theta(osv_arr0.r, osv_arr0.v, -theta_conj_opp+M_PI, SUN());
	arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, SUN());
	arr1 = constr_orbit_from_osv(osv_arr1.r, osv_arr1.v, SUN());
	double dt1 = arr1.t-arr0.t;

	while(dt0 < 0) dt0 += arr0.period;
	while(dt1 < 0) dt1 += arr0.period;
	if(dt0 < dt1) {
		double temp = dt0;
		dt0 = dt1-arr0.period;
		dt1 = temp;
	} else {
		dt0 -= arr0.period;
	}

	while(dt1 < min_dt) {
		double temp = dt1;
		dt1 = dt0 + arr0.period;
		dt0 = temp;
	}


	// x: dt, y: diff_vinf (data[0].x: number of data points beginning at index 1)
	struct Vector2D data[101];

//	printf("%f %f %f\n---\n", dt0/86400, dt1/86400, arr0.period/86400);

	double t0 = tf->date;
	double last_dt, dt, t1, diff_vinf;

	struct Vector v_init = add_vectors(tf->v_arr, scalar_multiply(tf->v_body,-1));


	while(dt0 < max_dt) {
		data[0].x = 0;
		int right_side = 0;	// 0 = left, 1 = right

		for(int i = 0; i < 100; i++) {
			if(i == 0) dt = dt0;
			if(dt < min_dt) dt = min_dt;
			if(dt > max_dt) dt = max_dt;

			t1 = t0 + dt / 86400;

			struct OSV osv_arr = osv_from_ephem(ephems[next_body->id - 1], t1, SUN());

			struct Transfer new_transfer = calc_transfer(circfb, tf->body, next_body, osv_dep.r, osv_dep.v, osv_arr.r, osv_arr.v, dt,
														 NULL);

			struct Vector v_dep = add_vectors(new_transfer.v0, scalar_multiply(tf->v_body, -1));

			diff_vinf = vector_mag(v_dep) - vector_mag(v_init);

			if (fabs(diff_vinf) < 1) {
				double beta = (M_PI - angle_vec_vec(v_dep, v_init)) / 2;
				double rp = (1 / cos(beta) - 1) * (tf->body->mu / (pow(vector_mag(v_dep), 2)));
				if (rp > tf->body->radius + tf->body->atmo_alt) {
					new_steps[counter] = (struct ItinStep*) malloc(sizeof(struct ItinStep));
					new_steps[counter]->body = next_body;
					new_steps[counter]->date = t1;
					new_steps[counter]->r = osv_arr.r;
					new_steps[counter]->v_dep = new_transfer.v0;
					new_steps[counter]->v_arr = new_transfer.v1;
					new_steps[counter]->v_body = osv_arr.v;
					new_steps[counter]->num_next_nodes = 0;
					new_steps[counter]->prev = tf;
					new_steps[counter]->next = NULL;
					counter++;
				}

				if(!right_side) right_side = 1;
				else break;
			}


			insert_new_data_point(data, dt, diff_vinf);

			if(!can_be_negative_monot_deriv(data)) break;
			last_dt = dt;
			if(i == 0) dt = dt1;
			else dt = root_finder_monot_deriv_next_x(data, right_side ? 1 : 0);
			if(i > 3) {
				if(dt == last_dt) break;
				if(dt >= dt1) dt = (dt1+data[(int) data[0].x-1].x)/2;
				if(dt < dt0) dt = (dt0+data[2].x)/2;
			}
			if(isnan(dt) || isinf(dt)) break;
		}



		double temp = dt1;
		dt1 = dt0 + arr0.period;
		dt0 = temp;
	}

	if(counter > 0) {
		tf->next = (struct ItinStep **) malloc(counter * sizeof(struct ItinStep *));
		for(int i = 0; i < counter; i++) tf->next[i] = new_steps[i];
		tf->num_next_nodes = counter;
	}
}

void find_viable_dsb_flybys(struct ItinStep *tf, struct Ephem **ephems, struct Body *body1, double min_dt0, double max_dt0, double min_dt1, double max_dt1) {
	int max_new_steps0 = (int) (max_dt0/86400-min_dt0/86400+1);
	int max_new_steps1 = (int) (max_dt1/86400-min_dt1/86400+1);
	struct ItinStep *new_steps[max_new_steps0*max_new_steps1];
	struct Body *body0 = tf->body;

	int counter = 0;

	struct OSV s0 = {tf->r, tf->v_arr};
	struct OSV p0 = {tf->r, tf->v_body};
	struct OSV s1;
	double dt0, dt1, jd_sb2, jd_arr;


	for(int i = 0; i <= max_dt0/86400-min_dt0/86400; i++) {

		dt0 = min_dt0/86400 + i;
		jd_sb2 = tf->date+dt0;
		struct OSV osv_sb2 = osv_from_ephem(ephems[body0->id-1], jd_sb2, SUN());

		for(int j = 0; j <= max_dt1/86400-min_dt1/86400; j++) {
			dt1 = min_dt1/86400 + j;
			jd_arr = jd_sb2 + dt1;

			struct OSV osv_arr = osv_from_ephem(ephems[body1->id-1], jd_arr, SUN());
			struct Transfer transfer_after_dsb = calc_transfer(circfb, body0, body1, osv_sb2.r, osv_sb2.v, osv_arr.r, osv_arr.v, (jd_arr-jd_sb2)*86400, NULL);

			s1.r = transfer_after_dsb.r0;
			s1.v = transfer_after_dsb.v0;

			struct DSB dsb = calc_double_swing_by(s0, p0, s1, osv_sb2, dt0, body0);

			if(dsb.dv < 10000) {
				new_steps[counter] = (struct ItinStep*) malloc(sizeof(struct ItinStep));
				new_steps[counter]->body = NULL;
				new_steps[counter]->date = tf->date + dsb.man_time/86400;
				new_steps[counter]->r = dsb.osv[1].r;
				new_steps[counter]->v_dep = dsb.osv[0].v;
				new_steps[counter]->v_arr = dsb.osv[1].v;
				new_steps[counter]->v_body = vec(0,0,0);
				new_steps[counter]->num_next_nodes = 1;
				new_steps[counter]->prev = tf;
				new_steps[counter]->next = (struct ItinStep**) malloc(sizeof(struct ItinStep*));
				new_steps[counter]->next[0] = (struct ItinStep*) malloc(sizeof(struct ItinStep));
				new_steps[counter]->next[0]->prev = new_steps[counter];

				struct ItinStep *curr = new_steps[counter]->next[0];
				curr->body = body0;
				curr->date = jd_sb2;
				curr->r = osv_sb2.r;
				curr->v_dep = dsb.osv[2].v;
				curr->v_arr = dsb.osv[3].v;
				curr->v_body = osv_sb2.v;
				curr->num_next_nodes = 1;
				curr->next = (struct ItinStep**) malloc(sizeof(struct ItinStep*));
				curr->next[0] = (struct ItinStep*) malloc(sizeof(struct ItinStep));
				curr->next[0]->prev = curr;

				curr = curr->next[0];
				curr->body = body1;
				curr->date = jd_arr;
				curr->r = osv_arr.r;
				curr->v_dep = transfer_after_dsb.v0;
				curr->v_arr = transfer_after_dsb.v1;
				curr->v_body = osv_arr.v;
				curr->num_next_nodes = 0;
				curr->next = NULL;
				counter++;
			}
		}
	}

	if(counter > 0) {
		tf->next = (struct ItinStep **) malloc(counter * sizeof(struct ItinStep *));
		for(int i = 0; i < counter; i++) tf->next[i] = new_steps[i];
		tf->num_next_nodes = counter;
	}
}

void print_itinerary(struct ItinStep *itin) {
	if(itin->prev != NULL) {
		print_itinerary(itin->prev);
		printf(" - ");
	}
	print_date(convert_JD_date(itin->date), 0);
}

int get_number_of_itineraries(struct ItinStep *itin) {
	if(itin->num_next_nodes == 0) return 1;
	int counter = 0;
	for(int i = 0; i < itin->num_next_nodes; i++) {
		counter += get_number_of_itineraries(itin->next[i]);
	}
	return counter;
}

int get_total_number_of_stored_steps(struct ItinStep *itin) {
	if(itin->num_next_nodes == 0) return 1;
	int counter = 1;
	for(int i = 0; i < itin->num_next_nodes; i++) {
		counter += get_total_number_of_stored_steps(itin->next[i]);
	}
	return counter;
}

void store_itineraries_in_array(struct ItinStep *itin, struct ItinStep **array, int *index) {
	if(itin->num_next_nodes == 0) {
		array[*index] = itin;
		(*index)++;
	}

	for(int i = 0; i < itin->num_next_nodes; i++) {
		store_itineraries_in_array(itin->next[i], array, index);
	}
}

double get_itinerary_duration(struct ItinStep *itin) {
	double jd1 = itin->date;
	while(itin->prev != NULL) itin = itin->prev;
	double jd0 = itin->date;
	return jd1-jd0;
}

void create_porkchop_point(struct ItinStep *itin, double* porkchop, int circ_cap_fb) {
	double dv, vinf = vector_mag(add_vectors(itin->v_arr, scalar_multiply(itin->v_body,-1)));
	if(circ_cap_fb == 0) dv = dv_circ(itin->body, itin->body->atmo_alt+100e3, vinf);
	else if(circ_cap_fb == 1) dv = dv_capture(itin->body, itin->body->atmo_alt+100e3, vinf);
	else dv = 0;
	porkchop[4] = dv;
	porkchop[1] = get_itinerary_duration(itin);

	porkchop[3] = 0;

	while(itin->prev->prev != NULL) {
		if(itin->body == NULL) {
			porkchop[3] += vector_mag(add_vectors(itin->next[0]->v_dep, scalar_multiply(itin->v_arr,-1)));
		}
		itin = itin->prev;
	}

	vinf = vector_mag(add_vectors(itin->v_dep, scalar_multiply(itin->prev->v_body,-1)));
	porkchop[2] = dv_circ(itin->prev->body, itin->prev->body->atmo_alt+100e3, vinf);
	porkchop[0] = itin->prev->date;
}

int calc_next_step(struct ItinStep *curr_step, struct Ephem **ephems, struct Body **bodies, const int *min_duration, const int *max_duration, int num_steps, int step) {
	if(bodies[step] != bodies[step-1]) find_viable_flybys(curr_step, ephems, bodies[step], min_duration[step-1]*86400, max_duration[step-1]*86400);
	else {
		find_viable_dsb_flybys(curr_step, ephems, bodies[step+1],
							   min_duration[step-1]*86400, max_duration[step-1]*86400, min_duration[step]*86400, max_duration[step]*86400);
	}

	if(curr_step->num_next_nodes == 0) {
		remove_step_from_itinerary(curr_step);
		return 0;
	}

	int num_valid = 0;
	int init_num_nodes = curr_step->num_next_nodes;
	int step_del = 0;
	int result;

	if(step < num_steps-1) {
		for(int i = 0; i < init_num_nodes; i++) {
			if(bodies[step] != bodies[step-1]) result = calc_next_step(curr_step->next[i-step_del], ephems, bodies, min_duration, max_duration, num_steps, step+1);
			else result = calc_next_step(curr_step->next[i-step_del]->next[0]->next[0], ephems, bodies, min_duration, max_duration, num_steps, step+2);
			num_valid += result;
			if(!result) step_del++;
		}
	} else return 1;

	return num_valid > 0;
}

int get_num_of_itin_layers(struct ItinStep *step) {
	int counter = 0;
	while(step != NULL) {
		counter++;
		if(step->next != NULL) step = step->next[0];
		else break;
	}
	return counter;
}

void store_itineraries_in_file(struct ItinStep *step, FILE *file, int layer, int variation) {
	fprintf(file, "#%d#%d\n", layer, variation);
	fprintf(file, "Date: %f\n", step->date);
	fprintf(file, "r: %lf, %lf, %lf\n", step->r.x, step->r.y, step->r.z);
	fprintf(file, "v_dep: %f, %f, %f\n", step->v_dep.x, step->v_dep.y, step->v_dep.z);
	fprintf(file, "v_arr: %f, %f, %f\n", step->v_arr.x, step->v_arr.y, step->v_arr.z);
	fprintf(file, "v_body: %f, %f, %f\n", step->v_body.x, step->v_body.y, step->v_body.z);
	fprintf(file, "Next Steps: %d\n", step->num_next_nodes);

	for(int i = 0; i < step->num_next_nodes; i++) {
		store_itineraries_in_file(step->next[i], file, layer+1, i);
	}
}

void store_itineraries_in_file_init(struct ItinStep **departures, int num_nodes, int num_deps) {
	char filename[19];  // 14 for date + 4 for .csv + 1 for string terminator
	sprintf(filename, "test.transfer");
	int num_steps = get_num_of_itin_layers(departures[0]);
	printf("Filesize: ~%.3f MB\n", (double)num_nodes*240/1e6);

	FILE *file;
	file = fopen(filename,"w");

	fprintf(file, "Number of stored nodes: %d\n", num_nodes);
	fprintf(file, "Number of Departures: %d, Number of Steps: %d\n", num_deps, num_steps);

	struct ItinStep *ptr = departures[0];

	fprintf(file, "Bodies: ");
	while(ptr != NULL) {
		if(ptr->body != NULL) fprintf(file, "%d", ptr->body->id);
		else fprintf(file, "-");
		if(ptr->next != NULL) {
			fprintf(file, ",");
			ptr = ptr->next[0];
		} else {
			fprintf(file, "#\n");
			break;
		}
	}

	for(int i = 0; i < num_deps; i++) {
		store_itineraries_in_file(departures[i], file, 0, i);
	}

	fclose(file);
}

struct ItinStepBin {
	struct Vector r;
	struct Vector v_dep, v_arr, v_body;
	double date;
	int num_next_nodes;
};

struct ItinStepBin convert_ItinStep_bin(struct ItinStep *step) {
	struct ItinStepBin bin_step = {step->r, step->v_dep, step->v_arr, step->v_body, step->date, step->num_next_nodes};
	return bin_step;
}

void convert_bin_ItinStep(struct ItinStepBin bin_step, struct ItinStep *step, struct Body *body) {
	step->body = body;
	step->r = bin_step.r;
	step->v_arr = bin_step.v_arr;
	step->v_body = bin_step.v_body;
	step->v_dep = bin_step.v_dep;
	step->date = bin_step.date;
	step->num_next_nodes = bin_step.num_next_nodes;
}

void store_itineraries_in_bfile(struct ItinStep *step, FILE *file) {
	struct ItinStepBin bin_step = convert_ItinStep_bin(step);
	fwrite(&bin_step, sizeof(struct ItinStepBin), 1, file);

	for(int i = 0; i < step->num_next_nodes; i++) {
		store_itineraries_in_bfile(step->next[i], file);
	}
}

void store_itineraries_in_bfile_init(struct ItinStep **departures, int num_nodes, int num_deps) {
	char filename[19];  // 14 for date + 4 for .csv + 1 for string terminator
	sprintf(filename, "test.itins");

	printf("Filesize: ~%.3f MB\n", (double)num_nodes*110/1e6);

	int bin_header[] = {num_nodes, num_deps, get_num_of_itin_layers(departures[0])};
	printf("%d, %d, %d, %lu\n", num_nodes, num_deps, get_num_of_itin_layers(departures[0]), sizeof(struct ItinStepBin));
	FILE *file;
	file = fopen(filename,"wb");

	fwrite(bin_header, sizeof(bin_header), 1, file);

	struct ItinStep *ptr = departures[0];

	// same algorithm as layer counter (part of header)
	while(ptr != NULL) {
		int body_id = (ptr->body != NULL) ? ptr->body->id : 0;
		fwrite(&body_id, sizeof(int), 1, file);
		if(ptr->next != NULL) ptr = ptr->next[0];
		else break;
	}

	int end_of_bodies_designator = -1;
	fwrite(&end_of_bodies_designator, sizeof(int), 1, file);

	for(int i = 0; i < num_deps; i++) {
		store_itineraries_in_bfile(departures[i], file);
	}

	fclose(file);
}

void load_itineraries_from_bfile(struct ItinStep *step, FILE *file, struct Body **body) {
	struct ItinStepBin bin_step;
	fread(&bin_step, sizeof(struct ItinStepBin), 1, file);
	convert_bin_ItinStep(bin_step, step, body[0]);
	if(step->num_next_nodes > 0) step->next = (struct ItinStep**) malloc(step->num_next_nodes*sizeof(struct ItinStep*));
	else step->next = NULL;

	for(int i = 0; i < step->num_next_nodes; i++) {
		step->next[i] = (struct ItinStep*) malloc(sizeof(struct ItinStep));
		step->next[i]->prev = step;
		load_itineraries_from_bfile(step->next[i], file, body+1);
	}
}

struct ItinStep ** load_itineraries_from_bfile_init() {
	char filename[19];  // 14 for date + 4 for .csv + 1 for string terminator
	sprintf(filename, "test.itins");

	int bin_header[3];

	FILE *file;
	file = fopen(filename,"rb");

	fread(bin_header, sizeof(bin_header), 1, file);

	int *bodies_id = (int*) malloc(bin_header[2] * sizeof(int));
	fread(bodies_id, sizeof(int), bin_header[2], file);

	int temp;
	fread(&temp, sizeof(int), 1, file);

	if(temp != -1) {
		printf("Problems reading itinerary file (Body list or header wrong)\n");
		fclose(file);
		return NULL;
	}

	struct ItinStep **departures = (struct ItinStep**) malloc(bin_header[1] * sizeof(struct ItinStep*));

	struct Body **bodies = (struct Body**) malloc(bin_header[2] * sizeof(struct Body*));
	for(int i = 0; i < bin_header[2]; i++) bodies[i] = (bodies_id[i] > 0) ? get_body_from_id(bodies_id[i]) : NULL;
	free(bodies_id);

	for(int i = 0; i < bin_header[1]; i++) {
		departures[i] = (struct ItinStep*) malloc(sizeof(struct ItinStep));
		departures[i]->prev = NULL;
		load_itineraries_from_bfile(departures[i], file, bodies);
	}


	fclose(file);
	free(bodies);
	return departures;
}

void remove_step_from_itinerary(struct ItinStep *step) {
	struct ItinStep *prev = step->prev;

	while(prev->num_next_nodes == 1 && step->prev->prev != NULL) {
		step = prev;
		prev = step->prev;
	}

	int index = 0;
	for(int i = 0; i < prev->num_next_nodes; i++) {
		if(prev->next[i] == step) { index = i; break; }
	}
	prev->num_next_nodes--;
	for(int i = index; i < prev->num_next_nodes; i++) {
		prev->next[i] = prev->next[i+1];
	}
	free_itinerary(step);
}

void free_itinerary(struct ItinStep *step) {
	if(step->next != NULL) {
		for(int i = 0; i < step->num_next_nodes; i++) {
			free_itinerary(step->next[i]);
		}
		free(step->next);
	}
	free(step);
}
