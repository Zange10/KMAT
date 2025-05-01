#ifndef KSP_MESH_CREATOR_H
#define KSP_MESH_CREATOR_H

#include "tools/analytic_geometry.h"
#include "orbit_calculator/itin_tool.h"
#include <stdlib.h>

typedef struct {
	struct Vector points[3];
} MeshTriangle;

typedef struct {
	struct Vector **points;
	struct Vector2D *outside_points;
	size_t num_columns;
	size_t num_outside_points;
	size_t *num_points;
} MeshGrid;

typedef struct {
	MeshTriangle *triangles;
	size_t num_triangles;
} Mesh;


Mesh create_mesh_from_grid(MeshGrid grid, double max_x_diff);
MeshGrid create_mesh_grid(double *x_vals, double **y_vals, double **z_vals, int num_cols, int *num_points);
MeshGrid grid_from_porkchop(struct PorkchopPoint *porkchop_points, int num_itins, int num_deps, int *num_itins_per_dep);
Mesh mesh_from_porkchop(struct PorkchopPoint *porkchop_points, int num_itins, int num_deps, int *num_itins_per_dep);


#endif //KSP_MESH_CREATOR_H
