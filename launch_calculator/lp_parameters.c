#include "lp_parameters.h"
#include <stdio.h>
#include <stdlib.h>
#include "launch_sim.h"
#include "tools/thread_pool.h"

double min_pe = 180e3;

struct ParamAnalysisData {
	double *min_values;
	double *step_size;
	int *steps;
};

struct ParamThreadArgs {
	struct LV lv;
	double payload_mass;
	double **pta_min_params;
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
		double min = 1e9;
		double *min_params = pta->pta_min_params[index];
		min_params[num_params] = 0; min_params[num_params+1] = 1e9; min_params[num_params+2] = 0;
		for(int j = 0; j < pad.steps[1]; j++) {
			lv.lp_params[1] = pad.min_values[1] + j*pad.step_size[1];
			for(int k = 0; k < pad.steps[2]; k++) {
				lv.lp_params[2] = pad.min_values[2] + k*pad.step_size[2];

				struct Launch_Results lr = run_launch_simulation(lv, payload_mass, 0.01, 0);

				if(lr.pe > min_pe && lr.dv < min) {
					for(int param_id = 0; param_id < num_params; param_id++)
						min_params[param_id] = lv.lp_params[param_id];
					min_params[num_params] = lr.pe;
					min_params[num_params+1] = lr.dv;
					min_params[num_params+2] = lr.rem_dv;
					min = lr.dv;
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



int lp_param_fixed_payload_analysis4(struct LV lv, double payload_mass, double *best_params, int only_check_for_orbit) {
	lv.lp_id = 4;
	int num_params = 3;

	struct ParamAnalysisData pad;
	pad.min_values = (double*) malloc(sizeof(double) * num_params);
	pad.step_size = (double*) malloc(sizeof(double) * num_params);
	pad.steps = (int*) malloc(sizeof(int) * num_params);
	double min_params[num_params+3];
	min_params[num_params] = 0;
	min_params[num_params+1] = 1e9;
	min_params[num_params+2] = 0;

	for(int i = 0; i < 3; i++) {

		pad.step_size[0] = i==0 ? 5e-6 : i==1 ? 3e-6 : 1e-6;
		pad.step_size[1] = i==0 ? 5e-6 : i==1 ? 3e-6 : 1e-6;
		pad.step_size[2] = i==0 ?    5 : i==1 ?    3 :    1;

		pad.steps[0] = i==0 ? 20 : 11;
		pad.steps[1] = i==0 ? 10 : 11;
		pad.steps[2] = i==0 ? 20 : 11;

		for(int j = 0; j < num_params; j++) {
			pad.min_values[j] = i==0 ? 0 : min_params[j] - pad.step_size[j] * (pad.steps[j]-1)/2;
			if(pad.min_values[j] < 0) pad.min_values[j] = 0;
		}

		double **pta_min_params = (double**) malloc(pad.steps[0] * sizeof(double*));
		for(int j = 0; j < pad.steps[0]; j++) pta_min_params[j] = (double*) malloc((num_params+3) * sizeof(double));

		struct ParamThreadArgs pta = {lv,payload_mass, pta_min_params, pad, only_check_for_orbit};

		show_progress("Fixed Payload Analysis: ", 0, 1);
		struct Thread_Pool thread_pool = use_thread_pool64(lp_param_fpa4, &pta);
		join_thread_pool(thread_pool);

		double min_dv = 1e9;
		for(int j = 0; j < pad.steps[0]; j++) {
			if(pta_min_params[j][num_params] > min_pe && pta_min_params[j][num_params+1] < min_dv) {
				for(int k = 0; k < num_params+3; k++) min_params[k] = pta_min_params[j][k];
				min_dv = min_params[num_params];
			}
			free(pta_min_params[j]);
		}
		free(pta_min_params);
		show_progress("Fixed Payload Analysis: ", 1, 1);
		printf("\n");
		if(only_check_for_orbit) break;
	}

	printf("%f %f %f %f %f %f\n", min_params[0], min_params[1], min_params[2], min_params[3], min_params[4], min_params[5]);

	if(best_params != NULL) {
		for(int i = 0; i < num_params + 2; i++) {
			best_params[i] = min_params[i];
		}
	}

	free(pad.min_values);
	free(pad.step_size);
	free(pad.steps);

	return min_params[num_params+1] < 1e9;
}



void calc_highest_payload_mass(struct LV lv) {
	int num_params = 3;
	double payload_mass = 100;
	double launch_params[5];
	double highest_payload_mass = 0;

	do {
		printf("Checking Payload mass of %f t\n", payload_mass/1000);
		lp_param_fixed_payload_analysis4(lv, payload_mass, launch_params, 1);
		if(launch_params[num_params+1] < 1e9) highest_payload_mass = payload_mass;
		payload_mass *= 10;
	} while(launch_params[num_params+1] < 1e9);

	if(highest_payload_mass == 0) {
		printf("No orbit with payload possible\n");
		return;
	}

	double step_size = highest_payload_mass;

	for(int i = 0; i < 3; i++) {
		for(int j = 1; j < 10; j++) {
			payload_mass = highest_payload_mass + step_size;
			printf("Checking Payload mass of %f t\n", payload_mass/1000);
			lp_param_fixed_payload_analysis4(lv, payload_mass, launch_params, 1);
			if(launch_params[num_params+1] < 1e9) highest_payload_mass = payload_mass;
			else break;
		}
		step_size /= 10;
	}

	printf("Highest Payload mass: %f t\n", payload_mass/1000);
}
