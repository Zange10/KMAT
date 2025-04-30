#include "lp_parameters.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "launch_sim.h"
#include "tools/thread_pool.h"

#define MAX_NUM_LP_PARAMS 5
const double MIN_PE = 160e3;



struct ParamAnalysisData {
	double min_values[MAX_NUM_LP_PARAMS];
	double step_size[MAX_NUM_LP_PARAMS];
	int steps[MAX_NUM_LP_PARAMS];
};

struct ParamLaunchResults {
	double lp_params[MAX_NUM_LP_PARAMS];
	double pe;
	double dv;
	double rem_dv;
};

struct ParamThreadArgs {
	struct LV lv;
	double payload_mass;
	struct ParamLaunchResults **pta_results;
	struct ParamAnalysisData pad;
	int only_check_for_orbit;
};


void *lp_param_fpa4(void *thread_args) {
	int num_params = 3;

	int index = get_incr_thread_counter(0);
	// increase finished counter to 1 (first finished should reflect a num of finished of 1)
	if(index == 0) get_incr_thread_counter(1);

	struct ParamThreadArgs *pta = (struct ParamThreadArgs*) thread_args;
	struct ParamAnalysisData pad = pta->pad;
	struct LV lv = pta->lv;
	double payload_mass = pta->payload_mass;
	while(index < pad.steps[0]) {
		lv.lp_params[0] = pad.min_values[0] + index	*pad.step_size[0];
		struct ParamLaunchResults *results = pta->pta_results[index];
		results->pe = 0; results->dv = 1e9; results->rem_dv = 0;

		for(int j = 0; j < pad.steps[1]; j++) {
			lv.lp_params[1] = pad.min_values[1] + j*pad.step_size[1];
			for(int k = 0; k < pad.steps[2]; k++) {
				lv.lp_params[2] = pad.min_values[2] + k*pad.step_size[2];
				double h_trans = log(lv.lp_params[2]/90) / (lv.lp_params[1]-lv.lp_params[0]);
				if(h_trans < 0) continue;
				
				struct Launch_Results lr = run_launch_simulation(lv, payload_mass, deg2rad(28.6), deg2rad(0), 0.01, 0, 0, 0);

				if(lr.pe > MIN_PE && lr.dv < results->dv) {
					for(int param_id = 0; param_id < num_params; param_id++)
						results->lp_params[param_id] = lv.lp_params[param_id];
					results->pe = lr.pe;
					results->dv = lr.dv;
					results->rem_dv = lr.rem_dv;
					if(pta->only_check_for_orbit) get_incr_thread_counter(2);
				}
				if(pta->only_check_for_orbit) {
					if(get_thread_counter(2) > 0) break;
				}
			}
		}
		double progress = get_incr_thread_counter(1);
		show_progress("Fixed Payload Analysis: ", progress, pad.steps[0]);
		index = get_incr_thread_counter(0);
	}

	return NULL;
}



