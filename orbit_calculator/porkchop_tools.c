#include "porkchop_tools.h"
#include "tool_funcs.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void create_porkchop(struct Porkchop_Properties pochopro, enum Transfer_Type tt, double *all_data) {
    int min_duration = pochopro.min_duration;         // [days]
    int max_duration = pochopro.max_duration;         // [days]
    double dep_time_steps = pochopro.dep_time_steps; // [seconds]
    double arr_time_steps = pochopro.arr_time_steps; // [seconds]

    double jd_min_dep = pochopro.jd_min_dep;
    double jd_max_dep = pochopro.jd_max_dep;

    struct Ephem *dep_ephem = pochopro.ephems[0];
    struct Ephem *arr_ephem = pochopro.ephems[1];

    // 4 = dep_time, duration, dv1, dv2
    int data_length = 4;
    int all_data_size = (int)(data_length*(max_duration-min_duration)/(arr_time_steps/(24*60*60)) * (jd_max_dep - jd_min_dep)/ (dep_time_steps/(24*60*60))) + 1;

    all_data[0] = 0;

    char progress_text[100];
    sprintf(progress_text, "Calculating Porkchop (%s -> %s)", pochopro.dep_body->name, pochopro.arr_body->name);

    double t_dep = jd_min_dep;
    while(t_dep < jd_max_dep) {
        double t_arr = t_dep + min_duration;
        struct OSV s0 = osv_from_ephem(dep_ephem, t_dep, SUN());
        show_progress(progress_text, (t_dep-jd_min_dep), (jd_max_dep-jd_min_dep));

        while(t_arr < t_dep + max_duration) {
            struct OSV s1 = osv_from_ephem(arr_ephem, t_arr, SUN());
            double data[3];
            calc_transfer(tt, pochopro.dep_body, pochopro.arr_body, s0.r, s0.v, s1.r, s1.v, (t_arr-t_dep) * (24*60*60), data);
            if(!isnan(data[2])) {
                for(int i = 1; i <= data_length; i++) {
                    if(i == 1) all_data[(int) all_data[0] + i] = t_dep;
                    else all_data[(int) all_data[0] + i] = data[i-2];
                }
                all_data[0] += data_length;
            }
            t_arr += (arr_time_steps) / (24 * 60 * 60);
        }
        t_dep += (dep_time_steps) / (24 * 60 * 60);
    }
    show_progress(progress_text, 1, 1);
    printf("\n%d trajectories analyzed\n", (int)(all_data_size-1)/4);
}

void decrease_porkchop_size(int i, double **porkchops, struct Ephem **ephems, struct Body **bodies) {
    double *temp = (double *) malloc(((int) porkchops[i][0] + 1) * sizeof(double));
    temp[0] = 0;
    if (i == 0) {
        double min_dep_dv = get_min_from_porkchop(porkchops[0], 3);
        for (int j = 0; j < (int) (porkchops[0][0] / 4); j++) {
            show_progress("Decreasing Porkchop size", (double) j, (porkchops[0][0] / 4));
            if (porkchops[0][j * 4 + 3] < 2 * min_dep_dv) {
                for (int k = 1; k <= 4; k++) {
                    temp[(int) temp[0] + k] = porkchops[0][j * 4 + k];
                }
                temp[0] += 4;
            }
        }
        show_progress("Decreasing Porkchop size", 1, 1);
        printf("\nTrajectories remaining: %d\n", (int) temp[0] / 4);
        free(porkchops[0]);
        porkchops[0] = realloc(temp, (int) (temp[0] + 1) * sizeof(double));
    } else {
        for (int j = 0; j < (int) (porkchops[i][0] / 4); j++) {
            show_progress("Finding fly-bys", (double) j, (porkchops[i][0] / 4));
            for (int k = 0; k < (int) (porkchops[i - 1][0] / 4); k++) {
                if (porkchops[i - 1][k * 4 + 1] + porkchops[i - 1][k * 4 + 2] != porkchops[i][j * 4 + 1]) continue;
                double arr_v = porkchops[i - 1][k * 4 + 4];
                double dep_v = porkchops[i][j * 4 + 3];
                if (fabs(arr_v - dep_v) < 10) {
                    double data[3]; // not used (just as parameter for calc_transfer)
                    double t_dep = porkchops[i - 1][k * 4 + 1];
                    double t_arr = porkchops[i][j * 4 + 1];
                    struct OSV s0 = osv_from_ephem(ephems[i - 1], t_dep, SUN());
                    struct OSV s1 = osv_from_ephem(ephems[i], t_arr, SUN());
                    struct Transfer transfer1 = calc_transfer(circcirc, bodies[i], bodies[i + 1], s0.r, s0.v, s1.r,
                                                              s1.v, (t_arr - t_dep) * (24 * 60 * 60), data);

                    t_dep = porkchops[i][j * 4 + 1];
                    t_arr = porkchops[i][j * 4 + 1] + porkchops[i][j * 4 + 2];
                    s0 = s1;
                    s1 = osv_from_ephem(ephems[i + 1], t_arr, SUN());
                    struct Transfer transfer2 = calc_transfer(circcirc, bodies[i], bodies[i + 1], s0.r, s0.v, s1.r,
                                                              s1.v, (t_arr - t_dep) * (24 * 60 * 60), data);

                    struct Vector temp1 = add_vectors(transfer1.v1, scalar_multiply(s0.v, -1));
                    struct Vector temp2 = add_vectors(transfer2.v0, scalar_multiply(s0.v, -1));
                    double beta = (M_PI - angle_vec_vec(temp1, temp2)) / 2;
                    double rp = (1 / cos(beta) - 1) * (bodies[i]->mu / (pow(vector_mag(temp1), 2)));
                    if (rp > bodies[i]->radius + bodies[i]->atmo_alt) {
                        for (int l = 1; l <= 4; l++) {
                            temp[(int) temp[0] + l] = porkchops[i][j * 4 + l];
                        }
                        temp[0] += 4;
                        break;  // only need to store this transfer once
                    }
                }
            }
        }

        show_progress("Finding fly-bys", 1, 1);
        if (temp[0] == 0) {
            printf("\nNo trajectories found\n");
            return;
        }
        printf("\nTrajectories remaining: %d\n", (int) temp[0] / 4);
        free(porkchops[i]);
        porkchops[i] = realloc(temp, (int) (temp[0] + 1) * sizeof(double));
    }
}

