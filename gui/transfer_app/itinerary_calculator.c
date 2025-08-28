#include "itinerary_calculator.h"
#include "orbit_calculator/transfer_calc.h"
#include "gui/gui_manager.h"
#include "gui/settings.h"
#include "gui/info_win_manager.h"
#include "tools/file_io.h"


GObject *tf_ic_window;
GObject *cb_ic_system;
GObject *cb_ic_central_body;
GObject *lb_ic_central_body;
GObject *cb_ic_depbody;
GObject *cb_ic_arrbody;
GObject *tf_ic_mindepdate;
GObject *tf_ic_maxdepdate;
GObject *tf_ic_maxarrdate;
GObject *tf_ic_maxdur;
GObject *cb_ic_transfertype;
GObject *tf_ic_totdv;
GObject *tf_ic_depdv;
GObject *tf_ic_satdv;
GObject *vp_ic_fbbodies;
GtkWidget *grid_ic_fbbodies;

CelestSystem *ic_system;

double ic_dep_periapsis = 50e3;
double ic_arr_periapsis = 50e3;

GtkWidget * ic_update_seq_body_grid(GObject *viewport, GtkWidget *grid);

void init_itinerary_calculator(GtkBuilder *builder) {
	tf_ic_window = gtk_builder_get_object(builder, "window");
	cb_ic_system = gtk_builder_get_object(builder, "cb_ic_system");
	cb_ic_central_body = gtk_builder_get_object(builder, "cb_ic_central_body");
	lb_ic_central_body = gtk_builder_get_object(builder, "lb_ic_central_body");
	cb_ic_depbody = gtk_builder_get_object(builder, "cb_ic_depbody");
	cb_ic_arrbody = gtk_builder_get_object(builder, "cb_ic_arrbody");
	tf_ic_mindepdate = gtk_builder_get_object(builder, "tf_ic_mindepdate");
	tf_ic_maxdepdate = gtk_builder_get_object(builder, "tf_ic_maxdepdate");
	tf_ic_maxarrdate = gtk_builder_get_object(builder, "tf_ic_maxarrdate");
	tf_ic_maxdur = gtk_builder_get_object(builder, "tf_ic_maxdur");
	cb_ic_transfertype = gtk_builder_get_object(builder, "cb_ic_transfertype");
	tf_ic_totdv = gtk_builder_get_object(builder, "tf_ic_totdv");
	tf_ic_depdv = gtk_builder_get_object(builder, "tf_ic_depdv");
	tf_ic_satdv = gtk_builder_get_object(builder, "tf_ic_satdv");
	vp_ic_fbbodies = gtk_builder_get_object(builder, "vp_ic_fbbodies");

	ic_system = NULL;

	create_combobox_dropdown_text_renderer(cb_ic_system, GTK_ALIGN_CENTER);
	create_combobox_dropdown_text_renderer(cb_ic_central_body, GTK_ALIGN_CENTER);
	create_combobox_dropdown_text_renderer(cb_ic_depbody, GTK_ALIGN_CENTER);
	create_combobox_dropdown_text_renderer(cb_ic_arrbody, GTK_ALIGN_CENTER);
	update_system_dropdown(GTK_COMBO_BOX(cb_ic_system));
	if(get_num_available_systems() > 0) {
		ic_system = get_available_systems()[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ic_system))];
		update_central_body_dropdown(GTK_COMBO_BOX(cb_ic_central_body), ic_system);
		update_body_dropdown(GTK_COMBO_BOX(cb_ic_depbody), ic_system);
		update_body_dropdown(GTK_COMBO_BOX(cb_ic_arrbody), ic_system);
		grid_ic_fbbodies = ic_update_seq_body_grid(vp_ic_fbbodies, grid_ic_fbbodies);
	}
}

void ic_change_date_type(enum DateType old_date_type, enum DateType new_date_type) {
	change_text_field_date_type(tf_ic_mindepdate, old_date_type, new_date_type);
	change_text_field_date_type(tf_ic_maxdepdate, old_date_type, new_date_type);
	change_text_field_date_type(tf_ic_maxarrdate, old_date_type, new_date_type);
}


