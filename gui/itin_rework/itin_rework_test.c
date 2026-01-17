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
		printf("|%50s:  %.3fs  (%.2f %%)\n", ptr->name, ptr->elapsed_time, ptr->elapsed_time/total_time*100);
		ptr = ptr->next;
	}
	print_separator(100);
	printf("|%50s:  %.3fs\n", "TOTAL TIME", total_time);
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

void get_dur_limit_wrt_vinf(Mesh2 *mesh, double jd_dep, double min_vinf, DataArray2 *init_limit_array, DataArray2 *new_limits, double tolerance) {
	Vector2 *init_lim = data_array2_get_data(init_limit_array);
	size_t num_init_lim = data_array2_size(init_limit_array);
	if(num_init_lim == 0) return;
	if(num_init_lim == 1) {
		double dvinf = get_mesh_interpolated_value(mesh, vec2(jd_dep, init_lim[0].y)) - min_vinf;
		if(dvinf > 0) data_array2_insert_new(new_limits, init_lim[0].x, init_lim[0].y);
		return;
	}

	bool left_branch = true;
	DataArray2 *data = data_array2_create();

	for(int lim_idx = 0; lim_idx < num_init_lim; lim_idx+=2) {
		double lim0 = init_lim[lim_idx].y+1e-9;		// floating precision
		double lim1 = init_lim[lim_idx+1].y-1e-9;	// floating precision

		double dur = lim0;

		for(int i = 0; i < 1000; i++) {
			double dvinf = get_mesh_interpolated_value(mesh, vec2(jd_dep, dur)) - min_vinf;
			if(i > 3 && dvinf > 0 && dvinf < tolerance) {
				data_array2_insert_new(new_limits, jd_dep, dur);
				if(left_branch && data_array2_get_data(data)[data_array2_size(data)-1].y > 0) {
					left_branch = false;
				} else {
					break;
				}
			}

			data_array2_insert_new(data, dur, dvinf);

			if(i == 0) {
				if(dvinf > 0) {
					data_array2_insert_new(new_limits, jd_dep, dur);
				} else {
					left_branch = false;
				}
				dur = lim1;
				continue;
			}
			if(i == 1) {
				if(dvinf > 0) data_array2_insert_new(new_limits, jd_dep, dur);
				else if(!left_branch) break;
			}
			if(!can_be_negative_monot_deriv(data)) break;
			dur = root_finder_monot_deriv_next_x(data, !left_branch);
		}
	}
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

	int group_id = 4;
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
	// attach_mesh_to_coordinate_system(ir_coord_sys1, mesh, CS_PLOT_TYPE_MESH_SKELETON, CS_AXIS_DATE, CS_AXIS_DURATION, TRUE, &remove_step_from_itinerary_void_ptr, TRUE);
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
	DataArray2 *outer_limits_all = data_array2_create();
	DataArray2 *vinf_limits_all = data_array2_create();

	for(int i = 0; i < num_deps; i++) {
		double jd_next_dep = mesh->mesh_box->min.x + (mesh->mesh_box->max.x-mesh->mesh_box->min.x)*i/(num_deps-1);
		DataArray2 *limits = get_dur_limits_for_dep_from_point_list(edges_array, jd_next_dep);
		for(int j = 0; j < data_array2_size(limits); j++) {
			data_array2_append_new(outer_limits_all, data_array2_get_data(limits)[j].x, data_array2_get_data(limits)[j].y);
		}

		double jd_vinf_dep = interpolate_from_sorted_data_array(vinf_array, jd_next_dep);
		if(isnan(jd_vinf_dep)) continue;

		get_dur_limit_wrt_vinf(mesh, jd_next_dep, jd_vinf_dep-tolerance*2, limits, vinf_limits_all, 1);
	}


	end_time_measurement(&tm, "Combine Porkchop with vinf line");
	print_timing_measurements(tm);
	free_timing_measurements(&tm);

	// scatter_data2(ir_coord_sys0, outer_limits_all, CS_AXIS_DATE, CS_AXIS_DURATION, TRUE);
	scatter_data2(ir_coord_sys0, vinf_limits_all, CS_AXIS_DATE, CS_AXIS_DURATION, TRUE);
	scatter_data2(ir_coord_sys1, vinf_limits_all, CS_AXIS_DATE, CS_AXIS_DURATION, FALSE);
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
