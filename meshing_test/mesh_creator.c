#include "mesh_creator.h"
#include "orbit_calculator/transfer_calc.h"
#include "orbit_calculator/transfer_tools.h"
#include <math.h>


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

void add_grid_outside_point(MeshGrid *grid, double x, double y) {
	grid->outside_points[grid->num_outside_points] = (struct Vector2D) {x,y};
	grid->num_outside_points++;
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
			int idx = j;
			while(idx > 0 && grid.points[i][idx].y < grid.points[i][idx-1].y) {
				struct Vector temp = grid.points[i][idx];
				grid.points[i][idx] = grid.points[i][idx-1];
				grid.points[i][idx-1] = temp;
				idx--;
			}
		}
	}
	grid.outside_points = malloc(10000*sizeof(struct Vector2D));
	grid.num_outside_points = 0;

	return grid;
};

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
		double dt = template->date-template->prev->date;
		find_viable_flybys(step, system, template->body, dt*0.75*86400, dt*1.25*86400);

		if(step->num_next_nodes == 0) {free_itinerary(step); return NULL;}
		printf("%d\n", step->num_next_nodes);
		step = step->next[0];
	}

	return step;
}

MeshGrid fine_mesh_grid_top_bottom(MeshGrid grid, double **init_durs, struct Dv_Filter dv_filter, struct PorkchopPoint *porkchop_points) {
	if(grid.points == NULL) return grid;

	int idx = 0;

	for(int i = 0; i < grid.num_columns; i++) {
		int num_col_values = grid.num_points[i]+10000;
		struct Vector *col = malloc(sizeof(struct Vector) * num_col_values);
		double dep = grid.points[i][0].x;
		double init_dur = init_durs[i][0];
		double step = grid.num_points[i] > 1 ? init_durs[i][0]-init_durs[i][1] : -10;
		int col_idx = 0;
		struct ItinStep *ptr = porkchop_points[idx].arrival;
		while(fabs(step) > 0.001) {
			init_dur += step;
//			printf("%f  %f\n", init_dur, porkchop_points[idx].arrival->prev->prev->date-porkchop_points[idx].arrival->prev->prev->prev->date);
			struct ItinStep *ptr_temp = calc_itinerary(dep, init_dur, dv_filter, ptr);
			if(ptr_temp != NULL) {
				if(ptr_temp->date-get_first(ptr_temp)->date > grid.points[i][0].y) {
					free_itinerary(get_first(ptr_temp));
					init_dur -= step;
					step /= -2;
					continue;
				}
				ptr = ptr_temp;
				col[col_idx].x = dep;
				col[col_idx].y = ptr->date-dep;
				double vinf = vector_mag(subtract_vectors(get_first(ptr)->next[0]->v_dep, get_first(ptr)->v_body));
				col[col_idx].z = dv_circ(get_first(ptr)->body, get_first(ptr)->body->atmo_alt+50e3, vinf);
				col[col_idx].z = 0;
				col_idx++;
			} else {
				init_dur -= step;
				step /= 2;
			}
		}

		for(int j = 0; j < grid.num_points[i]; j++) {
			col[col_idx].x = grid.points[i][j].x;
			col[col_idx].y = grid.points[i][j].y;
			col[col_idx].z = grid.points[i][j].z;
//			if(j < grid.num_points[i] - 10 && j > 10) j += grid.num_points[i]/50;
			col_idx++;
		}

		init_dur = init_durs[i][grid.num_points[i]-1];
		step = grid.num_points[i] > 1 ? init_durs[i][grid.num_points[i]-1]-init_durs[i][grid.num_points[i]-2] : 10;
		ptr = porkchop_points[idx+grid.num_points[i]-1].arrival;
		while(fabs(step) > 0.001) {
			init_dur += step;
			struct ItinStep *ptr_temp = calc_itinerary(dep, init_dur, dv_filter, ptr);
			if(ptr_temp != NULL) {
				if(ptr_temp->date-get_first(ptr_temp)->date < grid.points[i][grid.num_points[i]-1].y) {
					free_itinerary(get_first(ptr_temp));
					init_dur -= step;
					step /= -2;
					continue;
				}
				ptr = ptr_temp;
				col[col_idx].x = dep;
				col[col_idx].y = ptr->date-dep;
				double vinf = vector_mag(subtract_vectors(get_first(ptr)->next[0]->v_dep, get_first(ptr)->v_body));
				col[col_idx].z = dv_circ(get_first(ptr)->body, get_first(ptr)->body->atmo_alt+50e3, vinf);
				col[col_idx].z = 0;
				col_idx++;
			} else {
				init_dur -= step;
				step /= 2;
			}
		}


		idx += grid.num_points[i];
		free(grid.points[i]);
		grid.points[i] = col;
		grid.num_points[i] = col_idx;
	}

	return grid;
}

