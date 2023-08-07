#include "lp_parameters.h"
#include <stdio.h>
#include <math.h>

struct Lp_Params best_lp_params = {.a1 = 0.00003, .a2 = 0.000004, .b2 = 40, .h = 0};

struct Launch_Results best_results;
double payload;
double min_pe = 170e3;

void calc_params_a1(struct LV lv, struct Lp_Params lp_params, double step_size_a, double step_size_b);
void calc_params_a2(struct LV lv, struct Lp_Params lp_params, double step_size_a, double step_size_b);
void calc_params_b2(struct LV lv, struct Lp_Params lp_params, double step_size_b);


void calc_lp_param_adjustments(struct LV lv, double payload_mass, struct Lp_Params *return_params) {
    double step_size_a = 1e-6;
    double step_size_b = 1;
    payload = payload_mass;

    best_lp_params.h = log(best_lp_params.b2/90) / (best_lp_params.a2-best_lp_params.a1);

    best_results = calculate_launch(lv, payload_mass, best_lp_params, 1);

    while(best_results.pe < min_pe) {
        best_lp_params.a1 -= step_size_a;
        best_results = calculate_launch(lv, payload_mass, best_lp_params, 1);
    }
    struct Launch_Results last_results;

    do {
        struct Lp_Params lp_params = best_lp_params;
        last_results = best_results;
        calc_params_a1(lv, lp_params, step_size_a, step_size_b);
    } while(best_results.dv < last_results.dv);

    return_params->a1 = best_lp_params.a1;
    return_params->a2 = best_lp_params.a2;
    return_params->b2 = best_lp_params.b2;
    return_params->h = best_lp_params.h;
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
    lp_params.h = log(lp_params.b2/90) / (lp_params.a2-lp_params.a1);
    new_results = calculate_launch(lv, payload, lp_params, 1);
    if(new_results.pe > min_pe && new_results.dv < best_results.dv) {
        best_results = new_results;
        best_lp_params = lp_params;
        printf("------- a1: %f, a2: %f, b2: %g, h: %.02f, dv: %.02f, pe: %.02f ------- \n",
               best_lp_params.a1, best_lp_params.a2, best_lp_params.b2, best_lp_params.h, best_results.dv, best_results.pe/1000);
    }

    lp_params.b2 = best_lp_params.b2 + step_size_b;
    lp_params.h = log(lp_params.b2/90) / (lp_params.a2-lp_params.a1);
    new_results = calculate_launch(lv, payload, lp_params, 1);
    if(new_results.pe > min_pe && new_results.dv < best_results.dv) {
        best_results = new_results;
        best_lp_params = lp_params;
        printf("------- a1: %f, a2: %f, b2: %g, h: %.02f, dv: %.02f, pe: %.02f ------- \n",
               best_lp_params.a1, best_lp_params.a2, best_lp_params.b2, best_lp_params.h, best_results.dv, best_results.pe/1000);
    }

    lp_params.b2 = best_lp_params.b2 - step_size_b;
    lp_params.h = log(lp_params.b2/90) / (lp_params.a2-lp_params.a1);
    new_results = calculate_launch(lv, payload, lp_params, 1);
    if(new_results.pe > min_pe && new_results.dv < best_results.dv) {
        best_results = new_results;
        best_lp_params = lp_params;
        printf("------- a1: %f, a2: %f, b2: %g, h: %.02f, dv: %.02f, pe: %.02f ------- \n",
               best_lp_params.a1, best_lp_params.a2, best_lp_params.b2, best_lp_params.h, best_results.dv, best_results.pe/1000);
    }
}