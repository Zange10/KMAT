#include "ib_transfer_calc.h"

#include "tools/tool_funcs.h"
#include <math.h>

typedef struct ErrorFuncParams {
	double max_error;
	int val_idx;
} ErrorFuncParams;

typedef struct BoundaryFuncParams {
	Boundary soft_bdr, hard_bdr;
} BoundaryFuncParams;

bool is_quad_inside_soft_bdr(Quad *quad, void *params_p) {
	BoundaryFuncParams *params = params_p;
	return is_quad_inside_boundary(quad, params->soft_bdr);
}

bool has_quad_abs_error(Quad *quad, void *params_p) {
	ErrorFuncParams *params = params_p;
	double e = quad_abs_center_error(quad, params->val_idx);
	return !(e <= params->max_error);	// inverse should catch nan
}

bool has_quad_rel_error(Quad *quad, void *params_p) {
	ErrorFuncParams *params = params_p;
	double e = quad_rel_center_error(quad, params->val_idx);
	return !(e <= (1.0+params->max_error) && e >= (1.0-params->max_error));	// inverse should catch nan
}

bool has_quad_vinf_arr_error(Quad *quad, void *params_p) {
	ErrorFuncParams *params = params_p;

	double vinf = quad->center->val[MESH_VAL_ARRVINF];

	// vinf test
	double interp_vinf = get_quad_interpolated_value(quad, quad->center->pos, MESH_VAL_ARRVINF);
	double e_vinf = fabs(interp_vinf - vinf);
	if(e_vinf > params->max_error) return true;

	// vinf = |v_arr| test
	Vector3 interp_v_arr = vec3(
		get_quad_interpolated_value(quad, quad->center->pos, MESH_VAL_ARRX),
		get_quad_interpolated_value(quad, quad->center->pos, MESH_VAL_ARRY),
		get_quad_interpolated_value(quad, quad->center->pos, MESH_VAL_ARRZ)
		);
	Vector3 interp_v_body = vec3(
		get_quad_interpolated_value(quad, quad->center->pos, MESH_VAL_BODY_VX),
		get_quad_interpolated_value(quad, quad->center->pos, MESH_VAL_BODY_VY),
		get_quad_interpolated_value(quad, quad->center->pos, MESH_VAL_BODY_VZ)
		);
	Vector3 interp_vinf_varr = subtract_vec3(interp_v_arr, interp_v_body);
	double interp_vinf_from_varr = mag_vec3(interp_vinf_varr);
	double e_varr = fabs(interp_vinf_from_varr - vinf);
	if(e_varr > params->max_error) return true;

	// v_arr angle test
	Vector3 v_arr = vec3(
		quad->center->val[MESH_VAL_ARRX],
		quad->center->val[MESH_VAL_ARRY],
		quad->center->val[MESH_VAL_ARRZ]
		);
	Vector3 v_body = vec3(
		quad->center->val[MESH_VAL_BODY_VX],
		quad->center->val[MESH_VAL_BODY_VY],
		quad->center->val[MESH_VAL_BODY_VZ]
		);
	Vector3 vinf_varr = subtract_vec3(v_arr, v_body);
	double angle = angle_vec3_vec3(interp_vinf_varr, vinf_varr);
	double e_angle = rad2deg(angle);
	if(e_angle > params->max_error) return true;

	return false;
}

void get_next_groups(SegmentGroup *group, Body *next_body, CelestSystem *system, double min_dep, double max_dep, double min_dur, double max_dur) {
	int shift = get_opp_conj_min_shift(group->arr_body, next_body, system, min_dep, max_dep, min_dur, max_dur);
	bool group_was_valid = true;

	while(group_was_valid) {
		SegmentGroup *new_group = new_segment_group(group->arr_body, next_body, system);
		set_opposition_conjunction_group_boundary2(new_group, shift, min_dep, max_dep, min_dur, max_dur);

		if(data_array2_get_max(new_group->conj_opp_bdr.upper_bdrs[0]).y >= min_dur &&
			data_array2_get_min(new_group->conj_opp_bdr.lower_bdrs[0]).y <= max_dur) {
			append_to_segment_group(group, new_group);
			} else {
				free_segment_group(new_group);
				group_was_valid = false;
				break;
			}
		shift++;
	}
}

