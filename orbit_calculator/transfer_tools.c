#include "transfer_tools.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

struct Transfer2D calc_extreme_hyperbola(double r1, double r2, double target_dt, double dtheta, struct Body *attractor);

struct Transfer2D calc_2d_transfer_orbit(double r1, double r2, double target_dt, double dtheta, struct Body *attractor) {
    double theta1 = r1 > r2 ? deg2rad(180) : 0;
    double theta2 = theta1+dtheta;
    double mu = attractor->mu;
    double step = 0;
    double dt = 1e20;
    double a, e;

    if((r1/r2 > 1.05 || r2/r1 > 1.05) && dtheta > 2*M_PI - deg2rad(1)) return calc_extreme_hyperbola(r1, r2, target_dt, dtheta, attractor);

    int c = 0;

    while(fabs(dt-target_dt) > 1e-3) {

        if(c >= 1000) {
            if(fabs(dt-target_dt) < 100) {
                theta1 += step; // will be subtracted again after the loop
                break;
            }
            printf("\n");
            printf("%f, %f, %f", r1, r2, rad2deg(dtheta));
            printf("\n%f, %f, %f, %f, %f", target_dt/(24*60*60), dt/(24*60*60), fabs(target_dt-dt), rad2deg(theta1), rad2deg(step));
            fprintf(stderr, "\nMore than 1000 calculations tried (transfer orbit calculation)\n");
            exit(EXIT_FAILURE);
        }
        c++;

        theta1 = pi_norm(theta1);
        theta2 = pi_norm(theta1 + dtheta);
        e = (r2-r1)/(r1*cos(theta1)-r2*cos(theta2));// break endless loops
//        printf("(%f, %f, %f, %f) \n", rad2deg(theta1), rad2deg(theta2), e, rad2deg(step));
        if(e < 0){  // not possible
            if(step == 0) theta1 += deg2rad(1);
            else {
                theta1 -= step;
                step /= 4;
            }
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
            // is theta2 reachable?
            if((theta1 < M_PI  &&  theta2 > M_PI) ||
              ((theta1 < M_PI) == (theta2 < M_PI) && (theta1 > theta2))){
                if(step==0) {
                    // relevant for when looking for hyperbola close to the sun
                    if(fabs(theta1 - M_PI) < deg2rad(2) && dtheta > deg2rad(358)) theta1 += fabs(2*M_PI-dtheta)/100;
                    else theta1 += deg2rad(1);
                    continue;
                }
                theta1 -= step;
                step /= 4;
                continue;
            }

            double F1 = acosh((e + cos(theta1)) / (1 + e * cos(theta1)));
            t1 = (e * sinh(F1) - F1) / n;
            double F2 = acosh((e + cos(theta2)) / (1 + e * cos(theta2)));
            t2 = (e * sinh(F2) - F2) / n;
            // different quadrant
            if((theta1 < M_PI) != (theta2 < M_PI)) dt = t1+t2;
            // past periapsis
            else if(theta1 < M_PI) dt = t2-t1;
            // before periapsis
            else dt = t1-t2;
        }
//        printf(",%f", rad2deg(theta1));
//        printf("(%f, %f, %f)", dt/(24*60*60), target_dt/(24*60*60), rad2deg(step));
//        printf("(%f, %f, %f, %f, %f, %f, %f)\n", rad2deg(theta1), rad2deg(theta2), rad2deg(dtheta), t1/(24*60*60), t2/(24*60*60), (dt-target_dt)/(24*60*60), e);


        if(isnan(dt)){  // at this theta1 orbit not solvable
            if(step == 0) {
                theta1 += deg2rad(1);
                dt = 100;   // to not exit the loop
                continue;
            }
            theta1 -= step;
            step /= 4;
            dt = 100;   // to not exit the loop
            continue;
        }

        if(step != 0) {
            if ((dt - target_dt) * (r1 - r2) > 0) {
                if (step < 0) step *= -1.0 / 4;
            } else {
                if (step > 0) step *= -1.0 / 4;
            }
        } else {
            if ((dt - target_dt) * (r1 - r2) > 0) {
                step = deg2rad(1);
            } else {
                step = deg2rad(-1);
            }
        }
        theta1 += step;
    }