int lp_param_fixed_payload_analysis4(struct LV lv, double payload_mass, struct ParamLaunchResults *best_results, int only_check_for_orbit) {
	lv.lp_id = 4;
	int num_params = 3;

	struct ParamAnalysisData pad;

	struct ParamLaunchResults results = {.dv = 1e9};

	for(int i = 0; i < 3; i++) {

		pad.step_size[0] = i==0 ? 5e-6 : i==1 ? 3e-6 : 1e-6;
		pad.step_size[1] = i==0 ? 5e-6 : i==1 ? 3e-6 : 1e-6;
		pad.step_size[2] = i==0 ?    5 : i==1 ?    3 :    1;

		pad.steps[0] = i==0 ? 20 : 11;
		pad.steps[1] = i==0 ? 10 : 11;
		pad.steps[2] = i==0 ? 20 : 11;

		for(int j = 0; j < num_params; j++) {
			pad.min_values[j] = i==0 ? 0 : results.lp_params[j] - pad.step_size[j] * (pad.steps[j] - 1) / 2;
			if(pad.min_values[j] < 0) pad.min_values[j] = 0;
		}

		struct ParamLaunchResults **pta_results = (struct ParamLaunchResults**) malloc(pad.steps[0] * sizeof(struct ParamLaunchResults*));
		for(int j = 0; j < pad.steps[0]; j++) {
			pta_results[j] = (struct ParamLaunchResults *) malloc(sizeof(struct ParamLaunchResults));
			pta_results[j]->dv = 1e9;
		}

		struct ParamThreadArgs pta = {lv,payload_mass, pta_results, pad, only_check_for_orbit};

		show_progress("Fixed Payload Analysis: ", 0, 1);
		struct Thread_Pool thread_pool = use_thread_pool32(lp_param_fpa4, &pta);
		join_thread_pool(thread_pool);

		for(int j = 0; j < pad.steps[0]; j++) {
			if(pta_results[j]->pe > MIN_PE && pta_results[j]->dv < results.dv) {
				for(int k = 0; k < num_params; k++) results.lp_params[k] = pta_results[j]->lp_params[k];
				results.dv = pta_results[j]->dv;
				results.pe = pta_results[j]->pe;
				results.rem_dv = pta_results[j]->rem_dv;
			}
			free(pta_results[j]);
		}
		free(pta_results);
		show_progress("Fixed Payload Analysis: ", 1, 1);
		printf("\n");
		if(only_check_for_orbit || !(results.dv < 1e9)) break;
	}

	printf("%f %f %f %f %f %f\n",
		   results.lp_params[0], results.lp_params[1], results.lp_params[2], results.pe, results.dv, results.rem_dv);

	if(best_results != NULL) {
		for(int k = 0; k < num_params; k++) best_results->lp_params[k] = results.lp_params[k];
		best_results->dv = results.dv;
		best_results->pe = results.pe;
		best_results->rem_dv = results.rem_dv;
	}

	return results.dv < 1e9;
}



double calc_highest_payload_mass(struct LV lv) {
	double payload_mass = 100;
	struct ParamLaunchResults launch_params;
	double highest_payload_mass = 0;

	do {
		printf("Checking Payload mass of %f t\n", payload_mass/1000);
		lp_param_fixed_payload_analysis4(lv, payload_mass, &launch_params, 1);
		if(launch_params.dv < 1e9) highest_payload_mass = payload_mass;
		payload_mass *= 10;
	} while(launch_params.dv < 1e9);

	if(highest_payload_mass == 0) return 0;

	double step_size = highest_payload_mass;

	for(int i = 0; i < 3; i++) {
		for(int j = 1; j < 10; j++) {
			payload_mass = highest_payload_mass + step_size;
			printf("Checking Payload mass of %f t\n", payload_mass/1000);
			lp_param_fixed_payload_analysis4(lv, payload_mass, &launch_params, 1);
			if(launch_params.dv < 1e9) highest_payload_mass = payload_mass;
			else break;
		}
		step_size /= 10;
	}

	return highest_payload_mass;
}


void calc_payload_curve(struct LV lv) {
	double highest_payload_mass = calc_highest_payload_mass(lv);

	int data_points = 10;

	printf("Highest Payload mass: %.3f t\n", highest_payload_mass/1000);
	struct ParamLaunchResults curve_data[data_points];
	double payload_mass[data_points];

	for(int i = 0; i < data_points; i++) {
		payload_mass[i] = (i < data_points-1) ? highest_payload_mass * pow(0.75, i) : 0;
		printf("Checking Payload mass of %f t\n", payload_mass[i] / 1000);
		lp_param_fixed_payload_analysis4(lv, payload_mass[i], &curve_data[i], 0);
	}
	printf("\n--------\n");

	for(int i = 0; i < data_points; i++) {
		printf("Payload mass: %12.3f t; (%10.6f | %10.6f | %10.6f | %10.6f | %10.6f); Perigee: %6.2f km; Spent dv: %5.0f m/s; Rem dv: %5.0f m/s\n",
			   payload_mass[i]/1000,
			   curve_data[i].lp_params[0], curve_data[i].lp_params[1], curve_data[i].lp_params[2],
			   curve_data[i].lp_params[3], curve_data[i].lp_params[4],
			   curve_data[i].pe/1000, curve_data[i].dv, curve_data[i].rem_dv);
	}
	printf("--------\n");
}

