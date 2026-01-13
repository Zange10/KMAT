#include "itin_rework_test.h"
#include "itin_rework_tools.h"

#include "gui/gui_manager.h"
#include "gui/gui_tools/screen.h"
#include "gui/drawing.h"
#include "mesh.h"
#include "geometrylib.h"
#include "gui/gui_tools/coordinate_system.h"
#include <math.h>
#include <sys/time.h>

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
Screen *ir_screen0;
// Screen *ir_screen1;
CoordinateSystem *ir_coord_sys1;


double ir_dep_periapsis = 50e3;

DataArray2 *ir_data0 = NULL;
// DataArray2 *ir_data1 = NULL;

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

	ir_data0 = data_array2_create();
	// ir_data1 = data_array2_create();

	ir_screen0 = new_screen(GTK_WIDGET(da_ir_graphing0), &on_ir_screen_resize, NULL, NULL, NULL, NULL);
	set_screen_background_color(ir_screen0, 0.15, 0.15, 0.15);

	// ir_screen1 = new_screen(GTK_WIDGET(da_ir_graphing1), &on_ir_screen_resize, NULL, NULL, NULL, NULL);
	// set_screen_background_color(ir_screen1, 0.15, 0.15, 0.15);

	ir_coord_sys1 = new_coordinate_system(GTK_WIDGET(da_ir_graphing1));
}

void on_ir_screen_resize(GtkWidget *widget, cairo_t *cr, gpointer *ptr) {
	if((Screen*)ptr == ir_screen0) {
		resize_screen(ir_screen0);
		clear_screen(ir_screen0);
		draw_plot_from_data_array(ir_screen0->static_layer.cr, ir_screen0->width, ir_screen0->height, ir_data0);
		draw_screen(ir_screen0);
	}

	// if((Screen*)ptr == ir_screen1) {
	// 	resize_screen(ir_screen1);
	// 	clear_screen(ir_screen1);
	// 	draw_plot_from_data_array(ir_screen1->static_layer.cr, ir_screen1->width, ir_screen1->height, ir_data1);
	// 	draw_screen(ir_screen1);
	// }
}

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

	clear_screen(ir_screen0);
	// clear_screen(ir_screen1);
	data_array2_free(ir_data0);

	struct timeval start, end;
	double elapsed_time;
	gettimeofday(&start, NULL);  // Record the ending time


	DepartureGroup *departure_group = malloc(sizeof(DepartureGroup));
	DataArray2 *vinf_array = NULL;
	departure_group->dep_body = dep_body;
	departure_group->arr_body = arr_body;
	departure_group->num_departures = (int) target_numdeps;
	departure_group->system = ir_system;

	vinf_array = calc_min_vinf_line(departure_group, pcgroup+1, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, 1);



	gettimeofday(&end, NULL);  // Record the ending time
	elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
	printf("----- | Total elapsed time: %.3f s | ---------\n", elapsed_time);

	print_data_array2(vinf_array, "dep_date", "vinf");
	printf("size: %lu\n", data_array2_size(vinf_array));

	ir_data0 = vinf_array;

	draw_scatter_from_data_array(ir_screen0->static_layer.cr, ir_screen0->width, ir_screen0->height, ir_data0);
	plot_scatter_data2(ir_coord_sys1, vinf_array, CS_AXIS_NUMBER, CS_AXIS_NUMBER, FALSE);

	draw_screen(ir_screen0);
	free(departure_group);
}

