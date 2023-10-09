#include "transfer_calculator.h"
#include "analytic_geometry.h"
#include "celestial_bodies.h"
#include "orbit.h"
#include <stdio.h>
#include <sys/time.h>
#include <math.h>

struct Transfer2D {
    struct Orbit orbit;
    double theta1, theta2;
};

struct Transfer2D calc_2d_transfer_orbit(double r1, double r2, double target_dt, double dtheta, struct Body *attractor);
double calc_transfer_dv(struct Transfer2D transfer2d, struct Vector r1, struct Vector v1, struct Vector r2, struct Vector v2);


void init_transfer() {

    struct timeval start, end;
    gettimeofday(&start, NULL);  // Record the starting time
//    struct Vector r1_norm = {1, 0, 0};
//    struct Vector r1 = scalar_multiply(r1_norm, 150e9);
//    struct Vector v1_norm = {0, 1, 0};
//    struct Vector v1 = scalar_multiply(v1_norm, sqrt(SUN()->mu * (1 / vector_mag(r1))));
//    struct Vector r2_norm = {-0.8, 0.857, 0.14};
//    //r2_norm = norm_vector(r2_norm);
//    //struct Vector r2 = scalar_multiply(r2_norm, 108.9014e9);
//    struct Vector r2 = {-74e9, 79e9, 4e9};
//    struct Vector v2_norm = {-0.857, -0.8, 0};
    struct Vector r1 = {100e9, 180e9, -2e9};
    struct Vector v1 = {-20000, 15000, 2000};
    struct Vector r2 = {-374e9, -179e9, 4e9};
    struct Vector v2 = {8000, -13000, 3000};

    double dt = 271*(24*60*60);
    double dtheta = angle_vec_vec(r1, r2);
    struct Transfer2D transfer2d = calc_2d_transfer_orbit(vector_mag(r1), vector_mag(r2), dt, dtheta, SUN());
    double dv = calc_transfer_dv(transfer2d, r1, v1, r2, v2);
    printf("dv: %f m/s - dtheta: %f°\n", dv, rad2deg(dtheta));

    gettimeofday(&end, NULL);  // Record the ending time
    double elapsed_time;
    elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Elapsed time: %f seconds\n", elapsed_time);
}

struct Transfer2D calc_2d_transfer_orbit(double r1, double r2, double target_dt, double dtheta, struct Body *attractor) {
    double theta1 = r1 > r2 ? deg2rad(180) : 0;
    double theta2 = theta1+dtheta;
    double mu = attractor->mu;
    double step = -deg2rad(10);
    double dt = 1e20;
    double a = 0;
    double e = 0;

    while(fabs(dt-target_dt) > 1e-5) {
        theta1 = pi_norm(theta1);
        theta2 = pi_norm(theta1 + dtheta);
        e = (r2-r1)/(r1*cos(theta1)-r2*cos(theta2));
        double rp = r1*(1+e*cos(theta1))/(1+e);
        a = rp/(1-e);
        double n = sqrt(mu / pow(a,3));

        double E1 = acos( (e+cos(theta1))/(1+e*cos(theta1)) );
        double t1 = (E1-e*sin(E1))/n;
        //if(theta1 > M_PI) t1 = 2*M_PI * sqrt(pow(a,3) / mu) - t1;
        double E2 = acos( (e+cos(theta2))/(1+e*cos(theta2)) );
        double t2 = (E2-e*sin(E2))/n;
        //if(theta2 > M_PI) t2 = 2*M_PI * sqrt(pow(a,3) / mu) - t2;

        dt = theta1 < theta2 ? t2-t1 : t2 + t1;
        double ddt = dt-target_dt;
        double ts[] = {t1/(24*60*60), t2/(24*60*60)};
        double x[] = {dt/(24*60*60), target_dt/(24*60*60), ddt/(24*60*60)};


        if((dt-target_dt)*(r1-r2) > 0) {
            if(step < 0) step *= -1.0/3;
            theta1 += step;
        } else {
            if(step > 0) step *= -1.0/3;
            theta1 += step;
        }
    }

    printf("Theta1: %f°, Theta2: %f°\n", rad2deg(theta1), rad2deg(theta2));

    struct Transfer2D transfer = {constr_orbit(a, e, 0, 0, 0, SUN()), theta1, theta2};
    printf("a: %f, e: %f\n",a,e);
    return transfer;
}

struct Vector2D calc_v_2d(double r_mag, double v_mag, double theta, double gamma) {
    struct Vector2D r_norm = {-cos(theta), -sin(theta)};
    struct Vector2D r = scalar_multipl2d(r_norm, r_mag);
    struct Vector2D n = {-r.y, r.x};
    struct Vector2D v = rotate_vector2d(n, gamma);
    v = scalar_multipl2d(v, v_mag/ vector2d_mag(v));

