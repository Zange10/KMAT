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
GObject *tf_ir_pcgroup;

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
	tf_ir_pcgroup = gtk_builder_get_object(builder, "tf_ir_pcgroup");

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

double interpolate_from_sorted_data_array(DataArray2 *data_array, double x) {
	Vector2 *data = data_array2_get_data(data_array);
	size_t num_data = data_array2_size(data_array);
	if(x < data[0].x || x > data[num_data-1].x) return NAN;
	int idx = 0;
	while(idx < num_data-2 && data[idx + 1].x < x) idx++;

	Vector2 p0 = data[idx], p1 = data[idx + 1];

	double m = (p1.y-p0.y) / (p1.x-p0.x);
	return (x-p0.x) * m + p0.y;
}

void get_dur_limits_from_departure_date(MeshBox2 *box, double jd_dep, DataArray2 *data_array) {
	if(box->min.x > jd_dep || box->max.x < jd_dep) return;

	if(box->type == MESHBOX_SUBBOXES) {
		for(int i = 0; i < box->subboxes.num; i++) {
			get_dur_limits_from_departure_date(box->subboxes.boxes[i], jd_dep, data_array);
		}
	} else if(box->type == MESHBOX_TRIANGLES) {
		for(int i = 0; i < box->tri.num; i++) {
			if(triangle_is_edge(box->tri.triangles[i])) {
				Vector2 min, max;
				find_2dtriangle_minmax(box->tri.triangles[i], &min.x, &max.x, &min.y, &max.y);
				if(min.x > jd_dep || max.x < jd_dep) continue;
				for(int edge_idx = 0; edge_idx < 3; edge_idx++) {
					if(!box->tri.triangles[i]->adj_triangles[edge_idx]) {
						Vector2 p0 = box->tri.triangles[i]->points[edge_idx]->pos;
						Vector2 p1 = box->tri.triangles[i]->points[(edge_idx+1)%3]->pos;

						if(p0.x < jd_dep == p1.x < jd_dep && p0.x != jd_dep && p1.x != jd_dep) continue;

						double m = (p1.y-p0.y) / (p1.x-p0.x);
						double dur = (jd_dep-p0.x)*m+p0.y;
						data_array2_insert_new(data_array, jd_dep, dur);
					}
				}
			}
		}
	}
}

void get_dur_limits_from_all_triangles(Mesh2 *mesh, DataArray2 *data_array) {
	for(int i = 0; i < mesh->num_triangles; i++) {
		for(int edge_idx = 0; edge_idx < 3; edge_idx++) {
			if(!mesh->triangles[i]->adj_triangles[edge_idx]) {
				Vector2 p0 = mesh->triangles[i]->points[edge_idx]->pos;
				Vector2 p1 = mesh->triangles[i]->points[(edge_idx+1)%3]->pos;
				data_array2_insert_new(data_array, p0.x, p0.y);
				data_array2_insert_new(data_array, p1.x, p1.y);
			}
		}
	}
}



DataArray2 * get_dur_limits_from_edge_triangles(Mesh2 *mesh) {
	DataArray2 *data_array = data_array2_create();
	MeshTriangle2 *triangle = NULL;
	for(int i = 0; i < mesh->num_triangles; i++) {
		if(triangle_is_edge(mesh->triangles[i])) {
			triangle = mesh->triangles[i]; break;
		}
	}

	if(triangle == NULL) {return data_array;}
	MeshPoint2 *first_point = NULL;
	MeshPoint2 *prev_point = NULL;
	MeshPoint2 *current_point = NULL;

	for(int edge_idx = 0; edge_idx < 3; edge_idx++) {
		if(!triangle->adj_triangles[edge_idx]) {
			first_point = triangle->points[edge_idx];
			current_point = triangle->points[(edge_idx+1)%3];
			data_array2_insert_new(data_array, first_point->pos.x, first_point->pos.y);
			data_array2_insert_new(data_array, current_point->pos.x, current_point->pos.y);
		}
	}

	prev_point = first_point;

	do {
		for(int i = 0; i < current_point->num_triangles; i++) {
			if(triangle_is_edge(current_point->triangles[i])) {
				for(int edge_idx = 0; edge_idx < 3; edge_idx++) {
					if(!current_point->triangles[i]->adj_triangles[edge_idx]) {
						if(current_point == current_point->triangles[i]->points[edge_idx] && prev_point != current_point->triangles[i]->points[(edge_idx+1)%3]) {
							prev_point = current_point;
							current_point = current_point->triangles[i]->points[(edge_idx+1)%3];
							if(current_point == first_point) return data_array;
							data_array2_append_new(data_array, current_point->pos.x, current_point->pos.y);
							i = current_point->num_triangles;	// break loop outside loop
							break;
						}
						if(current_point == current_point->triangles[i]->points[(edge_idx+1)%3] && prev_point != current_point->triangles[i]->points[edge_idx]) {
							prev_point = current_point;
							current_point = current_point->triangles[i]->points[edge_idx];
							if(current_point == first_point) return data_array;
							data_array2_append_new(data_array, current_point->pos.x, current_point->pos.y);
							i = current_point->num_triangles;	// break loop outside loop
							break;
						}
					}
				}
			}
		}
	} while(current_point != first_point);
	return data_array;
}

