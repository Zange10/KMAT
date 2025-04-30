#include "mesh_creator.h"
#include "orbit_calculator/itin_tool.h"


double vector_distance(struct Vector2D v0, struct Vector2D v1) {
	struct Vector2D vdiff = {v1.x - v0.x, v1.y-v0.y};
	return vector2d_mag(vdiff);
}

MeshTriangle create_triangle_from_three_points(struct Vector p1, struct Vector p2, struct Vector p3) {
	MeshTriangle triangle;
	triangle.points[0] = p1;
	triangle.points[1] = p2;
	triangle.points[2] = p3;
	return triangle;
}

MeshGrid create_mesh_grid(double *x_vals, double **y_vals, double **z_vals, int num_cols, int *num_points) {
	MeshGrid grid = {.num_columns = num_cols};
	grid.num_points = malloc(sizeof(size_t) * num_cols);
	grid.points = malloc(sizeof(struct Vector*) * num_cols);
	for(int i = 0; i < num_cols; i++) {
		grid.num_points[i] = num_points[i];
		grid.points[i] = malloc(sizeof(struct Vector) * num_points[i]);
		for(int j = 0; j < num_points[i]; j++) {
			grid.points[i][j] = (struct Vector) {.x = x_vals[i], .y = y_vals[i][j], .z = z_vals[i][j]};
		}
	}

	return grid;
};

Mesh create_mesh_from_grid(MeshGrid grid) {
	Mesh mesh;
	mesh.num_triangles = 0;
	mesh.triangles = malloc(1000*sizeof(MeshTriangle));
	for(int x_idx = 0; x_idx < grid.num_columns-1; x_idx++) {
		int y_idx0 = 0, y_idx1 = 0;
		while(y_idx0 < grid.num_points[x_idx] - 1 && y_idx1 < grid.num_points[x_idx + 1] - 1) {
			struct Vector p1 = grid.points[x_idx][y_idx0];
			struct Vector p2 = grid.points[1 + x_idx][y_idx1];
			struct Vector p3 = grid.points[x_idx][1 + y_idx0];
			struct Vector p4 = grid.points[1 + x_idx][1 + y_idx1];

			if(vector_distance(vec2D(p1.x, p1.y), vec2D(p4.x, p4.y)) < vector_distance(vec2D(p2.x, p2.y), vec2D(p3.x, p3.y))) {
				mesh.triangles[mesh.num_triangles] = create_triangle_from_three_points(p1, p2, p4);
				y_idx1++;
			} else {
				mesh.triangles[mesh.num_triangles] = create_triangle_from_three_points(p1, p2, p3);
				y_idx0++;
			}
			mesh.num_triangles++;

		}

		if(y_idx0 == grid.num_points[x_idx] - 1) {
			while(y_idx1 < grid.num_points[x_idx + 1] - 1) {
				mesh.triangles[mesh.num_triangles] = create_triangle_from_three_points(grid.points[x_idx][y_idx0], grid.points[x_idx + 1][y_idx1], grid.points[x_idx + 1][y_idx1 + 1]);
				y_idx1++;
				mesh.num_triangles++;
			}
		}

		if(y_idx1 == grid.num_points[x_idx + 1] - 1) {
			while(y_idx0 < grid.num_points[x_idx] - 1) {
				mesh.triangles[mesh.num_triangles] = create_triangle_from_three_points(grid.points[x_idx][y_idx0], grid.points[x_idx][y_idx0 + 1], grid.points[x_idx + 1][y_idx1]);
				y_idx0++;
				mesh.num_triangles++;
			}
		}
	}

	return mesh;
}