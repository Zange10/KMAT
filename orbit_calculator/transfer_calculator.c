#include "transfer_calculator.h"
#include "transfer_tools.h"
#include "double_swing_by.h"
#include "tools/csv_writer.h"
#include "tools/tool_funcs.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

enum Transfer_Type final_tt = circcap;

void print_step(struct ItinStep *step) {
	printf("%s\n", step->body->name);
	print_date(convert_JD_date(step->date),1);
	print_vector(scalar_multiply(step->r,1e-9));
	print_vector(scalar_multiply(step->v_dep,1e-3));
	print_vector(scalar_multiply(step->v_arr,1e-3));
	print_vector(scalar_multiply(step->v_body,1e-3));
	printf("Num Nodes: %d\n", step->num_next_nodes);
}

void print_itinerary(struct ItinStep *itin) {
	if(itin->prev != NULL) {
		print_itinerary(itin->prev);
		printf(" - ");
	}
	print_date(convert_JD_date(itin->date), 0);
}

void print_itinerary2(struct ItinStep *itin, int step) {
//	printf("IN %d\n", step);
	if(itin->prev != NULL) printf("|");
	else printf("\n");
	for(int i = 0; i < step; i++) printf("-");
	printf("%d\n", itin->num_next_nodes);
	if(itin->num_next_nodes == 0) {
		return;
	}

	for(int i = 0; i < itin->num_next_nodes; i++) {
		print_itinerary2(itin->next[i], step+1);
	}
//	printf("OUT %d\n", step);
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

int calc_next_step(struct ItinStep *curr_step, struct Ephem **ephems, struct Body **bodies, const int *min_duration, const int *max_duration, int num_steps, int step) {
//	printf("++++  %d %d\n", curr_step->num_next_nodes, step);
	if(bodies[step] != bodies[step-1]) find_viable_flybys(curr_step, ephems, bodies[step], min_duration[step-1]*86400, max_duration[step-1]*86400);
	else {
		find_viable_dsb_flybys(curr_step, ephems, bodies[step+1],
							   min_duration[step-1]*86400, max_duration[step-1]*86400, min_duration[step]*86400, max_duration[step]*86400);
	}
//	printf("++++  %d %d\n", curr_step->num_next_nodes, step);

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
//			printf("STEP: %d, NUM: %d\n", step, curr_step->num_next_nodes);
//			printf("%d/%d\n", i, curr_step->num_next_nodes);
			if(bodies[step] != bodies[step-1]) result = calc_next_step(curr_step->next[i-step_del], ephems, bodies, min_duration, max_duration, num_steps, step+1);
			else result = calc_next_step(curr_step->next[i-step_del]->next[0]->next[0], ephems, bodies, min_duration, max_duration, num_steps, step+2);
			num_valid += result;
			if(!result) step_del++;
//			printf("%d/%d OUT (valid: %d)\n", i, curr_step->num_next_nodes, num_valid);
		}
	} else return 1;

//	if(bodies[k] != bodies[k-1]) curr_step = curr_step->next[0];
//	else {
//		curr_step = curr_step->next[0]->next[0]->next[0];
//		k++;
//	}
	return num_valid > 0;
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

void store_itineraries_in_file_init(struct ItinStep **departures, int num_nodes, int num_deps, int num_steps) {
	char filename[19];  // 14 for date + 4 for .csv + 1 for string terminator
	sprintf(filename, "test.transfer");

	printf("Filesize: ~%f.3 MB", (double)num_nodes*240/1e6);

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

void create_itinerary() {
	struct timeval start, end;
	double elapsed_time;
	gettimeofday(&start, NULL);  // Record the ending time

	int num_bodies = 9;
	int num_ephems = 12*100;	// 12 months for 100 years (1950-2050)
	struct Ephem **ephems = (struct Ephem**) malloc(num_bodies*sizeof(struct Ephem*));
	for(int i = 0; i < num_bodies; i++) {
		ephems[i] = (struct Ephem*) malloc(num_ephems*sizeof(struct Ephem));
		get_body_ephem(ephems[i], i+1);
	}

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

	struct Date min_dep_date = {1997, 10, 13};
	struct Date max_dep_date = {1997, 10, 17};
	double jd_min_dep = convert_date_JD(min_dep_date);
	double jd_max_dep = convert_date_JD(max_dep_date);
	int num_deps = (int) (jd_max_dep-jd_min_dep+1);

	int min_duration[] = {195, 422, 50, 200, 500};
	int max_duration[] = {196, 428, 60, 1000, 2000};

	struct ItinStep **departures = (struct ItinStep**) malloc(num_deps * sizeof(struct ItinStep*));

	for(int i = 0; i < num_deps; i++){
		double jd_dep = jd_min_dep+i;
		print_date(convert_JD_date(jd_dep),1);
		struct OSV osv_body0 = osv_from_ephem(ephems[bodies[0]->id-1], jd_dep, SUN());

		struct ItinStep *curr_step = (struct ItinStep*) malloc(sizeof(struct ItinStep));
		departures[i] = curr_step;
		curr_step->body = bodies[0];
		curr_step->date = jd_dep;
		curr_step->r = osv_body0.r;
		curr_step->v_body = osv_body0.v;
		curr_step->v_dep = vec(0,0,0);
		curr_step->v_arr = vec(0,0,0);
		curr_step->num_next_nodes = max_duration[0]-min_duration[0]+1;
		curr_step->prev = NULL;
		curr_step->next = (struct ItinStep**) malloc((max_duration[0]-min_duration[0]+1) * sizeof(struct ItinStep*));

		int fb1_del = 0;

		for(int j = 0; j <= max_duration[0] - min_duration[0]; j++) {
			double jd_arr = jd_dep + min_duration[0] + j;
			struct OSV osv_body1 = osv_from_ephem(ephems[bodies[1]->id-1], jd_arr, SUN());

			struct Transfer tf = calc_transfer(circfb, bodies[0], bodies[1], osv_body0.r, osv_body0.v,
											   osv_body1.r, osv_body1.v, (jd_arr-jd_dep)*86400, NULL);

			while(curr_step->prev != NULL) curr_step = curr_step->prev;
			curr_step->next[j-fb1_del] = (struct ItinStep*) malloc(sizeof(struct ItinStep));
			curr_step->next[j-fb1_del]->prev = curr_step;
			curr_step = curr_step->next[j-fb1_del];
			curr_step->body = bodies[1];
			curr_step->date = jd_arr;
			curr_step->r = osv_body1.r;
			curr_step->v_dep = tf.v0;
			curr_step->v_arr = tf.v1;
			curr_step->v_body = osv_body1.v;
			curr_step->num_next_nodes = 0;
			curr_step->next = NULL;

			if(!calc_next_step(curr_step, ephems, bodies, min_duration, max_duration, num_steps, 2)){
				fb1_del++;
			}
		}
	}

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


//	for(int i = 0; i < num_deps; i++) {
//		print_itinerary2(departures[i], 0);
//	}
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
		for(int i = 0; i < num_bodies; i++) free(ephems[i]);
		free(ephems);
		return;
	} else printf("\n%d itineraries found!\nNumber of Nodes: %d\n", num_itins, tot_num_itins);


	int index = 0;
	struct ItinStep **arrivals = (struct ItinStep**) malloc(num_itins * sizeof(struct ItinStep*));
	for(int i = 0; i < num_deps; i++) store_itineraries_in_array(departures[i], arrivals, &index);

	double *porkchop = (double *) malloc((5 * num_itins + 1) * sizeof(double));
	porkchop[0] = 0;
	for(int i = 0; i < num_itins; i++) {
		create_porkchop_point(arrivals[i], &porkchop[i*5+1], -1);
		porkchop[0] += 5;
	}

//	for(int i = 0; i < num_itins; i++) {
//		print_itinerary(arrivals[i]);
//		printf("\n");
//	}

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

	print_itinerary(arrivals[mind]);
	printf("  -  %f m/s\n", mindv);

//	store_itineraries_in_file_init(departures, tot_num_itins, num_deps, num_steps);

	for(int i = 0; i < num_deps; i++) free_itinerary(departures[i]);
	free(departures);
	free(arrivals);
	free(porkchop);
	for(int i = 0; i < num_bodies; i++) free(ephems[i]);
	free(ephems);
}

