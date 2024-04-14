#include "launch_sim.h"
#include "tools/analytic_geometry.h"
#include "celestial_bodies.h"
#include "launch_state.h"
#include "lv_profile.h"
#include <math.h>
#include <stdio.h>

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

void print_vessel_info2(struct tvs *v) {
	printf("\n______________________\nVESSEL:\n\n");
	printf("Thrust vac:\t%g kN\n", v -> F_vac/1000);
	printf("Thrust sl:\t%g kN\n", v -> F_sl/1000);
	printf("Burn rate:\t%g kg/s\n", v -> burn_rate);
	printf("Init mass:\t%g kg\n", v -> m0);
	printf("Dry mass:\t%g kg\n", v -> me);
	printf("Burn rate:\t%g kg/s\n", v -> burn_rate);
	printf("cd:\t%g\n", v -> cd);
	printf("A:\t%g m²\n", v -> A);
	printf("______________________\n\n");
}

void simulate_stage(struct LaunchState *state, struct tvs vessel, struct Body *body, double heading_at_launch, int stage_id);
void simulate_coast(struct LaunchState *state, struct tvs vessel, struct Body *body, double dt, int stage_id);
void setup_initial_state(struct LaunchState *state, double lat, struct Body *body);
struct Plane calc_plane_parallel_to_surf(struct Vector r);
struct Vector calc_surface_speed(struct Vector east_dir, struct Vector r, struct Vector v, struct Body *body);
struct Vector calc_thrust_vector(struct Vector r, struct Vector u2, double pitch, double heading);
double calc_heading_from_speed(struct Plane s, struct Vector v, double v_mag, double heading_at_launch);
double calc_atmo_press(double h, struct Body *body);
double calc_thrust(double F_vac, double F_sl, double p);
double calc_drag_force(double p, double v, double A, double c_d);

double pitch_program1(double h, const double lp_params[5]);
double pitch_program4(double h, const double lp_params[5]);




void print_v(struct Vector v) {
	printf("(%.3f, %.3f, %.3f)", v.x, v.y, v.z);
}

void run_launch_simulation(struct LV lv, double payload_mass) {
	struct Body *body = EARTH();
	struct LaunchState *launch_state = new_launch_state();
	setup_initial_state(launch_state, deg2rad(39.26), body);

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
			vessel.lp_params[0] = 30e-6;
			vessel.lp_params[1] = 6e-6;
			vessel.lp_params[2] = 58;
		} else {
			vessel.get_pitch = pitch_program1;
			vessel.lp_params[0] = 6.9;
		}

		simulate_stage(launch_state, vessel, body, deg2rad(90), i+1);
		launch_state = get_last_state(launch_state);
		simulate_coast(launch_state, vessel, body, 8, i+1);
		launch_state = get_last_state(launch_state);
	}


	free_launch_states(launch_state);
}

void setup_initial_state(struct LaunchState *state, double lat, struct Body *body) {
	state->r = vec(body->radius*cos(lat), 0, body->radius*sin(lat));
	struct Plane s = calc_plane_parallel_to_surf(state->r);
	struct Vector vb = calc_surface_speed(s.u, state->r, vec(0,0,0), body);
	state->v = scalar_multiply(vb, -1);

}

