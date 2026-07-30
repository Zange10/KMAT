#include "boundary.h"

#include <math.h>
#include <string.h>

#include "itin_rework_tools.h"
#include "external/orbitlib/external/geometrylib/src/data_array_def.h"
#include "gui/gui_tools/coordinate_system_drawing.h"


Boundary create_new_boundary() {
	return (Boundary) {NULL, NULL, 0, 0};
}

void append_to_boundary(Boundary *bdr, DataArray2 *upper, DataArray2 *lower) {
	if(bdr->num >= bdr->cap) {
		if(bdr->cap == 0) {
			bdr->cap = 4;
			bdr->upper_bdrs = malloc(bdr->cap*sizeof(DataArray2 *));
			bdr->lower_bdrs = malloc(bdr->cap*sizeof(DataArray2 *));
		} else {
			bdr->cap *= 2;
			DataArray2 **temp = realloc(bdr->upper_bdrs, bdr->cap*sizeof(Quad));
			if(temp) bdr->upper_bdrs = temp;
			temp = realloc(bdr->lower_bdrs, bdr->cap*sizeof(Quad));
			if(temp) bdr->lower_bdrs = temp;
		}
	}
	bdr->upper_bdrs[bdr->num] = upper;
	bdr->lower_bdrs[bdr->num] = lower;
	bdr->num++;
}

bool is_point_inside_boundary(Vector2 p, Boundary bdr) {
	for(int i = 0; i < bdr.num; i++) {
		size_t num = data_array2_size(bdr.lower_bdrs[i]);
		Vector2 *ld = data_array2_get_data(bdr.lower_bdrs[i]);

		if(ld[num-1].x < p.x) continue;
		if(ld[0].x > p.x) continue;

		if(	p.y >= interpolate_from_sorted_data_array2(bdr.lower_bdrs[i], p.x) &&
			p.y <= interpolate_from_sorted_data_array2(bdr.upper_bdrs[i], p.x))
			return true;
	}
	return false;
}

bool is_line_crossing_boundary(Vector2 p0, Vector2 p1, Boundary bdr) {
	for(int i = 0; i < bdr.num; i++) {
		size_t num = data_array2_size(bdr.lower_bdrs[i]);
		Vector2 *ld = data_array2_get_data(bdr.lower_bdrs[i]);
		Vector2 *ud = data_array2_get_data(bdr.upper_bdrs[i]);

		if(ld[num-1].x < p0.x && ld[num-1].x < p1.x) continue;
		if(ld[0].x > p0.x && ld[0].x > p1.x) continue;

		for(int j = 0; j < num-1; j++) {
			if(are_line_segments_intersecting2(ld[j], ld[j+1], p0, p1)) return true;
			if(are_line_segments_intersecting2(ud[j], ud[j+1], p0, p1)) return true;
		}
	}
	return false;
}

bool is_quad_crossed_by_boundary(Quad *quad, Boundary bdr) {
	for(int i = 0; i < bdr.num; i++) {
		size_t num = data_array2_size(bdr.lower_bdrs[i]);
		Vector2 *ld = data_array2_get_data(bdr.lower_bdrs[i]);

		if(ld[num-1].x < quad->corner[QUAD_NW]->pos.x) continue;
		if(ld[0].x > quad->corner[QUAD_NE]->pos.x) continue;

		if(is_quad_crossed_by_line(quad, bdr.lower_bdrs[i]) || is_quad_crossed_by_line(quad, bdr.upper_bdrs[i]))
			return true;
	}
	return false;
}

