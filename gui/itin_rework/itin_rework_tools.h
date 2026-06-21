#ifndef KMAT_ITIN_REWORK_TOOLS_H
#define KMAT_ITIN_REWORK_TOOLS_H


#include "orbitlib.h"
#include "orbit_calculator/itin_tool.h"
#include "mesh.h"
#include "quad.h"


typedef enum PorkchopMeshValueType {
	MESH_VAL_DATE,
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
	MESH_VAL_VINF,
	MESH_VAL_RPE,
	NUM_PORKCHOP_MESH_VALUE_TYPES	// number of porkchop mesh vals
} PorkchopMeshValueType;


typedef struct DepartureGroup {
	Body *dep_body;
	CelestSystem *system;
	int num_next_groups;
	int group_cap;
	struct SegmentGroup **segment_groups;
} DepartureGroup;

typedef struct SegmentGroup {
	struct ItinStep **segment_steps;
	int num_steps;
	Body *dep_body, *arr_body;
	CelestSystem *system;
	// Vector2 boundary0_top;
	// Vector2 boundary0_bottom;
	double boundary_gradient;
	DataArray2 *upper_boundary;
	DataArray2 *lower_boundary;
	int num_next_groups;
	int group_cap;
	Mesh2 *mesh;
	DataArray2 *vinf_array;
	struct SegmentGroup *prev;
	struct SegmentGroup **next;
	enum DepartureGroupBoundaryType {DEPARTURE_GROUP_BOUNDARY_TOP_OPP, DEPARTURE_GROUP_BOUNDARY_TOP_CONJ} top_boundary_type;
} SegmentGroup;

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

void find_root(OSV osv_dep, double jd_dep, Body *dep_body, Body *arr_body, CelestSystem *system, double dt0, double dt1, double max_depdv, double dep_periapsis, double *left_x, double *right_x, double tol);
DataArray2 * find_root2(double jd_dep, Body *dep_body, Body *arr_body, CelestSystem *system, double dt0, double dt1, double max_depdv, double dep_periapsis, double *left_x, double *right_x, double tol);
DataArray2 * find_local_peak_array(double jd_dep, Body *dep_body, Body *arr_body, CelestSystem *system, double dt0, double dt1, double tol, bool max_0_min_1);
Vector2 get_local_peak(double jd_dep, Body *dep_body, Body *arr_body, CelestSystem *system, double dt0, double dt1, double tol, bool max_0_min_1);
void get_prev_and_next_relative_plane_traversal(Body *body0, Body *body1, CelestSystem *system, double jd_date, double *prev_trav, double *next_trav);

DataArray2 * calc_dv_boundary(SegmentGroup *group, int departure_cap, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance);
void calc_porkchop_dv_boundaries(SegmentGroup *group, int departure_cap, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance);

void calc_bounded_porkchop_line(struct ItinStep *departure_step, Body *arr_body, CelestSystem *system, DataArray1 *dur_array, double min_dt, double max_dt, double dep_periapsis, double max_depdv, double dv_tolerance);
void calc_group_porkchop(SegmentGroup *group, int departure_cap, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance);
void calc_group_porkchop_outline(SegmentGroup *group, int departure_cap, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance);
void calc_coarse_group_porkchop(SegmentGroup *group, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, int num_duration_steps, double dep_periapsis, double max_depdv, double dv_tolerance);
void calc_group_porkchop_subgrid(SegmentGroup *group, MeshGrid2 *grid, size_t *grid_col_cap, int insert_col_idx, double jd_max_arr, double min_dt, double max_dt, double dep_periapsis, double max_depdv, double dv_tolerance);
DataArray2 * calc_min_vinf_line(SegmentGroup *group, double jd_min_dep, double jd_max_dep, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance);
DataArray2 * get_vinf_array_for_departure(QuadList *quads_at_x, double jd_dep);
DataArray2 * get_min_vinf_array_for_departure(QuadList *quads_at_x, double jd_dep, DataArray2 *min_vinf_array, double dv_tolerance, double min_dur, double max_dur);
DataArray2 * calc_vinf_boundary(SegmentGroup *group, Quad *quad, DataArray2 *min_vinf_array, double dv_tolerance);

void refine_porkchop_mesh(SegmentGroup *group, double dep_periapsis, double max_depdv, double dv_tolerance);
void update_mesh_triangle_status(SegmentGroup *group, double dv_tolerance);

double calc_opposition_conjunction_gradient(Body *dep_body, Body *arr_body, CelestSystem *system, double jd_dep);
void set_opposition_conjunction_group_boundary(SegmentGroup *group, int shift, double jd_min_dep, double jd_max_dep, double min_dur, double max_dur);

DataArray2 * get_dur_limits_from_edge_triangles(Mesh2 *mesh);
DataArray2 * get_dur_limits_for_dep_from_point_list(DataArray2 *edges_array, double jd_dep);
void get_dur_limit_wrt_vinf(Mesh2 *mesh, double jd_dep, double min_vinf, DataArray2 *init_limit_array, DataArray2 *new_limits, double tolerance);
Vector3 get_varr_from_mesh(Mesh2 *mesh, double jd_arr, double dur);
Vector3 get_vbody_from_mesh(Mesh2 *mesh, double jd_arr, double dur);
struct ItinStep * get_next_step_from_vinf(SegmentGroup *group, double v_inf, double jd_dep, double min_dur_dt, double max_dur_dt, bool leftside, double tolerance);
DataArray2 * get_vinf_limits(Mesh2 *mesh, DataArray2 *vinf_array, double tolerance);
FlyByGroups * get_flyby_groups_wrt_vinf(Mesh2 *mesh, SegmentGroup *departure_group, DataArray2 *vinf_limits, double tolerance);
Mesh2 * get_rpe_mesh_from_fb_groups(FlyByGroups *fb_groups, Mesh2 *prev_mesh, SegmentGroup *prev_departure_group, bool left_side);
FlyByGroups * get_refined_departure_groups(SegmentGroup *departure_group, DataArray2 *limits, double dep_periapsis, double max_dep_dv, double dv_tolerance);
Mesh2 * get_dep_mesh_from_fb_groups(FlyByGroups *fb_groups, SegmentGroup *departure_group);

#endif //KMAT_ITIN_REWORK_TOOLS_H