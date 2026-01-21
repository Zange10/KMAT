#ifndef KMAT_ITIN_REWORK_TOOLS_H
#define KMAT_ITIN_REWORK_TOOLS_H


#include "orbitlib.h"
#include "orbit_calculator/itin_tool.h"
#include "mesh.h"


typedef struct DepartureGroup {
	struct ItinStep **departures;
	int num_departures;
	Body *dep_body, *arr_body;
	CelestSystem *system;
	Vector2 boundary0_top;
	Vector2 boundary0_bottom;
	double boundary_gradient;
	struct DepartureGroup **next_groups;
	enum DepartureGroupBoundaryType {DEPARTURE_GROUP_BOUNDARY_TOP_OPP, DEPARTURE_GROUP_BOUNDARY_TOP_CONJ} top_boundary_type;
} DepartureGroup;

typedef struct FlyByGroup {
	DataArray2 *dep_dur;
	struct ItinStep **left_steps;
	struct ItinStep **right_steps;
	int step_cap;
} FlyByGroup;

typedef struct FlyByGroups {
	FlyByGroup **groups;
	int num_groups;
	int group_cap;
	int *num_groups_dep;
} FlyByGroups;

void find_root(OSV osv_dep, double jd_dep, Body *dep_body, Body *arr_body, CelestSystem *system, double dt0, double dt1, double max_depdv, double dep_periapsis, double *left_x, double *right_x);

DataArray2 * calc_porkchop_line(struct ItinStep *step, Body *dep_body, Body *arr_body, CelestSystem *system, double jd_dep, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance);
DataArray2 * calc_porkchop_line_static(Body *dep_body, Body *arr_body, CelestSystem *system, double jd_dep, double min_dur, double max_dur, double dep_periapsis, int num_points);
void calc_group_porkchop(DepartureGroup *group, int shift, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance);
DataArray2 * calc_min_vinf_line(DepartureGroup *group, int shift, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double vinf_tolerance);

double calc_opposition_conjunction_gradient(Body *dep_body, Body *arr_body, CelestSystem *system, double jd_dep);

DataArray2 * get_dur_limits_from_edge_triangles(Mesh2 *mesh);
DataArray2 * get_dur_limits_for_dep_from_point_list(DataArray2 *edges_array, double jd_dep);
void get_dur_limit_wrt_vinf(Mesh2 *mesh, double jd_dep, double min_vinf, DataArray2 *init_limit_array, DataArray2 *new_limits, double tolerance);
Vector3 get_varr_from_mesh(Mesh2 *mesh, double jd_arr, double dur);
Vector3 get_vbody_from_mesh(Mesh2 *mesh, double jd_arr, double dur);
struct ItinStep * get_next_step_from_vinf(DepartureGroup *group, double v_inf, double jd_dep, double min_dur_dt, double max_dur_dt, bool leftside, double tolerance);
DataArray2 * get_vinf_limits(Mesh2 *mesh, DataArray2 *vinf_array, double tolerance);
FlyByGroups * get_flyby_groups_wrt_vinf(Mesh2 *mesh, DepartureGroup *departure_group, DataArray2 *vinf_limits, double tolerance);
Mesh2 * get_rpe_mesh_from_fb_groups(FlyByGroups *fb_groups, Mesh2 *prev_mesh, DepartureGroup *prev_departure_group, bool left_side);


#endif //KMAT_ITIN_REWORK_TOOLS_H