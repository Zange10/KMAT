#include <math.h>
#include <stdio.h>
#include "orbit.h"



struct Orbit constr_orbit(double a, double e, double i, double lan, double arg_of_peri, struct Body *body) {
    struct Orbit new_orbit;
    new_orbit.body = body;
    new_orbit.a = a;
    new_orbit.e = e;
    new_orbit.inclination = i;
    new_orbit.raan = lan;
    new_orbit.arg_peri = arg_of_peri;
    new_orbit.theta = 0;
    new_orbit.apoapsis  = a*(1+e);
    new_orbit.periapsis = a*(1-e);
    new_orbit.period = 2*M_PI*sqrt(pow(a,3)/body->mu);
    return new_orbit;
}


struct Orbit constr_orbit_w_apsides(double apsis1, double apsis2, double inclination, struct Body *body) {
    struct Orbit new_orbit;
    new_orbit.body = body;
    if(apsis1 > apsis2) {
        new_orbit.apoapsis = apsis1;
        new_orbit.periapsis = apsis2;
    } else {
        new_orbit.apoapsis = apsis2;
        new_orbit.periapsis = apsis1;
    }
    new_orbit.a = (new_orbit.apoapsis+new_orbit.periapsis)/2;
    new_orbit.inclination = inclination;
    new_orbit.e = (new_orbit.apoapsis-new_orbit.periapsis)/(new_orbit.apoapsis+new_orbit.periapsis);
    new_orbit.period = 2*M_PI*sqrt(pow(new_orbit.a,3)/body->mu);

    // not given, therefore zero
    new_orbit.raan = 0;
    new_orbit.arg_peri = 0;
    new_orbit.theta = 0;

    return new_orbit;
}

struct Orbit constr_orbit_from_osv(struct Vector r, struct Vector v, struct Body *attractor) {
	struct Orbit new_orbit;
	new_orbit.body = attractor;
	
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
		RAAN = n_norm.y >= 0 ? acos(n_norm.x) : 2 * M_PI - acos(n_norm.x); // if n_norm.y is negative: raan > 180°
		i = acos(dot_product(k, norm_vector(h)));
		arg_peri = e.z >= 0 ? acos(dot_product(n_norm, e) / e_mag) : 2 * M_PI - acos(dot_product(n_norm, e) / e_mag);  // if r.z is positive: w > 180°
	} else {
		RAAN = 0;
		i = dot_product(k, norm_vector(h)) > 0 ? 0 : M_PI;
		arg_peri = cross_product(r,v).z * e.y > 0 ? acos(e.x/e_mag) : 2*M_PI - acos(e.x/e_mag);
	}
	double theta = v_r >= 0 ? acos(dot_product(e,r) / (e_mag*r_mag)) : 2*M_PI - acos(dot_product(e,r) / (e_mag*r_mag));
	
	double n = sqrt(mu / pow(fabs(a),3));
	double t, T = 0;
	if(e_mag < 1) {
		double E = 2*atan(sqrt((1 - e_mag)/(1 + e_mag))*tan(theta/2));
		t = (E - e_mag*sin(E))/n;
		T = 2*M_PI/n;
		if(t < 0) t += T;
	} else {
		double F = acosh((e_mag + cos(theta)) / (1 + e_mag * cos(theta)));
		t = (e_mag * sinh(F) - F) / n;
		if(v_r < 0) t *= -1;
	}
	
	new_orbit.a = a;
	new_orbit.e = e_mag;
	new_orbit.inclination = i;
	new_orbit.raan = RAAN;
	new_orbit.arg_peri = arg_peri;
	new_orbit.t = t;
	new_orbit.period = T;
	new_orbit.theta = theta;
	
	return new_orbit;
}

double calc_dtheta_from_dt(struct Orbit orbit, double dt) {
	double dtheta = 0;
	if(orbit.e < 1) {
		double T = orbit.period;
		double t = orbit.t;
		double target_t = orbit.t + dt;
		while(target_t > T) target_t -= T;
		while(target_t < 0) target_t += T;

		double step = deg2rad(5);
		double theta = orbit.theta;
		theta += fabs(t-target_t) > 1 ? dt/T * M_PI*2 : step;
		theta = pi_norm(theta);

		int c = 0;
		double e = orbit.e;
		double E;
		double n = sqrt(orbit.body->mu / pow(orbit.a,3));

		while(fabs(t-target_t) > 1) {
			c++;
			theta = pi_norm(theta);
			E = acos((e + cos(theta)) / (1 + e * cos(theta)));
			t = (E - e * sin(E)) / n;
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

		dtheta = theta-orbit.theta;
		while(floor(dt/T)   > dtheta/(2*M_PI)) dtheta += 2*M_PI;
		while(floor(dt/T)+1 < dtheta/(2*M_PI)) dtheta -= 2*M_PI;
	}
	return dtheta;
}

double calc_dt_from_dtheta(struct Orbit orbit, double dtheta) {
	double dt = 0;
	double e = orbit.e;
	if(e < 1) {
		double T = orbit.period;
		double n = sqrt(orbit.body->mu / pow(orbit.a,3));
		double E;

		double theta0 = orbit.theta;
		E = acos((e + cos(theta0)) / (1 + e * cos(theta0)));
		double t0 = (E - e * sin(E)) / n;
		if(theta0 > M_PI) t0 = T-t0;

		double theta1 = pi_norm(theta0 + dtheta);
		E = acos((e + cos(theta1)) / (1 + e * cos(theta1)));
		double t1 = (E - e * sin(E)) / n;
		if(theta1 > M_PI) t1 = T-t1;

		dt = t1-t0;
		//printf("%f° %f° %f %f %f\n", rad2deg(theta0), rad2deg(theta1), t0/86400, t1/86400, dt/86400);
		while(floor(dtheta/(2*M_PI))   > dt/T) dt += T;
		while(floor(dtheta/(2*M_PI))+1 < dt/T) dt -= T;
	}
	return dt;
}

double calc_orbital_speed(struct Orbit orbit, double r) {
    double v2 = orbit.body->mu * (2/r - 1/orbit.a);
    return sqrt(v2);
}

// Printing info #######################################################

void print_orbit_info(struct Orbit orbit) {
    struct Body *body = orbit.body;
    printf("\n______________________\nORBIT:\n\n");
    printf("Orbiting: \t\t%s\n", body->name);
    printf("Apoapsis:\t\t%g km\n", (orbit.apoapsis-body->radius)/1000);
    printf("Periapsis:\t\t%g km\n", (orbit.periapsis-body->radius)/1000);
    printf("Semi-major axis:\t%g km\n", orbit.a /1000);
    printf("Inclination:\t\t%g°\n", orbit.inclination);
    printf("Eccentricity:\t\t%g\n", orbit.e);
    printf("Orbital Period:\t\t%gs\n", orbit.period);
    printf("______________________\n\n");
}

void print_orbit_apsides(struct Orbit orbit) {
    struct Body *body = orbit.body;
    double apo = orbit.apoapsis;
    double peri = orbit.periapsis;
    apo  -= body->radius;
    peri -= body->radius;
    apo  /= 1000;
    peri /= 1000;
    printf("%gkm - %gkm", apo, peri);
}