void simple_transfer() {
    struct Body *bodies[2] = {EARTH(), JUPITER()};
    struct Date min_dep_date = {1977, 1, 1, 0, 0, 0};
    struct Date max_dep_date = {1978, 1, 1, 0, 0, 0};
    int min_duration = 400;         // [days]
    int max_duration = 1500;         // [days]
    double dep_time_steps = 24 * 60 * 60; // [seconds]
    double arr_time_steps = 24 * 60 * 60; // [seconds]

    double jd_min_dep = convert_date_JD(min_dep_date);
    double jd_max_dep = convert_date_JD(max_dep_date);

    double jd_max_arr = jd_max_dep + max_duration;

    int ephem_time_steps = 10;  // [days]
    int num_ephems = (int)(jd_max_arr - jd_min_dep) / ephem_time_steps + 1;

    struct Ephem **ephems = (struct Ephem**) malloc(2*sizeof(struct Ephem*));
    for(int i = 0; i < 2; i++) {
        ephems[i] = (struct Ephem*) malloc(num_ephems*sizeof(struct Ephem));
        get_ephem(ephems[i], num_ephems, bodies[i]->id, ephem_time_steps, jd_min_dep, jd_max_arr, 1);
    }

    struct Porkchop_Properties pochopro = {
            jd_min_dep,
            jd_max_dep,
            dep_time_steps,
            arr_time_steps,
            min_duration,
            max_duration,
            ephems,
            bodies[0],
            bodies[1]
    };
    // 4 = dep_time, duration, dv1, dv2
    int data_length = 4;
    int all_data_size = (int) (data_length * (max_duration - min_duration) / (arr_time_steps / (24 * 60 * 60)) *
                               (jd_max_dep - jd_min_dep) / (dep_time_steps / (24 * 60 * 60))) + 1;
    double *porkchop = (double *) malloc(all_data_size * sizeof(double));

    create_porkchop(pochopro, circcirc, porkchop);
    char data_fields[] = "dep_date,duration,dv_dep,dv_arr";
    write_csv(data_fields, porkchop);

    int mind = 0;
    double min = 1e9;

    for(int i = 0; i < porkchop[0]/4; i++) {
        if(porkchop[i*4+4] < min) {
            mind = i;
            min = porkchop[i*4+4];
        }
    }

    double t_dep = porkchop[mind*4+1];
    double t_arr = t_dep + porkchop[mind*4+2];

    free(porkchop);

    struct OSV s0 = osv_from_ephem(ephems[0], t_dep, SUN());
    struct OSV s1 = osv_from_ephem(ephems[1], t_arr, SUN());

    double data[3];
    struct Transfer transfer = calc_transfer(circcirc, bodies[0], bodies[1], s0.r, s0.v, s1.r, s1.v, (t_arr-t_dep) * (24*60*60), data);
    printf("Departure: ");
    print_date(convert_JD_date(t_dep),0);
    printf(", Arrival: ");
    print_date(convert_JD_date(t_arr),0);
    printf(" (%f days), Delta-v: %f m/s (%f m/s, %f m/s)\n",
           t_arr-t_dep, data[1]+data[2], data[1], data[2]);

    int num_states = 4;

    double times[] = {t_dep, t_dep, t_arr, t_arr};
    struct OSV osvs[num_states];
    osvs[0] = s0;
    osvs[1].r = transfer.r0;
    osvs[1].v = transfer.v0;
    osvs[2].r = transfer.r1;
    osvs[2].v = transfer.v1;
    osvs[3] = s1;

    double transfer_data[num_states*7+1];
    transfer_data[0] = 0;

    for(int i = 0; i < num_states; i++) {
        transfer_data[i*7+1] = times[i];
        transfer_data[i*7+2] = osvs[i].r.x;
        transfer_data[i*7+3] = osvs[i].r.y;
        transfer_data[i*7+4] = osvs[i].r.z;
        transfer_data[i*7+5] = osvs[i].v.x;
        transfer_data[i*7+6] = osvs[i].v.y;
        transfer_data[i*7+7] = osvs[i].v.z;
        transfer_data[0] += 7;
    }

    free(ephems[0]);
    free(ephems[1]);
    free(ephems);

    char pcsv;
    printf("Write transfer data to .csv (y/Y=yes)? ");
    scanf(" %c", &pcsv);
    if (pcsv == 'y' || pcsv == 'Y') {
        char transfer_data_fields[] = "JD,X,Y,Z,VX,VY,VZ";
        write_csv(transfer_data_fields, transfer_data);
    }

}

