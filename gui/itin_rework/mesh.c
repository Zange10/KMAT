#include "mesh.h"

#include <math.h>
#include <stdio.h>
#include <string.h>


Mesh2 * new_mesh() {
	Mesh2 *mesh = malloc(sizeof(Mesh2));
	mesh->num_triangles = 0;
	mesh->triangle_cap = 64;
	mesh->num_points = 0;
	mesh->point_cap = 64;
	mesh->triangles = malloc(mesh->triangle_cap*sizeof(MeshTriangle2*));
	mesh->points = malloc(mesh->point_cap*sizeof(MeshPoint2*));
	mesh->mesh_box = malloc(sizeof(MeshBox2));
	mesh->mesh_box->type = MESHBOX_SUBBOXES;
	mesh->mesh_box->subboxes.num = 0;
	mesh->mesh_box->subboxes.cap = 4;
	mesh->mesh_box->subboxes.boxes = malloc(mesh->mesh_box->subboxes.cap*sizeof(MeshBox2*));
	mesh->mesh_box->min = vec2(NAN, NAN);
	mesh->mesh_box->max = vec2(NAN, NAN);
	return mesh;
}

bool triangle_is_edge(MeshTriangle2 *triangle) {
	return !triangle->adj_triangles[0] || !triangle->adj_triangles[1] || !triangle->adj_triangles[2];
}

void find_2dtriangle_minmax(MeshTriangle2 *triangle, double *min_x, double *max_x, double *min_y, double *max_y) {
	*min_x = triangle->points[0]->pos.x;
	*max_x = triangle->points[0]->pos.x;
	*min_y = triangle->points[0]->pos.y;
	*max_y = triangle->points[0]->pos.y;

	for(int i = 1; i < 3; i++) {
		if(triangle->points[i]->pos.x < *min_x) *min_x = triangle->points[i]->pos.x;
		if(triangle->points[i]->pos.x > *max_x) *max_x = triangle->points[i]->pos.x;
		if(triangle->points[i]->pos.y < *min_y) *min_y = triangle->points[i]->pos.y;
		if(triangle->points[i]->pos.y > *max_y) *max_y = triangle->points[i]->pos.y;
	}
}

int is_inside_triangle(MeshTriangle2 *triangle, Vector2 p) {
	Vector2 a = triangle->points[0]->pos;
	Vector2 b = triangle->points[1]->pos;
	Vector2 c = triangle->points[2]->pos;
	Vector2 v0 = subtract_vec2(c,a);
	Vector2 v1 = subtract_vec2(b,a);
	Vector2 v2 = subtract_vec2(p,a);

	double d00 = dot_vec2(v0, v0);
	double d01 = dot_vec2(v0, v1);
	double d11 = dot_vec2(v1, v1);
	double d20 = dot_vec2(v2, v0);
	double d21 = dot_vec2(v2, v1);

	double denom = d00*d11 - d01*d01;

	double u = (d11*d20 - d01*d21) / denom;
	double v = (d00*d21 - d01*d20) / denom;

	return (u >= 0 && v >= 0 && u+v <= 1+1e-9);
}

double get_triangle_interpolated_value(Vector3 p0, Vector3 p1, Vector3 p2, Vector2 p) {
	Vector2 a = vec2(p0.x, p0.y);
	Vector2 b = vec2(p1.x, p1.y);
	Vector2 c = vec2(p2.x, p2.y);
	Vector2 v0 = subtract_vec2(c,a);
	Vector2 v1 = subtract_vec2(b,a);
	Vector2 v2 = subtract_vec2(p,a);

	double d00 = dot_vec2(v0, v0);
	double d01 = dot_vec2(v0, v1);
	double d11 = dot_vec2(v1, v1);
	double d20 = dot_vec2(v2, v0);
	double d21 = dot_vec2(v2, v1);

	double denom = d00*d11 - d01*d01;

	double v = (d00*d21 - d01*d20) / denom;
	double w = (d11*d20 - d01*d21) / denom;
	double u = 1-v-w;

	return u*p0.z + v*p1.z + w*p2.z;
}

