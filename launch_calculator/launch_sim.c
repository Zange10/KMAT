#include "launch_sim.h"
#include "tools/analytic_geometry.h"
#include "celestial_bodies.h"
#include "launch_state.h"
#include "lv_profile.h"
#include "launch_circularization.h"
#include <math.h>
#include <stdio.h>
#include <sys/time.h>

// Define a function pointer type for the function signature
typedef double (*PitchProgramPtr)(double, const double*);


struct tvs {
	double F_vac;		// Thrust in vacuum [N]
	double F_sl;		// Thrust at sea level [N]
	double m0;			// initial mass [kg]
	double me;			// dry mass [kg]
	double burn_rate;	// burn rate [kg/s]
	double cd;			// drag coefficient
	double A;			// cross-section area [m²]
	double lp_params[5];
	PitchProgramPtr get_pitch;
};


void simulate_stage(struct LaunchState *state, struct tvs vessel, struct Body *body, double heading_at_launch, int stage_id, double step);
void simulate_coast(struct LaunchState *state, struct tvs vessel, struct Body *body, double dt, int stage_id, double step);
void setup_initial_state(struct LaunchState *state, double lat, struct Body *body);
struct Plane calc_plane_parallel_to_surf(struct Vector r);
struct Vector calc_surface_speed(struct Vector east_dir, struct Vector r, struct Vector v, struct Body *body);
struct Vector calc_thrust_vector(struct Vector r, struct Vector u2, double pitch, double heading);
double calc_heading_from_speed(struct Plane s, struct Vector v, double v_mag, double heading_at_launch);
double calc_atmo_press(double h, struct Body *body);
double calc_thrust(double F_vac, double F_sl, double p);
double calc_drag_force(double p, double v, double A, double c_d);
double calc_launch_azi(struct Body *body, double latitude, double inclination, int north0_south1);


double pitch_program1(double h, const double lp_params[5]);
double pitch_program4(double h, const double lp_params[5]);




void print_launch_state_info(struct LaunchState *launch_state, struct Body *body) {
	struct Plane s = calc_plane_parallel_to_surf(launch_state->r);

	double gamma = angle_plane_vec(s, launch_state->v);
	double v_mag = vector_mag(launch_state->v);
	double vh = cos(gamma) * v_mag;
	double vv = sin(gamma) * v_mag;
	struct Vector vs = calc_surface_speed(s.u, launch_state->r, launch_state->v, body);

	struct Orbit orbit = constr_orbit_from_osv(launch_state->r, launch_state->v, body);
	double apoapsis = orbit.a*(1+orbit.e) - body->radius;
	double periapsis = orbit.a*(1-orbit.e) - body->radius;

	printf("\n______________________\nFLIGHT:\n\n");
	printf("Time:\t\t\t%.2f s\n", launch_state->t);
	printf("Altitude:\t\t%g km\n", (vector_mag(launch_state->r)-body->radius)/1000);
	printf("Vertical v:\t\t%g m/s\n", vv);
	printf("surfVelocity:\t%g m/s\n", vector_mag(vs));
	printf("Horizontal v:\t%g m/s\n", vh);
	printf("Velocity:\t\t%g m/s\n", vector_mag(launch_state->v));
	printf("\n");
	printf("Radius:\t\t\t%g km\n", vector_mag(scalar_multiply(launch_state->r, 1e-3)));
	printf("Apoapsis:\t\t%g km\n", apoapsis / 1000);
	printf("Periapsis:\t\t%g km\n", periapsis / 1000);
	printf("Eccentricity:\t%g\n", orbit.e);
	printf("Inclination:\t%g°\n", rad2deg(orbit.inclination));
	printf("______________________\n\n");
}

