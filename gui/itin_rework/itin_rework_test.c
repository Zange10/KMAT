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

MeshPoint2 * test_(double jd_dep, double duration, void *params_p) {
	SegmentGroup *group = params_p;
	Body *cb = group->dep_body->orbit.cb;

	Vector3 r0 = osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, cb).r;
	OSV osv_arr = osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_dep + duration, cb);
	Vector3 r1 = osv_arr.r;

	Lambert3 lambert_sol = calc_lambert3(r0, r1, duration*86400, cb);

	double *array = malloc(NUM_PORKCHOP_MESH_VALUE_TYPES * sizeof(double));
	array[MESH_VAL_DATE] = jd_dep+duration;
	array[MESH_VAL_DUR] = duration;
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
	array[MESH_VAL_RPE] = 1e9;

	MeshPoint2 *new_mesh_point = create_mesh_point(vec2(jd_dep, duration), array, NUM_PORKCHOP_MESH_VALUE_TYPES);

	return new_mesh_point;
}

MeshPoint2 * test_next(double jd_dep0, double duration0, void *params_p) {
	Vector2 pos = vec2(jd_dep0, duration0);
	SegmentGroup *group = params_p;
	Body *cb = group->dep_body->orbit.cb;

	Quad *quad_at_pos = get_quad_at_position(group->prev->quad, pos);

	double jd_dep = get_quad_interpolated_value(quad_at_pos, pos, MESH_VAL_DATE);
	double vinf = get_quad_interpolated_value(quad_at_pos, pos, MESH_VAL_VINF);
	double left_x = 0, right_x = 0;
	double dt0 = interpolate_from_sorted_data_array2(group->group_bdr.lower_bdrs[0], jd_dep)*86400;
	double dt1 = interpolate_from_sorted_data_array2(group->group_bdr.upper_bdrs[0], jd_dep)*86400;

	OSV osv_dep = osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, cb);
	find_lambert_root(osv_dep, jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, vinf, NAN, &left_x, &right_x, 1);
	double duration = left_x / 86400;

	OSV osv_arr = osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_dep + duration, cb);
	Lambert3 lambert_sol = calc_lambert3(osv_dep.r, osv_arr.r, duration*86400, cb);

	double *array = malloc(NUM_PORKCHOP_MESH_VALUE_TYPES * sizeof(double));
	array[MESH_VAL_DATE] = jd_dep+duration;
	array[MESH_VAL_DUR] = duration;
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

	if(duration >= dt0/86400) {
		Vector3 v_arr = vec3(
			get_quad_interpolated_value(quad_at_pos, pos, MESH_VAL_ARRX),
			get_quad_interpolated_value(quad_at_pos, pos, MESH_VAL_ARRY),
			get_quad_interpolated_value(quad_at_pos, pos, MESH_VAL_ARRZ)
			);
		Vector3 v_body = vec3(
			get_quad_interpolated_value(quad_at_pos, pos, MESH_VAL_BODY_VX),
			get_quad_interpolated_value(quad_at_pos, pos, MESH_VAL_BODY_VY),
			get_quad_interpolated_value(quad_at_pos, pos, MESH_VAL_BODY_VZ)
			);


		array[MESH_VAL_RPE] = get_flyby_periapsis(v_arr, lambert_sol.v0, v_body, group->dep_body);

		// if(array[MESH_VAL_RPE] > 3e6) {
		// 	printf("----------- %f  %f  %f  %f  %f\n", array[MESH_VAL_RPE]/1e3, duration, mag_vec3(subtract_vec3(v_arr, v_body)), mag_vec3(subtract_vec3(lambert_sol.v0, v_body)), vinf);
		// } else {
		// 	// printf("-- %f  %f  %f  %f  %f\n", array[MESH_VAL_RPE]/1e3, duration, mag_vec3(subtract_vec3(v_arr, v_body)), mag_vec3(subtract_vec3(lambert_sol.v0, v_body)), vinf);
		// }
	} else {
		array[MESH_VAL_RPE] = 0;
	}

	MeshPoint2 *new_mesh_point = create_mesh_point(pos, array, NUM_PORKCHOP_MESH_VALUE_TYPES);

	return new_mesh_point;
}

MeshPoint2 * test_next_dur(double jd_dep0, double duration0, void *params_p) {
	Vector2 pos = vec2(jd_dep0, duration0);
	SegmentGroup *group = params_p;
	Body *cb = group->dep_body->orbit.cb;

	Quad *quad_at_pos = get_quad_at_position(group->prev->quad, pos);

	double jd_dep = get_quad_interpolated_value(quad_at_pos, pos, MESH_VAL_DATE);
	double vinf = get_quad_interpolated_value(quad_at_pos, pos, MESH_VAL_VINF);
	double left_x = 0, right_x = 0;
	double dt0 = interpolate_from_sorted_data_array2(group->group_bdr.lower_bdrs[0], jd_dep)*86400;
	double dt1 = interpolate_from_sorted_data_array2(group->group_bdr.upper_bdrs[0], jd_dep)*86400;

	OSV osv_dep = osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, cb);
	find_lambert_root(osv_dep, jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, vinf, NAN, &left_x, &right_x, 1);
	double duration = left_x / 86400;
	if(duration <= dt0/86400) duration = 0;

	double *array = malloc(NUM_PORKCHOP_MESH_VALUE_TYPES * sizeof(double));
	array[MESH_VAL_DATE] = duration > 0 ? jd_dep+duration : NAN;
	array[MESH_VAL_DUR] = duration > 0 ? duration : NAN;

	MeshPoint2 *new_mesh_point = create_mesh_point(pos, array, NUM_PORKCHOP_MESH_VALUE_TYPES);

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
	array[MESH_VAL_DUR] = duration;
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
	array[MESH_VAL_RPE] = 0;

	mesh_point->val = array;
	mesh_point->num_val = NUM_PORKCHOP_MESH_VALUE_TYPES;
}

void test_populate_next(MeshPoint2 *mesh_point, void *params_p) {
	SegmentGroup *group = params_p;
	Body *cb = group->dep_body->orbit.cb;

	Quad *quad_at_pos = get_quad_at_position(group->prev->quad, mesh_point->pos);

	double jd_dep = get_quad_interpolated_value(quad_at_pos, mesh_point->pos, MESH_VAL_DATE);
	double vinf = get_quad_interpolated_value(quad_at_pos, mesh_point->pos, MESH_VAL_VINF);
	double left_x = 0, right_x = 0;
	double dt0 = interpolate_from_sorted_data_array2(group->group_bdr.lower_bdrs[0], jd_dep)*86400;
	double dt1 = interpolate_from_sorted_data_array2(group->group_bdr.upper_bdrs[0], jd_dep)*86400;

	OSV osv_dep = osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, cb);
	find_lambert_root(osv_dep, jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, vinf, NAN, &left_x, &right_x, 1);
	double duration = left_x / 86400;

	OSV osv_arr = osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_dep + duration, cb);
	Lambert3 lambert_sol = calc_lambert3(osv_dep.r, osv_arr.r, duration*86400, cb);

	double *array = malloc(NUM_PORKCHOP_MESH_VALUE_TYPES * sizeof(double));
	array[MESH_VAL_DATE] = jd_dep+duration;
	array[MESH_VAL_DUR] = duration;
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

	if(duration >= dt0/86400) {
		Vector3 v_arr = vec3(
			get_quad_interpolated_value(quad_at_pos, mesh_point->pos, MESH_VAL_ARRX),
			get_quad_interpolated_value(quad_at_pos, mesh_point->pos, MESH_VAL_ARRY),
			get_quad_interpolated_value(quad_at_pos, mesh_point->pos, MESH_VAL_ARRZ)
			);
		Vector3 v_body = vec3(
			get_quad_interpolated_value(quad_at_pos, mesh_point->pos, MESH_VAL_BODY_VX),
			get_quad_interpolated_value(quad_at_pos, mesh_point->pos, MESH_VAL_BODY_VY),
			get_quad_interpolated_value(quad_at_pos, mesh_point->pos, MESH_VAL_BODY_VZ)
			);


		array[MESH_VAL_RPE] = get_flyby_periapsis(v_arr, lambert_sol.v0, v_body, group->dep_body);

	} else {
		array[MESH_VAL_RPE] = 0;
	}

	mesh_point->val = array;
	mesh_point->num_val = NUM_PORKCHOP_MESH_VALUE_TYPES;
}

void test_populate_next_dur(MeshPoint2 *mesh_point, void *params_p) {
	SegmentGroup *group = params_p;
	Body *cb = group->dep_body->orbit.cb;

	Quad *quad_at_pos = get_quad_at_position(group->prev->quad, mesh_point->pos);

	double jd_dep = get_quad_interpolated_value(quad_at_pos, mesh_point->pos, MESH_VAL_DATE);
	double vinf = get_quad_interpolated_value(quad_at_pos, mesh_point->pos, MESH_VAL_VINF);
	double left_x = 0, right_x = 0;
	double dt0 = interpolate_from_sorted_data_array2(group->group_bdr.lower_bdrs[0], jd_dep)*86400;
	double dt1 = interpolate_from_sorted_data_array2(group->group_bdr.upper_bdrs[0], jd_dep)*86400;

	OSV osv_dep = osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, cb);
	find_lambert_root(osv_dep, jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, vinf, NAN, &left_x, &right_x, 1);
	double duration = left_x / 86400;
	if(duration <= dt0/86400) duration = 0;

	double *array = malloc(NUM_PORKCHOP_MESH_VALUE_TYPES * sizeof(double));
	array[MESH_VAL_DATE] = duration > 0 ? jd_dep+duration : NAN;
	array[MESH_VAL_DUR] = duration > 0 ? duration : NAN;

	mesh_point->val = array;
	mesh_point->num_val = NUM_PORKCHOP_MESH_VALUE_TYPES;
}

