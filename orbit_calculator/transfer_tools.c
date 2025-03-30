#include "transfer_tools.h"
#include "tools/data_tool.h"
#include "orbit_calculator/orbit_calculator.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>


double theta1_at_min_dt(double r0, double r1, double dtheta) {
	dtheta = pi_norm(dtheta);
	double max_theta1;
	if(dtheta < M_PI) {
		double r = r1/r0;
		double r1r2 = sqrt(1 + r*r - 2 * r * cos(dtheta));
		double beta = acos((1 + r1r2 * r1r2 - r * r) / (2 * r1r2));
		double alpha = M_PI / 2 - beta;
		max_theta1 = 2*M_PI-alpha;
	} else {
		max_theta1 = 2*M_PI-dtheta/2;
	}
	return pi_norm(max_theta1);
}

double theta1_at_max_dt(double r0, double r1, double dtheta) {
	double min_theta1;
	struct Vector2D p1 = {r0, 0};
	struct Vector2D p2 = {cos(dtheta)*r1, sin(dtheta)*r1};
	double pxr = (p2.x-p1.x)/(r1 - r0);
	double pyr = (p2.y-p1.y)/(r1 - r0);
	double p = (2*pxr)/(pxr*pxr + pyr*pyr);
	double q = (1-pyr*pyr)/(pxr*pxr + pyr*pyr);

	// IF NAN IN 2D TRANSFER CALC, LOOK HERE. SQRT SHOULD NOT GET NEGATIVE
	double inside_sqrt = p*p/4 - q;
	if(inside_sqrt < 0) inside_sqrt *= -1;

	double mx1 = -p/2 - sqrt(inside_sqrt);
	double mx2 = -p/2 + sqrt(inside_sqrt);

	struct Vector2D m_n[] = {
			{mx1, +sqrt(1-mx1*mx1)},
			{mx1, -sqrt(1-mx1*mx1)},
			{mx2, +sqrt(1-mx2*mx2)},
			{mx2, -sqrt(1-mx2*mx2)}
	};

	double m_ml_dot[4];

	// dot product of m and ml should be 0, due to imprecision not accurately 0 -> find smallest of the pairs
	// pair (0,1) and (2,3) (see above; two time two solutions of square root)
	for(int i = 0; i < 4; i++) m_ml_dot[i] = m_n[i].x*m_n[i].x+pxr*m_n[i].x+m_n[i].y*m_n[i].y+pyr*m_n[i].y;
	if(fabs(m_ml_dot[1]) < fabs(m_ml_dot[0])) m_n[0] = m_n[1];
	m_n[1] = fabs(m_ml_dot[2]) < fabs(m_ml_dot[3]) ? m_n[2] : m_n[3];

	for(int i = 0; i < 2; i++) {
		min_theta1 = ccw_angle_vec_vec_2d(p1, m_n[i]);
		double min_theta2 = ccw_angle_vec_vec_2d(p2, m_n[i]);
		if(min_theta1 > min_theta2) {
			break;
		}
	}
	return pi_norm(min_theta1);
}


struct Transfer2D calc_2d_transfer_orbit(double r0, double r1, double target_dt, double dtheta, struct Body *attractor) {
	// 0°, 180° and 360° are extreme edge cases with funky stuff happening with floating point imprecision -> adjust dtheta
	if(fabs(dtheta) < 0.001 ||
	   fabs(dtheta-M_PI) < 0.001) dtheta += 0.001;
	if(fabs(dtheta-2*M_PI) < 0.001) dtheta -= 0.001;

	double min_theta1 = r1 / r0 > 1 ? theta1_at_min_dt(r0, r1, dtheta) : theta1_at_max_dt(r0, r1, dtheta);
	double max_theta1 = r1 / r0 > 1 ? theta1_at_max_dt(r0, r1, dtheta) : theta1_at_min_dt(r0, r1, dtheta);

	if(min_theta1 > max_theta1) min_theta1 -= 2*M_PI;

	struct Vector2D data[103];
	data[0].x = 0;
	insert_new_data_point(data, min_theta1, r1/r0 > 1 ? -target_dt : 1e100);
	insert_new_data_point(data, max_theta1, r1/r0 > 1 ? 1e100 : -target_dt);

	// true anomaly of r0, theta1 not normed to pi and true anomaly of r1
	double theta1, theta1_pun, theta2, last_theta1_pun;
    double mu = attractor->mu;

