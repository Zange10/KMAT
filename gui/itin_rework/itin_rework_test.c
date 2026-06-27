#include "itin_rework_test.h"
#include "itin_rework_tools.h"

#include "gui/gui_manager.h"
#include "gui/gui_tools/screen.h"
#include "gui/drawing.h"
#include "mesh.h"
#include "mesh_drawing.h"
#include "geometrylib.h"
#include "gui/gui_tools/coordinate_system.h"
#include <math.h>
#include <sys/time.h>

#include "gui/gui_tools/coordinate_system_drawing.h"
#include "tools/tool_funcs.h"

GObject *ir_window;
GObject *da_ir_graphing0;
GObject *da_ir_graphing1;
GObject *cb_ir_system;
GObject *cb_ir_central_body;
GObject *cb_ir_depbody;
GObject *cb_ir_arrbody;
GObject *tf_ir_mindepdate;
GObject *tf_ir_maxdepdate;
GObject *tf_ir_mindur;
GObject *tf_ir_maxdur;
GObject *tf_ir_tolerance;
GObject *tf_ir_numdeps;
GObject *tf_ir_maxdv;
GObject *tf_ir_pcgroup0;
GObject *tf_ir_pcgroup1;
GObject *tf_ir_pcgroup2;

CelestSystem *ir_system;
CoordinateSystem *ir_coord_sys0;
CoordinateSystem *ir_coord_sys1;


double ir_dep_periapsis = 50e3;


typedef struct TimingMeasurement {
	struct TimingMeasurement *next;
	double elapsed_time;
	char name[256];
} TimingMeasurement;

typedef struct TimingMeasurements {
	struct timeval start, end;
	TimingMeasurement *first;
} TimingMeasurements;

TimingMeasurements init_timing_measurements() {
	TimingMeasurements tm;
	tm.first = NULL;
	gettimeofday(&tm.start, NULL);
	gettimeofday(&tm.end, NULL);
	return tm;
}

void start_time_measurement(TimingMeasurements *tm) {
	gettimeofday(&tm->start, NULL);
}

double get_total_timing_time(TimingMeasurements tm) {
	TimingMeasurement *ptr = tm.first;
	double total_time = 0;
	while(ptr) {
		total_time += ptr->elapsed_time;
		ptr = ptr->next;
	}
	return total_time;
}

void print_timing_measurements(TimingMeasurements tm) {
	double total_time = get_total_timing_time(tm);
	TimingMeasurement *ptr = tm.first;
	while(ptr) {
		printf("|%50s:%10.3fms  (%.2f %%)\n", ptr->name, ptr->elapsed_time*1000, ptr->elapsed_time/total_time*100);
		ptr = ptr->next;
	}
	print_separator(100);
	printf("|%50s:  %.3fms\n", "TOTAL TIME", total_time*1000);
	print_separator(100);
}

void end_time_measurement(TimingMeasurements *tm, char *name) {
	gettimeofday(&tm->end, NULL);
	TimingMeasurement *ptr = tm->first;
	if(ptr == NULL) {
		tm->first = malloc(sizeof(TimingMeasurement));
		ptr = tm->first;
	} else {
		while(ptr->next) ptr = ptr->next;
		ptr->next = malloc(sizeof(TimingMeasurement));
		ptr = ptr->next;
	}

	ptr->next = NULL;
	ptr->elapsed_time = (tm->end.tv_sec - tm->start.tv_sec) + (tm->end.tv_usec - tm->start.tv_usec) / 1000000.0;
	sprintf(ptr->name, "%s", name);
}

TimingMeasurement *get_last_timing_measurement(TimingMeasurements tm) {
	TimingMeasurement *ptr = tm.first;
	while(ptr->next) {
		ptr = ptr->next;
	}
	return ptr;
}

void free_timing_measurements(TimingMeasurements *tm) {
	if(!tm) return;
	if(tm->first) {
		TimingMeasurement *ptr = tm->first;
		while(ptr) {
			TimingMeasurement *next = ptr->next;
			free(ptr);
			ptr = next;
		}
	}
}

void on_ir_screen_resize(GtkWidget *widget, cairo_t *cr, gpointer *ptr);


void init_itin_rework_test(GtkBuilder *builder) {
	ir_window = gtk_builder_get_object(builder, "window");
	da_ir_graphing0 = gtk_builder_get_object(builder, "da_ir_graphing0");
	da_ir_graphing1 = gtk_builder_get_object(builder, "da_ir_graphing1");
	cb_ir_system = gtk_builder_get_object(builder, "cb_ir_system");
	cb_ir_central_body = gtk_builder_get_object(builder, "cb_ir_central_body");
	cb_ir_depbody = gtk_builder_get_object(builder, "cb_ir_depbody");
	cb_ir_arrbody = gtk_builder_get_object(builder, "cb_ir_arrbody");
	tf_ir_mindepdate = gtk_builder_get_object(builder, "tf_ir_mindepdate");
	tf_ir_maxdepdate = gtk_builder_get_object(builder, "tf_ir_maxdepdate");
	tf_ir_mindur = gtk_builder_get_object(builder, "tf_ir_maxarrdate");
	tf_ir_maxdur = gtk_builder_get_object(builder, "tf_ir_maxdur");
	tf_ir_tolerance = gtk_builder_get_object(builder, "tf_ir_tolerance");
	tf_ir_numdeps = gtk_builder_get_object(builder, "tf_ir_numdeps");
	tf_ir_maxdv = gtk_builder_get_object(builder, "tf_ir_maxdv");
	tf_ir_pcgroup0 = gtk_builder_get_object(builder, "tf_ir_pcgroup0");
	tf_ir_pcgroup1 = gtk_builder_get_object(builder, "tf_ir_pcgroup1");
	tf_ir_pcgroup2 = gtk_builder_get_object(builder, "tf_ir_pcgroup2");

	ir_system = NULL;

	create_combobox_dropdown_text_renderer(cb_ir_system, GTK_ALIGN_CENTER);
	create_combobox_dropdown_text_renderer(cb_ir_central_body, GTK_ALIGN_CENTER);
	create_combobox_dropdown_text_renderer(cb_ir_depbody, GTK_ALIGN_CENTER);
	create_combobox_dropdown_text_renderer(cb_ir_arrbody, GTK_ALIGN_CENTER);
	update_system_dropdown(GTK_COMBO_BOX(cb_ir_system));
	if(get_num_available_systems() > 0) {
		ir_system = get_available_systems()[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_system))];
		update_central_body_dropdown(GTK_COMBO_BOX(cb_ir_central_body), ir_system);
		update_body_dropdown(GTK_COMBO_BOX(cb_ir_depbody), ir_system);
		update_body_dropdown(GTK_COMBO_BOX(cb_ir_arrbody), ir_system);
	}

	ir_coord_sys0 = new_coordinate_system(GTK_WIDGET(da_ir_graphing0));
	ir_coord_sys1 = new_coordinate_system(GTK_WIDGET(da_ir_graphing1));
}

void remove_step_from_itinerary_void_ptr(void *ptr) { remove_step_from_itinerary(ptr); }

G_MODULE_EXPORT void on_ir_system_change() {
	if(get_num_available_systems() > 0) {
		ir_system = get_available_systems()[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_system))];
		update_central_body_dropdown(GTK_COMBO_BOX(cb_ir_central_body), ir_system);
		update_body_dropdown(GTK_COMBO_BOX(cb_ir_depbody), ir_system);
		update_body_dropdown(GTK_COMBO_BOX(cb_ir_arrbody), ir_system);
	}
}


G_MODULE_EXPORT void on_ir_central_body_change() {
	if(get_num_available_systems() > 0) {
		if(get_number_of_subsystems(get_available_systems()[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_system))]) == 0) {
			gtk_widget_set_sensitive(GTK_WIDGET(cb_ir_central_body), 0);
			return;
		}
		gtk_widget_set_sensitive(GTK_WIDGET(cb_ir_central_body), 1);
		CelestSystem *ic_og_system = get_available_systems()[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_system))];
		ir_system = get_subsystem_from_system_and_id(ic_og_system, gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_central_body)));
		update_body_dropdown(GTK_COMBO_BOX(cb_ir_depbody), ir_system);
		update_body_dropdown(GTK_COMBO_BOX(cb_ir_arrbody), ir_system);
	}
}

double * step_to_array(struct ItinStep *step) {
	double *array = malloc(NUM_PORKCHOP_MESH_VALUE_TYPES * sizeof(double));
	array[MESH_VAL_DATE] = step->date;
	array[MESH_VAL_DEPX] = step->v_dep.x;
	array[MESH_VAL_DEPY] = step->v_dep.y;
	array[MESH_VAL_DEPZ] = step->v_dep.z;
	array[MESH_VAL_BODY_RX] = step->r.x;
	array[MESH_VAL_BODY_RY] = step->r.y;
	array[MESH_VAL_BODY_RZ] = step->r.z;
	array[MESH_VAL_BODY_VX] = step->v_body.x;
	array[MESH_VAL_BODY_VY] = step->v_body.y;
	array[MESH_VAL_BODY_VZ] = step->v_body.z;
	array[MESH_VAL_ARRX] = step->v_arr.x;
	array[MESH_VAL_ARRY] = step->v_arr.y;
	array[MESH_VAL_ARRZ] = step->v_arr.z;
	array[MESH_VAL_VINF] = mag_vec3(subtract_vec3(step->v_arr, step->v_body));
	if(step->prev && step->prev->prev) {
		array[MESH_VAL_RPE] = get_flyby_periapsis(step->prev->v_arr, step->v_dep, step->prev->v_body, step->prev->body);
	} else array[MESH_VAL_RPE] = 1e9;
	return array;
}

void free_segment_group(SegmentGroup *group) {
	if (!group) return;
	for(int i = 0; i < group->num_next_groups; i++) {
		free_segment_group(group->next[i]);
	}

	// free_mesh(group->mesh);
	// data_array2_free(group->vinf_array);
	// if(group->segment_steps) free(group->segment_steps);
	free(group->next);

	free(group);
}


MeshPoint2 * quad_test_function(double x, double y, void *params) {
	double a = 10*sin(x/8);
	double b = 10*sin(y/3*x/60);
	double z = a*b;

	double *vals = malloc(sizeof(double));
	vals[0] = z;

	return create_mesh_point(vec2(x, y), vals, 1);
}

MeshPoint2 * test_(double jd_dep, double duration, void *params_p) {
	SegmentGroup *group = params_p;
	Body *cb = group->dep_body->orbit.cb;

	Vector3 r0 = osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, cb).r;
	OSV osv_arr = osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_dep + duration, cb);
	Vector3 r1 = osv_arr.r;

	Lambert3 lambert_sol = calc_lambert3(r0, r1, duration*86400, cb);

	double *array = malloc(NUM_PORKCHOP_MESH_VALUE_TYPES * sizeof(double));
	array[MESH_VAL_DATE] = jd_dep+duration;
	array[MESH_VAL_DEPX] = lambert_sol.v0.x;
	array[MESH_VAL_DEPY] = lambert_sol.v0.y;
	array[MESH_VAL_DEPZ] = lambert_sol.v0.z;
	array[MESH_VAL_BODY_RX] = osv_arr.r.x;
	array[MESH_VAL_BODY_RY] = osv_arr.r.y;
	array[MESH_VAL_BODY_RZ] = osv_arr.r.z;
	array[MESH_VAL_BODY_VX] = osv_arr.v.x;
	array[MESH_VAL_BODY_VY] = osv_arr.v.y;
	array[MESH_VAL_BODY_VZ] = osv_arr.v.z;
	array[MESH_VAL_ARRX] = lambert_sol.v1.x;
	array[MESH_VAL_ARRY] = lambert_sol.v1.y;
	array[MESH_VAL_ARRZ] = lambert_sol.v1.z;
	array[MESH_VAL_VINF] = mag_vec3(subtract_vec3(lambert_sol.v1, osv_arr.v));
	// if(group->prev_body) {
	// 	array[MESH_VAL_RPE] = get_flyby_periapsis(group->prev_v_arr, group->prev_v_dep, group->prev_v_body, group->prev_body);
	// } else array[MESH_VAL_RPE] = 1e9;

	MeshPoint2 *new_mesh_point = create_mesh_point(vec2(jd_dep, duration), array, NUM_PORKCHOP_MESH_VALUE_TYPES);

	return new_mesh_point;
}

