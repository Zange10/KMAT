#include "transfer_calc.h"
#include "tools/tool_funcs.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include "tools/thread_pool.h"



typedef struct Itin_Calc_Thread_Args {
	struct ItinStep **departures;
	Itin_Calc_Data *calc_data;
} Itin_Calc_Thread_Args;

struct Thread_Progress_and_Prop_Info {
	double jd_min_dep;
	double jd_max_dep;
} thread_progress_and_prop_info;

const int NUM_INITIAL_TRANSFERS_PER_BODY = 500;


void *calc_itins_from_departure(void *args) {
	Itin_Calc_Thread_Args *thread_args = (struct Itin_Calc_Thread_Args *)args;
	Itin_Calc_Data *calc_data = thread_args->calc_data;

	struct ItinStep **departures = thread_args->departures;
	struct Dv_Filter dv_filter = calc_data->dv_filter;
	double jd_min_dep = calc_data->jd_min_dep;
	double jd_max_dep = calc_data->jd_max_dep;
	double max_total_duration = calc_data->max_duration;

	enum ItinSequenceInfoType itin_seq_type = calc_data->seq_info.to_target.type;

	CelestSystem *system;
	Body *dep_body,
		*arr_body,			// only to_target: target body
		**fly_by_bodies;	// to_target: allowed fly_by_bodies; spec_seq: bodies in sequence
	int num_initial_transfers,
			num_steps,			// only spec_seq: number of steps in sequence
			num_flyby_bodies;	// only to_target: number of allowed fly_by_bodies

	enum Transfer_Type tt = dv_filter.last_transfer_type == TF_CIRC ? circcirc : dv_filter.last_transfer_type == TF_CAPTURE ? circcap : circfb;

	if(itin_seq_type == ITIN_SEQ_INFO_TO_TARGET) {
		// to target
		system = calc_data->seq_info.to_target.system;
		dep_body = calc_data->seq_info.to_target.dep_body;
		arr_body = calc_data->seq_info.to_target.arr_body;
		fly_by_bodies = calc_data->seq_info.to_target.flyby_bodies;
		num_flyby_bodies = calc_data->seq_info.to_target.num_flyby_bodies;
		num_initial_transfers = NUM_INITIAL_TRANSFERS_PER_BODY * (system->num_bodies - 1);
	} else {
		// specific sequence
		system = calc_data->seq_info.spec_seq.system;
		dep_body = calc_data->seq_info.spec_seq.bodies[0];
		fly_by_bodies = calc_data->seq_info.spec_seq.bodies;
		num_steps = calc_data->seq_info.spec_seq.num_steps;
		num_initial_transfers = NUM_INITIAL_TRANSFERS_PER_BODY;
	}

	// index in departure array (iterated over)
	int index = get_incr_thread_counter(0);
	// increase finished counter to 1 (first finished should reflect a num of finished of 1)

	double jd_dep = jd_min_dep + index;
	struct ItinStep *curr_step;
	OSV osv_body0, osv_body1;

	// used for progress feedback
	double jd_diff = jd_max_dep-jd_min_dep+1;

	while(jd_dep <= jd_max_dep && get_thread_counter(3) == 0) {
		osv_body0 = system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(dep_body->orbit, jd_dep) :
					osv_from_ephem(dep_body->ephem, dep_body->num_ephems, jd_dep, system->cb);

		curr_step = departures[index];
		curr_step->body = dep_body;
		curr_step->date = jd_dep;
		curr_step->r = osv_body0.r;
		curr_step->v_body = osv_body0.v;
		curr_step->v_dep = vec3(0, 0, 0);
		curr_step->v_arr = vec3(0, 0, 0);
		curr_step->num_next_nodes = num_initial_transfers;
		curr_step->prev = NULL;
		curr_step->next = (struct ItinStep **) malloc(curr_step->num_next_nodes * sizeof(struct ItinStep *));

		double jd_max_arr = jd_dep + max_total_duration < calc_data->jd_max_arr ? jd_dep + max_total_duration : calc_data->jd_max_arr;

		Body *next_step_body;
		int num_next_bodies = itin_seq_type == ITIN_SEQ_INFO_TO_TARGET ? num_flyby_bodies : 1;
		int next_step_id = 0;

		for(int i = 0; i < num_next_bodies; i++) {
			if(itin_seq_type == ITIN_SEQ_INFO_TO_TARGET) {
				next_step_body = fly_by_bodies[i];
				if(next_step_body == dep_body) continue;
			} else {
				next_step_body = fly_by_bodies[1];
			}

			OSV arr_body_temp_osv = system->prop_method == ORB_ELEMENTS ?
										   osv_from_elements(next_step_body->orbit, jd_dep) :
										   osv_from_ephem(next_step_body->ephem, next_step_body->num_ephems, jd_dep, system->cb);

			double r0 = mag_vec3(osv_body0.r), r1 = mag_vec3(arr_body_temp_osv.r);
			double r_ratio =  r1/r0;
			Hohmann hohmann = calc_hohmann_transfer(r0, r1, system->cb);
			double hohmann_dur = hohmann.dur/86400;
			double min_duration = 0.4 * hohmann_dur;
			double max_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_duration/hohmann_dur > 3) max_duration = hohmann_dur*3;
			double max_min_duration_diff = max_duration - min_duration;

			for(int j = 0; j < NUM_INITIAL_TRANSFERS_PER_BODY; j++) {
				double jd_arr = jd_dep + min_duration + max_min_duration_diff * j/(NUM_INITIAL_TRANSFERS_PER_BODY - 1);

				if(jd_arr > jd_max_arr) break;

				osv_body1 = system->prop_method == ORB_ELEMENTS ?
							osv_from_elements(next_step_body->orbit, jd_arr) :
							osv_from_ephem(next_step_body->ephem, next_step_body->num_ephems, jd_arr, system->cb);

				Lambert3 tf = calc_lambert3(osv_body0.r, osv_body1.r, (jd_arr - jd_dep) * 86400, system->cb);
				
				
				double dv_dep, dv_arr;
				if(dep_body != NULL) {
					double vinf = fabs(mag_vec3(subtract_vec3(tf.v0, osv_body0.v)));
					dv_dep = tt % 2 == 0 ? dv_capture(dep_body, alt2radius(dep_body, dv_filter.dep_periapsis), vinf) : dv_circ(dep_body,alt2radius(dep_body, dv_filter.dep_periapsis),vinf);
				} else dv_dep = mag_vec3(osv_body0.v);
				
				if(arr_body != NULL) {
					double vinf = fabs(mag_vec3(subtract_vec3(tf.v1, osv_body1.v)));
					if(tt < 2) dv_arr = dv_capture(arr_body, alt2radius(arr_body, dv_filter.arr_periapsis), vinf);
					else if(tt < 4) dv_arr = dv_circ(arr_body, alt2radius(arr_body, dv_filter.arr_periapsis), vinf);
					else dv_arr = 0;
				} else {
					dv_arr = mag_vec3(osv_body1.v);
				}
				
				if(dv_dep > dv_filter.max_totdv || dv_dep > dv_filter.max_depdv) continue;
				if(num_steps == 2 && (dv_dep + dv_arr > dv_filter.max_totdv || dv_arr > dv_filter.max_satdv)) continue;

				curr_step = get_first(curr_step);
				curr_step->next[next_step_id] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
				curr_step->next[next_step_id]->prev = curr_step;
				curr_step->next[next_step_id]->next = NULL;

				curr_step = curr_step->next[next_step_id];

				curr_step->body = next_step_body;
				curr_step->date = jd_arr;
				curr_step->r = osv_body1.r;
				curr_step->v_dep = tf.v0;
				curr_step->v_arr = tf.v1;
				curr_step->v_body = osv_body1.v;
				curr_step->num_next_nodes = 0;

				next_step_id++;
			}
		}

		curr_step = get_first(curr_step);
		curr_step->num_next_nodes = next_step_id;

		if(itin_seq_type == ITIN_SEQ_INFO_TO_TARGET) {
			int num_of_end_nodes = find_copy_and_store_end_nodes(curr_step, arr_body);

			// remove end nodes that do not satisfy dv requirements
			num_of_end_nodes = remove_end_nodes_that_do_not_satisfy_dv_requirements(curr_step, num_of_end_nodes, &dv_filter);
			if(curr_step->num_next_nodes > 0) {
				continue_to_next_steps_and_check_for_valid_itins(curr_step, num_of_end_nodes, &calc_data->seq_info.to_target, jd_max_arr, max_total_duration, &dv_filter);
			}
		} else {
			if(num_steps > 2) {
				int itin_del = 0;
				int init_num_next_nodes = curr_step->num_next_nodes;
				for(int i = 0; i < init_num_next_nodes; i++) {
					if(!calc_next_spec_itin_step(curr_step->next[i - itin_del], system, fly_by_bodies, jd_max_arr, &dv_filter, num_steps, 2)) itin_del++;
				}
			}
		}

		double progress = get_incr_thread_counter(1)+1;	// +1 because it gets the last value, not the incremented value
		show_progress("Transfer Calculation progress", progress, jd_diff);
		incr_thread_counter_by_amount(2, get_number_of_itineraries(get_first(curr_step)));
		index = get_incr_thread_counter(0);
		jd_dep = jd_min_dep + index;
	}
	return NULL;
}