    double dt;
    double a, e;

	for(int i = 0; i < 100; i++) {
		theta1_pun = root_finder_monot_func_next_x(data, r1/r0 > 1);
		if(i > 3 && last_theta1_pun == theta1_pun) break;

        theta1 = pi_norm(theta1_pun);
        theta2 = pi_norm(theta1 + dtheta);
        e = (r1 - r0) / (r0 * cos(theta1) - r1 * cos(theta2));
        if(e < 0){  // not possible
			printf("\n\n!!!!! PANIC e < 0 !!!!!!!!\n");
			printf("theta1: %.10f°; theta2: %f°; target dt: %fd\nr0: %fe6m; r1: %fe6m; dtheta: %f°\n", rad2deg(theta1), rad2deg(theta2), target_dt/86400, r0*1e-9, r1*1e-9, rad2deg(dtheta));
			printf("min theta1: %f°; max theta1: %f°\ne: %f; a: %f\n", rad2deg(min_theta1), rad2deg(max_theta1), e, a);
			printf("theta1 = [");
			for(int j = 1; j <= data[0].x; j++) {
				if(j!=1) printf(", ");
				printf("%.10f", rad2deg(data[j].x));
			}
			printf("]\ndt = [");
			for(int j = 1; j <= data[0].x; j++) {
				if(j!=1) printf(", ");
				printf("%.4f", data[j].y/86400);
			}
			printf("]\n");
			printf("-------------\n\n");
			exit(1);
        } else if(e==1) e += 1e-10;	// no calculations for parabola -> make it a hyperbola

        double rp = r0 * (1 + e * cos(theta1)) / (1 + e);
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
            if((theta1 < M_PI  &&  theta2 > M_PI) ||
              ((theta1 < M_PI) == (theta2 < M_PI) && (theta1 > theta2))){
				if(theta1 > data[(int) data[0].x-1].x) {
					data[(int) data[0].x].x = theta1;
				} else if(theta1 < data[2].x) {
					data[1].x = theta1;
				}
    
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
			printf("---!!!!   NAN   !!!!---\n");
			printf("theta1: %.10f°; theta2: %f°; target dt: %fd\nr0: %fe6m; r1: %fe6m; dtheta: %f°\n", rad2deg(theta1), rad2deg(theta2), target_dt/86400, r0*1e-9, r1*1e-9, rad2deg(dtheta));
			printf("min theta1: %f°; max theta1: %f°\nt1: %fd; t2: %fd; T: %fd; T/2: %fd\ne: %f; a: %f\n", rad2deg(min_theta1), rad2deg(max_theta1), t1/86400, t2/86400, T/86400, T/2/86400, e, a);
			printf("theta1 = [");
			for(int j = 1; j <= data[0].x; j++) {
				if(j!=1) printf(", ");
				printf("%.10f", rad2deg(data[j].x));
			}
			printf("]\ndt = [");
			for(int j = 1; j <= data[0].x; j++) {
				if(j!=1) printf(", ");
				printf("%.4f", data[j].y/86400);
			}
			printf("]\n");
			printf("-------------\n\n");
            break;
        }
		
		insert_new_data_point(data, theta1_pun, dt - target_dt);
		last_theta1_pun = theta1_pun;

		if(fabs(target_dt-dt) < 1) break;
    }

    struct Transfer2D transfer = {constr_orbit(a, e, 0, 0, 0, 0, attractor), theta1, theta2};
    return transfer;
}

struct Transfer calc_transfer(enum Transfer_Type tt, struct Body *dep_body, struct Body *arr_body, struct Vector r1, struct Vector v1, struct Vector r2, struct Vector v2, double dt, struct Body *attractor, double *data) {
    double dtheta = angle_vec_vec(r1, r2);
    if (cross_product(r1, r2).z < 0) dtheta = 2 * M_PI - dtheta;

