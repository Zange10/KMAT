#include "transfer_calculator.h"
#include "analytic_geometry.h"
#include "celestial_bodies.h"
#include "transfer_tools.h"
#include "ephem.h"
#include <stdio.h>
#include <sys/time.h>
#include <math.h>



void init_transfer() {


    struct Ephem ephem[750];
    get_ephem(ephem, sizeof(ephem) / sizeof(struct Ephem));

    for(int i = 0; i < 200; i++) {
        print_ephem(ephem[i]);
    }

    return;
    struct timeval start, end;
    gettimeofday(&start, NULL);  // Record the starting time
    struct Vector r1_norm = {1, 0, 0};
    struct Vector r1 = scalar_multiply(r1_norm, 150e9);
    struct Vector v1_norm = {0, 1, 0};
    struct Vector v1 = scalar_multiply(v1_norm, sqrt(SUN()->mu * (1 / vector_mag(r1))));
    struct Vector r2_norm = {-0.8, 0.857, 0.14};
    //r2_norm = norm_vector(r2_norm);
    //struct Vector r2 = scalar_multiply(r2_norm, 108.9014e9);
    struct Vector r2 = {-74e9, 79e9, 4e9};
    struct Vector v2_norm = {-0.857, -0.8, 0};
    struct Vector v2 = scalar_multiply(norm_vector(v2_norm), sqrt(SUN()->mu * (1 / vector_mag(r2))));

    //print_vector(scalar_multiply(r1, 1e-9));
    //print_vector(scalar_multiply(r2, 1e-9));
    //print_vector(v1);
    //print_vector(v2);

    /* class two transfer */
    //struct Vector r1 = {100e9, 180e9, 100e9};
    //struct Vector v1 = {-20000, 15000, 2000};
    //struct Vector r2 = {174e9, -379e9, 120e9};
    //struct Vector v2 = {3000, -18000, 3000};

    /* class one transfer*/
    //struct Vector r1 = {100e9, 180e9, 100e9};
    //struct Vector v1 = {-20000, 15000, 2000};
    //struct Vector r2 = {-374e9, -179e9, 120e9};
    //struct Vector v2 = {3000, -18000, 3000};

    for(int i = 30; i < 300; i++) {
        double dt = i * 24 * 60 * 60;
        double dtheta = angle_vec_vec(r1, r2);
        if (cross_product(r1, r2).z < 0) dtheta = 2 * M_PI - dtheta;
        struct Transfer2D transfer2d = calc_2d_transfer_orbit(vector_mag(r1), vector_mag(r2), dt, dtheta, SUN());

        struct Transfer transfer = calc_transfer_dv(transfer2d, r1, r2);


        double v_t1_inf = fabs(vector_mag(add_vectors(transfer.v0, scalar_multiply(v1, -1))));
        double dv = dv_circ(EARTH(), 150e3, v_t1_inf);
        double v_t2_inf = fabs(vector_mag(add_vectors(transfer.v1, scalar_multiply(v2, -1))));
        dv += dv_capture(VENUS(), 250e3, v_t2_inf);

        printf(",%f", dv);
    }
    //printf("dv: %f m/s - dtheta: %f°\n", dv, rad2deg(dtheta));

    printf("\n\n");
    gettimeofday(&end, NULL);  // Record the ending time
    double elapsed_time;
    elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Elapsed time: %f seconds\n", elapsed_time);
}