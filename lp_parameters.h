#ifndef KSP_LP_PARAMETERS_H
#define KSP_LP_PARAMETERS_H

#include "launch_calculator.h"

// calculate close to optimal parameter for launch profile
void calc_lp_param_adjustments(struct LV lv, double payload_mass, struct Lp_Params *return_params);

#endif //KSP_LP_PARAMETERS_H