void dsb_test3() {
	int num_bodies = 9;
	int num_ephems = 12*100;	// 12 months for 100 years (1950-2050)
	struct Ephem **ephems = (struct Ephem**) malloc(num_bodies*sizeof(struct Ephem*));
	for(int i = 0; i < num_bodies; i++) {
		ephems[i] = (struct Ephem*) malloc(num_ephems*sizeof(struct Ephem));
		get_body_ephem(ephems[i], i+1);
	}

	if(1) {
		struct Body *bodies[] = {EARTH(), VENUS(), MARS(), EARTH()};
		struct Date dep_date = {1959, 8, 16, 0,0,0};
		struct Date ven_date = {1960, 2, 7, 0,0,0};
		double jd_dep = convert_date_JD(dep_date);
		double jd_ven = convert_date_JD(ven_date);

		struct OSV osv_body0 = osv_from_ephem(ephems[bodies[0]->id-1], jd_dep, SUN());
		struct OSV osv_body1 = osv_from_ephem(ephems[bodies[1]->id-1], jd_ven, SUN());

		struct Transfer tf = calc_transfer(circfb, bodies[0], bodies[1], osv_body0.r, osv_body0.v,
											osv_body1.r, osv_body1.v, (jd_ven-jd_dep)*86400, NULL);


		struct ItinStep *curr_step = (struct ItinStep*) malloc(sizeof(struct ItinStep));
		curr_step->body = bodies[0];
		curr_step->date = jd_dep;
		curr_step->r = osv_body0.r;
		curr_step->v_body = osv_body0.v;
		curr_step->num_next_nodes = 1;
		curr_step->prev = NULL;
		curr_step->next = (struct ItinStep**) malloc(sizeof(struct ItinStep*));
		curr_step->next[0] = (struct ItinStep*) malloc(sizeof(struct ItinStep));
		curr_step->next[0]->prev = curr_step;

		curr_step = curr_step->next[0];
		curr_step->body = bodies[1];
		curr_step->date = jd_ven;
		curr_step->r = osv_body1.r;
		curr_step->v_dep = tf.v0;
		curr_step->v_arr = tf.v1;
		curr_step->v_body = osv_body1.v;
		curr_step->num_next_nodes = 0;
		curr_step->next = NULL;

//		print_vector(scalar_multiply(itin[0].v_body,1e-3));
//		print_vector(scalar_multiply(itin[0].r, 1e-9));
//		print_vector(scalar_multiply(itin[1].v_dep,1e-3));
//		print_vector(scalar_multiply(itin[1].v_arr, 1e-3));
//		print_vector(scalar_multiply(itin[1].v_body,1e-3));
//		print_vector(scalar_multiply(itin[1].r, 1e-9));


		struct timeval start, end;
		double elapsed_time;
		gettimeofday(&start, NULL);  // Record the ending time

		find_viable_flybys(curr_step, ephems, bodies[2], 20*86400, 400*86400);

		for(int i = 0; i < curr_step->num_next_nodes; i++) {
			find_viable_flybys(curr_step->next[i], ephems, bodies[3], 20*86400, 400*86400);

			if(curr_step->next[i]->num_next_nodes > 0) {
				printf("--  %f  --\n", curr_step->next[i]->next[0]->date);
				print_date(convert_JD_date(curr_step->prev->date), 1);
				print_date(convert_JD_date(curr_step->date), 1);
				print_date(convert_JD_date(curr_step->next[i]->date), 1);
				print_date(convert_JD_date(curr_step->next[i]->next[0]->date), 1);
			}
		}

		gettimeofday(&end, NULL);  // Record the ending time
		elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
		printf("----- | Total elapsed time: %.3f s | ---------\n", elapsed_time);


		while(curr_step->prev != NULL) curr_step = curr_step->prev;
		free_itinerary(curr_step);
	} else {

		int nums[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
		int len = sizeof(nums) / sizeof(int) * 2;

		double y1[len][1002];
		double y2[len][1002];
		double x[len][1002];

		struct timeval start, end;
		double elapsed_time;

		struct Body *bodies[] = {VENUS(), MARS()};
		int min_duration[] = {1};
		int max_duration[] = {500};

		gettimeofday(&start, NULL);  // Record the ending time

		for(int i = 0; i < len; i++) {
			//struct Date min_dep_date = {i <= 11 ? 1997 : 1998, nums[i <= 11 ? i : i - 12], 1, 0, 0, 0};
			struct Date min_dep_date = {1960, 2, 7, 0,0,0};
			double min_dep = convert_date_JD(min_dep_date);
			double jd_dep = min_dep;
			struct OSV osv_dep = osv_from_ephem(ephems[bodies[0]->id - 1], jd_dep, SUN());

			struct OSV osv_arr0 = osv_from_ephem(ephems[bodies[1]->id - 1], jd_dep, SUN());
			struct Vector proj_vec = proj_vec_plane(osv_dep.r, constr_plane(vec(0, 0, 0), osv_arr0.r, osv_arr0.v));
			double theta_conj_opp = angle_vec_vec(proj_vec, osv_arr0.r);
			if(cross_product(proj_vec, osv_arr0.r).z < 0) theta_conj_opp *= -1;
			else theta_conj_opp -= 3.14159256;

			struct OSV osv_arr1 = propagate_orbit_theta(osv_arr0.r, osv_arr0.v, -theta_conj_opp, SUN());
			struct Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, SUN());
			struct Orbit arr1 = constr_orbit_from_osv(osv_arr1.r, osv_arr1.v, SUN());
			double dt0 = arr1.t - arr0.t;

			osv_arr1 = propagate_orbit_theta(osv_arr0.r, osv_arr0.v, -theta_conj_opp + 3.14159256, SUN());
			arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, SUN());
			arr1 = constr_orbit_from_osv(osv_arr1.r, osv_arr1.v, SUN());
			double dt1 = arr1.t - arr0.t;
			while(dt0 < 86400 * 50) dt0 += arr0.period;
			while(dt1 < 86400 * 50) dt1 += arr0.period;
			//printf("%f %f\n", arr0.t/86400, arr1.t/86400);
			printf("[%f, %f], ", dt0 / 86400, dt1 / 86400);

			for(int j = min_duration[0]; j <= max_duration[0]; j++) {
				double jd_arr = jd_dep + j;
				struct OSV osv_arr = osv_from_ephem(ephems[bodies[1]->id - 1], jd_arr, SUN());
				double data[3];

				calc_transfer(circfb, EARTH(), VENUS(), osv_dep.r, osv_dep.v, osv_arr.r, osv_arr.v,
							  (jd_arr - jd_dep) * 86400,
							  data);

				x[i][0]++;
				y1[i][(int) x[i][0]] = data[1];
				x[i][(int) x[i][0]] = j;
			}
		}

		gettimeofday(&end, NULL);  // Record the ending time
		elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
		printf("----- | Total elapsed time: %.3f s | ---------\n", elapsed_time);

		//return;
		printf("x = [");
		for(int i = 0; i < len; i++) {
			if(i != 0) printf(", ");
			printf("[");
			for(int j = 1; j <= x[i][0]; j++) {
				if(j != 1) printf(", ");
				printf("%f", x[i][j]);
			}
			printf("]");
		}
		printf("]\n");
		printf("y1 = [");
		for(int i = 0; i < len; i++) {
			if(i != 0) printf(", ");
			printf("[");
			for(int j = 1; j <= x[i][0]; j++) {
				if(j != 1) printf(", ");
				printf("%f", y1[i][j]);
			}
			printf("]");
		}
		printf("]\n");
		printf("y2 = [");
		for(int i = 0; i < len; i++) {
			if(i != 0) printf(", ");
			printf("[");
			for(int j = 1; j <= x[i][0]; j++) {
				if(j != 1) printf(", ");
				printf("%f", y2[i][j]);
			}
			printf("]");
		}
		printf("]\n");
	}
	for(int i = 0; i < num_bodies; i++) {
		free(ephems[i]);
	}
	free(ephems);
}