DataArray2 * get_dur_limits_for_dep_from_point_list(DataArray2 *edges_array, double jd_dep) {
	DataArray2 *limits_inv = data_array2_create();
	Vector2 *edges = data_array2_get_data(edges_array);
	for(int i = 0; i < data_array2_size(edges_array); i++) {
		Vector2 p0 = edges[i];
		Vector2 p1 = edges[(i+1)%data_array2_size(edges_array)];

		if(p1.x < p0.x) {
			Vector2 temp = p0;
			p0 = p1;
			p1 = temp;
		}

		if(p0.x > jd_dep || p1.x < jd_dep) continue;
		if(p0.x == jd_dep) {
			data_array2_insert_new(limits_inv, p0.y, p0.x);
			continue;
		}
		if(p1.x == jd_dep) {
			data_array2_insert_new(limits_inv, p1.y, p1.x);
			continue;
		}

		double m = (p1.y-p0.y)/(p1.x-p0.x);
		double dur = (jd_dep - p0.x)*m + p0.y;
		data_array2_insert_new(limits_inv, dur, jd_dep);
	}

	DataArray2 *limits = data_array2_create();
	Vector2 *limits_inv_data = data_array2_get_data(limits_inv);
	double last = NAN;
	for(int i = 0; i < data_array2_size(limits_inv); i++) {
		if(limits_inv_data[i].x != last) {
			data_array2_insert_new(limits, jd_dep, limits_inv_data[i].x);
			last = limits_inv_data[i].x;
		}
	}
	data_array2_free(limits_inv);

	return limits;
}

double root_finder_almost_monot_deriv_next_x(DataArray2 *arr, int branch) {
	// branch = 0 for left branch, 1 for right branch
	Vector2 *data = data_array2_get_data(arr);
	int num_data = (int) data_array2_size(arr);

	int index;

	// left branch
	if(branch == 0) {
		index = 0;
		for(int i = 1; i < num_data; i++) {
			if(data[i].y < 0)	{ index = i; break; }
			else 				{ index = i; }
		}

		// right branch
	} else {
		index = num_data-1;
		for(int i = num_data-2; i >= 0; i--) {
			if(data[i].y < 0)	{ index =   i; break; }
			else 				{ index =   i; }
		}
	}

	if(branch == 0) return (data[index].x + data[index-1].x)/2;
	else 			return (data[index].x + data[index+1].x)/2;
}

void get_dur_limit_wrt_vinf(Mesh2 *mesh, double jd_dep, double min_vinf, DataArray2 *init_limit_array, DataArray2 *new_limits, double tolerance) {
	Vector2 *init_lim = data_array2_get_data(init_limit_array);
	size_t num_init_lim = data_array2_size(init_limit_array);
	if(num_init_lim == 0) return;
	if(num_init_lim == 1) {
		double dvinf = get_mesh_interpolated_value(mesh, vec2(jd_dep, init_lim[0].y)) - min_vinf;
		if(dvinf > 0) data_array2_insert_new(new_limits, init_lim[0].x, init_lim[0].y);
		return;
	}
	DataArray2 *new_limits_inv = data_array2_create();

	bool left_branch = true;
	DataArray2 *data = data_array2_create();

	for(int lim_idx = 0; lim_idx < num_init_lim; lim_idx+=2) {
		double lim0 = init_lim[lim_idx].y+1e-9;		// floating precision
		double lim1 = init_lim[lim_idx+1].y-1e-9;	// floating precision

		double dur = lim0;

		for(int i = 0; i < 100; i++) {
			double dvinf = get_mesh_interpolated_value(mesh, vec2(jd_dep, dur)) - min_vinf;
			if(i > 3 && dvinf > 0 && dvinf < tolerance) {
				data_array2_insert_new(new_limits_inv, dur, jd_dep);
				if(left_branch && data_array2_get_data(data)[data_array2_size(data)-1].y > 0) {
					left_branch = false;
				} else {
					break;
				}
			}

			data_array2_insert_new(data, dur, dvinf);

			if(i == 0) {
				if(dvinf > 0) {
					data_array2_insert_new(new_limits_inv, dur, jd_dep);
				} else {
					left_branch = false;
				}
				dur = lim1;
				continue;
			}
			if(i == 1) {
				if(dvinf > 0) data_array2_insert_new(new_limits_inv, dur, jd_dep);
				else if(!left_branch) break;
			}
			if(!can_be_negative_monot_deriv(data)) break;
			if(i < 30) dur = root_finder_monot_deriv_next_x(data, !left_branch);
			else dur = root_finder_almost_monot_deriv_next_x(data, !left_branch);
		}
	}

	for(int i = 0; i < data_array2_size(new_limits_inv); i++) {
		data_array2_append_new(new_limits, data_array2_get_data(new_limits_inv)[i].y, data_array2_get_data(new_limits_inv)[i].x);
	}
	data_array2_free(data);
	data_array2_free(new_limits_inv);
}

Vector3 get_varr_from_mesh(Mesh2 *mesh, double jd_arr, double dur) {
	MeshTriangle2 *triangle = get_mesh_triangle_at_position(mesh, vec2(jd_arr, dur));
	if(!triangle) return vec3(NAN, NAN, NAN);

	Vector3 tri_varrx[3];
	Vector3 tri_varry[3];
	Vector3 tri_varrz[3];

	for(int i = 0; i < 3; i++) {
		struct ItinStep *step = triangle->points[i]->data;
		tri_varrx[i] = vec3(triangle->points[i]->pos.x, triangle->points[i]->pos.y, step->v_arr.x);
		tri_varry[i] = vec3(triangle->points[i]->pos.x, triangle->points[i]->pos.y, step->v_arr.y);
		tri_varrz[i] = vec3(triangle->points[i]->pos.x, triangle->points[i]->pos.y, step->v_arr.z);
	}
	double varrx = get_triangle_interpolated_value(tri_varrx[0], tri_varrx[1], tri_varrx[2], vec2(jd_arr, dur));
	double varry = get_triangle_interpolated_value(tri_varry[0], tri_varry[1], tri_varry[2], vec2(jd_arr, dur));
	double varrz = get_triangle_interpolated_value(tri_varrz[0], tri_varrz[1], tri_varrz[2], vec2(jd_arr, dur));
	return vec3(varrx, varry, varrz);
}

