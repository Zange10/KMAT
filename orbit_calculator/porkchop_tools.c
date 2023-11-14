#include "porkchop_tools.h"
#include "tool_funcs.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void create_porkchop(struct Porkchop_Properties pochopro, enum Transfer_Type tt, double *all_data) {
/*    struct timeval start, end;
    gettimeofday(&start, NULL);  // Record the starting time*/

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

    int mind = 0;
    double mindv = 1e9;

    char progress_text[100];
    sprintf(progress_text, "Calculating Porkchop (%s -> %s)", pochopro.dep_body->name, pochopro.arr_body->name);

    double t_dep = jd_min_dep;
    while(t_dep < jd_max_dep) {
        double t_arr = t_dep + min_duration;
        struct OSV s0 = osv_from_ephem(dep_ephem, t_dep, SUN());
        show_progress(progress_text, (t_dep-jd_min_dep), (jd_max_dep-jd_min_dep));

        while(t_arr < t_dep + max_duration) {
            struct OSV s1 = osv_from_ephem(arr_ephem, t_arr, SUN());

//            printf("\n");
//            print_date(convert_JD_date(t_dep), 0);
//            printf(" (%f) - ", t_dep);
//            print_date(convert_JD_date(t_arr), 0);
//            printf(" (%.2f days)\n", t_arr-t_dep);
//            print_vector(scalar_multiply(s0.r,1e-9));
//            print_vector(scalar_multiply(s1.r,1e-9));
//            print_vector(scalar_multiply(r0,1e-9));
//            print_vector(scalar_multiply(r1,1e-9));
//            printf("\n(%f, %f, %f) (%f)\n", s0.r.x*1e-9, s0.r.y*1e-9, s0.r.z*1e-9, vector_mag(s0.r)*1e-9);
//            printf("(%f, %f, %f) (%f)\n", s1.r.x*1e-9, s1.r.y*1e-9, s1.r.z*1e-9, vector_mag(s1.r)*1e-9);
            double data[3];
            calc_transfer(tt, pochopro.dep_body, pochopro.arr_body, s0.r, s0.v, s1.r, s1.v, (t_arr-t_dep) * (24*60*60), data);
            if(!isnan(data[2])) {
                for(int i = 1; i <= data_length; i++) {
                    if(i == 1) all_data[(int) all_data[0] + i] = t_dep;
                    else all_data[(int) all_data[0] + i] = data[i-2];
                }


                if(data[1]+data[2] < mindv) {
                    mindv = data[1] + data[2];
                    mind = (int)(all_data[0]/data_length);
                }
                all_data[0] += data_length;
            }
//            printf(",%f", data[2]);
//            print_date(convert_JD_date(data[0]), 0);
//            printf("  %4.1f  %f\n", data[1], data[2]);
            t_arr += (arr_time_steps) / (24 * 60 * 60);
        }
        t_dep += (dep_time_steps) / (24 * 60 * 60);
    }
    show_progress(progress_text, 1, 1);
    printf("\n%d trajectories analyzed\n", (int)(all_data_size-1)/4);

//    char data_fields[] = "dep_date,duration,dv_dep,dv_arr";
//    write_csv(data_fields, all_data);


/*    gettimeofday(&end, NULL);  // Record the ending time
    double elapsed_time;
    elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("\n\nElapsed time: %f seconds\n", elapsed_time);*/

    t_dep = all_data[mind*4+1];
    double t_arr = t_dep + all_data[mind*4+2];

    //free(all_data);
}

double get_min_from_porkchop(double *pc, int index) {
    double min = 1e20;
    for(int i = 0; i < pc[0]/4; i++) {
        double temp = pc[i*4 + index];
        if(temp < min) min = temp;
    }
    return min;
}

double get_min_arr_from_porkchop(double *pc) {
    double min = 1e20;
    for(int i = 0; i < pc[0]/4; i++) {
        double arr = pc[i*4 + 1] + pc[i*4 + 2];
        if(arr < min) min = arr;
    }
    return min;
}

double get_max_arr_from_porkchop(double *pc) {
    double max = -1e20;
    for(int i = 0; i < pc[0]/4; i++) {
        double arr = pc[i*4 + 1] + pc[i*4 + 2];
        if(arr > max) max = arr;
    }
    return max;
}