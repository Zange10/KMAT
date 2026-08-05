#include "quad.h"

#include <math.h>
#include <stdio.h>
#include <string.h>


QuadList * create_quad_list() {
	QuadList *quad_list = malloc(sizeof(QuadList));
	quad_list->cap = 0;
	quad_list->num = 0;
	quad_list->quad = NULL;
	return quad_list;
}

void append_to_quad_list(QuadList *quad_list, Quad *quad) {
	if(quad_list->num >= quad_list->cap) {
		if(quad_list->cap == 0) {
			quad_list->cap = 8;
			quad_list->quad = malloc(quad_list->cap*sizeof(Quad));
		} else {
			quad_list->cap *= 2;
			Quad **temp = realloc(quad_list->quad, quad_list->cap*sizeof(Quad));
			if(temp) quad_list->quad = temp;
		}
	}
	quad_list->quad[quad_list->num++] = quad;
}

void remove_from_quad_list_at_idx(QuadList *quad_list, size_t idx) {
	if(!quad_list || idx >= quad_list->num) return;
	memmove(quad_list->quad+idx, quad_list->quad+idx+1, (quad_list->num-idx-1) * sizeof(Quad*));
	quad_list->num--;
}

void clear_quad_list(QuadList *quad_list) {
	quad_list->num = 0;
}

void free_quad_list(QuadList *quad_list) {
	free(quad_list->quad);
	free(quad_list);
}

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
	if(!root_quad->parent && !is_inside_quad(root_quad, pos)) return NULL;
	if(is_quad_flag(root_quad, QUAD_FLAG_IS_LEAF)) return root_quad;

	for(int i = 0; i < 4; i++) {
		if(is_inside_quad(root_quad->subquads[i], pos)) {
			Quad *quad_at_pos = get_quad_at_position(root_quad->subquads[i], pos);
			if(quad_at_pos) return quad_at_pos;
		}
	}
	return NULL;
}