Vector3 get_vbody_from_mesh(Mesh2 *mesh, double jd_arr, double dur) {
	MeshTriangle2 *triangle = get_mesh_triangle_at_position(mesh, vec2(jd_arr, dur));
	if(!triangle) return vec3(NAN, NAN, NAN);

	Vector3 tri_varrx[3];
	Vector3 tri_varry[3];
	Vector3 tri_varrz[3];

	for(int i = 0; i < 3; i++) {
		struct ItinStep *step = triangle->points[i]->data;
		tri_varrx[i] = vec3(triangle->points[i]->pos.x, triangle->points[i]->pos.y, step->v_body.x);
		tri_varry[i] = vec3(triangle->points[i]->pos.x, triangle->points[i]->pos.y, step->v_body.y);
		tri_varrz[i] = vec3(triangle->points[i]->pos.x, triangle->points[i]->pos.y, step->v_body.z);
	}
	double varrx = get_triangle_interpolated_value(tri_varrx[0], tri_varrx[1], tri_varrx[2], vec2(jd_arr, dur));
	double varry = get_triangle_interpolated_value(tri_varry[0], tri_varry[1], tri_varry[2], vec2(jd_arr, dur));
	double varrz = get_triangle_interpolated_value(tri_varrz[0], tri_varrz[1], tri_varrz[2], vec2(jd_arr, dur));
	return vec3(varrx, varry, varrz);
}

struct ItinStep * get_next_step_from_vinf(DepartureGroup *group, double v_inf, double jd_dep, double min_dur_dt, double max_dur_dt, bool leftside, double tolerance) {
	OSV osv_dep = group->system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(group->dep_body->orbit, jd_dep) :
					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);

	double dt0 = min_dur_dt, dt1 = max_dur_dt;

	// print_date(convert_JD_date(jd_dep, DATE_ISO), 0);
	// printf("       %f  |   %f     %f  |    %f     %f\n", jd_dep, dt0/86400, dt1/86400, dt0, dt1);

	// x: dt, y: diff_vinf
	DataArray2 *data = data_array2_create();

	double t0 = jd_dep;
	double last_dt, dt = dt0, t1, diff_vinf;

	for(int i = 0; i < 100; i++) {
		if(i == 0) dt = dt0;

		t1 = t0 + dt / 86400;

		OSV osv_arr = group->system->prop_method == ORB_ELEMENTS ?
				osv_from_elements(group->arr_body->orbit, t1) :
				osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, t1, group->system->cb);

		Lambert3 new_transfer = calc_lambert3(osv_dep.r, osv_arr.r, dt, group->system->cb);
		Vector3 v_dep = subtract_vec3(new_transfer.v0, osv_dep.v);
		diff_vinf = mag_vec3(v_dep) - v_inf;

		if(fabs(diff_vinf) < tolerance) {
			struct ItinStep *new_step = malloc(sizeof(struct ItinStep));
			new_step->body = group->arr_body;
			new_step->date = t1;
			new_step->r = osv_arr.r;
			new_step->v_dep = new_transfer.v0;
			new_step->v_arr = new_transfer.v1;
			new_step->v_body = osv_arr.v;
			new_step->num_next_nodes = 0;
			new_step->prev = NULL;
			new_step->next = NULL;
			return new_step;
		}

		data_array2_insert_new(data, dt, diff_vinf);

		if(!can_be_negative_monot_deriv(data)) break;
		last_dt = dt;
		if(i == 0) dt = dt1;
		else dt = root_finder_monot_deriv_next_x(data, !leftside);
		if(i > 3 && dt == last_dt) break;	// step size 0 (imprecision)
		if(isnan(dt) || isinf(dt)) break;
	}

	data_array2_free(data);
	return NULL;
}

typedef struct FlyByGroup {
	DataArray2 *dep_dur;
	struct ItinStep **steps;
} FlyByGroup;

