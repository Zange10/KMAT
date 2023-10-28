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
    /*struct Vector loc0 = {-1,2,0};
    struct Vector r = {1, 1, 1};
    struct Vector s = {2,0,-1};
    struct Vector loc1 = {2,1,-2};
    struct Vector t = {0,-1,3};
    struct Vector u = {2,2,0};

    struct Plane p1 = constr_plane(loc0, r,s);
    struct Plane p2 = constr_plane(loc1, t,u);

    struct Vector v = calc_intersecting_line_dir(p1, p2);

    //print_vector(v);

    return;*/

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
    struct Vector r1 = {100e9, 180e9, -100e9};
    struct Vector v1 = {-20000, 15000, 2000};
    struct Vector r2 = {-374e9, -179e9, -120e9};
    struct Vector v2 = {8000, -13000, 3000};

    double dt = 270*(24*60*60);
    double dtheta = angle_vec_vec(r1, r2);
    struct Transfer2D transfer2d = calc_2d_transfer_orbit(vector_mag(r1), vector_mag(r2), dt, dtheta, SUN());
    double dv = calc_transfer_dv(transfer2d, r1, v1, r2, v2);
    // printf("dv: %f m/s - dtheta: %f°\n", dv, rad2deg(dtheta));

    gettimeofday(&end, NULL);  // Record the ending time
    double elapsed_time;
    elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    //printf("Elapsed time: %f seconds\n", elapsed_time);
}

struct Transfer2D calc_2d_transfer_orbit(double r1, double r2, double target_dt, double dtheta, struct Body *attractor) {
    double theta1 = r1 > r2 ? deg2rad(180) : 0;
    double theta2 = theta1+dtheta;
    double mu = attractor->mu;
    double step = -deg2rad(10);
    double dt = 1e20;
    double a, e;

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

    printf("r: %f, %f, %f\n", r1/1e9, cos(theta1)*r1/1e9, sin(theta1)*r1/1e9);
    printf("r: %f, %f, %f\n", r2/1e9, cos(theta2)*r2/1e9, sin(theta2)*r2/1e9);

    printf("Theta1: %f°, Theta2: %f°\n", rad2deg(theta1), rad2deg(theta2));

    struct Transfer2D transfer = {constr_orbit(a, e, 0, 0, 0, SUN()), theta1, theta2};
    printf("a: %f, e: %f\n",a,e);
    return transfer;
}

struct Vector2D calc_v_2d(double r_mag, double v_mag, double theta, double gamma) {
    struct Vector2D r_norm = {cos(theta), sin(theta)};
    struct Vector2D r = scalar_multipl2d(r_norm, r_mag);
    struct Vector2D n = {-r.y, r.x};
    struct Vector2D v = rotate_vector2d(n, gamma);
    v = scalar_multipl2d(v, v_mag/ vector2d_mag(v));

    return v;
}

void printMatrix(double M[3][3]) {
    printf("\n");
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            printf("%.4f\t", M[i][j]); // Adjust the format specifier as needed
        }
        printf("\n"); // Move to the next row
    }
    printf("\n");
}

struct Vector heliocentric_rot(struct Vector2D v, double RAAN, double w, double inc) {
    double Q[3][3] = {
            {-sin(RAAN)*cos(inc)*sin(w)+cos(RAAN)*cos(w),     -sin(RAAN)*cos(inc)*cos(w)-cos(RAAN)*sin(w),   sin(RAAN)*sin(inc)},
            { cos(RAAN)*cos(inc)*sin(w)+sin(RAAN)*cos(w),      cos(RAAN)*cos(inc)*cos(w)-sin(RAAN)*sin(w),  -cos(RAAN)*sin(inc)},
            { sin(inc)*sin(w),                                 sin(inc)*cos(w),                              cos(inc)}};

    printMatrix(Q);

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
    struct Orbit orbit2d = transfer2d.orbit;
    double e = orbit2d.e;
    double theta1 = transfer2d.theta1;
    double theta2 = transfer2d.theta2;
    double argument_orbit = theta1 - M_PI;

    double gamma1 = atan(e*sin(theta1)/(1+e*cos(theta1)));
    double gamma2 = atan(e*sin(theta2)/(1+e*cos(theta2)));
    printf("theta2: %f, gamma2: %f\n", rad2deg(theta2), rad2deg(gamma2));

    double v_t1_mag = calc_orbital_speed(orbit2d, vector_mag(r1));
    double v_t2_mag = calc_orbital_speed(orbit2d, vector_mag(r2));

    struct Vector2D v_t1_2d = calc_v_2d(vector_mag(r1), v_t1_mag, theta1, gamma1);
    struct Vector2D v_t2_2d = calc_v_2d(vector_mag(r2), v_t2_mag, theta2, gamma2);

    print_vector2d(v_t1_2d);
    print_vector2d(v_t2_2d);

    // calculate RAAN, inclination and argument of periapsis
    struct Vector inters_line = calc_intersecting_line_dir(p_0, p_T);
    if(inters_line.y < 0) inters_line = scalar_multiply(inters_line, -1); // for rotation of RAAN in clock-wise direction
    struct Vector in_plane_up = cross_product(inters_line, calc_plane_norm_vector(p_T));    // 90° to intersecting line and norm vector of plane
    if(in_plane_up.z < 0) in_plane_up = scalar_multiply(in_plane_up, -1);   // this vector is always 90° before RAAN for prograde orbits
    double RAAN = in_plane_up.x < 0 ? angle_vec_vec(vec(1,0,0), inters_line) : angle_vec_vec(vec(1,0,0), inters_line) + M_PI;   // RAAN 90° behind in_plane_up

    double i = angle_plane_plane(p_T, p_0);   // also possible to get angle between p_0 and in_plane_up

    double arg_peri = 2*M_PI - theta1;
    if(RAAN < M_PI) {
        if(r1.z > 0) arg_peri += angle_vec_vec(inters_line, r1);
        else arg_peri += 2*M_PI - angle_vec_vec(inters_line, r1);
    } else {
        if(r1.z < 0) arg_peri += angle_vec_vec(inters_line, r1)+M_PI;
        else arg_peri += M_PI - angle_vec_vec(inters_line, r1);
    }


    struct Vector v_t1 = heliocentric_rot(v_t1_2d, RAAN, arg_peri, i);
    struct Vector v_t2 = heliocentric_rot(v_t2_2d, RAAN, arg_peri, i);
    printf("RAAN: %f, arg_peri: %f, i: %f\n", rad2deg(RAAN), rad2deg(arg_peri), rad2deg(i));

    double v_t1_inf = fabs(vector_mag(add_vectors(v_t1, scalar_multiply(v1, -1))));
    double dv1 = dv_circ(EARTH(), 150e3, v_t1_inf);
    double v_t2_inf = fabs(vector_mag(add_vectors(v_t2, scalar_multiply(v2, -1))));
    double dv2 = dv_capture(VENUS(), 250e3, v_t2_inf);

    print_vector(scalar_multiply(v_t1,1e-3));
    print_vector(scalar_multiply(v_t2,1e-3));

    printf("%f°\n", rad2deg(angle_plane_vec(p_T, v_t1)));
    //print_vector(norm_vector(v_t1));
    //print_vector(norm_vector(v_t2));
    // printf("%f, %f", v_t1_inf, v_t2_inf);

    // print_vector(scalar_multiply(v_t1,1e-3));
    // print_vector(scalar_multiply(v_t2,1e-3));

    return dv1+dv2;
}