double get_min_from_porkchop(const double *pc, int index) {
    double min = 1e20;
    for(int i = 0; i < pc[0]/4; i++) {
        double temp = pc[i*4 + index];
        if(temp < min) min = temp;
    }
    return min;
}

double get_min_arr_from_porkchop(const double *pc) {
    double min = 1e20;
    for(int i = 0; i < pc[0]/4; i++) {
        double arr = pc[i*4 + 1] + pc[i*4 + 2];
        if(arr < min) min = arr;
    }
    return min;
}

double get_max_arr_from_porkchop(const double *pc) {
    double max = -1e20;
    for(int i = 0; i < pc[0]/4; i++) {
        double arr = pc[i*4 + 1] + pc[i*4 + 2];
        if(arr > max) max = arr;
    }
    return max;
}

void get_cheapest_transfer_dates(double **porkchops, double *p_dep, double dv_dep, int index, int num_transfers, double *current_dates, double *jd_dates, double *min, double *final_porkchop) {
    for(int i = 0; i < (int)(porkchops[index][0] / 4); i++) {
        double *p_arr = porkchops[index] + i * 4;
        if(p_dep[1]+p_dep[2] != p_arr[1] || fabs(p_dep[4]-p_arr[3]) > 10) continue;

        current_dates[index] = p_arr[1];
        if(index < num_transfers-1) {
            get_cheapest_transfer_dates(porkchops, p_arr, dv_dep, index + 1, num_transfers, current_dates, jd_dates, min, final_porkchop);
        } else {
            current_dates[index+1] = p_arr[1]+p_arr[2];
            double dv_arr = p_arr[4];
            if (dv_dep + p_arr[4] < *min) {
                for(int j = 0; j <= num_transfers; j++) jd_dates[j] = current_dates[j];
                *min = dv_dep + dv_arr;
            }

            int pc_index = (int)final_porkchop[0];
            for(int j = (int)final_porkchop[0]-4; j >= 0; j -= 4) {
                if(current_dates[0] != final_porkchop[j+1]) break;
                if(current_dates[num_transfers] == final_porkchop[j+1]+final_porkchop[j+2]) {
                    pc_index = j;
                    break;
                }
            }
            if(pc_index < final_porkchop[0]) {
                if(dv_dep+dv_arr < final_porkchop[pc_index + 3] + final_porkchop[pc_index + 4]) {
                    final_porkchop[pc_index + 3] = dv_dep;
                    final_porkchop[pc_index + 4] = dv_arr;
                }
            } else {
                final_porkchop[pc_index + 1] = current_dates[0];
                final_porkchop[pc_index + 2] = current_dates[num_transfers] - current_dates[0];
                final_porkchop[pc_index + 3] = dv_dep;
                final_porkchop[pc_index + 4] = dv_arr;
                final_porkchop[0] += 4;
            }
        }
    }
}