MeshTriangle2 * get_mesh_triangle_at_position_from_box(MeshBox2 *box, Vector2 pos) {
	if(box->type == MESHBOX_SUBBOXES) {
		for(int i = 0; i < box->subboxes.num; i++) {
			if(pos.x >= box->subboxes.boxes[i]->min.x && pos.x <= box->subboxes.boxes[i]->max.x &&
				pos.y >= box->subboxes.boxes[i]->min.y && pos.y <= box->subboxes.boxes[i]->max.y) {
				return get_mesh_triangle_at_position_from_box(box->subboxes.boxes[i], pos);
			}
		}
	} else if(box->type == MESHBOX_TRIANGLES) {
		for(int i = 0; i < box->tri.num; i++) {
			if(is_inside_triangle(box->tri.triangles[i], pos)) return box->tri.triangles[i];
		}
	}
	return NULL;
}

MeshTriangle2 * get_mesh_triangle_at_position(Mesh2 *mesh, Vector2 pos) {
	return get_mesh_triangle_at_position_from_box(mesh->mesh_box, pos);
}

double get_mesh_interpolated_value(Mesh2 *mesh, Vector2 p) {
	MeshTriangle2 *tri2 = get_mesh_triangle_at_position(mesh, p);
	if(!tri2) return NAN;
	Vector3 tri3[3];
	for(int idx = 0; idx < 3; idx++) {
		tri3[idx].x = tri2->points[idx]->pos.x;
		tri3[idx].y = tri2->points[idx]->pos.y;
		tri3[idx].z = tri2->points[idx]->val;
	}
	return get_triangle_interpolated_value(tri3[0], tri3[1], tri3[2], p);
}

MeshTriangle2 * get_adj_triangle_from_two_points(MeshTriangle2 *original_triangle, MeshPoint2 *p0, MeshPoint2 *p1) {
	for(int i = 0; i < p0->num_triangles; i++) {
		for(int j = 0; j < 3; j++) {
			if(p0->triangles[i] != original_triangle && p0->triangles[i]->points[j] == p1) {
				return p0->triangles[i];
			}
		}
	}

	return NULL;
}

void add_as_adjacent_triangle_from_two_points(MeshTriangle2 *triangle, MeshTriangle2 *triangle_to_add, MeshPoint2 *p0, MeshPoint2 *p1) {
	if(!triangle || !triangle_to_add) return;

	int p0_idx = -1, p1_idx = -1;
	for(int i = 0; i < 3; i++) {
		if(triangle->points[i] == p0) p0_idx = i;
		if(triangle->points[i] == p1) p1_idx = i;
	}
	if(p0_idx < 0 || p1_idx < 0) return;

	if(p1_idx < p0_idx) {
		int temp = p0_idx;
		p0_idx = p1_idx;
		p1_idx = temp;
	}

	if(p0_idx == 0) {
		if(p1_idx == 1)	triangle->adj_triangles[0] = triangle_to_add;	// 0 - 1
		else			triangle->adj_triangles[2] = triangle_to_add;	// 2 - 0
	} else				triangle->adj_triangles[1] = triangle_to_add;	// 1 - 2
}

MeshTriangle2 * get_and_add_to_adjacent_triangle_from_two_points(MeshTriangle2 *center_triangle, MeshPoint2 *p0, MeshPoint2 *p1) {
	MeshTriangle2 *adj_triangle = get_adj_triangle_from_two_points(center_triangle, p0, p1);
	add_as_adjacent_triangle_from_two_points(adj_triangle, center_triangle, p0, p1);
	return adj_triangle;
}

