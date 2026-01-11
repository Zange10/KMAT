#ifndef KMAT_MESH_H
#define KMAT_MESH_H

#include "geometrylib.h"


typedef struct MeshPoint2 MeshPoint2;
typedef struct MeshTriangle2 MeshTriangle2;
typedef struct MeshGrid2 MeshGrid2;
typedef struct Mesh2 Mesh2;



struct MeshPoint2 {
	Vector2 pos;
	MeshTriangle2 **triangles;
	size_t num_triangles;
	size_t triangle_cap;
	void *data;
};

struct MeshTriangle2 {
	MeshPoint2 *points[3];
	MeshTriangle2 *adj_triangles[3];
};

struct MeshGrid2 {
	MeshPoint2 ***points;
	size_t num_cols;
	size_t *num_col_rows;
};

struct Mesh2 {
	MeshTriangle2 **triangles;
	MeshPoint2 **points;
	size_t num_triangles;
	size_t triangle_cap;
	size_t num_points;
	size_t point_cap;
};

bool triangle_is_edge(MeshTriangle2 *triangle);
MeshGrid2 create_mesh_grid(DataArray2 *pos, void **data);
Mesh2 create_mesh_from_grid(MeshGrid2 grid);
Mesh2 create_mesh_from_grid_w_angled_guideline(MeshGrid2 grid, double gradient);
void remove_triangle_from_mesh(Mesh2 *mesh, int tri_idx);
void free_grid_keep_points(MeshGrid2 *grid);
void free_mesh(Mesh2 *mesh, void (*free_data_func)(void *data));

#endif //KMAT_MESH_H