GtkWidget * ic_update_seq_body_grid(GObject *viewport, GtkWidget *grid) {
	if (grid != NULL && GTK_WIDGET(viewport) == gtk_widget_get_parent(grid)) {
		gtk_container_remove(GTK_CONTAINER(viewport), grid);
	}

	grid = gtk_grid_new();

	// Create labels and buttons and add them to the grid
	for (int body_idx = 0; body_idx < ic_system->num_bodies; body_idx++) {
		int row = body_idx;
		GtkWidget *widget;
		// Create a show body check button
		widget = gtk_check_button_new_with_label(ic_system->bodies[body_idx]->name);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 1);
		gtk_widget_set_halign(widget, GTK_ALIGN_START);

		// Set the label in the grid at the specified row and column
		gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
	}
	gtk_container_add (GTK_CONTAINER (viewport), grid);
	gtk_widget_show_all(GTK_WIDGET(viewport));

	return grid;
}

int get_num_selected_bodies_from_grid(GtkWidget *grid) {
	int num_bodies = 0;
	for(int i = 0; i < ic_system->num_bodies; i++) {
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_grid_get_child_at(GTK_GRID(grid), 0, i))))
			num_bodies++;
	}
	return num_bodies;
}

Body ** get_bodies_from_grid(GtkWidget *grid, int num_bodies) {
	Body **bodies = malloc(num_bodies * sizeof(Body*));
	int idx = 0;
	for(int i = 0; i < ic_system->num_bodies; i++) {
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_grid_get_child_at(GTK_GRID(grid), 0, i)))) {
			bodies[idx] = ic_system->bodies[i]; idx++;
		}
	}

	return bodies;
}



struct Itin_Calc_Data ic_calc_data;
struct Itin_Calc_Results ic_results;

void save_itineraries_ic(struct ItinStep **departures, int num_deps, int num_nodes, int num_itins) {
	if(departures == NULL || num_deps == 0) return;
	char filepath[255];
	if(!get_path_from_file_chooser(filepath,  ".itins", GTK_FILE_CHOOSER_ACTION_SAVE, "")) return;
	store_itineraries_in_bfile(departures, num_nodes, num_deps, num_itins, ic_calc_data, ic_system, filepath, get_current_bin_file_type());
}

gboolean end_ic_calc_thread() {
	end_sc_ic_progress_window();
	gtk_widget_set_sensitive(GTK_WIDGET(tf_ic_window), 1);

	save_itineraries_ic(ic_results.departures, ic_results.num_deps, ic_results.num_nodes, ic_results.num_itins);
	for(int i = 0; i < ic_results.num_deps; i++) free_itinerary(ic_results.departures[i]);
	free(ic_results.departures);
	free(ic_calc_data.seq_info.to_target.flyby_bodies);
	if(ic_results.num_deps == 0) show_msg_window("No itineraries found!");
	return G_SOURCE_REMOVE;
}

void ic_calc_thread() {
	char *string;

	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ic_mindepdate));
	ic_calc_data.jd_min_dep = convert_date_JD(date_from_string(string, get_settings_datetime_type()));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ic_maxdepdate));
	ic_calc_data.jd_max_dep = convert_date_JD(date_from_string(string, get_settings_datetime_type()));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ic_maxarrdate));
	ic_calc_data.jd_max_arr = convert_date_JD(date_from_string(string, get_settings_datetime_type()));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ic_maxdur));
	ic_calc_data.max_duration = strtod(string, NULL);
	if(get_settings_datetime_type() == DATE_KERBAL) ic_calc_data.max_duration /= 4;	// kerbal day is 4 times shorter (24h/6h)

	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ic_totdv));
	ic_calc_data.dv_filter.max_totdv = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ic_depdv));
	ic_calc_data.dv_filter.max_depdv = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ic_satdv));
	ic_calc_data.dv_filter.max_satdv = strtod(string, NULL);
	string = (char*) gtk_combo_box_get_active_id(GTK_COMBO_BOX(cb_ic_transfertype));
	ic_calc_data.dv_filter.last_transfer_type = (int) strtol(string, NULL, 10);

	ic_calc_data.dv_filter.dep_periapsis = ic_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ic_depbody))]->atmo_alt + ic_dep_periapsis;
	ic_calc_data.dv_filter.arr_periapsis = ic_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ic_arrbody))]->atmo_alt + ic_arr_periapsis;

	ic_calc_data.num_deps_per_date = 500;
	ic_calc_data.step_dep_date = 1;
	ic_calc_data.max_num_waiting_orbits = 0;

	// Make sure arrival body is a fly-by body
	Body *arr_body = ic_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ic_arrbody))];
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_grid_get_child_at(GTK_GRID(grid_ic_fbbodies), 0, get_body_system_id(arr_body, ic_system))), 1);

	struct ItinSequenceInfoToTarget seq_info = {
			.system = ic_system,
			.dep_body = ic_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ic_depbody))],
			.arr_body = arr_body,
			.num_flyby_bodies = get_num_selected_bodies_from_grid(grid_ic_fbbodies),
	};
	seq_info.flyby_bodies = get_bodies_from_grid(grid_ic_fbbodies, seq_info.num_flyby_bodies);
	ic_calc_data.seq_info.to_target = seq_info;

	ic_results = search_for_itineraries(ic_calc_data);

	// GUI stuff needs to happen in main thread
	g_idle_add((GSourceFunc)end_ic_calc_thread, NULL);
}