void set_nextgroups_depdv_boundary(SegmentGroup *departure, double min_dep, double max_dep, double min_dur, double max_dur, double dep_periapsis, double max_depdv) {
	for(int i = 0; i < departure->num_next_groups; i++) {
		SegmentGroup *group = departure->next[i];
		group->dv_bdr = calc_dv_boundary(group, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_depdv, 1);
		if(group->dv_bdr.num == 0) {
			free_segment_group(group);
			i--;
		} else {
			connect_boundary_ends(&group->dv_bdr);
		}
	}
}

void do_nextgroups_quad_generation(SegmentGroup *group) {
	for(int i = 0; i < group->num_next_groups; i++) {
		SegmentGroup *next_group = group->next[i];
		double quad_min_dep;
		double quad_max_dep;
		double quad_min_dur;
		double quad_max_dur;

		if(!group->prev) {
			Vector2 min = get_boundary_min(next_group->dv_bdr);
			Vector2 max = get_boundary_max(next_group->dv_bdr);
			quad_min_dep = min.x;
			quad_max_dep = max.x;
			quad_min_dur = min.y;
			quad_max_dur = max.y;
			double abs_grad = fabs(next_group->boundary_gradient);
			double ratio_dur = (quad_max_dep-quad_min_dep)*abs_grad / (quad_max_dur-quad_min_dur);
			double ratio_dep = 1.0/ratio_dur;

			int min_split = (int) log2(ratio_dur > ratio_dep ? ratio_dur : ratio_dep);
			next_group->min_rf_level = min_split+3;

			if(quad_max_dur-quad_min_dur < (quad_max_dep-quad_min_dep)*abs_grad) {
				quad_max_dur = (quad_max_dep-quad_min_dep)*abs_grad + quad_min_dur;
			} else {
				quad_max_dep = (quad_max_dur-quad_min_dur)/abs_grad + quad_min_dep;
			}
			int max_rf_level_dep = (int) (log2((quad_max_dep-quad_min_dep)/0.01)) + 1;
			int max_rf_level_dur = (int) (log2((quad_max_dur-quad_min_dur)/0.01)) + 1;
			int max_rf_level = max_rf_level_dep > max_rf_level_dur ? max_rf_level_dep : max_rf_level_dur;
			next_group->max_rf_level = max_rf_level;
		} else {
			quad_min_dep = next_group->prev->quad->corner[QUAD_NW]->pos.x;
			quad_max_dep = next_group->prev->quad->corner[QUAD_NE]->pos.x;
			quad_min_dur = next_group->prev->quad->corner[QUAD_SW]->pos.y;
			quad_max_dur = next_group->prev->quad->corner[QUAD_NW]->pos.y;
			next_group->min_rf_level = group->min_rf_level;
			next_group->max_rf_level = group->max_rf_level;
		}

		MeshPoint2 *p00 = create_mesh_point(vec2(quad_min_dep, quad_max_dur), NULL, 0);
		MeshPoint2 *p01 = create_mesh_point(vec2(quad_max_dep, quad_max_dur), NULL, 0);
		MeshPoint2 *p10 = create_mesh_point(vec2(quad_min_dep, quad_min_dur), NULL, 0);
		MeshPoint2 *p11 = create_mesh_point(vec2(quad_max_dep, quad_min_dur), NULL, 0);

		next_group->quad = create_quad_from_four_points(NULL, p00, p01, p10, p11, NULL);

		split_to_refinement_level(next_group->quad, NULL, next_group->min_rf_level);
	}
}

