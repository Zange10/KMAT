#include "quad.h"

#include <math.h>
#include <stdio.h>

MeshPoint2 * create_quad_point(double x, double y, QuadPointFunc *point_func) {
	if(point_func) return point_func->func(x, y, point_func->params);
	return create_mesh_point(vec2(x, y), NULL, 0);
}

Quad * create_quad_from_four_points(Quad *parent, MeshPoint2 *p00, MeshPoint2 *p01, MeshPoint2 *p10, MeshPoint2 *p11, QuadPointFunc *point_func) {
	Quad *quad = malloc(sizeof(Quad));
	quad->parent = parent;
	if(parent) quad->rf_level = parent->rf_level+1;
	else quad->rf_level = 0;
	quad->corner[QUAD_NW] = p00;
	quad->corner[QUAD_NE] = p01;
	quad->corner[QUAD_SW] = p10;
	quad->corner[QUAD_SE] = p11;
	double center_x = (quad->corner[QUAD_NW]->pos.x+quad->corner[QUAD_NE]->pos.x)/2.0;
	double center_y = (quad->corner[QUAD_NW]->pos.y+quad->corner[QUAD_SW]->pos.y)/2.0;

	quad->center = create_quad_point(center_x, center_y, point_func);

	for(int i = 0; i < 8; i++) quad->neighbours[i] = NULL;
	quad->flags = 0;
	set_quad_flag(quad, QUAD_FLAG_IS_LEAF);
	return quad;
}

Quad * get_root_quad(Quad *quad) {
	if(!quad) return NULL;
	while(quad->parent) quad = quad->parent;
	return quad;
}

void set_quad_flag(Quad *quad, QuadFlag flag) { quad->flags |= flag; }
void remove_quad_flag(Quad *quad, QuadFlag flag) { quad->flags &= ~flag; }
bool is_quad_flag(Quad *quad, QuadFlag flag) { return (quad->flags & flag) != 0; }

bool is_inside_quad(Quad *quad, Vector2 pos) {
	if(!quad) return false;
	if(pos.x < quad->corner[QUAD_NW]->pos.x) return false;
	if(pos.x > quad->corner[QUAD_NE]->pos.x) return false;
	if(pos.y > quad->corner[QUAD_NW]->pos.y) return false;
	if(pos.y < quad->corner[QUAD_SW]->pos.y) return false;
	return true;
}

Quad * get_quad_at_position(Quad *root_quad, Vector2 pos) {
	if(!root_quad) return NULL;
	if(is_quad_flag(root_quad, QUAD_FLAG_IS_LEAF)) return root_quad;

	for(int i = 0; i < 4; i++) {
		if(is_inside_quad(root_quad->subquads[i], pos))
			return get_quad_at_position(root_quad->subquads[i], pos);
	}
	return NULL;
}

void populate_quad_mesh_points(Quad *quad, QuadPointPopFunc *point_pop_func) {
	if(!quad) return;
	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) {
		point_pop_func->func(quad->center, point_pop_func->params);
		for(int i = 0; i < 4; i++) {
			if(!quad->corner[i]->val) point_pop_func->func(quad->corner[i], point_pop_func->params);
		}
	} else {
		for(int i = 0; i < 4; i++) {
			if(quad->subquads[i]) populate_quad_mesh_points(quad->subquads[i], point_pop_func);
		}
	}
}

double get_quad_interpolated_value(Quad *quad, Vector2 pos, int value_idx) {
	Vector3 p00 = vec3(quad->corner[QUAD_NW]->pos.x, quad->corner[QUAD_NW]->pos.y, quad->corner[QUAD_NW]->val[value_idx]);
	Vector3 p01 = vec3(quad->corner[QUAD_NE]->pos.x, quad->corner[QUAD_NE]->pos.y, quad->corner[QUAD_NE]->val[value_idx]);
	Vector3 p10 = vec3(quad->corner[QUAD_SW]->pos.x, quad->corner[QUAD_SW]->pos.y, quad->corner[QUAD_SW]->val[value_idx]);
	Vector3 p11 = vec3(quad->corner[QUAD_SE]->pos.x, quad->corner[QUAD_SE]->pos.y, quad->corner[QUAD_SE]->val[value_idx]);

	double tx = (pos.x - p00.x) / (p01.x - p00.x);
	double ty = (pos.y - p00.y) / (p10.y - p00.y);

	return (1.0 - tx) * (1.0 - ty) * p00.z +
		   tx         * (1.0 - ty) * p01.z +
		   (1.0 - tx) * ty         * p10.z +
		   tx         * ty         * p11.z;
}

