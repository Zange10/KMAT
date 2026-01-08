#ifndef KMAT_MESH_H
#define KMAT_MESH_H

#include "geometrylib.h"

struct MeshTriangle2;

typedef struct MeshPoint2 {
	Vector2 pos;
	struct MeshTriangle2 **triangles;
	size_t num_triangles;
	size_t triangle_cap;
	int is_edge;
	void *data;
} MeshPoint2;

enum TriangleDebugStatus {TRI_FLAG_01_IS_EDGE, TRI_FLAG_12_IS_EDGE, TRI_FLAG_20_IS_EDGE};

typedef u_int8_t mesh_triangle_flags;

typedef struct MeshTriangle2 {
	MeshPoint2 *points[3];
	mesh_triangle_flags point_flags;
} MeshTriangle2;

typedef struct MeshGrid2 {
	MeshPoint2 ***points;
	size_t num_cols;
	size_t *num_col_rows;
} MeshGrid2;

typedef struct Mesh2 {
	MeshTriangle2 **triangles;
	MeshPoint2 **points;
	size_t num_triangles;
	size_t max_num_triangles;
	size_t num_points;
	size_t max_num_points;
} Mesh2;

int is_triangle_edge(MeshTriangle2 triangle);
MeshGrid2 create_mesh_grid(DataArray2 *pos, void **data);
Mesh2 create_mesh_from_grid(MeshGrid2 grid);
Mesh2 create_mesh_from_grid_w_angled_guideline(MeshGrid2 grid, double gradient);
void remove_triangle_from_mesh(Mesh2 *mesh, int tri_idx);

#endif //KMAT_MESH_H