void test_populate(MeshPoint2 *mesh_point, void *params_p) {
	SegmentGroup *group = params_p;
	Body *cb = group->dep_body->orbit.cb;

	double jd_dep = mesh_point->pos.x;
	double duration = mesh_point->pos.y;

	Vector3 r0 = osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, cb).r;
	OSV osv_arr = osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_dep + duration, cb);
	Vector3 r1 = osv_arr.r;

	Lambert3 lambert_sol = calc_lambert3(r0, r1, duration*86400, cb);

	double *array = malloc(NUM_PORKCHOP_MESH_VALUE_TYPES * sizeof(double));
	array[MESH_VAL_DATE] = jd_dep+duration;
	array[MESH_VAL_DEPX] = lambert_sol.v0.x;
	array[MESH_VAL_DEPY] = lambert_sol.v0.y;
	array[MESH_VAL_DEPZ] = lambert_sol.v0.z;
	array[MESH_VAL_BODY_RX] = osv_arr.r.x;
	array[MESH_VAL_BODY_RY] = osv_arr.r.y;
	array[MESH_VAL_BODY_RZ] = osv_arr.r.z;
	array[MESH_VAL_BODY_VX] = osv_arr.v.x;
	array[MESH_VAL_BODY_VY] = osv_arr.v.y;
	array[MESH_VAL_BODY_VZ] = osv_arr.v.z;
	array[MESH_VAL_ARRX] = lambert_sol.v1.x;
	array[MESH_VAL_ARRY] = lambert_sol.v1.y;
	array[MESH_VAL_ARRZ] = lambert_sol.v1.z;
	array[MESH_VAL_VINF] = mag_vec3(subtract_vec3(lambert_sol.v1, osv_arr.v));
	// if(group->prev_body) {
	// 	array[MESH_VAL_RPE] = get_flyby_periapsis(group->prev_v_arr, group->prev_v_dep, group->prev_v_body, group->prev_body);
	// } else array[MESH_VAL_RPE] = 1e9;

	mesh_point->val = array;
	mesh_point->num_val = NUM_PORKCHOP_MESH_VALUE_TYPES;
}

typedef struct ErrorFuncParams {
	double max_error;
	int val_idx;
} ErrorFuncParams;

typedef struct BoundaryFuncParams {
	SegmentGroup *group;
} BoundaryFuncParams;

bool is_point_inside_boundary(Vector2 p, Boundary bdr) {
	for(int i = 0; i < bdr.num; i++) {
		size_t num = data_array2_size(bdr.lower_bdrs[i]);
		Vector2 *ld = data_array2_get_data(bdr.lower_bdrs[i]);

		if(ld[num-1].x < p.x) continue;
		if(ld[0].x > p.x) continue;

		if(	p.y >= interpolate_from_sorted_data_array(bdr.lower_bdrs[i], p.x) &&
			p.y <= interpolate_from_sorted_data_array(bdr.upper_bdrs[i], p.x))
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

		if(	quad->center->pos.y >= interpolate_from_sorted_data_array(bdr.lower_bdrs[i], quad->center->pos.x) &&
			quad->center->pos.y <= interpolate_from_sorted_data_array(bdr.upper_bdrs[i], quad->center->pos.x))
			return true;

		for(int j = 0; j < 4; j++) {
			if(	quad->corner[j]->pos.y >= interpolate_from_sorted_data_array(bdr.lower_bdrs[i], quad->corner[j]->pos.x) &&
				quad->corner[j]->pos.y <= interpolate_from_sorted_data_array(bdr.upper_bdrs[i], quad->corner[j]->pos.x))
				return true;
		}

		if(is_quad_crossed_by_line(quad, bdr.lower_bdrs[i]) || is_quad_crossed_by_line(quad, bdr.upper_bdrs[i]))
			return true;
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

Boundary combine_boundaries(Boundary bdr0, Boundary bdr1) {
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
				if(prev_was_out_l) data_array2_append_new(lower, l0[j-1].x, l0[j-1].y);
				data_array2_append_new(lower, l0[j].x, l0[j].y);
				prev_was_out_l = false;
			} else {
				if(!prev_was_out_l && j > 0) {
					data_array2_append_new(lower, l0[j].x, l0[j].y);
					append_to_boundary(&low_bdrs, NULL, lower);
					lower = data_array2_create();
				}
				prev_was_out_l = true;
			}
			if(is_point_inside_boundary(u0[j], bdr1)) {
				if(prev_was_out_u) data_array2_append_new(upper, u0[j-1].x, u0[j-1].y);
				data_array2_append_new(upper, u0[j].x, u0[j].y);
				prev_was_out_u = false;
			} else {
				if(!prev_was_out_u && j > 0) {
					data_array2_append_new(upper, u0[j].x, u0[j].y);
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
				if(prev_was_out_l) data_array2_append_new(lower, l1[j-1].x, l1[j-1].y);
				data_array2_append_new(lower, l1[j].x, l1[j].y);
				prev_was_out_l = false;
			} else {
				if(!prev_was_out_l && j > 0) {
					data_array2_append_new(lower, l1[j].x, l1[j].y);
					append_to_boundary(&low_bdrs, NULL, lower);
					lower = data_array2_create();
				}
				prev_was_out_l = true;
			}
			if(is_point_inside_boundary(u1[j], bdr0)) {
				if(prev_was_out_u) data_array2_append_new(upper, u1[j-1].x, u1[j-1].y);
				data_array2_append_new(upper, u1[j].x, u1[j].y);
				prev_was_out_u = false;
			} else {
				if(!prev_was_out_u && j > 0) {
					data_array2_append_new(upper, u1[j].x, u1[j].y);
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
					data_array2_append_new(up_bdrs.upper_bdrs[i], u_n[k].x, u_n[k].y);
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
					data_array2_append_new(low_bdrs.lower_bdrs[i], u_n[k].x, u_n[k].y);
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
					printf("%f\n", interpolate_from_sorted_data_array(low_bdrs.lower_bdrs[j], x));
					data_array2_insert_new(low_bdrs.lower_bdrs[j], x, interpolate_from_sorted_data_array(low_bdrs.lower_bdrs[j], x));
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
					data_array2_insert_new(up_bdrs.upper_bdrs[j], x, interpolate_from_sorted_data_array(up_bdrs.upper_bdrs[j], x));
					d = data_array2_get_data(up_bdrs.upper_bdrs[j]);
					num_d++;
				}
			}
		}

		last_x = x;
		print_date(convert_JD_date(x, DATE_ISO), 1);
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
					data_array2_append_new(new_split_bdr, d[k].x, d[k].y);
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
					data_array2_append_new(new_split_bdr, d[k].x, d[k].y);
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
					double y = interpolate_from_sorted_data_array(low_split_bdrs.lower_bdrs[0], x);
					double yu = interpolate_from_sorted_data_array(up_split_bdrs.upper_bdrs[j], x);
					double yub = interpolate_from_sorted_data_array(best_up, x);

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
			double y = interpolate_from_sorted_data_array(low_split_bdrs.lower_bdrs[0], x);
			double yu = interpolate_from_sorted_data_array(up_split_bdrs.upper_bdrs[best_up_idx], x);
			double yl = interpolate_from_sorted_data_array(low_split_bdrs.lower_bdrs[j], x);

			if(yu-yl < yu-y) {
				found_better_lower = true;
				break;
			}
		}

		if(!found_better_lower) {
			append_to_boundary(&new_boundary, low_split_bdrs.lower_bdrs[0], up_split_bdrs.upper_bdrs[best_up_idx]);

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

bool quad_test_is_in_bounds_function(Quad *quad, void *params_p) {
	BoundaryFuncParams *params = params_p;
	SegmentGroup *group = params->group;
	// bool is_croosed_by_bounds = is_quad_crossed_by_line(quad, group->group_bdr.upper_bdrs[0]) || is_quad_crossed_by_line(quad, group->group_bdr.lower_bdrs[0]);
	//
	// if(quad->center->pos.y < interpolate_from_sorted_data_array(group->group_bdr.lower_bdrs[0], quad->center->pos.x)) {
	// 	if(!is_croosed_by_bounds) return false;
	// }
	// if(quad->center->pos.y > interpolate_from_sorted_data_array(group->group_bdr.upper_bdrs[0], quad->center->pos.x)) {
	// 	if(!is_croosed_by_bounds) return false;
	// }
	//
	// if(is_croosed_by_bounds) {
	// 	set_quad_flag(quad, QUAD_FLAG_SPLIT);
	// }
	//
	//
	//
	// // inverse also catches NAN for interpolation if line x is too short
	// if( !(quad->center->pos.y >= interpolate_from_sorted_data_array(params->depdv_lower_boundary, quad->center->pos.x)) ||
	// 	!(quad->center->pos.y <= interpolate_from_sorted_data_array(params->depdv_upper_boundary, quad->center->pos.x))) {
	//
	// 	for(int i = 0; i < 4; i++) {
	// 		if( (quad->corner[i]->pos.y >= interpolate_from_sorted_data_array(params->depdv_lower_boundary, quad->corner[i]->pos.x)) &&
	// 			(quad->corner[i]->pos.y <= interpolate_from_sorted_data_array(params->depdv_upper_boundary, quad->corner[i]->pos.x))) return true;
	// 	}
	//
	// 	if(!is_quad_crossed_by_line(quad, params->depdv_lower_boundary) && !is_quad_crossed_by_line(quad, params->depdv_upper_boundary)) return false;
	// }
	//
	// return true;
	return is_quad_inside_boundary(quad, group->dv_bdr);
}

bool quad_test_error_function(Quad *quad, void *params_p) {
	ErrorFuncParams *params = params_p;

	double interp_val = get_quad_interpolated_value(quad, quad->center->pos, params->val_idx);
	double e = fabs(interp_val - quad->center->val[params->val_idx]);

	return e > params->max_error;
}

int match_to_boundary(Quad *quad, void *params_p, QuadPointFunc *point_func, int max_rf_level) {
	BoundaryFuncParams *params = params_p;
	SegmentGroup *group = params->group;

	if(!is_quad_inside_boundary(quad, group->dv_bdr)) {
		free_quad(quad, true); return 0;
	}
	// inverse also catches NAN for interpolation if line x is too short
	// if( !(quad->center->pos.y >= interpolate_from_sorted_data_array(params->depdv_lower_boundary, quad->center->pos.x)) ||
	// 	!(quad->center->pos.y <= interpolate_from_sorted_data_array(params->depdv_upper_boundary, quad->center->pos.x))) {
	//
	// 	bool points_inside_boundary = false;
	//
	// 	for(int i = 0; i < 4; i++) {
	// 		if( (quad->corner[i]->pos.y >= interpolate_from_sorted_data_array(params->depdv_lower_boundary, quad->corner[i]->pos.x)) &&
	// 			(quad->corner[i]->pos.y <= interpolate_from_sorted_data_array(params->depdv_upper_boundary, quad->corner[i]->pos.x))) points_inside_boundary = true;
	// 	}
	//
	// 	if(!points_inside_boundary && !is_quad_crossed_by_line(quad, params->depdv_lower_boundary) && !is_quad_crossed_by_line(quad, params->depdv_upper_boundary)) {
	// 		free_quad(quad, true); return 0;
	// 	}
	// }

	bool is_croosed_by_bounds = is_quad_crossed_by_line(quad, group->group_bdr.upper_bdrs[0]) || is_quad_crossed_by_line(quad, group->group_bdr.lower_bdrs[0]);

	int num_splits = 0;
	if(!is_quad_flag(quad, QUAD_FLAG_IS_LEAF) || is_croosed_by_bounds) {
		if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF) && quad->rf_level < max_rf_level) num_splits += split_quad(quad, point_func, NULL);
		for(int i = 0; i < 4; i++) {
			if(quad->subquads[i]) {
				num_splits += match_to_boundary(quad->subquads[i], params_p, point_func, max_rf_level);
			}
		}
	}

	return num_splits;
}

