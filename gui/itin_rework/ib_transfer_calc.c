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
	return e > params->max_error;
}

bool has_quad_rel_error(Quad *quad, void *params_p) {
	ErrorFuncParams *params = params_p;
	double e = quad_rel_center_error(quad, params->val_idx);
	return e > (1.0+params->max_error) || e < (1.0-params->max_error);
}

void get_next_groups(SegmentGroup *group, Body *next_body, CelestSystem *system, double min_dep, double max_dep, double min_dur, double max_dur) {
	int shift = get_opp_conj_min_shift(group->arr_body, next_body, system, min_dep, max_dep, min_dur, max_dur);
	bool group_was_valid = true;

	while(group_was_valid) {
		SegmentGroup *new_group = new_segment_group(group->arr_body, next_body, system);
		set_opposition_conjunction_group_boundary2(new_group, shift, min_dep, max_dep, min_dur, max_dur);

		if(data_array2_get_max(new_group->group_bdr.upper_bdrs[0]).y >= min_dur &&
			data_array2_get_min(new_group->group_bdr.lower_bdrs[0]).y <= max_dur) {
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
		Vector2 min = get_boundary_min(next_group->dv_bdr);
		Vector2 max = get_boundary_max(next_group->dv_bdr);
		double quad_min_dep = min.x;
		double quad_max_dep = max.x;
		double quad_min_dur = min.y;
		double quad_max_dur = max.y;
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
		int max_rf_level_dep = (int) (log2((quad_max_dep-quad_min_dep)/0.001)) + 1;
		int max_rf_level_dur = (int) (log2((quad_max_dur-quad_min_dur)/0.001)) + 1;
		int max_rf_level = max_rf_level_dep > max_rf_level_dur ? max_rf_level_dep : max_rf_level_dur;
		next_group->max_rf_level = max_rf_level;

		MeshPoint2 *p00 = create_mesh_point(vec2(quad_min_dep, quad_max_dur), NULL, 0);
		MeshPoint2 *p01 = create_mesh_point(vec2(quad_max_dep, quad_max_dur), NULL, 0);
		MeshPoint2 *p10 = create_mesh_point(vec2(quad_min_dep, quad_min_dur), NULL, 0);
		MeshPoint2 *p11 = create_mesh_point(vec2(quad_max_dep, quad_min_dur), NULL, 0);

		next_group->quad = create_quad_from_four_points(NULL, p00, p01, p10, p11, NULL);

		split_to_refinement_level(next_group->quad, NULL, next_group->min_rf_level);
		printf("Num of Leaves: %d\n", get_quad_leaves(next_group->quad, NULL));
	}
}

void do_nextgroups_boundary_matching(SegmentGroup *group) {
	for(int i = 0; i < group->num_next_groups; i++) {
		SegmentGroup *next_group = group->next[i];
		match_quads_to_boundary(next_group->quad, &next_group->dv_bdr, &next_group->group_bdr, next_group->min_rf_level, next_group->max_rf_level+5, false);
	}
}

void populate_departure_quads(SegmentGroup *departure) {
	for(int idx = 0; idx < departure->num_next_groups; idx++) {
		SegmentGroup *group = departure->next[idx];
		QuadPointPopFunc point_pop_func = {departure_pop_func, group};
		populate_quad_mesh_points(group->quad, &point_pop_func);
	}
}

void departure_devide_and_conquer(SegmentGroup *departure, double tolerance) {
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
		quad_devide_and_conquer(group->quad, group->max_rf_level, &point_func, &error_func, &bounds_func);
	}
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
	departure_devide_and_conquer(departure, 100);
	end_time_measurement(&tm, "Departure Divide & Conquer");

	// --

	// Next groups find group boundaries wrt respective min_dur and max_dur

	// Determine Vinf line (fb-date vs min_dv)

	// Determine Vinf boundary of next groups

	// Combine prev boundary and vinf boundary

	// Refining prev group inside new boundary wrt vinf and angle

	// Quad generation and splitting of next groups

	// Boundary matching of next groups

	// Populate Quads with duration

	// D&C wrt duration

	// (Remove Quads with invalid duration)

	// Populate Quads with RPE

	// D&C wrt RPE

	// Determine RPE Boundary

	// Combine previous boundary with RPE Boundary

	// D&C Arr vinf

	// --

	// For Arrival: D&C vinf and angle

	print_timing_measurements(tm);
	free_timing_measurements(&tm);
	return departure;
}