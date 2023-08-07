#ifndef KSP_LP_PARAMETERS_H
#define KSP_LP_PARAMETERS_H

#include "launch_calculator.h"

// calculate close to optimal parameter for launch profile
void lp_param_fixed_payload_analysis(struct LV lv, double payload_mass, struct Lp_Params *return_params);
// payload mass to launch parameter analysis
void lp_param_mass_analysis(struct LV lv, double payload_min);

#endif //KSP_LP_PARAMETERS_H