void simulate_stage(struct LaunchState *state, struct tvs vessel, struct Body *body, double heading_at_launch, int stage_id) {
	double m = vessel.m0;	// vessel mass [kg]
	double step = 0.01;		// step size in seconds [s]
	double r_mag;			// distance to center of earth [m]
	double h;				// altitude [m]
	struct Vector a;		// acceleration acting on the vessel [m/s²]
	struct Vector g;		// acceleration due to Gravity [m/s²]
	struct Vector a_T;		// acceleration due to Thrust [m/s²]
	struct Vector a_D;		// acceleration due to Drag [m/s²]
	struct Vector dv;		// change in velocity due to acceleration a [m/s]
	struct Vector dr;		// change in position due to velocity [m]
	struct Plane s;			// Plane parallel to surface at vessel position (s.u: east, s.v: north)
	struct Vector p;		// Thrust vector
	double pitch;			// pitch of vessel [rad]
	double heading;			// heading of vessel [rad]
	struct Vector vs;		// surface velocity [m/s]
	double vs_mag;			// surface speed [m/s]
	double atmo_p;			// atmospheric pressure [Pa]
	double ecc;				// eccentricity of orbit
	double last_ecc = 1;	// eccentricity of orbit from previous state

	int counter = 100;

	while(m-vessel.burn_rate * step > vessel.me) {
		r_mag = vector_mag(state->r);
		h = r_mag-body->radius;
		s = calc_plane_parallel_to_surf(state->r);
		vs = calc_surface_speed(s.u, state->r, state->v, body);
		vs_mag = vector_mag(vs);
		atmo_p = calc_atmo_press(h, body);

		heading = calc_heading_from_speed(s, vs, vs_mag, heading_at_launch);
		pitch = vessel.get_pitch(h, vessel.lp_params);
		p = calc_thrust_vector(state->r, s.v, pitch, heading);
		a_T = scalar_multiply(p, calc_thrust(vessel.F_vac, vessel.F_sl, atmo_p)/m);

		g = scalar_multiply(state->r, body->mu/(r_mag*r_mag*r_mag));
		a_D = (vs_mag != 0) ? scalar_multiply(norm_vector(vs), calc_drag_force(atmo_p, vs_mag, vessel.A, vessel.cd)/m) : vec(0,0,0);

		a = subtract_vectors(a_T, add_vectors(g, a_D));
		if(counter == 100) {
			printf("%6.2f  %d  %7.3f, %7.3f  % 12.2f, % 12.2f  %8.2f   ", state->t, stage_id, rad2deg(heading), rad2deg(pitch), h, vector_mag(state->v), calc_drag_force(atmo_p, vs_mag, vessel.A, vessel.cd)/m);
//			printf("%6.2f %d % 12.2f, % 12.2f   ", state->t, stage_id, h, vector_mag(state->v));
			print_v(scalar_multiply(state->r, 1e-6));
			printf(" | ");
			print_v(scalar_multiply(vs, 1e-3));
			printf(" | ");
//			print_v(p);
//			printf(" | ");
			print_v(g);
			printf(" | ");
			print_v(a_T);
			printf(" | ");
			print_v(a_D);
			printf(" | ");
			print_v(a);
			printf("\n");
			counter = 0;
		}
		counter++;

		state->next = append_launch_state(state);
		state = state->next;
		state->stage_id = stage_id;

		state->t = state->prev->t + step;
		dv = scalar_multiply(a, step);
		dr = scalar_multiply(add_vectors(state->prev->v, scalar_multiply(dv, 0.5)), step);
		state->v = add_vectors(state->prev->v, dv);
		state->r = add_vectors(state->prev->r, dr);

		ecc = constr_orbit_from_osv(state->r, state->v, body).e;

		m -= vessel.burn_rate * step;
		state->m = m;
		if(ecc > last_ecc && vs_mag > 5000) break;	// using vs_mag because already calculated
		last_ecc = ecc;
	}
}

void simulate_coast(struct LaunchState *state, struct tvs vessel, struct Body *body, double dt, int stage_id) {
	double m = vessel.m0;	// vessel mass [kg]
	double step = 0.01;		// step size in seconds [s]
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
		if(fabs(angle_plane_vec(s, v)) > deg2rad(85) || v_mag < 10) return heading_at_launch;
	}

	double heading = angle_vec_vec(v, s.v);
	if(angle_vec_vec(v, s.u) > M_PI/2) heading = 2*M_PI - heading;	// if retrograde (i.e. SSO or polar)
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










void ls_test() {
	struct LV lv;
	get_test_LV(&lv);
	run_launch_simulation(lv, 10000);








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
