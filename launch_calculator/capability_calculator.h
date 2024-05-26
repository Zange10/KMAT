#ifndef KSP_CAPABILITY_CALCULATOR_H
#define KSP_CAPABILITY_CALCULATOR_H

#include "launch_calculator/lv_profile.h"

void calc_payload_per_inclination(struct LV lv, double launch_lat, double *incl, double *pl_mass, int num_data_points);
void calc_capability_for_inclination(struct LV lv, double launch_lat, double incl, double *pl_mass, double *apoapsis, double *periapsis, double *rem_dv, double *poss_apo, int num_data_points);

#endif //KSP_CAPABILITY_CALCULATOR_H