void test_populate_next_from_dur(MeshPoint2 *mesh_point, void *params_p) {
	SegmentGroup *group = params_p;
	Body *cb = group->dep_body->orbit.cb;

	Quad *quad_at_pos = get_quad_at_position(group->prev->quad, mesh_point->pos);

	double jd_arr = mesh_point->val[MESH_VAL_DATE];
	double duration = mesh_point->val[MESH_VAL_DUR];
	double jd_dep = jd_arr-duration;

	OSV osv_dep = osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, cb);

	OSV osv_arr = osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_arr, cb);
	Lambert3 lambert_sol = calc_lambert3(osv_dep.r, osv_arr.r, duration*86400, cb);

	double *array = mesh_point->val;
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

	Vector3 v_arr = vec3(
		get_quad_interpolated_value(quad_at_pos, mesh_point->pos, MESH_VAL_ARRX),
		get_quad_interpolated_value(quad_at_pos, mesh_point->pos, MESH_VAL_ARRY),
		get_quad_interpolated_value(quad_at_pos, mesh_point->pos, MESH_VAL_ARRZ)
		);


	array[MESH_VAL_RPE] = get_flyby_periapsis(v_arr, lambert_sol.v0, osv_dep.v, group->dep_body);
}

typedef struct ErrorFuncParams {
	double max_error;
	int val_idx;
} ErrorFuncParams;

typedef struct BoundaryFuncParams {
	Boundary soft_bdr, hard_bdr;
} BoundaryFuncParams;

bool quad_test_is_in_bounds_function(Quad *quad, void *params_p) {
	BoundaryFuncParams *params = params_p;

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
	return is_quad_inside_boundary(quad, params->soft_bdr);
}

bool quad_test_abs_error_function(Quad *quad, void *params_p) {
	ErrorFuncParams *params = params_p;

	double interp_val = get_quad_interpolated_value(quad, quad->center->pos, params->val_idx);
	double e = fabs(interp_val - quad->center->val[params->val_idx]);

	return e > params->max_error;
}

bool quad_test_rel_error_function(Quad *quad, void *params_p) {
	ErrorFuncParams *params = params_p;

	double interp_val = get_quad_interpolated_value(quad, quad->center->pos, params->val_idx);
	double val = quad->center->val[params->val_idx];
	double e = interp_val > val ? interp_val / val : val / interp_val;

	return e > (params->max_error+1);
}

bool quad_test_nan_val_function(Quad *quad, void *params_p) {
	ErrorFuncParams *params = params_p;
	bool has_nan = false;
	for(int i = 0; i < 4; i++) {
		if(isnan(quad->corner[i]->val[params->val_idx])) has_nan = true;
	}
	if(isnan(quad->center->val[params->val_idx])) has_nan = true;
	return has_nan;
}

bool quad_test_vinf_arr(Quad *quad, void *params_p) {
	ErrorFuncParams *params = params_p;

	double vinf = quad->center->val[MESH_VAL_VINF];

	// vinf test
	double interp_vinf = get_quad_interpolated_value(quad, quad->center->pos, MESH_VAL_VINF);
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

int match_to_boundary(Quad *quad, void *params_p, QuadPointFunc *point_func, int min_rf_level, int max_rf_level, bool rm_hard_bdr_crossed) {
	BoundaryFuncParams *params = params_p;

	if(!is_quad_inside_boundary(quad, params->soft_bdr)) {
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

	bool is_crossed_by_bounds = false;
	int num_splits = 0;
	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF) && quad->rf_level < min_rf_level) {
		num_splits += split_quad(quad, point_func, NULL);
	} else {
		for(int i = 0; i < params->hard_bdr.num; i++) {
			if(is_quad_crossed_by_line(quad, params->hard_bdr.upper_bdrs[i]) || is_quad_crossed_by_line(quad, params->hard_bdr.lower_bdrs[i])) {
				is_crossed_by_bounds = true;
				break;
			}
		}
	}

	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF) && is_crossed_by_bounds) {
		if(quad->rf_level < max_rf_level) num_splits += split_quad(quad, point_func, NULL);
		else free_quad(quad, true);
	}

	if(!is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) {
		for(int i = 0; i < 4; i++) {
			if(quad->subquads[i]) {
				num_splits += match_to_boundary(quad->subquads[i], params_p, point_func, min_rf_level, max_rf_level, rm_hard_bdr_crossed);
			}
		}
	}

	return num_splits;
}

