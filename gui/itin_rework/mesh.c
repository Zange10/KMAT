#include "mesh.h"


int is_triangle_edge(MeshTriangle2 triangle) {
	return (triangle.point_flags >> TRI_FLAG_01_IS_EDGE & 1) || (triangle.point_flags >> TRI_FLAG_12_IS_EDGE & 1) || (triangle.point_flags >> TRI_FLAG_20_IS_EDGE & 1);
}

void add_edge_flag_to_triangle(MeshTriangle2 *triangle, MeshPoint2 *p0, MeshPoint2 *p1) {
	int idx0 = -1, idx1 = -1;
	for(int i = 0; i < 3; i++) if(triangle->points[i] == p0) {idx0 = i; break;}
	for(int i = 0; i < 3; i++) if(triangle->points[i] == p1) {idx1 = i; break;}

	if (idx0 < 0 || idx1 < 0) return;

	if(idx0 > idx1) {
		int temp = idx0;
		idx0 = idx1;
		idx1 = temp;
	}

	if(idx0 == 0) {
		if(idx1 == 1) {
			triangle->point_flags |= 1 << TRI_FLAG_01_IS_EDGE;
		} else {
			triangle->point_flags |= 1 << TRI_FLAG_20_IS_EDGE;
		}
	} else {
		triangle->point_flags |= 1 << TRI_FLAG_12_IS_EDGE;
	}
}

void remove_edge_flag_from_triangle(MeshTriangle2 *triangle, MeshPoint2 *p0, MeshPoint2 *p1) {
	int idx0 = -1, idx1 = -1;
	for(int i = 0; i < 3; i++) if(triangle->points[i] == p0) {idx0 = i; break;}
	for(int i = 0; i < 3; i++) if(triangle->points[i] == p1) {idx1 = i; break;}

	if(idx0 < 0 || idx1 < 0) return;

	if(idx0 > idx1) {
		int temp = idx0;
		idx0 = idx1;
		idx1 = temp;
	}

	if(idx0 == 0) {
		if(idx1 == 1) {
			triangle->point_flags &= ~(1 << TRI_FLAG_01_IS_EDGE);
		} else {
			triangle->point_flags &= ~(1 << TRI_FLAG_20_IS_EDGE);
		}
	} else {
		triangle->point_flags &= ~(1 << TRI_FLAG_12_IS_EDGE);
	}
}

void update_point_edges(MeshPoint2 *p0, MeshPoint2 *p1, int add_flag) {
	MeshTriangle2 *temp_triangle = NULL;
	for(int i = 0; i < p0->num_triangles; i++) {
		for(int j = 0; j < p1->num_triangles; j++) {
			if(p0->triangles[i] == p1->triangles[j]) {
				if(temp_triangle == NULL) temp_triangle = p0->triangles[i];
				else {
					if(add_flag) {
						add_edge_flag_to_triangle(temp_triangle, p0, p1);
						add_edge_flag_to_triangle(p0->triangles[i], p0, p1);
					} else {
						remove_edge_flag_from_triangle(temp_triangle, p0, p1);
						remove_edge_flag_from_triangle(p0->triangles[i], p0, p1);
					}
				}
			}
		}
	}
}

void remove_all_triangle_edge_flags_where_adjacents(MeshTriangle2 *triangle) {
	if((triangle->point_flags >> TRI_FLAG_01_IS_EDGE & 1)) update_point_edges(triangle->points[0], triangle->points[1], 0);
	if((triangle->point_flags >> TRI_FLAG_12_IS_EDGE & 1)) update_point_edges(triangle->points[1], triangle->points[2], 0);
	if((triangle->point_flags >> TRI_FLAG_20_IS_EDGE & 1)) update_point_edges(triangle->points[2], triangle->points[0], 0);
}

MeshTriangle2 * create_triangle_from_three_points(MeshPoint2 *p0, MeshPoint2 *p1, MeshPoint2 *p2) {
	MeshTriangle2 *triangle = malloc(sizeof(MeshTriangle2));
	triangle->points[0] = p0;
	triangle->points[1] = p1;
	triangle->points[2] = p2;
	triangle->point_flags = 0;

	for (int i = 0; i < 3; i++) {
		MeshPoint2 *p = i == 0 ? p0 : i == 1 ? p1 : p2;
		if (!p->triangles) {
			p->triangle_cap = 8;
			p->triangles = malloc(p->triangle_cap * sizeof(MeshTriangle2*));
		}
		if (p->num_triangles == p->triangle_cap) {
			p->triangle_cap *= 2;
			MeshTriangle2 **temp = realloc(p->triangles, p->triangle_cap * sizeof(MeshTriangle2*));
			if (temp) p->triangles = temp;
		}
		p->triangles[p->num_triangles] = triangle; p->num_triangles++;
	}

	if(p0->is_edge && p1->is_edge) triangle->point_flags |= (1 << TRI_FLAG_01_IS_EDGE);
	if(p1->is_edge && p2->is_edge) triangle->point_flags |= (1 << TRI_FLAG_12_IS_EDGE);
	if(p0->is_edge && p2->is_edge) triangle->point_flags |= (1 << TRI_FLAG_20_IS_EDGE);

	if(is_triangle_edge(*triangle)) remove_all_triangle_edge_flags_where_adjacents(triangle);

	return triangle;
}

