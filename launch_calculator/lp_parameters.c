#include "lp_parameters.h"
#include <stdio.h>
#include <math.h>

struct Lp_Params best_lp_params = {.a1 = 0.00003, .a2 = 0.000004, .b2 = 40, .h = 0};

struct Launch_Results best_results;
double min_pe = 170e3;

void lp_param_fixed_payload_analysis(struct LV lv, double payload_mass, struct Lp_Params *return_params) {
    double step_size_a = 2e-6;
    double step_size_b = 2;

    double min_a1 = 20e-6, max_a1 = 55e-6;
    double size_a1 = (max_a1-min_a1)/step_size_a;

    double min_a2 = 5e-6, max_a2 = 40e-6;
    double size_a2 = (max_a2-min_a2)/step_size_a;

    double min_b = 30, max_b = 70;
    double size_b = (max_b-min_b)/step_size_b;

    struct Lp_Params lp_params;
    struct Launch_Results results;

    double all_results[(int)(size_a1*size_a2*size_b)*6+1];
    all_results[0] = (int)(size_a1*size_a2*size_b)*6+1;
    best_results.dv = 10000;

    printf("Analyzing %g launches:\n\n", (all_results[0]-1)/6);

    for(int i = 0; i < (int)size_a1; i++) {
        lp_params.a1 = i*step_size_a + min_a1;
        for(int j = 0; j < (int)size_a2; j++) {
            printf("%.02f%%\n", (i*size_a2+j)/(size_a1*size_a2)*100);
            lp_params.a2 = j*step_size_a + min_a2;
            for(int k = 0; k < (int)size_b; k++) {
                lp_params.b2 = k*step_size_b+min_b;
                lp_params.h  = log(lp_params.b2/90) / (lp_params.a2-lp_params.a1);
                results = calculate_launch(lv, payload_mass, lp_params, 1);
                if(results.pe > min_pe && results.dv < best_results.dv) {
                    best_results = results;
                    best_lp_params = lp_params;
                }
                int index = (int)(i*size_a2*size_b + j*size_b + k)*6;
                all_results[index+1] = lp_params.a1;
                all_results[index+2] = lp_params.a2;
                all_results[index+3] = lp_params.b2;
                all_results[index+4] = lp_params.h;
                all_results[index+5] = results.dv;
                all_results[index+6] = results.pe;
            }
        }
    }
    char flight_data_fields[] = "a1,a2,b2,h,dv,pe";
    write_csv(flight_data_fields, all_results);

    printf("------- a1: %f, a2: %f, b2: %g, h: %.02f, dv: %.02f, pe: %.02f ------- \n",
           best_lp_params.a1, best_lp_params.a2, best_lp_params.b2, best_lp_params.h, best_results.dv, best_results.pe/1000);


    return_params->a1 = best_lp_params.a1;
    return_params->a2 = best_lp_params.a2;
    return_params->b2 = best_lp_params.b2;
    return_params->h = best_lp_params.h;
    return;
}

struct Analysis_Params {
    double step_size_a, step_size_b;
    double min_a1, max_a1;
    double min_a2, max_a2;

    double min_b, max_b;
};

void lp_param_analysis(struct LV lv, double payload_mass, struct Analysis_Params ap, int orbit_test_only) {
    double step_size_a = ap.step_size_a;
    double step_size_b = ap.step_size_b;

    double min_a1 = ap.min_a1, max_a1 = ap.max_a1;
    double size_a1 = (max_a1-min_a1)/step_size_a;

    double min_a2 = ap.min_a2, max_a2 = ap.max_a2;
    double size_a2 = (max_a2-min_a2)/step_size_a;

    double min_b = ap.min_b, max_b = ap.max_b;
    double size_b = (max_b-min_b)/step_size_b;

    struct Lp_Params lp_params;
    struct Launch_Results results;

    best_results.dv = 10000;

    printf("analyzing %g launches:\n\n", size_a1*size_a2*size_b);

    for(int i = 0; i < (int)size_a1; i++) {
        lp_params.a1 = i*step_size_a + min_a1;
        for(int j = 0; j < (int)size_a2; j++) {
            printf("%.02f%%\n", (i*size_a2+j)/(size_a1*size_a2)*100);
            lp_params.a2 = j*step_size_a + min_a2;
            for(int k = 0; k < (int)size_b; k++) {
                lp_params.b2 = k*step_size_b+min_b;
                lp_params.h  = log(lp_params.b2/90) / (lp_params.a2-lp_params.a1);
                results = calculate_launch(lv, payload_mass, lp_params, 1);
                if(results.pe > min_pe && results.dv < best_results.dv) {
                    best_results = results;
                    best_lp_params = lp_params;
                    if(orbit_test_only) return;
                }
            }
        }
    }

    printf("------- payload: %g, a1: %f, a2: %f, b2: %g, h: %.02f, dv: %.02f, pe: %.02f ------- \n",
           payload_mass, best_lp_params.a1, best_lp_params.a2, best_lp_params.b2, best_lp_params.h, best_results.dv, best_results.pe/1000);
}