Vector3 get_quad_max_values(Quad *quad, int quad_val_idx) {
	if(!quad) return vec3(NAN, NAN, NAN);
	Vector3 max = vec3(NAN, NAN, NAN);

	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) {
		for(int i = 0; i < 4; i++) {
			double val = quad->corner[i]->num_val > 0 ? quad->corner[i]->val[quad_val_idx] : NAN;
			if(isnan(max.x) || quad->corner[i]->pos.x > max.x) max.x = quad->corner[i]->pos.x;
			if(isnan(max.y) || quad->corner[i]->pos.y > max.y) max.y = quad->corner[i]->pos.y;
			if(quad_val_idx >= 0 && (isnan(max.z) || val > max.z)) max.z = val;
		}
		double val = quad->center->num_val > 0 ? quad->center->val[quad_val_idx] : NAN;
		if(quad_val_idx >= 0 && (isnan(max.z) || val > max.z)) max.z = val;
	} else {
		for(int i = 0; i < 4; i++) {
			if(quad->subquads[i]) {
				Vector3 vals = get_quad_max_values(quad->subquads[i], quad_val_idx);
				if(isnan(max.x) || vals.x > max.x) max.x = vals.x;
				if(isnan(max.y) || vals.y > max.y) max.y = vals.y;
				if(quad_val_idx >= 0 && (isnan(max.z) || vals.z > max.z)) max.z = vals.z;
			}
		}
	}
	return max;
}

Vector3 get_quad_min_values(Quad *quad, int quad_val_idx) {
	if(!quad) return vec3(NAN, NAN, NAN);
	Vector3 min = vec3(NAN, NAN, NAN);

	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) {
		for(int i = 0; i < 4; i++) {
			double val = quad->corner[i]->num_val > 0 ? quad->corner[i]->val[quad_val_idx] : NAN;
			if(isnan(min.x) || quad->corner[i]->pos.x < min.x) min.x = quad->corner[i]->pos.x;
			if(isnan(min.y) || quad->corner[i]->pos.y < min.y) min.y = quad->corner[i]->pos.y;
			if(quad_val_idx >= 0 && (isnan(min.z) || val < min.z)) min.z = val;
		}
		double val = quad->center->num_val > 0 ? quad->center->val[quad_val_idx] : NAN;
		if(quad_val_idx >= 0 && (isnan(min.z) || val < min.z)) min.z = val;
	} else {
		for(int i = 0; i < 4; i++) {
			if(quad->subquads[i]) {
				Vector3 vals = get_quad_min_values(quad->subquads[i], quad_val_idx);
				if(isnan(min.x) || vals.x < min.x) min.x = vals.x;
				if(isnan(min.y) || vals.y < min.y) min.y = vals.y;
				if(quad_val_idx >= 0 && (isnan(min.z) || vals.z < min.z)) min.z = vals.z;
			}
		}
	}
	return min;
}

double get_quad_min_value(Quad *quad, int quad_val_idx) {
	if(!quad) return NAN;
	double min = NAN;

	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) {
		for(int i = 0; i < 4; i++) {
			double val = quad->corner[i]->val[quad_val_idx];
			if(isnan(min) || val < min) min = val;
		}
	} else {
		for(int i = 0; i < 4; i++) {
			if(quad->subquads[i]) {
				double val = get_quad_min_value(quad->subquads[i], quad_val_idx);
				if(isnan(min) || val < min) min = val;
			}
		}
	}
	return min;
}

double get_quad_max_value(Quad *quad, int quad_val_idx) {
	if(!quad) return NAN;
	double max = NAN;

	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) {
		for(int i = 0; i < 4; i++) {
			double val = quad->corner[i]->val[quad_val_idx];
			if(isnan(max) || val > max) max = val;
		}
	} else {
		for(int i = 0; i < 4; i++) {
			if(quad->subquads[i]) {
				double val = get_quad_max_value(quad->subquads[i], quad_val_idx);
				if(isnan(max) || val > max) max = val;
			}
		}
	}
	return max;
}