int match_to_valid(Quad *quad, void *params_p, QuadPointFunc *point_func, int min_rf_level, int max_rf_level, bool rm_hard_bdr_crossed) {
	BoundaryFuncParams *params = params_p;

	if(!is_quad_inside_boundary(quad, params->soft_bdr)) {
		free_quad(quad, true); return 0;
	}

	bool is_crossed_by_bounds = false;
	int num_splits = 0;
	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF) && quad->rf_level < min_rf_level) {
		num_splits += split_quad(quad, point_func, NULL);
	} else {
		for(int i = 0; i < params->hard_bdr.num; i++) {
			if(is_quad_crossed_by_line(quad, params->hard_bdr.upper_bdrs[i]) || is_quad_crossed_by_line(quad, params->hard_bdr.lower_bdrs[i])) {
				is_crossed_by_bounds = true;
				break;
			}
		}
	}

	if(is_quad_flag(quad, QUAD_FLAG_IS_LEAF) && is_crossed_by_bounds) {
		if(quad->rf_level < max_rf_level) num_splits += split_quad(quad, point_func, NULL);
		else free_quad(quad, true);
	}

	if(!is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) {
		for(int i = 0; i < 4; i++) {
			if(quad->subquads[i]) {
				num_splits += match_to_valid(quad->subquads[i], params_p, point_func, min_rf_level, max_rf_level, rm_hard_bdr_crossed);
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

	// good transfers Earth-Venus-Mars:
	// Departure (Earth): | 1959-08-23 00:00:00
	// Fly-By 1 (Venus): | 1960-02-05 16:44:38 |  Periapsis: 3944.75km (dur = 166 days)
	// Fly-By 2 (Mars): | 1960-05-21 08:08:34 |  Periapsis: 143.39km (dur = 106 days)
	// Arrival (Earth): | 1960-11-26 18:38:42

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
		set_opposition_conjunction_group_boundary(new_group, i-10, min_dep, max_dep, min_dur, max_dur, false);

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

		MeshPoint2 *p00 = create_mesh_point(vec2(quad_min_dep, quad_max_dur), NULL, 0);
		MeshPoint2 *p01 = create_mesh_point(vec2(quad_max_dep, quad_max_dur), NULL, 0);
		MeshPoint2 *p10 = create_mesh_point(vec2(quad_min_dep, quad_min_dur), NULL, 0);
		MeshPoint2 *p11 = create_mesh_point(vec2(quad_max_dep, quad_min_dur), NULL, 0);

		group->quad = create_quad_from_four_points(NULL, p00, p01, p10, p11, NULL);
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
		BoundaryFuncParams bound_func_params = {.soft_bdr = group->dv_bdr, .hard_bdr = group->group_bdr};
		match_to_boundary(group->quad, &bound_func_params, NULL, group->min_rf_level, group->max_rf_level+5, false);
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
		BoundaryFuncParams bound_func_params = {.soft_bdr = group->dv_bdr};
		QuadBoundsFunc bounds_func = {quad_test_is_in_bounds_function, &bound_func_params};
		QuadErrorFunc error_func = {quad_test_abs_error_function, &err_func_params};
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
		max_dur = 700;

		// for loop to be exchanged with some sort of boundary check
		for(int i = 0; i < 50; i++) {
			SegmentGroup *new_group = malloc(sizeof(SegmentGroup));
			new_group->dep_body = arr_body;
			new_group->arr_body = body_tf;
			new_group->system = ir_system;
			new_group->num_next_groups = 0;
			new_group->group_cap = 0;
			new_group->next = NULL;
			new_group->prev = group;
			new_group->vinf_array = NULL;
			new_group->group_bdr = create_new_boundary();
			new_group->dv_bdr = create_new_boundary();
			new_group->vinf_bdr = create_new_boundary();
			new_group->rpe_bdr = create_new_boundary();
			new_group->min_rf_level = new_group->prev->min_rf_level;
			new_group->max_rf_level = new_group->prev->max_rf_level;
			set_opposition_conjunction_group_boundary(new_group, i-10, min_dep, max_dep, min_dur, max_dur, true);

			if(data_array2_size(new_group->group_bdr.upper_bdrs[0]) > 0) {
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
		max_dur = 700;

		for(int i = 0; i < group->num_next_groups; i++) {
			group->next[i]->vinf_array = calc_min_vinf_line(group->next[i], min_dep, max_dep, min_dur, max_dur, dep_periapsis, max_depdv, 1);
		}

	}

	end_time_measurement(&tm, "Vinf Line");
	start_time_measurement(&tm);


	SegmentGroup *group = departure.segment_groups[pcgroup0];

	// calc_vinf_boundary(group, group->next[pcgroup1], group->quad, group->next[pcgroup1]->vinf_array, 1);
	//
	// end_time_measurement(&tm, "vinf_boundary");
	//

	// for(int j = 0; j < group->dv_bdr.num; j++) {
	// 	plot_data2(ir_coord_sys0, group->dv_bdr.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// 	plot_data2(ir_coord_sys0, group->dv_bdr.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// 	plot_data2(ir_coord_sys1, group->dv_bdr.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// 	plot_data2(ir_coord_sys1, group->dv_bdr.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// }
	//
	// for(int j = 0; j < group->next[pcgroup1]->vinf_bdr.num; j++) {
	// 	plot_data2(ir_coord_sys0, group->next[pcgroup1]->vinf_bdr.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// 	plot_data2(ir_coord_sys0, group->next[pcgroup1]->vinf_bdr.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// 	scatter_data2(ir_coord_sys1, group->next[pcgroup1]->vinf_bdr.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// 	scatter_data2(ir_coord_sys1, group->next[pcgroup1]->vinf_bdr.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// }
	// return;

	// for(int i = 0; i < group->num_next_groups; i++) {
	for(int i = pcgroup1; i < pcgroup1+1; i++) {
		SegmentGroup *next_group = group->next[i];
		calc_vinf_boundary(group, next_group, group->quad, next_group->vinf_array, 1);


		ErrorFuncParams err_func_params = {
			.max_error = 1,
			.val_idx = MESH_VAL_VINF
		};
		BoundaryFuncParams bound_func_params = {.soft_bdr = group->dv_bdr};
		QuadBoundsFunc bounds_func = {quad_test_is_in_bounds_function, &bound_func_params};
		QuadErrorFunc error_func = {quad_test_abs_error_function, &err_func_params};
		QuadPointFunc point_func = {test_, group};
		int num_split_cycles = 0;

		QuadList *quad_list = create_quad_list();
		QuadList *quad_split_list = create_quad_list();

		double dv_tol = 1;
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
		// printf("%lu\n", group->next[i]->vinf_bdr.num);
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
	start_time_measurement(&tm);

	if(1) {
		ErrorFuncParams err_func_params = {
			.max_error = 0.5
		};
		BoundaryFuncParams bound_func_params = {.soft_bdr = new_boundary};
		QuadBoundsFunc bounds_func = {quad_test_is_in_bounds_function, &bound_func_params};
		QuadErrorFunc error_func = {quad_test_vinf_arr, &err_func_params};
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
	end_time_measurement(&tm, "Refining previous step inside dv vinf bdr");


	// attach_quad_to_coordinate_system(ir_coord_sys0, departure.segment_groups[pcgroup0]->quad, CS_PLOT_TYPE_QUAD_SKELETON, CS_AXIS_DATE, CS_AXIS_NUMBER, CS_AXIS_DATE, TRUE, MESH_VAL_DATE, TRUE);
	// for(int j = 0; j < new_boundary.num; j++) {
	// 	plot_scatter_data2(ir_coord_sys0, new_boundary.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// 	plot_scatter_data2(ir_coord_sys0, new_boundary.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// }
	//
	// return;

	// for(int j = 0; j < new_boundary.num; j++) {
	// 	// printf("%d ----\n", j);
	// 	// print_data_array2(new_boundary.lower_bdrs[j], "dep", "dur");
	// 	// print_data_array2(new_boundary.upper_bdrs[j], "dep", "dur");
	// 	plot_scatter_data2(ir_coord_sys0, new_boundary.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// 	plot_scatter_data2(ir_coord_sys0, new_boundary.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// }
	// for(int j = 0; j < new_boundary.num; j++) {
	// 	plot_data2(ir_coord_sys1, new_boundary.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// 	plot_data2(ir_coord_sys1, new_boundary.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// }


	// start_time_measurement(&tm);

	SegmentGroup *next_group = group->next[pcgroup1];

	// if(1) {
	// 	Vector3 min_quads = get_quad_min_values(group->quad, NAN);
	// 	Vector3 max_quads = get_quad_max_values(group->quad, NAN);
	//
	// 	double quad_min_dep = min_quads.x;
	// 	double quad_max_dep = max_quads.x;
	// 	double quad_min_dur = min_quads.y;
	// 	double quad_max_dur = max_quads.y;
	// 	double abs_grad = fabs(departure.segment_groups[pcgroup0]->boundary_gradient);
	// 	double ratio_dur = (quad_max_dep-quad_min_dep)*abs_grad / (quad_max_dur-quad_min_dur);
	// 	double ratio_dep = 1.0/ratio_dur;
	//
	// 	int min_split = (int) log2(ratio_dur > ratio_dep ? ratio_dur : ratio_dep);
	// 	next_group->min_rf_level = min_split+3;
	//
	//
	// 	// printf("%f  %f  %f\n", max_dep-min_dep, max_dur-min_dur, abs_grad);
	// 	// printf("%f   %f  %f   %d\n", (max_dep-min_dep)*abs_grad, ratio_dep, ratio_dur, min_split);
	//
	// 	if(quad_max_dur-quad_min_dur < (quad_max_dep-quad_min_dep)*abs_grad) {
	// 		quad_max_dur = (quad_max_dep-quad_min_dep)*abs_grad + quad_min_dur;
	// 	} else {
	// 		quad_max_dep = (quad_max_dur-quad_min_dur)/abs_grad + quad_min_dep;
	// 	}
	// 	int max_rf_level_dep = (int) (log2((quad_max_dep-quad_min_dep)/0.001)) + 1;
	// 	int max_rf_level_dur = (int) (log2((quad_max_dur-quad_min_dur)/0.001)) + 1;
	// 	int max_rf_level = max_rf_level_dep > max_rf_level_dur ? max_rf_level_dep : max_rf_level_dur;
	// 	next_group->max_rf_level = max_rf_level;
	//
	// 	MeshPoint2 *p00 = create_mesh_point(vec2(quad_min_dep, quad_max_dur), NULL, 0);
	// 	MeshPoint2 *p01 = create_mesh_point(vec2(quad_max_dep, quad_max_dur), NULL, 0);
	// 	MeshPoint2 *p10 = create_mesh_point(vec2(quad_min_dep, quad_min_dur), NULL, 0);
	// 	MeshPoint2 *p11 = create_mesh_point(vec2(quad_max_dep, quad_min_dur), NULL, 0);
	//
	// 	next_group->quad = create_quad_from_four_points(NULL, p00, p01, p10, p11, NULL);
	// }
	//
	// end_time_measurement(&tm, "Quad generation next step");
	start_time_measurement(&tm);

	if(1) {
		next_group->quad = copy_quad_skeleton(group->quad);
		printf("Num of Leaves: %d\n", get_quad_leaves(next_group->quad, NULL));
		// BoundaryFuncParams bound_func_params = {.soft_bdr = new_boundary};
		// QuadBoundsFunc bounds_func = {quad_test_is_in_bounds_function, &bound_func_params};
		// remove_out_of_bounds_quads(next_group->quad, &bounds_func);
		//
		// printf("Num of Leaves: %d\n", get_quad_leaves(next_group->quad, NULL));

		BoundaryFuncParams bound_func_params2 = {.soft_bdr = new_boundary, .hard_bdr = next_group->vinf_bdr};
		// match_to_boundary(next_group->quad, &bound_func_params2, NULL, 7,  next_group->max_rf_level-10, false);
		match_to_boundary(next_group->quad, &bound_func_params2, NULL, 9,  10, false);
		printf("Num of Leaves: %d\n", get_quad_leaves(next_group->quad, NULL));
	}

	end_time_measurement(&tm, "Boundary matching next step");


	// attach_quad_to_coordinate_system(ir_coord_sys0, departure.segment_groups[pcgroup0]->quad, CS_PLOT_TYPE_QUAD_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_NUMBER, CS_AXIS_DATE, TRUE, MESH_VAL_DATE, TRUE);
	// attach_quad_to_coordinate_system(ir_coord_sys1, departure.segment_groups[pcgroup0]->next[pcgroup1]->quad, CS_PLOT_TYPE_QUAD_SKELETON, CS_AXIS_DATE, CS_AXIS_NUMBER, CS_AXIS_NUMBER, TRUE, MESH_VAL_DATE, TRUE);
	//
	// for(int j = 0; j < new_boundary.num; j++) {
	// 	plot_scatter_data2(ir_coord_sys1, new_boundary.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// 	plot_scatter_data2(ir_coord_sys1, new_boundary.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// }
	//
	// // for(int j = 0; j < departure.segment_groups[pcgroup0]->next[pcgroup1]->vinf_bdr.num; j++) {
	// // 	plot_scatter_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->next[pcgroup1]->vinf_bdr.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// // 	plot_scatter_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->next[pcgroup1]->vinf_bdr.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// // }
	//
	// // for(int j = 0; j < departure.segment_groups[pcgroup0]->dv_bdr.num; j++) {
	// // 	plot_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->dv_bdr.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// // 	plot_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->dv_bdr.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// // }
	// return;

	start_time_measurement(&tm);

	QuadPointPopFunc point_pop_func = {test_populate_next_dur, next_group};
	populate_quad_mesh_points(next_group->quad, &point_pop_func);

	end_time_measurement(&tm, "Populate Quads with duration");



	// attach_quad_to_coordinate_system(ir_coord_sys0, departure.segment_groups[pcgroup0]->quad, CS_PLOT_TYPE_QUAD_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_NUMBER, CS_AXIS_DATE, TRUE, MESH_VAL_DATE, TRUE);
	// attach_quad_to_coordinate_system(ir_coord_sys0, departure.segment_groups[pcgroup0]->next[pcgroup1]->quad, CS_PLOT_TYPE_QUAD_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_NUMBER, CS_AXIS_NUMBER, TRUE, MESH_VAL_DUR, TRUE);
	// attach_quad_to_coordinate_system(ir_coord_sys1, departure.segment_groups[pcgroup0]->next[pcgroup1]->quad, CS_PLOT_TYPE_QUAD_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_NUMBER, CS_AXIS_DATE, TRUE, MESH_VAL_DATE, TRUE);
	//
	// for(int j = 0; j < new_boundary.num; j++) {
	// 	plot_scatter_data2(ir_coord_sys1, new_boundary.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// 	plot_scatter_data2(ir_coord_sys1, new_boundary.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// }
	//
	// for(int j = 0; j < departure.segment_groups[pcgroup0]->next[pcgroup1]->vinf_bdr.num; j++) {
	// 	plot_scatter_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->next[pcgroup1]->vinf_bdr.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// 	plot_scatter_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->next[pcgroup1]->vinf_bdr.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// }
	//
	// for(int j = 0; j < departure.segment_groups[pcgroup0]->dv_bdr.num; j++) {
	// 	plot_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->dv_bdr.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// 	plot_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->dv_bdr.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// }
	// return;

	start_time_measurement(&tm);
	if(1) {
		ErrorFuncParams err_func_params = {
			.val_idx = MESH_VAL_DATE
		};

		BoundaryFuncParams bound_func_params = {.soft_bdr = new_boundary};
		QuadBoundsFunc bounds_func = {quad_test_is_in_bounds_function, &bound_func_params};
		QuadErrorFunc error_func = {quad_test_nan_val_function, &err_func_params};
		QuadPointFunc point_func = {test_next_dur, next_group};
		int num_split_cycles = 0;

		QuadList *quad_list = create_quad_list();
		QuadList *quad_split_list = create_quad_list();

		get_quad_leaves(next_group->quad, quad_list);

		for(int i = 0; i < 20; i++) {
			num_split_cycles++;
			for(int j = 0; j < quad_list->num; j++) {
				// update_quad_error_flag(quad_list->quad[j], next_group->min_rf_level, next_group->max_rf_level-5, &error_func);
				update_quad_error_flag(quad_list->quad[j], next_group->min_rf_level, 12, &error_func);
				if(is_quad_flag(quad_list->quad[j], QUAD_FLAG_SPLIT))
					append_to_quad_list(quad_split_list, quad_list->quad[j]);
				if(is_quad_flag(quad_list->quad[j], QUAD_FLAG_ACC_ERR) && !is_quad_flag(quad_list->quad[j], QUAD_FLAG_SPLIT)) {
					free_quad(quad_list->quad[j], true);
					remove_from_quad_list_at_idx(quad_list, j);
					j--;
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
		printf("Num of Leaves: %d\n", get_quad_leaves(next_group->quad, NULL));
	}
	end_time_measurement(&tm, "Duration matching");

	// attach_quad_to_coordinate_system(ir_coord_sys0, departure.segment_groups[pcgroup0]->quad, CS_PLOT_TYPE_QUAD_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_NUMBER, CS_AXIS_DATE, TRUE, MESH_VAL_DATE, TRUE);
	// attach_quad_to_coordinate_system(ir_coord_sys1, departure.segment_groups[pcgroup0]->next[pcgroup1]->quad, CS_PLOT_TYPE_QUAD_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_NUMBER, CS_AXIS_NUMBER, TRUE, MESH_VAL_DATE, TRUE);
	//
	// for(int j = 0; j < new_boundary.num; j++) {
	// 	plot_scatter_data2(ir_coord_sys1, new_boundary.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// 	plot_scatter_data2(ir_coord_sys1, new_boundary.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// }
	//
	// // for(int j = 0; j < departure.segment_groups[pcgroup0]->next[pcgroup1]->vinf_bdr.num; j++) {
	// // 	plot_scatter_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->next[pcgroup1]->vinf_bdr.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// // 	plot_scatter_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->next[pcgroup1]->vinf_bdr.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// // }
	//
	// // for(int j = 0; j < departure.segment_groups[pcgroup0]->dv_bdr.num; j++) {
	// // 	plot_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->dv_bdr.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// // 	plot_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->dv_bdr.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// // }
	// return;

	start_time_measurement(&tm);

	QuadPointPopFunc point_pop_func2 = {test_populate_next_from_dur, next_group};
	populate_quad_mesh_points(next_group->quad, &point_pop_func2);

	end_time_measurement(&tm, "Populate Quads from duration");



	// attach_quad_to_coordinate_system(ir_coord_sys0, departure.segment_groups[pcgroup0]->quad, CS_PLOT_TYPE_QUAD_SKELETON, CS_AXIS_DATE, CS_AXIS_NUMBER, CS_AXIS_NUMBER, TRUE, MESH_VAL_VINF, TRUE);
	// attach_quad_to_coordinate_system(ir_coord_sys0, next_group->quad, CS_PLOT_TYPE_QUAD_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_NUMBER, CS_AXIS_NUMBER, TRUE, MESH_VAL_DUR, TRUE);
	// attach_quad_to_coordinate_system(ir_coord_sys1, next_group->quad, CS_PLOT_TYPE_QUAD_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_NUMBER, CS_AXIS_NUMBER, TRUE, MESH_VAL_RPE, TRUE);
	//
	// for(int j = 0; j < new_boundary.num; j++) {
	// 	plot_scatter_data2(ir_coord_sys0, new_boundary.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// 	plot_scatter_data2(ir_coord_sys0, new_boundary.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// }
	//
	// // for(int j = 0; j < departure.segment_groups[pcgroup0]->next[pcgroup1]->vinf_bdr.num; j++) {
	// // 	plot_scatter_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->next[pcgroup1]->vinf_bdr.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// // 	plot_scatter_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->next[pcgroup1]->vinf_bdr.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// // }
	//
	// // for(int j = 0; j < departure.segment_groups[pcgroup0]->dv_bdr.num; j++) {
	// // 	plot_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->dv_bdr.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// // 	plot_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->dv_bdr.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	// // }
	// return;

	start_time_measurement(&tm);

	double rel_acc = 100;
	double min_rel_acc = 0.1;
	ErrorFuncParams err_func_params = {
		.max_error = rel_acc/2,
		.val_idx = MESH_VAL_RPE
	};
	BoundaryFuncParams bound_func_params = {.soft_bdr = new_boundary};
	QuadBoundsFunc bounds_func = {quad_test_is_in_bounds_function, &bound_func_params};
	QuadErrorFunc error_func = {quad_test_rel_error_function, &err_func_params};
	QuadPointFunc point_func = {test_next, next_group};
	int num_split_cycles = 0;

	QuadList *quad_list = create_quad_list();
	QuadList *quad_split_list = create_quad_list();

	get_quad_leaves(next_group->quad, quad_list);

	for(int i = 0; i < 10; i++) {
		num_split_cycles++;
		for(int j = 0; j < quad_list->num; j++) {
			update_quad_error_flag(quad_list->quad[j], next_group->min_rf_level, next_group->max_rf_level, &error_func);
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
	printf("Num of Leaves: %d\n", get_quad_leaves(next_group->quad, NULL));

	end_time_measurement(&tm, "RPE D&C");


	quad_list = create_quad_list();
	get_quad_leaves(next_group->quad, quad_list);

	DataArray1 *rpes_array = data_array1_create();
	// double rpe_sum = 0;
	double r_b = next_group->dep_body->radius;

	for(int i = 0; i < quad_list->num; i++) {
		data_array1_insert_new(rpes_array, quad_list->quad[i]->center->val[MESH_VAL_RPE]/r_b);
	}

	print_data_array1_boxplot(rpes_array);
	data_array1_free(rpes_array);

	attach_quad_to_coordinate_system(ir_coord_sys0, group->quad, CS_PLOT_TYPE_QUAD_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_NUMBER, CS_AXIS_DATE, TRUE, MESH_VAL_DATE, TRUE);
	attach_quad_to_coordinate_system(ir_coord_sys1, group->next[pcgroup1]->quad, CS_PLOT_TYPE_QUAD_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_NUMBER, CS_AXIS_NUMBER, TRUE, MESH_VAL_RPE, TRUE);
	// attach_quad_to_coordinate_system(ir_coord_sys1, group->next[pcgroup1]->quad, CS_PLOT_TYPE_QUAD_SKELETON, CS_AXIS_DATE, CS_AXIS_NUMBER, TRUE, MESH_VAL_DATE, TRUE);

	for(int j = 0; j < new_boundary.num; j++) {
		plot_data2(ir_coord_sys1, new_boundary.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
		plot_data2(ir_coord_sys1, new_boundary.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
	}
	for(int j = 0; j < group->group_bdr.num; j++) {
		plot_data2(ir_coord_sys1, group->group_bdr.upper_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
		plot_data2(ir_coord_sys1, group->group_bdr.lower_bdrs[j], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
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




void draw_mesh_interpolated_points_error(cairo_t *cr, double width, double height, Mesh2 *mesh, double tolerance) {
	double step_dep = 0.5;
	double step_dur = 0.5;
	DataArray3 *absolute_error = data_array3_create();
	DataArray3 *relative_error = data_array3_create();
	DataArray2 *error_pos = data_array2_create();
	data_array2_append_new(error_pos, vec2(1e9, 1e9));
	data_array2_append_new(error_pos, vec2(-1e9, -1e9));

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
						data_array2_append_new(error_pos, vec2(jd_dep-2.43418e+06, dur));
						// data_array3_append_new(absolute_error, jd_dep-2.43418e+06, dur, fabs(dv_dep-interpl_value));
						data_array3_append_new(absolute_error, vec3(jd_dep, dur, fabs(dv_dep-interpl_value)));
						data_array3_append_new(relative_error, vec3(jd_dep-2.43418e+06, dur, fabs(dv_dep-interpl_value)/dv_dep));
					}
				}
			}
		}
	}
	// print_data_array3(relative_error, "dep", "dur", "rel_error");
	// print_data_array3(error, "dep", "dur", "error");
	printf(" %d / %d   (%.4f %%)\n", num_errors, num_points, (num_errors/(double)num_points)*100);

	scatter_data3(ir_coord_sys1, absolute_error, CS_AXIS_DATE, CS_AXIS_DURATION, CS_AXIS_NUMBER, true);
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

	start_time_measurement(&tm);

	DataArray2 *array1 = data_array2_create();

	double min_x = 0;
	double max_x = 60;
	double x = min_x;

	int n1 = 0;
	for(int i = 0; i < pcgroup2; i++) {
		n1++;

		double y = -1/tan(0.05*x+0.1)+pcgroup1;
		data_array2_insert_new(array1, vec2(x, y));

		if(x == min_x) x = max_x;
		// else x = root_finder_monot_func_next_x(array1, 0.01, 1e-12);


		if(fabs(y) < 1e-9 || isnan(x)) break;
	}

	DataArray2 *array2 = data_array2_create();
	x = min_x;
	int n2 = 0;
	for(int i = 0; i < pcgroup2; i++) {
		n2++;

		double y = -1/tan(0.05*x+0.1)+pcgroup1;
		data_array2_insert_new(array2, vec2(x, y));

		if(x == min_x) x = max_x;
		// else x = root_finder_monot_func_next_x(array2, 0.01, 1e-12);


		if(fabs(y) < 1e-9 || isnan(x)) break;
	}

	printf("%d  %d\n", n1, n2);
	DataArray2 *grad1 = data_array2_get_gradient(array1);
	DataArray2 *grad2 = data_array2_get_gradient(array2);
	plot_scatter_data2(ir_coord_sys0, array1, CS_AXIS_NUMBER, CS_AXIS_NUMBER, true);
	plot_scatter_data2(ir_coord_sys0, array2, CS_AXIS_NUMBER, CS_AXIS_NUMBER, false);
	plot_scatter_data2(ir_coord_sys1, grad2, CS_AXIS_NUMBER, CS_AXIS_NUMBER, true);
	plot_scatter_data2(ir_coord_sys1, grad2, CS_AXIS_NUMBER, CS_AXIS_NUMBER, false);
	// plot_data2(ir_coord_sys1, grad2, CS_AXIS_NUMBER, CS_AXIS_NUMBER, true);


	print_timing_measurements(tm);
	free_timing_measurements(&tm);
}

G_MODULE_EXPORT void on_calc_ir2_() {
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

	DataArray2 *array = data_array2_create();

	// good transfers Earth-Venus-Mars:
	// Departure (Earth): | 1959-08-23 00:00:00
	// Fly-By 1 (Venus): | 1960-02-05 16:44:38 |  Periapsis: 3944.75km (dur = 166 days)
	// Fly-By 2 (Mars): | 1960-05-21 08:08:34 |  Periapsis: 143.39km (dur = 106 days)
	// Arrival (Earth): | 1960-11-26 18:38:42

	Body *dep_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_depbody))];
	Body *arr_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_arrbody))];

	Datetime date = {1959, 06, 7};
	double jd_dep = convert_date_JD(date)+0.5;
	// jd_dep = min_dep;
	double dur = min_dur;
	double dur_step = 0.01;

	OSV osv_dep = ir_system->prop_method == ORB_ELEMENTS ?
				osv_from_elements(dep_body->orbit, jd_dep) :
				osv_from_ephem(dep_body->ephem, dep_body->num_ephems, jd_dep, ir_system->cb);

	while(dur < max_dur) {
		double jd_arr = jd_dep + dur;
		OSV osv_arr = ir_system->prop_method == ORB_ELEMENTS ?
				osv_from_elements(arr_body->orbit, jd_arr) :
				osv_from_ephem(arr_body->ephem, arr_body->num_ephems, jd_arr, ir_system->cb);

		Lambert3 new_transfer = calc_lambert3(osv_dep.r, osv_arr.r, dur*86400, ir_system->cb);
		double vinf = fabs(mag_vec3(subtract_vec3(new_transfer.v0, osv_dep.v)));

		data_array2_append_new(array, vec2(dur, vinf));

		dur += dur_step;
	}

	DataArray2 *grad = data_array2_get_gradient(array);
	DataArray2 *grad2 = data_array2_get_gradient(grad);
	plot_data2(ir_coord_sys0, array, CS_AXIS_NUMBER, CS_AXIS_NUMBER, true);
	plot_data2(ir_coord_sys1, grad, CS_AXIS_NUMBER, CS_AXIS_NUMBER, true);
	// plot_data2(ir_coord_sys1, grad2, CS_AXIS_NUMBER, CS_AXIS_NUMBER, true);


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

	Datetime date = {1959, 06, 7};
	// Datetime date = {1959, 03, 8};
	double jd_dep = convert_date_JD(date)+0.5;
	// double cent_peak = 146.645;
	double cent_peak = 147.052;
	min_dur = cent_peak;
	max_dur = 250;
	// max_dur = cent_peak;
	// min_dur = 40;
	// min_dur = 70;
	// max_dur = 182.5;
	double target_y = max_depdv;

	OSV osv_dep = ir_system->prop_method == ORB_ELEMENTS ?
				osv_from_elements(dep_body->orbit, jd_dep) :
				osv_from_ephem(dep_body->ephem, dep_body->num_ephems, jd_dep, ir_system->cb);


	double diff = 1e20;
	double last_dur = 1e20;
	double dur = min_dur;
	DataArray2 *array1 = data_array2_create();

	start_time_measurement(&tm);

	for(int i = 0; i < pcgroup2; i++) {
		double jd_arr = jd_dep + dur;
		OSV osv_arr = ir_system->prop_method == ORB_ELEMENTS ?
				osv_from_elements(arr_body->orbit, jd_arr) :
				osv_from_ephem(arr_body->ephem, arr_body->num_ephems, jd_arr, ir_system->cb);

		Lambert3 new_transfer = calc_lambert3(osv_dep.r, osv_arr.r, dur*86400, ir_system->cb);
		double vinf = fabs(mag_vec3(subtract_vec3(new_transfer.v0, osv_dep.v)));
		diff = vinf - target_y;

		data_array2_insert_new(array1, vec2(dur, diff));

		last_dur = dur;
		if(!can_be_negative_monot_deriv(array1)) break;
		if(dur == min_dur) dur = max_dur;
		else if(dur == max_dur) dur = (min_dur+max_dur)/2;
		else dur = root_finder_monot_deriv_next_x(array1, true);

		if(fabs(diff) < 1e-2 || isnan(dur)) break;
	}

	end_time_measurement(&tm, "1");

	diff = 1e20;
	last_dur = 1e20;
	dur = min_dur;
	DataArray2 *array2 = data_array2_create();

	start_time_measurement(&tm);

	for(int i = 0; i < pcgroup2; i++) {
		double jd_arr = jd_dep + dur;
		OSV osv_arr = ir_system->prop_method == ORB_ELEMENTS ?
				osv_from_elements(arr_body->orbit, jd_arr) :
				osv_from_ephem(arr_body->ephem, arr_body->num_ephems, jd_arr, ir_system->cb);

		Lambert3 new_transfer = calc_lambert3(osv_dep.r, osv_arr.r, dur*86400, ir_system->cb);
		double vinf = fabs(mag_vec3(subtract_vec3(new_transfer.v0, osv_dep.v)));
		diff = vinf - target_y;

		data_array2_insert_new(array2, vec2(dur, diff));

		last_dur = dur;
		if(dur == min_dur) dur = max_dur;
		else if(dur == max_dur) dur = (min_dur+max_dur)/2;
		else dur = root_finder_single_minimum_func_next_x(array2, false, 0.25, 1e-9);

		if(fabs(diff) < 1e-2 || isnan(dur)) break;
	}
	end_time_measurement(&tm, "2");
	printf("%lu  %lu\n", data_array2_size(array1), data_array2_size(array2));


	DataArray2 *array1_n = data_array2_create();
	DataArray2 *array2_n = data_array2_create();
	DataArray1 *array1_n1 = data_array1_create();
	DataArray1 *array2_n1 = data_array1_create();
	DataArray2 *array1_ = data_array2_create();
	DataArray2 *array2_ = data_array2_create();
	start_time_measurement(&tm);
	for(double t_vinf = 100; t_vinf < 12000; t_vinf += 1) {
		diff = 1e20;
		last_dur = 1e20;
		dur = min_dur;
		int n1 = 0;
		data_array2_clear(array1_);
		data_array2_clear(array2_);

		for(int i = 0; i < pcgroup2; i++) {
			n1++;
			double jd_arr = jd_dep + dur;
			OSV osv_arr = ir_system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(arr_body->orbit, jd_arr) :
					osv_from_ephem(arr_body->ephem, arr_body->num_ephems, jd_arr, ir_system->cb);

			Lambert3 new_transfer = calc_lambert3(osv_dep.r, osv_arr.r, dur*86400, ir_system->cb);
			double vinf = fabs(mag_vec3(subtract_vec3(new_transfer.v0, osv_dep.v)));
			diff = vinf - t_vinf;

			data_array2_insert_new(array1_, vec2(dur, diff));

			last_dur = dur;
			if(!can_be_negative_monot_deriv(array1_)) break;
			if(dur == min_dur) dur = max_dur;
			else if(dur == max_dur) dur = (min_dur+max_dur)/2;
			else dur = root_finder_monot_deriv_next_x(array1_, true);

			if(fabs(diff) < 1e-4 || isnan(dur)) break;
		}

		diff = 1e20;
		last_dur = 1e20;
		dur = min_dur;
		int n2 = 0;

		for(int i = 0; i < pcgroup2; i++) {
			n2++;
			double jd_arr = jd_dep + dur;
			OSV osv_arr = ir_system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(arr_body->orbit, jd_arr) :
					osv_from_ephem(arr_body->ephem, arr_body->num_ephems, jd_arr, ir_system->cb);

			Lambert3 new_transfer = calc_lambert3(osv_dep.r, osv_arr.r, dur*86400, ir_system->cb);
			double vinf = fabs(mag_vec3(subtract_vec3(new_transfer.v0, osv_dep.v)));
			diff = vinf - t_vinf;

			data_array2_insert_new(array2_, vec2(dur, diff));

			last_dur = dur;
			// if(!can_be_negative_monot_deriv(array2_)) break;
			if(dur == min_dur) dur = max_dur;
			else if(dur == max_dur) dur = (min_dur+max_dur)/2;
			else dur = root_finder_single_minimum_func_next_x(array2_, false, 0.25, 1e-9);

			if(fabs(diff) < 1e-4 || isnan(dur)) break;
		}
		data_array2_append_new(array1_n, vec2(t_vinf, n1));
		data_array2_append_new(array2_n, vec2(t_vinf, n2));
		data_array1_insert_new(array1_n1, n1);
		data_array1_insert_new(array2_n1, n2);
	}
	end_time_measurement(&tm, "test");


	plot_scatter_data2(ir_coord_sys0, array1, CS_AXIS_NUMBER, CS_AXIS_NUMBER, true);
	plot_scatter_data2(ir_coord_sys0, array2, CS_AXIS_NUMBER, CS_AXIS_NUMBER, false);
	// plot_scatter_data2(ir_coord_sys1, data_array2_get_gradient(array1), CS_AXIS_NUMBER, CS_AXIS_NUMBER, true);
	// plot_scatter_data2(ir_coord_sys1, data_array2_get_gradient(array2), CS_AXIS_NUMBER, CS_AXIS_NUMBER, false);
	plot_scatter_data2(ir_coord_sys1, array1_n, CS_AXIS_NUMBER, CS_AXIS_NUMBER, true);
	plot_scatter_data2(ir_coord_sys1, array2_n, CS_AXIS_NUMBER, CS_AXIS_NUMBER, false);
	print_data_array1_boxplot(array1_n1);
	print_data_array1_boxplot(array2_n1);

	print_timing_measurements(tm);
	free_timing_measurements(&tm);
}








// G_MODULE_EXPORT void on_calc_ir2() {
// 	TimingMeasurements tm = init_timing_measurements();
// 	char *string;
//
// 	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_mindepdate));
// 	double min_dep = convert_date_JD(date_from_string(string, DATE_ISO));
// 	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdepdate));
// 	double max_dep = convert_date_JD(date_from_string(string, DATE_ISO));
// 	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_mindur));
// 	double min_dur = strtod(string, NULL);
// 	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdur));
// 	double max_dur = strtod(string, NULL);
// 	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_tolerance));
// 	double tolerance = strtod(string, NULL);
// 	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_numdeps));
// 	double target_numdeps = strtod(string, NULL);
// 	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdv));
// 	double max_depdv = strtod(string, NULL);
// 	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup0));
// 	int pcgroup0 = (int) strtod(string, NULL);
// 	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup1));
// 	int pcgroup1 = (int) strtod(string, NULL);
// 	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup2));
// 	int pcgroup2 = (int) strtod(string, NULL);
//
// 	Body *dep_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_depbody))];
// 	Body *arr_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_arrbody))];
//
// 	double dep_periapsis = dep_body->atmo_alt + ir_dep_periapsis;
//
//
// 	// Datetime date = {1964, 12, 24, 16, 44};
// 	// double jd_date = convert_date_JD(date);
// 	// DataArray2 *test1 = find_local_peak_array(jd_date, dep_body, arr_body, ir_system, 2700*86400, 2750*86400, 1, 0);
// 	// plot_data2(ir_coord_sys0, test1, CS_AXIS_NUMBER, CS_AXIS_NUMBER, true);
// 	// return;
//
// 	Datetime date = {1959, 11, 14, 7, 17};
// 	double jd_dep = convert_date_JD(date);
// 	double dt0 = 407.663626*86400;
// 	double dt1 = 785.960957*86400;
//
// 	DataArray2 *vinf_array = find_local_peak_array(jd_dep, dep_body, arr_body, ir_system, dt0, dt1, 1, 1);
// 	plot_scatter_data2(ir_coord_sys0, vinf_array, CS_AXIS_NUMBER, CS_AXIS_NUMBER, false);
// 	return;
//
// 	// DataArray2 *array_ = find_local_peak_array(min_dep+max_depdv, dep_body, arr_body, ir_system, min_dur*86400, max_dur*86400, 1);
// 	// plot_scatter_data2(ir_coord_sys0, array_, CS_AXIS_NUMBER, CS_AXIS_NUMBER, true);
// 	// plot_scatter_data2(ir_coord_sys1, array_, CS_AXIS_NUMBER, CS_AXIS_NUMBER, false);
// 	// return;
//
//
// 	int num_iterations = (int) target_numdeps;
//
// 	start_time_measurement(&tm);
//
// 	DepartureGroup departure;
// 	departure.dep_body = dep_body;
// 	departure.num_next_groups = 0;
// 	departure.group_cap = 8;
// 	departure.segment_groups = malloc(departure.group_cap * sizeof(SegmentGroup *));
//
// 	// for loop to be exchanged with some sort of boundary check
// 	for(int i = 0; i < 50; i++) {
// 		SegmentGroup *new_group = malloc(sizeof(SegmentGroup));
// 		new_group->dep_body = dep_body;
// 		new_group->arr_body = arr_body;
// 		new_group->num_steps = 0;
// 		new_group->system = ir_system;
// 		new_group->num_next_groups = 0;
// 		new_group->group_cap = 0;
// 		new_group->next = NULL;
// 		new_group->prev = NULL;
// 		new_group->vinf_array = NULL;
// 		set_opposition_conjunction_group_boundary(new_group, i-10, min_dep, max_dep, min_dur, max_dur, false);
//
// 		calc_group_porkchop(new_group, num_iterations, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_depdv, tolerance);
// 		if(new_group->num_steps != 0) {
// 			if(departure.num_next_groups == departure.group_cap) {
// 				departure.group_cap *= 2;
// 				SegmentGroup **temp_groups = realloc(departure.segment_groups, departure.group_cap * sizeof(SegmentGroup *));
// 				if(temp_groups) departure.segment_groups = temp_groups;
// 			}
// 			departure.segment_groups[departure.num_next_groups++] = new_group;
// 		} else free(new_group);
// 	}
// 	printf("Number of Departure Groups: %d\n\n", departure.num_next_groups);
//
//
// 	end_time_measurement(&tm, "Porkchopping Departure Groups");
// 	start_time_measurement(&tm);
//
// 	DataArray2 *depdv_boundary = calc_dv_boundary(departure.segment_groups[pcgroup0], num_iterations, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_depdv, tolerance/10);
// 	DataArray2 *depdv_lower_boundary = data_array2_create();
// 	DataArray2 *depdv_upper_boundary = data_array2_create();
// 	size_t num_points = data_array2_size(depdv_boundary);
// 	Vector2 *data = data_array2_get_data(depdv_boundary);
//
// 	for(int i = 0; i < num_points; i+=2) {
// 		data_array2_append_new(depdv_lower_boundary, data[i].x, data[i].y);
// 		data_array2_append_new(depdv_upper_boundary, data[i+1].x, data[i+1].y);
// 	}
//
// 	end_time_measurement(&tm, "DV Boundary");
// 	start_time_measurement(&tm);
//
// 	DataArray2 *array = calc_min_vinf_line(departure.segment_groups[pcgroup0], min_dep, max_dep, min_dur, max_dur, dep_periapsis, max_depdv, 1);
//
// 	// DataArray2 *array = data_array2_create();
// 	// int num_steps = 5;
// 	// for(int i = 0; i < num_steps; i++) {
// 	// 	double jd_dep = min_dep + i*(max_dep-min_dep)/num_steps;
// 	// 	Vector2 peak = get_local_peak(jd_dep, dep_body, arr_body, ir_system, min_dur*86400, max_dur*86400, 1, 0);
// 	// 	data_array2_append_new(array, jd_dep, peak.x);
// 	// }
//
//
// 	end_time_measurement(&tm, "Peaks");
// 	print_timing_measurements(tm);
// 	free_timing_measurements(&tm);
//
// 	plot_scatter_data2(ir_coord_sys0, array, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
// 	plot_scatter_data2(ir_coord_sys1, array, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
//
// 	// plot_scatter_data2(ir_coord_sys0, departure.segment_groups[pcgroup0]->upper_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
// 	// plot_scatter_data2(ir_coord_sys0, departure.segment_groups[pcgroup0]->lower_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
// 	// plot_data2(ir_coord_sys0, depdv_lower_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
// 	// plot_data2(ir_coord_sys0, depdv_upper_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
// 	// plot_data2(ir_coord_sys1, depdv_lower_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, true);
// 	// plot_data2(ir_coord_sys1, depdv_upper_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
// }
//
//
// G_MODULE_EXPORT void on_calc_ir3() {
// 	TimingMeasurements tm = init_timing_measurements();
// 	char *string;
//
// 	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_mindepdate));
// 	double min_dep = convert_date_JD(date_from_string(string, DATE_ISO));
// 	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdepdate));
// 	double max_dep = convert_date_JD(date_from_string(string, DATE_ISO));
// 	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_mindur));
// 	double min_dur = strtod(string, NULL);
// 	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdur));
// 	double max_dur = strtod(string, NULL);
// 	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_tolerance));
// 	double tolerance = strtod(string, NULL);
// 	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_numdeps));
// 	double target_numdeps = strtod(string, NULL);
// 	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_maxdv));
// 	double max_depdv = strtod(string, NULL);
// 	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup0));
// 	int pcgroup0 = (int) strtod(string, NULL);
// 	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup1));
// 	int pcgroup1 = (int) strtod(string, NULL);
// 	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup2));
// 	int pcgroup2 = (int) strtod(string, NULL);
//
// 	start_time_measurement(&tm);
//
// 	Body *dep_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_depbody))];
// 	Body *arr_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_arrbody))];
//
// 	double dep_periapsis = dep_body->atmo_alt + ir_dep_periapsis;
//
// 	int num_iterations = (int) target_numdeps;
//
// 	DepartureGroup departure;
// 	departure.dep_body = dep_body;
// 	departure.num_next_groups = 0;
// 	departure.group_cap = 8;
// 	departure.segment_groups = malloc(departure.group_cap * sizeof(SegmentGroup *));
//
// 	// for loop to be exchanged with some sort of boundary check
// 	for(int i = 0; i < 50; i++) {
// 		SegmentGroup *new_group = malloc(sizeof(SegmentGroup));
// 		new_group->dep_body = dep_body;
// 		new_group->arr_body = arr_body;
// 		new_group->num_steps = 0;
// 		new_group->system = ir_system;
// 		new_group->num_next_groups = 0;
// 		new_group->group_cap = 0;
// 		new_group->next = NULL;
// 		new_group->prev = NULL;
// 		new_group->vinf_array = NULL;
// 		set_opposition_conjunction_group_boundary(new_group, i-10, min_dep, max_dep, min_dur, max_dur, false);
//
// 		// calc_group_porkchop(new_group, num_iterations, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_depdv, tolerance);
// 		if(data_array2_get_max(new_group->group_bdr.upper_bdrs[0]).y >= min_dur &&
// 			data_array2_get_min(new_group->group_bdr.lower_bdrs[0]).y <= max_dur) {
// 			if(departure.num_next_groups == departure.group_cap) {
// 				departure.group_cap *= 2;
// 				SegmentGroup **temp_groups = realloc(departure.segment_groups, departure.group_cap * sizeof(SegmentGroup *));
// 				if(temp_groups) departure.segment_groups = temp_groups;
// 			}
// 			departure.segment_groups[departure.num_next_groups++] = new_group;
// 		} else free(new_group);
// 	}
// 	printf("Number of Departure Groups: %d\n\n", departure.num_next_groups);
//
// 	end_time_measurement(&tm, "Porkchopping Departure Groups");
// 	start_time_measurement(&tm);
//
// 	DataArray2 *depdv_boundary = calc_dv_boundary(departure.segment_groups[pcgroup0], num_iterations, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_depdv, tolerance/10);
// 	DataArray2 *depdv_lower_boundary = data_array2_create();
// 	DataArray2 *depdv_upper_boundary = data_array2_create();
// 	size_t num_points = data_array2_size(depdv_boundary);
// 	Vector2 *data = data_array2_get_data(depdv_boundary);
//
// 	for(int i = 0; i < num_points; i+=2) {
// 		data_array2_append_new(depdv_lower_boundary, data[i].x, data[i].y);
// 		data_array2_append_new(depdv_upper_boundary, data[i+1].x, data[i+1].y);
// 	}
//
// 	// print_data_array2(depdv_lower_boundary, "date", "dur");
// 	// print_data_array2(depdv_upper_boundary, "date", "dur");
//
//
// 	end_time_measurement(&tm, "DV Boundary");
//
// 	start_time_measurement(&tm);
//
// 	ErrorFuncParams err_func_params = {
// 		.max_error = tolerance/2,
// 		.val_idx = MESH_VAL_VINF
// 	};
//
// 	BoundaryFuncParams bound_func_params = {
// 		.soft_bdr = departure.segment_groups[pcgroup0]->dv_bdr
// 	};
//
// 	double quad_min_dep = min_dep;
// 	double quad_max_dep = max_dep;
// 	double quad_min_dur = min_dur;
// 	double quad_max_dur = max_dur;
// 	double abs_grad = fabs(departure.segment_groups[pcgroup0]->boundary_gradient);
// 	double ratio_dur = (quad_max_dep-quad_min_dep)*abs_grad / (quad_max_dur-quad_min_dur);
// 	double ratio_dep = 1.0/ratio_dur;
//
// 	int min_split = (int) log2(ratio_dur > ratio_dep ? ratio_dur : ratio_dep);
//
//
// 	// printf("%f  %f  %f\n", max_dep-min_dep, max_dur-min_dur, abs_grad);
// 	// printf("%f   %f  %f   %d\n", (max_dep-min_dep)*abs_grad, ratio_dep, ratio_dur, min_split);
//
// 	if(quad_max_dur-quad_min_dur < (quad_max_dep-quad_min_dep)*abs_grad) {
// 		quad_max_dur = (quad_max_dep-quad_min_dep)*abs_grad + quad_min_dur;
// 	} else {
// 		quad_max_dep = (quad_max_dur-quad_min_dur)/abs_grad + quad_min_dep;
// 	}
// 	int max_rf_level_dep = (int) (log2((quad_max_dep-quad_min_dep)/0.0001)) + 1;
// 	int max_rf_level_dur = (int) (log2((quad_max_dur-quad_min_dur)/0.0001)) + 1;
// 	int max_rf_level = max_rf_level_dep > max_rf_level_dur ? max_rf_level_dep : max_rf_level_dur;
//
// 	// printf("%d  %d\n", max_rf_level_dep, max_rf_level_dur);
//
//
// 	QuadPointFunc point_func = {test_, departure.segment_groups[pcgroup0]};
// 	QuadPointPopFunc point_pop_func = {test_populate, departure.segment_groups[pcgroup0]};
// 	QuadBoundsFunc bounds_func = {quad_test_is_in_bounds_function, &bound_func_params};
// 	QuadErrorFunc error_func = {quad_test_abs_error_function, &err_func_params};
//
// 	MeshPoint2 *p00 = point_func.func(quad_min_dep, quad_max_dur, departure.segment_groups[pcgroup0]);
// 	MeshPoint2 *p01 = point_func.func(quad_max_dep, quad_max_dur, departure.segment_groups[pcgroup0]);
// 	MeshPoint2 *p10 = point_func.func(quad_min_dep, quad_min_dur, departure.segment_groups[pcgroup0]);
// 	MeshPoint2 *p11 = point_func.func(quad_max_dep, quad_min_dur, departure.segment_groups[pcgroup0]);
//
// 	// MeshPoint2 *p00 = point_func.func(0, 100, NULL);
// 	// MeshPoint2 *p01 = point_func.func(100, 100, NULL);
// 	// MeshPoint2 *p10 = point_func.func(0, 0, NULL);
// 	// MeshPoint2 *p11 = point_func.func(100, 0, NULL);
// 	Quad *quad = create_quad_from_four_points(NULL, p00, p01, p10, p11, &point_func);
//
//
// 	end_time_measurement(&tm, "Quad generation");
// 	start_time_measurement(&tm);
//
// 	match_to_boundary(quad, &bound_func_params, NULL, 0, max_rf_level+5, false);
//
// 	end_time_measurement(&tm, "Boundary matching");
// 	start_time_measurement(&tm);
//
// 	populate_quad_mesh_points(quad, &point_pop_func);
//
// 	end_time_measurement(&tm, "Populate Quads");
//
// 	start_time_measurement(&tm);
//
// 	int num_split_cycles = 0;
//
// 	for(int i = 0; i < pcgroup1; i++) {
// 		num_split_cycles++;
// 		update_quad_error_flag(quad, min_split+3, max_rf_level, &error_func);
// 		int num_splits = split_quads_with_flag(quad, &point_func);
// 		printf("Number of Splits during cycle %d: %d\n", num_split_cycles, num_splits);
// 		if(num_splits == 0) break;
// 		remove_out_of_bounds_quads(quad, &bounds_func);
// 	}
//
// 	printf("Num Split Cycles: %d\n", num_split_cycles);
//
// 	// printf("To Split: %d\n", update_quad_error_flag(quad, 0, error_func));
// 	printf("Num of Leaves: %d\n", get_quad_leaves(quad, NULL));
// 	end_time_measurement(&tm, "Divide & Conquer");
// 	// start_time_measurement(&tm);
//
// 	Mesh2 *mesh = create_mesh_from_quads(quad);
//
// 	end_time_measurement(&tm, "Creating Mesh");
//
// 	start_time_measurement(&tm);
// 	printf("\n-------\n");
// 	Vector3 min = get_quad_min_values(quad, MESH_VAL_VINF);
// 	Vector3 max = get_quad_max_values(quad, MESH_VAL_VINF);
// 	double num_checks = pcgroup2;
// 	DataArray2 *error_array = data_array2_create();
// 	DataArray3 *all_error_array = data_array3_create();
// 	DataArray1 *all_errors_sorted = data_array1_create();
// 	double err_sum = 0;
//
// 	for(int i = 0; i < num_checks; i++) {
// 		double x = i * (max.x - min.x) / num_checks + min.x;
// 		show_progress("Error Measuring", i, num_checks);
// 		for(int j = 0; j < num_checks; j++) {
// 			double y = j * (max.y - min.y) / num_checks + min.y;
// 			Quad *quad_at_p = get_quad_at_position(quad, vec2(x, y));
// 			if(!quad_at_p) continue;
// 			double interp_val = get_quad_interpolated_value(quad_at_p, vec2(x, y), MESH_VAL_VINF);
//
// 			MeshPoint2 *point = point_func.func(x, y, departure.segment_groups[pcgroup0]);
// 			double val = point->val[MESH_VAL_VINF];
// 			free(point->val);
// 			free(point);
//
// 			data_array3_append_new(all_error_array, x, y, fabs(val - interp_val));
// 			data_array1_insert_new(all_errors_sorted, fabs(val - interp_val));
// 			err_sum += fabs(val - interp_val);
// 			if(fabs(val - interp_val) > tolerance) {
// 				// print_date(convert_JD_date(x, DATE_ISO), 0);
// 				// printf("   %f  %f\n", y, fabs(val-interp_val));
// 				data_array2_append_new(error_array, x, y);
// 			}
// 		}
// 	}
// 	show_progress("Quad Error Measuring", 1, 1);
// 	printf("\n--\n");
//
// 	// print_data_array1(all_errors_sorted, "error");
// 	size_t num_err = data_array1_size(all_errors_sorted);
// 	double *err = data_array1_get_data(all_errors_sorted);
//
// 	printf("Min: %f\nQ1: %f\nMedian: %f\nQ3: %f\nMax: %f\nAvg: %f\n",
// 	       err[0], err[1 * num_err / 4], err[2 * num_err / 4], err[3 * num_err / 4], err[num_err - 1],
// 	       err_sum / (double) num_err);
//
// 	printf("Error Ratio: %.4f %%   (%lu/%lu)\n", (double) data_array2_size(error_array) / (double) num_err * 100,
// 	       data_array2_size(error_array), num_err);
//
//
// 	data_array3_free(all_error_array);
// 	data_array1_free(all_errors_sorted);
// 	end_time_measurement(&tm, "Quad Error Measuring");
//
// 	start_time_measurement(&tm);
// 	printf("\n-------\n");
// 	min = get_quad_min_values(quad, MESH_VAL_VINF);
// 	max = get_quad_max_values(quad, MESH_VAL_VINF);
// 	num_checks = pcgroup2;
// 	error_array = data_array2_create();
// 	all_error_array = data_array3_create();
// 	all_errors_sorted = data_array1_create();
// 	err_sum = 0;
//
// 	for(int i = 0; i < num_checks; i++) {
// 		double x = i * (max.x - min.x) / num_checks + min.x;
// 		show_progress("Error Measuring", i, num_checks);
// 		for(int j = 0; j < num_checks; j++) {
// 			double y = j * (max.y - min.y) / num_checks + min.y;
// 			MeshTriangle2 *triangle = get_mesh_triangle_at_position(mesh, vec2(x, y));
// 			if(!triangle) continue;
// 			Vector3 p0 = vec3(triangle->points[0]->pos.x, triangle->points[0]->pos.y, triangle->points[0]->val[MESH_VAL_VINF]);
// 			Vector3 p1 = vec3(triangle->points[1]->pos.x, triangle->points[1]->pos.y, triangle->points[1]->val[MESH_VAL_VINF]);
// 			Vector3 p2 = vec3(triangle->points[2]->pos.x, triangle->points[2]->pos.y, triangle->points[2]->val[MESH_VAL_VINF]);
// 			double interp_val = get_triangle_interpolated_value(p0, p1, p2, vec2(x, y));
//
// 			MeshPoint2 *point = point_func.func(x, y, departure.segment_groups[pcgroup0]);
// 			double val = point->val[MESH_VAL_VINF];
// 			free(point->val);
// 			free(point);
//
// 			data_array3_append_new(all_error_array, x, y, fabs(val - interp_val));
// 			data_array1_insert_new(all_errors_sorted, fabs(val - interp_val));
// 			err_sum += fabs(val - interp_val);
// 			if(fabs(val - interp_val) > tolerance) {
// 				// print_date(convert_JD_date(x, DATE_ISO), 0);
// 				// printf("   %f  %f\n", y, fabs(val-interp_val));
// 				data_array2_append_new(error_array, x, y);
// 			}
// 		}
// 	}
// 	show_progress("Mesh Error Measuring", 1, 1);
// 	printf("\n--\n");
//
// 	// print_data_array1(all_errors_sorted, "error");
// 	num_err = data_array1_size(all_errors_sorted);
// 	err = data_array1_get_data(all_errors_sorted);
//
// 	printf("Min: %f\nQ1: %f\nMedian: %f\nQ3: %f\nMax: %f\nAvg: %f\n",
// 	       err[0], err[1 * num_err / 4], err[2 * num_err / 4], err[3 * num_err / 4], err[num_err - 1],
// 	       err_sum / (double) num_err);
//
// 	printf("Error Ratio: %.4f %%   (%lu/%lu)\n", (double) data_array2_size(error_array) / (double) num_err * 100,
// 	       data_array2_size(error_array), num_err);
//
//
// 	data_array3_free(all_error_array);
// 	data_array1_free(all_errors_sorted);
// 	end_time_measurement(&tm, "Mesh Error Measuring");
//
// 	// attach_quad_to_coordinate_system(ir_coord_sys0, quad, CS_PLOT_TYPE_QUAD_DEBUG, CS_AXIS_DATE, CS_AXIS_NUMBER, TRUE, MESH_VAL_VINF, TRUE);
// 	attach_quad_to_coordinate_system(ir_coord_sys1, quad, CS_PLOT_TYPE_QUAD_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_NUMBER, CS_AXIS_NUMBER, FALSE, MESH_VAL_VINF, TRUE);
// 	// attach_mesh_to_coordinate_system(ir_coord_sys1, mesh, CS_PLOT_TYPE_MESH_TRIANGLE_DEBUG, CS_AXIS_DATE, CS_AXIS_NUMBER, TRUE, MESH_VAL_VINF, TRUE);
// 	attach_mesh_to_coordinate_system(ir_coord_sys0, mesh, CS_PLOT_TYPE_MESH_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_NUMBER, CS_AXIS_NUMBER, FALSE, MESH_VAL_VINF, TRUE);
// 	// plot_data2(ir_coord_sys0, line, CS_AXIS_NUMBER, CS_AXIS_NUMBER, false);
// 	plot_data2(ir_coord_sys0, departure.segment_groups[pcgroup0]->group_bdr.upper_bdrs[0], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
// 	plot_data2(ir_coord_sys0, departure.segment_groups[pcgroup0]->group_bdr.lower_bdrs[0], CS_AXIS_DATE, CS_AXIS_NUMBER, false);
// 	plot_data2(ir_coord_sys0, depdv_lower_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
// 	plot_data2(ir_coord_sys0, depdv_upper_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
// 	plot_data2(ir_coord_sys1, depdv_lower_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
// 	plot_data2(ir_coord_sys1, depdv_upper_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
// 	// scatter_data2(ir_coord_sys0, error_array, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
// 	// plot_scatter_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->upper_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
// 	// plot_scatter_data2(ir_coord_sys1, departure.segment_groups[pcgroup0]->lower_boundary, CS_AXIS_DATE, CS_AXIS_NUMBER, false);
// 	print_timing_measurements(tm);
// 	free_timing_measurements(&tm);
// }