    theta1 -= step; // reset theta1 from last change inside the loop
//    printf("OUT (%f, %f, %f, %f, %f, %f)\n", rad2deg(theta1), rad2deg(theta2), rad2deg(dtheta), dt/(24*60*60), e, a*1e-9);
//    printf("\n");

//    printf(",%f", e);

    struct Transfer2D transfer = {constr_orbit(a, e, 0, 0, 0, SUN()), theta1, theta2};
    return transfer;
}

struct Transfer2D calc_extreme_hyperbola(double r1, double r2, double target_dt, double dtheta, struct Body *attractor) {
    double theta1 = deg2rad(180);
    double theta2 = theta1+dtheta;
    double mu = attractor->mu;
    double step = deg2rad(1);
    double dt = 1e20;
    double a, e;
    int c = 0;

    while(fabs(dt-target_dt) > 1e-3) {
        if(c >= 200) {
            if(fabs(dt-target_dt) < 100) {
                theta1 += step; // will be subtracted again after the loop
                break;
            }
            printf("\n");
            printf("%f, %f, %f", r1, r2, rad2deg(dtheta));
            printf("\n%f, %f, %f, %f, %f", target_dt/(24*60*60), dt/(24*60*60), fabs(target_dt-dt), rad2deg(theta1), rad2deg(step));
            fprintf(stderr, "\nMore than 200 calculations tried (extreme hyperbola)\n");
            exit(EXIT_FAILURE);
        }
        c++;
        theta1 = pi_norm(theta1);
        theta2 = pi_norm(theta1 + dtheta);
        e = (r2-r1)/(r1*cos(theta1)-r2*cos(theta2));// break endless loops

//        printf("(%f, %f, %f, %f) ", rad2deg(theta1), rad2deg(theta2), e, rad2deg(step));

//        printf("(%f, %f, %f, %f) \n", rad2deg(theta1), rad2deg(theta2), e, rad2deg(step));
        if(e < 0){  // not possible
            if(theta2 > M_PI) {
                theta1 = M_PI;
                step /= 4;
            } else theta1 += step;
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
            if(theta1 < M_PI) {
                theta1 = M_PI;
                step *= -1;
                continue;
            }
            if(theta2 >= M_PI) {
                theta1 = M_PI;
                step /= 4;
                continue;
            }
            double F1 = acosh((e + cos(theta1)) / (1 + e * cos(theta1)));
            t1 = (e * sinh(F1) - F1) / n;
            double F2 = acosh((e + cos(theta2)) / (1 + e * cos(theta2)));
            t2 = (e * sinh(F2) - F2) / n;
            // different quadrant
            if((theta1 < M_PI) != (theta2 < M_PI)) dt = t1+t2;
                // past periapsis
            else if(theta1 < M_PI) dt = t2-t1;
                // before periapsis
            else dt = t1-t2;
        }
//        printf(",%f", rad2deg(theta1));
//        printf("(%f, %f, %f)", dt/(24*60*60), target_dt/(24*60*60), rad2deg(step));
//        printf("(%f, %f, %f, %g, %f, %f) ", rad2deg(theta1), rad2deg(theta2), rad2deg(dtheta), rad2deg(step), dt-target_dt, e);


        if(isnan(dt)){  // at this theta1 orbit not solvable
            theta1 += step;
            dt = 100;   // to not exit the loop
            continue;
        }

        if(step != 0) {
            if ((dt - target_dt) * (r1 - r2) > 0) {
                if (step < 0) step *= -1.0 / 4;
            } else {
                if (step > 0) step *= -1.0 / 4;
            }
        }
        theta1 += step;
    }