MeshTriangle2 * create_triangle_from_three_points(MeshPoint2 *p0, MeshPoint2 *p1, MeshPoint2 *p2) {
	MeshTriangle2 *triangle = malloc(sizeof(MeshTriangle2));
	triangle->points[0] = p0;
	triangle->points[1] = p1;
	triangle->points[2] = p2;
	triangle->adj_triangles[0] = get_and_add_to_adjacent_triangle_from_two_points(triangle, p0, p1);
	triangle->adj_triangles[1] = get_and_add_to_adjacent_triangle_from_two_points(triangle, p1, p2);
	triangle->adj_triangles[2] = get_and_add_to_adjacent_triangle_from_two_points(triangle, p2, p0);

	for(int i = 0; i < 3; i++) {
		MeshPoint2 *p = i == 0 ? p0 : i == 1 ? p1 : p2;
		if(!p->triangles) {
			p->triangle_cap = 8;
			p->triangles = malloc(p->triangle_cap * sizeof(MeshTriangle2*));
		}
		if(p->num_triangles == p->triangle_cap) {
			p->triangle_cap *= 2;
			MeshTriangle2 **temp = realloc(p->triangles, p->triangle_cap * sizeof(MeshTriangle2*));
			if(temp) p->triangles = temp;
		}
		p->triangles[p->num_triangles] = triangle; p->num_triangles++;
	}

	return triangle;
}

void remove_as_adjacent_triangle(MeshTriangle2 *adj_triangle, MeshTriangle2 *triangle_to_remove) {
	if(adj_triangle == NULL) return;
	for(int i = 0; i < 3; i++) {
		if(adj_triangle->adj_triangles[i] == triangle_to_remove) {
			adj_triangle->adj_triangles[i] = NULL;
			return;
		}
	}
}

void remove_triangle_from_point(MeshPoint2 *p, MeshTriangle2 *triangle_to_remove) {
	if(p == NULL) return;
	for(int i = 0; i < p->num_triangles; i++) {
		if(p->triangles[i] == triangle_to_remove) {
			p->triangles[i] = p->triangles[p->num_triangles-1];
			p->num_triangles--;
			return;
		}
	}
}

void remove_triangle_from_mesh(Mesh2 *mesh, int tri_idx) {
	MeshTriangle2 *triangle = mesh->triangles[tri_idx];
	for(int i = 0; i < 3; i++) {
		remove_as_adjacent_triangle(triangle->adj_triangles[i], triangle);
		remove_triangle_from_point(triangle->points[i], triangle);
	}

	free(triangle);
	mesh->triangles[tri_idx] = mesh->triangles[mesh->num_triangles-1];
	mesh->num_triangles--;
}


MeshGrid2 *create_mesh_grid(DataArray2 *pos, void **data) {
	MeshGrid2 *grid = malloc(sizeof(MeshGrid2));
	int col_cap = 8;
	grid->num_cols = 0;
	grid->num_col_rows = malloc(col_cap * sizeof(size_t));
	grid->points = malloc(col_cap * sizeof(MeshPoint2**));

	Vector2 *pos_data = data_array2_get_data(pos);
	int row_cap = 8;

	for(int i = 0; i < data_array2_size(pos); i++) {
		if(i == 0 || pos_data[i].x != pos_data[i-1].x) {
			grid->num_cols++;
			if(grid->num_cols > col_cap) {
				col_cap *= 2;
				void *temp = realloc(grid->points, col_cap * sizeof(MeshPoint2**));
				if(temp) grid->points = temp;
				temp = realloc(grid->num_col_rows, col_cap * sizeof(size_t));
				if(temp) grid->num_col_rows = temp;
			}
			if(pos_data[i].x < -1e9) {
				if(i+1 < data_array2_size(pos)) {
					grid->points[grid->num_cols-1] = NULL;
					grid->num_col_rows[grid->num_cols-1] = 0;
				} else {
					grid->num_cols--;
				}
				continue;
			}
			row_cap = 8;
			grid->points[grid->num_cols-1] = malloc(row_cap * sizeof(MeshPoint2*));
			grid->num_col_rows[grid->num_cols-1] = 0;
		}

		int col = (int) grid->num_cols-1;

		if(grid->num_col_rows[col] > row_cap) {
			row_cap *= 2;
			void *temp = realloc(grid->points[col], row_cap * sizeof(MeshPoint2*));
			if(temp) grid->points[col] = temp;
		}

		int row = (int) grid->num_col_rows[col];

		grid->points[col][row] = malloc(sizeof(MeshPoint2));
		grid->points[col][row]->pos = pos_data[i];
		grid->points[col][row]->val = 0;
		grid->points[col][row]->data = data[i];
		grid->points[col][row]->num_triangles = 0;
		grid->points[col][row]->triangle_cap = 0;
		grid->points[col][row]->triangles = NULL;
		grid->num_col_rows[col]++;
	}

	return grid;
}

