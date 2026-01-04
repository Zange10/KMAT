#ifndef KMAT_ITIN_REWORK_TOOLS_H
#define KMAT_ITIN_REWORK_TOOLS_H


#include "orbitlib.h"


void find_root(OSV osv_dep, double jd_dep, Body *dep_body, Body *arr_body, CelestSystem *system, double dt0, double dt1, double max_depdv, double dep_periapsis, double *left_x, double *right_x);

double calc_next_x(DataArray2 *arr, int index_0);

DataArray2 * calc_porkchop_line(Body *dep_body, Body *arr_body, CelestSystem *system, double jd_dep, double min_dur, double max_dur, double dep_periapsis, double max_depdv);
DataArray2 * calc_porkchop_line_static(Body *dep_body, Body *arr_body, CelestSystem *system, double jd_dep, double min_dur, double max_dur, double dep_periapsis, int num_points);


#endif //KMAT_ITIN_REWORK_TOOLS_H