bool is_quad_inside_boundary(Quad *quad, Boundary bdr) {
	for(int i = 0; i < bdr.num; i++) {
		size_t num = data_array2_size(bdr.lower_bdrs[i]);
		Vector2 *ld = data_array2_get_data(bdr.lower_bdrs[i]);

		if(ld[num-1].x < quad->corner[QUAD_NW]->pos.x) continue;
		if(ld[0].x > quad->corner[QUAD_NE]->pos.x) continue;

		if(	quad->center->pos.y >= interpolate_from_sorted_data_array2(bdr.lower_bdrs[i], quad->center->pos.x) &&
			quad->center->pos.y <= interpolate_from_sorted_data_array2(bdr.upper_bdrs[i], quad->center->pos.x))
			return true;

		for(int j = 0; j < 4; j++) {
			if(	quad->corner[j]->pos.y >= interpolate_from_sorted_data_array2(bdr.lower_bdrs[i], quad->corner[j]->pos.x) &&
				quad->corner[j]->pos.y <= interpolate_from_sorted_data_array2(bdr.upper_bdrs[i], quad->corner[j]->pos.x))
				return true;
		}

		if(is_quad_crossed_by_line(quad, bdr.lower_bdrs[i]) || is_quad_crossed_by_line(quad, bdr.upper_bdrs[i]))
			return true;

		if(is_inside_quad(quad, data_array2_get_data(bdr.lower_bdrs[i])[0])) return true;
		if(is_inside_quad(quad, data_array2_get_data(bdr.upper_bdrs[i])[0])) return true;
	}
	return false;
}

Vector2 get_boundary_min(Boundary bdr) {
	Vector2 min = vec2(NAN, NAN);
	for(int i = 0; i < bdr.num; i++) {
		Vector2 m = data_array2_get_min(bdr.lower_bdrs[i]);
		if(isnan(min.x) || m.x < min.x) min.x = m.x;
		if(isnan(min.y) || m.y < min.y) min.y = m.y;
	}
	return min;
}

Vector2 get_boundary_max(Boundary bdr) {
	Vector2 max = vec2(NAN, NAN);
	for(int i = 0; i < bdr.num; i++) {
		Vector2 m = data_array2_get_max(bdr.upper_bdrs[i]);
		if(isnan(max.x) || m.x > max.x) max.x = m.x;
		if(isnan(max.y) || m.y > max.y) max.y = m.y;
	}
	return max;
}

void free_boundary(Boundary *bdr) {
	if(!bdr) return;
	for(int i = 0; i < bdr->num; i++) {
		data_array2_free(bdr->upper_bdrs[i]);
		data_array2_free(bdr->lower_bdrs[i]);
	}
	if(bdr->upper_bdrs)
		free(bdr->upper_bdrs);
	if(bdr->lower_bdrs)
		free(bdr->lower_bdrs);
	bdr->upper_bdrs = NULL;
	bdr->lower_bdrs = NULL;
	bdr->cap = 0;
	bdr->num = 0;
}

typedef struct DataArray2Array {
	DataArray2 **arrs;
	size_t num;
	size_t cap;
} DataArray2Array;

DataArray2Array new_data_array2_array() {
	return (DataArray2Array) {NULL, 0, 0};
}

void append_to_data_array2_array(DataArray2Array *arrs, DataArray2 *arr) {
	if(arrs->num == arrs->cap) {
		if(arrs->cap == 0) {
			arrs->cap = 8;
			arrs->arrs = malloc(arrs->cap * sizeof(DataArray2*));
		} else {
			arrs->cap *= 2;
			DataArray2 **temp = realloc(arrs->arrs, arrs->cap * sizeof(DataArray2*));
			if(temp) arrs->arrs = temp;
		}
	}

	arrs->arrs[arrs->num++] = arr;
}

void remove_from_data_array2_array(DataArray2Array *arrs, int idx, bool free_array) {
	if(!arrs || idx < 0 || idx >= arrs->num) return;
	if(free_array) data_array2_free(arrs->arrs[idx]);
	memmove(arrs->arrs+idx, arrs->arrs+idx+1, (arrs->num-idx-1) * sizeof(DataArray2*));
	arrs->num--;
}

void free_data_array2_array(DataArray2Array *arrs) {
	if(arrs->arrs) {
		for(int i = 0; i < arrs->num; i++) {
			data_array2_free(arrs->arrs[i]);
		}
		free(arrs->arrs);
	}
	arrs->num = 0;
	arrs->cap = 0;
}

