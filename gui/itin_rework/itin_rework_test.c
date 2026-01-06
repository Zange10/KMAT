#include "itin_rework_test.h"
#include "itin_rework_tools.h"

#include "gui/gui_manager.h"
#include "gui/gui_tools/screen.h"
#include "gui/drawing.h"
#include "geometrylib.h"
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

CelestSystem *ir_system;
Screen *ir_screen0;
Screen *ir_screen1;

double ir_dep_periapsis = 50e3;

DataArray2 *ir_data0;
DataArray2 *ir_data1;

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
	ir_data1 = data_array2_create();

	ir_screen0 = new_screen(GTK_WIDGET(da_ir_graphing0), &on_ir_screen_resize, NULL, NULL, NULL, NULL);
	set_screen_background_color(ir_screen0, 0.15, 0.15, 0.15);

	ir_screen1 = new_screen(GTK_WIDGET(da_ir_graphing1), &on_ir_screen_resize, NULL, NULL, NULL, NULL);
	set_screen_background_color(ir_screen1, 0.15, 0.15, 0.15);
}

void on_ir_screen_resize(GtkWidget *widget, cairo_t *cr, gpointer *ptr) {
	if((Screen*)ptr == ir_screen0) {
		resize_screen(ir_screen0);
		clear_screen(ir_screen0);
		draw_plot_from_data_array(ir_screen0->static_layer.cr, ir_screen0->width, ir_screen0->height, ir_data0);
		draw_screen(ir_screen0);
	}

	if((Screen*)ptr == ir_screen1) {
		resize_screen(ir_screen1);
		clear_screen(ir_screen1);
		draw_plot_from_data_array(ir_screen1->static_layer.cr, ir_screen1->width, ir_screen1->height, ir_data1);
		draw_screen(ir_screen1);
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

	Body *dep_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_depbody))];
	Body *arr_body = ir_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ir_arrbody))];

	double dep_periapsis = dep_body->atmo_alt + ir_dep_periapsis;

	clear_screen(ir_screen0);
	clear_screen(ir_screen1);

	double jd_dep = min_dep;

	struct timeval start, end;
	double elapsed_time;
	gettimeofday(&start, NULL);  // Record the ending time

	int num_iterations = (int) target_numdeps;
	DataArray2 *new_data = NULL;
	int num_deps = num_iterations;
	struct ItinStep **departures = (struct ItinStep**) malloc(num_deps * sizeof(struct ItinStep*));
	for(int i = 0; i < num_deps; i++) departures[i] = (struct ItinStep*) malloc(sizeof(struct ItinStep));
	for(int i = 0; i < num_deps; i++) departures[i]->num_next_nodes = 0;
	for (int i = 0; i < num_deps; i++) {
		DataArray2 *temp_data = calc_porkchop_line(departures[i], dep_body, arr_body, ir_system, jd_dep+i*2, min_dur, max_dur, dep_periapsis, max_dep_dv, tolerance);
		if (!new_data) new_data = temp_data;
		else data_array2_free(temp_data);
	}

	gettimeofday(&end, NULL);  // Record the ending time
	elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
	printf("----- | Total elapsed time: %.3f s | ---------\n", elapsed_time);

	gettimeofday(&start, NULL);  // Record the ending time

	int old_num_points = 500;
	DataArray2 *old_data = NULL;
	for (int i = num_iterations-1; i >= 0; i--) {
		DataArray2 *temp_data = calc_porkchop_line_static(dep_body, arr_body, ir_system, jd_dep+i*5, min_dur, max_dur, dep_periapsis, old_num_points);
		if (!old_data) old_data = temp_data;
		else data_array2_free(temp_data);
	}
	gettimeofday(&end, NULL);  // Record the ending time
	elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
	printf("----- | Total elapsed time: %.3f s | ---------\n", elapsed_time);

	int num_points = 10000;
	DataArray2 *compare_data = calc_porkchop_line_static(dep_body, arr_body, ir_system, jd_dep, min_dur, max_dur, dep_periapsis, num_points);

	Vector2 *data = data_array2_get_data(compare_data);
	size_t num_data = data_array2_size(compare_data);
	DataArray2 *data_derivative = data_array2_create();

	for (int i = 0; i < num_data-1; i++) {
		double x = (data[i].x + data[i+1].x) / 2;
		double dy = (data[i+1].y - data[i].y) / (data[i+1].x - data[i].x);
		data_array2_append_new(data_derivative, x, dy);
	}


	DataArray2 *data_diff = data_array2_create();
	data = data_array2_get_data(new_data);
	for (int i = 0; i < num_points; i++) {
		int index = 0;
		double x = data_array2_get_data(compare_data)[i].x;
		double y = data_array2_get_data(compare_data)[i].y;
		for (int j = 0; j < data_array2_size(new_data)-1; j++) {
			if (data[j+1].x > x) break;
			index++;
		}
		double m = (data[index+1].y - data[index].y)/(data[index+1].x - data[index].x);
		double ip_y = data[index].y + m*(x-data[index].x);
		data_array2_append_new(data_diff, x, ip_y-y);
	}


	DataArray2 *data_diff_old = data_array2_create();
	data = data_array2_get_data(old_data);
	for (int i = 0; i < num_points; i++) {
		int index = 0;
		double x = data_array2_get_data(compare_data)[i].x;
		double y = data_array2_get_data(compare_data)[i].y;
		for (int j = 0; j < data_array2_size(old_data)-1; j++) {
			if (data[j+1].x > x) break;
			index++;
		}
		double m = (data[index+1].y - data[index].y)/(data[index+1].x - data[index].x);
		double ip_y = data[index].y + m*(x-data[index].x);
		data_array2_append_new(data_diff_old, x, ip_y-y);
	}

	// ir_data0 = new_data;
	// ir_data1 = data_diff;
	ir_data0 = new_data;
	ir_data1 = compare_data;
	// ir_data0 = data_diff;
	// ir_data1 = data_diff_old;

	// remove departure dates with no valid itinerary
	for(int i = 0; i < num_deps; i++) {
		if(departures[i] == NULL || departures[i]->num_next_nodes == 0) {
			num_deps--;
			for(int j = i; j < num_deps; j++) {
				departures[j] = departures[j + 1];
			}
			i--;
		}
	}

	int num_itins = 0, tot_num_itins = 0;
	for(int i = 0; i < num_deps; i++) num_itins += get_number_of_itineraries(departures[i]);
	for(int i = 0; i < num_deps; i++) tot_num_itins += get_total_number_of_stored_steps(departures[i]);

	printf("\n%d itineraries found!\nNumber of Nodes: %d\n", num_itins, tot_num_itins);

	int index = 0;
	struct ItinStep **arrivals = (struct ItinStep**) malloc(num_itins * sizeof(struct ItinStep*));
	for(int i = 0; i < num_deps; i++) store_itineraries_in_array(departures[i], arrivals, &index);
	struct PorkchopAnalyzerPoint *pp = malloc(num_itins * sizeof(struct PorkchopAnalyzerPoint));
	for(int i = 0; i < num_itins; i++) {
		pp[i].data = create_porkchop_point(arrivals[i], get_first(arrivals[0])->body->atmo_alt + dep_periapsis, arrivals[0]->body->atmo_alt + dep_periapsis);
		pp[i].inside_filter = 1;
		pp[i].group = NULL;
	}
	free(arrivals);

	draw_plot_from_data_array(ir_screen0->static_layer.cr, ir_screen0->width, ir_screen0->height, ir_data0);
	// draw_scatter_from_data_array(ir_screen0->static_layer.cr, ir_screen0->width, ir_screen0->height, ir_data0);

	// draw_plot_from_data_array(ir_screen1->static_layer.cr, ir_screen1->width, ir_screen1->height, ir_data1);
	// draw_scatter_from_data_array(ir_screen1->static_layer.cr, ir_screen1->width, ir_screen1->height, ir_data1);

	draw_porkchop(ir_screen1->static_layer.cr, ir_screen1->width, ir_screen1->height, pp, num_itins, TF_FLYBY, 0);



	data_array2_free(data_derivative);
	data_array2_free(data_diff);
	draw_screen(ir_screen0);
	draw_screen(ir_screen1);
}
