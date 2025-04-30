#ifndef KSP_MESH_CREATOR_H
#define KSP_MESH_CREATOR_H

#include "tools/analytic_geometry.h"
#include <stdlib.h>

typedef struct {
	struct Vector points[3];
} MeshTriangle;

typedef struct {
	struct Vector **points;
	size_t num_columns;
	size_t *num_points;
} MeshGrid;

typedef struct {
	MeshTriangle *triangles;
	size_t num_triangles;
} Mesh;


Mesh create_mesh_from_grid(MeshGrid grid);
MeshGrid create_mesh_grid(double *x_vals, double **y_vals, double **z_vals, int num_cols, int *num_points);


#endif //KSP_MESH_CREATOR_H