    struct Transfer2D transfer2d = calc_2d_transfer_orbit(vector_mag(r1), vector_mag(r2), dt, dtheta, attractor);
	struct Transfer transfer = calc_transfer_dv(transfer2d, r1, r2);
	
	
	if(data != NULL) {
		double dv1, dv2;
		if(dep_body != NULL) {
			double v_t1_inf = fabs(vector_mag(subtract_vectors(transfer.v0, v1)));
			dv1 = tt % 2 == 0 ? dv_capture(dep_body, dep_body->atmo_alt + 50e3, v_t1_inf) : dv_circ(dep_body,dep_body->atmo_alt + 50e3,v_t1_inf);
		} else dv1 = vector_mag(v1);

		if(arr_body != NULL) {
			double v_t2_inf = fabs(vector_mag(subtract_vectors(transfer.v1, v2)));
			if(tt < 2) dv2 = dv_capture(arr_body, arr_body->atmo_alt + 50e3, v_t2_inf);
			else if(tt < 4) dv2 = dv_circ(arr_body, arr_body->atmo_alt + 50e3, v_t2_inf);
			else dv2 = 0;
		} else {
			dv2 = vector_mag(v2);
		}
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
            { cos(RAAN)*cos(inc)*sin(w)+sin(RAAN)*cos(w),     cos(RAAN)*cos(inc)*cos(w)-sin(RAAN)*sin(w),  -cos(RAAN)*sin(inc)},
            { sin(inc)*sin(w),                                 		 sin(inc)*cos(w),                              		  cos(inc)}};

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
    if(inters_line.y < 0) inters_line = scalar_multiply(inters_line, -1); // for rotation of raan in clock-wise direction
    struct Vector in_plane_up = cross_product(inters_line, calc_plane_norm_vector(p_T));    // 90° to intersecting line and norm vector of plane
    if(in_plane_up.z < 0) in_plane_up = scalar_multiply(in_plane_up, -1);   // this vector is always 90° before raan for prograde orbits
    double RAAN = in_plane_up.x <= 0 ? angle_vec_vec(vec(1,0,0), inters_line) : angle_vec_vec(vec(1,0,0), inters_line) + M_PI;   // raan 90° behind in_plane_up

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


double calc_hohmann_transfer_duration(double r0, double r1, struct Body *attractor) {
	double sma_pow_3 = pow(((r0 + r1) / 2),3);
	return M_PI * sqrt(sma_pow_3 / attractor->mu);
}

void calc_hohmann_transfer_dv(double r0, double r1, struct Body *attractor, double *dv_dep, double *dv_arr) {
	*dv_dep = calc_maneuver_dV(r0, r0, r1, attractor);
	*dv_arr = calc_maneuver_dV(r1, r0, r1, attractor);
}

void calc_interplanetary_hohmann_transfer(struct Body *dep_body, struct Body *arr_body, struct Body *attractor, double *dur, double *dv_dep, double *dv_arr_cap, double *dv_arr_circ) {
	double r0 = dep_body->orbit.a;
	double r1 = arr_body->orbit.a;

	*dur = calc_hohmann_transfer_duration(r0, r1, attractor);
	calc_hohmann_transfer_dv(r0, r1, attractor, dv_dep, dv_arr_cap);
	*dv_arr_circ = *dv_arr_cap;

	*dv_dep = dv_circ(dep_body, dep_body->atmo_alt + 50e3, *dv_dep);
	*dv_arr_circ = dv_circ(arr_body, dep_body->atmo_alt + 50e3, *dv_arr_circ);
	*dv_arr_cap = dv_capture(arr_body, dep_body->atmo_alt + 50e3, *dv_arr_cap);
}


int is_flyby_viable(const double *t, struct OSV *osv, struct Body **body, struct Body *attractor) {
	double data[3];
	struct Transfer transfer1 = calc_transfer(circcirc, body[0], body[1], osv[0].r, osv[0].v, osv[1].r,
											  osv[1].v, (t[1] - t[0]) * (24 * 60 * 60), attractor, data);
	double arr_v = data[2];
	struct Transfer transfer2 = calc_transfer(circcirc, body[1], body[2], osv[1].r, osv[1].v, osv[2].r,
											  osv[2].v, (t[2] - t[1]) * (24 * 60 * 60), attractor, data);
	double dep_v = data[1];
	if (fabs(arr_v - dep_v) > 10) return 0;


	struct Vector v_arr = subtract_vectors(transfer1.v1, osv[1].v);
	struct Vector v_dep = subtract_vectors(transfer2.v0, osv[1].v);
	double beta = (M_PI - angle_vec_vec(v_arr, v_dep))/2;
	double rp = (1 / cos(beta) - 1) * (body[1]->mu / (pow(vector_mag(v_arr), 2)));
	if (rp > body[1]->radius + body[1]->atmo_alt) 	return 1;
	else 											return 0;
}

struct DepArrHyperbolaParams get_dep_hyperbola_params(struct Vector v_sat, struct Vector v_body, struct Body *body, double h_pe) {
	struct Vector v_inf = subtract_vectors(v_sat, v_body);
	double v_he = vector_mag(v_inf);
	double rp = body->radius + h_pe;
	double e = 1 + rp*v_he*v_he/body->mu;
	double a = rp/(1-e);
	double energy = -body->mu/(a);
	struct Plane xy = constr_plane(vec(0,0,0), vec(1,0,0), vec(0,1,0));
	struct Plane xz = constr_plane(vec(0,0,0), vec(1,0,0), vec(0,0,1));
	double decl = angle_plane_vec(xy, v_inf);
	decl = v_inf.z < 0 ? -fabs(decl) : fabs(decl);
	double bplane_angle = -angle_plane_vec(xz, v_inf);
	if(cross_product(v_inf,vec(0,1,0)).z < 0) bplane_angle = M_PI-bplane_angle;
	bplane_angle = pi_norm(bplane_angle);
	
