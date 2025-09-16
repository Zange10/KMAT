#include "itin_tool.h"
#include "double_swing_by.h"
#include "tools/thread_pool.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>


const int MIN_TRANSFER_DURATION = 10;	// days

void calc_time_to_next_conjunction_and_opposition(Vector3 r0, OSV osv0_next, Body *cb, double *next_conjunction_dt, double *next_opposition_dt) {
	Vector3 proj_vec = proj_vec3_plane3(r0, constr_plane3(vec3(0,0,0), osv0_next.r, osv0_next.v));
	double dta_conj = angle_vec3_vec3(proj_vec, osv0_next.r);
	if(cross_vec3(osv0_next.r, proj_vec).z < 0) dta_conj = 2*M_PI-dta_conj;
	double dta_opp = dta_conj > M_PI ? dta_conj-M_PI : dta_conj+M_PI;
	
	Orbit arr0 = constr_orbit_from_osv(osv0_next.r, osv0_next.v, cb);
	double period_arr0 = calc_orbital_period(arr0);
	double tpe_arr0 = calc_orbit_time_since_periapsis(arr0);
	
	Orbit arr_conj = arr0; arr_conj.ta = pi_norm(arr_conj.ta+dta_conj);
	Orbit arr_opp = arr0; arr_opp.ta = pi_norm(arr_opp.ta+dta_opp);
	double dt_conj = calc_orbit_time_since_periapsis(arr_conj)-tpe_arr0;
	double dt_opp = calc_orbit_time_since_periapsis(arr_opp)-tpe_arr0;
	
	if(dt_conj < 0) dt_conj += period_arr0;
	if(dt_opp < 0)	dt_opp += period_arr0;
	
	*next_conjunction_dt = dt_conj;
	*next_opposition_dt = dt_opp;
}

