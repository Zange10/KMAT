#ifndef KSP_LAUNCH_SIM_H
#define KSP_LAUNCH_SIM_H


#include "lv_profile.h"
#include "launch_state.h"

// resulting parameters from launch for parameter adjustment calculations
struct Launch_Results {
	double pe;		// Periapsis [m]
	double dv;		// spent delta-v [m/s]
	double rem_dv;	// spent delta-v [m/s]
	double rf;		// remaining fuel [kg]
	struct LaunchState *state;
};

void simulate_single_launch(struct LV lv);

struct Launch_Results run_launch_simulation(struct LV lv, double payload_mass, double latitude, double target_inclination, double step_size, int bool_print_info, int bool_return_state, double coast_time_after_launch);

#endif //KSP_LAUNCH_SIM_H