    return v;
}

struct Vector heliocentric_rot(struct Vector2D v, double RAAN, double w, double i) {
    double Q[3][3] = {
            {-sin(RAAN)*cos(i)*sin(w)+cos(RAAN)*cos(w),     -sin(RAAN)*cos(i)*cos(w)-cos(RAAN)*sin(w),   sin(RAAN)*sin(i)},
            { cos(RAAN)*cos(i)*sin(w)+sin(RAAN)*cos(w),      cos(RAAN)*cos(i)*cos(w)-sin(RAAN)*sin(w),  -cos(RAAN)*sin(i)},
            { sin(i)*sin(w),                                 sin(i)*cos(w),                              cos(i)}};

    double v_vec[3] = {v.x, v.y, 0};
    double result[3] = {0,0,0};

    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            result[i] += Q[i][j]*v_vec[j];
        }
    }
    struct Vector result_v = {result[0], result[1], result[2]};
    return result_v;
}

double dv_circ(struct Body *body, double rp, double vinf) {
    rp += body->radius;
    return sqrt(2*body->mu/rp+vinf*vinf) - sqrt(body->mu/rp);
}

double dv_capture(struct Body *body, double rp, double vinf) {
    rp += body->radius;
    return sqrt(2*body->mu/rp+vinf*vinf) - sqrt(2*body->mu/rp);
}

double calc_transfer_dv(struct Transfer2D transfer2d, struct Vector r1, struct Vector v1, struct Vector r2, struct Vector v2) {
    struct Vector origin = {0, 0, 0};
    struct Plane p_0 = constr_plane(origin, vec(1,0,0), vec(0,1,0));
    struct Plane p_T = constr_plane(origin, r1, r2);
    struct Plane p_1 = constr_plane(r1, v1, scalar_multiply(r1, -1));
    struct Plane p_2 = constr_plane(r2, v2, scalar_multiply(r2, -1));
    struct Orbit orbit2d = transfer2d.orbit;
    double a = orbit2d.a;
    double e = orbit2d.e;
    double theta1 = transfer2d.theta1;
    double theta2 = transfer2d.theta2;
    double argument_orbit = theta1 - M_PI;

    double gamma1 = e*sin(theta1)/(1+e*cos(theta1));
    double gamma2 = e*sin(theta2)/(1+e*cos(theta2));

    double v_t1_mag = calc_orbital_speed(orbit2d, vector_mag(r1));
    double v_t2_mag = calc_orbital_speed(orbit2d, vector_mag(r2));

    struct Vector2D v_t1_2d = calc_v_2d(vector_mag(r1), v_t1_mag, theta1, gamma1);
    struct Vector2D v_t2_2d = calc_v_2d(vector_mag(r2), v_t2_mag, theta2, gamma2);

    v_t1_2d = rotate_vector2d(v_t1_2d, argument_orbit);
    v_t2_2d = rotate_vector2d(v_t2_2d, argument_orbit);

    print_vector2d(v_t1_2d);
    print_vector2d(v_t2_2d);

    double RAAN = angle_vec_vec(vec(1,0,0), vec(r1.x,r1.y,0));
    double i = angle_plane_plane(p_T, p_0);
    double arg_peri = deg2rad(0);
    struct Vector v_t1 = heliocentric_rot(v_t1_2d, RAAN, arg_peri, i);
    struct Vector v_t2 = heliocentric_rot(v_t2_2d, RAAN, arg_peri, i);
    printf("RAAN: %f, arg_peri: %f, i: %f\n", rad2deg(RAAN), rad2deg(arg_peri), rad2deg(i));

    double v_t1_inf = fabs(vector_mag(add_vectors(v_t1, scalar_multiply(v1, -1))));
    double dv1 = dv_circ(EARTH(), 150e3, v_t1_inf);
    double v_t2_inf = fabs(vector_mag(add_vectors(v_t2, scalar_multiply(v2, -1))));
    double dv2 = dv_capture(VENUS(), 250e3, v_t2_inf);

    //print_vector(scalar_multiply(v_t1,1e-3));
    //print_vector(scalar_multiply(v_t2,1e-3));


    //print_vector(norm_vector(v_t1));
    //print_vector(norm_vector(v_t2));


    print_vector(scalar_multiply(v_t1,1e-3));
    print_vector(scalar_multiply(v_t2,1e-3));

    printf("%f\n", rad2deg(angle_vec_vec(vec(1,0,0), r1)));

    return dv1+dv2;
}