void split_at_boundary_ends(DataArray2Array *arrays, DataArray1 *ends_x) {
	for(int i = 0; i < arrays->num; i++) {
		for(int j = 0; j < data_array1_size(ends_x); j++) {
			DataArray2 *array = arrays->arrs[i];
			double end = data_array1_get(ends_x, j);
			if(data_array2_get(array, 0).x >= end || data_array2_get(array, -1).x <= end) continue;
			int idx = data_array2_idx_from_binary_search(array, vec2(end, NAN));
			if(data_array2_get(array, idx).x != end) {
				data_array2_insert_new(array, vec2(end, interpolate_from_sorted_data_array2(array, end)));
			}

			append_to_data_array2_array(arrays, data_array2_slice(array, 0, idx));
			append_to_data_array2_array(arrays, data_array2_slice(array, idx, -1));
			remove_from_data_array2_array(arrays, i, true);
			i--;
			break;
		}
	}
}

void connect_all_consecutive_boundary_arrays(DataArray2Array *arrays) {
	for(int i = 0; i < arrays->num; i++) {
		for(int j = 0; j < arrays->num; j++) {
			if(j == i) continue;

			Vector2 p = data_array2_get(arrays->arrs[i], -1);
			Vector2 p_next = data_array2_get(arrays->arrs[j], 0);
			if(p.x != p_next.x || p.y != p_next.y) {
				continue;
			}

			if(data_array2_size(arrays->arrs[j]) > 1)
				data_array2_append_array(arrays->arrs[i], data_array2_slice(arrays->arrs[j], 1, -1), true);

			remove_from_data_array2_array(arrays, j, true);
			if(j < i) i--;
			j = -1; // set to 0 before next cycle
		}
	}
}