void do_nextgroups_boundary_matching(SegmentGroup *group) {
	for(int i = 0; i < group->num_next_groups; i++) {
		SegmentGroup *next_group = group->next[i];

		if(!group->prev) {
			match_quads_to_boundary(next_group->quad, &next_group->dv_bdr, &next_group->conj_opp_bdr, next_group->min_rf_level, next_group->max_rf_level+5, false);
		} else {
			match_quads_to_boundary(next_group->quad, &next_group->group_bdr, &next_group->group_bdr, next_group->min_rf_level,  group->min_rf_level+7, false);
		}
		printf("Num of Leaves: %d\n", get_quad_leaves(next_group->quad, NULL));
	}
}

void populate_departure_quads(SegmentGroup *departure) {
	for(int idx = 0; idx < departure->num_next_groups; idx++) {
		SegmentGroup *group = departure->next[idx];
		QuadPointPopFunc point_pop_func = {departure_pop_func, group};
		populate_quad_mesh_points(group->quad, &point_pop_func);
	}
}

void populate_fly_by_quads_dep(SegmentGroup *group) {
	for(int idx = 0; idx < group->num_next_groups; idx++) {
		SegmentGroup *next_group = group->next[idx];
		QuadPointPopFunc point_pop_func = {flyby_dur_pop_func, next_group};
		populate_quad_mesh_points(next_group->quad, &point_pop_func);

		QuadList *quad_list = create_quad_list();
		get_quads_with_nan(next_group->quad, quad_list, MESH_VAL_ARRDATE);
		for(int i = 0; i < quad_list->num; i++) {
			free_quad(quad_list->quad[i], true);
		}
		free_quad_list(quad_list);
	}
}

void populate_fly_by_quads_rpe_from_dep(SegmentGroup *group) {
	for(int idx = 0; idx < group->num_next_groups; idx++) {
		SegmentGroup *next_group = group->next[idx];
		QuadPointPopFunc point_pop_func = {flyby_rpe_pop_func, next_group};
		populate_quad_mesh_points(next_group->quad, &point_pop_func);
	}
}

void departure_divide_and_conquer(SegmentGroup *departure, double tolerance) {
	ErrorFuncParams err_func_params = {
		.max_error = tolerance/2,
		.val_idx = MESH_VAL_ARRVINF
	};
	QuadErrorFunc error_func = {has_quad_abs_error, &err_func_params};

	for(int idx = 0; idx < departure->num_next_groups; idx++) {
		SegmentGroup *group = departure->next[idx];
		QuadPointFunc point_func = {departure_point_func, group};
		BoundaryFuncParams bound_func_params = {.soft_bdr = group->dv_bdr};
		QuadBoundsFunc bounds_func = {is_quad_inside_soft_bdr, &bound_func_params};
		quad_divide_and_conquer(group->quad, group->max_rf_level, &point_func, &error_func, &bounds_func);
	}
}

void flyby_divide_and_conquer_vinf(SegmentGroup *group, double tolerance) {
	ErrorFuncParams err_func_params = {
		.max_error = tolerance/2,
		.val_idx = MESH_VAL_ARRVINF
	};
	QuadErrorFunc error_func = {has_quad_abs_error, &err_func_params};

	for(int idx = 0; idx < group->num_next_groups; idx++) {
		SegmentGroup *next_group = group->next[idx];
		QuadPointFunc point_func = {flyby_point_func, next_group};
		BoundaryFuncParams bound_func_params = {.soft_bdr = next_group->group_bdr};
		QuadBoundsFunc bounds_func = {is_quad_inside_soft_bdr, &bound_func_params};
		quad_divide_and_conquer(next_group->quad, next_group->max_rf_level, &point_func, &error_func, &bounds_func);
	}
}


