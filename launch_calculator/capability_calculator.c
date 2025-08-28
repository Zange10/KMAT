#include "capability_calculator.h"
#include "launch_sim.h"
#include "orbitlib.h"
#include "celestial_bodies.h"

double calc_max_payload_capability(struct LV lv, double launch_lat, double incl) {
	double max_plmass;
	struct Launch_Results lr = run_launch_simulation(lv, 0, launch_lat, incl, 0.01, 0, 0, 0);
	max_plmass = lr.rf;
	if(lr.pe < 50e3) return 0;
	while(lr.rf > 1 && lr.pe > 50e3) {
		lr = run_launch_simulation(lv, max_plmass, launch_lat, incl, 0.01, 0, 0, 0);
		if(lr.pe > 50e3) max_plmass += lr.rf;
	}
	return max_plmass;
}

void calc_payload_per_inclination(struct LV lv, double launch_lat, double *incl, double *pl_mass, int num_data_points) {
	double max_inclination = deg2rad(110);
	double step = (max_inclination-launch_lat)/(num_data_points-1);

	for(int i = 0; i < num_data_points; i++) {
		incl[i] = launch_lat + i * step;
		pl_mass[i] = calc_max_payload_capability(lv, launch_lat, incl[i]);
	}
}

void calc_capability_for_inclination(struct LV lv, double launch_lat, double incl, double *pl_mass, double *apoapsis, double *periapsis, double *rem_dv, double *poss_apo, int num_data_points) {
	double step_multiplier = 0.85;
	double max_plmass = calc_max_payload_capability(lv, launch_lat, incl);
	pl_mass[0] = max_plmass*1.02;
	pl_mass[1] = max_plmass*1.005;
	pl_mass[2] = max_plmass;
	for(int i = 3; i < num_data_points-1; i++) pl_mass[i] = pl_mass[i-1]*step_multiplier;
	pl_mass[num_data_points-1] = 0;

	struct Launch_Results lr;
	Orbit orbit;
	for(int i = 0; i < num_data_points; i++) {
		lr = run_launch_simulation(lv, pl_mass[i], launch_lat, incl, 0.01, 0, 1, 0);
		rem_dv[i] = lr.rem_dv;

		orbit = constr_orbit_from_osv(lr.state->r, lr.state->v, EARTH());
		apoapsis[i] = calc_orbit_apoapsis(orbit);
		periapsis[i] = calc_orbit_periapsis(orbit);

		// apply rem dv manouvre
		orbit = constr_orbit_from_osv(lr.state->r, scale_vec3(norm_vec3(lr.state->v), mag_vec3(lr.state->v) + lr.rem_dv), EARTH());
		poss_apo[i] = calc_orbit_apoapsis(orbit);

		free_launch_states(lr.state);
	}
}