bool is_point_inside_mesh_box(MeshBox2 box, Vector2 point) {
	return (point.x >= box.min.x && point.x <= box.max.x && point.y >= box.min.y && point.y <= box.max.y);
}

void add_triangle_to_mesh_box(MeshBox2 *box, MeshTriangle2 *triangle) {
	if(!box) return;
	switch(box->type) {
		case MESHBOX_SUBBOXES:
			for(int i = 0; i < box->subboxes.num; i++) {
				double min_x, min_y, max_x, max_y;
				find_2dtriangle_minmax(triangle, &min_x, &max_x, &min_y, &max_y);
				if(min_x < box->subboxes.boxes[i]->max.x && max_x > box->subboxes.boxes[i]->min.x &&
					min_y < box->subboxes.boxes[i]->max.y && max_y > box->subboxes.boxes[i]->min.y)
				{
					add_triangle_to_mesh_box(box->subboxes.boxes[i], triangle);
				}
			}
			break;
		case MESHBOX_TRIANGLES:
			if(box->tri.cap == 0) {
				box->tri.cap = 8;
				box->tri.triangles = malloc(box->tri.cap * sizeof(MeshTriangle2*));
			} else if(box->tri.num == box->tri.cap) {
				box->tri.cap *= 2;
				MeshTriangle2 **temp = realloc(box->tri.triangles, box->tri.cap * sizeof(MeshTriangle2*));
				if(temp) box->tri.triangles = temp;
			}
			box->tri.triangles[box->tri.num++] = triangle;
			break;
	}
}

void add_triangle_to_mesh(Mesh2 *mesh, MeshTriangle2 *triangle) {
	if(mesh->num_triangles >= mesh->triangle_cap) {
		mesh->triangle_cap *= 2;
		void *temp = realloc(mesh->triangles, mesh->triangle_cap * sizeof(MeshTriangle2*));
		if(temp) mesh->triangles = temp;
	}

	mesh->triangles[mesh->num_triangles] = triangle;
	add_triangle_to_mesh_box(mesh->mesh_box, triangle);
	mesh->num_triangles++;
}

void add_point_to_mesh(Mesh2 *mesh, MeshPoint2 *point) {
	if(mesh->num_points >= mesh->point_cap) {
		mesh->point_cap *= 2;
		void *temp = realloc(mesh->points, mesh->point_cap * sizeof(MeshTriangle2*));
		if(temp) mesh->points = temp;
	}

	mesh->points[mesh->num_points] = point;
	mesh->num_points++;
}

