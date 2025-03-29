#include "transfer_calculator.h"
#include "orbit_calculator/transfer_calc.h"
#include "gui/gui_manager.h"
#include "gui/settings.h"
#include "gui/prog_win_manager.h"
#include "tools/file_io.h"
#include <string.h>

GObject *tf_tc_window;
GObject *cb_tc_system;
GObject *tf_tc_mindepdate;
GObject *tf_tc_maxdepdate;
GObject *tf_tc_maxarrdate;
GObject *tf_tc_maxdur;
GObject *cb_tc_transfertype;
GObject *tf_tc_totdv;
GObject *tf_tc_depdv;
GObject *tf_tc_satdv;
GObject *vp_tc_preview;
GtkWidget *grid_tc_preview;

struct PlannedStep *tc_step;

struct System *tc_system;

void update_tc_preview();

void init_transfer_calculator(GtkBuilder *builder) {
	tf_tc_window = gtk_builder_get_object(builder, "window");
	cb_tc_system = gtk_builder_get_object(builder, "cb_tc_system");
	tf_tc_mindepdate = gtk_builder_get_object(builder, "tf_tc_mindepdate");
	tf_tc_maxdepdate = gtk_builder_get_object(builder, "tf_tc_maxdepdate");
	tf_tc_maxarrdate = gtk_builder_get_object(builder, "tf_tc_maxarrdate");
	tf_tc_maxdur = gtk_builder_get_object(builder, "tf_tc_maxdur");
	cb_tc_transfertype = gtk_builder_get_object(builder, "cb_tc_transfertype");
	tf_tc_totdv = gtk_builder_get_object(builder, "tf_tc_totdv");
	tf_tc_depdv = gtk_builder_get_object(builder, "tf_tc_depdv");
	tf_tc_satdv = gtk_builder_get_object(builder, "tf_tc_satdv");
	vp_tc_preview = gtk_builder_get_object(builder, "vp_tc_preview");

	tc_step = NULL;
	create_combobox_dropdown_text_renderer(cb_tc_system);
	update_system_dropdown(GTK_COMBO_BOX(cb_tc_system));
	tc_system = NULL;
	if(get_num_available_systems() > 0) tc_system = get_available_systems()[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_tc_system))];
	update_tc_preview();
}

void tc_change_date_type(enum DateType old_date_type, enum DateType new_date_type) {
	change_text_field_date_type(tf_tc_mindepdate, old_date_type, new_date_type);
	change_text_field_date_type(tf_tc_maxdepdate, old_date_type, new_date_type);
	change_text_field_date_type(tf_tc_maxarrdate, old_date_type, new_date_type);
}

struct PlannedStep * get_first_tc(struct PlannedStep *step) {
	if(step == NULL) return NULL;
	while(step->prev != NULL) step = step->prev;
	return step;
}

struct PlannedStep * get_last_tc(struct PlannedStep *step) {
	if(step == NULL) return NULL;
	while(step->next != NULL) step = step->next;
	return step;
}

int get_num_tc_steps(struct PlannedStep *step) {
	step = get_first_tc(step);
	int num = 0;
	while(step != NULL) {num++; step = step->next;}
	return num;
}

void on_add_transfer_tc() {
	if(tc_system == NULL) return;

	struct PlannedStep *new_step = (struct PlannedStep*) malloc(sizeof(struct PlannedStep));
	new_step->prev = NULL;
	new_step->next = NULL;

	new_step->body = tc_system->bodies[0];

	if(tc_step != NULL) {
		struct PlannedStep *last = get_last_tc(tc_step);
		last->next = new_step;
		new_step->prev = last;
	}

	tc_step = new_step;

	update_tc_preview();
}

void on_remove_transfer_tc(GtkWidget *widget, struct PlannedStep *step) {
	if(step->prev != NULL) step->prev->next = step->next;
	if(step->next != NULL) step->next->prev = step->prev;

	tc_step = step->prev != NULL ? step->prev : step->next;

	free(step);
	update_tc_preview();
}

void on_update_step_body_tc(GtkWidget *widget, struct PlannedStep *step) {
	step->body = tc_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(widget))];
}

