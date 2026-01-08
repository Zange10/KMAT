#include "itin_rework_test.h"
#include "itin_rework_tools.h"

#include "gui/gui_manager.h"
#include "gui/gui_tools/screen.h"
#include "gui/drawing.h"
#include "mesh.h"
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
GObject *tf_ir_pcgroup;

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
	clear_screen(ir_screen1);

	double jd_dep = min_dep;
	int num_iterations = (int) target_numdeps;

	struct timeval start, end;
	double elapsed_time;
	gettimeofday(&start, NULL);  // Record the ending time

	int num_of_groups = 50;
	DepartureGroup **departure_groups = malloc(num_of_groups*sizeof(DepartureGroup *));
	int counter = 0;
	for (int i = 0; i < num_of_groups; i++) {
		departure_groups[counter] = malloc(sizeof(DepartureGroup));
		departure_groups[counter]->dep_body = dep_body;
		departure_groups[counter]->arr_body = arr_body;
		departure_groups[counter]->num_departures = num_iterations;
		departure_groups[counter]->system = ir_system;

		calc_group_porkchop(departure_groups[counter], i-10, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_dep_dv, tolerance);
		if (departure_groups[counter]->num_departures == 0) {free(departure_groups[counter]);}
		else counter++;
	}

	printf("%d\n", counter);

	gettimeofday(&end, NULL);  // Record the ending time
	elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
	printf("----- | Total elapsed time: %.3f s | ---------\n", elapsed_time);


	gettimeofday(&start, NULL);  // Record the ending time

	DataArray2 *new_data = NULL;
	int num_deps = num_iterations;
	struct ItinStep **departures = (struct ItinStep**) malloc(num_deps * sizeof(struct ItinStep*));
	for(int i = 0; i < num_deps; i++) departures[i] = (struct ItinStep*) malloc(sizeof(struct ItinStep));
	for(int i = 0; i < num_deps; i++) departures[i]->num_next_nodes = 0;
	for (int i = 0; i < num_deps; i++) {
		DataArray2 *temp_data = calc_porkchop_line(departures[i], dep_body, arr_body, ir_system, jd_dep+i*5, min_dur, max_dur, dep_periapsis, max_dep_dv, tolerance);
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

	double opp_conj_gradient = calc_opposition_conjunction_gradient(dep_body, arr_body, ir_system, jd_dep);

	DataArray2 *opp_data = data_array2_create();
	DataArray2 *conj_data = data_array2_create();
	DataArray2 *opp_diff_data = data_array2_create();
	DataArray2 *conj_diff_data = data_array2_create();
	DataArray2 *opp_err_data = data_array2_create();
	DataArray2 *conj_err_data = data_array2_create();
	double conj_diff_avg = 0;
	double opp_diff_avg = 0;
	double last_conjunction_dt, last_opposition_dt;
	int opp_counter = 0, conj_counter = 0;
	for (int i = 0; i < num_iterations; i++) {
		double dx = 5;
		double dep = min_dep + i*dx;
		OSV osv0 = ir_system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(dep_body->orbit, dep) :
					osv_from_ephem(dep_body->ephem, dep_body->num_ephems, dep, ir_system->cb);

		OSV osv_arr0 = ir_system->prop_method == ORB_ELEMENTS ?
					   osv_from_elements(arr_body->orbit, dep) :
					   osv_from_ephem(arr_body->ephem, arr_body->num_ephems, dep, ir_system->cb);
		double next_conjunction_dt, next_opposition_dt;
		calc_time_to_next_conjunction_and_opposition(osv0.r, osv_arr0, ir_system->cb, &next_conjunction_dt, &next_opposition_dt);

		if (i > 0) {
			double opp_guess = last_opposition_dt + dx*86400*opp_conj_gradient;
			double conj_guess = last_conjunction_dt + dx*86400*opp_conj_gradient;
			double period = calc_orbital_period(constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, ir_system->cb));

			while (opp_guess-next_opposition_dt   >  0.5 * period) next_opposition_dt  += period;
			while (opp_guess-next_opposition_dt   < -0.5 * period) next_opposition_dt  -= period;
			while (conj_guess-next_conjunction_dt >  0.5 * period) next_conjunction_dt += period;
			while (conj_guess-next_conjunction_dt < -0.5 * period) next_conjunction_dt -= period;
			data_array2_append_new(opp_err_data, dep-min_dep, (opp_guess-next_opposition_dt)/86400);
			data_array2_append_new(conj_err_data, dep-min_dep, (conj_guess-next_conjunction_dt)/86400);
		}
		data_array2_append_new(opp_data, dep-min_dep, next_opposition_dt/86400);
		data_array2_append_new(conj_data, dep-min_dep, next_conjunction_dt/86400);

		if (i > 0) {
			Vector2 *data_v = data_array2_get_data(opp_data);
			double dxdy = (data_v[i].y - data_v[i-1].y)/(data_v[i].x - data_v[i-1].x);

			data_array2_append_new(opp_diff_data, dep-min_dep, dxdy);
			opp_diff_avg += dxdy;
			opp_counter++;

			data_v = data_array2_get_data(conj_data);
			dxdy = (data_v[i].y - data_v[i-1].y)/(data_v[i].x - data_v[i-1].x);
			data_array2_append_new(conj_diff_data, dep-min_dep, dxdy);
			conj_diff_avg += dxdy;
			conj_counter++;
		}
		last_conjunction_dt = next_conjunction_dt;
		last_opposition_dt = next_opposition_dt;
	}

	opp_diff_avg /= opp_counter;
	conj_diff_avg /= conj_counter;

	print_data_array2(opp_data, "dep", "opp");
	print_data_array2(conj_data, "dep", "conj");
	print_data_array2(opp_diff_data, "dep", "d_opp");
	print_data_array2(conj_diff_data, "dep", "d_conj");
	printf("%f   %f   %f\n", opp_diff_avg, conj_diff_avg, opp_conj_gradient);




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

	int num_itins0 = 0, tot_num_itins = 0;
	for(int i = 0; i < num_deps; i++) num_itins0 += get_number_of_itineraries(departures[i]);
	for(int i = 0; i < num_deps; i++) tot_num_itins += get_total_number_of_stored_steps(departures[i]);

	printf("\n%d itineraries found!\nNumber of Nodes: %d\n", num_itins0, tot_num_itins);

	int index = 0;
	struct ItinStep **arrivals = (struct ItinStep**) malloc(num_itins0 * sizeof(struct ItinStep*));
	for(int i = 0; i < num_deps; i++) store_itineraries_in_array(departures[i], arrivals, &index);
	struct PorkchopAnalyzerPoint *pp0 = malloc(num_itins0 * sizeof(struct PorkchopAnalyzerPoint));
	for(int i = 0; i < num_itins0; i++) {
		pp0[i].data = create_porkchop_point(arrivals[i], get_first(arrivals[0])->body->atmo_alt + dep_periapsis, arrivals[0]->body->atmo_alt + dep_periapsis);
		pp0[i].inside_filter = 1;
		pp0[i].group = NULL;
	}
	free(arrivals);



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



	departures = departure_groups[pcgroup]->departures;
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

	int num_itins1 = 0; tot_num_itins = 0;
	for(int i = 0; i < departure_groups[pcgroup]->num_departures; i++) num_itins1 += get_number_of_itineraries(departures[i]);
	for(int i = 0; i < departure_groups[pcgroup]->num_departures; i++) tot_num_itins += get_total_number_of_stored_steps(departures[i]);

	printf("\n%d itineraries found!\nNumber of Nodes: %d\n", num_itins1, tot_num_itins);

	index = 0;
	arrivals = (struct ItinStep**) malloc(num_itins1 * sizeof(struct ItinStep*));
	for(int i = 0; i < departure_groups[pcgroup]->num_departures; i++) store_itineraries_in_array(departures[i], arrivals, &index);
	struct PorkchopAnalyzerPoint *pp1 = malloc(num_itins1 * sizeof(struct PorkchopAnalyzerPoint));
	for(int i = 0; i < num_itins1; i++) {
		pp1[i].data = create_porkchop_point(arrivals[i], get_first(arrivals[0])->body->atmo_alt + dep_periapsis, arrivals[0]->body->atmo_alt + dep_periapsis);
		pp1[i].inside_filter = 1;
		pp1[i].group = NULL;
	}
	free(arrivals);

	// ir_data0 = new_data;
	// ir_data1 = data_diff;
	// ir_data0 = new_data;
	// ir_data1 = compare_data;
	// ir_data0 = data_diff;
	// ir_data1 = data_diff_old;
	ir_data0 = opp_data;
	// ir_data1 = conj_data;
	// ir_data0 = opp_diff_data;
	ir_data1 = opp_err_data;

	// draw_plot_from_data_array(ir_screen0->static_layer.cr, ir_screen0->width, ir_screen0->height, ir_data0);
	// draw_scatter_from_data_array(ir_screen0->static_layer.cr, ir_screen0->width, ir_screen0->height, ir_data0);

	// draw_plot_from_data_array(ir_screen1->static_layer.cr, ir_screen1->width, ir_screen1->height, ir_data1);
	// draw_scatter_from_data_array(ir_screen1->static_layer.cr, ir_screen1->width, ir_screen1->height, ir_data1);

	draw_porkchop(ir_screen0->static_layer.cr, ir_screen0->width, ir_screen0->height, pp1, num_itins1, TF_FLYBY, 0);
	draw_porkchop(ir_screen1->static_layer.cr, ir_screen1->width, ir_screen1->height, pp0, num_itins0, TF_FLYBY, 0);
	// draw_plot_from_data_array(ir_screen1->static_layer.cr, ir_screen1->width, ir_screen1->height, grad_data);

	data_array2_free(data_derivative);
	data_array2_free(data_diff);
	draw_screen(ir_screen0);
	draw_screen(ir_screen1);
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
	clear_screen(ir_screen1);

	double jd_dep = min_dep;
	int num_iterations = (int) target_numdeps;

	struct timeval start, end;
	double elapsed_time;
	gettimeofday(&start, NULL);  // Record the ending time

	int num_of_groups = 50;
	DepartureGroup **departure_groups = malloc(num_of_groups*sizeof(DepartureGroup *));
	int counter = 0;
	for (int i = 0; i < num_of_groups; i++) {
		departure_groups[counter] = malloc(sizeof(DepartureGroup));
		departure_groups[counter]->dep_body = dep_body;
		departure_groups[counter]->arr_body = arr_body;
		departure_groups[counter]->num_departures = num_iterations;
		departure_groups[counter]->system = ir_system;

		calc_group_porkchop(departure_groups[counter], i-10, min_dep, max_dep, max_dep+max_dur, min_dur, max_dur, dep_periapsis, max_dep_dv, tolerance);
		if (departure_groups[counter]->num_departures == 0) {free(departure_groups[counter]);}
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
	for (int i = 0; i < departure_groups[pcgroup]->num_departures; i++) {
		struct ItinStep *step = departure_groups[pcgroup]->departures[i];
		if (step->num_next_nodes == -1) {
			double x = -1e10;
			double y = 0;
			data_array2_append_new(step_pos, x, y);
			steps[counter] = NULL;
			counter++;
		}
		if (step->num_next_nodes < 0) step->num_next_nodes = 0;

		for (int j = 0; j < step->num_next_nodes; j++) {
			double x = step->date;
			double y = step->next[j]->date - step->date;
			data_array2_append_new(step_pos, x, y);
			steps[counter] = step->next[j];
			counter++;
		}
	}


	MeshGrid2 grid = create_mesh_grid(step_pos, (void**) steps);
	for (int i = 0; i < grid.num_cols; i++) {
		if (grid.num_col_rows[i] == 0) {
			printf("\n%4d:   ---", i); continue;
		}
		printf("\n%4d: ", i);
		for (int j = 0; j < grid.num_col_rows[i]; j++) {
			printf("%6.0f, ", grid.points[i][j]->pos.y);
		}
	}
	printf("\n");
	Mesh2 mesh = create_mesh_from_grid(grid);


	for(int i = 0; i < mesh.num_points; i++) {
		MeshPoint2 *point = mesh.points[i];
		struct ItinStep *step = (struct ItinStep *) point->data;
		point->pos.x = step->date;
		point->pos.y = step->date - get_first(step)->date;
	}

	gettimeofday(&end, NULL);  // Record the ending time
	elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
	printf("----- | Total elapsed time: %.3f s | ---------\n", elapsed_time);


	resize_pcmesh_to_fit(mesh, ir_screen1->width, ir_screen1->height);
	draw_mesh(ir_screen1->static_layer.cr, &mesh);




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
	struct ItinStep **arrivals = (struct ItinStep**) malloc(num_itins1 * sizeof(struct ItinStep*));
	for(int i = 0; i < departure_groups[pcgroup]->num_departures; i++) store_itineraries_in_array(departures[i], arrivals, &index);
	struct PorkchopAnalyzerPoint *pp1 = malloc(num_itins1 * sizeof(struct PorkchopAnalyzerPoint));
	for(int i = 0; i < num_itins1; i++) {
		pp1[i].data = create_porkchop_point(arrivals[i], get_first(arrivals[0])->body->atmo_alt + dep_periapsis, arrivals[0]->body->atmo_alt + dep_periapsis);
		pp1[i].inside_filter = 1;
		pp1[i].group = NULL;
	}
	free(arrivals);

	draw_porkchop(ir_screen0->static_layer.cr, ir_screen0->width, ir_screen0->height, pp1, num_itins1, TF_FLYBY, 0);


	draw_screen(ir_screen0);
	draw_screen(ir_screen1);
}