Mesh2 * create_mesh_from_grid(MeshGrid2 *grid) {
	Mesh2 *mesh = new_mesh();

	for(int i = 0; i < grid->num_cols; i++) {
		for(int j = 0; j < grid->num_col_rows[i]; j++) {
			add_point_to_mesh(mesh, grid->points[i][j]);
		}
	}

	for(int x_idx = 0; x_idx < grid->num_cols-1; x_idx++) {
		if(x_idx < grid->num_cols-1 && grid->num_col_rows[x_idx+1] == 0) {x_idx++; continue;}
		int y_idx0 = 0, y_idx1 = 0;
		while(y_idx0 < grid->num_col_rows[x_idx] - 1 && y_idx1 < grid->num_col_rows[x_idx + 1] - 1) {
			MeshPoint2 *p0 = grid->points[x_idx][y_idx0];
			MeshPoint2 *p1 = grid->points[1 + x_idx][y_idx1];
			MeshPoint2 *p2 = grid->points[x_idx][1 + y_idx0];
			MeshPoint2 *p3 = grid->points[1 + x_idx][1 + y_idx1];

			double sq_dist03 = sq_mag_vec2(subtract_vec2(p0->pos, p3->pos));
			double sq_dist12 = sq_mag_vec2(subtract_vec2(p1->pos, p2->pos));

			if(sq_dist03 < sq_dist12) {
				add_triangle_to_mesh(mesh, create_triangle_from_three_points(p0, p1, p3));
				y_idx1++;
			} else {
				add_triangle_to_mesh(mesh, create_triangle_from_three_points(p0, p1, p2));
				y_idx0++;
			}
		}

		if(y_idx0 == grid->num_col_rows[x_idx] - 1) {
			while(y_idx1 < grid->num_col_rows[x_idx + 1] - 1) {
				MeshPoint2 *p0 = grid->points[x_idx][y_idx0];
				MeshPoint2 *p1 = grid->points[x_idx + 1][y_idx1];
				MeshPoint2 *p2 = grid->points[x_idx + 1][y_idx1 + 1];
				add_triangle_to_mesh(mesh, create_triangle_from_three_points(p0, p1, p2));
				y_idx1++;
			}
		}

		if(y_idx1 == grid->num_col_rows[x_idx + 1] - 1) {
			while(y_idx0 < grid->num_col_rows[x_idx] - 1) {
				MeshPoint2 *p0 = grid->points[x_idx][y_idx0];
				MeshPoint2 *p1 = grid->points[x_idx][y_idx0 + 1];
				MeshPoint2 *p2 = grid->points[x_idx + 1][y_idx1];
				add_triangle_to_mesh(mesh, create_triangle_from_three_points(p0, p1, p2));
				y_idx0++;
			}
		}
	}

	return mesh;
}