void flyby_divide_and_conquer(SegmentGroup *group, double tolerance) {
	ErrorFuncParams err_func_params = {
		.max_error = tolerance/2,
		.val_idx = MESH_VAL_DUR
	};
	QuadErrorFunc error_func = {has_quad_abs_error, &err_func_params};

	for(int idx = 0; idx < group->num_next_groups; idx++) {
		SegmentGroup *next_group = group->next[idx];
		QuadPointFunc point_func = {flyby_dur_func, next_group};
		BoundaryFuncParams bound_func_params = {.soft_bdr = next_group->group_bdr};
		QuadBoundsFunc bounds_func = {is_quad_inside_soft_bdr, &bound_func_params};
		quad_divide_and_conquer(next_group->quad, next_group->max_rf_level, &point_func, &error_func, &bounds_func);
	}
}


void rpe_divide_and_conquer(SegmentGroup *group) {
	for(int idx = 0; idx < group->num_next_groups; idx++) {
		SegmentGroup *next_group = group->next[idx];

		double rel_acc = 100;
		double min_rel_acc = 1e-2;
		ErrorFuncParams err_func_params = {
			.max_error = rel_acc/2,
			.val_idx = MESH_VAL_RPE
		};
		BoundaryFuncParams bound_func_params = {.soft_bdr = next_group->group_bdr};
		QuadBoundsFunc bounds_func = {is_quad_inside_soft_bdr, &bound_func_params};
		QuadErrorFunc error_func = {has_quad_rel_error, &err_func_params};
		QuadPointFunc point_func = {flyby_point_func, next_group};
		int num_split_cycles = 0;

		QuadList *quad_list = create_quad_list();
		QuadList *quad_split_list = create_quad_list();

		get_quad_leaves(next_group->quad, quad_list);
		printf("########  Num of Leaves: %d\n", get_quad_leaves(next_group->quad, NULL));

		for(int i = 0; i < 100; i++) {
			num_split_cycles++;
			for(int j = 0; j < quad_list->num; j++) {
				update_quad_error_flag(quad_list->quad[j], &error_func);
				if(is_quad_flag(quad_list->quad[j], QUAD_FLAG_SPLIT))
					append_to_quad_list(quad_split_list, quad_list->quad[j]);
			}

			int num_splits = 0;
			clear_quad_list(quad_list);
			for(int j = 0; j < quad_split_list->num; j++) {
				num_splits += split_quad(quad_split_list->quad[j], &point_func, quad_list);
			}
			clear_quad_list(quad_split_list);

			printf("Number of Splits during cycle %d: %d\n", num_split_cycles, num_splits);

			for(int j = 0; j < quad_list->num; j++) {
				Quad *quad = quad_list->quad[j];
				if(!bounds_func.func(quad, bounds_func.params)) {
					remove_from_quad_list_at_idx(quad_list, j);
					free_quad(quad, true);
					j--;
				}
			}
			if(num_splits == 0) {
				get_quad_leaves(next_group->quad, quad_list);
				for(int j = 0; j < quad_list->num; j++) {
					Quad *quad = quad_list->quad[j];
					bool no_valid_sol = true;
					for(int k = 0; k < 4; k++) {
						if(quad->corner[k]->val[MESH_VAL_RPE]*(rel_acc+1) / next_group->dep_body->radius > 1) {
							no_valid_sol = false; break;
						}
					}
					if(no_valid_sol && quad->center->val[MESH_VAL_RPE]*(rel_acc+1) / next_group->dep_body->radius > 1) {
						no_valid_sol = false;
					}
					if(no_valid_sol) {
						remove_from_quad_list_at_idx(quad_list, j);
						free_quad(quad, true);
						j--;
					}
				}

				rel_acc /= 2;
				if(rel_acc == min_rel_acc) break;
				if(rel_acc < min_rel_acc) rel_acc = min_rel_acc;
				err_func_params.max_error = rel_acc/2;
			}
		}

		free_quad_list(quad_list);
		free_quad_list(quad_split_list);
		printf("Num Split Cycles: %d\n", num_split_cycles);
		printf("-------  Num of Leaves: %d\n", get_quad_leaves(next_group->quad, NULL));
	}
}