struct Itin_Calc_Results search_for_itineraries(Itin_Calc_Data calc_data) {
	struct Itin_Calc_Results results = {NULL, 0, 0};

	struct timeval start, end;
	double elapsed_time;
	gettimeofday(&start, NULL);  // Record the ending time

	int num_deps = (int) (calc_data.jd_max_dep-calc_data.jd_min_dep+1);

	struct ItinStep **departures = (struct ItinStep**) malloc(num_deps * sizeof(struct ItinStep*));
	for(int i = 0; i < num_deps; i++) departures[i] = (struct ItinStep*) malloc(sizeof(struct ItinStep));
	for(int i = 0; i < num_deps; i++) departures[i]->num_next_nodes = 0;


	Itin_Calc_Thread_Args thread_args = {
			departures,
			&calc_data,
	};

	thread_progress_and_prop_info = (struct Thread_Progress_and_Prop_Info) {
			calc_data.jd_min_dep,
			calc_data.jd_max_dep
	};

	show_progress("Transfer Calculation progress", 0, 1);
	struct Thread_Pool thread_pool;
	thread_pool = use_thread_pool32(calc_itins_from_departure, &thread_args);
	join_thread_pool(thread_pool);
	show_progress("Transfer Calculation progress", 1, 1);
	printf("\n");

