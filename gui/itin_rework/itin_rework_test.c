#include "itin_rework_test.h"

#include "gui/gui_manager.h"
#include "gui/gui_tools/screen.h"
#include "gui/drawing.h"
#include "geometrylib.h"
#include <math.h>

GObject *ir_window;
GObject *da_ir_graphing;
GObject *cb_ir_system;
GObject *cb_ir_central_body;
GObject *cb_ir_depbody;
GObject *cb_ir_arrbody;
GObject *tf_ir_mindepdate;
GObject *tf_ir_maxdepdate;
GObject *tf_ir_mindur;
GObject *tf_ir_maxdur;

CelestSystem *ir_system;
Screen *ir_screen;

double ir_dep_periapsis = 50e3;

DataArray2 *ir_data;

void on_ir_screen_resize(GtkWidget *widget, cairo_t *cr, gpointer *ptr);


void init_itin_rework_test(GtkBuilder *builder) {
	ir_window = gtk_builder_get_object(builder, "window");
	da_ir_graphing = gtk_builder_get_object(builder, "da_ir_graphing");
	cb_ir_system = gtk_builder_get_object(builder, "cb_ir_system");
	cb_ir_central_body = gtk_builder_get_object(builder, "cb_ir_central_body");
	cb_ir_depbody = gtk_builder_get_object(builder, "cb_ir_depbody");
	cb_ir_arrbody = gtk_builder_get_object(builder, "cb_ir_arrbody");
	tf_ir_mindepdate = gtk_builder_get_object(builder, "tf_ir_mindepdate");
	tf_ir_maxdepdate = gtk_builder_get_object(builder, "tf_ir_maxdepdate");
	tf_ir_mindur = gtk_builder_get_object(builder, "tf_ir_maxarrdate");
	tf_ir_maxdur = gtk_builder_get_object(builder, "tf_ir_maxdur");

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

	ir_data = data_array2_create();

	ir_screen = new_screen(GTK_WIDGET(da_ir_graphing), &on_ir_screen_resize, NULL, NULL, NULL, NULL);
	set_screen_background_color(ir_screen, 0.15, 0.15, 0.15);
}

