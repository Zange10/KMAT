#include "sequence_calculator.h"
#include "orbit_calculator/transfer_calc.h"
#include "gui/gui_manager.h"
#include "gui/settings.h"
#include "gui/info_win_manager.h"
#include "tools/file_io.h"
#include <string.h>

GObject *tf_sc_window;
GObject *cb_sc_system;
GObject *tf_sc_mindepdate;
GObject *tf_sc_maxdepdate;
GObject *tf_sc_maxarrdate;
GObject *tf_sc_maxdur;
GObject *cb_sc_transfertype;
GObject *tf_sc_totdv;
GObject *tf_sc_depdv;
GObject *tf_sc_satdv;
GObject *vp_sc_preview;
GtkWidget *grid_sc_preview;

struct PlannedScStep *sc_step;

struct System *sc_system;

void update_sc_preview();

void init_sequence_calculator(GtkBuilder *builder) {
	tf_sc_window = gtk_builder_get_object(builder, "window");
	cb_sc_system = gtk_builder_get_object(builder, "cb_sc_system");
	tf_sc_mindepdate = gtk_builder_get_object(builder, "tf_sc_mindepdate");
	tf_sc_maxdepdate = gtk_builder_get_object(builder, "tf_sc_maxdepdate");
	tf_sc_maxarrdate = gtk_builder_get_object(builder, "tf_sc_maxarrdate");
	tf_sc_maxdur = gtk_builder_get_object(builder, "tf_sc_maxdur");
	cb_sc_transfertype = gtk_builder_get_object(builder, "cb_sc_transfertype");
	tf_sc_totdv = gtk_builder_get_object(builder, "tf_sc_totdv");
	tf_sc_depdv = gtk_builder_get_object(builder, "tf_sc_depdv");
	tf_sc_satdv = gtk_builder_get_object(builder, "tf_sc_satdv");
	vp_sc_preview = gtk_builder_get_object(builder, "vp_sc_preview");

	sc_step = NULL;
	create_combobox_dropdown_text_renderer(cb_sc_system);
	update_system_dropdown(GTK_COMBO_BOX(cb_sc_system));
	sc_system = NULL;
	if(get_num_available_systems() > 0) sc_system = get_available_systems()[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_sc_system))];
	update_sc_preview();
}

void sc_change_date_type(enum DateType old_date_type, enum DateType new_date_type) {
	change_text_field_date_type(tf_sc_mindepdate, old_date_type, new_date_type);
	change_text_field_date_type(tf_sc_maxdepdate, old_date_type, new_date_type);
	change_text_field_date_type(tf_sc_maxarrdate, old_date_type, new_date_type);
}

struct PlannedScStep * get_first_sc(struct PlannedScStep *step) {
	if(step == NULL) return NULL;
	while(step->prev != NULL) step = step->prev;
	return step;
}

struct PlannedScStep * get_last_sc(struct PlannedScStep *step) {
	if(step == NULL) return NULL;
	while(step->next != NULL) step = step->next;
	return step;
}

int get_num_sc_steps(struct PlannedScStep *step) {
	step = get_first_sc(step);
	int num = 0;
	while(step != NULL) {num++; step = step->next;}
	return num;
}

void on_add_transfer_sc() {
	if(sc_system == NULL) return;

	struct PlannedScStep *new_step = (struct PlannedScStep*) malloc(sizeof(struct PlannedScStep));
	new_step->prev = NULL;
	new_step->next = NULL;

	new_step->body = sc_system->bodies[0];

	if(sc_step != NULL) {
		struct PlannedScStep *last = get_last_sc(sc_step);
		last->next = new_step;
		new_step->prev = last;
	}

	sc_step = new_step;

	update_sc_preview();
}

void on_remove_transfer_sc(GtkWidget *widget, struct PlannedScStep *step) {
	if(step->prev != NULL) step->prev->next = step->next;
	if(step->next != NULL) step->next->prev = step->prev;

	sc_step = step->prev != NULL ? step->prev : step->next;

	free(step);
	update_sc_preview();
}

void on_update_step_body_sc(GtkWidget *widget, struct PlannedScStep *step) {
	step->body = sc_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(widget))];
}