int get_num_interval_per_dep(Vector2 *limits, int limit_idx0) {
	if(isnan(limits[limit_idx0*2].y)) return 0;
	int num_interval = 1;
	int limit_idx = limit_idx0+1;
	while(limits[limit_idx0*2].x == limits[limit_idx*2].x) {
		num_interval++;
		limit_idx++;
	}
	return num_interval;
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
	double max_dep_dv = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup));
	int pcgroup = (int) strtod(string, NULL);

	Body *dep_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_depbody))];
	Body *arr_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_arrbody))];

	double dep_periapsis = dep_body->atmo_alt + ir_dep_periapsis;

	double jd_dep = min_dep;
	int num_iterations = (int) target_numdeps;

	start_time_measurement(&tm);

	// int num_of_groups = 50;
	// DepartureGroup **departure_groups = malloc(num_of_groups*sizeof(DepartureGroup *));
	// int counter = 0;
	// for(int i = 0; i < num_of_groups; i++) {
	// 	departure_groups[counter] = malloc(sizeof(DepartureGroup));
	// 	departure_groups[counter]->dep_body = dep_body;
	// 	departure_groups[counter]->arr_body = arr_body;
	// 	departure_groups[counter]->num_departures = num_iterations;
	// 	departure_groups[counter]->system = ir_system;
	//
	// 	calc_group_porkchop(departure_groups[counter], i-10, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_dep_dv, tolerance);
	// 	if(departure_groups[counter]->num_departures == 0) {free(departure_groups[counter]);}
	// 	else counter++;
	// }
	//
	// printf("%d\n", counter);
	// num_of_groups = counter;

	DepartureGroup *departure_groups = malloc(sizeof(DepartureGroup));
	departure_groups->dep_body = dep_body;
	departure_groups->arr_body = arr_body;
	departure_groups->num_departures = num_iterations;
	departure_groups->system = ir_system;

	calc_group_porkchop(departure_groups, 6, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_dep_dv, tolerance);

	end_time_measurement(&tm, "Departure Porkchop");

	start_time_measurement(&tm);


	Mesh2 *mesh = new_mesh();
	struct ItinStep **steps = malloc(100000*sizeof(struct ItinStep *));

	DataArray2 *step_pos = data_array2_create();
	int counter = 0;
	for(int i = 0; i < departure_groups->num_departures; i++) {
		struct ItinStep *step = departure_groups->departures[i];
		if(step->num_next_nodes == -1) {
			double x = -1e10;
			double y = 0;
			data_array2_append_new(step_pos, x, y);
			steps[counter] = NULL;
			counter++;
		}
		if(step->num_next_nodes < 0) step->num_next_nodes = 0;

		for(int j = 0; j < step->num_next_nodes; j++) {
			double x = step->date;
			double y = step->next[j]->date - step->date;

			data_array2_append_new(step_pos, x, y);
			steps[counter] = step->next[j];
			counter++;
		}
	}

	MeshGrid2 *grid = create_mesh_grid(step_pos, (void**) steps);
	Mesh2 *group_mesh = create_mesh_from_grid_w_angled_guideline(grid, departure_groups->boundary_gradient);
	mesh = combine_meshes(mesh, group_mesh);
	free_grid_keep_points(grid);
	data_array2_free(step_pos);

	free(steps);



	end_time_measurement(&tm, "Building and Populating Mesh");

	start_time_measurement(&tm);

	for(int i = 0; i < mesh->num_points; i++) {
		MeshPoint2 *point = mesh->points[i];
		struct ItinStep *step = point->data;
		point->pos.x = step->date;
		point->pos.y = step->date - get_first(step)->date;
	}

	for(int i = 0; i < mesh->num_points; i++) {
		struct ItinStep *ptr = mesh->points[i]->data;
		double vinf = mag_vec3(subtract_vec3(ptr->v_arr, ptr->v_body));
		mesh->points[i]->val = vinf;
	}
	update_mesh_minmax(mesh);

	end_time_measurement(&tm, "Modifying Mesh");

	start_time_measurement(&tm);
	rebuild_mesh_boxes(mesh);
	end_time_measurement(&tm, "Rebuilding Boxes");


	attach_mesh_to_coordinate_system(ir_coord_sys1, mesh, CS_PLOT_TYPE_MESH_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_DURATION, TRUE, &remove_step_from_itinerary_void_ptr, TRUE);
	// attach_mesh_to_coordinate_system(ir_coord_sys1, mesh, CS_PLOT_TYPE_MESH_SKELETON, CS_AXIS_DATE, CS_AXIS_DURATION, FALSE, &remove_step_from_itinerary_void_ptr, FALSE);
	// attach_mesh_to_coordinate_system(ir_coord_sys1, mesh, CS_PLOT_TYPE_MESH_BOXES, CS_AXIS_DATE, CS_AXIS_DURATION, FALSE, &remove_step_from_itinerary_void_ptr, FALSE);

	start_time_measurement(&tm);

	DepartureGroup *departure_group = malloc(sizeof(DepartureGroup));
	DataArray2 *vinf_array = NULL;
	departure_group->dep_body = arr_body;
	departure_group->arr_body = get_body_by_name("Mars", ir_system);
	departure_group->num_departures = (int) target_numdeps;
	departure_group->system = ir_system;

	vinf_array = calc_min_vinf_line(departure_group, pcgroup, mesh->mesh_box->min.x, mesh->mesh_box->max.x, max_dep+max_dur, min_dur, max_dur, 1);

	end_time_measurement(&tm, "Vinf line");

	start_time_measurement(&tm);

	int num_deps = 1000;

	DataArray2 *edges_array = get_dur_limits_from_edge_triangles(mesh);

	end_time_measurement(&tm, "Dur limits from Edges");

	start_time_measurement(&tm);

	DataArray2 *outer_limits_all = data_array2_create();
	DataArray2 *vinf_limits_all = data_array2_create();
	DataArray2 *vinf_limit_jd_dep = data_array2_create();
	DataArray2 *valid_fb = data_array2_create();
	DataArray2 *min_dv2 = data_array2_create();

	double epsilon = 1e-6;
	double step = (mesh->mesh_box->max.x - mesh->mesh_box->min.x)/num_deps;
	double jd_next_dep = mesh->mesh_box->min.x+epsilon;

	while(jd_next_dep < mesh->mesh_box->max.x) {
		DataArray2 *limits = get_dur_limits_for_dep_from_point_list(edges_array, jd_next_dep);
		for(int j = 0; j < data_array2_size(limits); j++) {
			data_array2_append_new(outer_limits_all, data_array2_get_data(limits)[j].x, data_array2_get_data(limits)[j].y);
		}

		double jd_vinf_dep = interpolate_from_sorted_data_array(vinf_array, jd_next_dep);

		if(isnan(jd_vinf_dep)) {
			jd_next_dep += step;
			continue;
		}
		data_array2_clear(vinf_limit_jd_dep);
		get_dur_limit_wrt_vinf(mesh, jd_next_dep, jd_vinf_dep-tolerance*2, limits, vinf_limit_jd_dep, 1);

		if(data_array2_size(vinf_limit_jd_dep)%2 != 0) {
			jd_next_dep += epsilon;
			continue;
		}

		if(data_array2_size(vinf_limit_jd_dep) == 0) {
			// add a flagged pair
			// print_date(convert_JD_date(jd_next_dep, DATE_ISO), 1);
			data_array2_append_new(vinf_limits_all, jd_next_dep, NAN);
			data_array2_append_new(vinf_limits_all, jd_next_dep, NAN);
		}

		for(int j = 0; j < data_array2_size(vinf_limit_jd_dep); j++) {
			data_array2_append_new(vinf_limits_all, data_array2_get_data(vinf_limit_jd_dep)[j].x, data_array2_get_data(vinf_limit_jd_dep)[j].y);
		}
		if(jd_next_dep + step >= mesh->mesh_box->max.x && mesh->mesh_box->max.x - jd_next_dep >= 2*epsilon) {
			jd_next_dep = mesh->mesh_box->max.x - epsilon;
		} else jd_next_dep += step;
	}

	end_time_measurement(&tm, "Combine Porkchop with vinf line");
	start_time_measurement(&tm);


	int num_limits = (int) data_array2_size(vinf_limits_all)/2;
	Vector2 *limit_data = data_array2_get_data(vinf_limits_all);
	double min_jd_next_dep = limit_data[0].x;
	double max_jd_next_dep = limit_data[num_limits*2-1].x;
	double max_ddur = 0;

	DataArray1 *num_interval_change = data_array1_create();
	int last_num_intervals = 0;
	int interval_count = 0;
	data_array1_append_new(num_interval_change, limit_data[0].x);
	for(int i = 0; i < num_limits-1; i++) {
		bool empty_limit = isnan(data_array2_get_data(vinf_limits_all)[i*2].y);
		if(!empty_limit) {
			double ddur = limit_data[i*2+1].y - limit_data[i*2].y;
			if(ddur > max_ddur) max_ddur = ddur;
			interval_count++;
		}
		if(limit_data[(i+1)*2].x != limit_data[i*2].x){
			if(interval_count != last_num_intervals) {
				if(i != 0 && empty_limit) {
					data_array1_append_new(num_interval_change, limit_data[(i-1)*2].x);
					data_array1_append_new(num_interval_change, limit_data[i*2].x);
				} else if(i-interval_count >= 0) {
					data_array1_append_new(num_interval_change, limit_data[(i-interval_count)*2].x);
					data_array1_append_new(num_interval_change, limit_data[i*2].x);
				}
			}
			last_num_intervals = interval_count;
			interval_count = 0;
		}
	}
	// catch edges with (for some reason) a single point
	data_array1_append_new(num_interval_change, limit_data[(num_limits-1)*2-1].x);
	data_array1_append_new(num_interval_change, limit_data[num_limits*2-1].x);
	for(int i = 1; i < data_array1_size(num_interval_change); i += 3) {
		data_array1_insert_new(num_interval_change, (data_array1_get_data(num_interval_change)[i-1] + data_array1_get_data(num_interval_change)[i])/2);
	}
	for(int i = 0; i < data_array1_size(num_interval_change); i++) {
		print_date(convert_JD_date(data_array1_get_data(num_interval_change)[i], DATE_ISO), 1);
	}

	double jd_step = (max_jd_next_dep - min_jd_next_dep) / 50;
	double dur_step = max_ddur/20;
	int limit_idx = 0;
	counter = 0;
	double next_jd = limit_data[0].x;



	jd_dep = limit_data[0].x;
	OSV osv_dep = departure_group->system->prop_method == ORB_ELEMENTS ?
				osv_from_elements(departure_group->dep_body->orbit, jd_dep) :
				osv_from_ephem(departure_group->dep_body->ephem, departure_group->dep_body->num_ephems, jd_dep, departure_group->system->cb);
	OSV osv_arr0 = departure_group->system->prop_method == ORB_ELEMENTS ?
				   osv_from_elements(departure_group->arr_body->orbit, jd_dep) :
					osv_from_ephem(departure_group->arr_body->ephem, departure_group->arr_body->num_ephems, jd_dep, departure_group->system->cb);
	Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, departure_group->system->cb);
	double period_arr0 = calc_orbital_period(arr0);
	double next_conjunction_dt, next_opposition_dt, last_conjunction_dt, last_opposition_dt;
	calc_time_to_next_conjunction_and_opposition(osv_dep.r, osv_arr0, departure_group->system->cb, &next_conjunction_dt, &next_opposition_dt);

	double opp_guess, conj_guess;
	if(departure_group->top_boundary_type == DEPARTURE_GROUP_BOUNDARY_TOP_CONJ) {
		conj_guess = departure_group->boundary0_top.y + (jd_dep - departure_group->boundary0_top.x) * 86400 * departure_group->boundary_gradient;
		opp_guess = departure_group->boundary0_bottom.y + (jd_dep - departure_group->boundary0_bottom.x) * 86400 * departure_group->boundary_gradient;
	} else {
		opp_guess = departure_group->boundary0_top.y + (jd_dep - departure_group->boundary0_top.x) * 86400 * departure_group->boundary_gradient;
		conj_guess = departure_group->boundary0_bottom.y + (jd_dep - departure_group->boundary0_bottom.x) * 86400 * departure_group->boundary_gradient;
	}

	while(opp_guess-next_opposition_dt   >  0.5 * period_arr0) next_opposition_dt  += period_arr0;
	while(opp_guess-next_opposition_dt   < -0.5 * period_arr0) next_opposition_dt  -= period_arr0;
	while(conj_guess-next_conjunction_dt >  0.5 * period_arr0) next_conjunction_dt += period_arr0;
	while(conj_guess-next_conjunction_dt < -0.5 * period_arr0) next_conjunction_dt -= period_arr0;


	last_opposition_dt = next_opposition_dt;
	last_conjunction_dt = next_conjunction_dt;

	double last_jd_dep = jd_dep;


	FlyByGroup fb_group[100][10];
	int fb_num_groups_dep[100] = {0};
	int fb_group_x = -1; // is set to 0 during first loop
	int num_last_interval = 0;


	while(limit_idx < num_limits) {
		jd_dep = limit_data[limit_idx*2].x;

		osv_dep = departure_group->system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(departure_group->dep_body->orbit, jd_dep) :
					osv_from_ephem(departure_group->dep_body->ephem, departure_group->dep_body->num_ephems, jd_dep, departure_group->system->cb);

		osv_arr0 = departure_group->system->prop_method == ORB_ELEMENTS ?
					   osv_from_elements(departure_group->arr_body->orbit, jd_dep) :
					   osv_from_ephem(departure_group->arr_body->ephem, departure_group->arr_body->num_ephems, jd_dep, departure_group->system->cb);
		calc_time_to_next_conjunction_and_opposition(osv_dep.r, osv_arr0, departure_group->system->cb, &next_conjunction_dt, &next_opposition_dt);

		opp_guess = last_opposition_dt + (jd_dep-last_jd_dep)*86400*departure_group->boundary_gradient;
		conj_guess = last_conjunction_dt + (jd_dep-last_jd_dep)*86400*departure_group->boundary_gradient;

		while(opp_guess-next_opposition_dt   >  0.5 * period_arr0) next_opposition_dt  += period_arr0;
		while(opp_guess-next_opposition_dt   < -0.5 * period_arr0) next_opposition_dt  -= period_arr0;
		while(conj_guess-next_conjunction_dt >  0.5 * period_arr0) next_conjunction_dt += period_arr0;
		while(conj_guess-next_conjunction_dt < -0.5 * period_arr0) next_conjunction_dt -= period_arr0;

		last_opposition_dt = next_opposition_dt;
		last_conjunction_dt = next_conjunction_dt;
		last_jd_dep = jd_dep;


		double min_dur_dt, max_dur_dt;
		if(next_conjunction_dt < next_opposition_dt) {
			min_dur_dt = next_conjunction_dt;
			max_dur_dt = next_opposition_dt;
		} else {
			min_dur_dt = next_opposition_dt;
			max_dur_dt = next_conjunction_dt;
		}

		if(min_dur_dt < 86400*10) min_dur_dt = 86400*10;

		if(jd_dep < next_jd) {
			bool relevant_interval = false;
			for(int i = 0; i < data_array1_size(num_interval_change); i++) {
				if(jd_dep == data_array1_get_data(num_interval_change)[i] ||
					(data_array1_get_data(num_interval_change)[i] < limit_data[limit_idx*2].x &&
					data_array1_get_data(num_interval_change)[i] > limit_data[(limit_idx-1)*2].x)) {
					relevant_interval = true;
					break;
				}
			}
			if(!relevant_interval) { limit_idx++; continue; }
		}
		next_jd = jd_dep+jd_step;
		if(next_jd > max_jd_next_dep) next_jd = max_jd_next_dep;

		int num_interval = get_num_interval_per_dep(limit_data, limit_idx);
		if(num_last_interval != num_interval) {
			fb_group_x++;
			num_last_interval = num_interval;
			fb_num_groups_dep[fb_group_x] = num_interval;
			for(int i = 0; i < num_interval; i++) {
				fb_group[fb_group_x][i].dep_dur = data_array2_create();
				fb_group[fb_group_x][i].steps = malloc(1000*sizeof(struct ItinStep*));
			}
		}

		int fb_group_y = 0;

		while(limit_data[limit_idx*2].x == jd_dep && limit_idx < num_limits) {
			if(isnan(data_array2_get_data(vinf_limits_all)[limit_idx*2].y)) {limit_idx++; break;}
			double min_dur_temp = data_array2_get_data(vinf_limits_all)[limit_idx*2].y+1e-3;
			double max_dur_temp = data_array2_get_data(vinf_limits_all)[limit_idx*2+1].y-1e-3;
			int num_tests = (int) ((max_dur_temp - min_dur_temp)/dur_step) + 2;
			for(int i = 0; i < num_tests; i++) {
				double dur_temp = min_dur_temp + (max_dur_temp - min_dur_temp)*i/(num_tests-1);
				double vinf = get_mesh_interpolated_value(mesh, vec2(jd_dep, dur_temp));
				Vector3 v_arr = get_varr_from_mesh(mesh, jd_dep, dur_temp);
				Vector3 v_body = get_vbody_from_mesh(mesh, jd_dep, dur_temp);
				if(isnan(v_arr.x), isnan(v_body.x)) continue;
				counter++;
				struct ItinStep *next_step = NULL;
				double next_step_tolerance = 1;
				do {
					next_step = get_next_step_from_vinf(departure_group, vinf, jd_dep, min_dur_dt, max_dur_dt, true, next_step_tolerance);
					if(next_step) {
						double r_pe = get_flyby_periapsis(v_arr, next_step->v_dep, v_body, departure_groups->arr_body);
						if(r_pe/departure_groups->arr_body->radius > 0.8) {
							print_vec3(subtract_vec3(v_arr, v_body));
							print_vec3(subtract_vec3(next_step->v_dep, v_body));
							printf("%f   %f   (%s)\n", vinf, r_pe, departure_groups->arr_body->name);
							data_array2_append_new(valid_fb, jd_dep, dur_temp);
						}
						int step_idx = (int) data_array2_size(fb_group[fb_group_x][fb_group_y].dep_dur);
						fb_group[fb_group_x][fb_group_y].steps[step_idx] = next_step;
						data_array2_append_new(fb_group[fb_group_x][fb_group_y].dep_dur, jd_dep, dur_temp);
					} else {
						next_step_tolerance += tolerance;
					}
				} while(!next_step);
			}
			limit_idx++;
			fb_group_y++;
		}
	}

	printf("%d  %lu\n", counter, mesh->num_points);
	end_time_measurement(&tm, "Coarse meshing of next step");
	start_time_measurement(&tm);

	MeshGrid2 ***grids = malloc(100*sizeof(MeshGrid2**));
	for(int i = 0; i < 100; i++) {
		grids[i] = malloc(10*sizeof(MeshGrid2*));
	}

	if(fb_num_groups_dep[0] != 0) {
		for(int x_idx = 0; x_idx < 100; x_idx++) {
			for(int y_idx = 0; y_idx < fb_num_groups_dep[x_idx]; y_idx++) {
				grids[x_idx][y_idx] = create_mesh_grid(fb_group[x_idx][y_idx].dep_dur, (void*) fb_group[x_idx][y_idx].steps);
			}
		}
		Mesh2 *new_mesh = create_mesh_from_multiple_grids_w_angled_guideline(grids, 100, fb_num_groups_dep, departure_groups->boundary_gradient);

		// printf("---------_____-------\n");
		for(int i = 0; i < new_mesh->num_points; i++) {
			struct ItinStep *ptr = new_mesh->points[i]->data;
			jd_dep = new_mesh->points[i]->pos.x;
			double dur = new_mesh->points[i]->pos.y;
			double vinf = get_mesh_interpolated_value(new_mesh, vec2(jd_dep, dur));
			Vector3 v_arr = get_varr_from_mesh(mesh, jd_dep, dur);
			Vector3 v_body = get_vbody_from_mesh(mesh, jd_dep, dur);
			double r_pe = get_flyby_periapsis(v_arr, ptr->v_dep, v_body, departure_groups->arr_body);
			if(r_pe/departure_groups->arr_body->radius > 0.8) {
				print_vec3(subtract_vec3(v_arr, v_body));
				print_vec3(subtract_vec3(ptr->v_dep, v_body));
				printf("%f   %f   (%s)\n", vinf, (r_pe-departure_groups->arr_body->radius)/1000, departure_groups->arr_body->name);
			}
			new_mesh->points[i]->val = -r_pe;
		}

		end_time_measurement(&tm, "Calculate rpe for next step");
		start_time_measurement(&tm);

		for(int i = 0; i < new_mesh->num_triangles; i++) {
			bool remove_tri = true;
			// printf("%d / %d\n", i, new_mesh->num_triangles);
			for(int j = 0; j < 3; j++) {
				// printf("(");
				// print_date(convert_JD_date(new_mesh->triangles[i]->points[j]->pos.x, DATE_ISO), 0);
				// printf("  %f)  %f  |   ", new_mesh->triangles[i]->points[j]->pos.y, new_mesh->triangles[i]->points[j]->val/(-departure_groups->arr_body->radius));
				if(new_mesh->triangles[i]->points[j]->val/(-departure_groups->arr_body->radius) > 0.8) {
					remove_tri = false;
					break;
				}
			}
			// printf("   %s\n", remove_tri ? "true":"false");
			if(!remove_tri) continue;
			remove_triangle_from_mesh(new_mesh, i);
			i--;
		}
		update_mesh_minmax(new_mesh);
		rebuild_mesh_boxes(new_mesh);
		attach_mesh_to_coordinate_system(ir_coord_sys0, new_mesh, CS_PLOT_TYPE_MESH_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_DURATION, TRUE, NULL, TRUE);
	} else {
		clear_coordinate_system(ir_coord_sys0);
		draw_coordinate_system_data(ir_coord_sys0);
	}
	end_time_measurement(&tm, "Reduce range with rpe");
	print_timing_measurements(tm);
	free_timing_measurements(&tm);

	// scatter_data2(ir_coord_sys0, outer_limits_all, CS_AXIS_DATE, CS_AXIS_DURATION, TRUE);
	// scatter_data2(ir_coord_sys0, vinf_limits_all, CS_AXIS_DATE, CS_AXIS_DURATION, TRUE);
	// scatter_data2(ir_coord_sys0, valid_fb, CS_AXIS_DATE, CS_AXIS_DURATION, TRUE);
	scatter_data2(ir_coord_sys0, valid_fb, CS_AXIS_DATE, CS_AXIS_DURATION, FALSE);
	scatter_data2(ir_coord_sys1, valid_fb, CS_AXIS_DATE, CS_AXIS_DURATION, FALSE);
	// scatter_data2(ir_coord_sys1, vinf_limits_all, CS_AXIS_DATE, CS_AXIS_DURATION, FALSE);
	// scatter_data2(ir_coord_sys1, min_dv2, CS_AXIS_DATE, CS_AXIS_DURATION, FALSE);
	// plot_scatter_data2(ir_coord_sys0, dur_limit_top, CS_AXIS_DATE, CS_AXIS_DURATION, TRUE);
	// plot_scatter_data2(ir_coord_sys0, dur_limit_bottom, CS_AXIS_DATE, CS_AXIS_DURATION, FALSE);
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
		double min_x, max_x, min_y, max_y;
		MeshTriangle2 tri2d = *mesh->triangles[i];
		find_2dtriangle_minmax(&tri2d, &min_x, &max_x, &min_y, &max_y);

		for(double jd_dep = min_x; jd_dep <= max_x; jd_dep += step_dep) {
			for(double dur = min_y; dur <= max_y; dur += step_dur) {
				Vector2 p = vec2(jd_dep, dur);
				if(is_inside_triangle(&tri2d, p)) {
					Vector3 tri3[3];
					struct ItinStep *ptr = NULL;
					for(int idx = 0; idx < 3; idx++) {
						ptr = mesh->triangles[i]->points[idx]->data;
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
	double max_dep_dv = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup));
	int pcgroup = (int) strtod(string, NULL);

	Body *dep_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_depbody))];
	Body *arr_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_arrbody))];

	double dep_periapsis = dep_body->atmo_alt + ir_dep_periapsis;

	double jd_dep = min_dep;
	int num_iterations = (int) target_numdeps;

	start_time_measurement(&tm);

	int num_of_groups = 50;
	DepartureGroup **departure_groups = malloc(num_of_groups*sizeof(DepartureGroup *));
	int counter = 0;
	for(int i = 0; i < num_of_groups; i++) {
		departure_groups[counter] = malloc(sizeof(DepartureGroup));
		departure_groups[counter]->dep_body = dep_body;
		departure_groups[counter]->arr_body = arr_body;
		departure_groups[counter]->num_departures = num_iterations;
		departure_groups[counter]->system = ir_system;

		calc_group_porkchop(departure_groups[counter], i-10, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_dep_dv, tolerance);
		if(departure_groups[counter]->num_departures == 0) {free(departure_groups[counter]);}
		else counter++;
	}

	printf("%d\n", counter);
	num_of_groups = counter;

	end_time_measurement(&tm, "Departure Porkchop");

	start_time_measurement(&tm);


	Mesh2 *mesh = new_mesh();
	struct ItinStep **steps = malloc(100000*sizeof(struct ItinStep *));

	for(int group_id = 0; group_id < num_of_groups; group_id++) {
		DataArray2 *step_pos = data_array2_create();
		counter = 0;
		for(int i = 0; i < departure_groups[group_id]->num_departures; i++) {
			struct ItinStep *step = departure_groups[group_id]->departures[i];
			if(step->num_next_nodes == -1) {
				double x = -1e10;
				double y = 0;
				data_array2_append_new(step_pos, x, y);
				steps[counter] = NULL;
				counter++;
			}
			if(step->num_next_nodes < 0) step->num_next_nodes = 0;

			for(int j = 0; j < step->num_next_nodes; j++) {
				double x = step->date;
				double y = step->next[j]->date - step->date;

				data_array2_append_new(step_pos, x, y);
				steps[counter] = step->next[j];
				counter++;
			}
		}

		MeshGrid2 *grid = create_mesh_grid(step_pos, (void**) steps);
		Mesh2 *group_mesh = create_mesh_from_grid_w_angled_guideline(grid, departure_groups[group_id]->boundary_gradient);
		mesh = combine_meshes(mesh, group_mesh);
		free_grid_keep_points(grid);
		data_array2_free(step_pos);
	}
	free(steps);



	end_time_measurement(&tm, "Building and Populating Mesh");

	start_time_measurement(&tm);

	for(int i = 0; i < mesh->num_points; i++) {
		MeshPoint2 *point = mesh->points[i];
		struct ItinStep *step = point->data;
		point->pos.x = step->date;
		point->pos.y = step->date - get_first(step)->date;
	}

	for(int i = 0; i < mesh->num_points; i++) {
		struct ItinStep *ptr = mesh->points[i]->data;
		double vinf = mag_vec3(subtract_vec3(ptr->v_arr, ptr->v_body));
		mesh->points[i]->val = vinf;
	}
	update_mesh_minmax(mesh);

	end_time_measurement(&tm, "Modifying Mesh");

	start_time_measurement(&tm);
	rebuild_mesh_boxes(mesh);
	end_time_measurement(&tm, "Rebuilding Boxes");


	// attach_mesh_to_coordinate_system(ir_coord_sys1, mesh, CS_PLOT_TYPE_MESH_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_DURATION, TRUE, &remove_step_from_itinerary_void_ptr, TRUE);
	attach_mesh_to_coordinate_system(ir_coord_sys1, mesh, CS_PLOT_TYPE_MESH_SKELETON, CS_AXIS_DATE, CS_AXIS_DURATION, TRUE, &remove_step_from_itinerary_void_ptr, TRUE);
	// attach_mesh_to_coordinate_system(ir_coord_sys1, mesh, CS_PLOT_TYPE_MESH_BOXES, CS_AXIS_DATE, CS_AXIS_DURATION, FALSE, &remove_step_from_itinerary_void_ptr, FALSE);

	start_time_measurement(&tm);

	DepartureGroup *departure_group = malloc(sizeof(DepartureGroup));
	DataArray2 *vinf_array = NULL;
	departure_group->dep_body = arr_body;
	departure_group->arr_body = get_body_by_name("Mars", ir_system);
	departure_group->num_departures = (int) target_numdeps;
	departure_group->system = ir_system;

	// vinf_array = calc_min_vinf_line(departure_group, pcgroup+1, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, 1);
	vinf_array = calc_min_vinf_line(departure_group, pcgroup, mesh->mesh_box->min.x, mesh->mesh_box->max.x, mesh->mesh_box->max.x+max_dur, min_dur, max_dur, 1);
	plot_scatter_data2(ir_coord_sys0, vinf_array, CS_AXIS_DATE, CS_AXIS_NUMBER, FALSE);
	return;

	end_time_measurement(&tm, "Vinf line");

	start_time_measurement(&tm);

	DataArray2 *vinf_fit = data_array2_create();

	for(int i = 0; i < 1000; i++) {
		double jd_next_dep = mesh->mesh_box->min.x + (mesh->mesh_box->max.x-mesh->mesh_box->min.x)*i/1000;
		double jd_vinf_dep = interpolate_from_sorted_data_array(vinf_array, jd_next_dep);
		if(isnan(jd_vinf_dep)) continue;

		for(int dur = 50; dur < 300; dur+=1) {
			double vinf_arr = get_mesh_interpolated_value(mesh, vec2(jd_next_dep, dur));
			if(!isnan(vinf_arr) && jd_vinf_dep-tolerance < vinf_arr+tolerance) {
				data_array2_append_new(vinf_fit, jd_next_dep, dur);
			}
		}
	}


	end_time_measurement(&tm, "Combine Porkchop with vinf line");
	print_timing_measurements(tm);
	free_timing_measurements(&tm);

	scatter_data2(ir_coord_sys0, vinf_fit, CS_AXIS_DATE, CS_AXIS_DURATION, TRUE);
}

G_MODULE_EXPORT void on_calc_ir3() {
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
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup));
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
	DepartureGroup **departure_groups = malloc(num_of_groups*sizeof(DepartureGroup *));
	int counter = 0;
	for(int i = 0; i < num_of_groups; i++) {
		departure_groups[counter] = malloc(sizeof(DepartureGroup));
		departure_groups[counter]->dep_body = dep_body;
		departure_groups[counter]->arr_body = arr_body;
		departure_groups[counter]->num_departures = num_iterations;
		departure_groups[counter]->system = ir_system;

		calc_group_porkchop(departure_groups[counter], i-10, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_dep_dv, tolerance);
		if(departure_groups[counter]->num_departures == 0) {free(departure_groups[counter]);}
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
	for(int i = 0; i < departure_groups[pcgroup]->num_departures; i++) {
		struct ItinStep *step = departure_groups[pcgroup]->departures[i];
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


	MeshGrid2 *grid = create_mesh_grid(step_pos, (void**) steps);
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
	free_mesh(mesh, &remove_step_from_itinerary_void_ptr);
}