Boundary get_quad_mesh_value_boundary(Quad *quad, double val, int val_idx, int num_quad_points, bool enclose_higher) {
	Boundary bdr = create_new_boundary();
	QuadList *quad_list = create_quad_list();
	get_quad_leaves(quad, quad_list);
	Vector3 quad_min = get_quad_min_values(quad, -1);
	Vector3 quad_max = get_quad_max_values(quad, -1);

	DataArray2Array arrays = new_data_array2_array();

	for(int i = 0; i < quad_list->num; i++) {
		DataArray2 *arr = calc_quad_z_line(quad_list->quad[i], val, num_quad_points, val_idx);
		if(data_array2_size(arr) > 0) {
			append_to_data_array2_array(&arrays, arr);
		} else data_array2_free(arr);
	}

	free_quad_list(quad_list);
	if(arrays.num == 0) return bdr;

	connect_all_consecutive_boundary_arrays(&arrays);

	DataArray2 *upper_array = data_array2_create();
	data_array2_append_new(upper_array, vec2(quad_min.x, quad_max.y+1));
	data_array2_append_new(upper_array, vec2(quad_max.x, quad_max.y+1));
	DataArray2 *lower_array = data_array2_create();
	data_array2_append_new(lower_array, vec2(quad_min.x, quad_min.y-1));
	data_array2_append_new(lower_array, vec2(quad_max.x, quad_min.y-1));
	append_to_data_array2_array(&arrays, upper_array);
	append_to_data_array2_array(&arrays, lower_array);



	DataArray1 *ends_x = data_array1_create();
	for(int i = 0; i < arrays.num; i++) {
		data_array1_insert_new(ends_x, data_array2_get(arrays.arrs[i], 0).x);
		data_array1_insert_new(ends_x, data_array2_get(arrays.arrs[i], -1).x);
	}
	for(int i = 0; i < data_array1_size(ends_x)-1; i++) {
		if(data_array1_get(ends_x, i) == data_array1_get(ends_x, i+1)) data_array1_remove_at_idx(ends_x, i);
	}
	split_at_boundary_ends(&arrays, ends_x);

	while(arrays.num > 0) {
		DataArray2Array x_arrs = new_data_array2_array();
		double x0 = data_array1_get(ends_x, 0);
		data_array1_remove_at_idx(ends_x, 0);

		for(int i = 0; i < arrays.num; i++) {
			if(data_array2_get(arrays.arrs[i], 0).x == x0) {
				append_to_data_array2_array(&x_arrs, arrays.arrs[i]);
				remove_from_data_array2_array(&arrays, i, false);
				i--;
			}
		}

		if(x_arrs.num == 0) continue;

		// Selection sort
		for(int i = 0; i < x_arrs.num-1; i++) {
			int max_idx = i;
			for(int j = i+1; j < x_arrs.num; j++) {
				Vector2 *dm = data_array2_get_data(x_arrs.arrs[max_idx]);
				Vector2 *d = data_array2_get_data(x_arrs.arrs[j]);
				if(d[0].y > dm[0].y) {max_idx = j; continue;}
				if(d[0].y == dm[0].y) {
					double grad_d = (d[1].y - d[0].y)/(d[1].x-d[0].x);
					double grad_dm = (dm[1].y - dm[0].y)/(dm[1].x-dm[0].x);
					if(grad_d > grad_dm) {max_idx = j;}
				}
			}

			if(max_idx != i) {
				DataArray2 *temp = x_arrs.arrs[i];
				x_arrs.arrs[i] = x_arrs.arrs[max_idx];
				x_arrs.arrs[max_idx] = temp;
			}
		}

		if(x_arrs.num == 2) {
			double test_x = data_array2_get(x_arrs.arrs[0], (int) data_array2_size(x_arrs.arrs[0])/2).x;
			DataArray2 *vline = data_array2_create();
			data_array2_append_new(vline, vec2(test_x, quad_min.y));
			data_array2_append_new(vline, vec2(test_x, quad_max.y));

			QuadList *ql = create_quad_list();

			find_line_crossed_quads(quad, vline, ql);
			if(ql->num > 0) {
				double interp_val = get_quad_interpolated_value(ql->quad[0], vec2(test_x, ql->quad[0]->center->pos.y), val_idx);
				if(interp_val > val == enclose_higher) {
					append_to_boundary(&bdr, x_arrs.arrs[0], x_arrs.arrs[1]);
					remove_from_data_array2_array(&x_arrs, 0, false);
					remove_from_data_array2_array(&x_arrs, 0, false);
				}
			}

			free_data_array2_array(&x_arrs);
			data_array2_free(vline);
			free_quad_list(ql);
			continue;
		}

		Vector2 test_p = data_array2_get(x_arrs.arrs[1], (int) data_array2_size(x_arrs.arrs[1])/2);
		double dz_dy = get_partial_quad_mesh_z_derivative_of_y_wrt_x(quad, test_p, val_idx);

		if(enclose_higher == dz_dy < 0) remove_from_data_array2_array(&x_arrs, 0, true);

		while(x_arrs.num > 1) {
			append_to_boundary(&bdr, x_arrs.arrs[0], x_arrs.arrs[1]);
			remove_from_data_array2_array(&x_arrs, 0, false);
			remove_from_data_array2_array(&x_arrs, 0, false);
		}

		free_data_array2_array(&x_arrs);
	}

	free_data_array2_array(&arrays);
	data_array1_free(ends_x);

	return bdr;
}

Vector2 get_line_middle_point(DataArray2 *arr) {
	if(!arr || data_array2_size(arr) == 0) return vec2(NAN, NAN);
	if(data_array2_size(arr) == 1) return data_array2_get(arr, 0);
	Vector2 p;
	if(data_array2_size(arr) == 2) {
		Vector2 p0 = data_array2_get(arr, 0), p1 = data_array2_get(arr, 1);
		p.x = (p0.x + p1.x) / 2;
		p.y = get_y_value_from_x_value_of_line(p0, p1, p.x);
	} else {
		p = data_array2_get(arr, (int) data_array2_size(arr)/2);
	}

	return p;
}

