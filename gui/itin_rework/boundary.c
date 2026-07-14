#include "boundary.h"

#include <math.h>
#include <string.h>

#include "itin_rework_tools.h"
#include "external/orbitlib/external/geometrylib/src/data_array_def.h"


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
	}
	return false;
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

Boundary combine_boundaries(Boundary bdr0, Boundary bdr1) {
	remove_boundary_end_connections(&bdr0);
	remove_boundary_end_connections(&bdr1);

	Boundary low_bdrs = create_new_boundary();
	Boundary up_bdrs = create_new_boundary();

	// find upper and lower boundaries that are inside other boundary
	for(int i = 0; i < bdr0.num; i++) {
		DataArray2 *lower = data_array2_create();
		DataArray2 *upper = data_array2_create();
		Vector2 *l0 = data_array2_get_data(bdr0.lower_bdrs[i]);
		Vector2 *u0 = data_array2_get_data(bdr0.upper_bdrs[i]);
		size_t num0 = data_array2_size(bdr0.lower_bdrs[i]);
		if(num0 == 1) continue;
		bool prev_was_out_l = false;
		bool prev_was_out_u = false;
		for(int j = 0; j < num0; j++) {
			if(is_point_inside_boundary(l0[j], bdr1)) {
				if(prev_was_out_l) data_array2_append_new(lower, l0[j-1]);
				data_array2_append_new(lower, l0[j]);
				prev_was_out_l = false;
			} else {
				if(!prev_was_out_l && j > 0) {
					data_array2_append_new(lower, l0[j]);
					append_to_boundary(&low_bdrs, NULL, lower);
					lower = data_array2_create();
				}
				prev_was_out_l = true;
			}
			if(is_point_inside_boundary(u0[j], bdr1)) {
				if(prev_was_out_u) data_array2_append_new(upper, u0[j-1]);
				data_array2_append_new(upper, u0[j]);
				prev_was_out_u = false;
			} else {
				if(!prev_was_out_u && j > 0) {
					data_array2_append_new(upper, u0[j]);
					append_to_boundary(&up_bdrs, upper, NULL);
					upper = data_array2_create();
				}
				prev_was_out_u = true;
			}
		}
		if(data_array2_size(lower) > 0) append_to_boundary(&low_bdrs, NULL, lower);
		else data_array2_free(lower);
		if(data_array2_size(upper) > 0) append_to_boundary(&up_bdrs, upper, NULL);
		else data_array2_free(upper);
	}

	for(int i = 0; i < bdr1.num; i++) {
		DataArray2 *lower = data_array2_create();
		DataArray2 *upper = data_array2_create();
		Vector2 *l1 = data_array2_get_data(bdr1.lower_bdrs[i]);
		Vector2 *u1 = data_array2_get_data(bdr1.upper_bdrs[i]);
		size_t num1 = data_array2_size(bdr1.lower_bdrs[i]);
		bool prev_was_out_l = false;
		bool prev_was_out_u = false;
		for(int j = 0; j < num1; j++) {
			if(is_point_inside_boundary(l1[j], bdr0)) {
				if(prev_was_out_l) data_array2_append_new(lower, l1[j-1]);
				data_array2_append_new(lower, l1[j]);
				prev_was_out_l = false;
			} else {
				if(!prev_was_out_l && j > 0) {
					data_array2_append_new(lower, l1[j]);
					append_to_boundary(&low_bdrs, NULL, lower);
					lower = data_array2_create();
				}
				prev_was_out_l = true;
			}
			if(is_point_inside_boundary(u1[j], bdr0)) {
				if(prev_was_out_u) data_array2_append_new(upper, u1[j-1]);
				data_array2_append_new(upper, u1[j]);
				prev_was_out_u = false;
			} else {
				if(!prev_was_out_u && j > 0) {
					data_array2_append_new(upper, u1[j]);
					append_to_boundary(&up_bdrs, upper, NULL);
					upper = data_array2_create();
				}
				prev_was_out_u = true;
			}
		}
		if(data_array2_size(lower) > 0) append_to_boundary(&low_bdrs, NULL, lower);
		else data_array2_free(lower);
		if(data_array2_size(upper) > 0) append_to_boundary(&up_bdrs, upper, NULL);
		else data_array2_free(upper);
	}

	// connect boundaries that are end-to-end touching
	for(int i = 0; i < up_bdrs.num; i++) {
		for(int j = i+1; j < up_bdrs.num; j++) {
			Vector2 *u = data_array2_get_data(up_bdrs.upper_bdrs[i]);
			size_t num = data_array2_size(up_bdrs.upper_bdrs[i]);
			Vector2 *u_n = data_array2_get_data(up_bdrs.upper_bdrs[j]);
			size_t num_n = data_array2_size(up_bdrs.upper_bdrs[j]);

			if(u[num-1].x == u_n[0].x && u[num-1].y == u_n[0].y) {
				for(int k = 1; k < num_n; k++) {
					data_array2_append_new(up_bdrs.upper_bdrs[i], u_n[k]);
				}
				data_array2_free(up_bdrs.upper_bdrs[j]);
				memmove(&up_bdrs.upper_bdrs[j], &up_bdrs.upper_bdrs[j+1],
				(up_bdrs.num - (j+1)) * sizeof(DataArray2*));
				up_bdrs.num--;
				j = i;
			}
		}
	}
	for(int i = 0; i < low_bdrs.num; i++) {
		for(int j = i+1; j < low_bdrs.num; j++) {
			Vector2 *u = data_array2_get_data(low_bdrs.lower_bdrs[i]);
			size_t num = data_array2_size(low_bdrs.lower_bdrs[i]);
			Vector2 *u_n = data_array2_get_data(low_bdrs.lower_bdrs[j]);
			size_t num_n = data_array2_size(low_bdrs.lower_bdrs[j]);

			if(u[num-1].x == u_n[0].x && u[num-1].y == u_n[0].y) {
				for(int k = 1; k < num_n; k++) {
					data_array2_append_new(low_bdrs.lower_bdrs[i], u_n[k]);
				}
				data_array2_free(low_bdrs.lower_bdrs[j]);
				memmove(&low_bdrs.lower_bdrs[j], &low_bdrs.lower_bdrs[j+1],
				(low_bdrs.num - (j+1)) * sizeof(DataArray2*));
				low_bdrs.num--;
				j = i;
			}
		}
	}

	// find intersections and remove intersected ends
	for(int i = 0; i < up_bdrs.num; i++) {
		for(int j = i+1; j < up_bdrs.num; j++) {
			DataArray2 *inters = NULL;
			inters = get_line_intersections(up_bdrs.upper_bdrs[i], up_bdrs.upper_bdrs[j]);
			size_t num = data_array2_size(inters);
			Vector2 *data = data_array2_get_data(inters);
			for(int l = 0; l < num; l++) {
				Vector2 *d = data_array2_get_data(up_bdrs.upper_bdrs[i]);
				size_t num_d = data_array2_size(up_bdrs.upper_bdrs[i]);
				if(data[l].x <= d[1].x) d[0] = data[l];
				else d[num_d-1] = data[l];

				d = data_array2_get_data(up_bdrs.upper_bdrs[j]);
				num_d = data_array2_size(up_bdrs.upper_bdrs[j]);
				if(data[l].x <= d[1].x) d[0] = data[l];
				else d[num_d-1] = data[l];
			}
			data_array2_free(inters);
		}

		for(int j = 0; j < low_bdrs.num; j++) {
			DataArray2 *inters = NULL;
			inters = get_line_intersections(up_bdrs.upper_bdrs[i], low_bdrs.lower_bdrs[j]);
			size_t num = data_array2_size(inters);
			Vector2 *data = data_array2_get_data(inters);
			for(int l = 0; l < num; l++) {
				Vector2 *d = data_array2_get_data(up_bdrs.upper_bdrs[i]);
				size_t num_d = data_array2_size(up_bdrs.upper_bdrs[i]);
				if(data[l].x <= d[1].x) d[0] = data[l];
				else d[num_d-1] = data[l];

				d = data_array2_get_data(low_bdrs.lower_bdrs[j]);
				num_d = data_array2_size(low_bdrs.lower_bdrs[j]);
				if(data[l].x <= d[1].x) d[0] = data[l];
				else d[num_d-1] = data[l];
			}
			data_array2_free(inters);
		}
	}

	for(int i = 0; i < low_bdrs.num; i++) {
		for(int j = i+1; j < low_bdrs.num; j++) {
			DataArray2 *inters = NULL;
			inters = get_line_intersections(low_bdrs.lower_bdrs[i], low_bdrs.lower_bdrs[j]);
			size_t num = data_array2_size(inters);
			Vector2 *data = data_array2_get_data(inters);
			for(int l = 0; l < num; l++) {
				Vector2 *d = data_array2_get_data(low_bdrs.lower_bdrs[i]);
				size_t num_d = data_array2_size(low_bdrs.lower_bdrs[i]);
				if(data[l].x <= d[1].x) d[0] = data[l];
				else d[num_d-1] = data[l];

				d = data_array2_get_data(low_bdrs.lower_bdrs[j]);
				num_d = data_array2_size(low_bdrs.lower_bdrs[j]);
				if(data[l].x <= d[1].x) d[0] = data[l];
				else d[num_d-1] = data[l];
			}
			data_array2_free(inters);
		}
	}

	// get x values of each boundary end
	DataArray1 *end_x = data_array1_create();
	for(int i = 0; i < low_bdrs.num; i++) {
		Vector2 *d = data_array2_get_data(low_bdrs.lower_bdrs[i]);
		size_t num_d = data_array2_size(low_bdrs.lower_bdrs[i]);
		data_array1_insert_new(end_x, d[0].x);
		data_array1_insert_new(end_x, d[num_d-1].x);
	}
	for(int i = 0; i < up_bdrs.num; i++) {
		Vector2 *d = data_array2_get_data(up_bdrs.upper_bdrs[i]);
		size_t num_d = data_array2_size(up_bdrs.upper_bdrs[i]);
		data_array1_insert_new(end_x, d[0].x);
		data_array1_insert_new(end_x, d[num_d-1].x);
	}


	// insert boundary points at end x-values
	double *de = data_array1_get_data(end_x);
	size_t num_de = data_array1_size(end_x);
	double last_x = NAN;
	for(int i = 0; i < num_de; i++) {
		if(de[i] == last_x) continue;
		double x = de[i];

		for(int j = 0; j < low_bdrs.num; j++) {
			Vector2 *d = data_array2_get_data(low_bdrs.lower_bdrs[j]);
			size_t num_d = data_array2_size(low_bdrs.lower_bdrs[j]);
			if(d[0].x < x && d[num_d-1].x > x) {
				bool has_value_already = false;
				for(int k = 0; k < num_d; k++) {
					if(d[k].x == x) {
						has_value_already = true; break;
					}
				}
				if(!has_value_already) {
					// printf("%f\n", interpolate_from_sorted_data_array2(low_bdrs.lower_bdrs[j], x));
					data_array2_insert_new(low_bdrs.lower_bdrs[j], vec2(x, interpolate_from_sorted_data_array2(low_bdrs.lower_bdrs[j], x)));
					d = data_array2_get_data(low_bdrs.lower_bdrs[j]);
					num_d++;
				}
			}
		}

		for(int j = 0; j < up_bdrs.num; j++) {
			Vector2 *d = data_array2_get_data(up_bdrs.upper_bdrs[j]);
			size_t num_d = data_array2_size(up_bdrs.upper_bdrs[j]);
			if(d[0].x < x && d[num_d-1].x > x) {
				bool has_value_already = false;
				for(int k = 0; k < num_d; k++) {
					if(d[k].x == x) {
						has_value_already = true; break;
					}
				}
				if(!has_value_already) {
					data_array2_insert_new(up_bdrs.upper_bdrs[j], vec2(x, interpolate_from_sorted_data_array2(up_bdrs.upper_bdrs[j], x)));
					d = data_array2_get_data(up_bdrs.upper_bdrs[j]);
					num_d++;
				}
			}
		}

		last_x = x;
		// print_date(convert_JD_date(x, DATE_ISO), 1);
	}


	// split boundaries at all end x-values (create sections)
	Boundary low_split_bdrs = create_new_boundary();
	Boundary up_split_bdrs = create_new_boundary();
	for(int i = 0; i < num_de; i++) {
		if(de[i] == last_x) continue;
		double x = de[i];

		for(int j = 0; j < low_bdrs.num; j++) {
			Vector2 *d = data_array2_get_data(low_bdrs.lower_bdrs[j]);
			size_t num_d = data_array2_size(low_bdrs.lower_bdrs[j]);
			if(d[0].x < x && d[num_d-1].x >= x) {
				DataArray2 *new_split_bdr = data_array2_create();
				for(int k = 0; k < num_d; k++) {
					if(d[k].x < last_x) continue;
					if(d[k].x > x) break;
					data_array2_append_new(new_split_bdr, d[k]);
				}
				if(data_array2_size(new_split_bdr) > 0) {
					append_to_boundary(&low_split_bdrs, NULL, new_split_bdr);
				} else data_array2_free(new_split_bdr);
			}
		}

		for(int j = 0; j < up_bdrs.num; j++) {
			Vector2 *d = data_array2_get_data(up_bdrs.upper_bdrs[j]);
			size_t num_d = data_array2_size(up_bdrs.upper_bdrs[j]);
			if(d[0].x < x && d[num_d-1].x >= x) {
				DataArray2 *new_split_bdr = data_array2_create();
				for(int k = 0; k < num_d; k++) {
					if(d[k].x < last_x) continue;
					if(d[k].x > x) break;
					data_array2_append_new(new_split_bdr, d[k]);
				}
				if(data_array2_size(new_split_bdr) > 0) {
					append_to_boundary(&up_split_bdrs, new_split_bdr, NULL);
				} else data_array2_free(new_split_bdr);
			}
		}

		last_x = x;
	}

	data_array1_free(end_x);
	free_boundary(&up_bdrs);
	free_boundary(&low_bdrs);


	Boundary new_boundary = create_new_boundary();
	while(low_split_bdrs.num > 0) {
		Vector2 *d = data_array2_get_data(low_split_bdrs.lower_bdrs[0]);
		size_t num_d = data_array2_size(low_split_bdrs.lower_bdrs[0]);
		DataArray2 *best_up = NULL;
		int best_up_idx = -1;
		double diff_best = NAN;
		for(int j = 0; j < up_split_bdrs.num; j++) {
			Vector2 *du = data_array2_get_data(up_split_bdrs.upper_bdrs[j]);
			size_t num_du = data_array2_size(up_split_bdrs.upper_bdrs[j]);
			if(du[0].x != d[0].x) continue;
			if(du[0].y < d[0].y) continue;
			if(isnan(diff_best) || du[0].y - d[0].y < diff_best) {
				diff_best = du[0].y - d[0].y;
				best_up = up_split_bdrs.upper_bdrs[j];
				best_up_idx = j;
			// if begins at same point as previous best
			} else if(du[0].y - d[0].y == diff_best) {
				Vector2 *dub = data_array2_get_data(best_up);
				size_t num_dub = data_array2_size(best_up);

				// is farther at end?
				if(du[num_du-1].y - d[num_d-1].y > dub[num_dub-1].y - d[num_d-1].y) continue;

				// is closer at end?
				if(du[num_du-1].y - d[num_d-1].y < dub[num_dub-1].y - d[num_d-1].y) {
					best_up = up_split_bdrs.upper_bdrs[j];
					best_up_idx = j;
				// if begins and ends at same point as previous best, check middle
				} else {
					double x = (d[0].x + d[num_d-1].x)/2;
					double y = interpolate_from_sorted_data_array2(low_split_bdrs.lower_bdrs[0], x);
					double yu = interpolate_from_sorted_data_array2(up_split_bdrs.upper_bdrs[j], x);
					double yub = interpolate_from_sorted_data_array2(best_up, x);

					if(yu-y < yub-y) {
						best_up = up_split_bdrs.upper_bdrs[j];
						best_up_idx = j;
					}
				}
			}
		}

		// is there lower boundary that fits better
		Vector2 *du = data_array2_get_data(up_split_bdrs.upper_bdrs[best_up_idx]);
		size_t num_du = data_array2_size(up_split_bdrs.upper_bdrs[best_up_idx]);
		bool found_better_lower = false;
		for(int j = 1; j < low_split_bdrs.num; j++) {
			Vector2 *dl = data_array2_get_data(low_split_bdrs.lower_bdrs[j]);
			size_t num_dl = data_array2_size(low_split_bdrs.lower_bdrs[j]);
			if(du[0].x != dl[0].x) continue;
			if(du[0].y < dl[0].y) continue;
			if(du[0].y - dl[0].y > du[0].y - d[0].y) continue;
			if(du[0].y - dl[0].y < du[0].y - d[0].y){
				found_better_lower = true;
				break;
			}

			// is farther at end?
			if(du[num_du-1].y - dl[num_dl-1].y > du[num_du-1].y - d[num_d-1].y) continue;

			// is closer at end?
			if(du[num_du-1].y - dl[num_dl-1].y > du[num_du-1].y - d[num_d-1].y) {
				found_better_lower = true;
				break;
			}

			// if begins and ends at same point as previous best, check middle
			double x = (d[0].x + d[num_d-1].x)/2;
			double y = interpolate_from_sorted_data_array2(low_split_bdrs.lower_bdrs[0], x);
			double yu = interpolate_from_sorted_data_array2(up_split_bdrs.upper_bdrs[best_up_idx], x);
			double yl = interpolate_from_sorted_data_array2(low_split_bdrs.lower_bdrs[j], x);

			if(yu-yl < yu-y) {
				found_better_lower = true;
				break;
			}
		}

		if(!found_better_lower) {
			append_to_boundary(&new_boundary, up_split_bdrs.upper_bdrs[best_up_idx], low_split_bdrs.lower_bdrs[0]);

			memmove(&low_split_bdrs.lower_bdrs[0], &low_split_bdrs.lower_bdrs[1],
			(low_split_bdrs.num - 1) * sizeof(DataArray2*));
			memmove(&up_split_bdrs.upper_bdrs[best_up_idx], &up_split_bdrs.upper_bdrs[best_up_idx+1],
				(up_split_bdrs.num - (best_up_idx+1)) * sizeof(DataArray2*));
			low_split_bdrs.num--;
			up_split_bdrs.num--;
		} else {
			data_array2_free(low_split_bdrs.lower_bdrs[0]);
			memmove(&low_split_bdrs.lower_bdrs[0], &low_split_bdrs.lower_bdrs[1],
			(low_split_bdrs.num - 1) * sizeof(DataArray2*));
			low_split_bdrs.num--;
		}
	}

	free_boundary(&low_split_bdrs);
	free_boundary(&up_split_bdrs);

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
		Vector2 *l = data_array2_get_data(bdr->lower_bdrs[i]);
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

void plot_boundary(CoordinateSystem *coord_sys, Boundary bdr, CSAxisLabelType x_axis_type, CSAxisLabelType y_axis_type, bool clear_prev_data) {
	if(clear_prev_data) clear_coordinate_system(coord_sys);
	for(int j = 0; j < bdr.num; j++) {
		plot_data2(coord_sys, bdr.upper_bdrs[j], x_axis_type, y_axis_type, false);
		plot_data2(coord_sys, bdr.lower_bdrs[j], x_axis_type, y_axis_type, false);
	}
}

void scatter_boundary(CoordinateSystem *coord_sys, Boundary bdr, CSAxisLabelType x_axis_type, CSAxisLabelType y_axis_type, bool clear_prev_data) {
	if(clear_prev_data) clear_coordinate_system(coord_sys);
	for(int j = 0; j < bdr.num; j++) {
		scatter_data2(coord_sys, bdr.upper_bdrs[j], x_axis_type, y_axis_type, false);
		scatter_data2(coord_sys, bdr.lower_bdrs[j], x_axis_type, y_axis_type, false);
	}
}

void plot_scatter_boundary(CoordinateSystem *coord_sys, Boundary bdr, CSAxisLabelType x_axis_type, CSAxisLabelType y_axis_type, bool clear_prev_data) {
	if(clear_prev_data) clear_coordinate_system(coord_sys);
	for(int j = 0; j < bdr.num; j++) {
		plot_scatter_data2(coord_sys, bdr.upper_bdrs[j], x_axis_type, y_axis_type, false);
		plot_scatter_data2(coord_sys, bdr.lower_bdrs[j], x_axis_type, y_axis_type, false);
	}
}