	struct DepArrHyperbolaParams hyp_params = {rp, energy, decl, bplane_angle, M_PI/2};
	return hyp_params;
}

struct FlybyHyperbolaParams get_hyperbola_params(struct Vector v_arr, struct Vector v_dep, struct Vector v_body, struct Body *body, double h_pe) {
	struct Vector vinf_arr = subtract_vectors(v_arr, v_body);
	struct Vector vinf_dep = subtract_vectors(v_dep, v_body);
	
	struct FlybyHyperbolaParams hyp_params;
	hyp_params.arr_hyp = get_dep_hyperbola_params(v_arr, v_body, body, h_pe);
	hyp_params.dep_hyp = get_dep_hyperbola_params(v_dep, v_body, body, h_pe);
	hyp_params.arr_hyp.decl *= -1;
	hyp_params.arr_hyp.bplane_angle = pi_norm(M_PI + hyp_params.arr_hyp.bplane_angle);
	
	struct Vector N = cross_product(vinf_arr, vinf_dep);
	struct Vector B_arr = cross_product(vinf_arr, N);
	struct Vector B_dep = cross_product(vinf_dep, scalar_multiply(N,-1));
	
	// BVAZI (Azimuth of B-vector) calculated from south
	hyp_params.arr_hyp.bvazi = angle_vec_vec(vec(0,0,-1),B_arr);
	hyp_params.dep_hyp.bvazi = angle_vec_vec(vec(0,0,-1),B_dep);
	
	// if retrograde orbit
	if(N.z < 0) {
		hyp_params.arr_hyp.bvazi *= -1;
		hyp_params.dep_hyp.bvazi *= -1;
	}
	
	hyp_params.arr_hyp.bvazi = pi_norm(hyp_params.arr_hyp.bvazi);
	hyp_params.dep_hyp.bvazi = pi_norm(hyp_params.dep_hyp.bvazi);
	
	return hyp_params;
}

double get_flyby_periapsis(struct Vector v_arr, struct Vector v_dep, struct Vector v_body, struct Body *body) {
	struct Vector v1 = subtract_vectors(v_arr, v_body);
	struct Vector v2 = subtract_vectors(v_dep, v_body);
	double beta = (M_PI - angle_vec_vec(v1, v2))/2;
	return (1 / cos(beta) - 1) * (body->mu / (pow(vector_mag(v1), 2)));
}

double get_flyby_inclination(struct Vector v_arr, struct Vector v_dep, struct Vector v_body) {
	struct Vector v1 = subtract_vectors(v_arr, v_body);
	struct Vector v2 = subtract_vectors(v_dep, v_body);
	struct Plane ecliptic = constr_plane(vec(0, 0, 0), vec(1, 0, 0), vec(0, 1, 0));
	struct Plane hyperbola_plane = constr_plane(vec(0, 0, 0), v1, v2);
	return angle_plane_plane(ecliptic, hyperbola_plane);
}

struct OSV propagate_orbit_time(struct Orbit orbit, double dt, struct Body *attractor) {
	double theta = orbit.theta;
	double t = orbit.t;
	double e = orbit.e;
	double target_t = t + dt;
	double a = orbit.a;
	double raan = orbit.raan;
	double arg_peri = orbit.arg_peri;
	double i = orbit.inclination;
	double mu = attractor->mu;
	double T = orbit.period;