void determine_vinf_bdr_and_quad_refinement(SegmentGroup *group) {
	for(int i = 0; i < group->num_next_groups; i++) {
		SegmentGroup *next_group = group->next[i];
		calc_vinf_boundary(group, next_group, group->quad, next_group->vinf_struct_array.vinf_line, 1);


		ErrorFuncParams err_func_params = {
			.max_error = 1,
			.val_idx = MESH_VAL_ARRVINF
		};
		BoundaryFuncParams bound_func_params = {.soft_bdr = group->dv_bdr};
		QuadBoundsFunc bounds_func = {is_quad_inside_soft_bdr, &bound_func_params};
		QuadErrorFunc error_func = {has_quad_abs_error, &err_func_params};
		QuadPointFunc point_func = {departure_point_func, group};
		int num_split_cycles = 0;

		QuadList *quad_list = create_quad_list();
		QuadList *quad_split_list = create_quad_list();

		double dv_tol = 1;
		bool last_was_0 = false;

		for(int c = 0; c < 30; c++) {
			num_split_cycles++;
			err_func_params.max_error = dv_tol/2;

			if(c == 0 || last_was_0) {
				get_quad_leaves(group->quad, quad_list);
			}
			for(int j = 0; j < quad_list->num; j++) {
				if(!is_quad_crossed_by_boundary(quad_list->quad[j], next_group->vinf_bdr)) {
					remove_from_quad_list_at_idx(quad_list, j);
					j--;
				}
			}

			for(int j = 0; j < quad_list->num; j++) {
				update_quad_error_flag(quad_list->quad[j], &error_func);
				if(is_quad_flag(quad_list->quad[j], QUAD_FLAG_ACC_ERR) && quad_list->quad[j]->rf_level < group->max_rf_level) {
					append_to_quad_list(quad_split_list, quad_list->quad[j]);
					set_quad_flag(quad_list->quad[j], QUAD_FLAG_SPLIT);
				}
			}

			int num_splits = 0;
			clear_quad_list(quad_list);
			for(int j = 0; j < quad_split_list->num; j++) {
				num_splits += split_quad(quad_split_list->quad[j], &point_func, quad_list);
			}
			clear_quad_list(quad_split_list);

			printf("Number of Splits during cycle %d: %d\n", num_split_cycles, num_splits);

			for(int j = 0; j < quad_list->num; j++) {
				Quad *quad_ = quad_list->quad[j];
				if(!bounds_func.func(quad_, bounds_func.params)) {
					remove_from_quad_list_at_idx(quad_list, j);
					free_quad(quad_, true);
					j--;
				}
			}

			if(num_splits == 0) {
				if(last_was_0) break;
				last_was_0 = true;
				free_boundary(&next_group->vinf_bdr);
				next_group->vinf_bdr = create_new_boundary();
				calc_vinf_boundary(group, next_group, group->quad, next_group->vinf_struct_array.vinf_line, 1);
			} else last_was_0 = false;
		}

		free_quad_list(quad_list);
		free_quad_list(quad_split_list);
		if(group->next[i]->vinf_bdr.num == 0) {
			free_segment_group(group->next[i]);
			i--;
		}
	}
}