    theta1 -= step; // reset theta1 from last change inside the loop
//    printf("OUT (%f, %f, %f, %f, %f, %f)\n", rad2deg(theta1), rad2deg(theta2), rad2deg(dtheta), dt/(24*60*60), e, a*1e-9);
//    printf("\n");

//    printf(",%f", e);

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


struct OSV propagate_orbit(struct Vector r, struct Vector v, double dt, struct Body *attractor) {
    double r_mag = vector_mag(r);
    double v_mag = vector_mag(v);
    double v_r = dot_product(v,r) / r_mag;
    double mu = attractor->mu;

    double a = 1 / (2/r_mag - pow(v_mag,2)/mu);
    struct Vector h = cross_product(r,v);
    struct Vector e = scalar_multiply(add_vectors(cross_product(v,h), scalar_multiply(r, -mu/r_mag)), 1/mu);
    double e_mag = vector_mag(e);

    struct Vector k = {0,0,1};
    struct Vector n_vec = cross_product(k, h);
    struct Vector n_norm = norm_vector(n_vec);
    double RAAN, i, arg_peri;
    if(vector_mag(n_vec) != 0) {
        RAAN = n_norm.y >= 0 ? acos(n_norm.x) : 2 * M_PI - acos(n_norm.x); // if n_norm.y is negative: RAAN > 180°
        i = acos(dot_product(k, norm_vector(h)));
        arg_peri = e.z >= 0 ? acos(dot_product(n_norm, e) / e_mag) : 2 * M_PI - acos(dot_product(n_norm, e) / e_mag);  // if r.z is positive: w > 180°
    } else {
        RAAN = 0;
        i = dot_product(k, norm_vector(h)) > 0 ? 0 : M_PI;
        arg_peri = cross_product(r,v).z * e.y > 0 ? acos(e.x/e_mag) : 2*M_PI - acos(e.x/e_mag);
    }
    double theta = v_r >= 0 ? acos(dot_product(e,r) / (e_mag*r_mag)) : 2*M_PI - acos(dot_product(e,r) / (e_mag*r_mag));
    double E = 2 * atan(sqrt((1-e_mag)/(1+e_mag)) * tan(theta/2));
    double t = (E-e_mag*sin(E)) / sqrt(mu/ pow(a,3));
    double n = sqrt(mu / pow(fabs(a),3));
    double T = 2*M_PI/n;
    if(t < 0) t += T;

    double target_t = t+dt;
    double step = deg2rad(5);
    theta += step;

    while(target_t > T) target_t -= T;

    int c = 0;

    while(fabs(t-target_t) > 1e-3) {
        c++;
        theta = pi_norm(theta);
        E = acos((e_mag + cos(theta)) / (1 + e_mag * cos(theta)));
        t = (E - e_mag * sin(E)) / n;
        if(theta > M_PI) t = T-t;

        // prevent endless loops (floating point imprecision can lead to not changing values for very small steps)
        if(c == 500) break;

        if((target_t-t) > 0) {
            if(step < 0) step *= -1.0/4;
            while(theta+step > 2*M_PI) step *= 1.0/4;
            theta += step;
        } else {
            while(theta+step < 0) step *= 1.0/4;
            if(step > 0) step *= -1.0/4;
            theta += step;
        }
    }
    theta -= step; // reset theta1 from last change inside the loop

    double gamma = atan(e_mag*sin(theta)/(1+e_mag*cos(theta)));
    r_mag = a*(1-pow(e_mag,2)) / (1+e_mag*cos(theta));
    v_mag = sqrt(mu*(2/r_mag - 1/a));
    struct Vector2D r_2d = {cos(theta) * r_mag, sin(theta) * r_mag};
    struct Vector2D v_2d = calc_v_2d(r_mag, v_mag, theta, gamma);

    r = heliocentric_rot(r_2d, RAAN, arg_peri, i);
    v = heliocentric_rot(v_2d, RAAN, arg_peri, i);

    struct OSV osv = {r, v};
    return osv;
}