G_MODULE_EXPORT void on_calc_ir() {
	TimingMeasurements tm = init_timing_measurements();
	char *string;

	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_mindepdate));
	double min_dep = convert_date_JD(date_from_string(string, DATE_ISO));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdepdate));
	double max_dep = convert_date_JD(date_from_string(string, DATE_ISO));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_mindur));
	double min_dur = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdur));
	double max_dur = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_tolerance));
	double tolerance = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_numdeps));
	double target_numdeps = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdv));
	double max_depdv = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup0));
	int pcgroup0 = (int) strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup1));
	int pcgroup1 = (int) strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup2));
	int pcgroup2 = (int) strtod(string, NULL);

	start_time_measurement(&tm);

	Body *dep_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_depbody))];
	Body *arr_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_arrbody))];
	Body *body_tf = get_body_by_name("Mars", ir_system);

	double dep_periapsis = dep_body->atmo_alt + ir_dep_periapsis;

	int num_iterations = (int) target_numdeps;

	DepartureGroup departure;
	departure.dep_body = dep_body;
	departure.num_next_groups = 0;
	departure.group_cap = 8;
	departure.segment_groups = malloc(departure.group_cap * sizeof(SegmentGroup *));

	// for loop to be exchanged with some sort of boundary check
	for(int i = 0; i < 50; i++) {
		SegmentGroup *new_group = malloc(sizeof(SegmentGroup));
		new_group->dep_body = dep_body;
		new_group->arr_body = arr_body;
		// new_group->num_steps = 0;
		new_group->system = ir_system;
		new_group->num_next_groups = 0;
		new_group->group_cap = 0;
		new_group->next = NULL;
		new_group->prev = NULL;
		new_group->vinf_array = NULL;
		new_group->group_bdr = create_new_boundary();
		new_group->dv_bdr = create_new_boundary();
		new_group->vinf_bdr = create_new_boundary();
		new_group->rpe_bdr = create_new_boundary();
		new_group->quad = NULL;
		new_group->min_rf_level = 0;
		new_group->max_rf_level = 0;
		set_opposition_conjunction_group_boundary(new_group, i-10, min_dep, max_dep, min_dur, max_dur);

		// calc_group_porkchop(new_group, num_iterations, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_depdv, tolerance);
		if(data_array2_get_max(new_group->group_bdr.upper_bdrs[0]).y >= min_dur &&
			data_array2_get_min(new_group->group_bdr.lower_bdrs[0]).y <= max_dur) {
			if(departure.num_next_groups == departure.group_cap) {
				departure.group_cap *= 2;
				SegmentGroup **temp_groups = realloc(departure.segment_groups, departure.group_cap * sizeof(SegmentGroup *));
				if(temp_groups) departure.segment_groups = temp_groups;
			}
			departure.segment_groups[departure.num_next_groups++] = new_group;
		} else {
			free_boundary(&new_group->group_bdr);
			free(new_group);
		}
	}
	printf("Number of Departure Groups: %d\n\n", departure.num_next_groups);

	end_time_measurement(&tm, "Porkchopping Departure Groups");
	start_time_measurement(&tm);

	for(int i = 0; i < departure.num_next_groups; i++) {
		SegmentGroup *group = departure.segment_groups[i];
		set_dep_group_dv_boundary(group, num_iterations, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_depdv, 1);
		if(group->dv_bdr.num == 0) {
			free_boundary(&group->group_bdr);
			free_boundary(&group->dv_bdr);
			free_boundary(&group->vinf_bdr);
			free_boundary(&group->rpe_bdr);
			free(group);
			memmove(&departure.segment_groups[i],
				&departure.segment_groups[i+1],
				(departure.num_next_groups - (i+1)) * sizeof(SegmentGroup*));
			departure.num_next_groups--;
			i--;
		}
	}

	end_time_measurement(&tm, "DV Boundary");
	start_time_measurement(&tm);


	for(int i = 0; i < departure.num_next_groups; i++) {
		SegmentGroup *group = departure.segment_groups[i];
		double quad_min_dep = min_dep;
		double quad_max_dep = max_dep;
		double quad_min_dur = min_dur;
		double quad_max_dur = max_dur;
		double abs_grad = fabs(departure.segment_groups[pcgroup0]->boundary_gradient);
		double ratio_dur = (quad_max_dep-quad_min_dep)*abs_grad / (quad_max_dur-quad_min_dur);
		double ratio_dep = 1.0/ratio_dur;

		int min_split = (int) log2(ratio_dur > ratio_dep ? ratio_dur : ratio_dep);
		group->min_rf_level = min_split+3;


		// printf("%f  %f  %f\n", max_dep-min_dep, max_dur-min_dur, abs_grad);
		// printf("%f   %f  %f   %d\n", (max_dep-min_dep)*abs_grad, ratio_dep, ratio_dur, min_split);

		if(quad_max_dur-quad_min_dur < (quad_max_dep-quad_min_dep)*abs_grad) {
			quad_max_dur = (quad_max_dep-quad_min_dep)*abs_grad + quad_min_dur;
		} else {
			quad_max_dep = (quad_max_dur-quad_min_dur)/abs_grad + quad_min_dep;
		}
		int max_rf_level_dep = (int) (log2((quad_max_dep-quad_min_dep)/0.001)) + 1;
		int max_rf_level_dur = (int) (log2((quad_max_dur-quad_min_dur)/0.001)) + 1;
		int max_rf_level = max_rf_level_dep > max_rf_level_dur ? max_rf_level_dep : max_rf_level_dur;
		group->max_rf_level = max_rf_level;


		QuadPointFunc point_func = {test_, group};

		MeshPoint2 *p00 = point_func.func(quad_min_dep, quad_max_dur, group);
		MeshPoint2 *p01 = point_func.func(quad_max_dep, quad_max_dur, group);
		MeshPoint2 *p10 = point_func.func(quad_min_dep, quad_min_dur, group);
		MeshPoint2 *p11 = point_func.func(quad_max_dep, quad_min_dur, group);

		group->quad = create_quad_from_four_points(NULL, p00, p01, p10, p11, &point_func);
	}

	end_time_measurement(&tm, "Quad generation");
	start_time_measurement(&tm);
	// if(false) {
	//
	// 	ErrorFuncParams err_func_params = {
	// 		.max_error = tolerance/2,
	// 		.val_idx = MESH_VAL_VINF
	// 	};
	// 	BoundaryFuncParams bound_func_params = {.group = group};
	// 	QuadBoundsFunc bounds_func = {quad_test_is_in_bounds_function, &bound_func_params};
	// 	QuadErrorFunc error_func = {quad_test_error_function, &err_func_params};
	// }

	for(int idx = 0; idx < departure.num_next_groups; idx++) {
		SegmentGroup *group = departure.segment_groups[idx];
		BoundaryFuncParams bound_func_params = {.group = group};
		match_to_boundary(group->quad, &bound_func_params, NULL, group->max_rf_level+5);
	}

	end_time_measurement(&tm, "Boundary matching");
	start_time_measurement(&tm);

	for(int idx = 0; idx < departure.num_next_groups; idx++) {
		SegmentGroup *group = departure.segment_groups[idx];
		QuadPointPopFunc point_pop_func = {test_populate, group};
		populate_quad_mesh_points(group->quad, &point_pop_func);
	}

	end_time_measurement(&tm, "Populate Quads");

	start_time_measurement(&tm);


	for(int idx = 0; idx < departure.num_next_groups; idx++) {
		SegmentGroup *group = departure.segment_groups[idx];

		ErrorFuncParams err_func_params = {
			.max_error = tolerance/2,
			.val_idx = MESH_VAL_VINF
		};
		BoundaryFuncParams bound_func_params = {.group = group};
		QuadBoundsFunc bounds_func = {quad_test_is_in_bounds_function, &bound_func_params};
		QuadErrorFunc error_func = {quad_test_error_function, &err_func_params};
		QuadPointFunc point_func = {test_, group};

		int num_split_cycles = 0;

		QuadList *quad_list = create_quad_list();
		QuadList *quad_split_list = create_quad_list();

		get_quad_leaves(group->quad, quad_list);

		for(int i = 0; i < 100; i++) {
			num_split_cycles++;
			for(int j = 0; j < quad_list->num; j++) {
				update_quad_error_flag(quad_list->quad[j], group->min_rf_level, group->max_rf_level, &error_func);
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
			if(num_splits == 0) break;
		}

		free_quad_list(quad_list);
		free_quad_list(quad_split_list);
		printf("Num Split Cycles: %d\n", num_split_cycles);
		printf("Num of Leaves: %d\n", get_quad_leaves(group->quad, NULL));
	}

	end_time_measurement(&tm, "Divide & Conquer");
	start_time_measurement(&tm);

	for(int idx = 0; idx < departure.num_next_groups; idx++) {
		SegmentGroup *group = departure.segment_groups[idx];

		group->num_next_groups = 0;
		group->group_cap = 8;
		group->next = malloc(group->group_cap * sizeof(SegmentGroup *));
		Vector3 quad_min = get_quad_min_values(group->quad, MESH_VAL_DATE);
		Vector3 quad_max = get_quad_max_values(group->quad, MESH_VAL_DATE);

		min_dep = quad_min.z;
		max_dep = quad_max.z;
		min_dur = 90;
		max_dur = 500;

		// for loop to be exchanged with some sort of boundary check
		for(int i = 0; i < 50; i++) {
			SegmentGroup *new_group = malloc(sizeof(SegmentGroup));
			new_group->dep_body = arr_body;
			new_group->arr_body = body_tf;
			new_group->system = ir_system;
			new_group->num_next_groups = 0;
			new_group->group_cap = 0;
			new_group->next = NULL;
			new_group->prev = NULL;
			new_group->vinf_array = NULL;
			new_group->group_bdr = create_new_boundary();
			new_group->dv_bdr = create_new_boundary();
			new_group->vinf_bdr = create_new_boundary();
			new_group->rpe_bdr = create_new_boundary();
			set_opposition_conjunction_group_boundary(new_group, i-10, min_dep, max_dep, min_dur, max_dur);

			if(data_array2_get_max(new_group->group_bdr.upper_bdrs[0]).y >= 90 &&
				data_array2_get_min(new_group->group_bdr.lower_bdrs[0]).y <= 700) {
				if(group->num_next_groups == group->group_cap) {
					group->group_cap *= 2;
					SegmentGroup **temp_groups = realloc(group->next, group->group_cap * sizeof(SegmentGroup *));
					if(temp_groups) group->next = temp_groups;
				}
				group->next[group->num_next_groups++] = new_group;
				} else {
					free_boundary(&new_group->group_bdr);
					free(new_group);
				}
		}
		printf("Number of Departure Groups: %d\n\n", group->num_next_groups);
	}

	end_time_measurement(&tm, "Porkchopping Departure Groups");
	start_time_measurement(&tm);

	for(int idx = 0; idx < departure.num_next_groups; idx++) {
		SegmentGroup *group = departure.segment_groups[idx];
		Vector3 quad_min = get_quad_min_values(group->quad, MESH_VAL_DATE);
		Vector3 quad_max = get_quad_max_values(group->quad, MESH_VAL_DATE);

		min_dep = quad_min.z;
		max_dep = quad_max.z;
		min_dur = 90;
		max_dur = 500;

		for(int i = 0; i < group->num_next_groups; i++) {
			group->next[i]->vinf_array = calc_min_vinf_line(group->next[i], min_dep, max_dep, min_dur, max_dur, dep_periapsis, max_depdv, 1);
		}

	}

	end_time_measurement(&tm, "Vinf Line");
	start_time_measurement(&tm);


	SegmentGroup *group = departure.segment_groups[pcgroup0];

	// DataArray2 *array = calc_vinf_boundary(first_transfer.segment_groups[pcgroup1], quad, vinf_array, tolerance);
	// for(int i = 0; i < group->num_next_groups; i++) {
	for(int i = pcgroup1; i < pcgroup1+1; i++) {
		SegmentGroup *next_group = group->next[i];
		calc_vinf_boundary(group, next_group, group->quad, next_group->vinf_array, 1);


		ErrorFuncParams err_func_params = {
			.max_error = 10,
			.val_idx = MESH_VAL_VINF
		};
		BoundaryFuncParams bound_func_params = {.group = group};
		QuadBoundsFunc bounds_func = {quad_test_is_in_bounds_function, &bound_func_params};
		QuadErrorFunc error_func = {quad_test_error_function, &err_func_params};
		QuadPointFunc point_func = {test_, group};
		int num_split_cycles = 0;

		QuadList *quad_list = create_quad_list();
		QuadList *quad_split_list = create_quad_list();

		double dv_tol = 10;
		bool last_was_0 = false;

		for(int c = 0; c < 30; c++) {
			num_split_cycles++;
			err_func_params.max_error = dv_tol/2;

			if(c == 0 || last_was_0) {
				// for(int j = 0; j < next_group->vinf_bdr.num; j++) {
				// 	find_line_crossed_quads(group->quad, vinf_boundary, quad_list);
				// }
				get_quad_leaves(group->quad, quad_list);
			}
			for(int j = 0; j < quad_list->num; j++) {
				if(!is_quad_crossed_by_boundary(quad_list->quad[j], next_group->vinf_bdr)) {
					remove_from_quad_list_at_idx(quad_list, j);
					j--;
				}
			}

			for(int j = 0; j < quad_list->num; j++) {
				update_quad_error_flag(quad_list->quad[j], group->min_rf_level, group->max_rf_level, &error_func);
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
				calc_vinf_boundary(group, next_group, group->quad, next_group->vinf_array, 1);
			} else last_was_0 = false;
		}

		free_quad_list(quad_list);
		free_quad_list(quad_split_list);
		printf("%lu\n", group->next[i]->vinf_bdr.num);
		if(group->next[i]->vinf_bdr.num == 0) {
			free_boundary(&group->next[i]->group_bdr);
			free_boundary(&group->next[i]->dv_bdr);
			free_boundary(&group->next[i]->vinf_bdr);
			free_boundary(&group->next[i]->rpe_bdr);
			free(group->next[i]);
			memmove(&group->next[i], &group->next[i+1],
				(group->num_next_groups - (i+1)) * sizeof(SegmentGroup*));
			group->num_next_groups--;
			i--;
		}
	}
	end_time_measurement(&tm, "vinf_boundary");
	start_time_measurement(&tm);

	Boundary new_boundary = combine_boundaries(group->dv_bdr, group->next[pcgroup1]->vinf_bdr);


	end_time_measurement(&tm, "Combine DV Vinf Boundaries");

	for(int j = 0; j < new_boundary.num; j++) {
		printf("%d ----\n", j);
		print_data_array2(new_boundary.lower_bdrs[j], "dep", "dur");
		print_data_array2(new_boundary.upper_bdrs[j], "dep", "dur");
		plot_scatter_data2(ir_coord_sys0, new_boundary.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
		plot_scatter_data2(ir_coord_sys0, new_boundary.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	}
	for(int j = 0; j < new_boundary.num; j++) {
		plot_data2(ir_coord_sys1, new_boundary.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
		plot_data2(ir_coord_sys1, new_boundary.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	}

	// for(int j = 0; j < group->dv_bdr.num; j++) {
	// 	plot_data2(ir_coord_sys1, group->dv_bdr.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// 	plot_data2(ir_coord_sys1, group->dv_bdr.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// }
	// for(int i = 0; i < group->num_next_groups; i++) {
	// 	for(int j = 0; j < group->next[i]->vinf_bdr.num; j++) {
	// 		plot_data2(ir_coord_sys1, group->next[i]->vinf_bdr.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// 		plot_data2(ir_coord_sys1, group->next[i]->vinf_bdr.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// 	}
	// }
	//
	// attach_quad_to_coordinate_system(ir_coord_sys0, group->quad, CS_PLOT_TYPE_QUAD_SKELETON, CS_AXIS_DATE, CS_AXIS_NUMBER, TRUE, MESH_VAL_VINF, FALSE);

	// start_time_measurement(&tm);
	//
	// Mesh2 *mesh = create_mesh_from_quads(quad);
	//
	// end_time_measurement(&tm, "Creating Mesh");
	//
	//
	// for(int j = 0; j < group->dv_bdr.num; j++) {
	// 	plot_data2(ir_coord_sys0, group->dv_bdr.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// 	plot_data2(ir_coord_sys0, group->dv_bdr.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// }
	//
	// for(int j = 0; j < group->next[pcgroup1]->vinf_bdr.num; j++) {
	// 	plot_data2(ir_coord_sys0, group->next[pcgroup1]->vinf_bdr.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// 	plot_data2(ir_coord_sys0, group->next[pcgroup1]->vinf_bdr.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// }

	// for(int idx = 0; idx < departure.num_next_groups; idx++) {
	// 	SegmentGroup *group = departure.segment_groups[idx];
	// 	for(int j = 0; j < group->num_next_groups; j++) {
	// 		SegmentGroup *next_group = group->next[j];
	// 		plot_data2(ir_coord_sys0, next_group->vinf_array, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// 	}
	// }


	// for(int idx = 0; idx < departure.num_next_groups; idx++) {
	// 	SegmentGroup *group = departure.segment_groups[idx];
	// 	printf("idx: %d\n", idx);
	//
	// 	attach_quad_to_coordinate_system(ir_coord_sys0, group->quad, CS_PLOT_TYPE_QUAD_SKELETON, CS_AXIS_DATE, CS_AXIS_NUMBER, TRUE, MESH_VAL_VINF, FALSE);
	// 	attach_quad_to_coordinate_system(ir_coord_sys1, group->quad, CS_PLOT_TYPE_QUAD_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_NUMBER, FALSE, MESH_VAL_VINF, FALSE);
	// 	for(int j = 0; j < group->dv_bdr.num; j++) {
	// 		plot_data2(ir_coord_sys0, group->dv_bdr.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// 		plot_data2(ir_coord_sys0, group->dv_bdr.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// 	}
	// }

	// attach_quad_to_coordinate_system(ir_coord_sys0, quad, CS_PLOT_TYPE_QUAD_DEBUG, CS_AXIS_DATE, CS_AXIS_NUMBER, TRUE, MESH_VAL_VINF, TRUE);
	// attach_quad_to_coordinate_system(ir_coord_sys0, quad, CS_PLOT_TYPE_QUAD_SKELETON, CS_AXIS_DATE, CS_AXIS_NUMBER, TRUE, MESH_VAL_VINF, TRUE);
	// attach_quad_to_coordinate_system(ir_coord_sys0, quad, CS_PLOT_TYPE_QUAD_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_NUMBER, TRUE, MESH_VAL_VINF, TRUE);
	// attach_mesh_to_coordinate_system(ir_coord_sys1, mesh, CS_PLOT_TYPE_MESH_TRIANGLE_DEBUG, CS_AXIS_DATE, CS_AXIS_NUMBER, TRUE, MESH_VAL_VINF, TRUE);
	// attach_mesh_to_coordinate_system(ir_coord_sys0, mesh, CS_PLOT_TYPE_MESH_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_NUMBER, TRUE, MESH_VAL_VINF, TRUE);
	// attach_mesh_to_coordinate_system(ir_coord_sys0, mesh, CS_PLOT_TYPE_MESH_SKELETON, CS_AXIS_DATE, CS_AXIS_NUMBER, FALSE, MESH_VAL_VINF, TRUE);
	// plot_data2(ir_coord_sys0, line, CS_AXIS_NUMBER, CS_AXIS_NUMBER, false);
	// plot_data2(ir_coord_sys0, departure.segment_groups[pcgroup0]->upper_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// plot_data2(ir_coord_sys0, departure.segment_groups[pcgroup0]->lower_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// plot_data2(ir_coord_sys1, depdv_lower_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// plot_data2(ir_coord_sys1, depdv_upper_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// scatter_data2(ir_coord_sys0, error_array, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// plot_scatter_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->upper_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// plot_scatter_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->lower_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);

	// plot_scatter_data2(ir_coord_sys1, array, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// plot_data2(ir_coord_sys0, array, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// plot_data2(ir_coord_sys0, vinf_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// plot_data2(ir_coord_sys1, vinf_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// plot_scatter_data2(ir_coord_sys1, array1, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// plot_scatter_data2(ir_coord_sys1, array2, CS_AXIS_NUMBER, CS_AXIS_NUMBER, false);
	// plot_scatter_data2(ir_coord_sys0, first_transfer.segment_groups[pcgroup1]->upper_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// plot_scatter_data2(ir_coord_sys0, first_transfer.segment_groups[pcgroup1]->lower_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);

	print_timing_measurements(tm);
	free_timing_measurements(&tm);
}


G_MODULE_EXPORT void on_calc_ir_band_shenanigans() {
	TimingMeasurements tm = init_timing_measurements();
	char *string;

	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_mindepdate));
	double min_dep = convert_date_JD(date_from_string(string, DATE_ISO));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdepdate));
	double max_dep = convert_date_JD(date_from_string(string, DATE_ISO));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_mindur));
	double min_dur = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdur));
	double max_dur = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_tolerance));
	double tolerance = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_numdeps));
	double target_numdeps = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdv));
	double max_depdv = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup0));
	int pcgroup0 = (int) strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup1));
	int pcgroup1 = (int) strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup2));
	int pcgroup2 = (int) strtod(string, NULL);

	// start_time_measurement(&tm);
	//
	// DataArray2 *pos = data_array2_create();
	//
	// for(int i = -50; i < 300; i+=5) data_array2_append_new(pos, -2, i);
	// for(int i = -20; i < 250; i+=5) data_array2_append_new(pos, -1, i);
	// for(int i = 0; i < 200; i+=5) data_array2_append_new(pos, 0, i);
	// for(int i = 0; i < 70; i+=5) data_array2_append_new(pos, 1, i);
	// for(int i = 150; i < 200; i+=5) data_array2_append_new(pos, 1, i);
	// for(int i = 0; i < 5; i+=5) data_array2_append_new(pos, 2, i);
	// for(int i = 170; i < 200; i+=5) data_array2_append_new(pos, 2, i);
	// for(int i = 0; i < 15; i+=5) data_array2_append_new(pos, 3, i);
	// for(int i = 165; i < 200; i+=5) data_array2_append_new(pos, 3, i);
	// for(int i = 0; i < 50; i+=5) data_array2_append_new(pos, 4, i);
	// for(int i = 120; i < 200; i+=5) data_array2_append_new(pos, 4, i);
	// for(int i = 0; i < 200; i+=5) data_array2_append_new(pos, 5, i);
	// for(int i = -10; i < 220; i+=5) data_array2_append_new(pos, 6, i);
	// for(int i = 40; i < 300; i+=5) data_array2_append_new(pos, 7, i);
	// for(int i = -60; i < 100; i+=5) data_array2_append_new(pos, 8, i);
	//
	// MeshGrid2 *test_grid = create_mesh_grid(pos, NULL, 0);
	// end_time_measurement(&tm, "Grid");
	// start_time_measurement(&tm);
	// Mesh2 *test_mesh = create_mesh_from_grid_delaunay(test_grid);
	// end_time_measurement(&tm, "Delaunay");
	//
	//
	// attach_mesh_to_coordinate_system(ir_coord_sys0, test_mesh, CS_PLOT_TYPE_MESH_TRIANGLE_DEBUG, CS_AXIS_NUMBER, CS_AXIS_NUMBER, TRUE, 0, TRUE);
	// attach_mesh_to_coordinate_system(ir_coord_sys0, test_mesh, CS_PLOT_TYPE_MESH_SKELETON, CS_AXIS_NUMBER, CS_AXIS_NUMBER, FALSE, 0, FALSE);
	// print_timing_measurements(tm);
	// free_timing_measurements(&tm);
	// return;


	Body *dep_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_depbody))];
	Body *arr_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_arrbody))];

	double dep_periapsis = dep_body->atmo_alt + ir_dep_periapsis;

	int num_iterations = (int) target_numdeps;

	start_time_measurement(&tm);

	DepartureGroup departure;
	departure.dep_body = dep_body;
	departure.num_next_groups = 0;
	departure.group_cap = 8;
	departure.segment_groups = malloc(departure.group_cap * sizeof(SegmentGroup *));


	// for loop to be exchanged with some sort of boundary check
	for(int i = 0; i < 50; i++) {
		SegmentGroup *new_group = malloc(sizeof(SegmentGroup));
		new_group->dep_body = dep_body;
		new_group->arr_body = arr_body;
		new_group->num_steps = 0;
		new_group->system = ir_system;
		new_group->num_next_groups = 0;
		new_group->group_cap = 0;
		new_group->next = NULL;
		new_group->prev = NULL;
		new_group->vinf_array = NULL;
		set_opposition_conjunction_group_boundary(new_group, i-10, min_dep, max_dep, min_dur, max_dur);

		calc_group_porkchop(new_group, num_iterations, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_depdv, tolerance/10);
		// DataArray2 *boundary_array = calc_dv_boundary(new_group, num_iterations, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_depdv, tolerance/10);
		// new_group->vinf_array = boundary_array;
		if(new_group->num_steps != 0) {
		// if(data_array2_size(boundary_array) != 0) {
			if(departure.num_next_groups == departure.group_cap) {
				departure.group_cap *= 2;
				SegmentGroup **temp_groups = realloc(departure.segment_groups, departure.group_cap * sizeof(SegmentGroup *));
				if(temp_groups) departure.segment_groups = temp_groups;
			}
			departure.segment_groups[departure.num_next_groups++] = new_group;
		} else free(new_group);
	}
	printf("Number of Departure Groups: %d\n\n", departure.num_next_groups);

	end_time_measurement(&tm, "Porkchopping Departure Groups");
	start_time_measurement(&tm);

	for(int group_idx = 0; group_idx < departure.num_next_groups; group_idx++) {
		double **step_vals = malloc(10000000*sizeof(double *));
		DataArray2 *step_pos = data_array2_create();
		int counter = 0;
		SegmentGroup *group = departure.segment_groups[group_idx];
		for(int i = 0; i < group->num_steps; i++) {
			struct ItinStep *step = group->segment_steps[i];
			if(!step) {
				data_array2_append_new(step_pos, NAN, NAN);
				step_vals[counter] = NULL;
				counter++;
				continue;
			}

			for(int j = 0; j < step->num_next_nodes; j++) {
				double x = step->date;
				double y = step->next[j]->date - step->date;

				data_array2_append_new(step_pos, x, y);
				step_vals[counter] = step_to_array(step->next[j]);
				counter++;
			}
		}
		MeshGrid2 *grid = create_mesh_grid(step_pos, step_vals, NUM_PORKCHOP_MESH_VALUE_TYPES);

		DataArray2 *data = data_array2_create();
		// for(int threshold = 3000; threshold <= max_depdv; threshold+=500) {
			int threshold = pcgroup2;
			DataArray2 *threshold_high_dur = data_array2_create();
			DataArray2 *threshold_low_dur = data_array2_create();
			for(int i = 0; i < grid->num_cols; i++) {
				double prev_val = 2*max_depdv;
				double prev_dur = grid->points[i][0]->pos.y - 1E10;
				double high_dur = -1, low_dur = -1;
				for(int j = 0; j < grid->num_col_rows[i]; j++) {
					double val = grid->points[i][j]->val[MESH_VAL_VINF];
					double dur = grid->points[i][j]->pos.y;
					if(prev_val > threshold && val <= threshold) {
						double x0 = prev_dur;
						double x1 = dur;
						double y0 = prev_val;
						double y1 = val;

						double m = (y1-y0)/(x1-x0);
						double n = y1-m*x1;
						high_dur = (threshold-n)/m;
					} else if(prev_val < threshold && val >= threshold) {
						double x0 = prev_dur;
						double x1 = dur;
						double y0 = prev_val;
						double y1 = val;

						double m = (y1-y0)/(x1-x0);
						double n = y1-m*x1;
						low_dur = (threshold-n)/m;
					}
					prev_val = val;
					prev_dur = dur;
				}

				size_t high_size = data_array2_size(threshold_high_dur);
				size_t low_size = data_array2_size(threshold_low_dur);
				if(high_dur > 0 || (high_size > 0 && data_array2_get_data(threshold_high_dur)[high_size-1].y > 0))
					data_array2_append_new(threshold_high_dur, grid->points[i][0]->pos.x, high_dur);
				if(low_dur > 0 || (low_size > 0 && data_array2_get_data(threshold_low_dur)[low_size-1].y > 0))
					data_array2_append_new(threshold_low_dur, grid->points[i][0]->pos.x, low_dur);
			}
		// }

		printf("%ld\n", data_array2_size(threshold_high_dur));
		printf("%ld\n", data_array2_size(threshold_low_dur));

		for(int k = 0; k < data_array2_size(threshold_high_dur); k++) {
			double x = data_array2_get_data(threshold_high_dur)[k].x;
			double v = data_array2_get_data(threshold_high_dur)[k].y;
			if(v > 0) data_array2_append_new(data, x, v);
		}
		for(int k = data_array2_size(threshold_low_dur)-1; k > 0; k--) {
			double x = data_array2_get_data(threshold_low_dur)[k].x;
			double v = data_array2_get_data(threshold_low_dur)[k].y;
			if(v > 0) data_array2_append_new(data, x, v);
		}
		size_t data_size = data_array2_size(data);
		if(data_size > 0) data_array2_append_new(data, data_array2_get_data(data)[0].x, data_array2_get_data(data)[0].y);

		group->vinf_array = data;


		group->mesh = create_mesh_from_grid_w_angled_guideline(grid, group->boundary_gradient);
		group->mesh = create_mesh_from_grid_wrt_value(grid, MESH_VAL_VINF);
		free_grid_keep_points(grid);
		data_array2_free(step_pos);
		free(step_vals);
	}


	plot_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->vinf_array, CS_AXIS_DATE, CS_AXIS_NUMBER, FALSE);

	for(int i = 0; i < departure.num_next_groups; i++) free_segment_group(departure.segment_groups[i]);
	free(departure.segment_groups);
	print_timing_measurements(tm);
	free_timing_measurements(&tm);
	return;

	end_time_measurement(&tm, "Building Departure Meshes");
	start_time_measurement(&tm);

	update_mesh_triangle_status(departure.segment_groups[pcgroup0], tolerance);

	end_time_measurement(&tm, "Update Triangle Status");

	attach_mesh_to_coordinate_system(ir_coord_sys0, departure.segment_groups[pcgroup0]->mesh, CS_PLOT_TYPE_MESH_SKELETON, CS_AXIS_DATE, CS_AXIS_DURATION, TRUE, MESH_VAL_VINF, TRUE);
	attach_mesh_to_coordinate_system(ir_coord_sys1, departure.segment_groups[pcgroup0]->mesh, CS_PLOT_TYPE_MESH_TRIANGLE_DEBUG, CS_AXIS_DATE, CS_AXIS_DURATION, TRUE, MESH_VAL_VINF, TRUE);
	// plot_scatter_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->next[pcgroup1]->lower_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, TRUE);
	// plot_scatter_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->next[pcgroup1]->upper_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, FALSE);

	for(int i = 0; i < departure.num_next_groups; i++) free_segment_group(departure.segment_groups[i]);
	free(departure.segment_groups);
	print_timing_measurements(tm);
	free_timing_measurements(&tm);
}




