#ifndef KSP_TRANSFER_CALCULATOR_H
#define KSP_TRANSFER_CALCULATOR_H

#include "analytic_geometry.h"
#include "porkchop_tools.h"

void simple_transfer();
void create_swing_by_transfer();
struct Transfer calc_transfer(enum Transfer_Type tt, struct Body *dep_body, struct Body *arr_body, struct Vector r1, struct Vector v1, struct Vector r2, struct Vector v2, double dt, double *data);
void get_cheapest_transfer_dates(double **porkchops, double *p_dep, double dv_dep, int index, int num_transfers, double *current_dates, double *jd_dates, double *min, double *final_porkchop);


#endif //KSP_TRANSFER_CALCULATOR_H
