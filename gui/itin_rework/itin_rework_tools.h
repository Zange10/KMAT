#ifndef KMAT_ITIN_REWORK_TOOLS_H
#define KMAT_ITIN_REWORK_TOOLS_H


#include "boundary.h"
#include "orbitlib.h"
#include "orbit_calculator/itin_tool.h"
#include "mesh.h"
#include "quad.h"


typedef enum PorkchopMeshValueType {
	MESH_VAL_ARRDATE,
	MESH_VAL_DUR,
	MESH_VAL_DEPX,
	MESH_VAL_DEPY,
	MESH_VAL_DEPZ,
	MESH_VAL_BODY_RX,
	MESH_VAL_BODY_RY,
	MESH_VAL_BODY_RZ,
	MESH_VAL_BODY_VX,
	MESH_VAL_BODY_VY,
	MESH_VAL_BODY_VZ,
	MESH_VAL_ARRX,
	MESH_VAL_ARRY,
	MESH_VAL_ARRZ,
	MESH_VAL_ARRVINF,
	MESH_VAL_RPE,
	NUM_PORKCHOP_MESH_VALUE_TYPES	// number of porkchop mesh vals
} PorkchopMeshValueType;

typedef enum LambertBranch {
	LAMBERT_BRANCH_LEFT,
	LAMBERT_BRANCH_RIGHT
} LambertBranch;

typedef struct VinfStruct {
	double jd_dep;
	DataArray2 *vinf_array;
	int min_vinf_idx;
} VinfStruct;

typedef struct VinfStructArray {
	VinfStruct *vinf_arr;
	size_t num, cap;
	DataArray2 *vinf_line;
	DataArray2 *dur_line;
} VinfStructArray;

typedef struct DepartureGroup {
	Body *dep_body;
	CelestSystem *system;
	int num_next_groups;
	int group_cap;
	struct SegmentGroup **segment_groups;
} DepartureGroup;

typedef struct SegmentGroup {
	Body *dep_body, *arr_body;
	CelestSystem *system;
	double boundary_gradient;
	Boundary group_bdr, dv_bdr, vinf_bdr, rpe_bdr;
	DataArray2 *vinf_array;
	VinfStructArray vinf_struct_array;
	LambertBranch lam_branch;
	int num_next_groups;
	int group_cap;
	struct SegmentGroup *prev;
	struct SegmentGroup **next;
	Mesh2 *mesh;
	Quad *quad;
	int min_rf_level, max_rf_level;
	enum DepartureGroupBoundaryType {DEPARTURE_GROUP_BOUNDARY_TOP_OPP, DEPARTURE_GROUP_BOUNDARY_TOP_CONJ} top_boundary_type;
} SegmentGroup;

SegmentGroup * new_segment_group(Body *dep_body, Body *arr_body, CelestSystem *system);
void append_to_segment_group(SegmentGroup *group, SegmentGroup *new_group);
void free_segment_group(SegmentGroup *group);

void find_lambert_root(OSV osv_dep, double jd_dep, Body *dep_body, Body *arr_body, CelestSystem *system, double dt0, double dt1, double max_depdv, double dep_periapsis, double *left_x, double *right_x, double tol);
double find_segment_group_lambert_root(double jd_dep, SegmentGroup *group, double target_vinf, double min_dur, double max_dur, double tol);

DataArray2 * find_local_min_vinf_array(double jd_dep, Body *dep_body, Body *arr_body, CelestSystem *system, double dt0, double dt1, double tol);
DataArray2 * find_local_max_vinf_array(double jd_dep, Body *dep_body, Body *arr_body, CelestSystem *system, double dt0, double dt1, double tol);
DataArray2 * find_local_vinf_peak_array(double jd_dep, Body *dep_body, Body *arr_body, CelestSystem *system, double dt0, double dt1, double tol, bool max_0_min_1);
Vector2 get_local_peak(double jd_dep, Body *dep_body, Body *arr_body, CelestSystem *system, double dt0, double dt1, double tol, bool max_0_min_1);
double find_local_opp_conj(Body *dep_body, Body *arr_body, CelestSystem *system, double jd_dep, double min_dur, double max_dur);