Mesh mesh_from_porkchop(struct PorkchopPoint *porkchop_points, int num_itins, int num_deps, int *num_itins_per_dep) {
	double *dep = malloc(num_deps * sizeof(double));
	double **dur = malloc(num_deps * sizeof(double*));
	double **init_dur = malloc(num_deps * sizeof(double*));
	double **d_v = malloc(num_deps * sizeof(double*));
	int idx = 0;
	for(int i = 0; i < num_deps; i++) {
		dep[i] = porkchop_points[idx].dep_date;
		dur[i] = malloc(num_itins_per_dep[i] * sizeof(double*));
		init_dur[i] = malloc(num_itins_per_dep[i] * sizeof(double*));
		d_v[i] = malloc(num_itins_per_dep[i] * sizeof(double*));
		for(int j = 0; j < num_itins_per_dep[i]; j++) {
			struct ItinStep *step = porkchop_points[idx].arrival;
			while(step->prev->prev != NULL) step = step->prev;
			dur[i][j] = porkchop_points[idx].dur;
			init_dur[i][j] = step->date-step->prev->date;
			d_v[i][j] = porkchop_points[idx].dv_dep;
			idx++;
		}
	}

	MeshGrid grid = create_mesh_grid(dep, dur, d_v, num_deps, num_itins_per_dep);



	struct Dv_Filter dv_filter = {1e9, 4000, 1e9, TF_FLYBY};
	grid = fine_mesh_grid_top_bottom(grid, init_dur, dv_filter, porkchop_points);



	struct Vector max = grid.points[0][0];
	struct Vector min = grid.points[0][0];

	for(int i = 0; i < grid.num_columns; i++) {
		for(int j = 0; j < grid.num_points[i]; j++) {
			if(grid.points[i][j].x > max.x) max.x = grid.points[i][j].x;
			if(grid.points[i][j].x < min.x) min.x = grid.points[i][j].x;
			if(grid.points[i][j].y > max.y) max.y = grid.points[i][j].y;
			if(grid.points[i][j].y < min.y) min.y = grid.points[i][j].y;
			if(grid.points[i][j].z > max.z) max.z = grid.points[i][j].z;
			if(grid.points[i][j].z < min.z) min.z = grid.points[i][j].z;
		}
	}

	max.x += 0.1*(max.x-min.x);
	min.x -= 0.1*(max.x-min.x);
	max.y += 0.1*(max.y-min.y);
	min.y -= 0.1*(max.y-min.y);

	struct Vector gradient = {
			2000/(max.x-min.x),
			2000/(max.y-min.y),
			1/(max.z-min.z),
	};


	for(int i = 0; i < grid.num_columns; i++) {
		for(int j = 0; j < grid.num_points[i]; j++) {
			grid.points[i][j].x -= min.x;
			grid.points[i][j].x *= gradient.x;
			grid.points[i][j].y -= min.y;
			grid.points[i][j].y *= gradient.y;
			grid.points[i][j].z -= min.z;
			grid.points[i][j].z *= gradient.z;
		}
	}

	return create_mesh_from_grid(grid, gradient.x*1.01);
}

