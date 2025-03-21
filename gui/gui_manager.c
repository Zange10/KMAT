#include "gui_manager.h"
#include "prog_win_manager.h"
#include <gtk/gtk.h>
#include <locale.h>
#include "css_loader.h"
#include "settings.h"
#include "transfer_app/transfer_planner.h"
#include "transfer_app/porkchop_analyzer.h"
#include "transfer_app/transfer_calculator.h"
#include "transfer_app/itinerary_calculator.h"
//#include "launch_app/launch_analyzer.h"
//#include "launch_app/capability_analyzer.h"
//#include "launch_app/launch_parameter_analyzer.h"
//#include "database/lv_database.h"
//#include "launch_calculator/lv_profile.h"
//#include "gui/database_app/mission_db.h"


struct LV *all_launcher;
int *launcher_ids;
int num_launcher;

char itins_dir[] = "../Itineraries";

void activate_app(GtkApplication *app, gpointer gui_filepath);

void start_gui(const char* gui_filepath) {
	// init launcher from db for launch calc gui
//	num_launcher = get_all_launch_vehicles_from_database(&all_launcher, &launcher_ids);
//	setlocale(LC_NUMERIC, "C");	// Glade somehow uses commas instead of points for decimals...

	// init app
	GtkApplication *app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
	g_signal_connect (app, "activate", G_CALLBACK (activate_app), (gpointer) gui_filepath);
	g_application_run (G_APPLICATION (app), 0, NULL);
	g_object_unref (app);

	// gui runs...

	// reset transfer gui
	remove_all_transfers();
	free_all_porkchop_analyzer_itins();
	reset_ic();
	reset_tc();
	end_transfer_planner();
	// reset launch gui
//	close_launch_analyzer();
//	close_capability_analyzer();
//	close_launch_parameter_analyzer();
	// reset db gui
//	close_mission_db();
}

void activate_app(GtkApplication *app, gpointer gui_filepath) {
	/* Construct a GtkBuilder instance and load our UI description */
	setlocale(LC_NUMERIC, "C");	// Glade somehow uses commas instead of points for decimals...
	GtkBuilder *builder = gtk_builder_new ();
	gtk_builder_add_from_file(builder, gui_filepath, NULL);

	gtk_builder_connect_signals(builder, NULL);

	/* Connect signal handlers to the constructed widgets. */
	GObject *window = gtk_builder_get_object (builder, "window");
	gtk_window_set_application(GTK_WINDOW (window), app);
	gtk_widget_set_visible(GTK_WIDGET (window), TRUE);

	load_css();

	// init settings page
	init_global_settings(builder);
	// init transfer planner page
	init_itinerary_calculator(builder);
	init_transfer_calculator(builder);
	init_porkchop_analyzer(builder);
	init_transfer_planner(builder);
	// init launch calc page
//	init_launch_analyzer(builder);
//	init_capability_analyzer(builder);
//	init_launch_parameter_analyzer(builder);
	// init db page
//	init_mission_db(builder);
	// init progress window
	init_prog_window(builder);

	/* We do not need the builder anymore */
	g_object_unref(builder);
}


void create_combobox_dropdown_text_renderer(GObject *combo_box) {
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo_box), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo_box), renderer, "text", 0, NULL);
}

void change_text_field_date_type(GObject *text_field, enum DateType old_date_type, enum DateType new_date_type) {
	char *old_string, new_string[32];
	old_string = (char*) gtk_entry_get_text(GTK_ENTRY(text_field));
	if(!is_string_valid_date_format(old_string, old_date_type)) return;
	struct Date date = date_from_string(old_string, old_date_type);
	date = change_date_type(date, new_date_type);
	date_to_string(date, new_string, 0);
	gtk_entry_set_text(GTK_ENTRY(text_field), new_string);
}

void change_label_date_type(GObject *label, enum DateType old_date_type, enum DateType new_date_type) {
	char *old_string, new_string[32];
	old_string = (char*) gtk_label_get_text(GTK_LABEL(label));
	if(!is_string_valid_date_format(old_string, old_date_type)) return;
	struct Date date = date_from_string(old_string, old_date_type);
	date = change_date_type(date, new_date_type);
	date_to_string(date, new_string, 0);
	gtk_label_set_text(GTK_LABEL(label), new_string);
}

void change_button_date_type(GObject *button, enum DateType old_date_type, enum DateType new_date_type) {
	char *old_string, new_string[32];
	old_string = (char*) gtk_button_get_label(GTK_BUTTON(button));
	if(!is_string_valid_date_format(old_string, old_date_type)) return;
	struct Date date = date_from_string(old_string, old_date_type);
	date = change_date_type(date, new_date_type);
	date_to_string(date, new_string, 0);
	gtk_button_set_label(GTK_BUTTON(button), new_string);
}

