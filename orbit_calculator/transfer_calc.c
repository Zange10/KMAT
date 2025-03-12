#include "transfer_calc.h"
#include "transfer_tools.h"
#include "tools/tool_funcs.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "tools/thread_pool.h"


struct Itin_Spec_Thread_Args {
	struct ItinStep **departures;
	double jd_min_dep;
	double jd_max_dep;
	struct Ephem **ephems;
	struct Body **bodies;
	int *min_duration;
	int *max_duration;
	int num_steps;
	struct Dv_Filter *dv_filter;
};

struct Itin_To_Target_Thread {
	struct ItinStep **departures;
	double jd_min_dep;
	double jd_max_dep;
	double jd_max_arr;
	struct Body *dep_body;
	struct Body *arr_body;
	struct Ephem **body_ephems;
	int max_duration;
	struct Dv_Filter *dv_filter;
};

struct Thread_Progress_and_Prop_Info {
	struct ItinStep **departures;
	double jd_min_dep;
	double jd_max_dep;
} thread_progress_and_prop_info;




int initial_min_transfer_duration[11][11] = {
		{},
		{},
		{},
		{0, 50, 70, 0, 100, 500},
		{},
		{0, 0, 400, 400, 400},
		{},
		{},
		{},
		{},
		{}
};

int initial_max_transfer_duration[11][11] = {
		{},
		{},
		{},
		{0, 200, 300, 0, 500, 2000},
		{},
		{0, 0, 2000, 2000, 2000},
		{},
		{},
		{},
		{},
		{}
};