void dsb_test() {

//	int nums[] = {1,2,3,4,5,6,7,8,9,10,11,12};
//	int len = sizeof(nums)/sizeof(int)*2;
//
//	double y1[len][1002];
//	double y2[len][1002];
//	double x[len][1002];
//
//	struct timeval start, end;
//	double elapsed_time;
//	int num_bodies = 9;
//	int num_ephems = 12*100;	// 12 months for 100 years (1950-2050)
//	struct Ephem **ephems = (struct Ephem**) malloc(num_bodies*sizeof(struct Ephem*));
//	for(int i = 0; i < num_bodies; i++) {
//		ephems[i] = (struct Ephem*) malloc(num_ephems*sizeof(struct Ephem));
//		get_body_ephem(ephems[i], i+1);
//	}
//
//	struct Body *bodies[] = {EARTH(), VENUS()};
//	int min_duration[] = {1};
//	int max_duration[] = {500};
//
//
//	double min[3] = {1e9, 0, 0};
//	int counter = 0;
//	struct Date date[5];
//
//	gettimeofday(&start, NULL);  // Record the ending time
//
//	for(int i = 0; i < len; i++) {
//		struct Date min_dep_date = {i <= 11 ? 1997 : 1998, nums[i <= 11 ? i : i-12], 1, 0, 0, 0};
//		double min_dep = convert_date_JD(min_dep_date);
//		double jd_dep = min_dep;
//		struct OSV osv_dep = osv_from_ephem(ephems[bodies[0]->id - 1], jd_dep, SUN());
//		for(int j = min_duration[0]; j <= max_duration[0]; j++) {
//			double jd_arr = jd_dep + j;
//			struct OSV osv_arr = osv_from_ephem(ephems[bodies[1]->id - 1], jd_arr, SUN());
//			double data[3];
//
//			calc_transfer(circfb, EARTH(), VENUS(), osv_dep.r, osv_dep.v, osv_arr.r, osv_arr.v, (jd_arr - jd_dep) * 86400,
//						  data);
//
//			x[i][0]++;
//			y1[i][(int) x[i][0]] = data[1];
//			x[i][(int) x[i][0]] = j;
//		}
//	}
//
//	gettimeofday(&end, NULL);  // Record the ending time
//	elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
//	printf("----- | Total elapsed time: %.3f s | ---------\n", elapsed_time);
//
//	for(int i = 0; i < num_bodies; i++) {
//		free(ephems[i]);
//	}
//	free(ephems);
//
//	printf("x = [");
//	for(int i = 0; i < len; i++) {
//		if(i != 0) printf(", ");
//		printf("[");
//		for(int j = 1; j <= x[i][0]; j++) {
//			if(j != 1) printf(", ");
//			printf("%f", x[i][j]);
//		}
//		printf("]");
//	}
//	printf("]\n");
//	printf("y1 = [");
//	for(int i = 0; i < len; i++) {
//		if(i != 0) printf(", ");
//		printf("[");
//		for(int j = 1; j <= x[i][0]; j++) {
//			if(j != 1) printf(", ");
//			printf("%f", y1[i][j]);
//		}
//		printf("]");
//	}
//	printf("]\n");
//	printf("y2 = [");
//	for(int i = 0; i < len; i++) {
//		if(i != 0) printf(", ");
//		printf("[");
//		for(int j = 1; j <= x[i][0]; j++) {
//			if(j != 1) printf(", ");
//			printf("%f", y2[i][j]);
//		}
//		printf("]");
//	}
//	printf("]\n");


	struct timeval start, end;
	double elapsed_time;
	int num_bodies = 9;
	int num_ephems = 12*100;	// 12 months for 100 years (1950-2050)
	struct Ephem **ephems = (struct Ephem**) malloc(num_bodies*sizeof(struct Ephem*));
	for(int i = 0; i < num_bodies; i++) {
		ephems[i] = (struct Ephem*) malloc(num_ephems*sizeof(struct Ephem));
		get_body_ephem(ephems[i], i+1);
	}

	struct Body *bodies[] = {EARTH(), VENUS(), VENUS(), EARTH()}; //, JUPITER(), SATURN()};

	struct Date min_dep_date = {1997, 10, 10, 0, 0, 0};
	struct Date max_dep_date = {1997, 10, 15, 0, 0, 0};
	double min_dep = convert_date_JD(min_dep_date);
	double max_dep = convert_date_JD(max_dep_date);
	int min_duration[] = {198, 425, 55, 480};
	int max_duration[] = {200, 430, 60, 520};


	double min[3] = {1e9, 0, 0};
	int counter = 0;
	struct Date date[5];

	gettimeofday(&start, NULL);  // Record the ending time
	for(int i = 0; i <= max_dep-min_dep; i++) {
		for(int j = min_duration[0]; j <= max_duration[0]; j++) {
			for(int k = min_duration[1]; k <= max_duration[1]; k++) {
				printf("%d/%d   %d/%d   %d/%d\n", i, (int)(max_dep-min_dep), j-min_duration[0], (int)(max_duration[0]-min_duration[0]), k-min_duration[1], (int)(max_duration[1]-min_duration[1]));
				for(int l = min_duration[2]; l <= max_duration[2]; l++) {
					double jd_dep = min_dep + i;
					double jd_sb1 = min_dep + i + j;
					double jd_sb2 = min_dep + i + j + k;
					double jd_arr = min_dep + i + j + k + l;

					struct OSV osv_dep = osv_from_ephem(ephems[bodies[0]->id-1], jd_dep, SUN());
					struct OSV osv_sb1 = osv_from_ephem(ephems[bodies[1]->id-1], jd_sb1, SUN());
					struct OSV osv_sb2 = osv_from_ephem(ephems[bodies[2]->id-1], jd_sb2, SUN());
					struct OSV osv_arr = osv_from_ephem(ephems[bodies[3]->id-1], jd_arr, SUN());
					double data[3];

					struct Transfer transfer_dep = calc_transfer(circfb, EARTH(), VENUS(), osv_dep.r, osv_dep.v, osv_sb1.r, osv_sb1.v, (jd_sb1-jd_dep)*86400, data);
					struct Transfer transfer_arr = calc_transfer(circfb, VENUS(), EARTH(), osv_sb2.r, osv_sb2.v, osv_arr.r, osv_arr.v, (jd_arr-jd_sb2)*86400, NULL);

					struct OSV s0 = {transfer_dep.r1, transfer_dep.v1};
					struct OSV s1 = {transfer_arr.r0, transfer_arr.v0};

					struct DSB dsb = calc_double_swing_by(s0, osv_sb1, s1, osv_sb2, k, bodies[1]);
					counter++;
					if(dsb.dv + data[1] < min[0]) {
						min[0] = dsb.dv + data[1];
						min[1] = data[1];
						min[2] = dsb.dv;
						date[0] = convert_JD_date(jd_dep);
						date[1] = convert_JD_date(jd_sb1);
						date[2] = convert_JD_date(jd_sb1 + dsb.man_time/86400);
						date[3] = convert_JD_date(jd_sb2);
						date[4] = convert_JD_date(jd_arr);
					}
				}
			}
		}
	}


	gettimeofday(&end, NULL);  // Record the ending time
	elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
	printf("----- | Total elapsed time: %.3f s | ---------\n", elapsed_time);

	printf("%d\n", counter);

	for(int i = 0; i < 5; i++) print_date(date[i], 1);
	printf("%f.2m/s + %f.2m/s = %f.2m/s\n", min[1], min[2], min[0]);


	print_x();
	for(int i = 0; i < num_bodies; i++) {
		free(ephems[i]);
	}
	free(ephems);
}