Boundary combine_boundaries(Boundary bdr0, Boundary bdr1) {
	Boundary new_boundary = create_new_boundary();
	remove_boundary_end_connections(&bdr0);
	remove_boundary_end_connections(&bdr1);

	DataArray2Array low_bdrs0 = new_data_array2_array();
	DataArray2Array low_bdrs1 = new_data_array2_array();
	DataArray2Array up_bdrs0 = new_data_array2_array();
	DataArray2Array up_bdrs1 = new_data_array2_array();

	DataArray2Array *low_up_bdrs[4] = {&low_bdrs0, &low_bdrs1, &up_bdrs0, &up_bdrs1};

	// collect all boundaries
	for(int i = 0; i < bdr0.num; i++) {
		append_to_data_array2_array(&low_bdrs0, data_array2_slice(bdr0.lower_bdrs[i], 0, -1));
		append_to_data_array2_array(&up_bdrs0, data_array2_slice(bdr0.upper_bdrs[i], 0, -1));
	}
	for(int i = 0; i < bdr1.num; i++) {
		append_to_data_array2_array(&low_bdrs1, data_array2_slice(bdr1.lower_bdrs[i], 0, -1));
		append_to_data_array2_array(&up_bdrs1, data_array2_slice(bdr1.upper_bdrs[i], 0, -1));
	}

	DataArray1 *ends_x = data_array1_create();
	// collect all ends
	for(int bdrs_id = 0; bdrs_id < 4; bdrs_id++) {
		DataArray2Array *bdrs = low_up_bdrs[bdrs_id];
		for(int i = 0; i < bdrs->num; i++) {
			data_array1_insert_new(ends_x, data_array2_get(bdrs->arrs[i], 0).x);
			data_array1_insert_new(ends_x, data_array2_get(bdrs->arrs[i], -1).x);
		}
	}

	// collect all intersections and store as ends
	for(int bdrs_id = 0; bdrs_id < 4; bdrs_id++) {
		DataArray2Array *bdrs = low_up_bdrs[bdrs_id];
		for(int i = 0; i < bdrs->num; i++) {
			for(int bdrs_id2 = bdrs_id+1; bdrs_id2 < 4; bdrs_id2++) {
				if(bdrs_id%2 == bdrs_id2%2) continue;
				DataArray2Array *bdrs2 = low_up_bdrs[bdrs_id2];
				for(int j = 0; j < bdrs2->num; j++) {
					DataArray2 *arr0 = bdrs->arrs[i];
					DataArray2 *arr1 = bdrs2->arrs[j];
					if(data_array2_get(arr0, 0).x > data_array2_get(arr1, -1).x) continue;
					if(data_array2_get(arr1, 0).x > data_array2_get(arr0, -1).x) continue;

					DataArray2 *inters_p = get_line_intersections(arr0, arr1);
					for(int k = 0; k < data_array2_size(inters_p); k++) {
						if(!isnan(data_array2_get(inters_p, k).x))
							data_array1_insert_new(ends_x, data_array2_get(inters_p, k).x);
					}
					data_array2_free(inters_p);
				}
			}
		}
	}

	// remove double ends
	for(int i = 0; i < data_array1_size(ends_x); i++) {
		if(data_array1_get(ends_x, i) == data_array1_get(ends_x, i-1)) {
			data_array1_remove_at_idx(ends_x, i); i--;
		}
	}

	// split at ends
	for(int bdrs_id = 0; bdrs_id < 4; bdrs_id++) {
		DataArray2Array *bdrs = low_up_bdrs[bdrs_id];
		split_at_boundary_ends(bdrs, ends_x);
	}

	// remove out of bounds from other boundary
	for(int bdrs_id = 0; bdrs_id < 4; bdrs_id++) {
		DataArray2Array *bdrs = low_up_bdrs[bdrs_id];
		Boundary other_bdr = bdrs_id % 2 == 0 ? bdr1 : bdr0;
		for(int i = 0; i < bdrs->num; i++) {
			Vector2 p = get_line_middle_point(bdrs->arrs[i]);

			if(data_array2_size(bdrs->arrs[i]) == 0 || !is_point_inside_boundary(p, other_bdr)) {
				remove_from_data_array2_array(bdrs, i, true);
				i--;
			}
		}
	}

	DataArray2Array low_bdrs = new_data_array2_array();
	DataArray2Array up_bdrs = new_data_array2_array();

	// Move to new arrays
	for(int bdrs_id = 0; bdrs_id < 4; bdrs_id++) {
		DataArray2Array *bdrs = low_up_bdrs[bdrs_id];
		DataArray2Array *new_bdrs = bdrs_id < 2 ? &low_bdrs : &up_bdrs;
		while(bdrs->num > 0) {
			append_to_data_array2_array(new_bdrs, bdrs->arrs[0]);
			remove_from_data_array2_array(bdrs, 0, false);
		}
		free_data_array2_array(bdrs);
	}

	// connect boundary ends
	for(int c = 0; c < 2; c++) {
		DataArray2Array *bdrs = c == 0 ? &low_bdrs : &up_bdrs;
		connect_all_consecutive_boundary_arrays(bdrs);
	}

	data_array1_clear(ends_x);

	// collect all new ends
	for(int c = 0; c < 2; c++) {
		DataArray2Array *bdrs = c == 0 ? &low_bdrs : &up_bdrs;
		for(int i = 0; i < bdrs->num; i++) {
			data_array1_insert_new(ends_x, data_array2_get(bdrs->arrs[i], 0).x);
			data_array1_insert_new(ends_x, data_array2_get(bdrs->arrs[i], -1).x);
		}
	}

	// remove double ends
	for(int i = 0; i < data_array1_size(ends_x); i++) {
		if(data_array1_get(ends_x, i) == data_array1_get(ends_x, i-1)) {
			data_array1_remove_at_idx(ends_x, i); i--;
		}
	}

	// split at ends
	for(int c = 0; c < 2; c++) {
		DataArray2Array *bdrs = c == 0 ? &low_bdrs : &up_bdrs;
		split_at_boundary_ends(bdrs, ends_x);
	}

	// find pairs
	while(low_bdrs.num != 0 && up_bdrs.num != 0) {
		DataArray2Array x_arrs_low = new_data_array2_array();
		DataArray2Array x_arrs_up = new_data_array2_array();
		double x0 = data_array1_get(ends_x, 0);
		data_array1_remove_at_idx(ends_x, 0);

		for(int i = 0; i < low_bdrs.num; i++) {
			if(data_array2_get(low_bdrs.arrs[i], 0).x == x0) {
				append_to_data_array2_array(&x_arrs_low, low_bdrs.arrs[i]);
				remove_from_data_array2_array(&low_bdrs, i, false);
				i--;
			}
		}
		for(int i = 0; i < up_bdrs.num; i++) {
			if(data_array2_get(up_bdrs.arrs[i], 0).x == x0) {
				append_to_data_array2_array(&x_arrs_up, up_bdrs.arrs[i]);
				remove_from_data_array2_array(&up_bdrs, i, false);
				i--;
			}
		}

		if(x_arrs_low.num == 0 || x_arrs_up.num == 0) {
			free_data_array2_array(&x_arrs_low);
			free_data_array2_array(&x_arrs_up);
			continue;
		}

		// Selection sort
		for(int c = 0; c < 2; c++) {
			DataArray2Array *x_arrs = c == 0 ? &x_arrs_low : &x_arrs_up;
			for(int i = 0; i < x_arrs->num-1; i++) {
				int max_idx = i;
				for(int j = i+1; j < x_arrs->num; j++) {
					Vector2 *dm = data_array2_get_data(x_arrs->arrs[max_idx]);
					Vector2 *d = data_array2_get_data(x_arrs->arrs[j]);
					if(d[0].y > dm[0].y) {max_idx = j; continue;}
					if(d[0].y == dm[0].y) {
						double grad_d = (d[1].y - d[0].y)/(d[1].x-d[0].x);
						double grad_dm = (dm[1].y - dm[0].y)/(dm[1].x-dm[0].x);
						if(grad_d > grad_dm) {max_idx = j;}
					}
				}

				if(max_idx != i) {
					DataArray2 *temp = x_arrs->arrs[i];
					x_arrs->arrs[i] = x_arrs->arrs[max_idx];
					x_arrs->arrs[max_idx] = temp;
				}
			}
		}

		while(x_arrs_up.num > 0 && x_arrs_low.num > 0) {
			Vector2 pu = get_line_middle_point(x_arrs_up.arrs[0]);
			Vector2 pl = get_line_middle_point(x_arrs_low.arrs[0]);
			if(pl.y > pu.y) {
				remove_from_data_array2_array(&x_arrs_low, 0, true);
				continue;
			}
			if(x_arrs_up.num > 1) {
				Vector2 pu2 = get_line_middle_point(x_arrs_up.arrs[1]);
				if(pu2.y > pl.y) {
					remove_from_data_array2_array(&x_arrs_up, 0, true);
					continue;
				}
			}
			append_to_boundary(&new_boundary, x_arrs_up.arrs[0], x_arrs_low.arrs[0]);
			remove_from_data_array2_array(&x_arrs_up, 0, false);
			remove_from_data_array2_array(&x_arrs_low, 0, false);
		}
		free_data_array2_array(&x_arrs_low);
		free_data_array2_array(&x_arrs_up);
	}

	free_data_array2_array(&low_bdrs);
	free_data_array2_array(&up_bdrs);
	data_array1_free(ends_x);

	return new_boundary;
}

