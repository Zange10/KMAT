#ifndef KMAT_IB_TRANSFER_CALC_H
#define KMAT_IB_TRANSFER_CALC_H

#include "ib_transfer_calc_tools.h"

SegmentGroup * calc_interpolation_based_itins(Body **body_sequence, int num_bodies, CelestSystem *system, double min_dep, double max_dep, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double arr_periapsis, double max_arrdv);

#endif //KMAT_IB_TRANSFER_CALC_H