void run_launch_simulation(struct LV lv, double payload_mass) {
	double step_size = 0.001;
	struct Body *body = EARTH();
	double latitude = deg2rad(28.6);
	double inclination = deg2rad(0);
	double launch_heading = calc_launch_azi(body, latitude, inclination, 0);

	struct LaunchState *launch_state = new_launch_state();
	setup_initial_state(launch_state, latitude, body);

	for(int i = 0; i < lv.stage_n; i++) {
		struct tvs vessel = {
				lv.stages[i].F_vac,
				lv.stages[i].F_sl,
				lv.stages[i].m0,
				lv.stages[i].me,
				lv.stages[i].burn_rate,
				lv.c_d,
				lv.A,
		};
		vessel.m0 += payload_mass;
		vessel.me += payload_mass;
		if(i == 0) {
			vessel.get_pitch = pitch_program4;
			vessel.lp_params[0] = 36e-6;
			vessel.lp_params[1] = 12e-6;
			vessel.lp_params[2] = 49;
		} else {
			vessel.get_pitch = NULL;
		}

		simulate_stage(launch_state, vessel, body, launch_heading, i+1, step_size);
		launch_state = get_last_state(launch_state);
		simulate_coast(launch_state, vessel, body, 8, i+1, step_size);
		launch_state = get_last_state(launch_state);

		print_launch_state_info(launch_state, body);
	}

	free_launch_states(launch_state);
}

void setup_initial_state(struct LaunchState *state, double lat, struct Body *body) {
	state->r = vec(body->radius*cos(lat), 0, body->radius*sin(lat));
	struct Plane s = calc_plane_parallel_to_surf(state->r);
	struct Vector vb = calc_surface_speed(s.u, state->r, vec(0,0,0), body);
	state->v = scalar_multiply(vb, -1);

}

void simulate_stage(struct LaunchState *state, struct tvs vessel, struct Body *body, double heading_at_launch, int stage_id, double step) {
	double m = vessel.m0;	// vessel mass [kg]
	double r_mag;			// distance to center of earth [m]
	double v_mag;			// orbital speed [m/s]
	double h;				// altitude [m]
	struct Vector a;		// acceleration acting on the vessel [m/s²]
	struct Vector g;		// acceleration due to Gravity [m/s²]
	struct Vector a_T;		// acceleration due to Thrust [m/s²]
	struct Vector a_D;		// acceleration due to Drag [m/s²]
	struct Vector dv;		// change in velocity due to acceleration a [m/s]
	struct Vector dr;		// change in position due to velocity [m]
	struct Plane s;			// Plane parallel to surface at vessel position (s.u: east, s.v: north)
	struct Vector p;		// Thrust vector
	double F_T;				// Thrust of engines [N]
	double pitch;			// pitch of vessel [rad]
	double heading;			// heading of vessel [rad]
	struct Vector vs;		// surface velocity [m/s]
	double vs_mag;			// surface speed [m/s]
	double atmo_p;			// atmospheric pressure [Pa]
	double ecc;				// eccentricity of orbit
	double last_ecc = 1;	// eccentricity of orbit from previous state
	double gamma;			// flight path angle [rad]
	double vh;				// horizontal speed [m/s]
	double vv;				// vertical speed [m/s]

	while(m-vessel.burn_rate * step > vessel.me) {
		r_mag = vector_mag(state->r);
		h = r_mag-body->radius;
		s = calc_plane_parallel_to_surf(state->r);
		vs = calc_surface_speed(s.u, state->r, state->v, body);
		vs_mag = vector_mag(vs);
		atmo_p = calc_atmo_press(h, body);
		F_T = calc_thrust(vessel.F_vac, vessel.F_sl, atmo_p);

		heading = calc_heading_from_speed(s, vs, vs_mag, heading_at_launch);
		if(vessel.get_pitch != NULL) pitch = vessel.get_pitch(h, vessel.lp_params);
		else {
			gamma = angle_plane_vec(s, state->v);
			v_mag = vector_mag(state->v);
			vh = cos(gamma) * v_mag;
			vv = sin(gamma) * v_mag;
			pitch = circularization_pitch(F_T, m, vessel.burn_rate, vh, vv, r_mag, body->mu);
		}
		p = calc_thrust_vector(state->r, s.v, pitch, heading);
		a_T = scalar_multiply(p, F_T/m);

		g = scalar_multiply(state->r, body->mu/(r_mag*r_mag*r_mag));
		a_D = (vs_mag != 0) ? scalar_multiply(norm_vector(vs), calc_drag_force(atmo_p, vs_mag, vessel.A, vessel.cd)/m) : vec(0,0,0);

		a = subtract_vectors(a_T, add_vectors(g, a_D));

		state->next = append_launch_state(state);
		state = state->next;
		state->stage_id = stage_id;

		state->t = state->prev->t + step;
		dv = scalar_multiply(a, step);
		dr = scalar_multiply(add_vectors(state->prev->v, scalar_multiply(dv, 0.5)), step);
		state->v = add_vectors(state->prev->v, dv);
		state->r = add_vectors(state->prev->r, dr);

		m -= vessel.burn_rate * step;
		state->m = m;

		if(vs_mag > 6500) {
			ecc = constr_orbit_from_osv(state->r, state->v, body).e;
			if(ecc > last_ecc && vs_mag > 5000) break;
			last_ecc = ecc;
		}
	}
}