void remove_triangle_from_mesh(Mesh2 *mesh, int tri_idx) {
	for(int i = 0; i < 3; i++) update_point_edges(mesh->triangles[tri_idx]->points[i], mesh->triangles[tri_idx]->points[(i+1)%3], 1);
	for(int i = 0; i < 3; i++) {
		update_point_edges(mesh->triangles[tri_idx]->points[i], mesh->triangles[tri_idx]->points[(i+1)%3], 1);
		MeshPoint2 *point = mesh->triangles[tri_idx]->points[i];
		point->is_edge = 1;
		for(int j = 0; j < mesh->triangles[tri_idx]->points[i]->num_triangles; j++) {
			if(point->triangles[j] == mesh->triangles[tri_idx]) {
				point->triangles[j] = point->triangles[point->num_triangles-1];
				point->num_triangles--;
				break;
			}
		}
	}

	free(mesh->triangles[tri_idx]);
	mesh->triangles[tri_idx] = mesh->triangles[mesh->num_triangles-1];
	mesh->num_triangles--;
}


MeshGrid2 create_mesh_grid(DataArray2 *pos, void **data) {
	MeshGrid2 grid;
	int col_cap = 8;
	grid.num_cols = 0;
	grid.num_col_rows = malloc(col_cap * sizeof(size_t));
	grid.points = malloc(col_cap * sizeof(MeshPoint2**));

	Vector2 *pos_data = data_array2_get_data(pos);
	int row_cap = 8;

	for (int i = 0; i < data_array2_size(pos); i++) {
		if (i == 0 || pos_data[i].x != pos_data[i-1].x) {
			grid.num_cols++;
			if (grid.num_cols > col_cap) {
				col_cap *= 2;
				void *temp = realloc(grid.points, col_cap * sizeof(MeshPoint2**));
				if (temp) grid.points = temp;
				temp = realloc(grid.num_col_rows, col_cap * sizeof(size_t));
				if (temp) grid.num_col_rows = temp;
			}

			row_cap = 8;
			grid.points[grid.num_cols-1] = malloc(row_cap * sizeof(MeshPoint2*));
			grid.num_col_rows[grid.num_cols-1] = 0;
		}

		int col = (int) grid.num_cols-1;

		if (grid.num_col_rows[col] > row_cap) {
			row_cap *= 2;
			void *temp = realloc(grid.points[col], row_cap * sizeof(MeshPoint2*));
			if (temp) grid.points[col] = temp;
		}

		int row = (int) grid.num_col_rows[col];

		grid.points[col][row] = malloc(sizeof(MeshPoint2));
		grid.points[col][row]->pos = pos_data[i];
		grid.points[col][row]->data = data[i];
		grid.points[col][row]->num_triangles = 0;
		grid.points[col][row]->is_edge = 0;
		grid.points[col][row]->triangle_cap = 0;
		grid.points[col][row]->triangles = NULL;
		grid.num_col_rows[col]++;
	}

	return grid;
}

Mesh2 create_mesh_from_grid(MeshGrid2 grid) {
	Mesh2 mesh;
	mesh.num_triangles = 0;
	mesh.max_num_triangles = 1000000;
	mesh.num_points = 0;
	mesh.max_num_points = 1000000;
	mesh.triangles = malloc(mesh.max_num_triangles*sizeof(MeshTriangle2*));
	mesh.points = malloc(mesh.max_num_points*sizeof(MeshPoint2*));

	for(int i = 0; i < grid.num_cols; i++) {
		for(int j = 0; j < grid.num_col_rows[i]; j++) {
			mesh.points[mesh.num_points] = grid.points[i][j];
			mesh.num_points++;
		}
	}

	for(int x_idx = 0; x_idx < grid.num_cols-1; x_idx++) {
		int y_idx0 = 0, y_idx1 = 0;
		while(y_idx0 < grid.num_col_rows[x_idx] - 1 && y_idx1 < grid.num_col_rows[x_idx + 1] - 1) {
			MeshPoint2 *p0 = grid.points[x_idx][y_idx0];
			MeshPoint2 *p1 = grid.points[1 + x_idx][y_idx1];
			MeshPoint2 *p2 = grid.points[x_idx][1 + y_idx0];
			MeshPoint2 *p3 = grid.points[1 + x_idx][1 + y_idx1];

			double sq_dist03 = sq_mag_vec2(subtract_vec2(p0->pos, p3->pos));
			double sq_dist12 = sq_mag_vec2(subtract_vec2(p1->pos, p2->pos));

			if(sq_dist03 < sq_dist12) {
				mesh.triangles[mesh.num_triangles] = create_triangle_from_three_points(p0, p1, p3);
				y_idx1++;
			} else {
				mesh.triangles[mesh.num_triangles] = create_triangle_from_three_points(p0, p1, p2);
				y_idx0++;
			}
			mesh.num_triangles++;

		}

		if(y_idx0 == grid.num_col_rows[x_idx] - 1) {
			while(y_idx1 < grid.num_col_rows[x_idx + 1] - 1) {
				mesh.triangles[mesh.num_triangles] = create_triangle_from_three_points(grid.points[x_idx][y_idx0], grid.points[x_idx + 1][y_idx1], grid.points[x_idx + 1][y_idx1 + 1]);
				y_idx1++;
				mesh.num_triangles++;
			}
		}

		if(y_idx1 == grid.num_col_rows[x_idx + 1] - 1) {
			while(y_idx0 < grid.num_col_rows[x_idx] - 1) {
				mesh.triangles[mesh.num_triangles] = create_triangle_from_three_points(grid.points[x_idx][y_idx0], grid.points[x_idx][y_idx0 + 1], grid.points[x_idx + 1][y_idx1]);
				y_idx0++;
				mesh.num_triangles++;
			}
		}
	}

	return mesh;
}