void get_prev_and_next_relative_plane_traversal(Body *body0, Body *body1, CelestSystem *system, double jd_date, double *prev_trav, double *next_trav);
double calc_opposition_conjunction_gradient(Body *dep_body, Body *arr_body, CelestSystem *system, double jd_dep);
int get_opp_conj_min_shift(Body *dep_body, Body *arr_body, CelestSystem *system, double jd_min_dep, double jd_max_dep, double min_dur, double max_dur);
void set_opposition_conjunction_group_boundary(SegmentGroup *group, int shift, double jd_min_dep, double jd_max_dep, double min_dur, double max_dur, bool cut_at_durminmax);
void set_opposition_conjunction_group_boundary2(SegmentGroup *group, int shift, double jd_min_dep, double jd_max_dep, double min_dur, double max_dur);


Boundary calc_dv_boundary(SegmentGroup *group, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance);

DataArray2 * calc_min_vinf_line(SegmentGroup *group, double jd_min_dep, double jd_max_dep, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance);
VinfStructArray calc_min_vinf_line2(SegmentGroup *group, double jd_min_dep, double jd_max_dep, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance);
DataArray2 * get_vinf_array_for_departure(QuadList *quads_at_x, double jd_dep);
DataArray2 * get_min_vinf_array_for_departure(QuadList *quads_at_x, double jd_dep, DataArray2 *min_vinf_array, double dv_tolerance, double min_dur, double max_dur);
void calc_vinf_boundary(SegmentGroup *dep_group, SegmentGroup *group, Quad *quad, DataArray2 *min_vinf_array, double dv_tolerance);



// ###################################################################
// OLD
// ###################################################################

// DataArray2 * calc_dv_boundary(SegmentGroup *group, int departure_cap, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance);
// void calc_porkchop_dv_boundaries(SegmentGroup *group, int departure_cap, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance);
// void calc_bounded_porkchop_line(struct ItinStep *departure_step, Body *arr_body, CelestSystem *system, DataArray1 *dur_array, double min_dt, double max_dt, double dep_periapsis, double max_depdv, double dv_tolerance);
// void calc_group_porkchop(SegmentGroup *group, int departure_cap, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance);
// void calc_group_porkchop_outline(SegmentGroup *group, int departure_cap, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance);
// void calc_coarse_group_porkchop(SegmentGroup *group, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, int num_duration_steps, double dep_periapsis, double max_depdv, double dv_tolerance);
// void calc_group_porkchop_subgrid(SegmentGroup *group, MeshGrid2 *grid, size_t *grid_col_cap, int insert_col_idx, double jd_max_arr, double min_dt, double max_dt, double dep_periapsis, double max_depdv, double dv_tolerance);
//
// void refine_porkchop_mesh(SegmentGroup *group, double dep_periapsis, double max_depdv, double dv_tolerance);
// void update_mesh_triangle_status(SegmentGroup *group, double dv_tolerance);
//
// DataArray2 * get_dur_limits_from_edge_triangles(Mesh2 *mesh);
// DataArray2 * get_dur_limits_for_dep_from_point_list(DataArray2 *edges_array, double jd_dep);
// void get_dur_limit_wrt_vinf(Mesh2 *mesh, double jd_dep, double min_vinf, DataArray2 *init_limit_array, DataArray2 *new_limits, double tolerance);
// Vector3 get_varr_from_mesh(Mesh2 *mesh, double jd_arr, double dur);
// Vector3 get_vbody_from_mesh(Mesh2 *mesh, double jd_arr, double dur);
// struct ItinStep * get_next_step_from_vinf(SegmentGroup *group, double v_inf, double jd_dep, double min_dur_dt, double max_dur_dt, bool leftside, double tolerance);
// DataArray2 * get_vinf_limits(Mesh2 *mesh, DataArray2 *vinf_array, double tolerance);


#endif //KMAT_ITIN_REWORK_TOOLS_H
