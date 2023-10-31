#include "transfer_calculator.h"
#include "celestial_bodies.h"
#include "transfer_tools.h"
#include "ephem.h"
#include "csv_writer.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>



struct Ephem get_last_ephem(struct Ephem *ephem, double date);

void create_porkchop() {

    struct timeval start, end;
    gettimeofday(&start, NULL);  // Record the starting time

    struct Date min_dep_date = {2024, 1, 1, 0, 0, 0};
    struct Date max_dep_date = {2025, 1, 1, 0, 0, 0};
    int min_duration = 300;         // [days]
    int max_duration = 1200;         // [days]
    double dep_time_steps = 24 * 60 * 60; // [seconds]
    double arr_time_steps = 24 * 60 * 60; // [seconds]

    double jd_min_dep = convert_date_JD(min_dep_date);
    double jd_max_dep = convert_date_JD(max_dep_date);

    int ephem_time_steps = 10;  // [days]
    int num_ephems = (int)(max_duration + jd_max_dep - jd_min_dep) / ephem_time_steps + 1;

    int all_data_size = (int)(4*(max_duration-min_duration)/(arr_time_steps/(24*60*60)) * (jd_max_dep - jd_min_dep)/ (dep_time_steps/(24*60*60))) + 1;

    double * all_data;
    all_data = (double*) malloc(all_data_size * sizeof(double));
    all_data[0] = 0;

    struct Ephem earth_ephem[num_ephems];
    struct Ephem venus_ephem[num_ephems];

    get_ephem(earth_ephem, num_ephems, 3, 10, jd_min_dep, jd_max_dep, 1);
    get_ephem(venus_ephem, num_ephems, 5, 10, jd_min_dep + min_duration, jd_max_dep + max_duration, 1);

    int progress = -1;
    int mind = 0;
    double mindv = 1e9;

    double t_dep = jd_min_dep;
    while(t_dep < jd_max_dep) {
        double t_arr = t_dep + min_duration;
        struct Ephem last_eph0 = get_last_ephem(earth_ephem, t_dep);
        struct Vector r0 = {last_eph0.x, last_eph0.y, last_eph0.z};
        struct Vector v0 = {last_eph0.vx, last_eph0.vy, last_eph0.vz};
        double dt0 = (t_dep-last_eph0.date)*(24*60*60);
        struct Orbital_State_Vectors s0 = propagate_orbit(r0, v0, dt0, SUN());
//        print_date(convert_JD_date(t_dep), 1);

        if((int)(100*(t_dep-jd_min_dep)/(jd_max_dep-jd_min_dep)) > progress) {
            progress = (int)(100*(t_dep-jd_min_dep)/(jd_max_dep-jd_min_dep));
            printf("% 3d%%\n",progress);
        }

        while(t_arr < t_dep + max_duration) {
            struct Ephem last_eph1 = get_last_ephem(venus_ephem, t_arr);
            struct Vector r1 = {last_eph1.x, last_eph1.y, last_eph1.z};
            struct Vector v1 = {last_eph1.vx, last_eph1.vy, last_eph1.vz};
            double dt1 = (t_arr-last_eph1.date)*(24*60*60);

            struct Orbital_State_Vectors s1 = propagate_orbit(r1, v1, dt1, SUN());

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
            calc_transfer(s0.r, s0.v, s1.r, s1.v, (t_arr-t_dep) * (24*60*60), data);
            if(!isnan(data[2])) {
                all_data[(int) all_data[0] + 1] = t_dep;
                all_data[(int) all_data[0] + 2] = data[0];
                all_data[(int) all_data[0] + 3] = data[1];
                all_data[(int) all_data[0] + 4] = data[2];
                if(data[1]+data[2] < mindv) {
                    mindv = data[1] + data[2];
                    mind = (int)(all_data[0]/4);
                }
                all_data[0] += 4;
            }
//            printf(",%f", data[2]);
//            print_date(convert_JD_date(data[0]), 0);
//            printf("  %4.1f  %f\n", data[1], data[2]);
            t_arr += (arr_time_steps) / (24 * 60 * 60);
        }
        t_dep += (dep_time_steps) / (24 * 60 * 60);
    }

    printf("%d trajectories analyzed\n", (int)(all_data_size-1)/4);

    char data_fields[] = "dep_date,duration,dv_dep,dv_arr";
    write_csv(data_fields, all_data);


    gettimeofday(&end, NULL);  // Record the ending time
    double elapsed_time;
    elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("\n\nElapsed time: %f seconds\n", elapsed_time);

    t_dep = all_data[mind*4+1];
    double t_arr = t_dep + all_data[mind*4+2];

    free(all_data);

    struct Ephem last_eph0 = get_last_ephem(earth_ephem, t_dep);
    struct Vector r0 = {last_eph0.x, last_eph0.y, last_eph0.z};
    struct Vector v0 = {last_eph0.vx, last_eph0.vy, last_eph0.vz};
    double dt0 = (t_dep-last_eph0.date)*(24*60*60);
    struct Orbital_State_Vectors s0 = propagate_orbit(r0, v0, dt0, SUN());

    struct Ephem last_eph1 = get_last_ephem(venus_ephem, t_arr);
    struct Vector r1 = {last_eph1.x, last_eph1.y, last_eph1.z};
    struct Vector v1 = {last_eph1.vx, last_eph1.vy, last_eph1.vz};
    double dt1 = (t_arr-last_eph1.date)*(24*60*60);
    struct Orbital_State_Vectors s1 = propagate_orbit(r1, v1, dt1, SUN());

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
    struct Orbital_State_Vectors osvs[num_states];
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

    char transfer_data_fields[] = "JD,X,Y,Z,VX,VY,VZ";
    write_csv(transfer_data_fields, transfer_data);
}

struct Transfer calc_transfer(struct Vector r1, struct Vector v1, struct Vector r2, struct Vector v2, double dt, double *data) {
    double dtheta = angle_vec_vec(r1, r2);
    if (cross_product(r1, r2).z < 0) dtheta = 2 * M_PI - dtheta;

    struct Transfer2D transfer2d = calc_2d_transfer_orbit(vector_mag(r1), vector_mag(r2), dt, dtheta, SUN());

    struct Transfer transfer = calc_transfer_dv(transfer2d, r1, r2);

    double v_t1_inf = fabs(vector_mag(add_vectors(transfer.v0, scalar_multiply(v1, -1))));
    double dv1 = dv_circ(EARTH(), 180e3, v_t1_inf);
    double v_t2_inf = fabs(vector_mag(add_vectors(transfer.v1, scalar_multiply(v2, -1))));
    double dv2 = dv_capture(JUPITER(), 1000e3, v_t2_inf);
    data[0] = dt/(24*60*60);
    data[1] = dv1;
    data[2] = dv2;
    return transfer;
}

struct Ephem get_last_ephem(struct Ephem *ephem, double date) {
    int i = 0;
    while (ephem[i].date > 0) {
        if(date < ephem[i].date) return ephem[i-1];
        i++;
    }
    return ephem[i-1];
}
