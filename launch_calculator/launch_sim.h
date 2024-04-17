#ifndef KSP_LAUNCH_SIM_H
#define KSP_LAUNCH_SIM_H


#include "lv_profile.h"

// resulting parameters from launch for parameter adjustment calculations
struct Launch_Results {
	double pe;      // Periapsis [m]
	double dv;      // spent delta-v [m/s]
	double rf;      // remaining fuel [kg]
};

void simulate_single_launch(struct LV lv);

struct Launch_Results run_launch_simulation(struct LV lv, double payload_mass, double step_size, int bool_print_info);

#endif //KSP_LAUNCH_SIM_H
