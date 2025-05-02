#ifndef KSP_MESH_CREATOR_H
#define KSP_MESH_CREATOR_H

#include "tools/analytic_geometry.h"
#include "orbit_calculator/itin_tool.h"
#include <stdlib.h>

struct PcMeshTriangle;

typedef struct PcMeshPoint {
	struct Vector data;
	struct PorkchopPoint porkchop_point;
	struct PcMeshTriangle **triangles;
	size_t num_triangles;
	size_t max_num_triangles;
	int is_edge;
} PcMeshPoint;

enum TriangleDebugStatus {TRI_FLAG_01_IS_EDGE, TRI_FLAG_12_IS_EDGE, TRI_FLAG_20_IS_EDGE, TRI_FLAG_01_IS_LONG, TRI_FLAG_12_IS_LONG, TRI_FLAG_20_IS_LONG};

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
	PcMeshTriangle *triangles;
	PcMeshPoint **points;
	size_t num_triangles;
	size_t max_num_triangles;
	size_t num_points;
	size_t max_num_points;
} PcMesh;

int is_triangle_edge(PcMeshTriangle triangle);
int is_triangle_big(PcMeshTriangle triangle);
PcMesh create_pcmesh_from_grid(PcMeshGrid grid);
PcMeshGrid create_pcmesh_grid_from_porkchop(struct PorkchopPoint *porkchop_points, int num_deps, int *num_itins_per_dep);
PcMesh mesh_from_porkchop(struct PorkchopPoint *porkchop_points, int num_itins, int num_deps, int *num_itins_per_dep);
void resize_mesh_to_fit(PcMesh mesh, double max_x, double max_y, double max_z);

#endif //KSP_MESH_CREATOR_H
