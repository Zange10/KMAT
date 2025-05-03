#include "mesh_creator.h"
#include "orbit_calculator/transfer_calc.h"
#include "orbit_calculator/transfer_tools.h"
#include <math.h>


double vector_distance(struct Vector2D v0, struct Vector2D v1) {
	struct Vector2D vdiff = {v1.x - v0.x, v1.y-v0.y};
	return vector2d_mag(vdiff);
}

int is_triangle_edge(PcMeshTriangle triangle) {
	return (triangle.point_flags >> TRI_FLAG_01_IS_EDGE & 1) || (triangle.point_flags >> TRI_FLAG_12_IS_EDGE & 1) || (triangle.point_flags >> TRI_FLAG_20_IS_EDGE & 1);
}

int is_triangle_big(PcMeshTriangle triangle) {
	return (triangle.point_flags >> TRI_FLAG_01_IS_LONG & 1) || (triangle.point_flags >> TRI_FLAG_12_IS_LONG & 1) || (triangle.point_flags >> TRI_FLAG_20_IS_LONG & 1);
}

void add_edge_flag_to_triangle(PcMeshTriangle *triangle, PcMeshPoint *p0, PcMeshPoint *p1) {
	int idx0, idx1;
	for(int i = 0; i < 3; i++) if(triangle->points[i] == p0) {idx0 = i; break;}
	for(int i = 0; i < 3; i++) if(triangle->points[i] == p1) {idx1 = i; break;}

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

int get_edge_flag_of_triangle(PcMeshTriangle *triangle, PcMeshPoint *p0, PcMeshPoint *p1) {
	int idx0, idx1;
	for(int i = 0; i < 3; i++) if(triangle->points[i] == p0) {idx0 = i; break;}
	for(int i = 0; i < 3; i++) if(triangle->points[i] == p1) {idx1 = i; break;}

	if(idx0 > idx1) {
		int temp = idx0;
		idx0 = idx1;
		idx1 = temp;
	}

	if(idx0 == 0) {
		if(idx1 == 1) {
			return triangle->point_flags >> TRI_FLAG_01_IS_EDGE & 1;
		} else {
			return triangle->point_flags >> TRI_FLAG_20_IS_EDGE & 1;
		}
	} else {
		return triangle->point_flags >> TRI_FLAG_12_IS_EDGE & 1;
	}
}

void remove_edge_flag_from_triangle(PcMeshTriangle *triangle, PcMeshPoint *p0, PcMeshPoint *p1) {
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

void update_point_edges(PcMeshPoint *p0, PcMeshPoint *p1, int add_flag) {
	PcMeshTriangle *temp_triangle = NULL;
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

void remove_all_triangle_edge_flags_where_adjacents(PcMeshTriangle *triangle) {
	if((triangle->point_flags >> TRI_FLAG_01_IS_EDGE & 1)) update_point_edges(triangle->points[0], triangle->points[1], 0);
	if((triangle->point_flags >> TRI_FLAG_12_IS_EDGE & 1)) update_point_edges(triangle->points[1], triangle->points[2], 0);
	if((triangle->point_flags >> TRI_FLAG_20_IS_EDGE & 1)) update_point_edges(triangle->points[2], triangle->points[0], 0);
}

PcMeshTriangle * create_triangle_from_three_points(PcMeshPoint *p0, PcMeshPoint *p1, PcMeshPoint *p2, struct Vector2D big_threshold) {
	PcMeshTriangle *triangle = malloc(sizeof(PcMeshTriangle));
	triangle->points[0] = p0;
	triangle->points[1] = p1;
	triangle->points[2] = p2;
	triangle->point_flags = 0;

	p0->triangles[p0->num_triangles] = triangle; p0->num_triangles++;
	p1->triangles[p1->num_triangles] = triangle; p1->num_triangles++;
	p2->triangles[p2->num_triangles] = triangle; p2->num_triangles++;

	struct Vector diff_01 = subtract_vectors(p0->data, p1->data);
	struct Vector diff_12 = subtract_vectors(p1->data, p2->data);
	struct Vector diff_20 = subtract_vectors(p2->data, p0->data);

	if(p0->is_edge && p1->is_edge) triangle->point_flags |= (1 << TRI_FLAG_01_IS_EDGE);
	if(p1->is_edge && p2->is_edge) triangle->point_flags |= (1 << TRI_FLAG_12_IS_EDGE);
	if(p0->is_edge && p2->is_edge) triangle->point_flags |= (1 << TRI_FLAG_20_IS_EDGE);

	if(is_triangle_edge(*triangle)) remove_all_triangle_edge_flags_where_adjacents(triangle);

	if(fabs(diff_01.x) > big_threshold.x || fabs(diff_01.y) > big_threshold.y) triangle->point_flags |= (1 << TRI_FLAG_01_IS_LONG);
	if(fabs(diff_12.x) > big_threshold.x || fabs(diff_12.y) > big_threshold.y) triangle->point_flags |= (1 << TRI_FLAG_12_IS_LONG);
	if(fabs(diff_20.x) > big_threshold.x || fabs(diff_20.y) > big_threshold.y) triangle->point_flags |= (1 << TRI_FLAG_20_IS_LONG);

	return triangle;
}

void remove_triangle_from_pcmesh(PcMesh *mesh, int tri_idx) {
	for(int i = 0; i < 3; i++) update_point_edges(mesh->triangles[tri_idx]->points[i], mesh->triangles[tri_idx]->points[(i+1)%3], 1);
	for(int i = 0; i < 3; i++) {
		update_point_edges(mesh->triangles[tri_idx]->points[i], mesh->triangles[tri_idx]->points[(i+1)%3], 1);
		PcMeshPoint *point = mesh->triangles[tri_idx]->points[i];
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


struct ItinStep * calc_itinerary(double dep, double init_dur, struct Dv_Filter dv_filter, struct ItinStep *template) {
	template = get_first(template);
	struct Body *body = template->body;
	struct System *system = body->orbit.body->system;
	enum Transfer_Type tt = dv_filter.last_transfer_type == TF_CIRC ? circcirc : dv_filter.last_transfer_type == TF_CAPTURE ? circcap : circfb;

	struct OSV osv_body = system->calc_method == ORB_ELEMENTS ?
						  osv_from_elements(body->orbit, dep, system) :
						  osv_from_ephem(body->ephem, dep, system->cb);


	struct ItinStep *step;
	step = (struct ItinStep*) malloc(sizeof(struct ItinStep));
	step->body = template->body;
	step->date = dep;
	step->r = osv_body.r;
	step->v_dep = vec(0,0,0);
	step->v_arr = vec(0,0,0);
	step->v_body = osv_body.v;
	step->num_next_nodes = 0;
	step->prev = NULL;
	step->next = NULL;

	template = template->next[0];
	body = template->body;

	osv_body = system->calc_method == ORB_ELEMENTS ?
			   osv_from_elements(body->orbit, dep+init_dur, system) :
			   osv_from_ephem(body->ephem, dep+init_dur, system->cb);

	double data[3];
	struct Transfer tf = calc_transfer(tt, step->body, body, step->r, step->v_body,
									   osv_body.r, osv_body.v, init_dur * 86400, system->cb, data);

	if(data[1] > dv_filter.max_totdv || data[1] > dv_filter.max_depdv) {free_itinerary(step); return NULL;}
	if(template->next == NULL && (data[1] + data[2] > dv_filter.max_totdv || data[2] > dv_filter.max_satdv)) {free_itinerary(step); return NULL;}

	step->next = (struct ItinStep **) malloc(sizeof(struct ItinStep*));
	step->next[0] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
	step->next[0]->prev = step;
	step->next[0]->next = NULL;
	step = step->next[0];
	step->body = body;
	step->date = dep+init_dur;
	step->r = osv_body.r;
	step->v_dep = tf.v0;
	step->v_arr = tf.v1;
	step->v_body = osv_body.v;
	step->num_next_nodes = 0;

	while(template->next != NULL) {
		template = template->next[0];
		if(template->num_next_nodes > 1) printf("template: %d\n", template->prev->num_next_nodes);
		double dt = template->date-template->prev->date;
		find_viable_flybys(step, system, template->body, dt*0.01*86400, dt*2.17*86400);

		if(step->num_next_nodes == 0) {free_itinerary(step); return NULL;}
//		if(step->num_next_nodes > 1) printf("together: %d %d\n", template->prev->num_next_nodes, step->num_next_nodes);
		step = step->next[0];
	}

	return step;
}

//PcMeshGrid fine_mesh_grid_top_bottom(PcMeshGrid grid, double **init_durs, struct Dv_Filter dv_filter, struct PorkchopPoint *porkchop_points) {
//	if(grid.points == NULL) return grid;
//
//	int idx = 0;
//
//	for(int i = 0; i < grid.num_columns; i++) {
//		int num_col_values = grid.num_points[i]+10000;
//		struct Vector *col = malloc(sizeof(struct Vector) * num_col_values);
//		double dep = grid.points[i][0].x;
//		double init_dur = init_durs[i][0];
//		double step = grid.num_points[i] > 1 ? init_durs[i][0]-init_durs[i][1] : -10;
//		int col_idx = 0;
//		struct ItinStep *ptr = porkchop_points[idx].arrival;
//		while(fabs(step) > 0.001) {
//			init_dur += step;
////			printf("%f  %f\n", init_dur, porkchop_points[idx].arrival->prev->prev->date-porkchop_points[idx].arrival->prev->prev->prev->date);
//			struct ItinStep *ptr_temp = calc_itinerary(dep, init_dur, dv_filter, ptr);
//			if(ptr_temp != NULL) {
//				if(ptr_temp->date-get_first(ptr_temp)->date > grid.points[i][0].y) {
//					free_itinerary(get_first(ptr_temp));
//					init_dur -= step;
//					step /= -2;
//					continue;
//				}
//				ptr = ptr_temp;
//				col[col_idx].x = dep;
//				col[col_idx].y = get_first(ptr)->next[0]->date-dep;
//				double vinf = vector_mag(subtract_vectors(get_first(ptr)->next[0]->v_dep, get_first(ptr)->v_body));
//				col[col_idx].z = dv_circ(get_first(ptr)->body, get_first(ptr)->body->atmo_alt+50e3, vinf);
//				col[col_idx].z = 0;
//				col_idx++;
//			} else {
//				init_dur -= step;
//				step /= 2;
//			}
//		}
//
//		for(int j = 0; j < grid.num_points[i]; j++) {
//			col[col_idx].x = grid.points[i][j].x;
//			col[col_idx].y = grid.points[i][j].y;
//			col[col_idx].z = grid.points[i][j].z;
////			if(j < grid.num_points[i] - 10 && j > 10) j += grid.num_points[i]/50;
//			col_idx++;
//		}
//
//		init_dur = init_durs[i][grid.num_points[i]-1];
//		step = grid.num_points[i] > 1 ? init_durs[i][grid.num_points[i]-1]-init_durs[i][grid.num_points[i]-2] : 10;
//		ptr = porkchop_points[idx+grid.num_points[i]-1].arrival;
//		while(fabs(step) > 0.001) {
//			init_dur += step;
//			struct ItinStep *ptr_temp = calc_itinerary(dep, init_dur, dv_filter, ptr);
//			if(ptr_temp != NULL) {
//				if(ptr_temp->date-get_first(ptr_temp)->date < grid.points[i][grid.num_points[i]-1].y) {
//					free_itinerary(get_first(ptr_temp));
//					init_dur -= step;
//					step /= -2;
//					continue;
//				}
//				ptr = ptr_temp;
//				col[col_idx].x = dep;
//				col[col_idx].y = get_first(ptr)->next[0]->date-dep;
//				double vinf = vector_mag(subtract_vectors(get_first(ptr)->next[0]->v_dep, get_first(ptr)->v_body));
//				col[col_idx].z = dv_circ(get_first(ptr)->body, get_first(ptr)->body->atmo_alt+50e3, vinf);
//				col[col_idx].z = 0;
//				col_idx++;
//			} else {
//				init_dur -= step;
//				step /= 2;
//			}
//		}
//
//
//		idx += grid.num_points[i];
//		free(grid.points[i]);
//		grid.points[i] = col;
//		grid.num_points[i] = col_idx;
//	}
//
//	return grid;
//}
//
//PcMesh mesh_from_porkchop(struct PorkchopPoint *porkchop_points, int num_itins, int num_deps, int *num_itins_per_dep) {
//	double *dep = malloc(num_deps * sizeof(double));
//	double **dur = malloc(num_deps * sizeof(double*));
//	double **init_dur = malloc(num_deps * sizeof(double*));
//	double **d_v = malloc(num_deps * sizeof(double*));
//	int idx = 0;
//	for(int i = 0; i < num_deps; i++) {
//		dep[i] = porkchop_points[idx].dep_date;
//		dur[i] = malloc(num_itins_per_dep[i] * sizeof(double*));
//		init_dur[i] = malloc(num_itins_per_dep[i] * sizeof(double*));
//		d_v[i] = malloc(num_itins_per_dep[i] * sizeof(double*));
//		for(int j = 0; j < num_itins_per_dep[i]; j++) {
//			struct ItinStep *step = porkchop_points[idx].arrival;
//			while(step->prev->prev != NULL) step = step->prev;
//			dur[i][j] = porkchop_points[idx].dur;
//			init_dur[i][j] = step->date-step->prev->date;
//			d_v[i][j] = porkchop_points[idx].dv_dep;
//			idx++;
//		}
//	}
//
//	PcMeshGrid grid = create_mesh_grid(dep, init_dur, d_v, num_deps, num_itins_per_dep);
//
//
//
////	struct Dv_Filter dv_filter = {1e9, 8000, 1e9, TF_FLYBY};
////	grid = fine_mesh_grid_top_bottom(grid, init_dur, dv_filter, porkchop_points);
//
//
//
//	struct Vector max = grid.points[0][0];
//	struct Vector min = grid.points[0][0];
//
//	for(int i = 0; i < grid.num_columns; i++) {
//		for(int j = 0; j < grid.num_points[i]; j++) {
//			if(grid.points[i][j].x > max.x) max.x = grid.points[i][j].x;
//			if(grid.points[i][j].x < min.x) min.x = grid.points[i][j].x;
//			if(grid.points[i][j].y > max.y) max.y = grid.points[i][j].y;
//			if(grid.points[i][j].y < min.y) min.y = grid.points[i][j].y;
//			if(grid.points[i][j].z > max.z) max.z = grid.points[i][j].z;
//			if(grid.points[i][j].z < min.z) min.z = grid.points[i][j].z;
//		}
//	}
//
//	max.x += 0.1*(max.x-min.x);
//	min.x -= 0.1*(max.x-min.x);
//	max.y += 0.1*(max.y-min.y);
//	min.y -= 0.1*(max.y-min.y);
//
//	struct Vector gradient = {
//			2000/(max.x-min.x),
//			2000/(max.y-min.y),
//			1/(max.z-min.z),
//	};
//
//
//	for(int i = 0; i < grid.num_columns; i++) {
//		for(int j = 0; j < grid.num_points[i]; j++) {
//			grid.points[i][j].x -= min.x;
//			grid.points[i][j].x *= gradient.x;
//			grid.points[i][j].y -= min.y;
//			grid.points[i][j].y *= gradient.y;
//			grid.points[i][j].z -= min.z;
//			grid.points[i][j].z *= gradient.z;
//		}
//	}
//
//	return create_mesh_from_grid(grid, gradient.x*1.01);
//}
//
//PcMeshGrid create_mesh_grid(double *x_vals, double **y_vals, double **z_vals, int num_cols, int *num_points) {
//
//	for(int i = 0; i < num_cols; i++) {
//		grid.num_col_rows[i] = num_points[i];
//		grid.points[i] = malloc(sizeof(struct Vector) * num_points[i]);
//		for(int j = 0; j < num_points[i]; j++) {
//			PcMeshPoint *new_point = malloc(sizeof(PcMeshPoint));
//			*new_point = (PcMeshPoint) {
//					.data = (struct Vector) {.x = x_vals[i], .y = y_vals[i][j], .z = z_vals[i][j]},
//					.porkchop_point =
//			};
//			grid.points[i][j] = (struct Vector) {.x = x_vals[i], .y = y_vals[i][j], .z = z_vals[i][j]};
//			int idx = j;
//			while(idx > 0 && grid.points[i][idx].y < grid.points[i][idx-1].y) {
//				struct Vector temp = grid.points[i][idx];
//				grid.points[i][idx] = grid.points[i][idx-1];
//				grid.points[i][idx-1] = temp;
//				idx--;
//			}
//		}
//	}
//
//	return grid;
//};

struct Vector2D get_from_point_dir_for_fine_mesh(PcMeshPoint *prev_point, PcMeshPoint *curr_point, PcMeshPoint *next_point) {
	struct Vector2D v0 = norm_vector2d(vec2D(curr_point->data.x - prev_point->data.x, curr_point->data.y - prev_point->data.y));
	struct Vector2D v1 = norm_vector2d(vec2D(curr_point->data.x - next_point->data.x, curr_point->data.y - next_point->data.y));

	struct Vector2D v = add_vectors2d(v0, v1);
	if(vector2d_mag(v) != 0) {
		v = norm_vector2d(v);
		if(curr_point->num_triangles == 1) return v;
	} else {
		v = vec2D(v0.y, -v0.x);
	}

	PcMeshPoint *rand_point;
	for(int i = 0; i < 3; i++) {
		rand_point = curr_point->triangles[0]->points[i];
		if(rand_point != curr_point && rand_point != prev_point && rand_point != next_point) break;
	}
	struct Vector2D v2 = vec2D(curr_point->data.x - rand_point->data.x, curr_point->data.y - rand_point->data.y);

	if(angle_vec_vec_2d(v, v0) > angle_vec_vec_2d(v, v2)) {
		return v;
	} else {
		return vec2D(-v.x, -v.y);
	};
}
struct Vector2D get_from_line_perpendicular_dir_for_fine_mesh(PcMeshPoint *p0, PcMeshPoint *p1, PcMeshTriangle *triangle) {
	PcMeshPoint *opp_point;
	for(int i = 0; i < 3; i++) {
		opp_point = triangle->points[i];
		if(opp_point != p0 && opp_point != p1) break;
	}

	struct Vector2D v_diff = norm_vector2d(vec2D(p0->data.x - p1->data.x, p0->data.y - p1->data.y));
	struct Vector2D v_diff_opp = norm_vector2d(vec2D(opp_point->data.x - (p0->data.x + p1->data.x)/2, opp_point->data.y - (p0->data.y + p1->data.y)/2));

	struct Vector2D v = vec2D(v_diff.y, -v_diff.x);

	if(angle_vec_vec_2d(v, v_diff_opp) > M_PI/2) {
		return v;
	} else {
		return vec2D(-v.x, -v.y);
	};
}

void get_prev_edge_point(PcMeshPoint *curr_point, PcMeshPoint **prev_point) {
	PcMeshPoint *point;
	for(int i = 0; i < curr_point->num_triangles; i++) {
		for(int j = 0; j < 3; j++) {
			point = curr_point->triangles[i]->points[j];
			if(point->is_edge && point != curr_point) {*prev_point = point; return;}
		}
	}
}

void get_next_edge_point(PcMeshPoint *prev_point, PcMeshPoint *curr_point, PcMeshPoint **next_point) {
	PcMeshPoint *point;
	for(int i = 0; i < curr_point->num_triangles; i++) {
		for(int j = 0; j < 3; j++) {
			point = curr_point->triangles[i]->points[j];
			if(point->is_edge && point != curr_point && point != prev_point) {
				if(!curr_point->triangles[i]->points[0]->is_edge || !curr_point->triangles[i]->points[1] || !curr_point->triangles[i]->points[2]) {
					*next_point = point;
					return;
				} else {
					if(get_edge_flag_of_triangle(curr_point->triangles[i], point, curr_point)) {
						*next_point = point;
						return;
					}
				}
			}
		}
	}
}

PcMeshTriangle * get_edge_triangle(PcMeshPoint *curr_point, PcMeshPoint *next_point) {
	for(int i = 0; i < curr_point->num_triangles; i++) {
		for(int j = 0; j < 3; j++) {
			if(curr_point->triangles[i]->points[j] == next_point) {return curr_point->triangles[i];}
		}
	}
}

struct ItinStep * find_porkchop_edge(struct Vector2D base, struct Vector2D dir, double min_dist, double max_dist, double min_step, struct ItinStep *reference_itin, struct Dv_Filter dv_filter) {
	double dist = -1;
	double step = 0.2;
	struct ItinStep *ptr = reference_itin;
	while(fabs(step) > min_step) {
		double dep = base.x + dist*dir.x;
		double init_dur = base.y + dist*dir.y;

		struct ItinStep *ptr_temp = calc_itinerary(dep, init_dur, dv_filter, ptr);
		if(ptr_temp != NULL) {
			ptr = ptr_temp;
		} else {
			dist -= step;
			step /= 4;
		}
		dist += step;
	}

	if(ptr == reference_itin) return NULL;

	return get_first(ptr);
}

void fine_mesh_around_edge(PcMesh *mesh, double max_dist, double min_dist, struct Dv_Filter dv_filter) {
	PcMeshPoint ***new_points;
	PcMeshPoint *prev_point, *curr_point, *next_point, *initial_point;
	PcMeshPoint **points_to_fine_mesh = malloc(mesh->num_points * sizeof(PcMeshPoint));
	int num_points_to_fine_mesh = 0;
	PcMeshTriangle *curr_triangle;

	for(int i = 0; i < mesh->num_points; i++) {
		if(mesh->points[i]->is_edge) {
			points_to_fine_mesh[num_points_to_fine_mesh] = mesh->points[i];
			num_points_to_fine_mesh++;
		}
	}

	printf("%d\n", num_points_to_fine_mesh);
//	return;

	while(num_points_to_fine_mesh > 0) {
		initial_point = points_to_fine_mesh[0];
		curr_point = initial_point;

		get_prev_edge_point(curr_point, &prev_point);


		PcMeshPoint *new_point = NULL;
		PcMeshPoint *last_point = NULL;
		struct ItinStep *step = NULL;
		struct Vector2D dir;

		do {
			for(int i = 0; i < num_points_to_fine_mesh; i++) {
				if(points_to_fine_mesh[i] == curr_point) {
					num_points_to_fine_mesh--;
					points_to_fine_mesh[i] = points_to_fine_mesh[num_points_to_fine_mesh];
				}
			}

			get_next_edge_point(prev_point, curr_point, &next_point);



			dir = get_from_line_perpendicular_dir_for_fine_mesh(curr_point, next_point, get_edge_triangle(curr_point, next_point));
			step = find_porkchop_edge(vec2D(curr_point->data.x, curr_point->data.y), dir, 0, max_dist, 0.001, curr_point->porkchop_point.arrival, dv_filter);
			if(step != NULL) {
				step = get_first(step);
				double vinf = vector_mag(subtract_vectors(get_first(step)->next[0]->v_dep, get_first(step)->v_body));
				new_point = malloc(sizeof(PcMeshPoint));
				*new_point = (PcMeshPoint) {
						.data = (struct Vector) {.x = step->date, .y = step->next[0]->date-step->date, dv_circ(get_first(step)->body, get_first(step)->body->atmo_alt+50e3, vinf)},
						.porkchop_point = create_porkchop_point(get_last(step)),
						.is_edge = 1,
						.is_artificial = 1,
						.triangles = malloc(10 * sizeof(PcMeshTriangle*)),
						.num_triangles = 0,
						.max_num_triangles = 10
				};

//				curr_point->is_edge = 0;

				mesh->points[mesh->num_points] = new_point;
				mesh->num_points++;
				new_point->porkchop_point = create_porkchop_point(get_last(step));
//				printf("Found: %f %f %f\n", new_point->data.x, new_point->data.y, new_point->data.z);

				mesh->triangles[mesh->num_triangles] = create_triangle_from_three_points(curr_point, next_point, new_point, vec2D(1e9, 1e9));
				mesh->triangles[mesh->num_triangles]->point_flags |= 1 << TRI_FLAG_IS_NEW;
				mesh->num_triangles++;
				if(last_point != NULL) {
					mesh->triangles[mesh->num_triangles] = create_triangle_from_three_points(curr_point, last_point, new_point, vec2D(1e9, 1e9));
					mesh->triangles[mesh->num_triangles]->point_flags |= 1 << TRI_FLAG_IS_NEW;
					mesh->num_triangles++;
				}
			} else new_point = NULL;

			last_point = new_point;


//			dir = get_from_point_dir_for_fine_mesh(prev_point, curr_point, next_point);
//			step = find_porkchop_edge(vec2D(curr_point->data.x, curr_point->data.y), dir, 0, max_dist, 0.001, curr_point->porkchop_point.arrival, dv_filter);
//			if(step != NULL) {
//				step = get_first(step);
//				double vinf = vector_mag(subtract_vectors(get_first(step)->next[0]->v_dep, get_first(step)->v_body));
//				new_point = malloc(sizeof(PcMeshPoint));
//				*new_point = (PcMeshPoint) {
//						.data = (struct Vector) {.x = step->date, .y = step->next[0]->date-step->date, dv_circ(get_first(step)->body, get_first(step)->body->atmo_alt+50e3, vinf)},
//						.porkchop_point = create_porkchop_point(get_last(step)),
//						.is_edge = 1,
//						.is_artificial = 1,
//						.triangles = malloc(10 * sizeof(PcMeshTriangle*)),
//						.num_triangles = 0,
//						.max_num_triangles = 10
//				};
//
//				mesh->points[mesh->num_points] = new_point;
//				mesh->num_points++;
//				new_point->porkchop_point = create_porkchop_point(get_last(step));
////				printf("Found: %f %f %f\n", new_point->data.x, new_point->data.y, new_point->data.z);
//
//				if(last_point == NULL && prev_point != NULL) {
//					mesh->triangles[mesh->num_triangles] = create_triangle_from_three_points(curr_point, next_point, new_point, vec2D(1e9, 1e9));
//					mesh->triangles[mesh->num_triangles]->point_flags |= 1 << TRI_FLAG_IS_NEW;
//					mesh->num_triangles++;
//				} else {
//					mesh->triangles[mesh->num_triangles] = create_triangle_from_three_points(curr_point, last_point, new_point, vec2D(1e9, 1e9));
//					mesh->triangles[mesh->num_triangles]->point_flags |= 1 << TRI_FLAG_IS_NEW;
//					mesh->num_triangles++;
//				}
//			} else new_point = NULL;

			last_point = new_point;

			prev_point = curr_point;
			curr_point = next_point;
		} while(curr_point != initial_point);
	}

	free(points_to_fine_mesh);
}

void reduce_pcmesh_big_triangles(PcMesh *mesh, struct Dv_Filter dv_filter) {
	for(int i = 0; i < mesh->num_triangles; i++) {
		if(is_triangle_big(*mesh->triangles[i])) {
			struct Vector2D barycenter_xy = {
					.x = (mesh->triangles[i]->points[0]->data.x + mesh->triangles[i]->points[1]->data.x + mesh->triangles[i]->points[2]->data.x)/3,
					.y = (mesh->triangles[i]->points[0]->data.y + mesh->triangles[i]->points[1]->data.y + mesh->triangles[i]->points[2]->data.y)/3
			};
			struct ItinStep *ptr = calc_itinerary(barycenter_xy.x, barycenter_xy.y, dv_filter, mesh->triangles[i]->points[0]->porkchop_point.arrival);
			if(ptr != NULL) {
				free_itinerary(get_first(ptr));
				mesh->triangles[i]->point_flags |= (1 << TRI_FLAG_SAVED_BIG);
			} else {
				remove_triangle_from_pcmesh(mesh, i);
				i--;
				continue;
			}
		}
	}
}

PcMeshGrid create_pcmesh_grid_from_porkchop(struct PorkchopPoint *porkchop_points, int num_deps, int *num_itins_per_dep) {
	PcMeshGrid grid = {.num_cols = num_deps};
	grid.num_col_rows = malloc(sizeof(size_t) * num_deps);
	grid.points = malloc(sizeof(struct Vector*) * num_deps);

	int idx = 0;
	for(int i = 0; i < num_deps; i++) {
		grid.num_col_rows[i] = num_itins_per_dep[i];
		grid.points[i] = malloc(sizeof(struct Vector) * num_itins_per_dep[i]);
		for(int j = 0; j < num_itins_per_dep[i]; j++) {
			struct ItinStep *step = porkchop_points[idx].arrival;
			while(step->prev->prev != NULL) step = step->prev;

			PcMeshPoint *new_point = malloc(sizeof(PcMeshPoint));
			*new_point = (PcMeshPoint) {
					.data = (struct Vector) {.x = porkchop_points[idx].dep_date, .y = step->date-step->prev->date, .z = porkchop_points[idx].dv_dep},
					.porkchop_point = porkchop_points[idx],
					.is_edge = i == 0 || i == num_deps-1 || j == 0 || j == num_itins_per_dep[i]-1,
					.triangles = malloc(100 * sizeof(PcMeshTriangle*)),
					.num_triangles = 0,
					.max_num_triangles = 100
			};
			grid.points[i][j] = new_point;
			idx++;
		}
	}

	return grid;
}

void resize_pcmesh_to_fit(PcMesh mesh, double max_x, double max_y, double max_z) {
	struct Vector max = mesh.points[0]->data;
	struct Vector min = mesh.points[0]->data;

	for(int i = 0; i < mesh.num_points; i++) {
		if(mesh.points[i]->data.x > max.x) max.x = mesh.points[i]->data.x;
		if(mesh.points[i]->data.x < min.x) min.x = mesh.points[i]->data.x;
		if(mesh.points[i]->data.y > max.y) max.y = mesh.points[i]->data.y;
		if(mesh.points[i]->data.y < min.y) min.y = mesh.points[i]->data.y;
		if(mesh.points[i]->data.z > max.z) max.z = mesh.points[i]->data.z;
		if(mesh.points[i]->data.z < min.z) min.z = mesh.points[i]->data.z;
	}

	max.x += 0.1*(max.x-min.x);
	min.x -= 0.1*(max.x-min.x);
	max.y += 0.1*(max.y-min.y);
	min.y -= 0.1*(max.y-min.y);

	struct Vector gradient = {
			max_x/(max.x-min.x),
			max_y/(max.y-min.y),
			max_z/(max.z-min.z),
	};


	for(int i = 0; i < mesh.num_points; i++) {
		mesh.points[i]->data.x -= min.x;
		mesh.points[i]->data.x *= gradient.x;
		mesh.points[i]->data.y -= min.y;
		mesh.points[i]->data.y *= gradient.y;
		mesh.points[i]->data.y  = max_y-mesh.points[i]->data.y;
		mesh.points[i]->data.z -= min.z;
		mesh.points[i]->data.z *= gradient.z;
	}
}



void convert_pcmesh_to_total_dur(PcMesh mesh) {
	for(int i = 0; i < mesh.num_points; i++) {
		PcMeshPoint *point = mesh.points[i];
		point->data.y = point->porkchop_point.arrival->date - get_first(point->porkchop_point.arrival)->date;
	}
}

PcMesh create_pcmesh_from_grid(PcMeshGrid grid) {
	PcMesh mesh;
	mesh.num_triangles = 0;
	mesh.max_num_triangles = 1000000;
	mesh.num_points = 0;
	mesh.max_num_points = 1000000;
	mesh.triangles = malloc(mesh.max_num_triangles*sizeof(PcMeshTriangle*));
	mesh.points = malloc(mesh.max_num_points*sizeof(PcMeshPoint*));

	for(int i = 0; i < grid.num_cols; i++) {
		for(int j = 0; j < grid.num_col_rows[i]; j++) {
			mesh.points[mesh.num_points] = grid.points[i][j];
			mesh.num_points++;
		}
	}

	struct Vector2D big_threshold = {1.5, 6};

	for(int x_idx = 0; x_idx < grid.num_cols-1; x_idx++) {
		int y_idx0 = 0, y_idx1 = 0;
		while(y_idx0 < grid.num_col_rows[x_idx] - 1 && y_idx1 < grid.num_col_rows[x_idx + 1] - 1) {
			PcMeshPoint *p0 = grid.points[x_idx][y_idx0];
			PcMeshPoint *p1 = grid.points[1 + x_idx][y_idx1];
			PcMeshPoint *p2 = grid.points[x_idx][1 + y_idx0];
			PcMeshPoint *p3 = grid.points[1 + x_idx][1 + y_idx1];

			double dist03 = vector_distance(vec2D(p0->data.x, p0->data.y), vec2D(p3->data.x, p3->data.y));
			double dist12 = vector_distance(vec2D(p1->data.x, p1->data.y), vec2D(p2->data.x, p2->data.y));


//			double dist01 = vector_distance(vec2D(p0->data.x, p0->data.y), vec2D(p1->data.x, p1->data.y));
//			double dist02 = vector_distance(vec2D(p0->data.x, p0->data.y), vec2D(p2->data.x, p2->data.y));
//			double dist13 = vector_distance(vec2D(p1->data.x, p1->data.y), vec2D(p3->data.x, p3->data.y));
//			double dist23 = vector_distance(vec2D(p3->data.x, p3->data.y), vec2D(p2->data.x, p2->data.y));
//			printf("%f   %f        %f  %f\n", dist01, dist23, dist02, dist13);

			if(dist03 < dist12) {
				mesh.triangles[mesh.num_triangles] = create_triangle_from_three_points(p0, p1, p3, big_threshold);
				y_idx1++;
			} else {
				mesh.triangles[mesh.num_triangles] = create_triangle_from_three_points(p0, p1, p2, big_threshold);
				y_idx0++;
			}
			mesh.num_triangles++;

		}

		if(y_idx0 == grid.num_col_rows[x_idx] - 1) {
			while(y_idx1 < grid.num_col_rows[x_idx + 1] - 1) {
				mesh.triangles[mesh.num_triangles] = create_triangle_from_three_points(grid.points[x_idx][y_idx0], grid.points[x_idx + 1][y_idx1], grid.points[x_idx + 1][y_idx1 + 1], big_threshold);
				y_idx1++;
				mesh.num_triangles++;
			}
		}

		if(y_idx1 == grid.num_col_rows[x_idx + 1] - 1) {
			while(y_idx0 < grid.num_col_rows[x_idx] - 1) {
				mesh.triangles[mesh.num_triangles] = create_triangle_from_three_points(grid.points[x_idx][y_idx0], grid.points[x_idx][y_idx0 + 1], grid.points[x_idx + 1][y_idx1], big_threshold);
				y_idx0++;
				mesh.num_triangles++;
			}
		}
	}

	return mesh;
}