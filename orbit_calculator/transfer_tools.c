#include "transfer_tools.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

struct Transfer2D calc_extreme_hyperbola(double r1, double r2, double target_dt, double dtheta, struct Body *attractor);

struct Transfer2D calc_2d_transfer_orbit(double r1, double r2, double target_dt, double dtheta, struct Body *attractor) {
    double theta1 = r1 > r2 ? deg2rad(180) : 0;
    double theta2 = theta1+dtheta;
    double mu = attractor->mu;

    double initial_step = deg2rad(20);
    double step = 0;
    double dt = 1e20;
    double a, e;

    int c = 0;

    while(fabs(dt-target_dt) > 1) {

        if(c >= 1000) {
            if(fabs(dt-target_dt) < 86400 && fabs(step) < 1e-10) {
                theta1 += step; // will be subtracted again after the loop
                break;
            }
            return calc_extreme_hyperbola(r1, r2, target_dt, dtheta, attractor);
        }
        c++;

        theta1 = pi_norm(theta1);
        theta2 = pi_norm(theta1 + dtheta);
        e = (r2-r1)/(r1*cos(theta1)-r2*cos(theta2));// break endless loops
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
                step = initial_step;
            } else {
                step = -initial_step;
            }
        }
        theta1 += step;
    }

    theta1 -= step; // reset theta1 from last change inside the loop

    struct Transfer2D transfer = {constr_orbit(a, e, 0, 0, 0, SUN()), theta1, theta2};
    return transfer;
}

struct Transfer2D calc_extreme_hyperbola(double r1, double r2, double target_dt, double dtheta, struct Body *attractor) {
    double theta1 = r1>r2 ? deg2rad(180) : deg2rad(180)-dtheta;
    double theta2 = r1>r2 ? theta1+dtheta: deg2rad(180);
    double mu = attractor->mu;
    double step = deg2rad(1);
    double dt = 1e20;
    double a, e;
    int c = 0;