MeshGrid grid_from_porkchop(struct PorkchopPoint *porkchop_points, int num_itins, int num_deps, int *num_itins_per_dep) {
	double *dep = malloc(num_deps * sizeof(double));
	double **dur = malloc(num_deps * sizeof(double*));
	double **init_dur = malloc(num_deps * sizeof(double*));
	double **d_v = malloc(num_deps * sizeof(double*));
	int idx = 0;
	for(int i = 0; i < num_deps; i++) {
		dep[i] = porkchop_points[idx].dep_date;
		dur[i] = malloc(num_itins_per_dep[i] * sizeof(double*));
		init_dur[i] = malloc(num_itins_per_dep[i] * sizeof(double*));
		d_v[i] = malloc(num_itins_per_dep[i] * sizeof(double*));
		for(int j = 0; j < num_itins_per_dep[i]; j++) {
			struct ItinStep *step = porkchop_points[idx].arrival;
			while(step->prev->prev != NULL) step = step->prev;
			dur[i][j] = porkchop_points[idx].dur;
			init_dur[i][j] = step->date-step->prev->date;
			d_v[i][j] = porkchop_points[idx].dv_dep;
			idx++;
		}
	}

	MeshGrid grid = create_mesh_grid(dep, dur, d_v, num_deps, num_itins_per_dep);
//	grid = fine_mesh_grid_top_bottom(grid, init_dur);
	struct Vector max = grid.points[0][0];
	struct Vector min = grid.points[0][0];

	for(int i = 0; i < grid.num_columns; i++) {
		for(int j = 0; j < grid.num_points[i]; j++) {
			if(grid.points[i][j].x > max.x) max.x = grid.points[i][j].x;
			if(grid.points[i][j].x < min.x) min.x = grid.points[i][j].x;
			if(grid.points[i][j].y > max.y) max.y = grid.points[i][j].y;
			if(grid.points[i][j].y < min.y) min.y = grid.points[i][j].y;
			if(grid.points[i][j].z > max.z) max.z = grid.points[i][j].z;
			if(grid.points[i][j].z < min.z) min.z = grid.points[i][j].z;
		}
	}

	struct Vector gradient = {
			2000/(max.x-min.x),
			2000/(max.y-min.y),
			1/(max.z-min.z)/max.z,
	};


	for(int i = 0; i < grid.num_columns; i++) {
		for(int j = 0; j < grid.num_points[i]; j++) {
			grid.points[i][j].x -= min.x;
			grid.points[i][j].x *= gradient.x;
			grid.points[i][j].y -= min.y;
			grid.points[i][j].y *= gradient.y;
			grid.points[i][j].z -= min.z;
			grid.points[i][j].z *= gradient.z;
		}
	}

	return grid;
}

Mesh create_mesh_from_grid(MeshGrid grid, double max_x_diff) {
	Mesh mesh;
	mesh.num_triangles = 0;
	mesh.triangles = malloc(1000000*sizeof(MeshTriangle));
	for(int x_idx = 0; x_idx < grid.num_columns-1; x_idx++) {
		if(grid.points[x_idx+1][0].x - grid.points[x_idx][0].x > max_x_diff) continue;
		int y_idx0 = 0, y_idx1 = 0;
		while(y_idx0 < grid.num_points[x_idx] - 1 && y_idx1 < grid.num_points[x_idx + 1] - 1) {
			struct Vector p1 = grid.points[x_idx][y_idx0];
			struct Vector p2 = grid.points[1 + x_idx][y_idx1];
			struct Vector p3 = grid.points[x_idx][1 + y_idx0];
			struct Vector p4 = grid.points[1 + x_idx][1 + y_idx1];

			double dist14 = vector_distance(vec2D(p1.x, p1.y), vec2D(p4.x, p4.y));
			double dist23 = vector_distance(vec2D(p2.x, p2.y), vec2D(p3.x, p3.y));

			if(dist14 < dist23) {
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