void simulate_coast(struct LaunchState *state, struct tvs vessel, struct Body *body, double dt, int stage_id, double step) {
	double m = vessel.m0;	// vessel mass [kg]
	double r_mag;			// distance to center of earth [m]
	double h;				// altitude [m]
	struct Vector a;		// acceleration acting on the vessel [m/s²]
	struct Vector g;		// acceleration due to Gravity [m/s²]
	struct Vector a_D;		// acceleration due to Drag [m/s²]
	struct Vector dv;		// change in velocity due to acceleration a [m/s]
	struct Vector dr;		// change in position due to velocity [m]
	struct Plane s;			// Plane parallel to surface at vessel position (s.u: east, s.v: north)
	struct Vector vs;		// surface velocity [m/s]
	double vs_mag;			// surface speed [m/s]
	double atmo_p;			// atmospheric pressure [Pa]
	double t1 = state->t+dt;

	while(state->t < t1) {
		r_mag = vector_mag(state->r);
		h = r_mag-body->radius;
		s = calc_plane_parallel_to_surf(state->r);
		vs = calc_surface_speed(s.u, state->r, state->v, body);
		vs_mag = vector_mag(vs);
		atmo_p = calc_atmo_press(h, body);

		g = scalar_multiply(state->r, body->mu/(r_mag*r_mag*r_mag));
		a_D = scalar_multiply(norm_vector(vs), calc_drag_force(atmo_p, vs_mag, vessel.A, vessel.cd)/m);

		a = subtract_vectors(vec(0,0,0), add_vectors(g, a_D));

		state->next = append_launch_state(state);
		state = state->next;
		state->stage_id = -stage_id;

		state->t = state->prev->t + step;
		dv = scalar_multiply(a, step);
		dr = scalar_multiply(add_vectors(state->prev->v, scalar_multiply(dv, 0.5)), step);
		state->v = add_vectors(state->prev->v, dv);
		state->r = add_vectors(state->prev->r, dr);
		state->m = m;
	}
}

double pitch_program1(double h, const double lp_params[5]) {
	double a = lp_params[0];
	return deg2rad(a);
}

double pitch_program2(double h, const double lp_params[5]) {
	double a = lp_params[0];
	return 90.0 * exp(-a * h);
}

double pitch_program3(double h, const double lp_params[5]) {
	double a = lp_params[0];
	double b = lp_params[1];
	return (90.0-b) * exp(-a * h) + b;
}

double pitch_program4(double h, const double lp_params[5]) {
	double a1 = lp_params[0];
	double a2 = lp_params[1];
	double b2 = lp_params[2];
	double h_trans = log(b2/90) / (a2-a1);	// transition altitude

	double pitch_deg = (h < h_trans) ? 90.0 * exp(-a1 * h) : b2 * exp(-a2 * h);
	return deg2rad(pitch_deg);
}

struct Plane calc_plane_parallel_to_surf(struct Vector r) {
	struct Plane s;
	s.loc = r;
	s.u = norm_vector(cross_product(vec(0,0,1), r));
	s.v = norm_vector(cross_product(r, s.u));
	if(s.v.z < 0) s.v = scalar_multiply(s.v, -1); // should be unnecessary, but to be on safe side
	return s;
}

struct Vector calc_surface_speed(struct Vector east_dir, struct Vector r, struct Vector v, struct Body *body) {
	struct Plane eq = constr_plane(vec(0,0,0), vec(1,0,0), vec(0,1,0));
	double lat = angle_plane_vec(eq, r);
	double v_b = 2*M_PI*vector_mag(r) / body->rotation_period * cos(lat);
	struct Vector v_b_vec = scalar_multiply(east_dir, v_b);
	return subtract_vectors(v, v_b_vec);
}

struct Vector calc_thrust_vector(struct Vector r, struct Vector u2, double pitch, double heading) {
	struct Vector p = rotate_vector_around_axis(u2, r, -heading);
	struct Vector n = cross_product(p, r);