// settings stuff -------------------------------------------------------------------------
void change_gui_date_type(enum DateType old_date_type, enum DateType new_date_type) {
	ic_change_date_type(old_date_type, new_date_type);
	tc_change_date_type(old_date_type, new_date_type);
	pa_change_date_type(old_date_type, new_date_type);
	tp_change_date_type(old_date_type, new_date_type);
}


// transfer calc gui stuff ----------------------------------------------------------------
char * get_itins_directory() {
	return itins_dir;
}

void update_system_dropdown(GtkComboBox *cb_sel_system) {
	GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	GtkTreeIter iter;
	// Add items to the list store
	for(int i = 0; i < get_num_available_systems(); i++) {
		gtk_list_store_append(store, &iter);
		char entry[30];
		sprintf(entry, "%s", get_available_systems()[i]->name);
		gtk_list_store_set(store, &iter, 0, entry, 1, i, -1);
	}

	gtk_combo_box_set_model(cb_sel_system, GTK_TREE_MODEL(store));
	gtk_combo_box_set_active(cb_sel_system, 0);

	g_object_unref(store);
}


void update_body_dropdown(GtkComboBox *cb_sel_body, struct System *system) {
	GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	GtkTreeIter iter;
	// Add items to the list store
	for(int i = 0; i < system->num_bodies; i++) {
		gtk_list_store_append(store, &iter);
		char entry[30];
		sprintf(entry, "%s", system->bodies[i]->name);
		gtk_list_store_set(store, &iter, 0, entry, 1, i, -1);
	}

	gtk_combo_box_set_model(cb_sel_body, GTK_TREE_MODEL(store));
	gtk_combo_box_set_active(cb_sel_body, 0);

	g_object_unref(store);
}


//// launch calc gui stuff ----------------------------------------------------------------
//void update_launcher_dropdown(GtkComboBox *cb_sel_launcher) {
//	GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
//	GtkTreeIter iter;
//	// Add items to the list store
//	for(int i = 0; i < num_launcher; i++) {
//		gtk_list_store_append(store, &iter);
//		char entry[30];
//		sprintf(entry, "%s", all_launcher[i].name);
//		gtk_list_store_set(store, &iter, 0, entry, 1, i, -1);
//	}
//
//	gtk_combo_box_set_model(cb_sel_launcher, GTK_TREE_MODEL(store));
//	gtk_combo_box_set_active(cb_sel_launcher, 0);
//
//	g_object_unref(store);
//}
//
//void update_profile_dropdown(GtkComboBox *cb_sel_launcher, GtkComboBox *cb_sel_profile) {
//	int id = gtk_combo_box_get_active(GTK_COMBO_BOX(cb_sel_launcher));
//	struct LaunchProfiles_DB profiles = db_get_launch_profiles_from_lv_id(launcher_ids[id]);
//
//	GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
//	GtkTreeIter iter;
//	// Add items to the list store
//	for(int i = 0; i < profiles.num_profiles; i++) {
//		gtk_list_store_append(store, &iter);
//		char entry[50];
//		switch(profiles.profile[i].profiletype) {
//			case 1: sprintf(entry, "p = %.0f", profiles.profile[i].lp_params[0]);
//				break;
//			case 2: sprintf(entry, "p = 90*exp(-%f*h)", profiles.profile[i].lp_params[0]);
//				break;
//			case 3: sprintf(entry, "p = (90-%.0f)*exp(-%f*h) + %.0f",
//							profiles.profile[i].lp_params[1],
//							profiles.profile[i].lp_params[0],
//							profiles.profile[i].lp_params[1]);
//				break;
//			case 4: sprintf(entry, "p4(%f, %f, %f)",
//							profiles.profile[i].lp_params[0],
//							profiles.profile[i].lp_params[2],
//							profiles.profile[i].lp_params[1]);
//				break;
//		}
//		gtk_list_store_set(store, &iter, 0, entry, 1, i, -1);
//	}
//
//	gtk_combo_box_set_model(GTK_COMBO_BOX(cb_sel_profile), GTK_TREE_MODEL(store));
//	gtk_combo_box_set_active(GTK_COMBO_BOX(cb_sel_profile), 0);
//
//	g_object_unref(store);
//}
//
//struct LV * get_all_launcher() {
//	return all_launcher;
//}
//
//int * get_launcher_ids() {
//	return launcher_ids;
//}