Mesh2 * create_mesh_from_grid_w_angled_guideline(MeshGrid2 *grid, double gradient) {
	Mesh2 *mesh = new_mesh();

	Vector2 min = grid->points[0][0]->pos;
	Vector2 max = grid->points[0][0]->pos;

	for(int i = 0; i < grid->num_cols; i++) {
		for(int j = 0; j < grid->num_col_rows[i]; j++) {
			add_point_to_mesh(mesh, grid->points[i][j]);
			if(grid->points[i][j]->pos.x < min.x) min.x = grid->points[i][j]->pos.x;
			if(grid->points[i][j]->pos.x > max.x) max.x = grid->points[i][j]->pos.x;
			if(grid->points[i][j]->pos.y < min.y) min.y = grid->points[i][j]->pos.y;
			if(grid->points[i][j]->pos.y > max.y) max.y = grid->points[i][j]->pos.y;
		}
	}

	int subbox_rows = 10, subbox_cols = 10;
	mesh->mesh_box->min = min;
	mesh->mesh_box->max = max;
	mesh->mesh_box->subboxes.num = subbox_rows * subbox_cols;
	mesh->mesh_box->subboxes.cap = subbox_rows * subbox_cols;
	MeshBox2 **temp = realloc(mesh->mesh_box->subboxes.boxes, mesh->mesh_box->subboxes.num * sizeof(MeshBox2*));
	if(temp) mesh->mesh_box->subboxes.boxes = temp;
	// mesh->mesh_box->subboxes.boxes = malloc(mesh->mesh_box->subboxes.num * sizeof(MeshBox2*));
	mesh->mesh_box->type = MESHBOX_SUBBOXES;
	int idx = 0;
	for(int col = 0; col < subbox_cols; col++) {
		double min_x = min.x + col * (max.x - min.x) / subbox_cols;
		double max_x = min.x + (col+1) * (max.x - min.x) / subbox_cols;
		for(int row = 0; row < subbox_rows; row++) {
			double min_y = min.y + row * (max.y - min.y) / subbox_rows;
			double max_y = min.y + (row+1) * (max.y - min.y) / subbox_rows;
			mesh->mesh_box->subboxes.boxes[idx] = malloc(sizeof(MeshBox2));
			mesh->mesh_box->subboxes.boxes[idx]->min = vec2(min_x, min_y);
			mesh->mesh_box->subboxes.boxes[idx]->max = vec2(max_x, max_y);
			mesh->mesh_box->subboxes.boxes[idx]->type = MESHBOX_TRIANGLES;
			mesh->mesh_box->subboxes.boxes[idx]->tri.cap = 0;
			mesh->mesh_box->subboxes.boxes[idx]->tri.num = 0;
			mesh->mesh_box->subboxes.boxes[idx]->tri.triangles = NULL;
			idx++;
		}
	}

	for(int x_idx = 0; x_idx < grid->num_cols-1; x_idx++) {
		if(x_idx < grid->num_cols-1 && grid->num_col_rows[x_idx+1] == 0) {x_idx++; continue;}
		int y_idx0 = 0, y_idx1 = 0;
		while(y_idx0 < grid->num_col_rows[x_idx] - 1 && y_idx1 < grid->num_col_rows[x_idx + 1] - 1) {
			MeshPoint2 *p0 = grid->points[x_idx][y_idx0];
			MeshPoint2 *p1 = grid->points[1 + x_idx][y_idx1];
			MeshPoint2 *p2 = grid->points[x_idx][1 + y_idx0];
			MeshPoint2 *p3 = grid->points[1 + x_idx][1 + y_idx1];

			Vector2 p1_shifted = p1->pos;
			Vector2 p3_shifted = p3->pos;
			double dx = p1->pos.x - p0->pos.x;
			p1_shifted.y += -gradient * dx;
			p3_shifted.y += -gradient * dx;

			double sq_dist03 = sq_mag_vec2(subtract_vec2(p0->pos, p3_shifted));
			double sq_dist12 = sq_mag_vec2(subtract_vec2(p1_shifted, p2->pos));

			if(sq_dist03 < sq_dist12) {
				add_triangle_to_mesh(mesh, create_triangle_from_three_points(p0, p1, p3));
				y_idx1++;
			} else {
				add_triangle_to_mesh(mesh, create_triangle_from_three_points(p0, p1, p2));
				y_idx0++;
			}
		}

		if(y_idx0 == grid->num_col_rows[x_idx] - 1) {
			while(y_idx1 < grid->num_col_rows[x_idx + 1] - 1) {
				MeshPoint2 *p0 = grid->points[x_idx][y_idx0];
				MeshPoint2 *p1 = grid->points[x_idx + 1][y_idx1];
				MeshPoint2 *p2 = grid->points[x_idx + 1][y_idx1 + 1];
				add_triangle_to_mesh(mesh, create_triangle_from_three_points(p0, p1, p2));
				y_idx1++;
			}
		}

		if(y_idx1 == grid->num_col_rows[x_idx + 1] - 1) {
			while(y_idx0 < grid->num_col_rows[x_idx] - 1) {
				MeshPoint2 *p0 = grid->points[x_idx][y_idx0];
				MeshPoint2 *p1 = grid->points[x_idx][y_idx0 + 1];
				MeshPoint2 *p2 = grid->points[x_idx + 1][y_idx1];
				add_triangle_to_mesh(mesh, create_triangle_from_three_points(p0, p1, p2));
				y_idx0++;
			}
		}
	}

	return mesh;
}

void free_mesh_box(MeshBox2 *box);

bool is_triangle_bouding_box_inside_rectangle(MeshTriangle2 *triangle, Vector2 min, Vector2 max) {
	double min_x, max_x, min_y, max_y;
	find_2dtriangle_minmax(triangle, &min_x, &max_x, &min_y, &max_y);
	if(max_x < min.x || min_x > max.x || max_y < min.y || min_y > max.y) return false;
	return true;
}

void add_triangles_to_mesh_box(MeshBox2 *box, MeshTriangle2 **triangles, size_t num_triangles) {
	for(int i = 0; i < num_triangles; i++) {
		if(is_triangle_bouding_box_inside_rectangle(triangles[i], box->min, box->max)) {
			if(box->tri.num == box->tri.cap) {
				box->tri.cap *= 2;
				MeshTriangle2 **temp = realloc(box->tri.triangles, box->tri.cap * sizeof(MeshTriangle2*));
				if(temp) box->tri.triangles = temp;
			}
			box->tri.triangles[box->tri.num++] = triangles[i];
		}
	}
}

