#include "transfer_calc.h"
#include "transfer_tools.h"
#include "tools/tool_funcs.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "tools/thread_pool.h"
#include "orbit_calculator.h"


struct Itin_Spec_Thread_Args {
	struct ItinStep **departures;
	double jd_min_dep;
	double jd_max_dep;
	double jd_max_arr;
	struct Body **bodies;
	int max_duration;
	int num_steps;
	struct System *system;
	struct Dv_Filter *dv_filter;
};

struct Itin_To_Target_Thread {
	struct ItinStep **departures;
	double jd_min_dep;
	double jd_max_dep;
	double jd_max_arr;
	int max_duration;
	struct ItinSequenceInfo *seq_info;
	struct Dv_Filter *dv_filter;
};

struct Thread_Progress_and_Prop_Info {
	struct ItinStep **departures;
	double jd_min_dep;
	double jd_max_dep;
} thread_progress_and_prop_info;

const int NUM_INITIAL_TRANSFERS = 500;



void *calc_spec_itin_from_departure(void *args) {
	struct Itin_Spec_Thread_Args *thread_args = (struct Itin_Spec_Thread_Args *)args;

	int max_duration = thread_args->max_duration;
	struct Body **bodies = thread_args->bodies;
	struct System *system = thread_args->system;
	int num_steps = thread_args->num_steps;

	int index = get_incr_thread_counter(0);
	// increase finished counter to 1 (first finished should reflect a num of finished of 1)
	if(index == 0) get_incr_thread_counter(1);

	double jd_dep = thread_args->jd_min_dep + index;
	struct ItinStep *curr_step;

	double jd_diff = thread_args->jd_max_dep-thread_args->jd_min_dep+1;

	while(jd_dep <= thread_args->jd_max_dep) {
		struct OSV osv_body0 = system->calc_method == ORB_ELEMENTS ?
				osv_from_elements(bodies[0]->orbit, jd_dep, system) :
				osv_from_ephem(bodies[0]->ephem, jd_dep, system->cb);

		curr_step = thread_args->departures[index];
		curr_step->body = bodies[0];
		curr_step->date = jd_dep;
		curr_step->r = osv_body0.r;
		curr_step->v_body = osv_body0.v;
		curr_step->v_dep = vec(0, 0, 0);
		curr_step->v_arr = vec(0, 0, 0);
		curr_step->num_next_nodes = NUM_INITIAL_TRANSFERS;
		curr_step->prev = NULL;
		curr_step->next = (struct ItinStep **) malloc(curr_step->num_next_nodes * sizeof(struct ItinStep *));
		
		
		double jd_max_arr = jd_dep + max_duration < thread_args->jd_max_arr ? jd_dep + max_duration : thread_args->jd_max_arr;
		
		int fb1_del = 0;
		
		struct OSV arr_body_temp_osv = system->calc_method == ORB_ELEMENTS ?
									   osv_from_elements(bodies[1]->orbit, jd_dep, system) :
									   osv_from_ephem(bodies[1]->ephem, jd_dep, system->cb);
		
		double r0 = vector_mag(osv_body0.r), r1 = vector_mag(arr_body_temp_osv.r);
		double r_ratio =  r1/r0;
		double hohmann_dur = calc_hohmann_transfer_duration(r0, r1, system->cb)/86400;
		double min_init_duration = 0.4 * hohmann_dur;
		double max_init_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_init_duration/hohmann_dur > 3) max_init_duration = hohmann_dur*3;
		double max_min_duration_diff = max_init_duration - min_init_duration;
		
		for(int j = 0; j < NUM_INITIAL_TRANSFERS; j++) {
			double jd_arr = jd_dep + min_init_duration + max_min_duration_diff * j/(NUM_INITIAL_TRANSFERS-1);
			
			struct OSV osv_body1 = system->calc_method == ORB_ELEMENTS ?
					osv_from_elements(bodies[1]->orbit, jd_arr, system) :
				    osv_from_ephem(bodies[1]->ephem, jd_arr, system->cb);

			double data[3];

			struct Transfer tf = calc_transfer(circfb, bodies[0], bodies[1], osv_body0.r, osv_body0.v,
											   osv_body1.r, osv_body1.v, (jd_arr - jd_dep) * 86400, system->cb, data);


			while(curr_step->prev != NULL) curr_step = curr_step->prev;
			curr_step->next[j - fb1_del] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
			curr_step->next[j - fb1_del]->prev = curr_step;
			curr_step->next[j - fb1_del]->next = NULL;

			if(data[1] > thread_args->dv_filter->max_totdv || data[1] > thread_args->dv_filter->max_depdv) {
				struct ItinStep *rem_step = curr_step->next[j - fb1_del];
				remove_step_from_itinerary(rem_step);
				fb1_del++;
				continue;
			}

			curr_step = curr_step->next[j - fb1_del];
			curr_step->body = bodies[1];
			curr_step->date = jd_arr;
			curr_step->r = osv_body1.r;
			curr_step->v_dep = tf.v0;
			curr_step->v_arr = tf.v1;
			curr_step->v_body = osv_body1.v;
			curr_step->num_next_nodes = 0;

			curr_step = curr_step->prev;

			if(num_steps > 2) {
				if(!calc_next_spec_itin_step(curr_step->next[j - fb1_del], system, bodies, jd_max_arr,
											 thread_args->dv_filter, num_steps, 2)) fb1_del++;
			}
		}
		double progress = get_incr_thread_counter(1);
		show_progress("Transfer Calculation progress: ", progress, jd_diff);
		incr_thread_counter_by_amount(2, get_number_of_itineraries(get_first(curr_step)));
		index = get_incr_thread_counter(0);
		jd_dep = thread_args->jd_min_dep + index;
	}
	return NULL;
}