int get_num_quad_leaves(Quad *quad) {
	if(!quad) return 0;
	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) return 1;
	int sum = 0;
	for(int i = 0; i < 4; i++) {
		if(quad->subquads[i]) {
			sum += get_num_quad_leaves(quad->subquads[i]);
		}
	}
	return sum;
}

void print_quadtree(Quad *quad) {
	if(!quad) return;
	for(int i = 0; i < quad->rf_level; i++) printf(" - ");
	printf("%p\n", quad);
	if(!is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) {
		for(int i = 0; i < 4; i++) {
			if(quad->subquads[i]) {
				print_quadtree(quad->subquads[i]);
			}
		}
	}
}

bool is_quad_crossed_by_line_segment(Quad *quad, Vector2 p0, Vector2 p1) {
	if(isnan(p0.x) || isnan(p0.y) || isnan(p1.x) || isnan(p1.y)) return false;
	if(is_inside_quad(quad, p0) || is_inside_quad(quad, p1)) return true;
	Vector2 pq0 = quad->corner[QUAD_NW]->pos;
	Vector2 pq1 = quad->corner[QUAD_NE]->pos;
	Vector2 pq2 = quad->corner[QUAD_SE]->pos;
	Vector2 pq3 = quad->corner[QUAD_SW]->pos;

	if(are_line_segments_intersecting2(p0, p1, pq0, pq1)) return true;
	if(are_line_segments_intersecting2(p0, p1, pq1, pq2)) return true;
	if(are_line_segments_intersecting2(p0, p1, pq2, pq3)) return true;
	if(are_line_segments_intersecting2(p0, p1, pq3, pq0)) return true;

	return false;
}

bool is_quad_crossed_by_line(Quad *quad, DataArray2 *line) {
	size_t num_points = data_array2_size(line);
	Vector2 *line_data = data_array2_get_data(line);

	for(int i = 1; i < num_points; i++) {
		if(is_quad_crossed_by_line_segment(quad, line_data[i], line_data[i - 1])) return true;
	}
	return false;
}

void find_line_crossed_quads(Quad *quad, DataArray2 *line, Quad ***quad_array, size_t *quad_array_size, size_t *quad_array_cap) {
	if(!quad) return;

	if(!is_quad_crossed_by_line(quad, line)) return;

	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) {
		if(*quad_array_size+1 >= *quad_array_cap) {
			*quad_array_cap *= 2;
			void *temp = realloc(*quad_array, *quad_array_cap * sizeof(Quad*));
			if(temp) *quad_array = temp;
		}
		(*quad_array)[(*quad_array_size)++] = quad;
	} else {
		for(int i = 0; i < 4; i++) {
			if(quad->subquads[i]) {
				find_line_crossed_quads(quad->subquads[i], line, quad_array, quad_array_size, quad_array_cap);
			}
		}
	}
}

void remove_out_of_bounds_quads(Quad *quad, QuadBoundsFunc *bounds_func) {
	if(!bounds_func->func(quad, bounds_func->params)) {
		free_quad(quad, true);
		return;
	}
	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) return;

	for(int i = 0; i < 4; i++) {
		if(quad->subquads[i]) {
			remove_out_of_bounds_quads(quad->subquads[i], bounds_func);
		}
	}
}

int update_quad_error_flag(Quad *quad, int min_rf_level, int max_rf_level, QuadErrorFunc *errfunc) {
	if(!quad) return 0;
	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) {
		if(quad->rf_level < min_rf_level || errfunc->func(quad, errfunc->params)) {
			set_quad_flag(quad, QUAD_FLAG_ACC_ERR);
			if(quad->rf_level < max_rf_level) set_quad_flag(quad, QUAD_FLAG_SPLIT);
			return 1;
		}
		return 0;
	}
	int sum = 0;
	for(int i = 0; i < 4; i++) {
		if(quad->subquads[i]) {
			sum += update_quad_error_flag(quad->subquads[i], min_rf_level, max_rf_level, errfunc);
		}
	}
	return sum;
}


int split_quads_with_flag(Quad *quad, QuadPointFunc *point_func) {
	if(!quad) return 0;
	int num_splits = 0;
	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) {
		if(is_quad_flag(quad, QUAD_FLAG_SPLIT)) {
			num_splits += split_quad(quad, point_func);
		}
	} else {
		for(int i = 0; i < 4; i++) {
			if(quad->subquads[i]) {
				num_splits += split_quads_with_flag(quad->subquads[i], point_func);
			}
		}
	}
	return num_splits;
}

