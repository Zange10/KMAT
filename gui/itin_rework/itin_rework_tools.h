#ifndef KMAT_ITIN_REWORK_TOOLS_H
#define KMAT_ITIN_REWORK_TOOLS_H


#include "orbitlib.h"
#include "orbit_calculator/itin_tool.h"


typedef struct DepartureGroup {
	struct ItinStep **departures;
	int num_departures;
	Body *dep_body, *arr_body;
	CelestSystem *system;
} DepartureGroup;

void find_root(OSV osv_dep, double jd_dep, Body *dep_body, Body *arr_body, CelestSystem *system, double dt0, double dt1, double max_depdv, double dep_periapsis, double *left_x, double *right_x);

DataArray2 * calc_porkchop_line(struct ItinStep *step, Body *dep_body, Body *arr_body, CelestSystem *system, double jd_dep, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance);
DataArray2 * calc_porkchop_line_static(Body *dep_body, Body *arr_body, CelestSystem *system, double jd_dep, double min_dur, double max_dur, double dep_periapsis, int num_points);
void calc_group_porkchop(DepartureGroup *group, int shift, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance);


double calc_opposition_conjunction_gradient(Body *dep_body, Body *arr_body, CelestSystem *system, double jd_dep);

#endif //KMAT_ITIN_REWORK_TOOLS_H