void *calc_itin_to_target_from_departure(void *args) {
	struct Itin_To_Target_Thread *thread_args = (struct Itin_To_Target_Thread *)args;

	struct ItinStep **departures = thread_args->departures;
	struct Dv_Filter *dv_filter = thread_args->dv_filter;
	double jd_min_dep = thread_args->jd_min_dep;
	double jd_max_dep = thread_args->jd_max_dep;
	double jd_max_arr = thread_args->jd_max_arr;
	int max_total_duration = thread_args->max_duration;
	struct System *system = thread_args->seq_info->system;
	struct Body *dep_body = thread_args->seq_info->dep_body;
	struct Body *arr_body = thread_args->seq_info->arr_body;

	int index = get_incr_thread_counter(0);
	// increase finished counter to 1 (first finished should reflect a num of finished of 1)
	if(index == 0) get_incr_thread_counter(1);

	double jd_dep = jd_min_dep + index;
	struct ItinStep *curr_step;
	struct OSV osv_body0, osv_body1;

	// used for progress feedback
	double jd_diff = jd_max_dep-jd_min_dep+1;

	while(jd_dep <= jd_max_dep) {
		osv_body0 = system->calc_method == ORB_ELEMENTS ?
				osv_from_elements(dep_body->orbit, jd_dep, system) :
				osv_from_ephem(dep_body->ephem, jd_dep, system->cb);

		curr_step = departures[index];
		curr_step->body = dep_body;
		curr_step->date = jd_dep;
		curr_step->r = osv_body0.r;
		curr_step->v_body = osv_body0.v;
		curr_step->v_dep = vec(0, 0, 0);
		curr_step->v_arr = vec(0, 0, 0);
		curr_step->num_next_nodes = NUM_INITIAL_TRANSFERS * (system->num_bodies-1);
		curr_step->prev = NULL;
		curr_step->next = (struct ItinStep **) malloc(curr_step->num_next_nodes * sizeof(struct ItinStep *));

		int next_step_id = 0;

		struct Body *next_step_body;

		for(int i = 0; i < thread_args->seq_info->num_flyby_bodies; i++) {
			next_step_body = thread_args->seq_info->flyby_bodies[i];
			if(next_step_body == dep_body) continue;

			struct OSV arr_body_temp_osv = system->calc_method == ORB_ELEMENTS ?
										   osv_from_elements(next_step_body->orbit, jd_dep, system) :
										   osv_from_ephem(next_step_body->ephem, jd_dep, system->cb);

			double r0 = vector_mag(osv_body0.r), r1 = vector_mag(arr_body_temp_osv.r);
			double r_ratio =  r1/r0;
			double hohmann_dur = calc_hohmann_transfer_duration(r0, r1, system->cb)/86400;
			double min_duration = 0.4 * hohmann_dur;
			double max_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_duration/hohmann_dur > 3) max_duration = hohmann_dur*3;
			double max_min_duration_diff = max_duration - min_duration;

			for(int j = 0; j < NUM_INITIAL_TRANSFERS; j++) {
				double jd_arr = jd_dep + min_duration + max_min_duration_diff * j/(NUM_INITIAL_TRANSFERS-1);

				if(jd_arr > jd_max_arr) break;

				osv_body1 = system->calc_method == ORB_ELEMENTS ?
						osv_from_elements(next_step_body->orbit, jd_arr, system) :
						osv_from_ephem(next_step_body->ephem, jd_arr, system->cb);

				double data[3];

				struct Transfer tf = calc_transfer(circfb, dep_body, next_step_body, osv_body0.r, osv_body0.v,
												   osv_body1.r, osv_body1.v, (jd_arr - jd_dep) * 86400, system->cb, data);


				if(data[1] > dv_filter->max_totdv || data[1] > dv_filter->max_depdv) continue;

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

		int num_of_end_nodes = find_copy_and_store_end_nodes(curr_step, arr_body);

		// remove end nodes that do not satisfy dv requirements
		num_of_end_nodes = remove_end_nodes_that_do_not_satisfy_dv_requirements(curr_step, num_of_end_nodes, dv_filter);
		if(curr_step->num_next_nodes > 0) {
			continue_to_next_steps_and_check_for_valid_itins(curr_step, num_of_end_nodes, thread_args->seq_info, jd_max_arr, max_total_duration, dv_filter);
		}

		double progress = get_incr_thread_counter(1);
		show_progress("Transfer Calculation progress", progress, jd_diff);
		incr_thread_counter_by_amount(2, get_number_of_itineraries(get_first(curr_step)));
		index = get_incr_thread_counter(0);
		jd_dep = jd_min_dep + index;
	}
	return NULL;
}

struct Transfer_Calc_Results search_for_itinerary_to_target(struct Transfer_To_Target_Calc_Data calc_data) {
	struct Transfer_Calc_Results results = {NULL, 0, 0};

	struct timeval start, end;
	double elapsed_time;
	gettimeofday(&start, NULL);  // Record the ending time

	int num_deps = (int) (calc_data.jd_max_dep-calc_data.jd_min_dep+1);

	struct ItinStep **departures = (struct ItinStep**) malloc(num_deps * sizeof(struct ItinStep*));
	for(int i = 0; i < num_deps; i++) departures[i] = (struct ItinStep*) malloc(sizeof(struct ItinStep));
	for(int i = 0; i < num_deps; i++) departures[i]->num_next_nodes = 0;


	struct Itin_To_Target_Thread thread_args = {
			departures,
			calc_data.jd_min_dep,
			calc_data.jd_max_dep,
			calc_data.jd_max_arr,
			calc_data.max_duration,
			calc_data.seq_info,
			&calc_data.dv_filter
	};

	thread_progress_and_prop_info = (struct Thread_Progress_and_Prop_Info) {
			departures,
			calc_data.jd_min_dep,
			calc_data.jd_max_dep
	};

	show_progress("Transfer Calculation progress", 0, 1);
	struct Thread_Pool thread_pool = use_thread_pool64(calc_itin_to_target_from_departure, &thread_args);
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

	return results;
}

struct Transfer_Calc_Results search_for_spec_itinerary(struct Transfer_Spec_Calc_Data calc_data) {
	struct Transfer_Calc_Results results = {NULL, 0, 0};

	struct timeval start, end;
	double elapsed_time;
	gettimeofday(&start, NULL);  // Record the ending time

	int num_deps = (int) (calc_data.jd_max_dep-calc_data.jd_min_dep+1);

	struct ItinStep **departures = (struct ItinStep**) malloc(num_deps * sizeof(struct ItinStep*));
	for(int i = 0; i < num_deps; i++) departures[i] = (struct ItinStep*) malloc(sizeof(struct ItinStep));
	for(int i = 0; i < num_deps; i++) departures[i]->num_next_nodes = 0;

	struct Itin_Spec_Thread_Args thread_args = {
			departures,
			calc_data.jd_min_dep,
			calc_data.jd_max_dep,
			calc_data.jd_max_arr,
			calc_data.bodies,
			calc_data.max_duration,
			calc_data.num_steps,
			calc_data.system,
			&calc_data.dv_filter
	};

	thread_progress_and_prop_info = (struct Thread_Progress_and_Prop_Info) {
		departures,
		calc_data.jd_min_dep,
		calc_data.jd_max_dep
	};

	show_progress("Transfer Calculation progress: ", 0, 1);
	struct Thread_Pool thread_pool = use_thread_pool64(calc_spec_itin_from_departure, &thread_args);
	join_thread_pool(thread_pool);
	show_progress("Transfer Calculation progress: ", 1, 1);
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