G_MODULE_EXPORT void on_calc_ic() {
	if(ic_system == NULL) return;
	gtk_widget_set_sensitive(GTK_WIDGET(tf_ic_window), 0);
	g_thread_new("calc_thread", (GThreadFunc) ic_calc_thread, NULL);
	init_sc_ic_progress_window();
}

G_MODULE_EXPORT void on_ic_system_change() {
	if(get_num_available_systems() > 0) {
		ic_system = get_available_systems()[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ic_system))];
		update_central_body_dropdown(GTK_COMBO_BOX(cb_ic_central_body), ic_system);
		update_body_dropdown(GTK_COMBO_BOX(cb_ic_depbody), ic_system);
		update_body_dropdown(GTK_COMBO_BOX(cb_ic_arrbody), ic_system);
		grid_ic_fbbodies = ic_update_seq_body_grid(vp_ic_fbbodies, grid_ic_fbbodies);
	}
}

G_MODULE_EXPORT void on_ic_central_body_change() {
	if(get_num_available_systems() > 0) {
		if(get_number_of_subsystems(get_available_systems()[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ic_system))]) == 0) {
			gtk_widget_set_sensitive(GTK_WIDGET(cb_ic_central_body), 0);
			return;
		}
		gtk_widget_set_sensitive(GTK_WIDGET(cb_ic_central_body), 1);
		CelestSystem *ic_og_system = get_available_systems()[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ic_system))];
		ic_system = get_subsystem_from_system_and_id(ic_og_system, gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ic_central_body)));
		update_body_dropdown(GTK_COMBO_BOX(cb_ic_depbody), ic_system);
		update_body_dropdown(GTK_COMBO_BOX(cb_ic_arrbody), ic_system);
		grid_ic_fbbodies = ic_update_seq_body_grid(vp_ic_fbbodies, grid_ic_fbbodies);
	}
}

G_MODULE_EXPORT void on_get_ic_ref_values() {
	if(ic_system == NULL) return;
	double dv_dep, dv_arr_cap, dv_arr_circ, dur;
	Body *dep_body = ic_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ic_depbody))];
	Body *arr_body = ic_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ic_arrbody))];

	if(dep_body == arr_body) return;
	
	Hohmann hohmann = calc_hohmann_transfer(dep_body->orbit.a, arr_body->orbit.a, ic_system->cb);
	
	dur = hohmann.dur;
	dv_dep = dv_circ(dep_body, altatmo2radius(dep_body,ic_dep_periapsis), hohmann.dv_dep);
	dv_arr_circ = dv_circ(arr_body, altatmo2radius(arr_body,ic_arr_periapsis), hohmann.dv_arr);
	dv_arr_cap = dv_capture(arr_body, altatmo2radius(arr_body,ic_arr_periapsis), hohmann.dv_arr);
	
	dur /= get_settings_datetime_type() != DATE_KERBAL ? (24*60*60) : (6*60*60);

	char msg[256];
	sprintf(msg, "Hohmann Transfer from %s to %s \n(from %.3E km to %.3E km circular orbit):\n\n"
				 "Departure dv: %.0f m/s\n"
				 "Arrival Capture dv: %.0f m/s\n"
				 "Arrival Circularization: %.0f m/s\n"
				 "Duration: %.0f days",
			dep_body->name, arr_body->name, dep_body->orbit.a/1e3, arr_body->orbit.a/1e3, dv_dep, dv_arr_cap, dv_arr_circ, dur);
	show_msg_window(msg);
}

void reset_ic() {

}