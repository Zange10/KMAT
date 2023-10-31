#ifndef KSP_TRANSFER_CALCULATOR_H
#define KSP_TRANSFER_CALCULATOR_H

#include "analytic_geometry.h"

void create_porkchop();

void calc_transfer(struct Vector r1, struct Vector v1, struct Vector r2, struct Vector v2, double dt, double *data);

#endif //KSP_TRANSFER_CALCULATOR_H
