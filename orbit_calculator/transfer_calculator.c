#include "transfer_calculator.h"
#include "transfer_tools.h"
#include "tools/csv_writer.h"
#include "tools/tool_funcs.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "tools/thread_pool.h"
#include "itin_tool.h"


struct Itin_Thread_Args {
	double jd_min_dep;
	double jd_max_dep;
	struct ItinStep **departures;
	struct Ephem **ephems;
	struct Body **bodies;
	int *min_duration;
	int *max_duration;
	int num_steps;
};

void *calc_from_departure(void *args) {
	struct Itin_Thread_Args *thread_args = (struct Itin_Thread_Args *)args;

	int *min_duration = thread_args->min_duration;
	int *max_duration = thread_args->max_duration;
	struct Ephem **ephems = thread_args->ephems;
	struct Body **bodies = thread_args->bodies;
	int num_steps = thread_args->num_steps;

	int index = get_thread_counter();
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

			struct Transfer tf = calc_transfer(circfb, bodies[0], bodies[1], osv_body0.r, osv_body0.v,
											   osv_body1.r, osv_body1.v, (jd_arr - jd_dep) * 86400, NULL);

			while(curr_step->prev != NULL) curr_step = curr_step->prev;
			curr_step->next[j - fb1_del] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
			curr_step->next[j - fb1_del]->prev = curr_step;
			curr_step = curr_step->next[j - fb1_del];
			curr_step->body = bodies[1];
			curr_step->date = jd_arr;
			curr_step->r = osv_body1.r;
			curr_step->v_dep = tf.v0;
			curr_step->v_arr = tf.v1;
			curr_step->v_body = osv_body1.v;
			curr_step->num_next_nodes = 0;
			curr_step->next = NULL;

			if(num_steps > 2) {
				if(!calc_next_step(curr_step, ephems, bodies, min_duration, max_duration, num_steps, 2)) {
					fb1_del++;
				}
			}
		}
		show_progress("Transfer Calculation progress: ", index, jd_diff);
		index = get_thread_counter();
		jd_dep = thread_args->jd_min_dep + index;
	}
	return NULL;
}

void create_itinerary() {
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

//	struct Body *bodies[] = {EARTH(), VENUS(), MARS(), EARTH()};
//	int num_steps = sizeof(bodies)/sizeof(struct Body*);
//
//	struct Date min_dep_date = {1959, 1, 1};
//	struct Date max_dep_date = {1959, 12, 31};
//	double jd_min_dep = convert_date_JD(min_dep_date);
//	double jd_max_dep = convert_date_JD(max_dep_date);
//	int num_deps = (int) (jd_max_dep-jd_min_dep+1);
//
//	int min_duration[] = {150, 100, 50};
//	int max_duration[] = {200, 500, 500};

//	struct Body *bodies[] = {EARTH(), JUPITER(), SATURN(), URANUS(), NEPTUNE()};
//	int num_steps = sizeof(bodies)/sizeof(struct Body*);
//
//	struct Date min_dep_date = {1977, 5, 15};
//	struct Date max_dep_date = {1977, 11, 21};
//	double jd_min_dep = convert_date_JD(min_dep_date);
//	double jd_max_dep = convert_date_JD(max_dep_date);
//	int num_deps = (int) (jd_max_dep-jd_min_dep+1);
//
//	int min_duration[] = {500, 100, 300, 300};
//	int max_duration[] = {1000, 3000, 3000, 3000};


	struct Body *bodies[] = {EARTH(), VENUS(), VENUS(), EARTH(), JUPITER(), SATURN()};
	int num_steps = sizeof(bodies)/sizeof(struct Body*);

	struct Date min_dep_date = {1997, 10, 12};
	struct Date max_dep_date = {1997, 10, 17};
	double jd_min_dep = convert_date_JD(min_dep_date);
	double jd_max_dep = convert_date_JD(max_dep_date);
	int num_deps = (int) (jd_max_dep-jd_min_dep+1);

	int min_duration[] = {194, 422, 52, 200, 500};
	int max_duration[] = {198, 427, 57, 1000, 2000};

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
			jd_min_dep,
			jd_max_dep,
			departures,
			ephems,
			bodies,
			min_duration,
			max_duration,
			num_steps
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


	int num_itins = 0, tot_num_itins = 0;
	for(int i = 0; i < num_deps; i++) num_itins += get_number_of_itineraries(departures[i]);
	for(int i = 0; i < num_deps; i++) tot_num_itins += get_total_number_of_stored_steps(departures[i]);

	gettimeofday(&end, NULL);  // Record the ending time
	elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
	printf("----- | Total elapsed time: %.3f s | ---------\n", elapsed_time);

	if(num_itins == 0) {
		printf("\nNo itineraries found!\n");
		for(int i = 0; i < num_deps; i++) free_itinerary(departures[i]);
		free(departures);
		for(int i = 0; i < num_bodies; i++) free(body_ephems[i]);
		free(body_ephems);
		free(ephems);
		return;
	} else printf("\n%d itineraries found!\nNumber of Nodes: %d\n", num_itins, tot_num_itins);


	int index = 0;
	struct ItinStep **arrivals = (struct ItinStep**) malloc(num_itins * sizeof(struct ItinStep*));
	for(int i = 0; i < num_deps; i++) store_itineraries_in_array(departures[i], arrivals, &index);

	double *porkchop = (double *) malloc((5 * num_itins + 1) * sizeof(double));
	porkchop[0] = 0;
	for(int i = 0; i < num_itins; i++) {
		create_porkchop_point(arrivals[i], &porkchop[i*5+1]);
		porkchop[0] += 5;
	}

	char data_fields[] = "dep_date,duration,dv_dep,dv_mcc,dv_arr";
	write_csv(data_fields, porkchop);

	double mindv = porkchop[1+2]+porkchop[1+3]+porkchop[1+4];
	int mind = 0;
	for(int i = 1; i < num_itins; i++) {
		double dv = porkchop[1+i*5+2]+porkchop[1+i*5+3]+porkchop[1+i*5+4];
		if(dv < mindv) {
			mindv = dv;
			mind = i;
		}
	}

	printf("\nBest for capture:\n");
	print_itinerary(arrivals[mind]);
	printf("  -  %f m/s\n", mindv);

	mindv = porkchop[1+2]+porkchop[1+3]+porkchop[1+4];
	mind = 0;
	for(int i = 1; i < num_itins; i++) {
		double dv = porkchop[1+i*5+2]+porkchop[1+i*5+3];
		if(dv < mindv) {
			mindv = dv;
			mind = i;
		}
	}

	printf("\nBest for fly-by:\n");
	print_itinerary(arrivals[mind]);
	printf("  -  %f m/s\n\n", mindv);

	store_itineraries_in_bfile(departures, tot_num_itins, num_deps);

	for(int i = 0; i < num_deps; i++) free_itinerary(departures[i]);
	free(departures);
	free(arrivals);
	free(porkchop);
	for(int i = 0; i < num_bodies; i++) free(body_ephems[i]);
	free(body_ephems);
	free(ephems);
}