	double n = sqrt(mu / pow(fabs(a),3));

	double step = deg2rad(5);
	// if dt is basically 0, only add step, as this gets subtracted after the loop (not going inside loop)
	if(e<1) theta += fabs(t-target_t) > 1 ? dt/T * M_PI*2 : step;

	theta = pi_norm(theta);
	while(target_t > T && e < 1) target_t -= T;
	while(target_t < 0 && e < 1) target_t += T;

	int c = 0;

	while(fabs(t-target_t) > 1) {
		c++;
		// prevent endless loops (floating point imprecision can lead to not changing values for very small steps)
		if(c == 500) break;

		theta = pi_norm(theta);
		if(e < 1) {
			double E = acos((e + cos(theta))/(1 + e*cos(theta)));
			t = (E - e*sin(E))/n;
			if(theta > M_PI) t = T-t;
		} else {
			//printf("[%f %f %f %f]\n", t/(24*60*60), target_t/(24*60*60), (target_t-t)/(24*60*60), rad2deg(theta));
			double F = acosh((e + cos(theta))/(1 + e*cos(theta)));
			t = (e*sinh(F) - F)/n;
			if(theta > M_PI) t *= -1;
			if(isnan(t)) {
				step /= 2;
				theta -= step;
				t = 100;	// to not exit the loop;
				continue;
			}
		}

		// check in which half t is with respect to target_t (forwards or backwards from target_t) and move it closer
		if(target_t < T/2 || e > 1) {
			if(t > target_t && (t < target_t+T/2  || e > 1)) {
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

	double gamma = atan(e*sin(theta)/(1+e*cos(theta)));
	double r_mag = a*(1-pow(e,2)) / (1+e*cos(theta));
	double v_mag = sqrt(mu*(2/r_mag - 1/a));
	struct Vector2D r_2d = {cos(theta) * r_mag, sin(theta) * r_mag};
	struct Vector2D v_2d = calc_v_2d(r_mag, v_mag, theta, gamma);

	struct Vector r = heliocentric_rot(r_2d, raan, arg_peri, i);
	struct Vector v = heliocentric_rot(v_2d, raan, arg_peri, i);

	struct OSV osv = {r, v};
	return osv;
}

struct OSV propagate_orbit_theta(struct Orbit orbit, double dtheta, struct Body *attractor) {
	double theta = pi_norm(orbit.theta+dtheta);
	double e = orbit.e;
	double a = orbit.a;
	double RAAN = orbit.raan;
	double arg_peri = orbit.arg_peri;
	double i = orbit.inclination;
	double mu = attractor->mu;
	
	double gamma = atan(e*sin(theta)/(1+e*cos(theta)));
	double r_mag = a*(1-pow(e,2)) / (1+e*cos(theta));
	double v_mag = sqrt(mu*(2/r_mag - 1/a));
	struct Vector2D r_2d = {cos(theta) * r_mag, sin(theta) * r_mag};
	struct Vector2D v_2d = calc_v_2d(r_mag, v_mag, theta, gamma);

	struct Vector r = heliocentric_rot(r_2d, RAAN, arg_peri, i);
	struct Vector v = heliocentric_rot(v_2d, RAAN, arg_peri, i);

	struct OSV osv = {r, v};
	return osv;
}

struct OSV osv_from_ephem(struct Ephem *ephem_list, double date, struct Body *attractor) {
    struct Ephem ephem = get_closest_ephem(ephem_list, date);
    struct Vector r1 = {ephem.x, ephem.y, ephem.z};
    struct Vector v1 = {ephem.vx, ephem.vy, ephem.vz};
    double dt1 = (date - ephem.date) * (24 * 60 * 60);
    struct OSV osv = propagate_orbit_time(constr_orbit_from_osv(r1,v1,attractor), dt1, attractor);
    return osv;
}

struct OSV osv_from_elements(struct Orbit orbit, double date, struct System *system) {
	double dt = (date - system->ut0) * (24 * 60 * 60);
	struct OSV osv = propagate_orbit_time(orbit, dt, system->cb);
	return osv;
}