#include "transfer_calc.h"
#include "transfer_tools.h"
#include "tools/tool_funcs.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "tools/thread_pool.h"
#include "tools/data_tool.h"
#include "double_swing_by.h"


struct Itin_Thread_Args {
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

// TODO: rename
struct Itin_Thread_Args2 {
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




int initial_min_transfer_duration[10][10] = {
		{},
		{},
		{},
		{0, 50, 70, 0, 100, 500},
		{},
		{},
		{},
		{},
		{},
		{}
};

int initial_max_transfer_duration[10][10] = {
		{},
		{},
		{},
		{0, 200, 300, 0, 500, 2000},
		{},
		{},
		{},
		{},
		{},
		{}
};




void *calc_from_departure(void *args) {
	struct Itin_Thread_Args *thread_args = (struct Itin_Thread_Args *)args;

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
				if(!calc_next_step(curr_step, ephems, bodies, min_duration, max_duration, thread_args->dv_filter, num_steps, 2)) {
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



// TODO: rename
void *calc_from_departure2(void *args) {
	struct Itin_Thread_Args2 *thread_args = (struct Itin_Thread_Args2 *)args;

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
		for(int i = 0; i <= 9; i++) {
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

		for(int body_id = 1; body_id <= 8; body_id++) {
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

// TODO: rename
void test_itinerary() {
	struct Transfer_Calc_Results results = {NULL, 0};

	struct timeval start, end;
	double elapsed_time;
	gettimeofday(&start, NULL);  // Record the ending time

	int num_bodies = 8+2;	// planets + arrival body + departure body (arr and dep body ephems = NULL if planet)
	int num_body_ephems = 12*100;	// 12 months for 100 years (1950-2050)
	struct Ephem **body_ephems = (struct Ephem**) malloc(num_bodies*sizeof(struct Ephem*));
	body_ephems[0] = NULL;
	body_ephems[9] = NULL;
	for(int i = 1; i <= num_bodies-2; i++) {
		body_ephems[i] = (struct Ephem*) malloc(num_body_ephems*sizeof(struct Ephem));
		get_body_ephem(body_ephems[i], i);
	}

	struct Body *dep_body = EARTH();
	struct Body *arr_body = JUPITER();

	// Voyager
//	double jd_min_dep = 2441683.5000000;
//	double jd_max_dep = 2442413.5000000;
//	double jd_max_arr = 2449718.5000000;

// Earth -> Jupiter 1967
	double jd_min_dep = 2439500.5000000;
	double jd_max_dep = 2439920.5000000;
	double jd_max_arr = 2442510.5000000;

	int num_deps = (int) (jd_max_dep-jd_min_dep+1);

	int max_duration = 3000;

	struct ItinStep **departures = (struct ItinStep**) malloc(num_deps * sizeof(struct ItinStep*));
	for(int i = 0; i < num_deps; i++) departures[i] = (struct ItinStep*) malloc(sizeof(struct ItinStep));

	struct Dv_Filter dv_filter = {7000, 1e6, 1e6, 0};

	struct Itin_Thread_Args2 thread_args = {
			departures,
			jd_min_dep,
			jd_max_dep,
			jd_max_arr,
			dep_body,
			arr_body,
			body_ephems,
			max_duration,
			&dv_filter
	};

	show_progress("Transfer Calculation progress", 0, 1);
	struct Thread_Pool thread_pool = use_thread_pool64(calc_from_departure2, &thread_args);
	join_thread_pool(thread_pool);
//	calc_from_departure2(&thread_args);
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

	// TODO: Delete later
//	double x[2000];
//	for(int i = 0; i < num_deps; i++) {
//		for(int j = 0; j < departures[i]->num_next_nodes; j++) {
//			for(int k = 0; k < departures[i]->next[j]->num_next_nodes; k++) {
//				print_date(convert_JD_date(departures[i]->date), 0);
//				printf("       ");
//				print_date(convert_JD_date(departures[i]->next[j]->date), 0);
//				printf("       ");
//				print_date(convert_JD_date(departures[i]->next[j]->next[k]->date), 1);
//				printf("----  %s      %s      %s    (%f)----\n", departures[i]->body->name, departures[i]->next[j]->body->name, departures[i]->next[j]->next[k]->body->name,
//					   dv_circ(departures[i]->body, 200e3, vector_mag(subtract_vectors(departures[i]->next[j]->v_dep, departures[i]->v_body))));
//			}
//		}
//	}
//	print_double_array("x", x, 2000);
//	print_date(convert_JD_date(departures[0]->date), 1);

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






	// TODO: Remove later
//	struct ItinStep **arrivals = malloc(num_itins * sizeof(struct ItinStep*));
//	int arridx = 0;
//	for(int i = 0; i < num_deps; i++) {
//		store_itineraries_in_array(departures[i], arrivals, &arridx);
//	}
//	for(int i = 0; i < num_itins; i++) {
//		struct ItinStep *arr = arrivals[i];
//		struct ItinStep *ptr = arr;
//		int num_layers = 1;
//		while(ptr->prev != NULL) {num_layers++; ptr = ptr->prev;}
//		for(int j = num_layers-1; j >= 0; j--) {
//			ptr = arr;
//			for(int k = 0; k < j; k++) ptr = ptr->prev;
//			if(j != num_layers-1) printf("  ");
//			print_date(convert_JD_date(ptr->date), 0);
//		}
//		printf("\n");
//		for(int j = num_layers-1; j >= 0; j--) {
//			ptr = arr;
//			for(int k = 0; k < j; k++) ptr = ptr->prev;
//			if(j != num_layers-1) printf(" -> ");
//			printf("%s", ptr->body->name);
//			ptr = ptr->prev;
//		}
//		printf("\n");
//	}
	for(int i = 0; i < num_deps; i++) {
		free_itinerary(departures[i]);
	}
}

struct Transfer_Calc_Results create_itinerary(struct Transfer_Calc_Data calc_data) {
	struct Transfer_Calc_Results results = {NULL, 0};

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

	struct Itin_Thread_Args thread_args = {
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

	show_progress("Transfer Calculation progress: ", 0, 1);
	struct Thread_Pool thread_pool = use_thread_pool64(calc_from_departure, &thread_args);
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



void test_dsb() {
	struct Body *bodies[6];

	int num_bodies = 8+2;	// planets + arrival body + departure body (arr and dep body ephems = NULL if planet)
	int num_body_ephems = 12*100;	// 12 months for 100 years (1950-2050)
	struct Ephem **body_ephems = (struct Ephem**) malloc(num_bodies*sizeof(struct Ephem*));
	body_ephems[0] = NULL;
	body_ephems[9] = NULL;
	for(int i = 1; i <= num_bodies-2; i++) {
		body_ephems[i] = (struct Ephem*) malloc(num_body_ephems*sizeof(struct Ephem));
		get_body_ephem(body_ephems[i], i);
	}

	int min_duration[6], max_duration[6];
	int counter = 0;
	double jd_min_dep, jd_max_dep;


	bodies[counter] = EARTH();
	struct Date min_date = {1997, 10, 06};	// Cassini: 1997-10-06
	struct Date max_date = {1997, 10, 06};
	jd_min_dep = convert_date_JD(min_date);
	jd_max_dep = convert_date_JD(max_date);
	counter++;

	bodies[counter] = VENUS();
	min_duration[counter] = 197;	// Cassini 197
	max_duration[counter] = 197;
	counter++;

	bodies[counter] = VENUS();
	min_duration[counter] = 400;	// Cassini 425
	max_duration[counter] = 600;
	counter++;

	bodies[counter] = EARTH();
	min_duration[counter] = 57;	// Cassini 65
	max_duration[counter] = 57;
	counter++;

	bodies[counter] = JUPITER();
	min_duration[counter] = 200;	// Cassini 559
	max_duration[counter] = 2000;
	counter++;

	bodies[counter] = SATURN();
	min_duration[counter] = 800;	// Cassini 1273
	max_duration[counter] = 2000;

	double test[1000];
	int numtest = 0;

	struct Dv_Filter dv_filter = {1e9, 1e9, 1e9};
	double t[4], tf_duration[4];

	double all_d_v[100000], all_dep[100000], all_fb0[100000], all_fb1[100000], all_fb2[100000], all_alt[100000], all_ddv[100000];
	double id_v[10000], idep[10000], ifb0[10000], ifb1[10000], ifb2[10000], ialt[100000], iddv[100000];;
	double d_v[10000], dep[10000], fb0[10000], fb1[10000], fb2[10000];
	int num_all = 0, inum_viable = 0, num_viable = 0;
	struct Vector2D error1ddv[1000];
	struct Vector2D error1alt[1000];
	struct Vector2D error2ddv[1000];
	struct Vector2D error2alt[1000];
	struct Vector2D viable_dv[1000];

	struct Vector2D best_dvs[1000];

//	for(tf_duration[2] = min_duration[3]; tf_duration[2] <= max_duration[3]; tf_duration[2] += 40.0) {
	for(double ttt2 = 460; ttt2 <= 550; ttt2 += 5) {
		double venus_period = 224.7;
		double step = venus_period/10;
		printf("%f\n", ttt2);
		error1alt[0].x = 0;
		error1ddv[0].x = 0;
		error2alt[0].x = 0;
		error2ddv[0].x = 0;
		viable_dv[0].x = 0;
		for(int c = 0; c < 5; c++) {
			if(c == 0) {
				for(int j = 0; j < 9; j++) {
					test[j] = venus_period*2 + (j-4)*step;
				}
				numtest = 9;
			} else if(c == 1) {
				step /= 4;
				numtest = 0;
				for(int i = 0; i < error1ddv[0].x; i++) {
					if(error1ddv[i+1].y < 1) {
						for(int j = -2; j <= 2; j++) {
							if(j == 0) continue;
							test[numtest] = error1ddv[i + 1].x + j * step;
							numtest++;
						}
					}
				}
			} else if(c == 2) {
				step /= 8;
				numtest = 0;
				for(int i = 0; i < error1ddv[0].x; i++) {
					if(error1alt[i+1].y > bodies[3]->radius + bodies[3]->atmo_alt) {
						for(int j = -4; j <= 4; j++) {
							if(j == 0) continue;
							test[numtest] = error1ddv[i + 1].x + j * step;
							numtest++;
						}
					}
				}
			} else if(c == 3) {
				step /= 4;
				numtest = 0;
				for(int i = 0; i < error2ddv[0].x; i++) {
					if(error2ddv[i+1].y < 1) {
						for(int j = -2; j <= 2; j++) {
							if(j == 0) continue;
							test[numtest] = error2ddv[i + 1].x + j * step;
							numtest++;
						}
					}
				}
			} else if(c == 4) {
				step /= 16;
				numtest = 0;
				int minidx = get_min_value_index_from_data(viable_dv);
				for(int j = -16; j <= 16; j++) {
					if(j == 0) continue;
					test[numtest] = viable_dv[minidx].x + j * step;
					numtest++;
				}
			}

			for(double jd_dep = jd_min_dep; jd_dep <= jd_max_dep; jd_dep+=1.0) {
				for(tf_duration[0] = min_duration[1]; tf_duration[0] <= max_duration[1]; tf_duration[0] += 40.0) {
		//			for(tf_duration[1] = min_duration[2]; tf_duration[1] <= max_duration[2]; tf_duration[1]+=1) {
					for(int testIdx = 0; testIdx < numtest; testIdx++) {

						tf_duration[1] = test[testIdx];

						t[0] = jd_dep + tf_duration[0];
						t[1] = t[0] + tf_duration[1];
						//t[2] = t[1] + tf_duration[2];
						t[2] = t[0] + ttt2;
						tf_duration[2] = t[2] - t[1];

						if(t[2] < t[1]) continue;

						struct OSV osv_dep = osv_from_ephem(body_ephems[bodies[0]->id], jd_dep, SUN());
						struct OSV dsb_osv[3] = {
								osv_from_ephem(body_ephems[bodies[1]->id], t[0], SUN()),
								osv_from_ephem(body_ephems[bodies[2]->id], t[1], SUN()),
								osv_from_ephem(body_ephems[bodies[3]->id], t[2], SUN())
						};

						double data[3];
						struct Transfer tf_launch = calc_transfer(circfb, EARTH(), VENUS(), osv_dep.r, osv_dep.v,
																  dsb_osv[0].r,
																  dsb_osv[0].v, tf_duration[0] * 86400, data);
						struct Transfer tf_after_dsb = calc_transfer(circfb, bodies[2], bodies[3], dsb_osv[1].r,dsb_osv[1].v,
																	 dsb_osv[2].r, dsb_osv[2].v, tf_duration[2] * 86400,
																	 NULL);

						struct OSV s0 = {tf_launch.r1, tf_launch.v1};
						struct OSV p0 = {dsb_osv[0].r, dsb_osv[0].v};
						struct OSV s1 = {tf_after_dsb.r0, tf_after_dsb.v0};
						struct OSV p1 = {dsb_osv[1].r, dsb_osv[1].v};

						struct DSB dsb = calc_double_swing_by(s0, p0, s1, p1, tf_duration[1], bodies[1]);

						if(dsb.dv < 3000) {
							all_d_v[num_all] = dsb.dv;
							all_dep[num_all] = jd_dep;
							all_fb0[num_all] = tf_duration[0];
							all_fb1[num_all] = tf_duration[1];
							all_fb2[num_all] = tf_duration[2];
							num_all++;
						} else continue;

						struct ItinStep itin_step = {
								bodies[3],
								tf_after_dsb.r1,
								tf_after_dsb.v1,
								dsb_osv[2].v,
								tf_after_dsb.v0,
								t[2],
								0,
								NULL,
								NULL
						};

						find_viable_flybys(&itin_step, body_ephems[bodies[4]->id], bodies[4], min_duration[4] * 86400,
										   max_duration[4] * 86400);


						insert_new_data_point2(error1alt, tf_duration[1], get_best_alt());
						insert_new_data_point2(error1ddv, tf_duration[1], get_best_diff_vinf());

						all_alt[num_all - 1] = get_best_alt();
						all_ddv[num_all - 1] = get_best_diff_vinf();

						if(itin_step.num_next_nodes > 0) {
							id_v[inum_viable] = dsb.dv;
							idep[inum_viable] = jd_dep;
							ifb0[inum_viable] = tf_duration[0];
							ifb1[inum_viable] = tf_duration[1];
							ifb2[inum_viable] = tf_duration[2];
							ifb2[inum_viable] = tf_duration[2];
							ialt[inum_viable] = get_best_alt();
							iddv[inum_viable] = get_best_diff_vinf();
							inum_viable++;
						}
	//					printf("num next nodes: %d\n", itin_step.num_next_nodes);

						for(int i = 0; i < itin_step.num_next_nodes; i++) {
							find_viable_flybys(itin_step.next[i], body_ephems[bodies[5]->id], bodies[5],
											   min_duration[5] * 86400,
											   max_duration[5] * 86400);
							insert_new_data_point2(error2alt, tf_duration[1], get_best_alt());
							insert_new_data_point2(error2ddv, tf_duration[1], get_best_diff_vinf());
							if(itin_step.next[i]->num_next_nodes > 0) {
								insert_new_data_point2(viable_dv, tf_duration[1], dsb.dv);
								printf("SAT num next nodes: %d\n", itin_step.next[i]->num_next_nodes);
								printf("%f\n", dsb.dv);
								print_double_array("data", data, 3);

								d_v[num_viable] = dsb.dv;
								dep[num_viable] = jd_dep;
								fb0[num_viable] = tf_duration[0];
								fb1[num_viable] = tf_duration[1];
								fb2[num_viable] = tf_duration[2];
								num_viable++;
							}
						}

						for(int i = 0; i < itin_step.num_next_nodes; i++) {
							free_itinerary(itin_step.next[i]);
						}

					}
				}
			}
		}
		if(viable_dv[0].x > 0) insert_new_data_point2(best_dvs, t[2], viable_dv[get_min_value_index_from_data(viable_dv)].y);
	}

//	print_double_array("all_d_v", all_d_v, num_all);
//	print_double_array("all_dep", all_dep, num_all);
//	print_double_array("all_fb0", all_fb0, num_all);
//	print_double_array("all_fb1", all_fb1, num_all);
//	print_double_array("all_fb2", all_fb2, num_all);
//	print_double_array("all_alt", all_alt, num_all);
//	print_double_array("all_ddv", all_ddv, num_all);
//	printf("\n");
//	print_double_array("id_v", id_v, inum_viable);
//	print_double_array("idep", idep, inum_viable);
//	print_double_array("ifb0", ifb0, inum_viable);
//	print_double_array("ifb1", ifb1, inum_viable);
//	print_double_array("ifb2", ifb2, inum_viable);
//	print_double_array("ialt", ialt, inum_viable);
//	print_double_array("iddv", iddv, inum_viable);
//	printf("\n");
//	print_double_array("d_v", d_v, num_viable);
//	print_double_array("dep", dep, num_viable);
//	print_double_array("fb0", fb0, num_viable);
//	print_double_array("fb1", fb1, num_viable);
//	print_double_array("fb2", fb2, num_viable);
//	printf("\n");
//
//	print_data_vector("fbe1", "errorddv1", error1ddv);
//	print_data_vector("fbe1", "erroralt1", error1alt);
//	print_data_vector("fbe2", "errorddv2", error2ddv);
//	print_data_vector("fbe2", "erroralt2", error2alt);


	print_data_vector("t2", "best_dvs", best_dvs);

	for(int i = 0; i < num_bodies; i++) free(body_ephems[i]);
	free(body_ephems);
}