	// remove departure dates with no valid itinerary
	for(int i = 0; i < num_deps; i++) {
		if(departures[i] == NULL || departures[i]->num_next_nodes == 0) {
			num_deps--;
			for(int j = i; j < num_deps; j++) {
				departures[j] = departures[j + 1];
			}
			i--;
		}
	}

	gettimeofday(&end, NULL);  // Record the ending time
	elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
	printf("----- | Total elapsed time: %.3f s | ---------\n", elapsed_time);

	int num_itins = 0, num_nodes = 0;
	for(int i = 0; i < num_deps; i++) num_itins += get_number_of_itineraries(departures[i]);
	for(int i = 0; i < num_deps; i++) num_nodes += get_total_number_of_stored_steps(departures[i]);

	printf("\n%d itineraries found!\nNumber of Nodes: %d\n", num_itins, num_nodes);

	results.departures = departures;
	results.num_deps = num_deps;
	results.num_nodes = num_nodes;
	results.num_itins = num_itins;

	return results;
}

struct Transfer_Calc_Status get_current_transfer_calc_status() {
	double jd_diff = thread_progress_and_prop_info.jd_max_dep-thread_progress_and_prop_info.jd_min_dep+1;
	int num_deps = get_thread_counter(1);

	return (struct Transfer_Calc_Status) {
		.num_itins = get_thread_counter(2),
		.num_deps = num_deps,
		.jd_diff = jd_diff,
		.progress = (double)num_deps/jd_diff
	};
}