void dsb_test2() {
	struct timeval start, end;
	double elapsed_time;
	int num_bodies = 9;
	int num_ephems = 12*100;	// 12 months for 100 years (1950-2050)
	struct Ephem **ephems = (struct Ephem**) malloc(num_bodies*sizeof(struct Ephem*));
	for(int i = 0; i < num_bodies; i++) {
		ephems[i] = (struct Ephem*) malloc(num_ephems*sizeof(struct Ephem));
		get_body_ephem(ephems[i], i+1);
	}

	struct Body *Galileo_bodies[] = {VENUS(), EARTH(), EARTH(), JUPITER()};
	struct Date Galileo[4]= {
			{1990, 2, 10, 0, 0, 0},
			{1990, 12, 8,0,0,0},
			{1992, 12, 8,0,0,0},
			{1995, 12, 7,0,0,0},
	};

	struct Body *Cassini_bodies[] = {EARTH(), VENUS(), VENUS(), EARTH()};
	struct Date Cassini[4]= {
			{1997, 10, 15, 0, 0, 0},
			{1998, 4, 26,0,0,0},
			{1999, 6, 24,0,0,0},
			{1999, 8, 18,0,0,0},
	};

	struct Body *test_bodies[] = {EARTH(), VENUS(), VENUS(), EARTH()};
	struct Date test[4]= {
			{1970, 9, 1, 0, 0, 0},
			{1970, 12, 2,0,0,0},
			{1972, 4, 3,0,0,0},
			{1973, 11, 4,0,0,0},
	};
	
	struct Body **bodies;
	struct Date *dates;

	int id = 2;

	switch(id) {
		case 0:
			bodies = Cassini_bodies;
			dates = Cassini; break;
		case 1:
			bodies = Galileo_bodies;
			dates = Galileo; break;
		case 2:
			bodies = test_bodies;
			dates = test; break;
	}

	struct Date dep_date = dates[0];
	struct Date sb1_date = dates[1];
	struct Date sb2_date = dates[2];
	struct Date arr_date = dates[3];

	double jd_dep = convert_date_JD(dep_date);
	double jd_sb1 = convert_date_JD(sb1_date);
	double jd_sb2 = convert_date_JD(sb2_date);
	double jd_arr = convert_date_JD(arr_date);

	double durations[] = {jd_sb1-jd_dep, jd_sb2-jd_sb1, jd_arr-jd_sb2};
	
	struct OSV osv_dep = osv_from_ephem(ephems[bodies[0]->id-1], jd_dep, SUN());
	struct OSV osv_sb1 = osv_from_ephem(ephems[bodies[1]->id-1], jd_sb1, SUN());
	struct OSV osv_sb2 = osv_from_ephem(ephems[bodies[2]->id-1], jd_sb2, SUN());
	struct OSV osv_arr = osv_from_ephem(ephems[bodies[3]->id-1], jd_arr, SUN());
	
	struct Transfer transfer_dep = calc_transfer(circfb, EARTH(), VENUS(), osv_dep.r, osv_dep.v, osv_sb1.r, osv_sb1.v, (jd_sb1-jd_dep)*86400, NULL);
	struct Transfer transfer_arr = calc_transfer(circfb, VENUS(), EARTH(), osv_sb2.r, osv_sb2.v, osv_arr.r, osv_arr.v, (jd_arr-jd_sb2)*86400, NULL);
	
	struct OSV s0 = {transfer_dep.r1, transfer_dep.v1};
	struct OSV s1 = {transfer_arr.r0, transfer_arr.v0};
	
	
	gettimeofday(&start, NULL);  // Record the ending time
	struct DSB dsb = calc_double_swing_by(s0, osv_sb1, s1, osv_sb2, durations[1], bodies[1]);
	
	gettimeofday(&end, NULL);  // Record the ending time
	elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
	printf("\n----- | Total elapsed time: %.3f s | ---------\n", elapsed_time);
	
	for(int i = 0; i < num_bodies; i++) {
		free(ephems[i]);
	}
	free(ephems);
}