void draw_mesh_interpolated_points_error(cairo_t *cr, double width, double height, Mesh2 *mesh, double tolerance) {
	double step_dep = 0.5;
	double step_dur = 0.5;
	DataArray3 *absolute_error = data_array3_create();
	DataArray3 *relative_error = data_array3_create();
	DataArray2 *error_pos = data_array2_create();
	data_array2_append_new(error_pos, 1e9, 1e9);
	data_array2_append_new(error_pos, -1e9, -1e9);

	int num_points = 0, num_errors = 0;

	for(int i = 0; i < mesh->num_triangles; i++) {
		Vector2 min, max;
		MeshTriangle2 tri2d = *mesh->triangles[i];
		find_2dtriangle_minmax(&tri2d, &min, &max);

		for(double jd_dep = min.x; jd_dep <= max.x; jd_dep += step_dep) {
			for(double dur = min.y; dur <= max.y; dur += step_dur) {
				Vector2 p = vec2(jd_dep, dur);
				if(is_inside_triangle(&tri2d, p)) {
					Vector3 tri3[3];
					struct ItinStep *ptr = NULL;
					for(int idx = 0; idx < 3; idx++) {
						ptr = mesh->triangles[i]->points[idx]->old_data;
						double vinf = mag_vec3(subtract_vec3(ptr->v_dep, ptr->prev->v_body));
						double dv_dep = dv_circ(ptr->prev->body, ir_dep_periapsis+ptr->prev->body->radius, vinf);

						tri3[idx].x = mesh->triangles[i]->points[idx]->pos.x;
						tri3[idx].y = mesh->triangles[i]->points[idx]->pos.y;
						tri3[idx].z = dv_dep;
					}
					double interpl_value = get_triangle_interpolated_value(tri3[0], tri3[1], tri3[2], p);

					Body *dep_body = ptr->prev->body;
					Body *arr_body = ptr->body;
					CelestSystem *system = ptr->body->orbit.cb->system;

					OSV osv0 = system->prop_method == ORB_ELEMENTS ?
						           osv_from_elements(dep_body->orbit, jd_dep) :
						           osv_from_ephem(dep_body->ephem, dep_body->num_ephems, jd_dep, system->cb);

					double jd_arr = jd_dep + dur;

					OSV osv1 = system->prop_method == ORB_ELEMENTS ?
								osv_from_elements(arr_body->orbit, jd_arr) :
								osv_from_ephem(arr_body->ephem, arr_body->num_ephems, jd_arr, system->cb);

					Lambert3 tf = calc_lambert3(osv0.r, osv1.r, (jd_arr - jd_dep) * 86400, system->cb);
					double vinf = mag_vec3(subtract_vec3(tf.v0, osv0.v));
					double dv_dep = dv_circ(ptr->prev->body, ir_dep_periapsis+ptr->prev->body->radius, vinf);

					num_points++;
					Vector2 *error_data = data_array2_get_data(error_pos);
					if(jd_dep-2.43418e+06 < error_data[0].x) error_data[0].x = jd_dep-2.43418e+06;
					if(dur < error_data[0].y) error_data[0].y = dur;
					if(jd_dep-2.43418e+06 > error_data[1].x) error_data[1].x = jd_dep-2.43418e+06;
					if(dur > error_data[1].y) error_data[1].y = dur;
					if(fabs(dv_dep-interpl_value) > tolerance) {
						num_errors++;
						data_array2_append_new(error_pos, jd_dep-2.43418e+06, dur);
						// data_array3_append_new(absolute_error, jd_dep-2.43418e+06, dur, fabs(dv_dep-interpl_value));
						data_array3_append_new(absolute_error, jd_dep, dur, fabs(dv_dep-interpl_value));
						data_array3_append_new(relative_error, jd_dep-2.43418e+06, dur, fabs(dv_dep-interpl_value)/dv_dep);
					}
				}
			}
		}
	}
	// print_data_array3(relative_error, "dep", "dur", "rel_error");
	// print_data_array3(error, "dep", "dur", "error");
	printf(" %d / %d   (%.4f %%)\n", num_errors, num_points, (num_errors/(double)num_points)*100);

	scatter_data3(ir_coord_sys1, absolute_error, CS_AXIS_DATE, CS_AXIS_DURATION, true);
	draw_scatter_from_data_array(cr, width, height, error_pos);
}