void subdivide_mesh_box(MeshBox2 **box_p, int num_rows, int num_cols, int level) {
	MeshBox2 *box = *box_p;
	MeshBox2 *new_box = malloc(sizeof(MeshBox2));
	new_box->min = box->min;
	new_box->max = box->max;
	new_box->type = MESHBOX_SUBBOXES;
	new_box->subboxes.num = 0;
	new_box->subboxes.cap = num_rows*num_cols;
	new_box->subboxes.boxes = malloc(num_rows*num_cols * sizeof(MeshBox2*));

	for(int col = 0; col < num_cols; col++) {
		double min_x = box->min.x + col * (box->max.x - box->min.x) / num_cols;
		double max_x = box->min.x + (col+1) * (box->max.x - box->min.x) / num_cols;
		for(int row = 0; row < num_rows; row++) {
			double min_y = box->min.y + row * (box->max.y - box->min.y) / num_rows;
			double max_y = box->min.y + (row+1) * (box->max.y - box->min.y) / num_rows;
			int idx = (int) new_box->subboxes.num;
			new_box->subboxes.boxes[idx] = malloc(sizeof(MeshBox2));
			new_box->subboxes.boxes[idx]->min = vec2(min_x, min_y);
			new_box->subboxes.boxes[idx]->max = vec2(max_x, max_y);
			new_box->subboxes.boxes[idx]->type = MESHBOX_TRIANGLES;
			new_box->subboxes.boxes[idx]->tri.cap = 8;
			new_box->subboxes.boxes[idx]->tri.num = 0;
			new_box->subboxes.boxes[idx]->tri.triangles = malloc(new_box->subboxes.boxes[idx]->tri.cap * sizeof(MeshTriangle2*));
			add_triangles_to_mesh_box(new_box->subboxes.boxes[idx], box->tri.triangles, box->tri.num);
			if(new_box->subboxes.boxes[idx]->tri.num == 0) {
				free(new_box->subboxes.boxes[idx]->tri.triangles);
				free(new_box->subboxes.boxes[idx]);
				continue;
			}
			if(new_box->subboxes.boxes[idx]->tri.num > 100) {
				if(level < 3)
					subdivide_mesh_box(&new_box->subboxes.boxes[idx], num_rows, num_cols, level+1);
			}
			new_box->subboxes.num++;
		}
	}

	free_mesh_box(box);
	*box_p = new_box;
}

void rebuild_mesh_boxes(Mesh2 *mesh) {
	MeshBox2 *new_box = malloc(sizeof(MeshBox2));
	new_box->min = mesh->mesh_box->min;
	new_box->max = mesh->mesh_box->max;
	new_box->type = MESHBOX_TRIANGLES;
	new_box->tri.num = mesh->num_triangles;
	new_box->tri.cap = mesh->num_triangles;
	new_box->tri.triangles = malloc(mesh->num_triangles * sizeof(MeshTriangle2*));
	memcpy(new_box->tri.triangles, mesh->triangles, mesh->num_triangles * sizeof(MeshTriangle2*));

	if(new_box->tri.num > 100) subdivide_mesh_box(&new_box, 5, 5, 0);

	free_mesh_box(mesh->mesh_box);
	mesh->mesh_box = new_box;
}