void on_ir_screen_resize(GtkWidget *widget, cairo_t *cr, gpointer *ptr) {
	if((Screen*)ptr == ir_screen) {
		resize_screen(ir_screen);
		clear_screen(ir_screen);
		draw_plot_from_data_array(ir_screen->static_layer.cr, ir_screen->width, ir_screen->height, ir_data);
		draw_screen(ir_screen);
	}
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

bool almost_colinear_f(Vector2 p0, Vector2 p1, Vector2 p2, double tol) {
	double abx = p1.x - p0.x;
	double aby = p1.y - p0.y;
	double acx = p2.x - p0.x;
	double acy = p2.y - p0.y;

	double area2 = abx * acy - aby * acx;
	return (area2 * area2) <= (tol * tol * (abx * abx + aby * aby));
}

double calc_next_x(DataArray2 *arr, int index_0) {
	Vector2 *data = &(data_array2_get_data(arr)[index_0]);
	size_t num_data = data_array2_size(arr)-index_0;
	// DataArray2 *data_derivative = data_array2_create();
	//
	//
	//
	// for (int i = 0; i < num_data-1; i++) {
	// 	double x = (data[i].x + data[i+1].x) / 2;
	// 	double dy = (data[i+1].y - data[i].y) / (data[i+1].x - data[i].x);
	// 	data_array2_append_new(data_derivative, x, dy);
	// }
	//
	// Vector2 *dxdy = data_array2_get_data(data_derivative);
	// print_data_array2(data_derivative, "dur", "ddv");

	for (int i = 0; i < num_data-2; i++) {
		if (data[i+1].x-data[i].x < 1 && data[i+2].x-data[i+1].x < 1) continue;
		if (!almost_colinear_f(data[i], data[i+1], data[i+2], 1e-1)) {
			// data_array2_free(data_derivative);
			if (data[i+1].x-data[i].x > data[i+2].x-data[i+1].x) {
				printf("- %d  %f  %f  %f   -  %f\n", i, data[i].x, data[i+1].x, data[i+2].x, (data[i+1].x+data[i].x)/2);
				return (data[i+1].x+data[i].x)/2;
			} else {
				printf("+ %d %f  %f  %f   -  %f\n", i, data[i].x, data[i+1].x, data[i+2].x, (data[i+2].x+data[i+1].x)/2);
				return (data[i+2].x+data[i+1].x)/2;
			}
		}
	}

	// data_array2_free(data_derivative);

	return -1;
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

	Body *dep_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_depbody))];
	Body *arr_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_arrbody))];

	double dep_periapsis = dep_body->atmo_alt + ir_dep_periapsis;

	clear_screen(ir_screen);
	data_array2_clear(ir_data);

	double jd_dep = min_dep;
	OSV osv0 = ir_system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(dep_body->orbit, jd_dep) :
					osv_from_ephem(dep_body->ephem, dep_body->num_ephems, jd_dep, ir_system->cb);

	printf("%f    %f\n%f    %f\n%s    %s\n", min_dep, max_dep, min_dur, max_dur, dep_body->name, arr_body->name);

	OSV osv_arr0 = ir_system->prop_method == ORB_ELEMENTS ?
				   osv_from_elements(arr_body->orbit, jd_dep) :
				   osv_from_ephem(arr_body->ephem, arr_body->num_ephems, jd_dep, ir_system->cb);
	double next_conjunction_dt, next_opposition_dt;
	calc_time_to_next_conjunction_and_opposition(osv0.r, osv_arr0, ir_system->cb, &next_conjunction_dt, &next_opposition_dt);
	Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, ir_system->cb);
	double period_arr0 = calc_orbital_period(arr0);
	double dt0, dt1;
	if(next_conjunction_dt < next_opposition_dt) {
		dt0 = next_opposition_dt - period_arr0;
		dt1 = next_conjunction_dt;
	} else {
		dt0 = next_conjunction_dt - period_arr0;
		dt1 = next_opposition_dt;
	}


	int max_new_steps = 10000;
	double min_dt = min_dur*86400;
	double max_dt = max_dur*86400;
	double min_dt_step = 1;
	int counter = 0;
	double last_dt, dt, t1;

	while(dt0 < max_dt && counter < max_new_steps) {
		if (data_array2_size(ir_data) == 0) dt = dt0;
		else dt = dt1;
		int index_0 = (int) data_array2_size(ir_data)-1;
		if (index_0 < 0) index_0 = 0;

		for(int i = 0; i < max_new_steps/10; i++) {
			if(dt1 < min_dt) break;
			if(dt < min_dt) dt = min_dt;
			if(dt > max_dt) dt = max_dt;
			printf("%f  %f  %f  %f  %f\n", min_dt, max_dt, dt0, dt1, dt);

			double jd_arr = jd_dep + dt / 86400;

			OSV osv1 = ir_system->prop_method == ORB_ELEMENTS ?
						osv_from_elements(arr_body->orbit, jd_arr) :
						osv_from_ephem(arr_body->ephem, arr_body->num_ephems, jd_arr, ir_system->cb);

			Lambert3 tf = calc_lambert3(osv0.r, osv1.r, (jd_arr - jd_dep) * 86400, ir_system->cb);

			double vinf = fabs(mag_vec3(subtract_vec3(tf.v0, osv0.v)));
			double dv_dep = dv_circ(dep_body,alt2radius(dep_body, dep_periapsis),vinf);

			data_array2_insert_new(ir_data, dt/86400, dv_dep);
			// printf("%f  %f\n", dt/86400, dv_dep);
			counter++;

			if (dt == dt0 || dt == min_dt) dt = dt1;
			else if (dt == dt1 || dt == max_dt) dt = (dt + (dt0 > min_dt ? dt0 : min_dt)) / 2;
			else dt = calc_next_x(ir_data, index_0)*86400;
			if (dt < 0) break;
		}
		double temp = dt1;
		dt1 = dt0 + period_arr0;
		dt0 = temp;
	}

	print_data_array2(ir_data, "dur", "dv");

	printf("%d\n", counter);

	draw_plot_from_data_array(ir_screen->static_layer.cr, ir_screen->width, ir_screen->height, ir_data);
	// draw_scatter_from_data_array(ir_screen->static_layer.cr, ir_screen->width, ir_screen->height, ir_data);


	int num_points = 10000;
	DataArray2 *compare_data = data_array2_create();

	for(int i = 0; i < num_points; i++) {
		double dur = (max_dur-min_dur)/num_points*i + min_dur;
		double jd_arr = min_dep + dur;
		OSV osv1 = ir_system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(arr_body->orbit, jd_arr) :
					osv_from_ephem(arr_body->ephem, arr_body->num_ephems, jd_arr, ir_system->cb);

		Lambert3 tf = calc_lambert3(osv0.r, osv1.r, (jd_arr - jd_dep) * 86400, ir_system->cb);

		double vinf = fabs(mag_vec3(subtract_vec3(tf.v0, osv0.v)));
		double dv_dep = dv_circ(dep_body,alt2radius(dep_body, dep_periapsis),vinf);

		data_array2_append_new(compare_data, dur, dv_dep);
	}

	Vector2 *data = data_array2_get_data(compare_data);
	size_t num_data = data_array2_size(compare_data);
	DataArray2 *data_derivative = data_array2_create();

	for (int i = 0; i < num_data-1; i++) {
		double x = (data[i].x + data[i+1].x) / 2;
		double dy = (data[i+1].y - data[i].y) / (data[i+1].x - data[i].x);
		data_array2_append_new(data_derivative, x, dy);
	}
	draw_plot_from_data_array(ir_screen->static_layer.cr, ir_screen->width, ir_screen->height, compare_data);
	// draw_scatter_from_data_array(ir_screen->static_layer.cr, ir_screen->width, ir_screen->height, compare_data);
	// draw_plot_from_data_array(ir_screen->static_layer.cr, ir_screen->width, ir_screen->height, data_derivative);

	data_array2_free(data_derivative);
	data_array2_free(compare_data);
	draw_screen(ir_screen);
}