void populate_quad_mesh_points(Quad *quad, QuadPointPopFunc *point_pop_func) {
	if(!quad) return;
	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) {
		point_pop_func->func(quad->center, point_pop_func->params);
		for(int i = 0; i < 4; i++) {
			point_pop_func->func(quad->corner[i], point_pop_func->params);
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

Vector2 get_quad_z_line_side_cut_wrt_neighbour_rflevel(Quad *quad, QuadEdge edge, double z, int val_idx) {
	Quad *n0 = NULL, *n1 = NULL;
	Vector3 p0, p1, pm = vec3(0, 0, 0);

	switch(edge) {
		case QUAD_N:
			n0 = quad->neighbours[QUAD_NNW];
			n1 = quad->neighbours[QUAD_NNE];
			p0 = meshpoint_to_vector(quad->corner[QUAD_NW], val_idx);
			p1 = meshpoint_to_vector(quad->corner[QUAD_NE], val_idx);
			break;
		case QUAD_W:
			n0 = quad->neighbours[QUAD_NWW];
			n1 = quad->neighbours[QUAD_SWW];
			p0 = meshpoint_to_vector(quad->corner[QUAD_NW], val_idx);
			p1 = meshpoint_to_vector(quad->corner[QUAD_SW], val_idx);
			break;
		case QUAD_E:
			n0 = quad->neighbours[QUAD_NEE];
			n1 = quad->neighbours[QUAD_SEE];
			p0 = meshpoint_to_vector(quad->corner[QUAD_NE], val_idx);
			p1 = meshpoint_to_vector(quad->corner[QUAD_SE], val_idx);
			break;
		case QUAD_S:
			n0 = quad->neighbours[QUAD_SSW];
			n1 = quad->neighbours[QUAD_SSE];
			p0 = meshpoint_to_vector(quad->corner[QUAD_SW], val_idx);
			p1 = meshpoint_to_vector(quad->corner[QUAD_SE], val_idx);
			break;
		default: return vec2(NAN, NAN);
	}

	bool has_middle_point = false;

	if(n0 && n0->rf_level > quad->rf_level || n1 && n1->rf_level > quad->rf_level) {
		has_middle_point = true;
		if(n0) {
			switch(edge) {
				case QUAD_N:
				case QUAD_W: pm = meshpoint_to_vector(n0->corner[QUAD_SE], val_idx); break;
				case QUAD_E: pm = meshpoint_to_vector(n0->corner[QUAD_SW], val_idx); break;
				case QUAD_S: pm = meshpoint_to_vector(n0->corner[QUAD_NE], val_idx); break;
				default: return vec2(NAN, NAN);
			}
		} else {
			switch(edge) {
				case QUAD_N: pm = meshpoint_to_vector(n1->corner[QUAD_SW], val_idx); break;
				case QUAD_W: pm = meshpoint_to_vector(n1->corner[QUAD_NE], val_idx); break;
				case QUAD_E:
				case QUAD_S: pm = meshpoint_to_vector(n1->corner[QUAD_NW], val_idx); break;
				default: return vec2(NAN, NAN);
			}
		}
	}

	if(!has_middle_point) {
		if(edge == QUAD_N || edge == QUAD_S) {
			double x = get_x_value_from_y_value_of_line(vec2(p0.x, p0.z), vec2(p1.x, p1.z), z);
			if(x >= p0.x && x <= p1.x) return vec2(x, p0.y);
		} else {
			double y = get_x_value_from_y_value_of_line(vec2(p0.y, p0.z), vec2(p1.y, p1.z), z);
			if(y < p0.y && y > p1.y) return vec2(p0.x, y);
		}
	} else {
		if(edge == QUAD_N || edge == QUAD_S) {
			double x = get_x_value_from_y_value_of_line(vec2(p0.x, p0.z), vec2(pm.x, pm.z), z);
			if(x >= p0.x && x <= pm.x) return vec2(x, p0.y);
			x = get_x_value_from_y_value_of_line(vec2(pm.x, pm.z), vec2(p1.x, p1.z), z);
			if(x >= pm.x && x <= p1.x) return vec2(x, p1.y);
		} else {
			double y = get_x_value_from_y_value_of_line(vec2(p0.y, p0.z), vec2(pm.y, pm.z), z);
			if(y < p0.y && y > pm.y) return vec2(p0.x, y);
			y = get_x_value_from_y_value_of_line(vec2(pm.y, pm.z), vec2(p1.y, p1.z), z);
			if(y < pm.y && y > p1.y) return vec2(p1.x, y);
		}
	}
	return vec2(NAN, NAN);
}

DataArray2 * calc_quad_z_line(Quad *quad, double z, int num, int val_idx) {
	Vector3 p0 = meshpoint_to_vector(quad->corner[QUAD_NW], val_idx);
	Vector3 p1 = meshpoint_to_vector(quad->corner[QUAD_NE], val_idx);
	Vector3 p2 = meshpoint_to_vector(quad->corner[QUAD_SW], val_idx);
	Vector3 p3 = meshpoint_to_vector(quad->corner[QUAD_SE], val_idx);

	DataArray2 *array = data_array2_create();

	for(int i = 0; i < 4; i++) {
		Vector2 p = get_quad_z_line_side_cut_wrt_neighbour_rflevel(quad, i, z, val_idx);
		if(isnan(p.x)) continue;
		// while(data_array2_size(array) > 1 && p.x >= data_array2_get(array, 0).x && p.x <= data_array2_get(array, -1).x) {
		// 	data_array2_remove_at_idx(array, 0);
		// 	if(data_array2_size(array) > 0) data_array2_remove_at_idx(array, -1);
		// }
		data_array2_insert_new(array, p);
	}

	if(data_array2_size(array) <= 1) return array;

	double tx0 = (data_array2_get(array, 0).x-p0.x)/(p1.x-p0.x);
	double tx1 = (data_array2_get(array, 1).x-p0.x)/(p1.x-p0.x);
	double dtx = tx1-tx0;

	for(int i = 1; i < num-1; i++) {
		double tx = tx0 + dtx/(num-1) * i;

		double a = p0.z-p1.z;
		double b = z-p0.z;
		double c = -p0.z+p2.z;
		double d = p0.z-p1.z-p2.z+p3.z;

		double ty = (a*tx + b)/(c+d*tx);
		if(ty > 0 && ty < 1) {
			double x = p0.x + tx*(p1.x-p0.x);
			double y = p0.y + ty*(p2.y-p0.y);
			data_array2_insert_new(array, vec2(x, y));
		}
	}

	return array;
}

double get_partial_quad_mesh_z_derivative_of_y_wrt_x(Quad *root_quad, Vector2 pos, int val_idx) {
	Quad *quad = get_quad_at_position(root_quad, pos);

	Vector3 p0 = meshpoint_to_vector(quad->corner[QUAD_NW], val_idx);
	Vector3 p1 = meshpoint_to_vector(quad->corner[QUAD_NE], val_idx);
	Vector3 p2 = meshpoint_to_vector(quad->corner[QUAD_SW], val_idx);
	Vector3 p3 = meshpoint_to_vector(quad->corner[QUAD_SE], val_idx);

	double tx = (pos.x - p0.x)/(p1.x - p0.x);
	double a = -p0.z+p2.z;
	double b = p0.z-p1.z-p2.z+p3.z;
	double dz_dty = a+b*tx;
	double dz_dy = dz_dty/(p2.y-p0.y);

	return dz_dy;
}

Vector3 get_quad_max_values(Quad *quad, int quad_val_idx) {
	if(!quad) return vec3(NAN, NAN, NAN);
	Vector3 max = vec3(NAN, NAN, NAN);

	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) {
		for(int i = 0; i < 4; i++) {
			if(isnan(max.x) || quad->corner[i]->pos.x > max.x) max.x = quad->corner[i]->pos.x;
			if(isnan(max.y) || quad->corner[i]->pos.y > max.y) max.y = quad->corner[i]->pos.y;
			if(quad_val_idx >= 0 && quad_val_idx < quad->corner[i]->num_val) {
				double val = quad->corner[i]->val[quad_val_idx];
				if(isnan(max.z) || val > max.z) max.z = val;
			}
		}
		double val = quad->center->num_val > 0 ? quad->center->val[quad_val_idx] : NAN;
		if(quad_val_idx >= 0 && (isnan(max.z) || val > max.z)) max.z = val;
	} else {
		for(int i = 0; i < 4; i++) {
			if(quad->subquads[i]) {
				Vector3 vals = get_quad_max_values(quad->subquads[i], quad_val_idx);
				if(isnan(max.x) || vals.x > max.x) max.x = vals.x;
				if(isnan(max.y) || vals.y > max.y) max.y = vals.y;
				if(isnan(max.z) || vals.z > max.z) max.z = vals.z;
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
			if(isnan(min.x) || quad->corner[i]->pos.x < min.x) min.x = quad->corner[i]->pos.x;
			if(isnan(min.y) || quad->corner[i]->pos.y < min.y) min.y = quad->corner[i]->pos.y;
			if(quad_val_idx >= 0 && quad_val_idx < quad->corner[i]->num_val) {
				double val = quad->corner[i]->val[quad_val_idx];
				if(isnan(min.z) || val < min.z) min.z = val;
			}
		}
		double val = quad->center->num_val > 0 ? quad->center->val[quad_val_idx] : NAN;
		if(quad_val_idx >= 0 && (isnan(min.z) || val < min.z)) min.z = val;
	} else {
		for(int i = 0; i < 4; i++) {
			if(quad->subquads[i]) {
				Vector3 vals = get_quad_min_values(quad->subquads[i], quad_val_idx);
				if(isnan(min.x) || vals.x < min.x) min.x = vals.x;
				if(isnan(min.y) || vals.y < min.y) min.y = vals.y;
				if(isnan(min.z) || vals.z < min.z) min.z = vals.z;
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

int get_quad_leaves(Quad *quad, QuadList *quad_list) {
	if(!quad) return 0;
	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) {
		if(quad_list) append_to_quad_list(quad_list, quad);
		return 1;
	}
	int sum = 0;
	for(int i = 0; i < 4; i++) {
		if(quad->subquads[i]) {
			sum += get_quad_leaves(quad->subquads[i], quad_list);
		}
	}
	return sum;
}

int get_quads_with_nan(Quad *quad, QuadList *quad_list, int quad_val_idx) {
	if(!quad) return 0;
	if(!quad_list) return 0;
	if(quad_val_idx < 0 || quad_val_idx > quad->center->num_val) return 0;
	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) {
		if(isnan(get_quad_interpolated_value(quad, quad->center->pos, quad_val_idx)) || isnan(quad->center->val[quad_val_idx])) {
			append_to_quad_list(quad_list, quad);
			return 1;
		}
		return 0;
	}
	int sum = 0;
	for(int i = 0; i < 4; i++) {
		if(quad->subquads[i]) {
			sum += get_quads_with_nan(quad->subquads[i], quad_list, quad_val_idx);
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

void find_line_crossed_quads(Quad *quad, DataArray2 *line, QuadList *quad_list) {
	if(!quad) return;

	if(!is_quad_crossed_by_line(quad, line)) return;

	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) {
		append_to_quad_list(quad_list, quad);
	} else {
		for(int i = 0; i < 4; i++) {
			if(quad->subquads[i]) {
				find_line_crossed_quads(quad->subquads[i], line, quad_list);
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

double quad_abs_center_error(Quad *quad, int val_idx) {
	double interp_val = get_quad_interpolated_value(quad, quad->center->pos, val_idx);
	return fabs(interp_val - quad->center->val[val_idx]);
}

double quad_rel_center_error(Quad *quad, int val_idx) {
	double interp_val = get_quad_interpolated_value(quad, quad->center->pos, val_idx);
	return fabs(interp_val - quad->center->val[val_idx])/quad->center->val[val_idx];
}

int update_quad_error_flag(Quad *quad, QuadErrorFunc *errfunc) {
	if(!quad) return 0;
	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) {
		if(errfunc->func(quad, errfunc->params)) {
			set_quad_flag(quad, QUAD_FLAG_ACC_ERR);
			return 1;
		}
		return 0;
	}
	int sum = 0;
	for(int i = 0; i < 4; i++) {
		if(quad->subquads[i]) {
			sum += update_quad_error_flag(quad->subquads[i], errfunc);
		}
	}
	return sum;
}

int split_quads_with_flag(Quad *quad, QuadPointFunc *point_func) {
	if(!quad) return 0;
	int num_splits = 0;
	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) {
		if(is_quad_flag(quad, QUAD_FLAG_SPLIT)) {
			num_splits += split_quad(quad, point_func, NULL);
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

int split_quad(Quad *quad, QuadPointFunc *point_func, QuadList *quad_list) {
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
				num_splits += split_quad(neighbours[i], point_func, quad_list);
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
		if(quad_list) append_to_quad_list(quad_list, quad->subquads[i]);
	}
	update_neighbours_after_split(quad->subquads, neighbours);
	quad->flags = 0;

	return num_splits+1;
}

int split_to_refinement_level(Quad *quad, QuadPointFunc *point_func, int min_rf_level) {
	if(!quad) return 0;
	int num_splits = 0;
	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) {
		if(quad->rf_level < min_rf_level) {
			num_splits += split_quad(quad, point_func, NULL);
		} else return 0;
	}
	for(int i = 0; i < 4; i++) {
		if(quad->subquads[i]) {
			num_splits += split_to_refinement_level(quad->subquads[i], point_func, min_rf_level);
		}
	}
	return num_splits;
}

void quad_divide_and_conquer(Quad *root_quad, int max_rf_level, QuadPointFunc *point_func, QuadErrorFunc *error_func, QuadBoundsFunc *bounds_func) {
	int num_split_cycles = 0;

	QuadList *quad_list = create_quad_list();
	QuadList *quad_split_list = create_quad_list();

	get_quad_leaves(root_quad, quad_list);

	for(int i = 0; i < 100; i++) {
		num_split_cycles++;
		for(int j = 0; j < quad_list->num; j++) {
			update_quad_error_flag(quad_list->quad[j], error_func);
			if(is_quad_flag(quad_list->quad[j], QUAD_FLAG_ACC_ERR) && quad_list->quad[j]->rf_level < max_rf_level) {
				append_to_quad_list(quad_split_list, quad_list->quad[j]);
				set_quad_flag(quad_list->quad[j], QUAD_FLAG_SPLIT);
			}
		}

		int num_splits = 0;
		clear_quad_list(quad_list);
		for(int j = 0; j < quad_split_list->num; j++) {
			num_splits += split_quad(quad_split_list->quad[j], point_func, quad_list);
		}
		clear_quad_list(quad_split_list);

		printf("Number of Splits during cycle %d: %d\n", num_split_cycles, num_splits);

		for(int j = 0; j < quad_list->num; j++) {
			Quad *quad = quad_list->quad[j];
			bool is_out_of_bounds = !bounds_func->func(quad, bounds_func->params);
			if(is_out_of_bounds) {
				remove_from_quad_list_at_idx(quad_list, j);
				free_quad(quad, true);
				j--;
			}
		}
		if(num_splits == 0) break;
	}

	free_quad_list(quad_list);
	free_quad_list(quad_split_list);
	printf("Num Split Cycles: %d\n", num_split_cycles);
	printf("Num of Leaves: %d\n", get_quad_leaves(root_quad, NULL));
}

void copy_subquad_skeleton(Quad *quad_to_copy, Quad *new_quad) {
	if(is_quad_flag(quad_to_copy, QUAD_FLAG_IS_LEAF)) return;
	split_quad(new_quad, NULL, NULL);

	for(int i = 0; i < 4; i++) {
		if(quad_to_copy->subquads[i]) copy_subquad_skeleton(quad_to_copy->subquads[i], new_quad->subquads[i]);
		else free_quad(new_quad->subquads[i], true);
	}
}

Quad * copy_quad_skeleton(Quad *quad) {
	MeshPoint2 *p00 = create_quad_point(quad->corner[QUAD_NW]->pos.x, quad->corner[QUAD_NW]->pos.y, NULL);
	MeshPoint2 *p01 = create_quad_point(quad->corner[QUAD_NE]->pos.x, quad->corner[QUAD_NE]->pos.y, NULL);
	MeshPoint2 *p10 = create_quad_point(quad->corner[QUAD_SW]->pos.x, quad->corner[QUAD_SW]->pos.y, NULL);
	MeshPoint2 *p11 = create_quad_point(quad->corner[QUAD_SE]->pos.x, quad->corner[QUAD_SE]->pos.y, NULL);
	Quad *new_quad = create_quad_from_four_points(NULL, p00, p01, p10, p11, NULL);
	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) return new_quad;

	split_quad(new_quad, NULL, NULL);

	for(int i = 0; i < 4; i++) {
		if(quad->subquads[i]) copy_subquad_skeleton(quad->subquads[i], new_quad->subquads[i]);
		else free_quad(new_quad->subquads[i], true);
	}

	return new_quad;
}

void create_mesh_triangles_from_quads(Quad *quad, Mesh2 *mesh) {
	if(!quad) return;
	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) {
		add_point_to_mesh(mesh, quad->center);
		for(int i = 0; i < 4; i++)
			if(!quad->corner[i]->triangles) add_point_to_mesh(mesh, quad->corner[i]);

		MeshPoint2 *p0 = quad->center, *p1 = quad->corner[QUAD_NW], *p2;
		for(int i = 0; i < 8; i++) {
			if(i == 1) p2 = quad->corner[QUAD_NE];
			if(i == 3) p2 = quad->corner[QUAD_SE];
			if(i == 5) p2 = quad->corner[QUAD_SW];
			if(i == 7) p2 = quad->corner[QUAD_NW];

			if(i == 0) {
				if(quad->neighbours[QUAD_NNW]) {
					if(quad->neighbours[QUAD_NNW]->rf_level > quad->rf_level)
						p2 = quad->neighbours[QUAD_NNW]->corner[QUAD_SE];
					else
						continue;
				} else if(quad->neighbours[QUAD_NNE]) {
					p2 = quad->neighbours[QUAD_NNE]->corner[QUAD_SW];
				} else {
					continue;
				}
			}

			if(i == 2) {
				if(quad->neighbours[QUAD_NEE]) {
					if(quad->neighbours[QUAD_NEE]->rf_level > quad->rf_level)
						p2 = quad->neighbours[QUAD_NEE]->corner[QUAD_SW];
					else
						continue;
				} else if(quad->neighbours[QUAD_SEE]) {
					p2 = quad->neighbours[QUAD_SEE]->corner[QUAD_NW];
				} else {
					continue;
				}
			}

			if(i == 4) {
				if(quad->neighbours[QUAD_SSW]) {
					if(quad->neighbours[QUAD_SSW]->rf_level > quad->rf_level)
						p2 = quad->neighbours[QUAD_SSW]->corner[QUAD_NE];
					else
						continue;
				} else if(quad->neighbours[QUAD_SSE]) {
					p2 = quad->neighbours[QUAD_SSE]->corner[QUAD_NW];
				} else {
					continue;
				}
			}

			if(i == 6) {
				if(quad->neighbours[QUAD_NWW]) {
					if(quad->neighbours[QUAD_NWW]->rf_level > quad->rf_level)
						p2 = quad->neighbours[QUAD_NWW]->corner[QUAD_SE];
					else
						continue;
				} else if(quad->neighbours[QUAD_SWW]) {
					p2 = quad->neighbours[QUAD_SWW]->corner[QUAD_NE];
				} else {
					continue;
				}
			}
			int rf_level = quad->rf_level + (i%2 == 0);
			// p2 before p1 because CCW
			add_triangle_to_mesh(mesh, create_triangle_from_three_points_with_rf_level(p0, p2, p1, rf_level, rf_level));
			p1 = p2;
		}
	} else {
		for(int i = 0; i < 4; i++) {
			if(quad->subquads[i]) create_mesh_triangles_from_quads(quad->subquads[i], mesh);
		}
	}
}

Mesh2 * create_mesh_from_quads(Quad *root_quad) {
	if(!root_quad) return NULL;
	Mesh2 *mesh = new_mesh();
	create_mesh_triangles_from_quads(root_quad, mesh);
	Vector3 min = get_quad_min_values(root_quad, -1);
	Vector3 max = get_quad_max_values(root_quad, -1);
	mesh->mesh_box->min = vec2(min.x, min.y);
	mesh->mesh_box->max = vec2(max.x, max.y);
	rebuild_mesh_boxes(mesh);
	return mesh;
}

void free_quad(Quad *quad, bool free_mesh_point) {
	if(!quad) return;
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