void resize_pcmesh_to_fit(Mesh2 mesh, double max_x, double max_y) {
	Vector2 max = mesh.points[0]-> pos;
	Vector2 min = mesh.points[0]-> pos;

	for(int i = 0; i < mesh.num_points; i++) {
		if(mesh.points[i]-> pos.x > max.x) max.x = mesh.points[i]-> pos.x;
		if(mesh.points[i]-> pos.x < min.x) min.x = mesh.points[i]-> pos.x;
		if(mesh.points[i]-> pos.y > max.y) max.y = mesh.points[i]-> pos.y;
		if(mesh.points[i]-> pos.y < min.y) min.y = mesh.points[i]-> pos.y;
	}

	max.x += 0.1*(max.x-min.x);
	min.x -= 0.1*(max.x-min.x);
	max.y += 0.1*(max.y-min.y);
	min.y -= 0.1*(max.y-min.y);

	Vector2 gradient = {
		max_x/(max.x-min.x),
		max_y/(max.y-min.y)
	};


	for(int i = 0; i < mesh.num_points; i++) {
		mesh.points[i]-> pos.x -= min.x;
		mesh.points[i]-> pos.x *= gradient.x;
		mesh.points[i]-> pos.y -= min.y;
		mesh.points[i]-> pos.y *= gradient.y;
		mesh.points[i]-> pos.y  = max_y-mesh.points[i]-> pos.y;
	}
}

void draw_mesh_triangle(cairo_t *cr, MeshTriangle2 triangle) {
	cairo_set_source_rgb(cr, 1,1,1);
	for(int i = 0; i < 3; i++) {
		draw_stroke(cr, vec2(triangle.points[i]->pos.x, triangle.points[i]->pos.y), vec2(triangle.points[(i+1)%3]->pos.x, triangle.points[(i+1)%3]->pos.y));
	}
}

void draw_mesh(cairo_t *cr, Mesh2 *mesh) {
	struct timeval start, end;
	gettimeofday(&start, NULL);
	cairo_set_source_rgb(cr, 0.0, 1.0, 1.0);
	for(int i = 0; i < mesh->num_triangles; i++) {
		draw_mesh_triangle(cr, *mesh->triangles[i]);
	}

	gettimeofday(&end, NULL);
	double duration = (end.tv_sec - start.tv_sec) +
					  (end.tv_usec - start.tv_usec) / 1e6;
	printf("Mesh Drawing: %.6f seconds\n", duration);
	printf("Triangles drawn: %zu\n", mesh->num_triangles);
}



void draw_triangle_debug(cairo_t *cr, Mesh2 *mesh) {
	struct timeval start, end;
	gettimeofday(&start, NULL);

	for(int i = 0; i < mesh->num_triangles; i++) {
		if(triangle_is_edge(mesh->triangles[i])) cairo_set_source_rgb(cr, 0, 0.5, 1);
		else cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);

		cairo_move_to(cr, mesh->triangles[i]->points[0]->pos.x, mesh->triangles[i]->points[0]->pos.y);
		cairo_line_to(cr, mesh->triangles[i]->points[1]->pos.x, mesh->triangles[i]->points[1]->pos.y);
		cairo_line_to(cr, mesh->triangles[i]->points[2]->pos.x, mesh->triangles[i]->points[2]->pos.y);
		cairo_close_path(cr);

		cairo_fill(cr);
	}



	gettimeofday(&end, NULL);
	double duration = (end.tv_sec - start.tv_sec) +
					  (end.tv_usec - start.tv_usec) / 1e6;
	printf("Interpolation Drawing: %.6f seconds\n", duration);

	printf("Triangles drawn: %zu\n", mesh->num_triangles);
}

void find_2dtriangle_minmax(MeshTriangle2 triangle, double *min_x, double *max_x, double *min_y, double *max_y) {
	*min_x = triangle.points[0]->pos.x;
	*max_x = triangle.points[0]->pos.x;
	*min_y = triangle.points[0]->pos.y;
	*max_y = triangle.points[0]->pos.y;

	for(int i = 1; i < 3; i++) {
		if(triangle.points[i]->pos.x < *min_x) *min_x = triangle.points[i]->pos.x;
		if(triangle.points[i]->pos.x > *max_x) *max_x = triangle.points[i]->pos.x;
		if(triangle.points[i]->pos.y < *min_y) *min_y = triangle.points[i]->pos.y;
		if(triangle.points[i]->pos.y > *max_y) *max_y = triangle.points[i]->pos.y;
	}
}

