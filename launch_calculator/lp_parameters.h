#ifndef KSP_LP_PARAMETERS_H
#define KSP_LP_PARAMETERS_H

#include "launch_calculator/launch_calculator.h"

//// calculate close to optimal parameter for launch profile
int lp_param_fixed_payload_analysis4(struct LV lv, double payload_mass, double *best_params, int only_check_for_orbit);
void calc_highest_payload_mass(struct LV lv);
//// payload mass to launch parameter analysis
//void lp_param_mass_analysis(struct LV lv, double payload_min, double payload_max);
//// returns highest payload mass
//double calc_highest_payload_mass(struct LV lv);

#endif //KSP_LP_PARAMETERS_H
