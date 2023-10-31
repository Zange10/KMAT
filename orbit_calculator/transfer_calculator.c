#include "transfer_calculator.h"
#include "celestial_bodies.h"
#include "transfer_tools.h"
#include "ephem.h"
#include <stdio.h>
#include <sys/time.h>
#include <math.h>


struct Ephem get_last_ephem(struct Ephem *ephem, int ephem_size, double date);

void create_porkchop() {
    struct timeval start, end;
    gettimeofday(&start, NULL);  // Record the starting time

    struct Date min_dep_date = {2025, 1, 1, 0, 0, 0};
    struct Date max_dep_date = {2026, 1, 1, 0, 0, 0};
    int max_duration = 300;         // [days]
    int min_duration = 20;         // [days]
    double time_steps = 1*24*60*60; // [seconds]

    double jd_min_dep = convert_date_JD(min_dep_date);
    double jd_max_dep = convert_date_JD(max_dep_date);

    int ephem_time_steps = 10;  // [days]
    int num_ephems = (int)(max_duration + jd_max_dep - jd_min_dep) / ephem_time_steps + 1;


    struct Ephem earth_ephem[num_ephems];
    struct Ephem venus_ephem[num_ephems];

    get_ephem(venus_ephem, num_ephems, 3, 10, jd_min_dep, jd_max_dep);
    get_ephem(earth_ephem, num_ephems, 2, 10, jd_min_dep + min_duration, jd_max_dep + max_duration);

    double t_dep = jd_min_dep;
    while(t_dep < jd_max_dep) {
        double t_arr = t_dep + min_duration;
        while(t_arr < t_dep + max_duration) {
            struct Ephem last_eph0 = get_last_ephem(earth_ephem, num_ephems, t_arr);
            struct Ephem last_eph1 = get_last_ephem(venus_ephem, num_ephems, t_arr);
            struct Vector r0 = {last_eph0.x, last_eph0.y, last_eph0.z};
            struct Vector v0 = {last_eph0.vx, last_eph0.vy, last_eph0.vz};
            struct Vector r1 = {last_eph1.x, last_eph1.y, last_eph1.z};
            struct Vector v1 = {last_eph1.vx, last_eph1.vy, last_eph1.vz};
            double dt0 = t_dep-last_eph0.date;
            double dt1 = t_arr-last_eph1.date;

            struct Orbital_State_Vectors s0 = propagate_orbit(r0, v0, dt0, SUN());
            struct Orbital_State_Vectors s1 = propagate_orbit(r1, v1, dt1, SUN());
            double data[3];

            data[0] = t_dep;
            calc_transfer(s0.r, s0.v, s1.r, s1.v, (t_arr-t_dep) * (24*60*60), data);

            print_date(convert_JD_date(data[0]), 0);
            printf("  %4.1f  %f\n", data[1], data[2]);
            t_arr += (time_steps)/(24*60*60);
        }
        t_dep += (time_steps)/(24*60*60);
    }


    gettimeofday(&end, NULL);  // Record the ending time
    double elapsed_time;
    elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Elapsed time: %f seconds\n", elapsed_time);
}

void calc_transfer(struct Vector r1, struct Vector v1, struct Vector r2, struct Vector v2, double dt, double *data) {
    double dtheta = angle_vec_vec(r1, r2);
    if (cross_product(r1, r2).z < 0) dtheta = 2 * M_PI - dtheta;

    struct Transfer2D transfer2d = calc_2d_transfer_orbit(vector_mag(r1), vector_mag(r2), dt, dtheta, SUN());

    struct Transfer transfer = calc_transfer_dv(transfer2d, r1, r2);

    double v_t1_inf = fabs(vector_mag(add_vectors(transfer.v0, scalar_multiply(v1, -1))));
    double dv = dv_circ(EARTH(), 200e3, v_t1_inf);
    double v_t2_inf = fabs(vector_mag(add_vectors(transfer.v1, scalar_multiply(v2, -1))));

    dv += dv_capture(VENUS(), 200e3, v_t2_inf);

    data[1] = dt/(24*60*60);
    data[2] = dv;
}

struct Ephem get_last_ephem(struct Ephem *ephem, int ephem_size, double date) {
    for(int i = 0; i < ephem_size; i++) {
        if(date < ephem[i].date) return ephem[i-1];
    }
    return ephem[ephem_size-1];
}