double calc_highest_payload_mass(struct LV lv) {
    struct Analysis_Params ap = {
            .min_a1 = 28e-6,
            .max_a1 = 42e-6,
            .min_a2 =  0e-6,
            .max_a2 = 20e-6,
            .min_b  = 20,
            .max_b  = 70,
            .step_size_a = 2e-6,
            .step_size_b = 2
    };

    lp_param_analysis(lv, 0, ap,0);

    ap.step_size_a = 2e-6;
    ap.step_size_b = 2;

    ap.max_a2 = best_lp_params.a2+ap.step_size_a;

    double payload_mass = 0;
    int max_i = 5;
    while(best_results.dv < 10000) {
        for(int i = max_i; i > 0; i--) {
            printf("\nCalculating %gkg payload\n", payload_mass + pow(10,i));
            lp_param_analysis(lv, payload_mass + pow(10,i), ap, 1);
            if(best_results.dv < 10000) {
                payload_mass += pow(10,i);
                break;
            } else {
                max_i--;
            }
        }
    }

    return payload_mass;
}

void lp_param_mass_analysis(struct LV lv, double payload_min, double payload_max) {
    struct Analysis_Params ap = {
            .min_a1 = 28e-6,
            .max_a1 = 42e-6,
            .min_a2 =  0e-6,
            .max_a2 = 20e-6,
            .min_b  = 20,
            .max_b  = 70,
            .step_size_a = 2e-6,
            .step_size_b = 2
    };
    lp_param_analysis(lv, 0, ap,0);
    struct Lp_Params payload_min_lp = best_lp_params;

    lp_param_analysis(lv, payload_max, ap, 0);
    double payload_step = (payload_max-payload_min)/10;

    if(best_lp_params.a1 < payload_min_lp.a1) {
        ap.min_a1 = best_lp_params.a1 - ap.step_size_a;
        ap.max_a1 = payload_min_lp.a1 + ap.step_size_a;
    } else {
        ap.min_a1 = payload_min_lp.a1 - ap.step_size_a;
        ap.max_a1 = best_lp_params.a1 + ap.step_size_a;
    }
    if(best_lp_params.a2 < payload_min_lp.a2) {
        ap.min_a2 = best_lp_params.a2 - ap.step_size_a;
        ap.max_a2 = payload_min_lp.a2 + ap.step_size_a;
    } else {
        ap.min_a2 = payload_min_lp.a2 - ap.step_size_a;
        ap.max_a2 = best_lp_params.a2 + ap.step_size_a;
    }
    if(best_lp_params.b2 < payload_min_lp.b2) {
        ap.min_b = best_lp_params.b2 - ap.step_size_b;
        ap.max_b = payload_min_lp.b2 + ap.step_size_b;
    } else {
        ap.min_b = payload_min_lp.b2 - ap.step_size_b;
        ap.max_b = best_lp_params.b2 + ap.step_size_b;
    }

    ap.step_size_a = 0.5e-6;
    ap.step_size_b = 0.5;

    double all_results[(int)((payload_max-payload_min)/payload_step+1)*7+1];
    all_results[0] = ((payload_max-payload_min)/payload_step+1)*7+1;

    double payload_mass;

    for(int i = 0; i < (int)((payload_max-payload_min)/payload_step+1); i++) {
        payload_mass = i*payload_step + payload_min;

        lp_param_analysis(lv, payload_mass, ap,0);

        int index = (int)(i*7+1);
        all_results[index+0] = payload_mass;
        all_results[index+1] = best_lp_params.a1;
        all_results[index+2] = best_lp_params.a2;
        all_results[index+3] = best_lp_params.b2;
        all_results[index+4] = best_lp_params.h;
        all_results[index+5] = best_results.dv;
        all_results[index+6] = best_results.pe;
    }

    char flight_data_fields[] = "pm,a1,a2,b2,h,dv,pe";
    write_csv(flight_data_fields, all_results);
}