int is_inside_triangle(MeshTriangle2 triangle, Vector2 p) {
	Vector2 a = triangle.points[0]->pos;
	Vector2 b = triangle.points[1]->pos;
	Vector2 c = triangle.points[2]->pos;
	Vector2 v0 = subtract_vec2(c,a);
	Vector2 v1 = subtract_vec2(b,a);
	Vector2 v2 = subtract_vec2(p,a);

	double d00 = dot_vec2(v0, v0);
	double d01 = dot_vec2(v0, v1);
	double d11 = dot_vec2(v1, v1);
	double d20 = dot_vec2(v2, v0);
	double d21 = dot_vec2(v2, v1);

	double denom = d00*d11 - d01*d01;

	double u = (d11*d20 - d01*d21) / denom;
	double v = (d00*d21 - d01*d20) / denom;

	return (u >= 0 && v >= 0 && u+v <= 1+1e-9);
}



void set_color_from_value(cairo_t *cr, double value) {
	double r = value;
	double g = 1-value;
	double b = 4*pow(value-0.5,2);
	cairo_set_source_rgb(cr, r,g,b);
}



double get_triangle_interpolated_value(Vector3 p0, Vector3 p1, Vector3 p2, Vector2 p) {
	Vector2 a = vec2(p0.x, p0.y);
	Vector2 b = vec2(p1.x, p1.y);
	Vector2 c = vec2(p2.x, p2.y);
	Vector2 v0 = subtract_vec2(c,a);
	Vector2 v1 = subtract_vec2(b,a);
	Vector2 v2 = subtract_vec2(p,a);

	double d00 = dot_vec2(v0, v0);
	double d01 = dot_vec2(v0, v1);
	double d11 = dot_vec2(v1, v1);
	double d20 = dot_vec2(v2, v0);
	double d21 = dot_vec2(v2, v1);

	double denom = d00*d11 - d01*d01;

	double v = (d00*d21 - d01*d20) / denom;
	double w = (d11*d20 - d01*d21) / denom;
	double u = 1-v-w;

	return u*p0.z + v*p1.z + w*p2.z;
}


void draw_mesh_interpolated_points(cairo_t *cr, Mesh2 mesh, int width, int height) {
	int step = 1;

	for(int i = 0; i < mesh.num_triangles; i++) {
		double min_x, max_x, min_y, max_y;
		MeshTriangle2 tri2d = *mesh.triangles[i];
		find_2dtriangle_minmax(tri2d, &min_x, &max_x, &min_y, &max_y);
		if(max_x < 0 || min_x > width || max_y < 0 || min_y > height) continue;
		for(int x = (int)min_x+1; x <= max_x; x+=step) {
			for(int y = (int)min_y+1; y <= max_y; y+=step) {
				Vector2 p = vec2(x, y);
				if(x >= 0 && x < width && y >= 0 && y < height && is_inside_triangle(tri2d, p)) {
					Vector3 tri3[3];
					for(int idx = 0; idx < 3; idx++) {
						struct ItinStep *ptr = mesh.triangles[i]->points[idx]->data;
						double vinf = mag_vec3(subtract_vec3(ptr->v_dep, ptr->prev->v_body));
						double dv_dep = dv_circ(ptr->prev->body, ir_dep_periapsis+ptr->prev->body->radius, vinf);

						tri3[idx].x = mesh.triangles[i]->points[idx]->pos.x;
						tri3[idx].y = mesh.triangles[i]->points[idx]->pos.y;
						tri3[idx].z = (dv_dep-4000)/6000;
					}
					double interpl_value = get_triangle_interpolated_value(tri3[0], tri3[1], tri3[2], p);
					set_color_from_value(cr, interpl_value);
					cairo_rectangle(cr, x, y, 1, 1);
					cairo_fill(cr);
				}
			}
		}
	}
}