void create_swing_by_transfer() {
    struct timeval start, end;
    double elapsed_time;


    struct Body *bodies[] = {EARTH(), VENUS(), VENUS(), EARTH(), JUPITER()};
    int num_bodies = (int) (sizeof(bodies)/sizeof(struct Body*));

    //struct Date min_dep_date = {1970, 1, 1, 0, 0, 0};
    //struct Date max_dep_date = {1971, 1, 1, 0, 0, 0};
    //int min_duration[] = {60, 200};//, 1800, 1500};         // [days]
    //int max_duration[] = {250, 500};//, 2600, 2200};         // [days]
    struct Date min_dep_date = {1997, 10, 1, 0, 0, 0};
    struct Date max_dep_date = {1997, 11, 1, 0, 0, 0};
    int min_duration[] = {90, 410, 90, 700};
    int max_duration[] = {250, 430, 150, 900};

    double dep_time_steps = 24 * 60 * 60; // [seconds]
    double arr_time_steps = 24 * 60 * 60; // [seconds]

    double jd_min_dep = convert_date_JD(min_dep_date);
    double jd_max_dep = convert_date_JD(max_dep_date);

    int max_total_dur = 0;
    for(int i = 0; i < num_bodies-1; i++) max_total_dur += max_duration[i];
    double jd_max_arr = jd_max_dep + max_total_dur;

    int ephem_time_steps = 10;  // [days]
    int num_ephems = (int)(jd_max_arr - jd_min_dep) / ephem_time_steps + 1;

    struct Ephem **ephems = (struct Ephem**) malloc(num_bodies*sizeof(struct Ephem*));
    for(int i = 0; i < num_bodies; i++) {
        int ephem_available = 0;
        for(int j = 0; j < i; j++) {
            if(bodies[i] == bodies[j]) {
                ephems[i] = ephems[j];
                ephem_available = 1;
                break;
            }
        }
        if(ephem_available) continue;
        ephems[i] = (struct Ephem*) malloc(num_ephems*sizeof(struct Ephem));
        get_ephem(ephems[i], num_ephems, bodies[i]->id, ephem_time_steps, jd_min_dep, jd_max_arr, 1);
    }

/*
    int c = 0;

    int u = 0;
    int g = 0;

    double x1[100000];
    double x2[100000];
    double x3[100000];
    double x4[100000];
    double x5[100000];
    double x6[10];
    double y1[20000];
    double y[100000];
    double *xs[] = {x1,x2,x3,x4,x5,x6,y,y1};
    int *ints[] = {&c,&u,&g};
    int max_i = 4;
    int us[max_i];

    double a1[500];
    double a2[500];
    double a3[500];
    double a4[500];
    double a5[500];
    double a6[500];


    if(0) {
        bodies[0] = EARTH();
        bodies[1] = VENUS();
        bodies[2] = EARTH();

        struct Date dep_date = {1997, 10, 15, 0, 0, 0};
        struct Date sb1_date = {1998, 04, 26, 0, 0, 0};
        struct Date sb2_date = {1999, 06, 24, 0, 0, 0};
        struct Date arr_date = {1999, 8, 18, 0, 0, 0};

        double jd_dep = convert_date_JD(dep_date);
        double jd_sb1 = convert_date_JD(sb1_date);
        double jd_sb2 = convert_date_JD(sb2_date);
        double jd_arr = convert_date_JD(arr_date);

        for(int i = 0; i < num_bodies; i++) {
            int ephem_available = 0;
            for(int j = 0; j < i; j++) {
                if(bodies[i] == bodies[j]) {
                    ephems[i] = ephems[j];
                    ephem_available = 1;
                    break;
                }
            }
            if(ephem_available) continue;
            ephems[i] = (struct Ephem*) malloc(num_ephems*sizeof(struct Ephem));
            get_ephem(ephems[i], num_ephems, bodies[i]->id, ephem_time_steps, jd_dep, jd_arr, 0);
        }

        double dwb_dur = jd_sb2-jd_sb1;
        struct OSV osv_dep = osv_from_ephem(ephems[0], jd_dep, SUN());
        struct OSV osv_sb1 = osv_from_ephem(ephems[1], jd_sb1, SUN());
        struct OSV osv_sb2 = osv_from_ephem(ephems[1], jd_sb2, SUN());
        struct OSV osv_arr = osv_from_ephem(ephems[2], jd_arr, SUN());

        struct Transfer transfer_dep = calc_transfer(circfb, VENUS(), EARTH(), osv_dep.r, osv_dep.v, osv_sb1.r, osv_sb1.v, (jd_sb1-jd_dep)*86400, NULL);
        struct Transfer transfer_arr = calc_transfer(circfb, EARTH(), JUPITER(), osv_sb2.r, osv_sb2.v, osv_arr.r, osv_arr.v, (jd_arr-jd_sb2)*86400, NULL);

        struct OSV osv0 = {transfer_dep.r1, transfer_dep.v1};
        struct OSV osv1 = {transfer_arr.r0, transfer_arr.v0};


        gettimeofday(&start, NULL);  // Record the starting time
        calc_double_swing_by(osv0, osv_sb1, osv1, osv_sb2, dwb_dur, bodies[1], xs, ints, 0);
        gettimeofday(&end, NULL);  // Record the ending time
        elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        printf("Elapsed time: %f seconds\n", elapsed_time);

        printf("\n%d %d\n", c, u);
        double minabs = 1e9;
        int mind;
        for (int a = 0; a < u; a++) {
            if (y[a] < minabs) {
                minabs = y[a];
                mind = a;
            }
        }
        printf("\nOrbital Period: %f\n", x1[mind]);
        printf("Dur to man: %f\n", x2[mind]);
        printf("theta: %f°\n", x3[mind]);
        printf("phi: %f°\n", x4[mind]);
        printf("Inclination: %f°\n", x5[mind]);
        printf("mindv: %f\n", y[mind]);

        printf("Targeting date: ");
        print_date(convert_JD_date(x2[mind]+jd_sb1),1);

        exit(0);
    }

if(1) {
    double dep0 = jd_min_dep + 240;
    double dwb_dur = 470;
    struct OSV e0 = osv_from_ephem(ephems[0], dep0, SUN());
    dep0 = jd_min_dep + 346;
    struct OSV p0 = osv_from_ephem(ephems[1], dep0, SUN());
    dep0 = jd_min_dep + 346 + dwb_dur;
    struct OSV p1 = osv_from_ephem(ephems[1], dep0, SUN());

    double temp_data[3];
    struct Transfer transfer = calc_transfer(circfb, EARTH(), VENUS(), e0.r, e0.v, p0.r, p0.v, 116.0 * 86400,
                                             temp_data);

    struct OSV osv0 = {transfer.r1, transfer.v1};
    struct OSV osv1 = {p1.r, rotate_vector_around_axis(scalar_multiply(p1.v, 1.2), p1.r, 0)};


    printf("\n---------\n\n\n");
    printf("%f %f %f\n\n", temp_data[0], temp_data[1], temp_data[2]);
    gettimeofday(&start, NULL);  // Record the starting time
    calc_double_swing_by(osv0, p0, osv1, p1, dwb_dur, VENUS(), xs, ints, 0);
    gettimeofday(&end, NULL);  // Record the ending time
    elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Elapsed time: %f seconds\n", elapsed_time);

    char zusatz[] = "a";

    if (0) {
        printf("x1%s = [", zusatz);
        for (int i = 0; i < u; i++) {
            if (i != 0) printf(", ");
            printf("%f", x1[i]);
        }
        printf("]\n");
        printf("x2%s = [", zusatz);
        for (int i = 0; i < u; i++) {
            if (i != 0) printf(", ");
            printf("%f", x2[i]);
        }
        printf("]\n");
        printf("x3%s = [", zusatz);
        for (int i = 0; i < u; i++) {
            if (i != 0) printf(", ");
            printf("%f", x3[i]);
        }
        printf("]\n");
        printf("x4%s = [", zusatz);
        for (int i = 0; i < u; i++) {
            if (i != 0) printf(", ");
            printf("%f", x4[i]);
        }
        printf("]\n");
        printf("x5%s = [", zusatz);
        for (int i = 0; i < u; i++) {
            if (i != 0) printf(", ");
            printf("%f", x5[i]);
        }
        printf("]\n");
        printf("y%s = [", zusatz);
        for (int i = 0; i < u; i++) {
            if (i != 0) printf(", ");
            printf("%f", y[i]);
        }
        printf("]\n");
    }

    printf("\n%d %d\n", c, u);
    double minabs = 1e9;
    int mind;
    for (int a = 0; a < u; a++) {
        if (y[a] < minabs) {
            minabs = y[a];
            mind = a;
        }
    }
    printf("\nOrbital Period: %f\n", x1[mind]);
    printf("Dur to man: %f\n", x2[mind]);
    printf("theta: %f°\n", x3[mind]);
    printf("phi: %f°\n", x4[mind]);
    printf("Inclination: %f°\n", x5[mind]);
    printf("mindv: %f\n", y[mind]);
    printf("\n%f\n", minabs);

} else {

    int b = 0;
    int d = 0;
    double tot_time = 0;
    double x10[10000];
    double x11[10000];
    double x12[10000];


    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            for (int k = 0; k < 5; k++) {
                for (int l = 0; l < 5; l++) {
                    double dep0 = jd_min_dep + 200 + i * 20;
                    double first_travel = 300 + l * 20;
                    double dwb_dur = 420 + j * 20;
                    printf("\n-------------\ndep: ");
                    print_date(convert_JD_date(dep0), 0);
                    printf("   -->  ");
                    print_date(convert_JD_date(jd_min_dep + first_travel), 0);
                    printf("  (i: %d, j: %d, k: %d, l: %d)\ntransfer duration: %fd, v_factor: %f\n-------------\n", i,
                           j, k, l, dwb_dur, 0.75 + k * 0.15);


                    struct OSV e0 = osv_from_ephem(ephems[0], dep0, SUN());
                    dep0 = jd_min_dep + first_travel;
                    struct OSV p0 = osv_from_ephem(ephems[1], dep0, SUN());
                    dep0 = jd_min_dep + first_travel + dwb_dur;
                    struct OSV p1 = osv_from_ephem(ephems[1], dep0, SUN());

                    double temp_data[3];
                    struct Transfer transfer = calc_transfer(circfb, EARTH(), VENUS(), e0.r, e0.v, p0.r, p0.v,
                                                             116.0 * 86400,
                                                             temp_data);

                    struct OSV osv0 = {transfer.r1, transfer.v1};
                    struct OSV osv1 = {p1.r, rotate_vector_around_axis(scalar_multiply(p1.v, 0.8 + k * 0.15), p1.r,
                                                                       deg2rad(3))};


                    x6[0] = -100;
                    x6[1] = -100;
                    x6[2] = -100;
                    x6[3] = -100;

                    //printf("\n---------\n\n\n");
                    //printf("%f %f %f\n\n", temp_data[0], temp_data[1], temp_data[2]);
                    gettimeofday(&start, NULL);  // Record the starting time
                    int via = calc_double_swing_by(osv0, p0, osv1, p1, dwb_dur, VENUS(), xs, ints, 1);

                    if(via) {
                        calc_double_swing_by(osv0, p0, osv1, p1, dwb_dur, VENUS(), xs, ints, 0);
                        x10[d] = x6[2];
                        d++;
                    }


                    gettimeofday(&end, NULL);  // Record the ending time
                    elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
                    tot_time += elapsed_time;
                    printf("Elapsed time: %f seconds\n", elapsed_time);

                    double min = 1e9;
                    int mind;
                    for (int a = 0; a < u; a++) {
                        if (y[a] < min) {
                            min = y[a];
                            mind = a;
                        }
                    }

                    //a1[b] = x1[mind];
                    //a2[b] = x2[mind];
                    //a3[b] = x3[mind];
                    //a4[b] = x4[mind];
                    //a5[b] = x5[mind];
                    //a6[b] = y[mind];


                    u = 0;
                    c = 0;
                    g = 0;
                    b++;


                    if (1) {
                        printf("x10 = [");
                        for (int m = 0; m < d; m++) {
                            if (m != 0) printf(", ");
                            printf("%f", x10[m]);
                        }
                        printf("]\n");
                        printf("x11 = [");
                        for (int m = 0; m < d; m++) {
                            if (m != 0) printf(", ");
                            printf("%f", x11[m]);
                        }
                        printf("]\n");
                        printf("x12 = [");
                        for (int m = 0; m < d; m++) {
                            if (m != 0) printf(", ");
                            printf("%f", x12[m]);
                        }
                        printf("]\n");
                    }

                    if (0) {
                        printf("x1 = [");
                        for (int m = 0; m < b; m++) {
                            if (m != 0) printf(", ");
                            printf("%f", a1[m]);
                        }
                        printf("]\n");
                        printf("x2 = [");
                        for (int m = 0; m < b; m++) {
                            if (m != 0) printf(", ");
                            printf("%f", a2[m]);
                        }
                        printf("]\n");
                        printf("x3 = [");
                        for (int m = 0; m < b; m++) {
                            if (m != 0) printf(", ");
                            printf("%f", a3[m]);
                        }
                        printf("]\n");
                        printf("x4 = [");
                        for (int m = 0; m < b; m++) {
                            if (m != 0) printf(", ");
                            printf("%f", a4[m]);
                        }
                        printf("]\n");
                        printf("x5 = [");
                        for (int m = 0; m < b; m++) {
                            if (m != 0) printf(", ");
                            printf("%f", a5[m]);
                        }
                        printf("]\n");
                        printf("y = [");
                        for (int m = 0; m < b; m++) {
                            if (m != 0) printf(", ");
                            printf("%f", a6[m]);
                        }
                        printf("]\n");

                        //exit(0);
                    }
                }
            }
        }
    }


    printf("\n Total time: %f seconds \nb: %d\n", tot_time, b);

}

    exit(0);*/

    double ** porkchops = (double**) malloc((num_bodies-1) * sizeof(double*));

    for(int i = 0; i < num_bodies-1; i++) {
        int is_double_sb = bodies[i] == bodies[i+1];

        double min_dep, max_dep;
        if(i == 0) {
            min_dep = jd_min_dep;
            max_dep = jd_max_dep;
        } else {
            min_dep = get_min_arr_from_porkchop(porkchops[i - 1]) + is_double_sb*min_duration[i];
            max_dep = get_max_arr_from_porkchop(porkchops[i - 1]) + is_double_sb*max_duration[i];
        }
        i += is_double_sb;

        struct Porkchop_Properties pochopro = {
                min_dep,
                max_dep,
                dep_time_steps,
                arr_time_steps,
                min_duration[i],
                max_duration[i],
                ephems+i,
                bodies[i],
                bodies[i+1]
        };
        // 4 = dep_time, duration, dv1, dv2
        int data_length = 4;
        int all_data_size = (int) (data_length * (max_duration[i] - min_duration[i]) / (arr_time_steps / (24 * 60 * 60)) *
                                   (max_dep - min_dep) / (dep_time_steps / (24 * 60 * 60))) + 1;
        porkchops[i] = (double *) malloc(all_data_size * sizeof(double));

        gettimeofday(&start, NULL);  // Record the starting time
        if(i<num_bodies-2) create_porkchop(pochopro, circcirc, porkchops[i]);
        else               create_porkchop(pochopro, final_tt, porkchops[i]);
        gettimeofday(&end, NULL);  // Record the ending time
        elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0; // NOLINT(*-narrowing-conversions)
        printf("Elapsed time: %f seconds\n", elapsed_time);

        gettimeofday(&start, NULL);  // Record the starting time
        double min_max_dsb_duration[2];
        if(is_double_sb) {
            min_max_dsb_duration[0] = min_duration[i-1];
            min_max_dsb_duration[1] = max_duration[i-1];
        }
        decrease_porkchop_size(i, is_double_sb, porkchops, ephems, bodies, min_max_dsb_duration);
        gettimeofday(&end, NULL);  // Record the ending time
        elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0; // NOLINT(*-narrowing-conversions)
        printf("Elapsed time: %f seconds\n", elapsed_time);


        // if no viable trajectory was found, exit...
        if(porkchops[i][0] == 0) return;
    }

    double *jd_dates = (double*) malloc(num_bodies*sizeof(double));

    int min_total_dur = 0;
    for(int i = 0; i < num_bodies-1; i++) min_total_dur += min_duration[i];

    double *final_porkchop = (double *) malloc((int)(((max_total_dur-min_total_dur)*(jd_max_dep-jd_min_dep))*4+1)*sizeof(double));
    final_porkchop[0] = 0;

    gettimeofday(&start, NULL);  // Record the starting time
    double dep_time_info[] = {(jd_max_dep-jd_min_dep)*86400/dep_time_steps+1, dep_time_steps, jd_min_dep};
    generate_final_porkchop(porkchops, num_bodies, jd_dates, final_porkchop, dep_time_info);

    show_progress("Looking for cheapest transfer", 1, 1);
    printf("\n");

    gettimeofday(&end, NULL);  // Record the ending time
    elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0; // NOLINT(*-narrowing-conversions)
    printf("Elapsed time: %f seconds\n", elapsed_time);

    char data_fields[] = "dep_date,duration,dv_dep,dv_arr";
    write_csv(data_fields, final_porkchop);

    free(final_porkchop);
    for(int i = 0; i < num_bodies-1; i++) free(porkchops[i]);
    free(porkchops);

    // 3 states per transfer (departure, arrival and final state)
    // 1 additional for initial start; 7 variables per state
    double transfer_data[((num_bodies-1)*3+1) * 7 + 1];
    transfer_data[0] = 0;

    struct OSV init_s = osv_from_ephem(ephems[0], jd_dates[0], SUN());

    transfer_data[1] = jd_dates[0];
    transfer_data[2] = init_s.r.x;
    transfer_data[3] = init_s.r.y;
    transfer_data[4] = init_s.r.z;
    transfer_data[5] = init_s.v.x;
    transfer_data[6] = init_s.v.y;
    transfer_data[7] = init_s.v.z;
    transfer_data[0] += 7;

    double dep_dv;

    for(int i = 0; i < num_bodies-1; i++) {
        struct OSV s0 = osv_from_ephem(ephems[i], jd_dates[i], SUN());
        struct OSV s1 = osv_from_ephem(ephems[i+1], jd_dates[i+1], SUN());

        double data[3];
        struct Transfer transfer;

        if(i < num_bodies-2) {
            transfer = calc_transfer(circcirc, bodies[i], bodies[i + 1], s0.r, s0.v, s1.r, s1.v,
                                     (jd_dates[i + 1] - jd_dates[i]) * (24 * 60 * 60), data);
        } else {
            transfer = calc_transfer(final_tt, bodies[i], bodies[i + 1], s0.r, s0.v, s1.r, s1.v,
                                     (jd_dates[i + 1] - jd_dates[i]) * (24 * 60 * 60), data);
        }

        if(i == 0) {
            printf("| T+    0 days (");
            print_date(convert_JD_date(jd_dates[i]),0);
            char *spacing = "                 ";
            printf(")%s- %s (%.2f m/s)\n", spacing, bodies[i]->name, data[1]);
            dep_dv = data[1];
        }
        printf("| T+% 5d days (", (int)(jd_dates[i+1]-jd_dates[0]));
        print_date(convert_JD_date(jd_dates[i+1]),0);
        printf(") - |% 5d days | - %s", (int)data[0], bodies[i+1]->name);
        if(i == num_bodies-2) printf(" (%.2f m/s)\nTotal: %.2f m/s\n", data[2], dep_dv+data[2]);
        else printf("\n");

        struct OSV osvs[3];
        osvs[0].r = transfer.r0;
        osvs[0].v = transfer.v0;
        osvs[1].r = transfer.r1;
        osvs[1].v = transfer.v1;
        osvs[2] = s1;

        for (int j = 0; j < 3; j++) {
            transfer_data[(int)transfer_data[0] + 1] = j == 0 ? jd_dates[i] : jd_dates[i+1];
            transfer_data[(int)transfer_data[0] + 2] = osvs[j].r.x;
            transfer_data[(int)transfer_data[0] + 3] = osvs[j].r.y;
            transfer_data[(int)transfer_data[0] + 4] = osvs[j].r.z;
            transfer_data[(int)transfer_data[0] + 5] = osvs[j].v.x;
            transfer_data[(int)transfer_data[0] + 6] = osvs[j].v.y;
            transfer_data[(int)transfer_data[0] + 7] = osvs[j].v.z;
            transfer_data[0] += 7;
        }
    }

    for(int i = 0; i < num_bodies; i++) {
        int ephem_double = 0;
        for(int j = 0; j < i; j++) {
            if(bodies[i] == bodies[j]) {
                ephems[i] = ephems[j];
                ephem_double = 1;
                break;
            }
        }
        if(ephem_double) continue;
        free(ephems[i]);
    }
    free(ephems);
    free(jd_dates);

    char pcsv;
    printf("Write transfer data to .csv (y/Y=yes)? ");
    scanf(" %c", &pcsv);
    if (pcsv == 'y' || pcsv == 'Y') {
        char transfer_data_fields[] = "JD,X,Y,Z,VX,VY,VZ";
        write_csv(transfer_data_fields, transfer_data);
    }
}
