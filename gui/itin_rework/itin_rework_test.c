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

G_MODULE_EXPORT void on_calc_ir() {
	return;
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
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ir_pcgroup0));
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

	SegmentGroup *departure_groups = malloc(sizeof(SegmentGroup));
	departure_groups->dep_body = dep_body;
	departure_groups->arr_body = arr_body;
	departure_groups->num_steps = num_iterations;
	departure_groups->system = ir_system;

	calc_group_porkchop(departure_groups, 5, num_iterations, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_dep_dv, tolerance);

	end_time_measurement(&tm, "Departure Porkchop");

	start_time_measurement(&tm);


	Mesh2 *mesh = new_mesh();
	struct ItinStep **steps = malloc(100000*sizeof(struct ItinStep *));

	DataArray2 *step_pos = data_array2_create();
	int counter = 0;
	for(int i = 0; i < departure_groups->num_steps; i++) {
		struct ItinStep *step = departure_groups->segment_steps[i];
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


	// attach_mesh_to_coordinate_system(ir_coord_sys1, mesh, CS_PLOT_TYPE_MESH_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_DURATION, TRUE, &remove_step_from_itinerary_void_ptr, TRUE);
	attach_mesh_to_coordinate_system(ir_coord_sys1, mesh, CS_PLOT_TYPE_MESH_SKELETON, CS_AXIS_DATE, CS_AXIS_DURATION, TRUE, &remove_step_from_itinerary_void_ptr, TRUE);

	start_time_measurement(&tm);

	SegmentGroup *departure_group = malloc(sizeof(SegmentGroup));
	DataArray2 *vinf_array = NULL;
	departure_group->dep_body = arr_body;
	departure_group->arr_body = get_body_by_name("Mars", ir_system);
	departure_group->num_steps = (int) target_numdeps;
	departure_group->system = ir_system;

	vinf_array = calc_min_vinf_line(departure_group, pcgroup, mesh->mesh_box->min.x, mesh->mesh_box->max.x, max_dep+max_dur, min_dur, max_dur, 1);

	end_time_measurement(&tm, "Vinf line");
	start_time_measurement(&tm);

	DataArray2 *vinf_limits = get_vinf_limits(mesh, vinf_array, tolerance);

	end_time_measurement(&tm, "Combine Porkchop with vinf line");
	start_time_measurement(&tm);

	FlyByGroups *fb_groups = get_flyby_groups_wrt_vinf(mesh, departure_group, vinf_limits, tolerance);

	end_time_measurement(&tm, "Coarse meshing of next step");
	start_time_measurement(&tm);

	Mesh2 *new_mesh = NULL;

	if(fb_groups->num_groups != 0) {
		new_mesh = get_rpe_mesh_from_fb_groups(fb_groups, mesh, departure_groups, true);

		end_time_measurement(&tm, "Calculate rpe for next step");
		start_time_measurement(&tm);

		for(int i = 0; i < new_mesh->num_triangles; i++) {
			bool remove_tri = true;
			for(int j = 0; j < 3; j++) {
				if(new_mesh->triangles[i]->points[j]->val/departure_groups->arr_body->radius > 0.8) {
					remove_tri = false;
					break;
				}
			}
			if(!remove_tri) continue;
			remove_triangle_from_mesh(new_mesh, i);
			i--;
		}
		update_mesh_minmax(new_mesh);
		rebuild_mesh_boxes(new_mesh);
	} else {
		clear_coordinate_system(ir_coord_sys0);
		draw_coordinate_system_data(ir_coord_sys0);
	}

	end_time_measurement(&tm, "Reduce range with rpe");
	start_time_measurement(&tm);

	for(int i = 0; i < mesh->num_points; i++) {
		MeshPoint2 *point = mesh->points[i];
		point->pos.x -= point->pos.y;
	}
	update_mesh_minmax(mesh);
	rebuild_mesh_boxes(mesh);
	for(int i = 0; i < new_mesh->num_points; i++) {
		MeshPoint2 *point = new_mesh->points[i];
		point->pos.x -= point->pos.y;
	}
	update_mesh_minmax(new_mesh);
	rebuild_mesh_boxes(new_mesh);
	end_time_measurement(&tm, "Modifying Meshes");


	start_time_measurement(&tm);

	int counter0 = 0;
	int counter1 = 0;
	for(int i = 0; i < mesh->num_triangles; i++) {
		if(is_triangle_bouding_box_inside_rectangle(mesh->triangles[i], new_mesh->mesh_box->min, new_mesh->mesh_box->max)) {
			for(int j = 0; j < new_mesh->num_triangles; j++) {
				Vector2 min, max;
				find_2dtriangle_minmax(new_mesh->triangles[j], &min, &max);
				if(is_triangle_bouding_box_inside_rectangle(mesh->triangles[i], min, max)) {counter0++; remove_triangle_from_mesh(mesh, i); i--; break;}
			}
		} else counter1++;
	}

	DataArray2 *edges_array = get_dur_limits_from_edge_triangles(new_mesh);
	double epsilon = 1e-6;
	int num_deps = 200;
	double step = (new_mesh->mesh_box->max.x - new_mesh->mesh_box->min.x)/num_deps;
	jd_dep = new_mesh->mesh_box->min.x+epsilon;

	DataArray2 *limits = data_array2_create();

	while(jd_dep < new_mesh->mesh_box->max.x) {
		DataArray2 *limits_dep = get_dur_limits_for_dep_from_point_list(edges_array, jd_dep);
		for(int i = 0; i < data_array2_size(limits_dep); i++) {
			data_array2_append_new(limits, data_array2_get_data(limits_dep)[i].x, data_array2_get_data(limits_dep)[i].y);
		}
		data_array2_free(limits_dep);
		jd_dep += step;
	}

	fb_groups = get_refined_departure_groups(departure_groups, limits, dep_periapsis, max_dep_dv, 1);
	Mesh2 *refined_mesh = get_dep_mesh_from_fb_groups(fb_groups, departure_groups);

	end_time_measurement(&tm, "Mesh Refinement");
		start_time_measurement(&tm);

	for(int i = 0; i < refined_mesh->num_points; i++) {
		MeshPoint2 *point = refined_mesh->points[i];
		struct ItinStep *step = point->data;
		point->pos.x = step->date;
		point->pos.y = step->date - get_first(step)->date;
	}

	update_mesh_minmax(refined_mesh);
	rebuild_mesh_boxes(refined_mesh);
	// for(int i = 0; i < refined_mesh->num_points; i++) {
	// 	struct ItinStep *ptr = refined_mesh->points[i]->data;
	// 	double vinf = mag_vec3(subtract_vec3(ptr->v_arr, ptr->v_body));
	// 	mesh->points[i]->val = vinf;
	// }


	vinf_limits = get_vinf_limits(refined_mesh, vinf_array, 1);
	print_data_array2(vinf_limits, "dep", "dur");

	DataArray2 *temp_vinf_limit = data_array2_create();
	for(int i = 0; i < data_array2_size(vinf_limits); i++) {
		if(isnan(data_array2_get_data(vinf_limits)[i].y)) continue;
		data_array2_append_new(temp_vinf_limit, data_array2_get_data(vinf_limits)[i].x, data_array2_get_data(vinf_limits)[i].y);
	}

	fb_groups = get_flyby_groups_wrt_vinf(refined_mesh, departure_group, vinf_limits, 10);



	new_mesh = NULL;

		new_mesh = get_rpe_mesh_from_fb_groups(fb_groups, refined_mesh, departure_groups, true);

		end_time_measurement(&tm, "Calculate rpe for next step");
		start_time_measurement(&tm);

		for(int i = 0; i < new_mesh->num_triangles; i++) {
			bool remove_tri = true;
			for(int j = 0; j < 3; j++) {
				if(new_mesh->triangles[i]->points[j]->val/departure_groups->arr_body->radius > 1) {
					printf("%f\n", new_mesh->triangles[i]->points[j]->val/departure_groups->arr_body->radius);
					remove_tri = false;
					break;
				}
			}
			if(!remove_tri) continue;
			remove_triangle_from_mesh(new_mesh, i);
			i--;
		}
		update_mesh_minmax(new_mesh);
		rebuild_mesh_boxes(new_mesh);



	end_time_measurement(&tm, "Coarse meshing of next step");
	print_timing_measurements(tm);
	free_timing_measurements(&tm);

	scatter_data2(ir_coord_sys0, vinf_array, CS_AXIS_DATE, CS_AXIS_NUMBER, TRUE);


	if(new_mesh) {
		attach_mesh_to_coordinate_system(ir_coord_sys0, new_mesh, CS_PLOT_TYPE_MESH_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_DURATION, TRUE, &remove_step_from_itinerary_void_ptr, TRUE);
		attach_mesh_to_coordinate_system(ir_coord_sys1, refined_mesh, CS_PLOT_TYPE_MESH_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_DURATION, TRUE, NULL, TRUE);
		// attach_mesh_to_coordinate_system(ir_coord_sys0, new_mesh, CS_PLOT_TYPE_MESH_SKELETON, CS_AXIS_DATE, CS_AXIS_DURATION, FALSE, NULL, FALSE);
	}
	// scatter_data2(ir_coord_sys1, limits, CS_AXIS_DATE, CS_AXIS_DURATION, FALSE);
	scatter_data2(ir_coord_sys1, temp_vinf_limit, CS_AXIS_DATE, CS_AXIS_NUMBER, FALSE);
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

		calc_group_porkchop(new_group, i-10, num_iterations, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_depdv, tolerance);
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
		struct ItinStep **segment_steps = malloc(100000*sizeof(struct ItinStep *));
		DataArray2 *step_pos = data_array2_create();
		int counter = 0;
		SegmentGroup *group = departure.segment_groups[group_idx];
		for(int i = 0; i < group->num_steps; i++) {
			struct ItinStep *step = group->segment_steps[i];
			if(!step) {
				data_array2_append_new(step_pos, NAN, NAN);
				segment_steps[counter] = NULL;
				counter++;
				continue;
			}

			for(int j = 0; j < step->num_next_nodes; j++) {
				double x = step->date;
				double y = step->next[j]->date - step->date;

				data_array2_append_new(step_pos, x, y);
				segment_steps[counter] = step->next[j];
				counter++;
			}
		}
		MeshGrid2 *grid = create_mesh_grid(step_pos, (void**) segment_steps);
		group->mesh = new_mesh();
		group->mesh = create_mesh_from_grid_w_angled_guideline(grid, group->boundary_gradient);
		free_grid_keep_points(grid);
		data_array2_free(step_pos);
		free(segment_steps);
	}

	end_time_measurement(&tm, "Building Departure Meshes");
	start_time_measurement(&tm);

	for(int group_idx = 0; group_idx < departure.num_next_groups; group_idx++) {
		Mesh2 *mesh = departure.segment_groups[pcgroup0]->mesh;
		for(int i = 0; i < mesh->num_points; i++) {
			struct ItinStep *ptr = mesh->points[i]->data;
			double vinf = mag_vec3(subtract_vec3(ptr->v_arr, ptr->v_body));
			mesh->points[i]->val = vinf;
		}
	}

	end_time_measurement(&tm, "Setting Mesh Values to Vinf of first Fly-By");



	attach_mesh_to_coordinate_system(ir_coord_sys0, departure.segment_groups[pcgroup0]->mesh, CS_PLOT_TYPE_MESH_INTERPOLATION, CS_AXIS_DATE, CS_AXIS_DURATION, TRUE, &remove_step_from_itinerary_void_ptr, TRUE);




	for(int i = 0; i < departure.num_next_groups; i++) free(departure.segment_groups[i]);
	free(departure.segment_groups);
	print_timing_measurements(tm);
	free_timing_measurements(&tm);
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

		calc_group_porkchop(departure_groups[counter], i-10, num_iterations, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_dep_dv, tolerance);
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