	p = rotate_vector_around_axis(p, n, pitch);
	return p;
}

double calc_heading_from_speed(struct Plane s, struct Vector v, double v_mag, double heading_at_launch) {
	if(v_mag < 1000) {
		if(v_mag < 10 || fabs(angle_plane_vec(s, v)) > deg2rad(60)) return heading_at_launch;
	}
	struct Vector proj_vs = proj_vec_plane(v, s);
	double heading = angle_vec_vec(proj_vs, s.v);
	if(angle_vec_vec(v, s.u) > M_PI/2) heading = 2* M_PI - heading;	// if retrograde (i.e. SSO or polar)
	return heading;
}

double calc_atmo_press(double h, struct Body *body) {
	if(h<body->atmo_alt) return body->sl_atmo_p*exp(-h/body->scale_height);
	else return 0;
}

double calc_thrust(double F_vac, double F_sl, double p) {
	if(p == 0) return F_vac;
	return F_vac + (p/101325)*(F_sl-F_vac);    // sea level pressure: 101325 Pa
}

double calc_drag_force(double p, double v, double A, double c_d) {
	if(p == 0) return 0;
	double rho = p/57411.6;
	return 0.5*rho*A*c_d*pow(v,2);
}

double calc_launch_azi(struct Body *body, double latitude, double inclination, int north0_south1) {
	if(inclination < latitude) inclination = latitude;
	if(inclination > M_PI-latitude) inclination = M_PI - latitude;

	double surf_speed = cos(latitude) * body->radius*2*M_PI/body->rotation_period;
	double end_speed = 7800;    // orbital speed hard coded

	double azi1 = asin(cos(inclination)/cos(latitude));

	struct Vector2D azi1_v = {sin(azi1)*end_speed, cos(azi1)*end_speed};
	struct Vector2D azi2_v = {surf_speed, 0};
	struct Vector2D azi_v = add_vectors2d(azi1_v, scalar_multipl2d(azi2_v,-1));

	double azimuth = atan(azi_v.x / azi_v.y);
	if(north0_south1) azimuth = M_PI - azimuth;
	return azimuth;
}






void ls_test() {
	struct LV lv;
	get_test_LV(&lv);
	struct timeval start_time, end_time;
	double elapsed_time;
	gettimeofday(&start_time, NULL);
	run_launch_simulation(lv, 1000);
	gettimeofday(&end_time, NULL);

	// Calculate the elapsed time in seconds
	elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
				   (end_time.tv_usec - start_time.tv_usec) / 1000000.0;

	printf("Elapsed time: %f seconds\n", elapsed_time);
	printf("Elapsed time: %f ms\n", elapsed_time*1000);




//	struct Vector z = vec(0,0,1);
//	struct Vector x = vec(1,0,0);
//	struct Vector y = vec(0,1,0);
//
//	struct Plane p = constr_plane(vec(0,0,0), x, y);
//	for(int i = 0; i <= 380; i+=10) {
//		double a = deg2rad(i);
//		struct Vector v = vec(cos(a), 0, sin(a));
//		double anglex = angle_plane_vec(p, v);
//		printf("%.2f°\n", rad2deg(anglex));
//	}
//
//
//	print_v(cross_product(y,x));
//	return;
//	for(int i = 0; i <= 360; i += 30) {
//		for(int j = -90; j <= 90; j += 30) {
//			double theta = deg2rad(i);
//			double phi = deg2rad(j);
//			struct Vector v = vec(cos(phi) * cos(theta), cos(phi)*sin(theta), sin(phi));
//			print_v(v);
//
//			struct Plane s = calc_plane_parallel_to_surf(v);
//			printf(" | ");
//			print_v(s.u);
//			printf(" | ");
//			print_v(s.v);
//			printf(" | ");
//			struct Vector t = rotate_vector_around_axis(s.v, v, deg2rad(-170));
//			print_v(t);
//			double anglex = angle_vec_vec(t, s.u);
//			double anglez = anglex <= M_PI/2 ? angle_vec_vec(t, s.v) : M_PI*2 - angle_vec_vec(t, s.v);
//
//			printf(" | (% 6.2f°  % 6.2f°)", rad2deg(anglez), rad2deg(anglex));
//			printf("\n");
//		}
//	}
}
