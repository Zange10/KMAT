#include "lp_parameters.h"
#include <stdio.h>

struct Lp_Params best_lp_params = {.a1 = 0.00005, .a2 = 0.000005, .b2 = 80};
struct Launch_Results best_results;
double payload;

void calc_params_a1(struct LV lv, struct Lp_Params lp_params, double step_size_a, double step_size_b);
void calc_params_a2(struct LV lv, struct Lp_Params lp_params, double step_size_a, double step_size_b);
void calc_params_b2(struct LV lv, struct Lp_Params lp_params, double step_size_b);


void calc_lp_param_adjustments(struct LV lv, double payload_mass, struct Lp_Params *return_params) {
    double step_size_a = 1e-6;
    double step_size_b = 1;
    payload = payload_mass;

    best_results = calculate_launch(lv, payload_mass, best_lp_params, 1);

    while(best_results.pe < 175e3) {
        best_lp_params.a1 -= step_size_a;
        best_results = calculate_launch(lv, payload_mass, best_lp_params, 1);
    }
    struct Launch_Results last_results;

    do {
        struct Lp_Params lp_params = best_lp_params;
        last_results = best_results;
        calc_params_a1(lv, lp_params, step_size_a, step_size_b);
        printf("------- a1: %f, a2: %f, b2: %g ------- \n", best_lp_params.a1, best_lp_params.a2, best_lp_params.b2);
    } while(best_results.dv < last_results.dv);

    return_params->a1 = best_lp_params.a1;
    return_params->a2 = best_lp_params.a2;
    return_params->b2 = best_lp_params.b2;
}

void calc_params_a1(struct LV lv, struct Lp_Params lp_params, double step_size_a, double step_size_b) {
    lp_params.a1 = best_lp_params.a1;
    calc_params_a2(lv, lp_params, step_size_a, step_size_b);
    lp_params.a1 = best_lp_params.a1 + step_size_a;
    calc_params_a2(lv, lp_params, step_size_a, step_size_b);
    lp_params.a1 = best_lp_params.a1 - step_size_a;
    calc_params_a2(lv, lp_params, step_size_a, step_size_b);
}

void calc_params_a2(struct LV lv, struct Lp_Params lp_params, double step_size_a, double step_size_b) {
    lp_params.a2 = best_lp_params.a2;
    calc_params_b2(lv, lp_params, step_size_b);
    lp_params.a2 = best_lp_params.a2 + step_size_a;
    calc_params_b2(lv, lp_params, step_size_b);
    lp_params.a2 = best_lp_params.a2 - step_size_a;
    calc_params_b2(lv, lp_params, step_size_b);
}

void calc_params_b2(struct LV lv, struct Lp_Params lp_params, double step_size_b) {
    struct Launch_Results new_results;
    lp_params.b2 = best_lp_params.b2;
    new_results = calculate_launch(lv, payload, lp_params, 1);
    if(new_results.pe > 175e3 && new_results.dv < best_results.dv) {
        best_results = new_results;
        best_lp_params = lp_params;
    }

    lp_params.b2 = best_lp_params.b2 + step_size_b;
    new_results = calculate_launch(lv, payload, lp_params, 1);
    if(new_results.pe > 175e3 && new_results.dv < best_results.dv) {
        best_results = new_results;
        best_lp_params = lp_params;
    }

    lp_params.b2 = best_lp_params.b2 - step_size_b;
    new_results = calculate_launch(lv, payload, lp_params, 1);
    if(new_results.pe > 175e3 && new_results.dv < best_results.dv) {
        best_results = new_results;
        best_lp_params = lp_params;
    }
}