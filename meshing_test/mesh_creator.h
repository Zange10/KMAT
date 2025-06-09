#ifndef KSP_MESH_CREATOR_H
#define KSP_MESH_CREATOR_H

#include "tools/analytic_geometry.h"
#include "orbit_calculator/itin_tool.h"
#include <stdlib.h>

struct PcMeshTriangle;

struct PcMeshGroup;

typedef struct PcMeshPoint {
	struct Vector data;
	struct PorkchopPoint porkchop_point;
	struct PcMeshTriangle **triangles;
	struct PcMeshGroup *group;
	size_t num_triangles;
	size_t max_num_triangles;
	int is_edge;
	int is_artificial;
} PcMeshPoint;

typedef struct PcMeshGroups {
	struct PcMeshGroup **groups;
	int num_groups;
} PcMeshGroups;

typedef struct PcMeshGroup {
	int group_id;
	PcMeshPoint *points;
	size_t num_points;
	size_t num_deps;
	size_t *num_itins_per_dep;
	size_t num_alloc_points;
} PcMeshGroup;

enum TriangleDebugStatus {TRI_FLAG_01_IS_EDGE, TRI_FLAG_12_IS_EDGE, TRI_FLAG_20_IS_EDGE, TRI_FLAG_01_IS_LONG, TRI_FLAG_12_IS_LONG, TRI_FLAG_20_IS_LONG, TRI_FLAG_SAVED_BIG, TRI_FLAG_IS_NEW};

typedef u_int8_t pc_mesh_point_flags;

typedef struct PcMeshTriangle {
	PcMeshPoint *points[3];
	pc_mesh_point_flags point_flags;
} PcMeshTriangle;

typedef struct PcMeshGrid {
	PcMeshPoint ***points;
	size_t num_cols;
	size_t *num_col_rows;
} PcMeshGrid;

typedef struct PcMesh {
	PcMeshTriangle **triangles;
	PcMeshPoint **points;
	size_t num_triangles;
	size_t max_num_triangles;
	size_t num_points;
	size_t max_num_points;
} PcMesh;

int is_triangle_edge(PcMeshTriangle triangle);
int is_triangle_big(PcMeshTriangle triangle);
PcMesh create_pcmesh_from_grid(PcMeshGrid grid);
PcMeshGroups create_pcmesh_groups_grom_porkchop(struct PorkchopPoint *porkchop_points, int num_deps, int *num_itins_per_dep);
PcMeshGrid create_pcmesh_grid_from_porkchop(struct PorkchopPoint *porkchop_points, int num_deps, int *num_itins_per_dep);
PcMeshGrid create_pcmesh_grid_from_pcmesh_group(PcMeshGroup *group);
PcMesh mesh_from_porkchop(struct PorkchopPoint *porkchop_points, int num_itins, int num_deps, int *num_itins_per_dep);
void reduce_pcmesh_big_triangles(PcMesh *mesh, struct Dv_Filter dv_filter);
void resize_pcmesh_to_fit(PcMesh mesh, double max_x, double max_y, double max_z);
void convert_pcmesh_to_total_dur(PcMesh mesh);
void remove_triangle_from_pcmesh(PcMesh *mesh, int tri_idx);
void fine_mesh_around_edge(PcMesh *mesh, double max_dist, double min_dist, struct Dv_Filter dv_filter);

#endif //KSP_MESH_CREATOR_H
