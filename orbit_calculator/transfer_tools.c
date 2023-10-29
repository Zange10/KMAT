//
// Created by niklas on 29.10.23.
//

#include "transfer_tools.h"
#include <math.h>

struct Transfer2D calc_2d_transfer_orbit(double r1, double r2, double target_dt, double dtheta, struct Body *attractor) {
    double theta1 = r1 > r2 ? deg2rad(180) : 0;
    double theta2 = theta1+dtheta;
    double mu = attractor->mu;
    double step = -deg2rad(10);
    double dt = 1e20;
    double a, e;

    while(fabs(dt-target_dt) > 1e-3) {
        theta1 = pi_norm(theta1);
        theta2 = pi_norm(theta1 + dtheta);
        e = (r2-r1)/(r1*cos(theta1)-r2*cos(theta2));
        if(e < 0){  // not possible
            theta1 -= step;
            step /= 4;
            continue;
        }
        double rp = r1*(1+e*cos(theta1))/(1+e);
        a = rp/(1-e);
        double n = sqrt(mu / pow(fabs(a),3));

        double t1,t2;
        double T = 2*M_PI/n;

        if(e < 1) {
            double E1 = acos((e + cos(theta1)) / (1 + e * cos(theta1)));
            t1 = (E1 - e * sin(E1)) / n;
            if(theta1 > M_PI) t1 = T-t1;
            double E2 = acos((e + cos(theta2)) / (1 + e * cos(theta2)));
            t2 = (E2 - e * sin(E2)) / n;
            if(theta2 > M_PI) t2 = T-t2;
            dt = theta1 < theta2 ? t2-t1 : T-t1 + t2;
        } else {
            double F1 = acosh((e + cos(theta1)) / (1 + e * cos(theta1)));
            t1 = (e * sinh(F1) - F1) / n;
            double F2 = acosh((e + cos(theta2)) / (1 + e * cos(theta2)));
            t2 = (e * sinh(F2) - F2) / n;
            dt = theta1 < theta2 ? t2-t1 : t1 + t2;
        }

        if(isnan(dt)){  // at this theta1 orbit not solvable
            theta1 -= step;
            step /= 4;
            dt = 100;
            continue;
        }


        if((dt-target_dt)*(r1-r2) > 0) {
            if(step < 0) step *= -1.0/4;
            theta1 += step;
        } else {
            if(step > 0) step *= -1.0/4;
            theta1 += step;
        }
    }

    theta1 -= step; // reset theta1 from last change inside the loop

    struct Transfer2D transfer = {constr_orbit(a, e, 0, 0, 0, SUN()), theta1, theta2};
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

struct Vector heliocentric_rot(struct Vector2D v, double RAAN, double w, double inc) {
    double Q[3][3] = {
            {-sin(RAAN)*cos(inc)*sin(w)+cos(RAAN)*cos(w),     -sin(RAAN)*cos(inc)*cos(w)-cos(RAAN)*sin(w),   sin(RAAN)*sin(inc)},
            { cos(RAAN)*cos(inc)*sin(w)+sin(RAAN)*cos(w),      cos(RAAN)*cos(inc)*cos(w)-sin(RAAN)*sin(w),  -cos(RAAN)*sin(inc)},
            { sin(inc)*sin(w),                                 sin(inc)*cos(w),                              cos(inc)}};

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

struct Transfer calc_transfer_dv(struct Transfer2D transfer2d, struct Vector r1, struct Vector r2) {
    struct Vector origin = {0, 0, 0};
    struct Plane p_0 = constr_plane(origin, vec(1,0,0), vec(0,1,0));
    struct Plane p_T = constr_plane(origin, r1, r2);
    struct Orbit orbit2d = transfer2d.orbit;
    double e = orbit2d.e;
    double theta1 = transfer2d.theta1;
    double theta2 = transfer2d.theta2;

    double gamma1 = atan(e*sin(theta1)/(1+e*cos(theta1)));
    double gamma2 = atan(e*sin(theta2)/(1+e*cos(theta2)));

    double v_t1_mag = calc_orbital_speed(orbit2d, vector_mag(r1));
    double v_t2_mag = calc_orbital_speed(orbit2d, vector_mag(r2));

    struct Vector2D v_t1_2d = calc_v_2d(vector_mag(r1), v_t1_mag, theta1, gamma1);
    struct Vector2D v_t2_2d = calc_v_2d(vector_mag(r2), v_t2_mag, theta2, gamma2);

    // calculate RAAN, inclination and argument of periapsis
    struct Vector inters_line = calc_intersecting_line_dir(p_0, p_T);
    if(inters_line.y < 0) inters_line = scalar_multiply(inters_line, -1); // for rotation of RAAN in clock-wise direction
    struct Vector in_plane_up = cross_product(inters_line, calc_plane_norm_vector(p_T));    // 90° to intersecting line and norm vector of plane
    if(in_plane_up.z < 0) in_plane_up = scalar_multiply(in_plane_up, -1);   // this vector is always 90° before RAAN for prograde orbits
    double RAAN = in_plane_up.x <= 0 ? angle_vec_vec(vec(1,0,0), inters_line) : angle_vec_vec(vec(1,0,0), inters_line) + M_PI;   // RAAN 90° behind in_plane_up

    //double i = angle_plane_plane(p_T, p_0);   // can create angles greater than 90°
    double i = angle_plane_vec(p_0, in_plane_up);   // also possible to get angle between p_0 and in_plane_up

    double arg_peri = 2*M_PI - theta1;
    if(RAAN < M_PI) {
        if(r1.z >= 0) arg_peri += angle_vec_vec(inters_line, r1);
        else arg_peri += 2*M_PI - angle_vec_vec(inters_line, r1);
    } else {
        if(r1.z <= 0) arg_peri += angle_vec_vec(inters_line, r1)+M_PI;
        else arg_peri += M_PI - angle_vec_vec(inters_line, r1);
    }

    struct Vector v_t1 = heliocentric_rot(v_t1_2d, RAAN, arg_peri, i);
    struct Vector v_t2 = heliocentric_rot(v_t2_2d, RAAN, arg_peri, i);

    struct Transfer transfer = {r1, v_t1, r2, v_t2};

    return transfer;
}