Mesh2 * combine_meshes(Mesh2 *mesh0, Mesh2 *mesh1) {
	MeshPoint2 **temp_points = realloc(mesh0->points, (mesh0->num_points+mesh1->num_points) * sizeof(MeshPoint2*));
	if(temp_points) mesh0->points = temp_points;
	MeshTriangle2 **temp_triangles = realloc(mesh0->triangles, (mesh0->num_triangles+mesh1->num_triangles) * sizeof(MeshTriangle2*));
	if(temp_triangles) mesh0->triangles = temp_triangles;
	MeshBox2 **temp_boxes = realloc(mesh0->mesh_box->subboxes.boxes, (mesh0->mesh_box->subboxes.num + mesh1->mesh_box->subboxes.num) * sizeof(MeshBox2*));
	if(temp_boxes) mesh0->mesh_box->subboxes.boxes = temp_boxes;

	memcpy(mesh0->points+mesh0->num_points, mesh1->points, mesh1->num_points * sizeof(MeshPoint2*));
	memcpy(mesh0->triangles+mesh0->num_triangles, mesh1->triangles, mesh1->num_triangles * sizeof(MeshTriangle2*));
	memcpy(mesh0->mesh_box->subboxes.boxes + mesh0->mesh_box->subboxes.num, mesh1->mesh_box->subboxes.boxes, mesh1->mesh_box->subboxes.num * sizeof(MeshBox2*));

	mesh0->num_points = mesh0->num_points + mesh1->num_points;
	mesh0->num_triangles = mesh0->num_triangles + mesh1->num_triangles;
	mesh0->mesh_box->subboxes.num = mesh0->mesh_box->subboxes.num + mesh1->mesh_box->subboxes.num;
	mesh0->point_cap = mesh0->num_points;
	mesh0->triangle_cap = mesh0->num_triangles;
	mesh0->mesh_box->subboxes.cap = mesh0->mesh_box->subboxes.num;
	mesh0->mesh_box->min.x = fmin(mesh0->mesh_box->min.x, mesh1->mesh_box->min.x);
	mesh0->mesh_box->min.y = fmin(mesh0->mesh_box->min.y, mesh1->mesh_box->min.y);
	mesh0->mesh_box->max.x = fmax(mesh0->mesh_box->max.x, mesh1->mesh_box->max.x);
	mesh0->mesh_box->max.y = fmax(mesh0->mesh_box->max.y, mesh1->mesh_box->max.y);

	free(mesh1->triangles);
	free(mesh1->points);
	free(mesh1->mesh_box->subboxes.boxes);
	free(mesh1->mesh_box);
	free(mesh1);

	return mesh0;
}

void update_mesh_minmax(Mesh2 *mesh) {
	mesh->mesh_box->min = mesh->points[0]->pos;
	mesh->mesh_box->max = mesh->points[0]->pos;

	for(int i = 0; i < mesh->num_points; i++) {
		if(mesh->points[i]->pos.x < mesh->mesh_box->min.x) mesh->mesh_box->min.x = mesh->points[i]->pos.x;
		if(mesh->points[i]->pos.y < mesh->mesh_box->min.y) mesh->mesh_box->min.y = mesh->points[i]->pos.y;
		if(mesh->points[i]->pos.x > mesh->mesh_box->max.x) mesh->mesh_box->max.x = mesh->points[i]->pos.x;
		if(mesh->points[i]->pos.y > mesh->mesh_box->max.y) mesh->mesh_box->max.y = mesh->points[i]->pos.y;
	}
}

void free_grid_keep_points(MeshGrid2 *grid) {
	free(grid->num_col_rows);
	free(grid);
}

void free_mesh_box(MeshBox2 *box) {
	if(box->type == MESHBOX_SUBBOXES) {
		for(int i = 0; i < box->subboxes.num; i++) {
			free_mesh_box(box->subboxes.boxes[i]);
		}
		free(box->subboxes.boxes);
	} else if(box->type == MESHBOX_TRIANGLES) {
		free(box->tri.triangles);
	}
	free(box);
}

void free_mesh(Mesh2 *mesh, void (*free_data_func)(void *data)) {
	for(int i = 0; i < mesh->num_triangles; i++) { free(mesh->triangles[i]); }
	free(mesh->triangles);
	for(int i = 0; i < mesh->num_points; i++) {
		free_data_func(mesh->points[i]->data);
		free(mesh->points[i]->triangles);
		free(mesh->points[i]);
	}
	free_mesh_box(mesh->mesh_box);
	free(mesh);
}