void update_sc_preview() {
	// Remove grid if exists
	if (grid_sc_preview != NULL && GTK_WIDGET(vp_sc_preview) == gtk_widget_get_parent(grid_sc_preview)) {
		gtk_container_remove(GTK_CONTAINER(vp_sc_preview), grid_sc_preview);
	}

	grid_sc_preview = gtk_grid_new();

	GtkWidget *widget;
	struct PlannedScStep *step = get_first_sc(sc_step);
	int row = 0;

	while(step != NULL) {
		// Body drop-down
		widget = gtk_combo_box_new();
		create_combobox_dropdown_text_renderer(G_OBJECT(widget));
		update_body_dropdown(GTK_COMBO_BOX(widget), sc_system);
		gtk_combo_box_set_active(GTK_COMBO_BOX(widget), get_body_system_id(step->body, sc_system));
		g_signal_connect(widget, "changed", G_CALLBACK(on_update_step_body_sc), step);
		gtk_widget_set_halign(widget, GTK_ALIGN_FILL);
		gtk_widget_set_hexpand(widget, TRUE);
		gtk_grid_attach(GTK_GRID(grid_sc_preview), widget, 0, row, 1, 1);

		// Remove step button
		widget = gtk_button_new_with_label("-");
		gtk_widget_set_size_request(widget, 50, -1);
		g_signal_connect(widget, "clicked", G_CALLBACK(on_remove_transfer_sc), step);
		gtk_grid_attach(GTK_GRID(grid_sc_preview), widget, 1, row, 1, 1);
		step = step->next;
		row++;
	}

	// Add next step button
	widget = gtk_button_new_with_label("+");
	g_signal_connect(widget, "clicked", G_CALLBACK(on_add_transfer_sc), NULL);
	gtk_widget_set_halign(widget, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach(GTK_GRID(grid_sc_preview), widget, 0, row, 2, 1);

	gtk_container_add (GTK_CONTAINER (vp_sc_preview), grid_sc_preview);
	gtk_widget_show_all(GTK_WIDGET(vp_sc_preview));
}

void save_itineraries_sc(struct ItinStep **departures, int num_deps, int num_nodes) {
	if(departures == NULL || num_deps == 0) return;
	char filepath[255];
	if(!get_path_from_file_chooser(filepath,  ".itins", GTK_FILE_CHOOSER_ACTION_SAVE)) return;
	store_itineraries_in_bfile(departures, num_nodes, num_deps, sc_system, filepath, get_current_bin_file_type());
}

struct Transfer_Calc_Results sc_results;

gboolean end_sc_calc_thread() {
	end_sc_ic_progress_window();
	gtk_widget_set_sensitive(GTK_WIDGET(tf_sc_window), 1);

	save_itineraries_sc(sc_results.departures, sc_results.num_deps, sc_results.num_nodes);
	for(int i = 0; i < sc_results.num_deps; i++) free_itinerary(sc_results.departures[i]);
	free(sc_results.departures);
	if(sc_results.num_deps == 0) show_msg_window("No itineraries found!");
	return G_SOURCE_REMOVE;
}

void sc_calc_thread() {
	char *string;
	struct Transfer_Spec_Calc_Data calc_data;

	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_sc_mindepdate));
	calc_data.jd_min_dep = convert_date_JD(date_from_string(string, get_settings_datetime_type()));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_sc_maxdepdate));
	calc_data.jd_max_dep = convert_date_JD(date_from_string(string, get_settings_datetime_type()));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_sc_maxarrdate));
	calc_data.jd_max_arr = convert_date_JD(date_from_string(string, get_settings_datetime_type()));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_sc_maxdur));
	calc_data.max_duration = (int) strtol(string, NULL, 10);
	if(get_settings_datetime_type() == DATE_KERBAL) calc_data.max_duration /= 4;	// kerbal day is 4 times shorter (24h/6h)

	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_sc_totdv));
	calc_data.dv_filter.max_totdv = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_sc_depdv));
	calc_data.dv_filter.max_depdv = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_sc_satdv));
	calc_data.dv_filter.max_satdv = strtod(string, NULL);
	string = (char*) gtk_combo_box_get_active_id(GTK_COMBO_BOX(cb_sc_transfertype));
	calc_data.dv_filter.last_transfer_type = (int) strtol(string, NULL, 10);

	struct PlannedScStep *step = get_first_sc(sc_step);
	calc_data.num_steps = get_num_sc_steps(step);
	calc_data.bodies = malloc(calc_data.num_steps * sizeof(struct Body*));
	for(int i = 0; i < calc_data.num_steps; i++) {
		calc_data.bodies[i] = step->body;
		step = step->next;
	}

	calc_data.system = sc_system;

	sc_results = search_for_spec_itinerary(calc_data);

	// GUI stuff needs to happen in main thread
	free(calc_data.bodies);
	g_idle_add((GSourceFunc)end_sc_calc_thread, NULL);
}


G_MODULE_EXPORT void on_calc_sc() {
	if(sc_system == NULL || get_num_sc_steps(sc_step) < 2) return;
	gtk_widget_set_sensitive(GTK_WIDGET(tf_sc_window), 0);
	g_thread_new("calc_thread", (GThreadFunc) sc_calc_thread, NULL);
	init_sc_ic_progress_window();
}

G_MODULE_EXPORT void on_sc_system_change() {
	if(get_num_available_systems() > 0) sc_system = get_available_systems()[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_sc_system))];
	reset_sc();
	update_sc_preview();
}

void reset_sc() {
	if(sc_step == NULL) return;
	sc_step = get_first_sc(sc_step);
	while(sc_step->next != NULL) {
		sc_step = sc_step->next;
		free(sc_step->prev);
	}
	free(sc_step);
	sc_step = NULL;
}
