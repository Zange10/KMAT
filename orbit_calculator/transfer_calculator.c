#include "transfer_calculator.h"
#include "celestial_bodies.h"
#include "transfer_tools.h"
#include "ephem.h"
#include "csv_writer.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>

struct Porkchop_Properties {
    double jd_min_dep, jd_max_dep, dep_time_steps, arr_time_steps;
    int min_duration, max_duration;
    struct Ephem **ephems;
    struct Body *dep_body, *arr_body;
};

enum Transfer_Type {
    capcap,
    circcap,
    capcirc,
    circcirc,
    capfb,
    circfb
};

void create_porkchop(struct Porkchop_Properties pochopro, enum Transfer_Type tt, double *all_data);
struct Ephem get_last_ephem(struct Ephem *ephem, double date);
struct Transfer calc_transfer(enum Transfer_Type tt, struct Body *dep_body, struct Body *arr_body, struct Vector r1, struct Vector v1, struct Vector r2, struct Vector v2, double dt, double *data);
double get_min_arr_from_porkchop(double *pc);
double get_max_arr_from_porkchop(double *pc);
double get_min_from_porkchop(double *pc, int index);
struct OSV default_osv();


void create_transfer() {
    struct Body *bodies[3] = {EARTH(), VENUS(), MARS()};
    int num_bodies = (int) (sizeof(bodies)/sizeof(struct Body*));

    struct Date min_dep_date = {2024, 10, 1, 0, 0, 0};
    struct Date max_dep_date = {2025, 10, 1, 0, 0, 0};
    int min_duration[2] = {60, 120};         // [days]
    int max_duration[2] = {200, 500};         // [days]
    double dep_time_steps = 24 * 60 * 60; // [seconds]
    double arr_time_steps = 24 * 60 * 60; // [seconds]

    double jd_min_dep = convert_date_JD(min_dep_date);
    double jd_max_dep = convert_date_JD(max_dep_date);

    int max_total_dur = 0;
    for(int i = 0; i < num_bodies-1; i++) max_total_dur += max_duration[i];
    double jd_max_arr = convert_date_JD(max_dep_date) + max_total_dur;

    int ephem_time_steps = 10;  // [days]
    int num_ephems = (int)(jd_max_arr - jd_min_dep) / ephem_time_steps + 1;

    struct Ephem **ephems = (struct Ephem**) malloc(num_bodies*sizeof(struct Ephem*));
    for(int i = 0; i < num_bodies; i++) {
        ephems[i] = (struct Ephem*) malloc(num_ephems*sizeof(struct Ephem));
        get_ephem(ephems[i], num_ephems, bodies[i]->id, ephem_time_steps, jd_min_dep, jd_max_arr, 1);
    }


    double ** porkchops = (double**) malloc((num_bodies-1) * sizeof(double*));

    for(int i = 0; i < num_bodies-1; i++) {
        double min_dep, max_dep;
        if(i == 0) {
            min_dep = jd_min_dep;
            max_dep = jd_max_dep;
        } else {
            min_dep = get_min_arr_from_porkchop(porkchops[i-1]);
            max_dep = get_max_arr_from_porkchop(porkchops[i-1]);
        }

        struct Porkchop_Properties pochopro = {
                min_dep,
                max_dep,
                dep_time_steps,
                arr_time_steps,
                min_duration[i],
                max_duration[i],
                ephems+i,
                EARTH(),
                VENUS()
        };
        // 4 = dep_time, duration, dv1, dv2
        int data_length = 4;
        int all_data_size = (int) (data_length * (max_duration[i] - min_duration[i]) / (arr_time_steps / (24 * 60 * 60)) *
                                   (max_dep - min_dep) / (dep_time_steps / (24 * 60 * 60))) + 1;
        porkchops[i] = (double *) malloc(all_data_size * sizeof(double));

        create_porkchop(pochopro, circcirc, porkchops[i]);

        if(i == 0) {
            double min_dep_dv = get_min_from_porkchop(porkchops[0], 3);
            int num_2_min_dv = 0;
            for(int j = 0; j < (int)(porkchops[0][0]/4); j++) {
                if(porkchops[0][j*4 + 3] < 2*min_dep_dv) num_2_min_dv++;
            }
            double *temp = (double*) malloc(4 * num_2_min_dv * sizeof(double) + 1);
            temp[0] = 0;
            for(int j = 0; j < (int)(porkchops[0][0]/4); j++) {
                if(porkchops[0][j*4 + 3] < 2*min_dep_dv) {
                    for(int k = 1; k <= 4; k++) {
                        temp[(int)temp[0] + k] = porkchops[0][j*4 + k];
                    }
                    temp[0] += 4;
                }
            }
            free(porkchops[0]);
            porkchops[0] = temp;
        } else {
            int num_fb = 0;
            for(int j = 0; j < (int)(porkchops[i-1][0]/4); j++) {
                for(int k = 0; k < (int)(porkchops[i][0]/4); k++) {
                    if(porkchops[i-1][j * 4 + 1] + porkchops[i-1][j * 4 + 2] != porkchops[i][k * 4 + 1]) continue;
                    double arr_v = porkchops[i-1][j * 4 + 4];
                    double dep_v = porkchops[i][k * 4 + 3];
                    if(fabs(arr_v - dep_v) < 10) num_fb++;
                }
            }

            double *temp = (double*) malloc(4 * num_fb * sizeof(double) + 1);
            temp[0] = 0;
            for(int j = 0; j < (int)(porkchops[i-1][0]/4); j++) {
                for(int k = 0; k < (int)(porkchops[i][0]/4); k++) {
                    if(porkchops[i-1][j * 4 + 1] + porkchops[i-1][j * 4 + 2] != porkchops[i][k * 4 + 1]) continue;
                    double arr_v = porkchops[i-1][j * 4 + 4];
                    double dep_v = porkchops[i][k * 4 + 3];
                    if(fabs(arr_v - dep_v) < 10) {
                        for (int l = 1; l <= 4; l++) {
                            temp[(int) temp[0] + l] = porkchops[i][k * 4 + l];
                        }
                        temp[0] += 4;
                    }
                }
            }
            free(porkchops[i]);
            porkchops[i] = temp;
        }
    }
}