void calc_payload_curve4(struct LV lv, double *payload_mass, double *a1, double *a2, double *b2, double *dv, int data_points) {
	double highest_payload_mass = calc_highest_payload_mass(lv);

	printf("Highest Payload mass: %.3f t\n", highest_payload_mass/1000);
	struct ParamLaunchResults *curve_data = (struct ParamLaunchResults*) malloc(sizeof(struct ParamLaunchResults)*data_points);

	for(int i = 0; i < data_points; i++) {
		payload_mass[i] = (i < data_points-1) ? highest_payload_mass * pow(0.75, i) : 0;
		printf("Checking Payload mass of %f t\n", payload_mass[i] / 1000);
		lp_param_fixed_payload_analysis4(lv, payload_mass[i], &curve_data[i], 0);
	}
	printf("\n--------\n");

	for(int i = 0; i < data_points; i++) {
//		printf("Payload mass: %12.3f t; (%10.6f | %10.6f | %10.6f | %10.6f | %10.6f); Perigee: %6.2f km; Spent dv: %5.0f m/s; Rem dv: %5.0f m/s\n",
//			   payload_mass[i]/1000,
//			   curve_data[i].lp_params[0], curve_data[i].lp_params[1], curve_data[i].lp_params[2],
//			   curve_data[i].lp_params[3], curve_data[i].lp_params[4],
//			   curve_data[i].pe/1000, curve_data[i].dv, curve_data[i].rem_dv);
		payload_mass[i] = payload_mass[i]/1000;
		a1[i] = curve_data[i].lp_params[0];
		a2[i] = curve_data[i].lp_params[1];
		b2[i] = curve_data[i].lp_params[2];
		dv[i] = curve_data[i].rem_dv;
	}
//	printf("--------\n");

	free(curve_data);
}

double calc_highest_payload_mass_with_set_lp_params(struct LV lv) {
	double payload_mass = 100;
	double highest_payload_mass = 0;
	struct Launch_Results lr;

	do {
		printf("Checking Payload mass of %f t\n", payload_mass/1000);
		lr = run_launch_simulation(lv, payload_mass, deg2rad(28.6), deg2rad(0), 0.005, 0, 0, 0);
		if(lr.pe > MIN_PE) highest_payload_mass = payload_mass;
		payload_mass *= 10;
	} while(lr.pe > MIN_PE);

	if(highest_payload_mass == 0) return 0;

	double step_size = highest_payload_mass;

	for(int i = 0; i < 3; i++) {
		for(int j = 1; j < 10; j++) {
			payload_mass = highest_payload_mass + step_size;
			printf("Checking Payload mass of %f t\n", payload_mass/1000);
			lr = run_launch_simulation(lv, payload_mass, deg2rad(28.6), deg2rad(0), 0.005, 0, 0, 0);
			if(lr.pe > MIN_PE) highest_payload_mass = payload_mass;
			else break;
		}
		step_size /= 10;
	}

	return highest_payload_mass;
}

void calc_payload_curve_with_set_lp_params(struct LV lv) {
	double highest_payload_mass = calc_highest_payload_mass_with_set_lp_params(lv);

	int data_points = 10;

	printf("Highest Payload mass: %.3f t\n", highest_payload_mass/1000);
	struct ParamLaunchResults curve_data[data_points];
	double payload_mass[data_points];
	struct Launch_Results lr;

	for(int i = 0; i < data_points; i++) {
		payload_mass[i] = (i < data_points-1) ? highest_payload_mass * pow(0.75, i) : 0;
		printf("Checking Payload mass of %f t\n", payload_mass[i] / 1000);
		lr = run_launch_simulation(lv, payload_mass[i], deg2rad(28.6), deg2rad(0), 0.005, 0, 0, 0);
		for(int k = 0; k < MAX_NUM_LP_PARAMS; k++) curve_data[i].lp_params[k] = lv.lp_params[k];
		curve_data[i].dv = lr.dv;
		curve_data[i].pe = lr.pe;
		curve_data[i].rem_dv = lr.rem_dv;
	}
	printf("\n--------\n");

	for(int i = 0; i < data_points; i++) {
		printf("Payload mass: %12.3f t; (%10.6f | %10.6f | %10.6f | %10.6f | %10.6f); Perigee: %6.2f km; Spent dv: %5.0f m/s; Rem dv: %5.0f m/s\n",
			   payload_mass[i]/1000,
			   curve_data[i].lp_params[0], curve_data[i].lp_params[1], curve_data[i].lp_params[2],
			   curve_data[i].lp_params[3], curve_data[i].lp_params[4],
			   curve_data[i].pe/1000, curve_data[i].dv, curve_data[i].rem_dv);
	}
	printf("--------\n");
}