bool check_neighbour_integrity(Quad *quad) {
	if(!quad) return true;
	printf("%p\n", quad);
	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) {
		for(int i = 0; i < 8; i++) {
			if(!quad->neighbours[i]) continue;
			if(!is_quad_flag(quad->neighbours[i], QUAD_FLAG_IS_LEAF)) {
				return false;
			}
		}
	} else {
		for(int i = 0; i < 4; i++) {
			if(quad->subquads[i]) {
				check_neighbour_integrity(quad->subquads[i]);
			}
		}
	}
}

void set_quad_neighbours(Quad *center, Quad *tl, Quad *tr, Quad *lt, Quad *lb, Quad *rt, Quad *rb, Quad *bl, Quad *br) {
	center->neighbours[0] = tl;
	center->neighbours[1] = tr;
	center->neighbours[2] = lt;
	center->neighbours[3] = lb;
	center->neighbours[4] = rt;
	center->neighbours[5] = rb;
	center->neighbours[6] = bl;
	center->neighbours[7] = br;
}

void update_neighbours_after_split(Quad *subquads[4], Quad *neighbours[8]) {
	set_quad_neighbours(subquads[QUAD_NW],
		neighbours[QUAD_NNW], neighbours[QUAD_NNW],
		neighbours[QUAD_NWW], neighbours[QUAD_NWW],
		subquads[QUAD_NE], subquads[QUAD_NE],
		subquads[QUAD_SW], subquads[QUAD_SW]);

	set_quad_neighbours(subquads[QUAD_NE],
		neighbours[QUAD_NNE], neighbours[QUAD_NNE],
		subquads[QUAD_NW], subquads[QUAD_NW],
		neighbours[QUAD_NEE], neighbours[QUAD_NEE],
		subquads[QUAD_SE], subquads[QUAD_SE]);

	set_quad_neighbours(subquads[QUAD_SW],
		subquads[QUAD_NW], subquads[QUAD_NW],
		neighbours[QUAD_SWW], neighbours[QUAD_SWW],
		subquads[QUAD_SE], subquads[QUAD_SE],
		neighbours[QUAD_SSW], neighbours[QUAD_SSW]);

	set_quad_neighbours(subquads[QUAD_SE],
		subquads[QUAD_NE], subquads[QUAD_NE],
		subquads[QUAD_SW], subquads[QUAD_SW],
		neighbours[QUAD_SEE], neighbours[QUAD_SEE],
		neighbours[QUAD_SSE], neighbours[QUAD_SSE]);

	int rf_level = subquads[0]->rf_level;

	if(neighbours[QUAD_NNW]) {
		if(neighbours[QUAD_NNW]->rf_level < rf_level) {
			neighbours[QUAD_NNW]->neighbours[QUAD_SSW] = subquads[QUAD_NW];
			neighbours[QUAD_NNW]->neighbours[QUAD_SSE] = subquads[QUAD_NE];
		} else {
			neighbours[QUAD_NNW]->neighbours[QUAD_SSW] = subquads[QUAD_NW];
			neighbours[QUAD_NNW]->neighbours[QUAD_SSE] = subquads[QUAD_NW];
			if(neighbours[QUAD_NNE]) {
				neighbours[QUAD_NNE]->neighbours[QUAD_SSW] = subquads[QUAD_NE];
				neighbours[QUAD_NNE]->neighbours[QUAD_SSE] = subquads[QUAD_NE];
			}
		}
	} else if(neighbours[QUAD_NNE]) {
		neighbours[QUAD_NNE]->neighbours[QUAD_SSW] = subquads[QUAD_NE];
		neighbours[QUAD_NNE]->neighbours[QUAD_SSE] = subquads[QUAD_NE];
	}

	if(neighbours[QUAD_NWW]) {
		if(neighbours[QUAD_NWW]->rf_level < rf_level) {
			neighbours[QUAD_NWW]->neighbours[QUAD_NEE] = subquads[QUAD_NW];
			neighbours[QUAD_NWW]->neighbours[QUAD_SEE] = subquads[QUAD_SW];
		} else {
			neighbours[QUAD_NWW]->neighbours[QUAD_NEE] = subquads[QUAD_NW];
			neighbours[QUAD_NWW]->neighbours[QUAD_SEE] = subquads[QUAD_NW];
			if(neighbours[QUAD_SWW]) {
				neighbours[QUAD_SWW]->neighbours[QUAD_NEE] = subquads[QUAD_SW];
				neighbours[QUAD_SWW]->neighbours[QUAD_SEE] = subquads[QUAD_SW];
			}
		}
	} else if(neighbours[QUAD_SWW]) {
		neighbours[QUAD_SWW]->neighbours[QUAD_NEE] = subquads[QUAD_SW];
		neighbours[QUAD_SWW]->neighbours[QUAD_SEE] = subquads[QUAD_SW];
	}

	if(neighbours[QUAD_NEE]) {
		if(neighbours[QUAD_NEE]->rf_level < rf_level) {
			neighbours[QUAD_NEE]->neighbours[QUAD_NWW] = subquads[QUAD_NE];
			neighbours[QUAD_NEE]->neighbours[QUAD_SWW] = subquads[QUAD_SE];
		} else {
			neighbours[QUAD_NEE]->neighbours[QUAD_NWW] = subquads[QUAD_NE];
			neighbours[QUAD_NEE]->neighbours[QUAD_SWW] = subquads[QUAD_NE];
			if(neighbours[QUAD_SEE]) {
				neighbours[QUAD_SEE]->neighbours[QUAD_NWW] = subquads[QUAD_SE];
				neighbours[QUAD_SEE]->neighbours[QUAD_SWW] = subquads[QUAD_SE];
			}
		}
	} else if(neighbours[QUAD_SEE]) {
		neighbours[QUAD_SEE]->neighbours[QUAD_NWW] = subquads[QUAD_SE];
		neighbours[QUAD_SEE]->neighbours[QUAD_SWW] = subquads[QUAD_SE];
	}

	if(neighbours[QUAD_SSW]) {
		if(neighbours[QUAD_SSW]->rf_level < rf_level) {
			neighbours[QUAD_SSW]->neighbours[QUAD_NNW] = subquads[QUAD_SW];
			neighbours[QUAD_SSW]->neighbours[QUAD_NNE] = subquads[QUAD_SE];
		} else {
			neighbours[QUAD_SSW]->neighbours[QUAD_NNW] = subquads[QUAD_SW];
			neighbours[QUAD_SSW]->neighbours[QUAD_NNE] = subquads[QUAD_SW];
			if(neighbours[QUAD_SSE]) {
				neighbours[QUAD_SSE]->neighbours[QUAD_NNW] = subquads[QUAD_SE];
				neighbours[QUAD_SSE]->neighbours[QUAD_NNE] = subquads[QUAD_SE];
			}
		}
	} else if(neighbours[QUAD_SSE]) {
		neighbours[QUAD_SSE]->neighbours[QUAD_NNW] = subquads[QUAD_SE];
		neighbours[QUAD_SSE]->neighbours[QUAD_NNE] = subquads[QUAD_SE];
	}
}