G_MODULE_EXPORT void on_calc_ir2() {
	TimingMeasurements tm = init_timing_measurements();
	char *string;

	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_mindepdate));
	double min_dep = convert_date_JD(date_from_string(string, DATE_ISO));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdepdate));
	double max_dep = convert_date_JD(date_from_string(string, DATE_ISO));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_mindur));
	double min_dur = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdur));
	double max_dur = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_tolerance));
	double tolerance = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_numdeps));
	double target_numdeps = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdv));
	double max_depdv = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup0));
	int pcgroup0 = (int) strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup1));
	int pcgroup1 = (int) strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup2));
	int pcgroup2 = (int) strtod(string, NULL);

	Body *dep_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_depbody))];
	Body *arr_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_arrbody))];

	double dep_periapsis = dep_body->atmo_alt + ir_dep_periapsis;


	// Datetime date = {1964, 12, 24, 16, 44};
	// double jd_date = convert_date_JD(date);
	// DataArray2 *test1 = find_local_peak_array(jd_date, dep_body, arr_body, ir_system, 2700*86400, 2750*86400, 1, 0);
	// plot_data2(ir_coord_sys0, test1, CS_AXIS_NUMBER, CS_AXIS_NUMBER, true);
	// return;

	Datetime date = {1959, 11, 14, 7, 17};
	double jd_dep = convert_date_JD(date);
	double dt0 = 407.663626*86400;
	double dt1 = 785.960957*86400;

	DataArray2 *vinf_array = find_local_peak_array(jd_dep, dep_body, arr_body, ir_system, dt0, dt1, 1, 1);
	plot_scatter_data2(ir_coord_sys0, vinf_array, CS_AXIS_NUMBER, CS_AXIS_NUMBER, false);
	return;

	// DataArray2 *array_ = find_local_peak_array(min_dep+max_depdv, dep_body, arr_body, ir_system, min_dur*86400, max_dur*86400, 1);
	// plot_scatter_data2(ir_coord_sys0, array_, CS_AXIS_NUMBER, CS_AXIS_NUMBER, true);
	// plot_scatter_data2(ir_coord_sys1, array_, CS_AXIS_NUMBER, CS_AXIS_NUMBER, false);
	// return;


	int num_iterations = (int) target_numdeps;

	start_time_measurement(&tm);

	DepartureGroup departure;
	departure.dep_body = dep_body;
	departure.num_next_groups = 0;
	departure.group_cap = 8;
	departure.segment_groups = malloc(departure.group_cap * sizeof(SegmentGroup *));

	// for loop to be exchanged with some sort of boundary check
	for(int i = 0; i < 50; i++) {
		SegmentGroup *new_group = malloc(sizeof(SegmentGroup));
		new_group->dep_body = dep_body;
		new_group->arr_body = arr_body;
		new_group->num_steps = 0;
		new_group->system = ir_system;
		new_group->num_next_groups = 0;
		new_group->group_cap = 0;
		new_group->next = NULL;
		new_group->prev = NULL;
		new_group->vinf_array = NULL;
		set_opposition_conjunction_group_boundary(new_group, i-10, min_dep, max_dep, min_dur, max_dur);

		calc_group_porkchop(new_group, num_iterations, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_depdv, tolerance);
		if(new_group->num_steps != 0) {
			if(departure.num_next_groups == departure.group_cap) {
				departure.group_cap *= 2;
				SegmentGroup **temp_groups = realloc(departure.segment_groups, departure.group_cap * sizeof(SegmentGroup *));
				if(temp_groups) departure.segment_groups = temp_groups;
			}
			departure.segment_groups[departure.num_next_groups++] = new_group;
		} else free(new_group);
	}
	printf("Number of Departure Groups: %d\n\n", departure.num_next_groups);


	end_time_measurement(&tm, "Porkchopping Departure Groups");
	start_time_measurement(&tm);

	DataArray2 *depdv_boundary = calc_dv_boundary(departure.segment_groups[pcgroup0], num_iterations, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_depdv, tolerance/10);
	DataArray2 *depdv_lower_boundary = data_array2_create();
	DataArray2 *depdv_upper_boundary = data_array2_create();
	size_t num_points = data_array2_size(depdv_boundary);
	Vector2 *data = data_array2_get_data(depdv_boundary);

	for(int i = 0; i < num_points; i+=2) {
		data_array2_append_new(depdv_lower_boundary, data[i].x, data[i].y);
		data_array2_append_new(depdv_upper_boundary, data[i+1].x, data[i+1].y);
	}

	end_time_measurement(&tm, "DV Boundary");
	start_time_measurement(&tm);

	DataArray2 *array = calc_min_vinf_line(departure.segment_groups[pcgroup0], min_dep, max_dep, min_dur, max_dur, dep_periapsis, max_depdv, 1);

	// DataArray2 *array = data_array2_create();
	// int num_steps = 5;
	// for(int i = 0; i < num_steps; i++) {
	// 	double jd_dep = min_dep + i*(max_dep-min_dep)/num_steps;
	// 	Vector2 peak = get_local_peak(jd_dep, dep_body, arr_body, ir_system, min_dur*86400, max_dur*86400, 1, 0);
	// 	data_array2_append_new(array, jd_dep, peak.x);
	// }


	end_time_measurement(&tm, "Peaks");
	print_timing_measurements(tm);
	free_timing_measurements(&tm);

	plot_scatter_data2(ir_coord_sys0, array, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	plot_scatter_data2(ir_coord_sys1, array, CS_AXIS_DATE, CS_AXIS_NUMBER, false);

	// plot_scatter_data2(ir_coord_sys0, departure.segment_groups[pcgroup0]->upper_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// plot_scatter_data2(ir_coord_sys0, departure.segment_groups[pcgroup0]->lower_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// plot_data2(ir_coord_sys0, depdv_lower_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// plot_data2(ir_coord_sys0, depdv_upper_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// plot_data2(ir_coord_sys1, depdv_lower_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, true);
	// plot_data2(ir_coord_sys1, depdv_upper_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
}

