#ifndef KSP_LP_PARAMETERS_H
#define KSP_LP_PARAMETERS_H

#include "launch_calculator/launch_calculator.h"

void calc_payload_curve(struct LV lv);
void calc_payload_curve4(struct LV lv, double *payload_mass, double *a1, double *a2, double *b2, double *dv, int data_points);
void calc_payload_curve_with_set_lp_params(struct LV lv);


#endif //KSP_LP_PARAMETERS_H