void draw_mesh_interpolated_points_error(cairo_t *cr, double width, double height, Mesh2 mesh, double tolerance) {
	double step_dep = 0.5;
	double step_dur = 0.5;
	DataArray3 *absolute_error = data_array3_create();
	DataArray3 *relative_error = data_array3_create();
	DataArray2 *error_pos = data_array2_create();
	data_array2_append_new(error_pos, 1e9, 1e9);
	data_array2_append_new(error_pos, -1e9, -1e9);

	int num_points = 0, num_errors = 0;

	for(int i = 0; i < mesh.num_triangles; i++) {
		double min_x, max_x, min_y, max_y;
		MeshTriangle2 tri2d = *mesh.triangles[i];
		find_2dtriangle_minmax(tri2d, &min_x, &max_x, &min_y, &max_y);

		for(double jd_dep = min_x; jd_dep <= max_x; jd_dep += step_dep) {
			for(double dur = min_y; dur <= max_y; dur += step_dur) {
				Vector2 p = vec2(jd_dep, dur);
				if(is_inside_triangle(tri2d, p)) {
					Vector3 tri3[3];
					struct ItinStep *ptr = NULL;
					for(int idx = 0; idx < 3; idx++) {
						ptr = mesh.triangles[i]->points[idx]->data;
						double vinf = mag_vec3(subtract_vec3(ptr->v_dep, ptr->prev->v_body));
						double dv_dep = dv_circ(ptr->prev->body, ir_dep_periapsis+ptr->prev->body->radius, vinf);

						tri3[idx].x = mesh.triangles[i]->points[idx]->pos.x;
						tri3[idx].y = mesh.triangles[i]->points[idx]->pos.y;
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
	print_data_array3(relative_error, "dep", "dur", "rel_error");
	// print_data_array3(error, "dep", "dur", "error");
	printf(" %d / %d   (%.4f %%)\n", num_errors, num_points, (num_errors/(double)num_points)*100);

	scatter_data3(ir_coord_sys1, absolute_error, CS_AXIS_DATE, CS_AXIS_DURATION, true);
	draw_scatter_from_data_array(cr, width, height, error_pos);
}

G_MODULE_EXPORT void on_calc_ir2() {
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

	clear_screen(ir_screen0);
	// clear_screen(ir_screen1);

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

	struct ItinStep **steps = malloc(10000*sizeof(struct ItinStep *));
	DataArray2 *step_pos = data_array2_create();
	counter = 0;
	for(int i = 0; i < departure_groups[pcgroup]->num_departures; i++) {
		struct ItinStep *step = departure_groups[pcgroup]->departures[i];
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


	MeshGrid2 grid = create_mesh_grid(step_pos, (void**) steps);
	// for(int i = 0; i < grid.num_cols; i++) {
	// 	if(grid.num_col_rows[i] == 0) {
	// 		printf("\n%4d:   ---", i); continue;
	// 	}
	// 	printf("\n%4d: ", i);
	// 	for(int j = 0; j < grid.num_col_rows[i]; j++) {
	// 		printf("%6.0f, ", grid.points[i][j]->pos.y);
	// 	}
	// }
	// printf("\n");
	// Mesh2 mesh = create_mesh_from_grid(grid);
	Mesh2 mesh = create_mesh_from_grid_w_angled_guideline(grid, departure_groups[pcgroup]->boundary_gradient);


	for(int i = 0; i < mesh.num_points; i++) {
		MeshPoint2 *point = mesh.points[i];
		struct ItinStep *step = point->data;
		point->pos.x = step->date;
		point->pos.y = step->date - get_first(step)->date;
	}

	gettimeofday(&end, NULL);  // Record the ending time
	elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
	printf("----- | Total elapsed time: %.3f s | ---------\n", elapsed_time);


	// resize_pcmesh_to_fit(mesh, ir_screen1->width, ir_screen1->height);
	// draw_mesh_interpolated_points(ir_screen1->static_layer.cr, mesh, ir_screen1->width, ir_screen1->height);
	// draw_mesh(ir_screen0->static_layer.cr, &mesh);
	draw_triangle_debug(ir_screen0->static_layer.cr, &mesh);


	struct ItinStep **departures = departure_groups[pcgroup]->departures;
	// remove departure dates with no valid itinerary
	for(int i = 0; i < departure_groups[pcgroup]->num_departures; i++) {
		if(departures[i] == NULL || departures[i]->num_next_nodes == 0) {
			departure_groups[pcgroup]->num_departures--;
			for(int j = i; j < departure_groups[pcgroup]->num_departures; j++) {
				departures[j] = departures[j + 1];
			}
			i--;
		}
	}

	int num_itins1 = 0, tot_num_itins = 0;
	for(int i = 0; i < departure_groups[pcgroup]->num_departures; i++) num_itins1 += get_number_of_itineraries(departures[i]);
	for(int i = 0; i < departure_groups[pcgroup]->num_departures; i++) tot_num_itins += get_total_number_of_stored_steps(departures[i]);

	printf("\n%d itineraries found!\nNumber of Nodes: %d\n", num_itins1, tot_num_itins);

	int index = 0;
	struct ItinStep **arrivals = malloc(num_itins1 * sizeof(struct ItinStep*));
	for(int i = 0; i < departure_groups[pcgroup]->num_departures; i++) store_itineraries_in_array(departures[i], arrivals, &index);
	struct PorkchopAnalyzerPoint *pp1 = malloc(num_itins1 * sizeof(struct PorkchopAnalyzerPoint));
	for(int i = 0; i < num_itins1; i++) {
		pp1[i].data = create_porkchop_point(arrivals[i], get_first(arrivals[0])->body->atmo_alt + dep_periapsis, arrivals[0]->body->atmo_alt + dep_periapsis);
		pp1[i].inside_filter = 1;
		pp1[i].group = NULL;
	}
	free(arrivals);

	// draw_porkchop(ir_screen0->static_layer.cr, ir_screen0->width, ir_screen0->height, pp1, num_itins1, TF_FLYBY, 0);

	
	DepartureGroup *departure_group = malloc(sizeof(DepartureGroup));
	DataArray2 *vinf_array = NULL;
	departure_group = malloc(sizeof(DepartureGroup));
	departure_group->dep_body = dep_body;
	departure_group->arr_body = arr_body;
	departure_group->num_departures = num_iterations*10;
	departure_group->system = ir_system;

	vinf_array = calc_min_vinf_line(departure_group, pcgroup+1, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, 1);
	if(departure_group->num_departures == 0) {free(departure_group);}
	else counter++;
	// print_data_array2(vinf_array, "dep_date", "vinf");
	printf("size: %lu\n", data_array2_size(vinf_array));

	ir_data0 = vinf_array;

	// draw_scatter_from_data_array(ir_screen0->static_layer.cr, ir_screen0->width, ir_screen0->height, ir_data0);

	draw_screen(ir_screen0);
	// draw_screen(ir_screen1);
}

void remove_step_from_itinerary_void_ptr(void *ptr) { remove_step_from_itinerary(ptr); }

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

	clear_screen(ir_screen0);
	// clear_screen(ir_screen1);

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


	MeshGrid2 grid = create_mesh_grid(step_pos, (void**) steps);
	Mesh2 mesh = create_mesh_from_grid_w_angled_guideline(grid, departure_groups[pcgroup]->boundary_gradient);
	free_grid_keep_points(&grid);


	// for(int i = 0; i < mesh.num_points; i++) {
	// 	MeshPoint2 *point = mesh.points[i];
	// 	struct ItinStep *step = point->data;
	// 	point->pos.x = step->date;
	// 	point->pos.y = step->date - get_first(step)->date;
	// }

	gettimeofday(&end, NULL);  // Record the ending time
	elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
	printf("----- | Total elapsed time: %.3f s | ---------\n", elapsed_time);


	draw_mesh_interpolated_points_error(ir_screen0->static_layer.cr, ir_screen0->width, ir_screen0->height, mesh, tolerance);

	// resize_pcmesh_to_fit(mesh, ir_screen1->width, ir_screen1->height);
	// draw_mesh_interpolated_points(ir_screen1->static_layer.cr, mesh, ir_screen1->width, ir_screen1->height);
	// draw_mesh(ir_screen1->static_layer.cr, &mesh);

	draw_screen(ir_screen0);
	// draw_screen(ir_screen1);
	free_mesh(&mesh, &remove_step_from_itinerary_void_ptr);
}