G_MODULE_EXPORT void on_calc_ir2_tri_porkchop() {
	TimingMeasurements tm = init_timing_measurements();
	char *string;

	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_mindepdate));
	double min_dep = convert_date_JD(date_from_string(string, DATE_ISO));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdepdate));
	double max_dep = convert_date_JD(date_from_string(string, DATE_ISO));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_mindur));
	double min_dur = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdur));
	double max_dur = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_tolerance));
	double tolerance = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_numdeps));
	double target_numdeps = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdv));
	double max_depdv = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup0));
	int pcgroup0 = (int) strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup1));
	int pcgroup1 = (int) strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup2));
	int pcgroup2 = (int) strtod(string, NULL);

	Body *dep_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_depbody))];
	Body *arr_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_arrbody))];

	double dep_periapsis = dep_body->atmo_alt + ir_dep_periapsis;

	int num_iterations = (int) target_numdeps;

	start_time_measurement(&tm);

	DepartureGroup departure;
	departure.dep_body = dep_body;
	departure.num_next_groups = 0;
	departure.group_cap = 8;
	departure.segment_groups = malloc(departure.group_cap * sizeof(SegmentGroup *));


	// for loop to be exchanged with some sort of boundary check
	for(int i = 0; i < 50; i++) {
		SegmentGroup *new_group = malloc(sizeof(SegmentGroup));
		new_group->dep_body = dep_body;
		new_group->arr_body = arr_body;
		new_group->num_steps = 0;
		new_group->system = ir_system;
		new_group->num_next_groups = 0;
		new_group->group_cap = 0;
		new_group->next = NULL;
		new_group->prev = NULL;
		new_group->vinf_array = NULL;
		set_opposition_conjunction_group_boundary(new_group, i-10, min_dep, max_dep, min_dur, max_dur);

		calc_group_porkchop(new_group, num_iterations, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_depdv, tolerance);
		if(new_group->num_steps != 0) {
			if(departure.num_next_groups == departure.group_cap) {
				departure.group_cap *= 2;
				SegmentGroup **temp_groups = realloc(departure.segment_groups, departure.group_cap * sizeof(SegmentGroup *));
				if(temp_groups) departure.segment_groups = temp_groups;
			}
			departure.segment_groups[departure.num_next_groups++] = new_group;
		} else free(new_group);
	}
	printf("Number of Departure Groups: %d\n\n", departure.num_next_groups);

	end_time_measurement(&tm, "Porkchopping Departure Groups");
	start_time_measurement(&tm);

	for(int group_idx = 0; group_idx < departure.num_next_groups; group_idx++) {
		double **step_vals = malloc(100000*sizeof(double *));
		DataArray2 *step_pos = data_array2_create();
		int counter = 0;
		SegmentGroup *group = departure.segment_groups[group_idx];
		for(int i = 0; i < group->num_steps; i++) {
			struct ItinStep *step = group->segment_steps[i];
			if(!step) {
				data_array2_append_new(step_pos, NAN, NAN);
				step_vals[counter] = NULL;
				counter++;
				continue;
			}

			for(int j = 0; j < step->num_next_nodes; j++) {
				double x = step->date;
				double y = step->next[j]->date - step->date;

				data_array2_append_new(step_pos, x, y);
				step_vals[counter] = step_to_array(step->next[j]);
				counter++;
			}
		}
		MeshGrid2 *grid = create_mesh_grid(step_pos, step_vals, NUM_PORKCHOP_MESH_VALUE_TYPES);
		group->mesh = create_mesh_from_grid_w_angled_guideline(grid, group->boundary_gradient);
		free_grid_keep_points(grid);
		data_array2_free(step_pos);
		free(step_vals);
	}

	end_time_measurement(&tm, "Building Departure Meshes");
	start_time_measurement(&tm);

	update_mesh_triangle_status(departure.segment_groups[pcgroup0], tolerance);

	end_time_measurement(&tm, "Update Triangle Status");
	start_time_measurement(&tm);

	for(int group_idx = 0; group_idx < departure.num_next_groups; group_idx++) {
		SegmentGroup *group = departure.segment_groups[group_idx];
		Mesh2 *mesh = group->mesh;
		double min_fb_jd = get_mesh_min_value(mesh, MESH_VAL_DATE);
		double max_fb_jd = get_mesh_max_value(mesh, MESH_VAL_DATE);
		double max_mesh_vinf = get_mesh_max_value(mesh, MESH_VAL_VINF);
		print_date(convert_JD_date(min_fb_jd, DATE_ISO), 1);
		print_date(convert_JD_date(max_fb_jd, DATE_ISO), 1);
		// tbd something with boundary
		for(int i = -20; i < 30; i++) {
			SegmentGroup *new_group = malloc(sizeof(SegmentGroup));
			new_group->dep_body = arr_body;
			new_group->arr_body = get_body_by_name("Mars", ir_system);
			new_group->num_steps = 0;
			new_group->system = ir_system;
			new_group->num_next_groups = 0;
			new_group->group_cap = 0;
			new_group->next = NULL;
			new_group->prev = departure.segment_groups[group_idx];
			set_opposition_conjunction_group_boundary(new_group, i, min_fb_jd, max_fb_jd, min_dur, max_dur);
			// new_group->vinf_array = calc_min_vinf_line(new_group, min_fb_jd, max_fb_jd, max_dep+max_dur, min_dur, max_dur, tolerance);
			if(data_array2_size(new_group->vinf_array) == 0 || max_mesh_vinf+tolerance < data_array2_get_min(new_group->vinf_array).y) {
				data_array2_free(new_group->vinf_array);
				free(new_group);
				continue;
			}
			if(group->group_cap == 0) {
				group->group_cap = 8;
				group->next = malloc(group->group_cap * sizeof(SegmentGroup*));
			} else if(group->num_next_groups == group->group_cap) {
				group->group_cap *= 2;
				SegmentGroup **temp_next_groups = realloc(group->next, group->group_cap * sizeof(SegmentGroup*));
				if(temp_next_groups) group->next = temp_next_groups;
			}
			group->next[group->num_next_groups++] = new_group;
		}
		printf("Number of fb groups: %d\n", group->num_next_groups);
	}

	end_time_measurement(&tm, "Determine Vinf lines");

	start_time_measurement(&tm);

	// for(int group0_idx = 0; group0_idx < departure.num_next_groups; group0_idx++) {
	// 	SegmentGroup *group0 = departure.segment_groups[group0_idx];
	// 	Mesh2 *mesh = group0->mesh;
	// 	for(int group1_idx = 0; group1_idx < group0->num_next_groups; group1_idx++) {
	// 		// TODO rework get_vinf_limits wrt actual jd_dep
	// 		// DataArray2 *vinf_limits = get_vinf_limits(mesh, vinf_array, tolerance);
	// 		printf("%d  %d\n", group0_idx, group1_idx);
	// 	}
	// }

	end_time_measurement(&tm, "Combine Porkchop with vinf line");


	attach_mesh_to_coordinate_system(ir_coord_sys0, departure.segment_groups[pcgroup0]->mesh, CS_PLOT_TYPE_MESH_TRIANGLE_DEBUG, CS_AXIS_DATE, CS_AXIS_DURATION, TRUE, MESH_VAL_VINF, TRUE);
	attach_mesh_to_coordinate_system(ir_coord_sys1, departure.segment_groups[pcgroup0]->mesh, CS_PLOT_TYPE_MESH_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_DURATION, TRUE, MESH_VAL_VINF, TRUE);
	// attach_mesh_to_coordinate_system(ir_coord_sys1, departure.segment_groups[pcgroup0]->mesh, CS_PLOT_TYPE_MESH_SKELETON, CS_AXIS_DATE, CS_AXIS_DURATION, TRUE, &remove_step_from_itinerary_void_ptr, TRUE);
	// plot_scatter_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->next[pcgroup1]->vinf_array, CS_AXIS_DATE, CS_AXIS_NUMBER, TRUE);
	// plot_scatter_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->next[pcgroup1]->lower_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, TRUE);
	// plot_scatter_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->next[pcgroup1]->upper_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, FALSE);

	for(int i = 0; i < departure.num_next_groups; i++) free_segment_group(departure.segment_groups[i]);
	free(departure.segment_groups);
	print_timing_measurements(tm);
	free_timing_measurements(&tm);
}


