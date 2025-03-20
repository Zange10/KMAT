#include "itinerary_calculator.h"
#include "orbit_calculator/transfer_calc.h"
#include "gui/gui_manager.h"
#include "gui/settings.h"
#include "gui/prog_win_manager.h"
#include "tools/file_io.h"


GObject *cb_ic_system;
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
GObject *tf_ic_window;

struct System *ic_system;

void init_itinerary_calculator(GtkBuilder *builder) {
	tf_ic_window = gtk_builder_get_object(builder, "window");
	cb_ic_system = gtk_builder_get_object(builder, "cb_ic_system");
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

	ic_system = NULL;

	create_combobox_dropdown_text_renderer(cb_ic_system);
	create_combobox_dropdown_text_renderer(cb_ic_depbody);
	create_combobox_dropdown_text_renderer(cb_ic_arrbody);
	update_system_dropdown(GTK_COMBO_BOX(cb_ic_system));
	if(get_num_available_systems() > 0) {
		ic_system = get_available_systems()[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ic_system))];
		update_body_dropdown(GTK_COMBO_BOX(cb_ic_depbody), ic_system);
		update_body_dropdown(GTK_COMBO_BOX(cb_ic_arrbody), ic_system);
	}
}

void ic_change_date_type(enum DateType old_date_type, enum DateType new_date_type) {
	change_text_field_date_type(tf_ic_mindepdate, old_date_type, new_date_type);
	change_text_field_date_type(tf_ic_maxdepdate, old_date_type, new_date_type);
	change_text_field_date_type(tf_ic_maxarrdate, old_date_type, new_date_type);
}


void save_itineraries_ic(struct ItinStep **departures, int num_deps, int num_nodes) {
	if(departures == NULL || num_deps == 0) return;

	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
	gint res;

	// Create the file chooser dialog
	dialog = gtk_file_chooser_dialog_new("Save File", NULL, action,
										 "_Cancel", GTK_RESPONSE_CANCEL,
										 "_Save", GTK_RESPONSE_ACCEPT,
										 NULL);

	// Set initial folder
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), "./Itineraries");

	// Create a filter for files with the extension .itin
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*.itins");
	gtk_file_filter_set_name(filter, ".itins");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	// Run the dialog
	res = gtk_dialog_run(GTK_DIALOG(dialog));
	if (res == GTK_RESPONSE_ACCEPT) {
		char *filepath;
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		filepath = gtk_file_chooser_get_filename(chooser);

		store_itineraries_in_bfile(departures, num_nodes, num_deps, ic_system, filepath, 2);
		g_free(filepath);
	}

	// Destroy the dialog
	gtk_widget_destroy(dialog);
}




struct Transfer_Calc_Results results;

gboolean end_calc_thread() {
	end_tc_ic_progress_window();
	gtk_widget_set_sensitive(GTK_WIDGET(tf_ic_window), 1);

	save_itineraries_ic(results.departures, results.num_deps, results.num_nodes);
	for(int i = 0; i < results.num_deps; i++) free_itinerary(results.departures[i]);
	free(results.departures);
	return G_SOURCE_REMOVE;
}

void ic_calc_thread() {
	char *string;
	struct Transfer_To_Target_Calc_Data calc_data;

	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ic_mindepdate));
	calc_data.jd_min_dep = convert_date_JD(date_from_string(string, get_settings_datetime_type()));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ic_maxdepdate));
	calc_data.jd_max_dep = convert_date_JD(date_from_string(string, get_settings_datetime_type()));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ic_maxarrdate));
	calc_data.jd_max_arr = convert_date_JD(date_from_string(string, get_settings_datetime_type()));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ic_maxdur));
	calc_data.max_duration = (int) strtol(string, NULL, 10);
	if(get_settings_datetime_type() == DATE_KERBAL) calc_data.max_duration /= 4;	// kerbal day is 4 times shorter (24h/6h)

	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ic_totdv));
	calc_data.dv_filter.max_totdv = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ic_depdv));
	calc_data.dv_filter.max_depdv = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ic_satdv));
	calc_data.dv_filter.max_satdv = strtod(string, NULL);
	string = (char*) gtk_combo_box_get_active_id(GTK_COMBO_BOX(cb_ic_transfertype));
	calc_data.dv_filter.last_transfer_type = (int) strtol(string, NULL, 10);

	calc_data.dep_body_id = gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ic_depbody));
	calc_data.arr_body_id = gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ic_arrbody));

	calc_data.system = ic_system;

	results = search_for_itinerary_to_target(calc_data);

	g_idle_add((GSourceFunc)end_calc_thread, NULL);
}

G_MODULE_EXPORT G_MODULE_EXPORT void on_calc_ic() {
	if(ic_system == NULL) return;
	gtk_widget_set_sensitive(GTK_WIDGET(tf_ic_window), 0);
	g_thread_new("calc_thread", (GThreadFunc) ic_calc_thread, NULL);
	init_tc_ic_progress_window();
}

G_MODULE_EXPORT void on_ic_system_change() {
	if(get_num_available_systems() > 0) {
		ic_system = get_available_systems()[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ic_system))];
		update_body_dropdown(GTK_COMBO_BOX(cb_ic_depbody), ic_system);
		update_body_dropdown(GTK_COMBO_BOX(cb_ic_arrbody), ic_system);
	}
}

void reset_ic() {

}