void connect_boundary_ends(Boundary *bdr) {
	for(int i = 0; i < bdr->num; i++) {
		bool connect_front = true;
		bool connect_back = true;
		Vector2 *u0 = data_array2_get_data(bdr->upper_bdrs[i]);
		Vector2 *l0 = data_array2_get_data(bdr->lower_bdrs[i]);
		size_t num0 = data_array2_size(bdr->upper_bdrs[i]);


		if(u0[0].y == l0[0].y) connect_front = false;
		if(u0[num0-1].y == l0[num0-1].y) connect_back = false;

		for(int j = 0; j < bdr->num; j++) {
			Vector2 *u1 = data_array2_get_data(bdr->upper_bdrs[j]);
			Vector2 *l1 = data_array2_get_data(bdr->lower_bdrs[j]);
			size_t num1 = data_array2_size(bdr->upper_bdrs[i]);
			if(connect_front && u0[0].x == u1[num1-1].x && u0[0].y == u1[num1-1].y) {
				connect_front = false;
			}
			if(connect_front && l0[0].x == l1[num1-1].x && l0[0].y == l1[num1-1].y) {
				connect_front = false;
			}
			if(connect_back && u0[num0-1].x == u1[0].x && u0[num0-1].y == u1[0].y) {
				connect_back = false;
			}
			if(connect_back && l0[num0-1].x == l1[0].x && l0[num0-1].y == l1[0].y) {
				connect_back = false;
			}
		}

		if(connect_front) {
			data_array2_insert_new(bdr->upper_bdrs[i], data_array2_get_data(bdr->lower_bdrs[i])[0]);
			data_array2_insert_new(bdr->lower_bdrs[i], data_array2_get_data(bdr->lower_bdrs[i])[0]);
			num0++;
		}

		if(connect_back) {
			data_array2_append_new(bdr->upper_bdrs[i], data_array2_get_data(bdr->upper_bdrs[i])[num0-1]);
			data_array2_append_new(bdr->lower_bdrs[i], data_array2_get_data(bdr->upper_bdrs[i])[num0-1]);
		}
	}
}