G_MODULE_EXPORT void on_calc_ir3() {
	TimingMeasurements tm = init_timing_measurements();
	char *string;

	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_mindepdate));
	double min_dep = convert_date_JD(date_from_string(string, DATE_ISO));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdepdate));
	double max_dep = convert_date_JD(date_from_string(string, DATE_ISO));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_mindur));
	double min_dur = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdur));
	double max_dur = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_tolerance));
	double tolerance = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_numdeps));
	double target_numdeps = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdv));
	double max_depdv = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup0));
	int pcgroup0 = (int) strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup1));
	int pcgroup1 = (int) strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup2));
	int pcgroup2 = (int) strtod(string, NULL);

	start_time_measurement(&tm);

	Body *dep_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_depbody))];
	Body *arr_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_arrbody))];

	double dep_periapsis = dep_body->atmo_alt + ir_dep_periapsis;

	int num_iterations = (int) target_numdeps;

	DepartureGroup departure;
	departure.dep_body = dep_body;
	departure.num_next_groups = 0;
	departure.group_cap = 8;
	departure.segment_groups = malloc(departure.group_cap * sizeof(SegmentGroup *));

	// for loop to be exchanged with some sort of boundary check
	for(int i = 0; i < 50; i++) {
		SegmentGroup *new_group = malloc(sizeof(SegmentGroup));
		new_group->dep_body = dep_body;
		new_group->arr_body = arr_body;
		new_group->num_steps = 0;
		new_group->system = ir_system;
		new_group->num_next_groups = 0;
		new_group->group_cap = 0;
		new_group->next = NULL;
		new_group->prev = NULL;
		new_group->vinf_array = NULL;
		set_opposition_conjunction_group_boundary(new_group, i-10, min_dep, max_dep, min_dur, max_dur);

		// calc_group_porkchop(new_group, num_iterations, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_depdv, tolerance);
		if(data_array2_get_max(new_group->group_bdr.upper_bdrs[0]).y >= min_dur &&
			data_array2_get_min(new_group->group_bdr.lower_bdrs[0]).y <= max_dur) {
			if(departure.num_next_groups == departure.group_cap) {
				departure.group_cap *= 2;
				SegmentGroup **temp_groups = realloc(departure.segment_groups, departure.group_cap * sizeof(SegmentGroup *));
				if(temp_groups) departure.segment_groups = temp_groups;
			}
			departure.segment_groups[departure.num_next_groups++] = new_group;
		} else free(new_group);
	}
	printf("Number of Departure Groups: %d\n\n", departure.num_next_groups);

	end_time_measurement(&tm, "Porkchopping Departure Groups");
	start_time_measurement(&tm);

	DataArray2 *depdv_boundary = calc_dv_boundary(departure.segment_groups[pcgroup0], num_iterations, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_depdv, tolerance/10);
	DataArray2 *depdv_lower_boundary = data_array2_create();
	DataArray2 *depdv_upper_boundary = data_array2_create();
	size_t num_points = data_array2_size(depdv_boundary);
	Vector2 *data = data_array2_get_data(depdv_boundary);

	for(int i = 0; i < num_points; i+=2) {
		data_array2_append_new(depdv_lower_boundary, data[i].x, data[i].y);
		data_array2_append_new(depdv_upper_boundary, data[i+1].x, data[i+1].y);
	}

	// print_data_array2(depdv_lower_boundary, "date", "dur");
	// print_data_array2(depdv_upper_boundary, "date", "dur");


	end_time_measurement(&tm, "DV Boundary");

	start_time_measurement(&tm);

	ErrorFuncParams err_func_params = {
		.max_error = tolerance/2,
		.val_idx = MESH_VAL_VINF
	};

	BoundaryFuncParams bound_func_params = {
		.group = departure.segment_groups[pcgroup0]
	};

	double quad_min_dep = min_dep;
	double quad_max_dep = max_dep;
	double quad_min_dur = min_dur;
	double quad_max_dur = max_dur;
	double abs_grad = fabs(departure.segment_groups[pcgroup0]->boundary_gradient);
	double ratio_dur = (quad_max_dep-quad_min_dep)*abs_grad / (quad_max_dur-quad_min_dur);
	double ratio_dep = 1.0/ratio_dur;

	int min_split = (int) log2(ratio_dur > ratio_dep ? ratio_dur : ratio_dep);


	// printf("%f  %f  %f\n", max_dep-min_dep, max_dur-min_dur, abs_grad);
	// printf("%f   %f  %f   %d\n", (max_dep-min_dep)*abs_grad, ratio_dep, ratio_dur, min_split);

	if(quad_max_dur-quad_min_dur < (quad_max_dep-quad_min_dep)*abs_grad) {
		quad_max_dur = (quad_max_dep-quad_min_dep)*abs_grad + quad_min_dur;
	} else {
		quad_max_dep = (quad_max_dur-quad_min_dur)/abs_grad + quad_min_dep;
	}
	int max_rf_level_dep = (int) (log2((quad_max_dep-quad_min_dep)/0.0001)) + 1;
	int max_rf_level_dur = (int) (log2((quad_max_dur-quad_min_dur)/0.0001)) + 1;
	int max_rf_level = max_rf_level_dep > max_rf_level_dur ? max_rf_level_dep : max_rf_level_dur;

	// printf("%d  %d\n", max_rf_level_dep, max_rf_level_dur);


	QuadPointFunc point_func = {test_, departure.segment_groups[pcgroup0]};
	QuadPointPopFunc point_pop_func = {test_populate, departure.segment_groups[pcgroup0]};
	QuadBoundsFunc bounds_func = {quad_test_is_in_bounds_function, &bound_func_params};
	QuadErrorFunc error_func = {quad_test_error_function, &err_func_params};

	MeshPoint2 *p00 = point_func.func(quad_min_dep, quad_max_dur, departure.segment_groups[pcgroup0]);
	MeshPoint2 *p01 = point_func.func(quad_max_dep, quad_max_dur, departure.segment_groups[pcgroup0]);
	MeshPoint2 *p10 = point_func.func(quad_min_dep, quad_min_dur, departure.segment_groups[pcgroup0]);
	MeshPoint2 *p11 = point_func.func(quad_max_dep, quad_min_dur, departure.segment_groups[pcgroup0]);

	// MeshPoint2 *p00 = point_func.func(0, 100, NULL);
	// MeshPoint2 *p01 = point_func.func(100, 100, NULL);
	// MeshPoint2 *p10 = point_func.func(0, 0, NULL);
	// MeshPoint2 *p11 = point_func.func(100, 0, NULL);
	Quad *quad = create_quad_from_four_points(NULL, p00, p01, p10, p11, &point_func);


	end_time_measurement(&tm, "Quad generation");
	start_time_measurement(&tm);

	match_to_boundary(quad, &bound_func_params, NULL, max_rf_level+5);

	end_time_measurement(&tm, "Boundary matching");
	start_time_measurement(&tm);

	populate_quad_mesh_points(quad, &point_pop_func);

	end_time_measurement(&tm, "Populate Quads");

	start_time_measurement(&tm);

	int num_split_cycles = 0;

	for(int i = 0; i < pcgroup1; i++) {
		num_split_cycles++;
		update_quad_error_flag(quad, min_split+3, max_rf_level, &error_func);
		int num_splits = split_quads_with_flag(quad, &point_func);
		printf("Number of Splits during cycle %d: %d\n", num_split_cycles, num_splits);
		if(num_splits == 0) break;
		remove_out_of_bounds_quads(quad, &bounds_func);
	}

	printf("Num Split Cycles: %d\n", num_split_cycles);

	// printf("To Split: %d\n", update_quad_error_flag(quad, 0, error_func));
	printf("Num of Leaves: %d\n", get_quad_leaves(quad, NULL));
	end_time_measurement(&tm, "Divide & Conquer");
	// start_time_measurement(&tm);

	Mesh2 *mesh = create_mesh_from_quads(quad);

	end_time_measurement(&tm, "Creating Mesh");

	start_time_measurement(&tm);
	printf("\n-------\n");
	Vector3 min = get_quad_min_values(quad, MESH_VAL_VINF);
	Vector3 max = get_quad_max_values(quad, MESH_VAL_VINF);
	double num_checks = pcgroup2;
	DataArray2 *error_array = data_array2_create();
	DataArray3 *all_error_array = data_array3_create();
	DataArray1 *all_errors_sorted = data_array1_create();
	double err_sum = 0;

	for(int i = 0; i < num_checks; i++) {
		double x = i * (max.x - min.x) / num_checks + min.x;
		show_progress("Error Measuring", i, num_checks);
		for(int j = 0; j < num_checks; j++) {
			double y = j * (max.y - min.y) / num_checks + min.y;
			Quad *quad_at_p = get_quad_at_position(quad, vec2(x, y));
			if(!quad_at_p) continue;
			double interp_val = get_quad_interpolated_value(quad_at_p, vec2(x, y), MESH_VAL_VINF);

			MeshPoint2 *point = point_func.func(x, y, departure.segment_groups[pcgroup0]);
			double val = point->val[MESH_VAL_VINF];
			free(point->val);
			free(point);

			data_array3_append_new(all_error_array, x, y, fabs(val - interp_val));
			data_array1_insert_new(all_errors_sorted, fabs(val - interp_val));
			err_sum += fabs(val - interp_val);
			if(fabs(val - interp_val) > tolerance) {
				// print_date(convert_JD_date(x, DATE_ISO), 0);
				// printf("   %f  %f\n", y, fabs(val-interp_val));
				data_array2_append_new(error_array, x, y);
			}
		}
	}
	show_progress("Quad Error Measuring", 1, 1);
	printf("\n--\n");

	// print_data_array1(all_errors_sorted, "error");
	size_t num_err = data_array1_size(all_errors_sorted);
	double *err = data_array1_get_data(all_errors_sorted);

	printf("Min: %f\nQ1: %f\nMedian: %f\nQ3: %f\nMax: %f\nAvg: %f\n",
	       err[0], err[1 * num_err / 4], err[2 * num_err / 4], err[3 * num_err / 4], err[num_err - 1],
	       err_sum / (double) num_err);

	printf("Error Ratio: %.4f %%   (%lu/%lu)\n", (double) data_array2_size(error_array) / (double) num_err * 100,
	       data_array2_size(error_array), num_err);


	data_array3_free(all_error_array);
	data_array1_free(all_errors_sorted);
	end_time_measurement(&tm, "Quad Error Measuring");

	start_time_measurement(&tm);
	printf("\n-------\n");
	min = get_quad_min_values(quad, MESH_VAL_VINF);
	max = get_quad_max_values(quad, MESH_VAL_VINF);
	num_checks = pcgroup2;
	error_array = data_array2_create();
	all_error_array = data_array3_create();
	all_errors_sorted = data_array1_create();
	err_sum = 0;

	for(int i = 0; i < num_checks; i++) {
		double x = i * (max.x - min.x) / num_checks + min.x;
		show_progress("Error Measuring", i, num_checks);
		for(int j = 0; j < num_checks; j++) {
			double y = j * (max.y - min.y) / num_checks + min.y;
			MeshTriangle2 *triangle = get_mesh_triangle_at_position(mesh, vec2(x, y));
			if(!triangle) continue;
			Vector3 p0 = vec3(triangle->points[0]->pos.x, triangle->points[0]->pos.y, triangle->points[0]->val[MESH_VAL_VINF]);
			Vector3 p1 = vec3(triangle->points[1]->pos.x, triangle->points[1]->pos.y, triangle->points[1]->val[MESH_VAL_VINF]);
			Vector3 p2 = vec3(triangle->points[2]->pos.x, triangle->points[2]->pos.y, triangle->points[2]->val[MESH_VAL_VINF]);
			double interp_val = get_triangle_interpolated_value(p0, p1, p2, vec2(x, y));

			MeshPoint2 *point = point_func.func(x, y, departure.segment_groups[pcgroup0]);
			double val = point->val[MESH_VAL_VINF];
			free(point->val);
			free(point);

			data_array3_append_new(all_error_array, x, y, fabs(val - interp_val));
			data_array1_insert_new(all_errors_sorted, fabs(val - interp_val));
			err_sum += fabs(val - interp_val);
			if(fabs(val - interp_val) > tolerance) {
				// print_date(convert_JD_date(x, DATE_ISO), 0);
				// printf("   %f  %f\n", y, fabs(val-interp_val));
				data_array2_append_new(error_array, x, y);
			}
		}
	}
	show_progress("Mesh Error Measuring", 1, 1);
	printf("\n--\n");

	// print_data_array1(all_errors_sorted, "error");
	num_err = data_array1_size(all_errors_sorted);
	err = data_array1_get_data(all_errors_sorted);

	printf("Min: %f\nQ1: %f\nMedian: %f\nQ3: %f\nMax: %f\nAvg: %f\n",
	       err[0], err[1 * num_err / 4], err[2 * num_err / 4], err[3 * num_err / 4], err[num_err - 1],
	       err_sum / (double) num_err);

	printf("Error Ratio: %.4f %%   (%lu/%lu)\n", (double) data_array2_size(error_array) / (double) num_err * 100,
	       data_array2_size(error_array), num_err);


	data_array3_free(all_error_array);
	data_array1_free(all_errors_sorted);
	end_time_measurement(&tm, "Mesh Error Measuring");

	// attach_quad_to_coordinate_system(ir_coord_sys0, quad, CS_PLOT_TYPE_QUAD_DEBUG, CS_AXIS_DATE, CS_AXIS_NUMBER, TRUE, MESH_VAL_VINF, TRUE);
	attach_quad_to_coordinate_system(ir_coord_sys1, quad, CS_PLOT_TYPE_QUAD_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_NUMBER, FALSE, MESH_VAL_VINF, TRUE);
	// attach_mesh_to_coordinate_system(ir_coord_sys1, mesh, CS_PLOT_TYPE_MESH_TRIANGLE_DEBUG, CS_AXIS_DATE, CS_AXIS_NUMBER, TRUE, MESH_VAL_VINF, TRUE);
	attach_mesh_to_coordinate_system(ir_coord_sys0, mesh, CS_PLOT_TYPE_MESH_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_NUMBER, FALSE, MESH_VAL_VINF, TRUE);
	// plot_data2(ir_coord_sys0, line, CS_AXIS_NUMBER, CS_AXIS_NUMBER, false);
	plot_data2(ir_coord_sys0, departure.segment_groups[pcgroup0]->group_bdr.upper_bdrs[0], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	plot_data2(ir_coord_sys0, departure.segment_groups[pcgroup0]->group_bdr.lower_bdrs[0], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	plot_data2(ir_coord_sys0, depdv_lower_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	plot_data2(ir_coord_sys0, depdv_upper_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	plot_data2(ir_coord_sys1, depdv_lower_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	plot_data2(ir_coord_sys1, depdv_upper_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// scatter_data2(ir_coord_sys0, error_array, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// plot_scatter_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->upper_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// plot_scatter_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->lower_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	print_timing_measurements(tm);
	free_timing_measurements(&tm);
}



G_MODULE_EXPORT void on_calc_ir3_old_() {
	TimingMeasurements tm = init_timing_measurements();
	char *string;

	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_mindepdate));
	double min_dep = convert_date_JD(date_from_string(string, DATE_ISO));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdepdate));
	double max_dep = convert_date_JD(date_from_string(string, DATE_ISO));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_mindur));
	double min_dur = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdur));
	double max_dur = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_tolerance));
	double tolerance = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_numdeps));
	double target_numdeps = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdv));
	double max_depdv = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup0));
	int pcgroup0 = (int) strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup1));
	int pcgroup1 = (int) strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup2));
	int pcgroup2 = (int) strtod(string, NULL);

	Body *dep_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_depbody))];
	Body *arr_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_arrbody))];

	double dep_periapsis = dep_body->atmo_alt + ir_dep_periapsis;

	int num_iterations = (int) target_numdeps;

	start_time_measurement(&tm);

	DepartureGroup departure;
	departure.dep_body = dep_body;
	departure.num_next_groups = 0;
	departure.group_cap = 8;
	departure.segment_groups = malloc(departure.group_cap * sizeof(SegmentGroup *));


	// for loop to be exchanged with some sort of boundary check
	for(int i = 0; i < 50; i++) {
		SegmentGroup *new_group = malloc(sizeof(SegmentGroup));
		new_group->dep_body = dep_body;
		new_group->arr_body = arr_body;
		new_group->num_steps = 0;
		new_group->system = ir_system;
		new_group->num_next_groups = 0;
		new_group->group_cap = 0;
		new_group->next = NULL;
		new_group->prev = NULL;
		new_group->vinf_array = NULL;
		set_opposition_conjunction_group_boundary(new_group, i-10, min_dep, max_dep, min_dur, max_dur);

		calc_group_porkchop(new_group, num_iterations, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_depdv, tolerance);
		if(new_group->num_steps != 0) {
			if(departure.num_next_groups == departure.group_cap) {
				departure.group_cap *= 2;
				SegmentGroup **temp_groups = realloc(departure.segment_groups, departure.group_cap * sizeof(SegmentGroup *));
				if(temp_groups) departure.segment_groups = temp_groups;
			}
			departure.segment_groups[departure.num_next_groups++] = new_group;
		} else free(new_group);
	}
	printf("Number of Departure Groups: %d\n\n", departure.num_next_groups);

	end_time_measurement(&tm, "Porkchopping Departure Groups");
	start_time_measurement(&tm);

	for(int group_idx = 0; group_idx < departure.num_next_groups; group_idx++) {
		double **step_vals = malloc(100000*sizeof(double *));
		DataArray2 *step_pos = data_array2_create();
		int counter = 0;
		SegmentGroup *group = departure.segment_groups[group_idx];
		for(int i = 0; i < group->num_steps; i++) {
			struct ItinStep *step = group->segment_steps[i];
			if(!step) {
				data_array2_append_new(step_pos, NAN, NAN);
				step_vals[counter] = NULL;
				counter++;
				continue;
			}

			for(int j = 0; j < step->num_next_nodes; j++) {
				double x = step->date;
				double y = step->next[j]->date - step->date;

				data_array2_append_new(step_pos, x, y);
				step_vals[counter] = step_to_array(step->next[j]);
				counter++;
			}
		}
		MeshGrid2 *grid = create_mesh_grid(step_pos, step_vals, NUM_PORKCHOP_MESH_VALUE_TYPES);
		group->mesh = create_mesh_from_grid_w_angled_guideline(grid, group->boundary_gradient);
		free_grid_keep_points(grid);
		data_array2_free(step_pos);
		free(step_vals);
	}

	end_time_measurement(&tm, "Building Departure Meshes");
	start_time_measurement(&tm);

	update_mesh_triangle_status(departure.segment_groups[pcgroup0], tolerance);

	end_time_measurement(&tm, "Update Triangle Status");

	attach_mesh_to_coordinate_system(ir_coord_sys0, departure.segment_groups[pcgroup0]->mesh, CS_PLOT_TYPE_MESH_SKELETON, CS_AXIS_DATE, CS_AXIS_DURATION, TRUE, MESH_VAL_VINF, TRUE);
	attach_mesh_to_coordinate_system(ir_coord_sys1, departure.segment_groups[pcgroup0]->mesh, CS_PLOT_TYPE_MESH_TRIANGLE_DEBUG, CS_AXIS_DATE, CS_AXIS_DURATION, TRUE, MESH_VAL_VINF, TRUE);
	// plot_scatter_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->next[pcgroup1]->lower_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, TRUE);
	// plot_scatter_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->next[pcgroup1]->upper_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, FALSE);

	for(int i = 0; i < departure.num_next_groups; i++) free_segment_group(departure.segment_groups[i]);
	free(departure.segment_groups);
	print_timing_measurements(tm);
	free_timing_measurements(&tm);
}