    while(fabs(dt-target_dt) > 1) {
        if(c >= 1000) {
            if(fabs(dt-target_dt) < 86400 && fabs(step) < 1e-10) {
                theta1 += step; // will be subtracted again after the loop
                break;
            }
            printf("\n");
            printf("%f, %f, %f", r1, r2, rad2deg(dtheta));
            printf("\n%f, %f, %f, %f, %f", target_dt/(24*60*60), dt/(24*60*60), fabs(target_dt-dt), rad2deg(theta1), rad2deg(step));
            fprintf(stderr, "\nMore than 1000 calculations tried (extreme hyperbola)\n");
            //exit(EXIT_FAILURE);
            break;
        }

        c++;
        theta1 = pi_norm(theta1);
        theta2 = pi_norm(theta1 + dtheta);
        e = (r2-r1)/(r1*cos(theta1)-r2*cos(theta2));

        if(e < 0){  // not possible
            if(theta2 > M_PI) {
                theta1 = r1>r2 ? M_PI : M_PI - dtheta;
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
                theta1 = r1>r2 ? M_PI : M_PI - dtheta;
                step *= -1;
                continue;
            }
            if(theta2 >= M_PI) {
                theta1 = r1>r2 ? M_PI : M_PI - dtheta;
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

    struct Transfer2D transfer = {constr_orbit(a, e, 0, 0, 0, SUN()), theta1, theta2};
    return transfer;
}

struct Transfer calc_transfer(enum Transfer_Type tt, struct Body *dep_body, struct Body *arr_body, struct Vector r1, struct Vector v1, struct Vector r2, struct Vector v2, double dt, double *data) {
    double dtheta = angle_vec_vec(r1, r2);
    if (cross_product(r1, r2).z < 0) dtheta = 2 * M_PI - dtheta;

    struct Transfer2D transfer2d = calc_2d_transfer_orbit(vector_mag(r1), vector_mag(r2), dt, dtheta, SUN());
    struct Transfer transfer = calc_transfer_dv(transfer2d, r1, r2);

    if(data != NULL) {
        double v_t1_inf = fabs(vector_mag(add_vectors(transfer.v0, scalar_multiply(v1, -1))));
        double dv1 = tt % 2 == 0 ? dv_capture(dep_body, dep_body->atmo_alt+100e3, v_t1_inf) : dv_circ(dep_body, dep_body->atmo_alt+100e3, v_t1_inf);

        double v_t2_inf = fabs(vector_mag(add_vectors(transfer.v1, scalar_multiply(v2, -1))));
        double dv2;
        if(tt < 2)      dv2 = dv_capture(arr_body, arr_body->atmo_alt+100e3, v_t2_inf);
        else if(tt < 4) dv2 = dv_circ(arr_body, arr_body->atmo_alt+100e3, v_t2_inf);
        else            dv2 = 0;

        data[0] = dt / (24 * 60 * 60);
        data[1] = dv1;
        data[2] = dv2;
    }
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

struct Swingby_Peak_Search_Params {
    double peak_dur;
    double transfer_dur;
    double T;
    double *interval;
    double *min;
    double **xs;
    double *angles;
    int **ints;
    struct Body *body;
    struct OSV *osvs;
    struct Vector v_t00;
};

double test[2];



int find_double_swing_by_zero_sec_sb_diff(struct Swingby_Peak_Search_Params spsp, int only_right_leg) {
    struct timeval start, end;
    double elapsed_time;
    double **xs = spsp.xs;
    double *min = spsp.min;
    double peak_dur = spsp.peak_dur;
    double transfer_duration = spsp.transfer_dur;
    double T = spsp.T;
    int **ints = spsp.ints;
    struct Body *body = spsp.body;
    struct Vector v_t00 = spsp.v_t00;

    struct OSV p0 = spsp.osvs[0];
    struct OSV s0 = spsp.osvs[1];
    struct OSV p1 = spsp.osvs[2];
    struct OSV s1 = spsp.osvs[3];

    double angle = spsp.angles[0];
    double theta = spsp.angles[1];
    double phi = spsp.angles[2];
    double defl = spsp.angles[3];

    double *x1 = xs[0];
    double *x2 = xs[1];
    double *x3 = xs[2];
    double *x4 = xs[3];
    double *x5 = xs[4];
    double *x6 = xs[5];
    double *y = xs[6];
    double *y1 = xs[7];
    int *c = ints[0];
    int *u = ints[1];
    int *g = ints[2];
    double min_dur = spsp.interval[0];
    double max_dur = spsp.interval[1];


    double mu = body->orbit.body->mu;
    int is_edge = 0;
    int right_leg_only_positive = 1;
    for(int i = -1 + only_right_leg*2; i <= 1; i+=2) {
        double step = i*86400.0 * 10;
        double dur = peak_dur+step;
        int temp_counter = 0;
        if(peak_dur < min_dur) {
            i+=2;
            if(i>1) break;
            dur = min_dur;
            is_edge = 1;
        }
        if(peak_dur > max_dur) {
            if(i==1) break;
            dur = max_dur;
            is_edge = 1;
        }
        double diff_vinf = 1e9;
        double last_diff_vinf = 1e9;
        //printf("DUR: %6.2f step: %f i:%d    min_dur: %f    max_dur: %f ----------------------------\n", peak_dur/86400, max_dur/86400, i, min_dur/86400, max_dur/86400);
        while (fabs(diff_vinf) > 10) {
            //printf("\n");
            temp_counter++;
            if(temp_counter > 100) {
                break; // break endless loop
            }
            //if (dur > max_dur) break;
            gettimeofday(&start, NULL);  // Record the starting time
            struct OSV osv_m0 = propagate_orbit(p0.r, v_t00, dur, SUN());
            gettimeofday(&end, NULL);  // Record the ending time
            elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
            //printf("%6.4f", elapsed_time);
            test[0] += elapsed_time;
            if(rad2deg(angle_vec_vec(osv_m0.r, p1.r)) < 0.5) {
                dur += step;
                step += step;
                continue;
            }
            gettimeofday(&start, NULL);  // Record the starting time

            struct Transfer transfer = calc_transfer(capfb, body, body, osv_m0.r, osv_m0.v, p1.r, p1.v,
                                                     transfer_duration * 86400 - dur, NULL);
            gettimeofday(&end, NULL);  // Record the ending time
            elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
            //printf(" %6.4f", elapsed_time);
            test[1] += elapsed_time;
            struct Vector temp = add_vectors(transfer.v0, scalar_multiply(osv_m0.v, -1));
            double mag = vector_mag(temp);

            *c += 1;
            struct Vector temp1 = add_vectors(transfer.v1, scalar_multiply(p1.v, -1));
            struct Vector temp2 = add_vectors(s1.v, scalar_multiply(p1.v, -1));

            diff_vinf = vector_mag(temp1) - vector_mag(temp2);
            //printf("dur: %f, step: %f, diff_vinf: %f\n", dur/86400, step, diff_vinf);
            if(peak_dur < 0 && dur == min_dur && diff_vinf < 0) return 0;
            if(peak_dur > max_dur && dur == max_dur && diff_vinf < 0) return 1;
            if (fabs(diff_vinf) < 10) {
                double a_temp = 1 / (2 / vector_mag(s1.r) - pow(vector_mag(s1.v), 2) / mu);
                double beta = (M_PI - angle_vec_vec(temp1, temp2)) / 2;
                double rp = (1 / cos(beta) - 1) * (body->mu / (pow(vector_mag(temp1), 2)));
                if (rp > body->radius + body->atmo_alt) {
                    //printf("theta: %6.2f°, phi: %6.2f°, deflection: %5.2f°, beta: %5.2f°, dur: %6.2f, angle: %5.2f°, ", rad2deg(theta), rad2deg(phi), rad2deg(defl), rad2deg(beta), dur/86400, rad2deg(angle));
                    //printf("T: %3dd, dv: %8.2f, Periapsis: %10.3fkm\n", (int)(T/86400), mag, (rp-body->radius)/1000);

                    x1[*u] = T / 86400;
                    x2[*u] = dur / 86400;
                    x3[*u] = rad2deg(theta);
                    x4[*u] = rad2deg(phi);
                    x5[*u] = rad2deg(angle);
                    y[*u] = mag;
                    *u += 1;
                    if (mag < min[0]) {
                        min[0] = mag;
                        min[1] = dur;
                        //printf("\n---\ntheta: %6.2f°, phi: %6.2f°, deflection: %5.2f°, beta: %5.2f°, dur: %6.2f, angle: %5.2f°, ",
                        //       rad2deg(theta), rad2deg(phi), rad2deg(defl), rad2deg(beta), dur / 86400, rad2deg(angle));
                        //printf("T: %3dd, dv: %8.2f, Periapsis: %10.3fkm\n", (int) (T / 86400), mag,
                        //       (rp - body->radius) / 1000);
                        //print_vector(temp);
                        break;
                    }
                }
            }

            double gradient = (diff_vinf - last_diff_vinf)/step;

            if(diff_vinf > 0) {
                double max_rel_T_pos = T;
                if(dur+T > max_dur) max_rel_T_pos = max_dur-dur;
                if(i == -1 && max_rel_T_pos > peak_dur) max_rel_T_pos = peak_dur;
                double max_rel_T_neg = -T;
                if(dur-T < 0) max_rel_T_neg = -dur;
                if(i == 1 && max_rel_T_neg < peak_dur) max_rel_T_neg = peak_dur;
                //printf("max_rel(+): %f, max_rel(-): %f, gradient: %f, gradT(+): %f, gradT(-): %f\n", max_rel_T_pos/86400, max_rel_T_neg/86400, gradient, diff_vinf+gradient*max_rel_T_pos, diff_vinf+gradient*max_rel_T_neg);
                if(diff_vinf+gradient*max_rel_T_pos > 0 && diff_vinf+gradient*max_rel_T_neg > 0 && (fabs(step) < 86400 || is_edge)) {
                    //printf("GRADIENT: %f, %f, %f, %f\n", gradient, step, gradient*T, diff_vinf);
                    break;
                }
                if(step*i > 0 == gradient*i > 0 && dur != min_dur && dur != max_dur) {
                    step /= -4;
                }
            } else {
                if(right_leg_only_positive && i == 1) right_leg_only_positive = 0;
                if(step*i > 0 && dur != min_dur && dur != max_dur && dur != peak_dur) {
                    step /= -4;
                }
            }

            dur += step;
            if(dur < min_dur) {
                step /= -4;
                dur = min_dur;
            } else if(dur > max_dur) {
                step /= -4;
                dur = max_dur;
            }
            if(i == 1 && dur < peak_dur && peak_dur > min_dur) {
                step /= -4;
                dur = peak_dur;
            } else if(i == -1 && dur > peak_dur && peak_dur < max_dur) {
                step /= -4;
                dur = peak_dur;
            }
            last_diff_vinf = diff_vinf;
        }
    }
    return right_leg_only_positive;
}

int calc_double_swing_by(struct OSV s0, struct OSV p0, struct OSV s1, struct OSV p1, double transfer_duration, struct Body *body, double **xs, int **ints, int only_viability) {
    double mu = body->orbit.body->mu;
    struct timeval start, end;
    double elapsed_time;


    test[0] = 0;
    test[1] = 0;

    double target_max = 2500;
    double tol_it1 = 4000;
    double tol_it2 = 500;
    double tolerance = 0.05;
    int num_angle_analyse = only_viability ? 10 : 20;

    double min_rp = body->radius+body->atmo_alt;
    double body_T = M_PI*2 * sqrt(pow(body->orbit.a,3)/mu);
    struct Vector v_soi0 = add_vectors(s0.v, scalar_multiply(p0.v,-1));
    double v_inf = vector_mag(v_soi0);
    double min_beta = acos(1 / (1 + (min_rp * pow(v_inf, 2)) / body->mu));
    double max_defl = M_PI - 2*min_beta;
    int negative_true_anomaly = ((transfer_duration*86400)/body_T - (int)((transfer_duration*86400)/body_T)) > 0.5;


    printf("\nmin beta: %.2f°; max deflection: %.2f° (vinf: %f m/s)\n\n", rad2deg(min_beta), rad2deg(max_defl), v_inf);

    double angle_step_size, theta, phi, max_theta, max_phi;
    double angles[3];
    double min_dv, min_man_t, min_theta, min_phi;
    double min_theta_phi[2];

    for(int i = 0; i < 4; i++) {
        gettimeofday(&start, NULL);  // Record the starting time
        min_dv = 1e9;
        int total_counter = 0;
        int partial_counter = 0;
        if (i == 0) {
            angle_step_size = 2 * max_defl / num_angle_analyse;
            theta = -max_defl - angle_step_size;
            max_theta = max_defl;
            max_phi = max_defl;
            min_theta_phi[0] = -max_defl;
        } else {
            angle_step_size = 2 * angles[0] / num_angle_analyse;
            theta = angles[1] - angles[0] - angle_step_size;
            max_theta = angles[1] + angles[0];
            max_phi = angles[2] + angles[0];
            min_theta_phi[0] = angles[1] - angles[0];
        }

        while (theta <= max_theta) {
            theta += angle_step_size;
            struct Vector rot_axis_1 = norm_vector(cross_product(v_soi0, p0.r));
            struct Vector v_soi_ = rotate_vector_around_axis(v_soi0, rot_axis_1, theta);
            phi = i == 0 ? -max_defl - angle_step_size : angles[2] - angles[0] - angle_step_size;
            min_theta_phi[1] = i == 0 ? -max_defl : angles[2] - angles[0];

            while (phi <= max_phi) {
                total_counter++;
                phi += angle_step_size;

                double defl = acos(cos(theta) * sin(M_PI_2 - phi));
                if (defl > max_defl) continue;

                struct Vector rot_axis_2 = norm_vector(cross_product(v_soi_, rot_axis_1));
                struct Vector v_soi = rotate_vector_around_axis(v_soi_, rot_axis_2, phi);

                double angle = angle_vec_vec(v_soi0, v_soi);
                //printf("(%3d°, %3d°, %6.2f°)  -  %.2f°\n", i, j, rad2deg(beta), rad2deg(angle));
                //angle = angle_vec_vec(vt_00_norm, p0.v);
                //printf("%.2f°\n", angle);

                struct Vector v_t00 = add_vectors(v_soi, p0.v);
                struct Plane plane = constr_plane(vec(0, 0, 0), p0.r, p1.r);
                angle = angle_plane_vec(plane, v_t00);

                double a = 1 / (2 / vector_mag(s0.r) - pow(vector_mag(v_t00), 2) / mu);
                double T = M_PI * 2 * sqrt(pow(a, 3) / mu);
                double T_ratio = T / body_T - (int) (T / body_T);

                // maybe think about more resonances in future (not only 1:1, 2:1, 3:1,...)
                if (T_ratio > tolerance && T_ratio < 1 - tolerance) {
                    continue;
                }

                double theta_temp = angle_vec_vec(p0.r, p1.r);
                if (negative_true_anomaly) theta_temp *= -1;

                double n = sqrt(mu / pow(a, 3));
                struct Vector h = cross_product(s0.r, v_t00);
                struct Vector e_vec = scalar_multiply(
                        add_vectors(cross_product(v_t00, h), scalar_multiply(s0.r, -mu / vector_mag(s0.r))), 1 / mu);
                double e = vector_mag(e_vec);
                double vr = dot_product(v_t00, s0.r) / vector_mag(s0.r);
                double theta0 = acos((dot_product(e_vec, s0.r)) / (e * vector_mag(s0.r)));
                if (vr < 0) theta0 = 2 * M_PI - theta0;
                double E0 = acos((e + cos(theta0)) / (1 + e * cos(theta0)));
                double t0 = (E0 - e * sin(E0)) / n;
                if (theta0 > M_PI) t0 *= -1;
                //printf("E0: %f, t0: %f, theta: %f\n", E0, t0, rad2deg(theta0));

                theta_temp += theta0;
                while (theta_temp < 0) theta_temp += M_PI * 2;
                while (theta_temp > M_PI * 2) theta_temp -= M_PI * 2;

                double t1, t2;

                double E1 = acos((e + cos(theta_temp)) / (1 + e * cos(theta_temp)));
                theta_temp += M_PI;
                double E2 = acos((e + cos(theta_temp)) / (1 + e * cos(theta_temp)));
                theta_temp -= M_PI;
                if (theta_temp < M_PI) {
                    t1 = (E1 - e * sin(E1)) / n - t0;
                    t2 = T - (E2 - e * sin(E2)) / n - t0;
                } else {
                    t1 = T - (E1 - e * sin(E1)) / n - t0;
                    t2 = (E2 - e * sin(E2)) / n - t0;
                }

                while (t1 > T) t1 -= T;
                while (t2 > T) t2 -= T;
                while (t1 < 0) t1 += T;
                while (t2 < 0) t2 += T;

                if (t2 < t1) {
                    double temp = t1;
                    t1 = t2;
                    t2 = temp;
                }

                double min_temp[] = {1e9, 0};
                double angles_temp[] = {angle, theta, phi, defl};
                struct OSV osvs[] = {p0, s0, p1, s1};
                double interval[2];

                double t = 0;
                int counter = -1;
                double max_dur = transfer_duration * 86400 - T * 0.1;
                double min_dur = 86400;
                double t1t2 = t2 - t1;
                double t2t1 = (t1 + T) - t2;

                int right_leg_only_positive = 0;
                //printf("theta: %6.2f°, phi: %6.2f°\n", rad2deg(theta), rad2deg(phi));
                partial_counter++;
                while (t < transfer_duration * 86400) {
                    if (counter % 2 == 0) {
                        t = t1 + T * (counter / 2);
                        interval[0] = t - t2t1;
                        if (interval[0] < min_dur) interval[0] = min_dur;
                        interval[1] = t + t1t2;
                        if (interval[1] > max_dur) interval[1] = max_dur;
                    } else {
                        t = t2 + T * (counter / 2);
                        if (counter < 0) t -= T;
                        interval[0] = t - t1t2;
                        if (interval[0] < min_dur) interval[0] = min_dur;
                        interval[1] = t + t2t1;
                        if (interval[1] > max_dur) interval[1] = max_dur;
                    }


                    counter++;

                    struct Swingby_Peak_Search_Params spsp = {
                            t,
                            transfer_duration,
                            T,
                            interval,
                            min_temp,
                            xs,
                            angles_temp,
                            ints,
                            body,
                            osvs,
                            v_t00
                    };

                    right_leg_only_positive = find_double_swing_by_zero_sec_sb_diff(spsp, right_leg_only_positive);

                    if(min_temp[0] < min_dv) {
                        xs[5][i] = min_temp[0];
                        if(only_viability && min_temp[0] < target_max + tol_it1) return 1;
                        min_dv = min_temp[0];
                        min_man_t = min_temp[1];
                        min_theta = theta;
                        min_phi = phi;
                    }
                    //printf("%f, %f\n", min_temp[0], min_temp[1]);
                    //break;
                }
                //exit(0);
                //break;
            }
            //printf("theta: %6.2f°\n", rad2deg(theta));
            //break;
        }

        if(min_dv >= 1e9 ||
                (i == 0 && min_dv >= target_max+tol_it1) ||
                (i == 1 && min_dv >= target_max+tol_it2) ||
                only_viability) break;

        xs[5][i] = min_dv;
        angles[0] = i == 0 ? max_defl/2 : angles[0]/8;
        num_angle_analyse += 5;
        if(i==0) num_angle_analyse = 20;
        if(i == 1)num_angle_analyse = 25;
        angles[1] = min_theta;
        angles[2] = min_phi;



        gettimeofday(&end, NULL);  // Record the ending time
        elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        printf("%f  (%f°, %f°, %f°)  %d %d", min_dv, rad2deg(min_theta), rad2deg(min_phi), rad2deg(angle_step_size), total_counter, partial_counter);
        printf("    | Elapsed time: %f seconds |\n", elapsed_time);
        double min_theta_temp, min_phi_temp;
        if (i == 0) {
            min_theta_temp = -max_defl - angle_step_size;
        } else {
            angle_step_size = 2 * angles[0] / num_angle_analyse;
            theta = angles[1] - angles[0] - angle_step_size;
            max_theta = angles[1] + angles[0];
            max_phi = angles[2] + angles[0];
        }
        printf("(%f° %f°) (%f°, %f°)\n", rad2deg(min_theta_phi[0]), rad2deg(max_theta), rad2deg(min_theta_phi[1]), rad2deg(max_phi));
        //printf("%f\n", M_PI*2 * sqrt(pow(body->orbit.a,3)/mu) / 86400);
        //printf("%f\n", 2*M_PI*2 * sqrt(pow(body->orbit.a,3)/mu) / 86400);

        //printf("%f°\n", rad2deg(angle_vec_vec(p0.r, p1.r)));
    }
    printf("%f - %f - %f\n", test[0], test[1], test[0]+test[1]);
    exit(0);
    return 0;
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
    // if dt is basically 0, only add step, as this gets subtracted after the loop (not going inside loop)
    theta += fabs(t-target_t) > 1 ? dt/T * M_PI*2 : step;

    theta = pi_norm(theta);
    while(target_t > T) target_t -= T;

    int c = 0;

    while(fabs(t-target_t) > 1) {
        c++;
        theta = pi_norm(theta);
        E = acos((e_mag + cos(theta)) / (1 + e_mag * cos(theta)));
        t = (E - e_mag * sin(E)) / n;
        if(theta > M_PI) t = T-t;

        // prevent endless loops (floating point imprecision can lead to not changing values for very small steps)
        if(c == 500) break;

        // check in which half t is with respect to target_t (forwards or backwards from target_t) and move it closer
        if(target_t < T/2) {
            if(t > target_t && t < target_t+T/2) {
                if (step > 0) step *= -1.0 / 4;
            } else {
                if (step < 0) step *= -1.0 / 4;
            }
        } else {
            if(t < target_t && t > target_t-T/2) {
                if (step < 0) step *= -1.0 / 4;
            } else {
                if (step > 0) step *= -1.0 / 4;
            }
        }
        theta += step;
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

struct OSV osv_from_ephem(struct Ephem *ephem_list, double date, struct Body *attractor) {
    struct Ephem ephem = get_last_ephem(ephem_list, date);
    struct Vector r1 = {ephem.x, ephem.y, ephem.z};
    struct Vector v1 = {ephem.vx, ephem.vy, ephem.vz};
    double dt1 = (date - ephem.date) * (24 * 60 * 60);
    struct OSV osv = propagate_orbit(r1, v1, dt1, attractor);
    return osv;
}