void remove_boundary_end_connections(Boundary *bdr) {
	for(int i = 0; i < bdr->num; i++) {
		Vector2 *u = data_array2_get_data(bdr->upper_bdrs[i]);
		size_t num0 = data_array2_size(bdr->upper_bdrs[i]);

		if(num0 < 2) continue;

		if(u[0].x == u[1].x) {
			data_array2_remove_at_idx(bdr->upper_bdrs[i], 0);
			data_array2_remove_at_idx(bdr->lower_bdrs[i], 0);
			num0--;
		}
		if(num0 < 2) continue;
		if(u[num0-1].x == u[num0-2].x) {
			data_array2_remove_at_idx(bdr->upper_bdrs[i], (int) num0-1);
			data_array2_remove_at_idx(bdr->lower_bdrs[i], (int) num0-1);
		}
	}
}

void set_bdr_color(cairo_t *cr, int group_id, double alpha) {
	switch(group_id%8) {
		case 1:
			cairo_set_source_rgba(cr, 0, 0, 0.8, alpha);
			break;
		case 2:
			cairo_set_source_rgba(cr, 0, 0.8, 0.0, alpha);
			break;
		case 3:
			cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, alpha);
			break;
		case 4:
			cairo_set_source_rgba(cr, 0.8, 0, 0.3, alpha);
			break;
		case 5:
			cairo_set_source_rgba(cr, 0.3, 0.3, 0.3, alpha);
			break;
		case 6:
			cairo_set_source_rgba(cr, 0, 0, 0.3, alpha);
			break;
		case 7:
			cairo_set_source_rgba(cr, 0.8, 0, 0.8, alpha);
			break;
		default:
			cairo_set_source_rgba(cr, 0, 0.8, 0.8, alpha);
			break;
	}
}