G_MODULE_EXPORT void on_calc_ir3_old() {
	char *string;

	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_mindepdate));
	double min_dep = convert_date_JD(date_from_string(string, DATE_ISO));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdepdate));
	double max_dep = convert_date_JD(date_from_string(string, DATE_ISO));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_mindur));
	double min_dur = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdur));
	double max_dur = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_tolerance));
	double tolerance = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_numdeps));
	double target_numdeps = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdv));
	double max_dep_dv = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup0));
	int pcgroup = (int) strtod(string, NULL);

	Body *dep_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_depbody))];
	Body *arr_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_arrbody))];

	double dep_periapsis = dep_body->atmo_alt + ir_dep_periapsis;

	double jd_dep = min_dep;
	int num_iterations = (int) target_numdeps;

	struct timeval start, end;
	double elapsed_time;
	gettimeofday(&start, NULL);  // Record the ending time

	int num_of_groups = 50;
	SegmentGroup **departure_groups = malloc(num_of_groups*sizeof(SegmentGroup *));
	int counter = 0;
	for(int i = 0; i < num_of_groups; i++) {
		departure_groups[counter] = malloc(sizeof(SegmentGroup));
		departure_groups[counter]->dep_body = dep_body;
		departure_groups[counter]->arr_body = arr_body;
		departure_groups[counter]->num_steps = num_iterations;
		departure_groups[counter]->system = ir_system;

		// calc_group_porkchop(departure_groups[counter], i-10, num_iterations, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_dep_dv, tolerance);
		if(departure_groups[counter]->num_steps == 0) {free(departure_groups[counter]);}
		else counter++;
	}

	printf("%d\n", counter);

	gettimeofday(&end, NULL);  // Record the ending time
	elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
	printf("----- | Total elapsed time: %.3f s | ---------\n", elapsed_time);


	gettimeofday(&start, NULL);  // Record the ending time

	int step_cap = 1000;
	struct ItinStep **steps = malloc(step_cap*sizeof(struct ItinStep *));
	DataArray2 *step_pos = data_array2_create();
	counter = 0;
	for(int i = 0; i < departure_groups[pcgroup]->num_steps; i++) {
		struct ItinStep *step = departure_groups[pcgroup]->segment_steps[i];
		if(step->num_next_nodes == -1) {
			double x = -1e10;
			double y = 0;
			data_array2_append_new(step_pos, x, y);

			if(counter == step_cap) {
				step_cap *= 2;
				struct ItinStep **temp = realloc(steps, sizeof(struct ItinStep) * step_cap);
				if(temp) steps = temp;
			}
			steps[counter] = NULL;
			counter++;
		}
		if(step->num_next_nodes < 0) step->num_next_nodes = 0;

		for(int j = 0; j < step->num_next_nodes; j++) {
			double x = step->date;
			double y = step->next[j]->date - step->date;
			data_array2_append_new(step_pos, x, y);

			if(counter == step_cap) {
				step_cap *= 2;
				struct ItinStep **temp = realloc(steps, sizeof(struct ItinStep) * step_cap);
				if(temp) steps = temp;
			}
			steps[counter] = step->next[j];
			counter++;
		}
	}


	MeshGrid2 *grid = create_mesh_grid(step_pos, NULL, 0);
	Mesh2 *mesh = create_mesh_from_grid_w_angled_guideline(grid, departure_groups[pcgroup]->boundary_gradient);
	free_grid_keep_points(grid);


	// for(int i = 0; i < mesh.num_points; i++) {
	// 	MeshPoint2 *point = mesh.points[i];
	// 	struct ItinStep *step = point->data;
	// 	point->pos.x = step->date;
	// 	point->pos.y = step->date - get_first(step)->date;
	// }

	gettimeofday(&end, NULL);  // Record the ending time
	elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
	printf("----- | Total elapsed time: %.3f s | ---------\n", elapsed_time);


	// draw_mesh_interpolated_points_error(ir_screen0->static_layer.cr, ir_screen0->width, ir_screen0->height, mesh, tolerance);

	// resize_pcmesh_to_fit(mesh, ir_screen1->width, ir_screen1->height);
	// draw_mesh_interpolated_points(ir_screen1->static_layer.cr, mesh, ir_screen1->width, ir_screen1->height);
	// draw_mesh(ir_screen1->static_layer.cr, &mesh);

	// draw_screen(ir_screen0);
	// draw_screen(ir_screen1);
	free_mesh(mesh);
}