void update_tc_preview() {
	// Remove grid if exists
	if (grid_tc_preview != NULL && GTK_WIDGET(vp_tc_preview) == gtk_widget_get_parent(grid_tc_preview)) {
		gtk_container_remove(GTK_CONTAINER(vp_tc_preview), grid_tc_preview);
	}

	grid_tc_preview = gtk_grid_new();

	GtkWidget *widget;
	struct PlannedStep *step = get_first_tc(tc_step);
	int row = 0;

	while(step != NULL) {
		// Body drop-down
		widget = gtk_combo_box_new();
		create_combobox_dropdown_text_renderer(G_OBJECT(widget));
		update_body_dropdown(GTK_COMBO_BOX(widget), tc_system);
		gtk_combo_box_set_active(GTK_COMBO_BOX(widget), get_body_system_id(step->body, tc_system));
		g_signal_connect(widget, "changed", G_CALLBACK(on_update_step_body_tc), step);
		gtk_widget_set_halign(widget, GTK_ALIGN_FILL);
		gtk_widget_set_hexpand(widget, TRUE);
		gtk_grid_attach(GTK_GRID(grid_tc_preview), widget, 0, row, 1, 1);

		// Remove step button
		widget = gtk_button_new_with_label("-");
		gtk_widget_set_size_request(widget, 50, -1);
		g_signal_connect(widget, "clicked", G_CALLBACK(on_remove_transfer_tc), step);
		gtk_grid_attach(GTK_GRID(grid_tc_preview), widget, 1, row, 1, 1);
		step = step->next;
		row++;
	}

	// Add next step button
	widget = gtk_button_new_with_label("+");
	g_signal_connect(widget, "clicked", G_CALLBACK(on_add_transfer_tc), NULL);
	gtk_widget_set_halign(widget, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach(GTK_GRID(grid_tc_preview), widget, 0, row, 2, 1);

	gtk_container_add (GTK_CONTAINER (vp_tc_preview), grid_tc_preview);
	gtk_widget_show_all(GTK_WIDGET(vp_tc_preview));
}

void save_itineraries_tc(struct ItinStep **departures, int num_deps, int num_nodes) {
	if(departures == NULL || num_deps == 0) return;
	char filepath[255];
	if(!get_path_from_file_chooser(filepath,  ".itins", GTK_FILE_CHOOSER_ACTION_SAVE)) return;
	store_itineraries_in_bfile(departures, num_nodes, num_deps, tc_system, filepath, get_current_bin_file_type());
}

struct Transfer_Calc_Results tc_results;

gboolean end_tc_calc_thread() {
	end_tc_ic_progress_window();
	gtk_widget_set_sensitive(GTK_WIDGET(tf_tc_window), 1);

	save_itineraries_tc(tc_results.departures, tc_results.num_deps, tc_results.num_nodes);
	for(int i = 0; i < tc_results.num_deps; i++) free_itinerary(tc_results.departures[i]);
	free(tc_results.departures);
	return G_SOURCE_REMOVE;
}

void tc_calc_thread() {
	char *string;
	struct Transfer_Calc_Data calc_data;

	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_tc_mindepdate));
	calc_data.jd_min_dep = convert_date_JD(date_from_string(string, get_settings_datetime_type()));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_tc_maxdepdate));
	calc_data.jd_max_dep = convert_date_JD(date_from_string(string, get_settings_datetime_type()));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_tc_maxarrdate));
	calc_data.jd_max_arr = convert_date_JD(date_from_string(string, get_settings_datetime_type()));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_tc_maxdur));
	calc_data.max_duration = (int) strtol(string, NULL, 10);
	if(get_settings_datetime_type() == DATE_KERBAL) calc_data.max_duration /= 4;	// kerbal day is 4 times shorter (24h/6h)

	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_tc_totdv));
	calc_data.dv_filter.max_totdv = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_tc_depdv));
	calc_data.dv_filter.max_depdv = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_tc_satdv));
	calc_data.dv_filter.max_satdv = strtod(string, NULL);
	string = (char*) gtk_combo_box_get_active_id(GTK_COMBO_BOX(cb_tc_transfertype));
	calc_data.dv_filter.last_transfer_type = (int) strtol(string, NULL, 10);

	struct PlannedStep *step = get_first_tc(tc_step);
	calc_data.num_steps = get_num_tc_steps(step);
	printf("%d\n", get_num_tc_steps(step));
	calc_data.bodies = malloc(calc_data.num_steps * sizeof(struct Body*));
	for(int i = 0; i < calc_data.num_steps; i++) {
		calc_data.bodies[i] = step->body;
		step = step->next;
	}

	calc_data.system = tc_system;

	tc_results = search_for_spec_itinerary(calc_data);

	// GUI stuff needs to happen in main thread
	free(calc_data.bodies);
	g_idle_add((GSourceFunc)end_tc_calc_thread, NULL);
}


G_MODULE_EXPORT void on_calc_tc() {
	if(tc_system == NULL || tc_step == NULL) return;
	gtk_widget_set_sensitive(GTK_WIDGET(tf_tc_window), 0);
	g_thread_new("calc_thread", (GThreadFunc) tc_calc_thread, NULL);
	init_tc_ic_progress_window();
}

G_MODULE_EXPORT void on_tc_system_change() {
	if(get_num_available_systems() > 0) tc_system = get_available_systems()[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_tc_system))];
	reset_tc();
	update_tc_preview();
}

void reset_tc() {
	if(tc_step == NULL) return;
	tc_step = get_first_tc(tc_step);
	while(tc_step->next != NULL) {
		tc_step = tc_step->next;
		free(tc_step->prev);
	}
	free(tc_step);
	tc_step = NULL;
}
