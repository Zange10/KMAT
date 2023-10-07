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
void calc_3d_orbit(struct Transfer2D transfer2d, struct Vector r1, struct Vector v1, struct Vector r2, struct Vector v2);


void init_transfer() {
    struct Vector r1_norm = {1, 0, 0};
    struct Vector r1 = scalar_multipl(r1_norm, 149.7848e9);
    struct Vector v1_norm = {0, 1, 0};
    struct Vector v1 = scalar_multipl(v1_norm, sqrt(SUN()->mu * (1 / vector_mag(r1))));
    struct Vector r2_norm = {-0.8, 0.857, 0};
    r2_norm = norm_vector(r2_norm);
    struct Vector r2 = scalar_multipl(r2_norm, 108.9014e9);
    struct Vector v2_norm = {-0.6, -0.8, 0};
    v2_norm = norm_vector(v2_norm);
    struct Vector v2 = scalar_multipl(v2_norm, sqrt(SUN()->mu * (1 / vector_mag(r2))));
    double dt = 109*(24*60*60);
    double dtheta = angle_vec_vec(r1, r2);
    print_vector(scalar_multipl(r2, 1e-9));
    struct Transfer2D transfer2d = calc_2d_transfer_orbit(vector_mag(r1), vector_mag(r2), dt, dtheta, SUN());

    calc_3d_orbit(transfer2d, r1, v1, r2, v2);
}

struct Transfer2D calc_2d_transfer_orbit(double r1, double r2, double target_dt, double dtheta, struct Body *attractor) {
    double theta1 = r1 > r2 ? deg2rad(180) : 0;
    double theta2 = theta1+dtheta;
    double mu = attractor->mu;
    double step = deg2rad(10);
    double dt = 1e20;
    double a = 0;
    double e = 0;

    struct timeval start, end;
    gettimeofday(&start, NULL);  // Record the starting time

    while(fabs(dt-target_dt) > 1e-10) {
        theta2 = theta1 + dtheta;
        e = (r2-r1)/(r1*cos(theta1)-r2*cos(theta2));
        double rp = r1*(1+e*cos(theta1))/(1+e);
        a = rp/(1-e);
        double n = sqrt(mu / pow(a,3));

        double E1 = acos( (e+cos(theta1))/(1+e*cos(theta1)) );
        double t1 = (E1-e*sin(E1))/n;
        if(theta1 > M_PI) t1 = 2*M_PI * sqrt(pow(a,3) / mu) - t1;
        double E2 = acos( (e+cos(theta2))/(1+e*cos(theta2)) );
        double t2 = (E2-e*sin(E2))/n;
        if(theta2 > M_PI) t2 = 2*M_PI * sqrt(pow(a,3) / mu) - t2;

        dt = t2-t1;

        if(dt-target_dt > 0) {
            if(step < 0) step *= -1.0/5;
            theta1 += step;
        } else {
            if(step > 0) step *= -1.0/5;
            theta1 += step;
        }
    }

    gettimeofday(&end, NULL);  // Record the ending time
    double elapsed_time;
    elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Elapsed time: %f seconds\n", elapsed_time);

    printf("th1: %f, duration: %f, arrival: %f\n", rad2deg(theta1), dt/(24*60*60), dt/(24*60*60)+7259);
    printf("a: %f, e: %f, th2: %f\n", a/1e9, e, rad2deg(theta2));
    struct Transfer2D transfer = {constr_orbit(a, e, 0, 0, 0, SUN()), theta1, theta2};
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

void calc_3d_orbit(struct Transfer2D transfer2d, struct Vector r1, struct Vector v1, struct Vector r2, struct Vector v2) {
    struct Vector origin = {0, 0, 0};
    struct Plane p_T = constr_plane(origin, r1, r2);
    struct Plane p_1 = constr_plane(r1, v1, scalar_multipl(r1, -1));
    struct Plane p_2 = constr_plane(r2, v2, scalar_multipl(r2, -1));
    struct Orbit orbit2d = transfer2d.orbit;
    double a = orbit2d.a;
    double e = orbit2d.e;
    double theta1 = transfer2d.theta1;
    double theta2 = transfer2d.theta2;
    double arg_peri = theta1-M_PI;

    double gamma1 = e*sin(theta1)/(1+e*cos(theta1));
    double gamma2 = e*sin(theta2)/(1+e*cos(theta2));

    double v_t1_mag = calc_orbital_speed(orbit2d, vector_mag(r1));
    double v_t2_mag = calc_orbital_speed(orbit2d, vector_mag(r2));

    struct Vector2D v_t1_2d = calc_v_2d(vector_mag(r1), v_t1_mag, theta1, gamma1);
    struct Vector2D v_t2_2d = calc_v_2d(vector_mag(r2), v_t2_mag, theta2, gamma2);

    v_t1_2d = rotate_vector2d(v_t1_2d, arg_peri);
    v_t2_2d = rotate_vector2d(v_t2_2d, arg_peri);
    struct Vector2D temp = {0, 1};

    double a1 = rad2deg(angle_vec_vec_2d(temp, v_t1_2d));
    double a2 = rad2deg(gamma1);
    printf("angle: %f, gamma: %f, diff: %f\n", a1, a2, fabs(a2)- fabs(a1));
    print_vector2d(v_t1_2d);


    temp.x = -r2.y;
    temp.y = r2.x;

    a1 = rad2deg(angle_vec_vec_2d(temp, v_t2_2d));
    a2 = rad2deg(gamma2);
    printf("angle: %f, gamma: %f, diff: %f\n", a1, a2, fabs(a2)- fabs(a1));
    print_vector2d(v_t2_2d);

}