void create_porkchop(struct Porkchop_Properties pochopro, enum Transfer_Type tt, double *all_data) {
    struct timeval start, end;
    gettimeofday(&start, NULL);  // Record the starting time

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

    int progress = -1;
    int mind = 0;
    double mindv = 1e9;

    double t_dep = jd_min_dep;
    while(t_dep < jd_max_dep) {
        double t_arr = t_dep + min_duration;
        struct Ephem last_eph0 = get_last_ephem(dep_ephem, t_dep);
        struct Vector r0 = {last_eph0.x, last_eph0.y, last_eph0.z};
        struct Vector v0 = {last_eph0.vx, last_eph0.vy, last_eph0.vz};
        double dt0 = (t_dep-last_eph0.date)*(24*60*60);
        struct OSV s0 = propagate_orbit(r0, v0, dt0, SUN());
//        print_date(convert_JD_date(t_dep), 1);

        if((int)(100*(t_dep-jd_min_dep)/(jd_max_dep-jd_min_dep)) > progress) {
            progress = (int)(100*(t_dep-jd_min_dep)/(jd_max_dep-jd_min_dep));
            printf("% 3d%%\n",progress);
        }

        while(t_arr < t_dep + max_duration) {
            struct Ephem last_eph1 = get_last_ephem(arr_ephem, t_arr);
            struct Vector r1 = {last_eph1.x, last_eph1.y, last_eph1.z};
            struct Vector v1 = {last_eph1.vx, last_eph1.vy, last_eph1.vz};
            double dt1 = (t_arr-last_eph1.date)*(24*60*60);

            struct OSV s1 = propagate_orbit(r1, v1, dt1, SUN());

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

    printf("%d trajectories analyzed\n", (int)(all_data_size-1)/4);

//    char data_fields[] = "dep_date,duration,dv_dep,dv_arr";
//    write_csv(data_fields, all_data);


    gettimeofday(&end, NULL);  // Record the ending time
    double elapsed_time;
    elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("\n\nElapsed time: %f seconds\n", elapsed_time);

    t_dep = all_data[mind*4+1];
    double t_arr = t_dep + all_data[mind*4+2];

    //free(all_data);
/*
    struct Ephem last_eph0 = get_last_ephem(dep_ephem, t_dep);
    struct Vector r0 = {last_eph0.x, last_eph0.y, last_eph0.z};
    struct Vector v0 = {last_eph0.vx, last_eph0.vy, last_eph0.vz};
    double dt0 = (t_dep-last_eph0.date)*(24*60*60);
    struct OSV s0 = propagate_orbit(r0, v0, dt0, SUN());

    struct Ephem last_eph1 = get_last_ephem(arr_ephem, t_arr);
    struct Vector r1 = {last_eph1.x, last_eph1.y, last_eph1.z};
    struct Vector v1 = {last_eph1.vx, last_eph1.vy, last_eph1.vz};
    double dt1 = (t_arr-last_eph1.date)*(24*60*60);
    struct OSV s1 = propagate_orbit(r1, v1, dt1, SUN());

    double data[3];
    struct Transfer transfer = calc_transfer(s0.r, s0.v, s1.r, s1.v, (t_arr-t_dep) * (24*60*60), data);
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
    char pcsv;
    printf("Write transfer data to .csv (y/Y=yes)? ");
    scanf(" %c", &pcsv);
    if (pcsv == 'y' || pcsv == 'Y') {
        char transfer_data_fields[] = "JD,X,Y,Z,VX,VY,VZ";
        write_csv(transfer_data_fields, transfer_data);
    }*/
}

struct Transfer calc_transfer(enum Transfer_Type tt, struct Body *dep_body, struct Body *arr_body, struct Vector r1, struct Vector v1, struct Vector r2, struct Vector v2, double dt, double *data) {
    double dtheta = angle_vec_vec(r1, r2);
    if (cross_product(r1, r2).z < 0) dtheta = 2 * M_PI - dtheta;

    struct Transfer2D transfer2d = calc_2d_transfer_orbit(vector_mag(r1), vector_mag(r2), dt, dtheta, SUN());

    struct Transfer transfer = calc_transfer_dv(transfer2d, r1, r2);

    double v_t1_inf = fabs(vector_mag(add_vectors(transfer.v0, scalar_multiply(v1, -1))));
    double dv1 = tt % 2 == 0 ? dv_capture(dep_body, 200e3, v_t1_inf) : dv_circ(dep_body, 200e3, v_t1_inf);
    double v_t2_inf = fabs(vector_mag(add_vectors(transfer.v1, scalar_multiply(v2, -1))));
    double dv2;
    if(tt < 2)      dv2 = dv_capture(arr_body, 200e3, v_t2_inf);
    else if(tt < 4) dv2 = dv_circ(arr_body, 200e3, v_t2_inf);
    else            dv2 = 0;
    data[0] = dt/(24*60*60);
    data[1] = dv1;
    data[2] = dv2;
    return transfer;
}

struct OSV default_osv() {
    struct Vector r = {0,0,0};
    struct Vector v = {0,0,0};
    struct OSV osv = {r,v};
    return osv;
}

struct Ephem get_last_ephem(struct Ephem *ephem, double date) {
    int i = 0;
    while (ephem[i].date > 0) {
        if(date < ephem[i].date) return ephem[i-1];
        i++;
    }
    return ephem[i-1];
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