void *calc_spec_itin_from_departure(void *args) {
	struct Itin_Spec_Thread_Args *thread_args = (struct Itin_Spec_Thread_Args *)args;

	int *min_duration = thread_args->min_duration;
	int *max_duration = thread_args->max_duration;
	struct Ephem **ephems = thread_args->ephems;
	struct Body **bodies = thread_args->bodies;
	int num_steps = thread_args->num_steps;

	int index = get_incr_thread_counter(0);
	// increase finished counter to 1 (first finished should reflect a num of finished of 1)
	if(index == 0) get_incr_thread_counter(1);

	double jd_dep = thread_args->jd_min_dep + index;
	struct ItinStep *curr_step;

	double jd_diff = thread_args->jd_max_dep-thread_args->jd_min_dep+1;

	while(jd_dep <= thread_args->jd_max_dep) {
		struct OSV osv_body0 = osv_from_ephem(ephems[0], jd_dep, SUN());

		curr_step = thread_args->departures[index];
		curr_step->body = bodies[0];
		curr_step->date = jd_dep;
		curr_step->r = osv_body0.r;
		curr_step->v_body = osv_body0.v;
		curr_step->v_dep = vec(0, 0, 0);
		curr_step->v_arr = vec(0, 0, 0);
		curr_step->num_next_nodes = max_duration[0] - min_duration[0] + 1;
		curr_step->prev = NULL;
		curr_step->next = (struct ItinStep **) malloc(
				(max_duration[0] - min_duration[0] + 1) * sizeof(struct ItinStep *));

		int fb1_del = 0;

		for(int j = 0; j <= max_duration[0] - min_duration[0]; j++) {
			double jd_arr = jd_dep + min_duration[0] + j;
			struct OSV osv_body1 = osv_from_ephem(ephems[1], jd_arr, SUN());

			double data[3];

			struct Transfer tf = calc_transfer(circfb, bodies[0], bodies[1], osv_body0.r, osv_body0.v,
											   osv_body1.r, osv_body1.v, (jd_arr - jd_dep) * 86400, data);


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

			if(num_steps > 2) {
				if(!calc_next_spec_itin_step(curr_step, ephems, bodies, min_duration, max_duration,
											 thread_args->dv_filter, num_steps, 2)) {
					fb1_del++;
				}
			}
		}
		double progress = get_incr_thread_counter(1);
		show_progress("Transfer Calculation progress: ", progress, jd_diff);
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
	struct Body *dep_body = thread_args->dep_body;
	struct Body *arr_body = thread_args->arr_body;
	int max_total_duration = thread_args->max_duration;
	struct Ephem **body_ephems = thread_args->body_ephems;

	int index = get_incr_thread_counter(0);
	// increase finished counter to 1 (first finished should reflect a num of finished of 1)
	if(index == 0) get_incr_thread_counter(1);

	double jd_dep = jd_min_dep + index;
	struct ItinStep *curr_step;
	struct OSV osv_body0, osv_body1;

	// used for progress feedback
	double jd_diff = jd_max_dep-jd_min_dep+1;

	while(jd_dep <= jd_max_dep) {
		if(body_ephems[0] != NULL)
			osv_body0 = osv_from_ephem(body_ephems[0], jd_dep, SUN());
		else
			osv_body0 = osv_from_ephem(body_ephems[dep_body->id], jd_dep, SUN());

		curr_step = departures[index];
		curr_step->body = dep_body;
		curr_step->date = jd_dep;
		curr_step->r = osv_body0.r;
		curr_step->v_body = osv_body0.v;
		curr_step->v_dep = vec(0, 0, 0);
		curr_step->v_arr = vec(0, 0, 0);
		curr_step->num_next_nodes = 0;
		for(int i = 0; i <= 10; i++) {
			int min_duration = initial_min_transfer_duration[dep_body->id][i];
			int max_duration = initial_max_transfer_duration[dep_body->id][i];
			int max_min_duration_diff = max_duration - min_duration;
			if(max_duration == 0) max_min_duration_diff = -1; // gets added afterward to 1 making it 0
			curr_step->num_next_nodes += max_min_duration_diff+1;
		}
		curr_step->prev = NULL;
		curr_step->next = (struct ItinStep **) malloc(curr_step->num_next_nodes * sizeof(struct ItinStep *));

		int next_step_id = 0;

		struct Body *next_step_body;

		for(int body_id = 1; body_id <= 10; body_id++) {
			int min_duration = initial_min_transfer_duration[dep_body->id][body_id];
			int max_duration = initial_max_transfer_duration[dep_body->id][body_id];
			int max_min_duration_diff = max_duration - min_duration;

			if(max_duration == 0) continue;


			next_step_body = get_body_from_id(body_id);

			for(int j = 0; j <= max_min_duration_diff; j++) {
				double jd_arr = jd_dep + min_duration + j;

				if(jd_arr > jd_max_arr) break;

				osv_body1 = osv_from_ephem(body_ephems[body_id], jd_arr, SUN());

				double data[3];

				struct Transfer tf = calc_transfer(circfb, dep_body, next_step_body, osv_body0.r, osv_body0.v,
												   osv_body1.r, osv_body1.v, (jd_arr - jd_dep) * 86400, data);


				if(data[1] > dv_filter->max_totdv || data[1] > dv_filter->max_depdv) continue;

				curr_step = get_first(curr_step);
				curr_step->next[next_step_id] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
				curr_step->next[next_step_id]->prev = curr_step;
				curr_step->next[next_step_id]->next = NULL;

				curr_step = curr_step->next[next_step_id];

				curr_step->body = get_body_from_id(body_id);
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
			continue_to_next_steps_and_check_for_valid_itins(curr_step, num_of_end_nodes, body_ephems, arr_body, jd_max_arr, max_total_duration, dv_filter);
		}

		double progress = get_incr_thread_counter(1);
		show_progress("Transfer Calculation progress", progress, jd_diff);
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

	int num_bodies = 9+2;	// planets + arrival body + departure body (arr and dep body ephems = NULL if planet)
	int num_body_ephems = 12*100;	// 12 months for 100 years (1950-2050)
	struct Ephem **body_ephems = (struct Ephem**) malloc(num_bodies*sizeof(struct Ephem*));
	body_ephems[0] = NULL;
	body_ephems[10] = NULL;
	for(int i = 1; i <= num_bodies-2; i++) {
		body_ephems[i] = (struct Ephem*) malloc(num_body_ephems*sizeof(struct Ephem));
		get_body_ephem(body_ephems[i], i);
	}

	int num_deps = (int) (calc_data.jd_max_dep-calc_data.jd_min_dep+1);

	struct ItinStep **departures = (struct ItinStep**) malloc(num_deps * sizeof(struct ItinStep*));
	for(int i = 0; i < num_deps; i++) departures[i] = (struct ItinStep*) malloc(sizeof(struct ItinStep));
	for(int i = 0; i < num_deps; i++) departures[i]->num_next_nodes = 0;


	struct Itin_To_Target_Thread thread_args = {
			departures,
			calc_data.jd_min_dep,
			calc_data.jd_max_dep,
			calc_data.jd_max_arr,
			calc_data.dep_body,
			calc_data.arr_body,
			body_ephems,
			calc_data.max_duration,
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

	for(int i = 0; i < num_bodies; i++) free(body_ephems[i]);
	free(body_ephems);

	return results;
}

struct Transfer_Calc_Results search_for_spec_itinerary(struct Transfer_Calc_Data calc_data) {
	struct Transfer_Calc_Results results = {NULL, 0, 0};

	struct timeval start, end;
	double elapsed_time;
	gettimeofday(&start, NULL);  // Record the ending time

	int num_bodies = 9;
	int num_body_ephems = 12*100;	// 12 months for 100 years (1950-2050)
	struct Ephem **body_ephems = (struct Ephem**) malloc(num_bodies*sizeof(struct Ephem*));
	for(int i = 0; i < num_bodies; i++) {
		body_ephems[i] = (struct Ephem*) malloc(num_body_ephems*sizeof(struct Ephem));
		get_body_ephem(body_ephems[i], i+1);
	}

	struct Body **bodies = calc_data.bodies;
	int num_steps = calc_data.num_steps;

	double jd_min_dep = calc_data.jd_min_dep;
	double jd_max_dep = calc_data.jd_max_dep;
	int num_deps = (int) (jd_max_dep-jd_min_dep+1);

	int *min_duration = calc_data.min_duration;
	int *max_duration = calc_data.max_duration;

	struct Ephem **ephems = (struct Ephem**) malloc(num_steps*sizeof(struct Ephem*));
	for(int i = 0; i < num_steps; i++) {
		int ephem_available = 0;
		for(int j = 0; j < i; j++) {
			if(bodies[i] == bodies[j]) {
				ephems[i] = ephems[j];
				ephem_available = 1;
				break;
			}
		}
		if(ephem_available) continue;
		ephems[i] = body_ephems[bodies[i]->id-1];
	}

	struct ItinStep **departures = (struct ItinStep**) malloc(num_deps * sizeof(struct ItinStep*));
	for(int i = 0; i < num_deps; i++) departures[i] = (struct ItinStep*) malloc(sizeof(struct ItinStep));

	struct Itin_Spec_Thread_Args thread_args = {
			departures,
			jd_min_dep,
			jd_max_dep,
			ephems,
			bodies,
			min_duration,
			max_duration,
			num_steps,
			&calc_data.dv_filter
	};

	thread_progress_and_prop_info = (struct Thread_Progress_and_Prop_Info) {
		departures,
		jd_min_dep,
		jd_max_dep
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


	for(int i = 0; i < num_bodies; i++) free(body_ephems[i]);
	free(body_ephems);
	free(ephems);

	return results;
}

struct Transfer_Calc_Status get_current_transfer_calc_status() {
	double jd_diff = thread_progress_and_prop_info.jd_max_dep-thread_progress_and_prop_info.jd_min_dep+1;
	int num_deps = get_thread_counter(1);

	return (struct Transfer_Calc_Status) {
		.num_deps = num_deps,
		.jd_diff = jd_diff,
		.progress = (double)num_deps/jd_diff
	};
}