void refine_group_wrt_next_group_bdrs(SegmentGroup *group) {
	ErrorFuncParams err_func_params = {
		.max_error = 1.0/2
	};
	QuadErrorFunc error_func = {has_quad_vinf_arr_error, &err_func_params};
	QuadPointFunc point_func = {group->prev->prev ? &flyby_point_func : &departure_point_func, group};

	int num_split_cycles = 0;

	QuadList *quad_list = create_quad_list();
	QuadList *quad_split_list = create_quad_list();

	get_quad_leaves(group->quad, quad_list);

	for(int i = 0; i < 100; i++) {
		num_split_cycles++;

		for(int j = 0; j < quad_list->num; j++) {
			bool is_inside_bdr = false;
			for(int group_idx = 0; group_idx < group->num_next_groups; group_idx++) {
				if(is_quad_inside_boundary(quad_list->quad[j], group->next[group_idx]->group_bdr)) {
					is_inside_bdr = true;
					break;
				}
			}
			if(!is_inside_bdr) {
				remove_from_quad_list_at_idx(quad_list, j); j--;
			}
		}

		for(int j = 0; j < quad_list->num; j++) {
			update_quad_error_flag(quad_list->quad[j], &error_func);
			if(is_quad_flag(quad_list->quad[j], QUAD_FLAG_ACC_ERR) && quad_list->quad[j]->rf_level < group->max_rf_level) {
				append_to_quad_list(quad_split_list, quad_list->quad[j]);
				set_quad_flag(quad_list->quad[j], QUAD_FLAG_SPLIT);
			}
		}

		int num_splits = 0;
		clear_quad_list(quad_list);
		for(int j = 0; j < quad_split_list->num; j++) {
			num_splits += split_quad(quad_split_list->quad[j], &point_func, quad_list);
		}
		clear_quad_list(quad_split_list);

		printf("Number of Splits during cycle %d: %d\n", num_split_cycles, num_splits);
		if(num_splits == 0) break;
	}

	free_quad_list(quad_list);
	free_quad_list(quad_split_list);
	printf("Num Split Cycles: %d\n", num_split_cycles);
	printf("Num of Leaves: %d\n", get_quad_leaves(group->quad, NULL));
}

void calc_interpolation_based_next_step(SegmentGroup *group, Body **body_sequence, int num_rem_bodies, CelestSystem *system) {
	// Next groups find group boundaries wrt respective min_dur and max_dur
	Vector3 quad_min = get_quad_min_values(group->quad, MESH_VAL_ARRDATE);
	Vector3 quad_max = get_quad_max_values(group->quad, MESH_VAL_ARRDATE);
	double min_dep = quad_min.z;
	double max_dep = quad_max.z;
	double min_dur = 90;
	double max_dur = 700;
	get_next_groups(group, body_sequence[0], system, min_dep, max_dep, min_dur, max_dur);

	// Determine Vinf line (fb-date vs min_dv)
	for(int i = 0; i < group->num_next_groups; i++) {
		group->next[i]->vinf_struct_array = calc_min_vinf_line2(group->next[i], min_dep, max_dep, min_dur, max_dur, 1);
	}

	// Determine Vinf boundary of next groups
	determine_vinf_bdr_and_quad_refinement(group);

	// Combine prev boundary and vinf boundary
	for(int i = 0; i < group->num_next_groups; i++) {
		Boundary new_boundary = combine_boundaries(group->dv_bdr, group->next[i]->vinf_bdr);
		if(new_boundary.num == 0) {
			free_boundary(&new_boundary);
			free_segment_group(group->next[i]);
			i--; continue;
		}
		free_boundary(&group->next[i]->group_bdr);
		group->next[i]->group_bdr = new_boundary;
	}

	// Refining prev group inside new boundary wrt vinf and angle
	refine_group_wrt_next_group_bdrs(group);

	// Quad generation and splitting of next groups
	do_nextgroups_quad_generation(group);

	// Boundary matching of next groups
	do_nextgroups_boundary_matching(group);

	// Populate Quads with duration
	populate_fly_by_quads_dep(group);

	// D&C wrt duration
	flyby_divide_and_conquer(group, 1);

	// Remove Quads with invalid duration
	for(int i = 0; i < group->num_next_groups; i++) {
		QuadList *quad_list = create_quad_list();
		get_quads_with_nan(group->next[i]->quad, quad_list, MESH_VAL_ARRDATE);
		for(int j = 0; j < quad_list->num; j++) {
			free_quad(quad_list->quad[j], true);
		}
		free_quad_list(quad_list);
	}

	// Remove Groups with no quads
	for(int i = 0; i < group->num_next_groups; i++) {
		if(get_quad_leaves(group->next[i]->quad, NULL) == 0) {
			free_segment_group(group->next[i]); i--;
		}
	}

	// Populate Quads with RPE
	populate_fly_by_quads_rpe_from_dep(group);

	// D&C wrt RPE
	rpe_divide_and_conquer(group);

	// Remove Groups with no quads
	for(int i = 0; i < group->num_next_groups; i++) {
		if(get_quad_leaves(group->next[i]->quad, NULL) == 0) {
			free_segment_group(group->next[i]); i--;
		}
	}

	// Determine RPE Boundary
	for(int i = 0; i < group->num_next_groups; i++) {
		group->next[i]->rpe_bdr = get_rpe_boundary(group->next[i]);
	}

	// Combine previous boundary with RPE Boundary
	for(int i = 0; i < group->num_next_groups; i++) {
		Boundary new_group_bdr = combine_boundaries(group->next[i]->rpe_bdr, group->next[i]->group_bdr);
		free_boundary(&group->next[i]->group_bdr);
		group->next[i]->group_bdr = new_group_bdr;
	}

	// D&C Arr vinf
	flyby_divide_and_conquer_vinf(group, 100.0);

	// --

	// For Arrival: D&C vinf and angle
}