int split_quad(Quad *quad, QuadPointFunc *point_func) {
	if(!quad) return 0;
	if(!is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) return 0;

	int num_splits = 0;

	Quad *neighbours[8];
	bool changed = false;

	do {
		changed = false;
		for(int i = 0; i < 8; i++) {neighbours[i] = quad->neighbours[i];}
		for(int i = 0; i < 8; i++) {
			if(!neighbours[i]) continue;
			if(neighbours[i]->rf_level < quad->rf_level) {
				num_splits += split_quad(neighbours[i], point_func);
				changed = true;
			}
		}
	} while(changed);

	MeshPoint2 *corners[4];
	MeshPoint2 *edges[4];

	if(!quad->center) {
		double center_x = (quad->corner[QUAD_NW]->pos.x+quad->corner[QUAD_NE]->pos.x)/2.0;
		double center_y = (quad->corner[QUAD_NW]->pos.y+quad->corner[QUAD_SW]->pos.y)/2.0;

		quad->center = create_quad_point(center_x, center_y, point_func);
	}

	if(neighbours[QUAD_NNW] && neighbours[QUAD_NNW]->rf_level > quad->rf_level)
		edges[QUAD_N] = neighbours[QUAD_NNW]->corner[QUAD_SE];
	else if(neighbours[QUAD_NNE] && neighbours[QUAD_NNE]->rf_level > quad->rf_level)
		edges[QUAD_N] = neighbours[QUAD_NNE]->corner[QUAD_SW];
	else
		edges[QUAD_N] = create_quad_point(quad->center->pos.x, quad->corner[QUAD_NW]->pos.y, point_func);

	if(neighbours[QUAD_NWW] && neighbours[QUAD_NWW]->rf_level > quad->rf_level)
		edges[QUAD_W] = neighbours[QUAD_NWW]->corner[QUAD_SE];
	else if(neighbours[QUAD_SWW] && neighbours[QUAD_SWW]->rf_level > quad->rf_level)
		edges[QUAD_W] = neighbours[QUAD_SWW]->corner[QUAD_NE];
	else
		edges[QUAD_W] = create_quad_point(quad->corner[QUAD_NW]->pos.x, quad->center->pos.y, point_func);

	if(neighbours[QUAD_NEE] && neighbours[QUAD_NEE]->rf_level > quad->rf_level)
		edges[QUAD_E] = neighbours[QUAD_NEE]->corner[QUAD_SW];
	else if(neighbours[QUAD_SEE] && neighbours[QUAD_SEE]->rf_level > quad->rf_level)
		edges[QUAD_E] = neighbours[QUAD_SEE]->corner[QUAD_NW];
	else
		edges[QUAD_E] = create_quad_point(quad->corner[QUAD_NE]->pos.x, quad->center->pos.y, point_func);

	if(neighbours[QUAD_SSW] && neighbours[QUAD_SSW]->rf_level > quad->rf_level)
		edges[QUAD_S] = neighbours[QUAD_SSW]->corner[QUAD_NE];
	else if(neighbours[QUAD_SSE] && neighbours[QUAD_SSE]->rf_level > quad->rf_level)
		edges[QUAD_S] = neighbours[QUAD_SSE]->corner[QUAD_NW];
	else
		edges[QUAD_S] = create_quad_point(quad->center->pos.x, quad->corner[QUAD_SW]->pos.y, point_func);

	for(int i = 0; i < 4; i++) {
		switch(i) {
			case 0:
				corners[QUAD_NW] = quad->corner[QUAD_NW];
				corners[QUAD_NE] = edges[QUAD_N];
				corners[QUAD_SW] = edges[QUAD_W];
				corners[QUAD_SE] = quad->center;
				break;
			case 1:
				corners[QUAD_NW] = edges[QUAD_N];
				corners[QUAD_NE] = quad->corner[QUAD_NE];
				corners[QUAD_SW] = quad->center;
				corners[QUAD_SE] = edges[QUAD_E];
				break;
			case 2:
				corners[QUAD_NW] = edges[QUAD_W];
				corners[QUAD_NE] = quad->center;
				corners[QUAD_SW] = quad->corner[QUAD_SW];
				corners[QUAD_SE] = edges[QUAD_S];
				break;
			case 3:
				corners[QUAD_NW] = quad->center;
				corners[QUAD_NE] = edges[QUAD_E];
				corners[QUAD_SW] = edges[QUAD_S];
				corners[QUAD_SE] = quad->corner[QUAD_SE];
				break;
			default:
				corners[QUAD_NW] = NULL;
				corners[QUAD_NE] = NULL;
				corners[QUAD_SW] = NULL;
				corners[QUAD_SE] = NULL;
		}

		quad->subquads[i] = create_quad_from_four_points(quad, corners[QUAD_NW], corners[QUAD_NE], corners[QUAD_SW], corners[QUAD_SE], point_func);
	}
	update_neighbours_after_split(quad->subquads, neighbours);
	quad->flags = 0;

	return num_splits+1;
}

void free_quad(Quad *quad, bool free_mesh_point) {
	if(!is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) {
		for(int i = 0; i < 4; i++) {
			if(quad->subquads[i]) {
				free_quad(quad->subquads[i], free_mesh_point);
			}
		}
	} else {
		for(int i = 0; i < 8; i++) {
			if(!quad->neighbours[i]) continue;
			for(int j = 0; j < 8; j++) {
				if(!quad->neighbours[i]->neighbours[j]) continue;
				if(quad->neighbours[i]->neighbours[j] == quad) quad->neighbours[i]->neighbours[j] = NULL;
			}
		}



		if(free_mesh_point) {
			// free mesh point if lonely
		}
	}

	if(quad->parent) {
		for(int i = 0; i < 4; i++) {
			if(quad->parent->subquads[i] == quad) quad->parent->subquads[i] = NULL;
		}
	}

	free(quad);
}