void find_viable_flybys(struct ItinStep *tf, CelestSystem *system, Body *next_body, double min_dt, double max_dt) {
	OSV osv_dep = {tf->r, tf->v_body};
	OSV osv_arr0 = system->prop_method == ORB_ELEMENTS ?
				   osv_from_elements(next_body->orbit, tf->date) :
				   osv_from_ephem(next_body->ephem, next_body->num_ephems, tf->date, system->cb);
	double next_conjunction_dt, next_opposition_dt;
	calc_time_to_next_conjunction_and_opposition(osv_dep.r, osv_arr0, system->cb, &next_conjunction_dt, &next_opposition_dt);
	Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, system->cb);
	double period_arr0 = calc_orbital_period(arr0);
	
	double dt0, dt1;
	if(next_conjunction_dt < next_opposition_dt) {
		dt0 = next_opposition_dt - period_arr0;
		dt1 = next_conjunction_dt;
	} else {
		dt0 = next_conjunction_dt - period_arr0;
		dt1 = next_opposition_dt;
	}
	
	// x: dt, y: diff_vinf
	DataArray2 *data = data_array2_create();
	int max_new_steps = 100;
	struct ItinStep *new_steps[max_new_steps];
	int counter = 0;

	double t0 = tf->date;
	double last_dt, dt, t1, diff_vinf;

	Vector3 v_init = subtract_vec3(tf->v_arr, tf->v_body);

	while(dt0 < max_dt && counter < max_new_steps) {
		int right_side = 0;	// 0 = left, 1 = right

		for(int i = 0; i < 100; i++) {
			if(i == 0) dt = dt0;
			if(dt < min_dt) dt = min_dt;
			if(dt > max_dt) dt = max_dt;

			t1 = t0 + dt / 86400;

			struct OSV osv_arr = system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(next_body->orbit, t1) :
					osv_from_ephem(next_body->ephem, next_body->num_ephems, t1, system->cb);

			Lambert3 new_transfer = calc_lambert3(osv_dep.r, osv_arr.r, dt, system->cb);
			Vector3 v_dep = subtract_vec3(new_transfer.v0, tf->v_body);
			diff_vinf = mag_vec3(v_dep) - mag_vec3(v_init);

			if(fabs(diff_vinf) < 1) {
				if(get_flyby_periapsis(tf->v_arr, new_transfer.v0, tf->v_body, tf->body) > altatmo2radius(tf->body, 10e3)) {	// +10e3 to avoid precision errors when checking for fly-by viability later on
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


			data_array2_insert_new(data, dt, diff_vinf);

			if(!can_be_negative_monot_deriv(data)) break;
			last_dt = dt;
			if(i == 0) dt = dt1;
			else dt = root_finder_monot_deriv_next_x(data, right_side ? 1 : 0);
			if(i > 3 && dt == last_dt) break;	// step size 0 (imprecision)
			if(isnan(dt) || isinf(dt)) break;
		}

		data_array2_clear(data);

		double temp = dt1;
		dt1 = dt0 + period_arr0;
		dt0 = temp;
	}
	
	data_array2_free(data);

	if(counter > 0) {
		if(tf->num_next_nodes == 0) tf->next = (struct ItinStep **) malloc(counter * sizeof(struct ItinStep *));
		else {
			struct ItinStep ** new_next = realloc(tf->next, (counter+tf->num_next_nodes) * sizeof(struct ItinStep *));
			if(new_next == NULL) {
				printf("\nERROR WHILE REALLOCATING ITINERARIES DURING TRANSFER CALCULATION!!!\n");
				return;
			}
			tf->next = new_next;
		}
		for(int i = 0; i < counter; i++) tf->next[i+tf->num_next_nodes] = new_steps[i];

		tf->num_next_nodes += counter;
	}
}


//void find_viable_dsb_flybys(struct ItinStep *tf, struct Ephem **ephems, struct Body *body1, double min_dt0, double max_dt0, double min_dt1, double max_dt1) {
//	int max_new_steps0 = (int) (max_dt0/86400-min_dt0/86400+1);
//	int max_new_steps1 = (int) (max_dt1/86400-min_dt1/86400+1);
//	struct ItinStep *new_steps[max_new_steps0*max_new_steps1];
//	struct Body *body0 = tf->body;
//
//	int counter = 0;
//
//	struct OSV s0 = {tf->r, tf->v_arr};
//	struct OSV p0 = {tf->r, tf->v_body};
//	struct OSV s1;
//	double dt0, dt1, jd_sb2, jd_arr;
//
//
//	for(int i = 0; i <= max_dt0/86400-min_dt0/86400; i++) {
//
//		dt0 = min_dt0/86400 + i;
//		jd_sb2 = tf->date+dt0;
//		struct OSV osv_sb2 = osv_from_ephem(ephems[0], jd_sb2, SUN());
//
//		for(int j = 0; j <= max_dt1/86400-min_dt1/86400; j++) {
//			dt1 = min_dt1/86400 + j;
//			jd_arr = jd_sb2 + dt1;
//
//			struct OSV osv_arr = osv_from_ephem(ephems[1], jd_arr, SUN());
//			struct Transfer transfer_after_dsb = calc_transfer(circfb, body0, body1, osv_sb2.r, osv_sb2.v, osv_arr.r, osv_arr.v, (jd_arr-jd_sb2)*86400, SUN(), NULL, 0, 0);
//
//			s1.r = transfer_after_dsb.r0;
//			s1.v = transfer_after_dsb.v0;
//
//			struct DSB dsb = calc_double_swing_by(s0, p0, s1, osv_sb2, dt0, body0);
//
//			if(dsb.dv < 10000) {
//				new_steps[counter] = (struct ItinStep*) malloc(sizeof(struct ItinStep));
//				new_steps[counter]->body = NULL;
//				new_steps[counter]->date = tf->date + dsb.man_time/86400;
//				new_steps[counter]->r = dsb.osv[1].r;
//				new_steps[counter]->v_dep = dsb.osv[0].v;
//				new_steps[counter]->v_arr = dsb.osv[1].v;
//				new_steps[counter]->v_body = vec(0,0,0);
//				new_steps[counter]->num_next_nodes = 1;
//				new_steps[counter]->prev = tf;
//				new_steps[counter]->next = (struct ItinStep**) malloc(sizeof(struct ItinStep*));
//				new_steps[counter]->next[0] = (struct ItinStep*) malloc(sizeof(struct ItinStep));
//				new_steps[counter]->next[0]->prev = new_steps[counter];
//
//				struct ItinStep *curr = new_steps[counter]->next[0];
//				curr->body = body0;
//				curr->date = jd_sb2;
//				curr->r = osv_sb2.r;
//				curr->v_dep = dsb.osv[2].v;
//				curr->v_arr = dsb.osv[3].v;
//				curr->v_body = osv_sb2.v;
//				curr->num_next_nodes = 1;
//				curr->next = (struct ItinStep**) malloc(sizeof(struct ItinStep*));
//				curr->next[0] = (struct ItinStep*) malloc(sizeof(struct ItinStep));
//				curr->next[0]->prev = curr;
//
//				curr = curr->next[0];
//				curr->body = body1;
//				curr->date = jd_arr;
//				curr->r = osv_arr.r;
//				curr->v_dep = transfer_after_dsb.v0;
//				curr->v_arr = transfer_after_dsb.v1;
//				curr->v_body = osv_arr.v;
//				curr->num_next_nodes = 0;
//				curr->next = NULL;
//				counter++;
//			}
//		}
//	}
//
//	// if there were next nodes found, store them
//	if(counter > 0) {
//		tf->next = (struct ItinStep **) malloc(counter * sizeof(struct ItinStep *));
//		for(int i = 0; i < counter; i++) tf->next[i] = new_steps[i];
//		tf->num_next_nodes = counter;
//	}
//}



struct ItinStep * get_first(struct ItinStep *tf) {
	if(tf == NULL) return NULL;
	while(tf->prev != NULL) tf = tf->prev;
	return tf;
}

struct ItinStep * get_last(struct ItinStep *tf) {
	if(tf == NULL) return NULL;
	while(tf->next != NULL) tf = tf->next[0];
	return tf;
}


void print_itinerary(struct ItinStep *itin) {
	if(itin->prev != NULL) {
		print_itinerary(itin->prev);
		printf(" - ");
	}
	print_date(convert_JD_date(itin->date, DATE_ISO), 0);
}

int get_number_of_itineraries(struct ItinStep *itin) {
	if(itin == NULL || (itin->prev == NULL && itin->num_next_nodes == 0)) return 0;
	if(itin->num_next_nodes == 0) return 1;
	int counter = 0;
	for(int i = 0; i < itin->num_next_nodes; i++) {
		counter += get_number_of_itineraries(itin->next[i]);
	}
	return counter;
}

int get_total_number_of_stored_steps(struct ItinStep *itin) {
	if(itin == NULL) return 0;
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

struct PorkchopPoint *create_porkchop_array_from_departures(struct ItinStep **departures, int num_deps, double dep_periapsis, double arr_periapsis) {
	int num_itins = 0;
	for(int i = 0; i < num_deps; i++) num_itins += get_number_of_itineraries(departures[i]);

	struct ItinStep **arrivals = (struct ItinStep**) malloc(num_itins * sizeof(struct ItinStep*));
	int index = 0;
	for(int i = 0; i < num_deps; i++) store_itineraries_in_array(departures[i], arrivals, &index);
	struct PorkchopPoint *porkchop_points = malloc(num_itins * sizeof(struct PorkchopPoint));
	for(int i = 0; i < num_itins; i++) {
		porkchop_points[i] = create_porkchop_point(arrivals[i], dep_periapsis, arr_periapsis);
	}
	free(arrivals);

	return porkchop_points;
}

struct PorkchopPoint create_porkchop_point(struct ItinStep *itin, double dep_periapsis, double arr_periapsis) {
	struct PorkchopPoint pp;
	pp.arrival = itin;
	pp.dur = get_itinerary_duration(itin);

	double vinf = mag_vec3(subtract_vec3(itin->v_arr, itin->v_body));
	pp.dv_arr_cap = dv_capture(itin->body, alt2radius(itin->body, arr_periapsis), vinf);
	pp.dv_arr_circ = dv_circ(itin->body, alt2radius(itin->body, arr_periapsis), vinf);

	pp.dv_dsm = 0;
	while(itin->prev->prev != NULL) {
		if(itin->body == NULL) {
			pp.dv_dsm += mag_vec3(subtract_vec3(itin->next[0]->v_dep, itin->v_arr));
		}
		itin = itin->prev;
	}

	pp.dep_date = itin->prev->date;
	vinf = mag_vec3(subtract_vec3(itin->v_dep, itin->prev->v_body));
	pp.dv_dep = dv_circ(itin->prev->body, alt2radius(itin->prev->body, dep_periapsis), vinf);
	return pp;
}

int calc_next_spec_itin_step(struct ItinStep *curr_step, CelestSystem *system, Body **bodies, double jd_max_arr, struct Dv_Filter *dv_filter, int num_steps, int step) {
	double max_duration = jd_max_arr-curr_step->date;
	double min_duration = MIN_TRANSFER_DURATION;
	if(max_duration > min_duration && get_thread_counter(3) == 0) {
		if(bodies[step] != bodies[step - 1]) find_viable_flybys(curr_step, system, bodies[step], min_duration * 86400, max_duration * 86400);
		else {
			printf("DSB not yet reimplemented!\n");
			//		find_viable_dsb_flybys(curr_step, &bodies[step+1]->ephem, bodies[step+1],
			//							   min_duration[step-1]*86400, max_duration[step-1]*86400, min_duration[step]*86400, max_duration[step]*86400);
		}

		for(int i = 0; i < curr_step->num_next_nodes; i++) {
			struct ItinStep *next = bodies[step] != bodies[step - 1] ? curr_step->next[i] : curr_step->next[i]->next[0]->next[0];
			if(step == num_steps - 1) {
				struct PorkchopPoint porkchop_point = create_porkchop_point(next, dv_filter->dep_periapsis, dv_filter->arr_periapsis);
				double dv_sat = porkchop_point.dv_dsm;
				if(dv_filter->last_transfer_type == 1) dv_sat += porkchop_point.dv_arr_cap;
				if(dv_filter->last_transfer_type == 2) dv_sat += porkchop_point.dv_arr_circ;
				if(dv_sat > dv_filter->max_satdv || porkchop_point.dv_dep + dv_sat > dv_filter->max_totdv) {
					if(curr_step->num_next_nodes <= 1) {
						remove_step_from_itinerary(next);
						return 0;
					} else {
						remove_step_from_itinerary(next);
					}
					i--;
				}
			}
		}
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
			if(bodies[step] != bodies[step-1]) result = calc_next_spec_itin_step(curr_step->next[i - step_del], system, bodies, jd_max_arr, dv_filter, num_steps, step + 1);
			else result = calc_next_spec_itin_step(curr_step->next[i - step_del]->next[0]->next[0], system, bodies, jd_max_arr, dv_filter, num_steps, step + 2);
			num_valid += result;
			if(!result) step_del++;
		}
	} else return 1;

	return num_valid > 0;
}

int calc_next_itin_to_target_step(struct ItinStep *curr_step, struct ItinSequenceInfoToTarget *seq_info, double jd_max_arr, double max_total_duration, struct Dv_Filter *dv_filter) {
	for(int i = 0; i < seq_info->num_flyby_bodies; i++) {
		if(get_thread_counter(3) > 0) break;
		if(seq_info->flyby_bodies[i] == curr_step->body) continue;
		double jd_max;
		if(get_first(curr_step)->date + max_total_duration < jd_max_arr)
			jd_max = get_first(curr_step)->date + max_total_duration;
		else
			jd_max = jd_max_arr;
		double max_duration = jd_max-curr_step->date;
		double min_duration = MIN_TRANSFER_DURATION;
		if(max_duration > min_duration)
			find_viable_flybys(curr_step, seq_info->system, seq_info->flyby_bodies[i], min_duration*86400, max_duration*86400);
	}


	if(curr_step->num_next_nodes == 0) {
		remove_step_from_itinerary(curr_step);
		return 0;
	}

	int num_of_end_nodes = find_copy_and_store_end_nodes(curr_step, seq_info->arr_body);

	// remove end nodes that do not satisfy dv requirements
	num_of_end_nodes = remove_end_nodes_that_do_not_satisfy_dv_requirements(curr_step, num_of_end_nodes, dv_filter);
	if(curr_step->num_next_nodes <= 0) return 0;

	// returns 1 if has valid steps (0 otherwise)
	return continue_to_next_steps_and_check_for_valid_itins(curr_step, num_of_end_nodes, seq_info, jd_max_arr, max_total_duration, dv_filter);
}

int continue_to_next_steps_and_check_for_valid_itins(struct ItinStep *curr_step, int num_of_end_nodes, struct ItinSequenceInfoToTarget *seq_info, double jd_max_arr, double max_total_duration, struct Dv_Filter *dv_filter) {
	int num_valid = num_of_end_nodes;
	int init_num_nodes = curr_step->num_next_nodes;
	int step_del = 0;
	int has_valid_init;

	for(int i = 0; i < init_num_nodes-num_of_end_nodes; i++) {
		int idx = i - step_del;
		has_valid_init = calc_next_itin_to_target_step(curr_step->next[idx], seq_info, jd_max_arr, max_total_duration, dv_filter);
		num_valid += has_valid_init;
		if(!has_valid_init) step_del++;
	}

	return num_valid > 0;
}

int find_copy_and_store_end_nodes(struct ItinStep *curr_step, struct Body *arr_body) {
	int num_of_end_nodes = 0;
	for(int i = 0; i < curr_step->num_next_nodes; i++) {
		if(curr_step->next[i]->body == arr_body) num_of_end_nodes++;
	}
	// if there were end nodes found, copy them and store them at the end
	if(num_of_end_nodes > 0) {
		struct ItinStep ** new_next = realloc(curr_step->next, (num_of_end_nodes+curr_step->num_next_nodes) * sizeof(struct ItinStep *));
		if(new_next == NULL) {
			printf("\nERROR WHILE REALLOCATING ITINERARIES DURING END NODE COPYING!!!\n");
			return 0;
		}
		curr_step->next = new_next;

		int end_node_idx = 0;

		for(int i = 0; i < curr_step->num_next_nodes; i++) {
			if(curr_step->next[i]->body == arr_body) {
				struct ItinStep *end_node = malloc(num_of_end_nodes * sizeof(struct ItinStep));
				end_node->body 	= curr_step->next[i]->body;
				end_node->date 	= curr_step->next[i]->date;
				end_node->r 	= curr_step->next[i]->r;
				end_node->v_dep = curr_step->next[i]->v_dep;
				end_node->v_arr = curr_step->next[i]->v_arr;
				end_node->v_body= curr_step->next[i]->v_body;
				end_node->num_next_nodes = 0;
				end_node->prev 	= curr_step->next[i]->prev;
				end_node->next 	= NULL;

				curr_step->next[curr_step->num_next_nodes + end_node_idx] = end_node;
				end_node_idx++;
			}
		}
	}

	curr_step->num_next_nodes += num_of_end_nodes;
	return num_of_end_nodes;
}

int remove_end_nodes_that_do_not_satisfy_dv_requirements(struct ItinStep *curr_step, int num_of_end_nodes, struct Dv_Filter *dv_filter) {
	for(int i = curr_step->num_next_nodes-num_of_end_nodes; i < curr_step->num_next_nodes; i++) {
		struct ItinStep *next = curr_step->next[i];
		struct PorkchopPoint porkchop_point = create_porkchop_point(next, dv_filter->dep_periapsis, dv_filter->arr_periapsis);
		double dv_sat = porkchop_point.dv_dsm;
		if(dv_filter->last_transfer_type == 1) dv_sat += porkchop_point.dv_arr_cap;
		if(dv_filter->last_transfer_type == 2) dv_sat += porkchop_point.dv_arr_circ;
		if(dv_sat > dv_filter->max_satdv || porkchop_point.dv_dep + dv_sat > dv_filter->max_totdv) {
			if(curr_step->num_next_nodes <= 1) {
				remove_step_from_itinerary(next);
				curr_step->num_next_nodes = 0;
			} else {
				remove_step_from_itinerary(next);
			}
			i--;
			num_of_end_nodes--;
		}
	}
	return num_of_end_nodes;
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

void update_itin_body_osvs(struct ItinStep *step, CelestSystem *system) {
	OSV body_osv;
	while(step != NULL) {
		if(step->body != NULL) {
			body_osv = system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(step->body->orbit, step->date) :
					osv_from_ephem(step->body->ephem, step->body->num_ephems, step->date, system->cb);
			step->r = body_osv.r;
			step->v_body = body_osv.v;
		} else step->v_body = vec3(0, 0, 0);	// don't draw the trajectory (except changed in later calc)
		step = step->next != NULL ? step->next[0] : NULL;
	}
}

void calc_itin_v_vectors_from_dates_and_r(struct ItinStep *step, CelestSystem *system) {
	if(step == NULL) return;
	struct ItinStep *next;
	while(step->next != NULL) {
		next = step->next[0];

		if(next->body == NULL) {
			if(next->next == NULL) {
				step = next;
				continue;
			} else if(next->next[0]->next == NULL) {
				next = next->next[0];
				double dt = (next->date - step->date) * 86400;
				Lambert3 transfer = calc_lambert3(step->r, next->r, dt, system->cb);
				next->v_dep = transfer.v0;
				next->v_arr = transfer.v1;
			} else {
				struct ItinStep *sb2 = next->next[0];
				struct ItinStep *arr = next->next[0]->next[0];
				OSV osv_sb2 = {next->next[0]->r, next->next[0]->v_body};
				OSV osv_arr = {next->next[0]->next[0]->r, next->next[0]->next[0]->v_body};
				Lambert3 transfer_after_dsb = calc_lambert3(osv_sb2.r, osv_arr.r, (arr->date - sb2->date) * 86400, system->cb);

				OSV s0 = {step->r, step->v_arr};
				OSV p0 = {step->r, step->v_body};
				OSV s1 = {transfer_after_dsb.r0, transfer_after_dsb.v0};

				double dt = sb2->date - step->date;
				struct DSB dsb = calc_double_swing_by(s0, p0, s1, osv_sb2, dt, sb2->body);
				if(dsb.dv < 10000) {
					next->r = dsb.osv[1].r;
					next->v_dep = dsb.osv[0].v;
					next->v_arr = dsb.osv[1].v;
					next->date = step->date + dsb.man_time / 86400;
					next->v_body = vec3(1, 0, 0);    // draw the trajectory
				} else {
					next = next->next[0];
					dt = (next->date - step->date) * 86400;
					Lambert3 transfer= calc_lambert3(step->r, next->r, dt, system->cb);
					next->v_dep = transfer.v0;
					next->v_arr = transfer.v1;
				}
			}
		} else {
			double dt = (next->date - step->date) * 86400;
			Lambert3 transfer = calc_lambert3(step->r, next->r, dt, system->cb);
			next->v_dep = transfer.v0;
			next->v_arr = transfer.v1;
		}
		step = next;
	}
}

void copy_step_body_vectors_and_date(struct ItinStep *orig_step, struct ItinStep *step_copy) {
	step_copy->body = orig_step->body;
	step_copy->r = orig_step->r;
	step_copy->v_body = orig_step->v_body;
	step_copy->v_arr = orig_step->v_arr;
	step_copy->v_dep = orig_step->v_dep;
	step_copy->date = orig_step->date;
}

struct ItinStep * create_itin_copy(struct ItinStep *step) {
	struct ItinStep *new_step = (struct ItinStep*) malloc(sizeof(struct ItinStep));
	copy_step_body_vectors_and_date(step, new_step);
	new_step->prev = NULL;
	new_step->num_next_nodes = step->num_next_nodes;
	if(step->num_next_nodes > 0) {
		new_step->next = (struct ItinStep **) malloc(step->num_next_nodes * sizeof(struct ItinStep *));
		for(int i = 0; i < new_step->num_next_nodes; i++) {
			new_step->next[i] = create_itin_copy(step->next[i]);
			new_step->next[i]->prev = new_step;
		}
	} else new_step->next = NULL;
	return new_step;
}

struct ItinStep * create_itin_copy_from_arrival(struct ItinStep *step) {
	struct ItinStep *step_copy = (struct ItinStep*) malloc(sizeof(struct ItinStep));
	copy_step_body_vectors_and_date(step, step_copy);
	step_copy->next = NULL;
	step_copy->num_next_nodes = 0;

	while(step->prev != NULL) {
		step_copy->prev = (struct ItinStep*) malloc(sizeof(struct ItinStep));
		step_copy->prev->next = (struct ItinStep**) malloc(sizeof(struct ItinStep*));
		step_copy->prev->next[0] = step_copy;
		step_copy->prev->num_next_nodes = 1;

		step = step->prev;
		step_copy = step_copy->prev;
		copy_step_body_vectors_and_date(step, step_copy);
	}


	step_copy->prev = NULL;
	return get_last(step_copy);
}

int is_valid_itinerary(struct ItinStep *step) {
	if(step == NULL || step->body == NULL || step->prev == NULL) return 0;
	while(step != NULL) {
		if(step->body == NULL && step->v_body.x == 0) return 0;
		step = step->prev;
	}

	return 1;
}

void store_step_in_file(struct ItinStep *step, FILE *file, int layer, int variation) {
	fprintf(file, "#%d#%d\n", layer, variation);
	fprintf(file, "Date: %f\n", step->date);
	fprintf(file, "r: %lf, %lf, %lf\n", step->r.x, step->r.y, step->r.z);
	fprintf(file, "v_dep: %f, %f, %f\n", step->v_dep.x, step->v_dep.y, step->v_dep.z);
	fprintf(file, "v_arr: %f, %f, %f\n", step->v_arr.x, step->v_arr.y, step->v_arr.z);
	fprintf(file, "v_body: %f, %f, %f\n", step->v_body.x, step->v_body.y, step->v_body.z);
	fprintf(file, "Next Steps: %d\n", step->num_next_nodes);

	for(int i = 0; i < step->num_next_nodes; i++) {
		store_step_in_file(step->next[i], file, layer + 1, i);
	}
}

void store_itineraries_in_file(struct ItinStep **departures, int num_nodes, int num_deps) {
	char filename[50];  // 14 for date + 4 for .csv + 1 for string terminator
	sprintf(filename, "./Itineraries/test.transfer");
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
		store_step_in_file(departures[i], file, 0, i);
	}

	fclose(file);
}

void itinerary_short_overview_to_string(struct ItinStep *step, enum DateType date_type, double dep_periapsis, double arr_periapsis, char *string) {
	sprintf(string, "");
	if(step == NULL) return;
	step = get_first(step);
	char date_string[32];
	while(step != NULL) {
		if(step->prev != NULL) sprintf(string, "%s - ", string);
		sprintf(string, "%s%s", string, step->body->name);
		if(step->next != NULL) step = step->next[0];
		else break;
	}
	
	date_to_string(convert_JD_date(get_first(step)->date, date_type), date_string, 1);
	sprintf(string, "%s\nDeparture: %s\n", string, date_string);
	date_to_string(convert_JD_date(get_last(step)->date, date_type), date_string, 1);
	double dt = get_last(step)->date-get_first(step)->date;
	if(date_type == DATE_KERBAL) dt *= 4;
	sprintf(string, "%sArrival: %s\nDuration: %.2f days\n", string, date_string, dt);
	
	struct PorkchopPoint pp = create_porkchop_point(get_last(step), dep_periapsis, arr_periapsis);
	
	sprintf(string, "%s\nDeparture dv: %.2f m/s (Periapsis: %.2fkm)\n", string, pp.dv_dep, dep_periapsis/1e3);
	sprintf(string, "%sArrival dv: %.2f m/s  (Capture; Periapsis: %.2fkm)\n", string, pp.dv_arr_cap, arr_periapsis/1e3);
	sprintf(string, "%sArrival dv: %.2f m/s  (Circularization; Periapsis: %.2fkm)\n\n--\n", string, pp.dv_arr_circ, arr_periapsis/1e3);
	
	step = get_first(step);
	int step_id = 0;
	
	while(step != NULL) {
		date_to_string(convert_JD_date(step->date, date_type), date_string, 1);
		if(step_id == 0) sprintf(string, "%sDeparture", string);
		else if(step->next == NULL) sprintf(string, "%sArrival", string);
		else sprintf(string, "%sFly-By %d", string, step_id);
		sprintf(string, "%s (%s): | %s", string, step->body->name, date_string);
		if(step_id == 0) {
//			sprintf(string, "%sPeriapsis: %.2fkm\n", string, dep_periapsis/1e3);
			sprintf(string, "%s\n", string);
		} else if(step->next == NULL) {
//			sprintf(string, "%sPeriapsis: %.2fkm\n", string, arr_periapsis/1e3);
			sprintf(string, "%s\n", string);
		} else {
			Vector3 v_arr = step->v_arr;
			Vector3 v_dep = step->next[0]->v_dep;
			Vector3 v_body = step->v_body;
			double rp = get_flyby_periapsis(v_arr, v_dep, v_body, step->body);
			sprintf(string, "%s |  Periapsis: %.2fkm\n", string, (rp-step->body->radius)/1e3);
		}
		if(step->next != NULL) step = step->next[0];
		else step = NULL;
		step_id++;
	}
}

void itinerary_detailed_overview_to_string(struct ItinStep *step, enum DateType date_type, double dep_periapsis, double arr_periapsis, char *string) {
	sprintf(string, "");
	if(step == NULL) return;
	step = get_first(step);
	char date_string[32];
	while(step != NULL) {
		if(step->prev != NULL) sprintf(string, "%s - ", string);
		sprintf(string, "%s%s", string, step->body->name);
		if(step->next != NULL) step = step->next[0];
		else break;
	}
	
	date_to_string(convert_JD_date(get_first(step)->date, date_type), date_string, 1);
	sprintf(string, "%s\nDeparture: %s\n", string, date_string);
	date_to_string(convert_JD_date(get_last(step)->date, date_type), date_string, 1);
	double dt = get_last(step)->date-get_first(step)->date;
	if(date_type == DATE_KERBAL) dt *= 4;
	sprintf(string, "%sArrival: %s\nDuration: %.2f days\n", string, date_string, dt);
	
	struct PorkchopPoint pp = create_porkchop_point(get_last(step), dep_periapsis, arr_periapsis);
	
	sprintf(string, "%s\nDeparture dv: %.2f m/s\n", string, pp.dv_dep);
	sprintf(string, "%sArrival dv: %.2f m/s  (Capture)\n", string, pp.dv_arr_cap);
	sprintf(string, "%sArrival dv: %.2f m/s  (Circularization)\n\n--\n", string, pp.dv_arr_circ);
	
	step = get_first(step);
	int step_id = 0;
	
	while(step != NULL) {
		date_to_string(convert_JD_date(step->date, date_type), date_string, 1);
		if(step_id == 0) sprintf(string, "%s\nDeparture", string);
		else if(step->next == NULL) sprintf(string, "%s\nArrival", string);
		else sprintf(string, "%s\nFly-By %d", string, step_id);
		sprintf(string, "%s (%s):\nDate: %s  (T+%.2f days)\n", string, step->body->name, date_string, step->date-get_first(step)->date);
		if(step_id == 0) {
			sprintf(string, "%sPeriapsis: %.2fkm\n", string, dep_periapsis/1e3);
		} else if(step->next == NULL) {
			sprintf(string, "%sPeriapsis: %.2fkm\n", string, arr_periapsis/1e3);
		} else {
			Vector3 v_arr = step->v_arr;
			Vector3 v_dep = step->next[0]->v_dep;
			Vector3 v_body = step->v_body;
			double rp = get_flyby_periapsis(v_arr, v_dep, v_body, step->body);
			double incl = get_flyby_inclination(v_arr, v_dep, v_body, get_body_equatorial_plane(step->body));
			sprintf(string, "%sPeriapsis: %.2fkm\nInclination: %.2f°\n", string, (rp-step->body->radius)/1e3, rad2deg(incl));
		}
		if(step->next != NULL) step = step->next[0];
		else step = NULL;
		step_id++;
	}
}

void itinerary_step_parameters_to_string(char *s_labels, char *s_values, enum DateType date_type, double dep_periapsis, double arr_periapsis, struct ItinStep *step) {
	if(step == NULL) {sprintf(s_labels,""); sprintf(s_values,""); return;}
	HyperbolaParams hyp_params;

	// is departure step
	if(step->prev == NULL) {
		hyp_params = get_hyperbola_params(vec3(0,0,0), step->next[0]->v_dep, step->v_body, step->body, dep_periapsis, HYP_DEPARTURE);
		sprintf(s_labels, "Departure\n"
						  "T+:\n"
						  "RadPer:\n"
						  "AltPer:\n"
						  "Min Incl:\n\n"
						  "C3Energy:\n"
						  "RHA (out):\n"
						  "DHA (out):");

		sprintf(s_values, "\n0 days\n"
						  "%.0f km\n"
						  "%.0f km\n"
						  "%.2f°\n\n"
						  "%.2f km²/s²\n"
						  "%.2f°\n"
						  "%.2f°",
				hyp_params.rp / 1000, (hyp_params.rp-step->body->radius) / 1000, rad2deg(angle_plane3_vec3(get_body_equatorial_plane(step->body), subtract_vec3(step->next[0]->v_dep, step->v_body))), hyp_params.c3_energy / 1e6,
				rad2deg(hyp_params.outgoing.bplane_angle), rad2deg(hyp_params.outgoing.decl));

	} else if(step->num_next_nodes == 0) {
		double dt_in_days = step->date - get_first(step)->date;
		if(date_type == DATE_KERBAL) dt_in_days *= 4;
		hyp_params = get_hyperbola_params(step->v_arr, vec3(0,0,0), step->v_body, step->body, arr_periapsis, HYP_ARRIVAL);
		sprintf(s_labels, "Arrival\n"
						  "T+:\n"
						  "RadPer:\n"
						  "AltPer:\n"
						  "Min Incl:\n"
						  "Max Incl:\n\n"
						  "C3Energy:\n"
						  "RHA (in):\n"
						  "DHA (in):");

		sprintf(s_values, "\n%.2f days\n"
						  "%.0f km\n"
						  "%.0f km\n"
						  "%.2f°\n"
						  "%.2f°\n\n"
						  "%.2f km²/s²\n"
						  "%.2f°\n"
						  "%.2f°",
				dt_in_days, hyp_params.rp / 1000, (hyp_params.rp-step->body->radius) / 1000, rad2deg(angle_plane3_vec3(get_body_equatorial_plane(step->body), subtract_vec3(step->v_arr, step->v_body))), 180.0 - fabs(rad2deg(angle_plane3_vec3(get_body_equatorial_plane(step->body), subtract_vec3(step->v_arr, step->v_body)))), hyp_params.c3_energy / 1e6,
				rad2deg(hyp_params.incoming.bplane_angle), rad2deg(hyp_params.incoming.decl));

	} else {
		if(step->body != NULL) {
			Vector3 v_arr = step->v_arr;
			Vector3 v_dep = step->next[0]->v_dep;
			Vector3 v_body = step->v_body;
			double incl = get_flyby_inclination(v_arr, v_dep, v_body, get_body_equatorial_plane(step->body));

			hyp_params = get_hyperbola_params(step->v_arr, step->next[0]->v_dep,
											  step->v_body, step->body, 0, HYP_FLYBY);
			double dt_in_days = step->date - get_first(step)->date;
			if(date_type == DATE_KERBAL) dt_in_days *= 4;

			sprintf(s_labels, "Hyperbola\n"
							  "T+:\n"
							  "RadPer:\n"
							  "AltPer:\n"
							  "Inclination:\n\n"
							  "C3Energy:\n"
							  "RHA (in):\n"
							  "DHA (in):\n"
							  "BVAZI (in):\n"
							  "RHA (out):\n"
							  "DHA (out):\n"
							  "BVAZI (out):");

			sprintf(s_values, "\n%.2f days\n"
							  "%.0f km\n"
							  "%.0f km\n"
							  "%.2f°\n\n"
							  "%.2f km²/s²\n"
							  "%.2f°\n"
							  "%.2f°\n"
							  "%.2f°\n"
							  "%.2f°\n"
							  "%.2f°\n"
							  "%.2f°",
					dt_in_days, hyp_params.rp / 1000, (hyp_params.rp-step->body->radius) / 1000, rad2deg(incl),
					hyp_params.c3_energy / 1e6,
					rad2deg(hyp_params.incoming.bplane_angle), rad2deg(hyp_params.incoming.decl),
					rad2deg(hyp_params.incoming.bvazi),
					rad2deg(hyp_params.outgoing.bplane_angle), rad2deg(hyp_params.outgoing.decl),
					rad2deg(hyp_params.outgoing.bvazi));
		} else {
			double dt_in_days = step->date - get_first(step)->date;
			if(date_type == DATE_KERBAL) dt_in_days *= 4;
			double dist_to_sun = mag_vec3(step->r);

			Vector3 orbit_prograde = step->v_arr;
			Vector3 orbit_normal = cross_vec3(step->r, step->v_arr);
			Vector3 orbit_radialin = cross_vec3(orbit_normal, step->v_arr);
			Vector3 dv_vec = subtract_vec3(step->next[0]->v_dep, step->v_arr);

			// dv vector in S/C coordinate system (prograde, radial in, normal) (sign it if projected vector more than 90° from target vector / pointing in opposite direction)
			Vector3 dv_vec_sc = {
					mag_vec3(proj_vec3_vec3(dv_vec, orbit_prograde)) * (angle_vec3_vec3(proj_vec3_vec3(dv_vec, orbit_prograde), orbit_prograde) < M_PI/2 ? 1 : -1),
					mag_vec3(proj_vec3_vec3(dv_vec, orbit_radialin)) * (angle_vec3_vec3(proj_vec3_vec3(dv_vec, orbit_radialin), orbit_radialin) < M_PI/2 ? 1 : -1),
					mag_vec3(proj_vec3_vec3(dv_vec, orbit_normal)) * (angle_vec3_vec3(proj_vec3_vec3(dv_vec, orbit_normal), orbit_normal) < M_PI/2 ? 1 : -1)
			};

			sprintf(s_labels, "DSM\n"
							  "T+:\n"
							  "Distance to the Sun:\n"
							  "Dv Prograde:\n"
							  "Dv Radial:\n"
							  "Dv Normal:\n"
							  "Total:");

			sprintf(s_values, "\n%.2f days\n"
							  "%.3f AU\n"
							  "%.3f m/s\n"
							  "%.3f m/s\n"
							  "%.3f m/s\n"
							  "%.3f m/s",
					dt_in_days, dist_to_sun / 1.495978707e11, dv_vec_sc.x, dv_vec_sc.y, dv_vec_sc.z, mag_vec3(dv_vec_sc));
		}
	}
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
	if(step == NULL) return;
	if(step->next != NULL) {
		for(int i = 0; i < step->num_next_nodes; i++) {
			free_itinerary(step->next[i]);
		}
		free(step->next);
	}
	free(step);
}