void draw_boundary_in_coordinate_system(CoordinateSystem *coord_sys, int group_id) {
	CSDataPointGroup *group = coord_sys->groups[group_id];
	Boundary *bdr = group->bdr;
	cairo_t *cr = coord_sys->screen->static_layer.cr;

	set_bdr_color(cr, group_id, 1);

	double transparency = 0.3;

	int total_number_points = 0;
	for(int i = 0; i < bdr->num; i++) {
		total_number_points += (int) data_array2_size(bdr->lower_bdrs[i]);
		total_number_points += (int) data_array2_size(bdr->upper_bdrs[i]);
	}

	for(int i = 0; i < bdr->num; i++) {
		if(group->plot_type == CS_PLOT_TYPE_BDR_PLOT || group->plot_type == CS_PLOT_TYPE_BDR_PLOT_SCATTER) {
			for(int j = 1; j < data_array2_size(bdr->lower_bdrs[i]); j++) {
				Vector2 p0 = to_coordinate_system_space(data_array2_get(bdr->lower_bdrs[i], j-1), coord_sys);
				Vector2 p1 = to_coordinate_system_space(data_array2_get(bdr->lower_bdrs[i], j), coord_sys);
				draw_line_into_coordinate_system(cr, p0, p1, coord_sys->origin);
			}
			for(int j = 1; j < data_array2_size(bdr->upper_bdrs[i]); j++) {
				Vector2 p0 = to_coordinate_system_space(data_array2_get(bdr->upper_bdrs[i], j-1), coord_sys);
				Vector2 p1 = to_coordinate_system_space(data_array2_get(bdr->upper_bdrs[i], j), coord_sys);
				draw_line_into_coordinate_system(cr, p0, p1, coord_sys->origin);
			}

			set_bdr_color(cr, group_id, transparency);
			Vector2 p = to_coordinate_system_space(data_array2_get(bdr->lower_bdrs[i], 0), coord_sys);
			cairo_move_to(cr, p.x, p.y);
			for(int j = 1; j < data_array2_size(bdr->lower_bdrs[i]); j++) {
				p = to_coordinate_system_space(data_array2_get(bdr->lower_bdrs[i], j), coord_sys);
				cairo_line_to(cr, p.x, p.y);
			}
			for(int j = (int) data_array2_size(bdr->upper_bdrs[i])-1; j >= 0; j--) {
				p = to_coordinate_system_space(data_array2_get(bdr->upper_bdrs[i], j), coord_sys);
				cairo_line_to(cr, p.x, p.y);
			}
			cairo_close_path(cr);
			cairo_fill(cr);
			set_bdr_color(cr, group_id, 1);
		}

		if(group->plot_type == CS_PLOT_TYPE_BDR_SCATTER || group->plot_type == CS_PLOT_TYPE_BDR_PLOT_SCATTER) {
			int radius = 2;
			if(total_number_points < 10000) radius += 2;
			for(int j = 0; j < data_array2_size(bdr->lower_bdrs[i]); j++) {
				Vector2 p = to_coordinate_system_space(data_array2_get(bdr->lower_bdrs[i], j), coord_sys);
				draw_coordinate_system_data_point(cr, p.x, p.y, radius);
			}
			for(int j = 0; j < data_array2_size(bdr->upper_bdrs[i]); j++) {
				Vector2 p = to_coordinate_system_space(data_array2_get(bdr->upper_bdrs[i], j), coord_sys);
				draw_coordinate_system_data_point(cr, p.x, p.y, radius);
			}
		}
	}
}