SegmentGroup * calc_interpolation_based_itins(Body **body_sequence, int num_bodies, CelestSystem *system, double min_dep, double max_dep, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double arr_periapsis, double max_arrdv) {
	for(int i = 0; i < num_bodies; i++) {
		if(i != 0) printf(" -> ");
		printf("%s", body_sequence[i]->name);
	}
	printf("\n");

	TimingMeasurements tm = init_timing_measurements();

	// define Departure group
	start_time_measurement(&tm);
	SegmentGroup *departure = new_segment_group(body_sequence[0], body_sequence[0], system);
	end_time_measurement(&tm, "Define Departure");

	// --

	// Departure groups find group boundaries wrt min_dur and max_dur
	start_time_measurement(&tm);
	get_next_groups(departure, body_sequence[1], system, min_dep, max_dep, min_dur, max_dur);
	end_time_measurement(&tm, "Determine Departure Groups");

	// Determine dv boundary for dep groups
	start_time_measurement(&tm);
	set_nextgroups_depdv_boundary(departure, min_dep, max_dep, min_dur, max_dur, dep_periapsis, max_depdv);
	end_time_measurement(&tm, "Departure Groups DV-Boundary");

	// Quad generation and splitting wrt boundary
	start_time_measurement(&tm);
	do_nextgroups_quad_generation(departure);
	end_time_measurement(&tm, "Departure Groups Quad Generation");

	// Departure group boundary matching
	start_time_measurement(&tm);
	do_nextgroups_boundary_matching(departure);
	end_time_measurement(&tm, "Group Boundary Matching");

	// Populate Quads
	start_time_measurement(&tm);
	populate_departure_quads(departure);
	end_time_measurement(&tm, "Populate Departure Quads");

	// D&C wrt Arrival vinf
	start_time_measurement(&tm);
	departure_divide_and_conquer(departure, 100.0);
	end_time_measurement(&tm, "Departure Divide & Conquer");

	// --

	for(int i = 0; i < departure->num_next_groups; i++) {
		printf("GROUP %d -----------------\n", i);
		start_time_measurement(&tm);
		calc_interpolation_based_next_step(departure->next[i], &body_sequence[2], num_bodies-2, system);
		char str[32];
		sprintf(str, "Group %d", i);
		end_time_measurement(&tm, str);
	}

	print_timing_measurements(tm);
	free_timing_measurements(&tm);
	return departure;
}