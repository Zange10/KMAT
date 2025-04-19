#include "gui_manager.h"
#include "info_win_manager.h"
#include <gtk/gtk.h>
#include <locale.h>
#include "css_loader.h"
#include "settings.h"
#include "transfer_app/transfer_planner.h"
#include "transfer_app/porkchop_analyzer.h"
#include "transfer_app/sequence_calculator.h"
#include "transfer_app/itinerary_calculator.h"
#include "launch_app/launch_analyzer.h"
#include "launch_app/capability_analyzer.h"
#include "launch_app/launch_parameter_analyzer.h"
#include "database/lv_database.h"
#include "launch_calculator/lv_profile.h"
#include "gui/database_app/mission_db.h"
#include "tools/file_io.h"
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>

void resolve_win_relative_path(const char *relative_path, char *absolute_path) {
	char full_path[MAX_PATH];
	if (_fullpath(full_path, relative_path, MAX_PATH)) {
		strcpy(absolute_path, full_path);
	} else {
		printf("Error resolving relative path.\n");
		absolute_path[0] = '\0'; // In case of error
	}
}
#endif

struct LV *all_launcher;
int *launcher_ids;
int num_launcher;

char itins_dir[] = "../Itineraries";

void activate_app(GtkApplication *app, gpointer gui_filepath);

void start_gui(const char* gui_filepath) {
	// init launcher from db for launch calc gui
//	num_launcher = get_all_launch_vehicles_from_database(&all_launcher, &launcher_ids);
	setlocale(LC_NUMERIC, "C");	// Glade somehow uses commas instead of points for decimals...

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
	reset_sc();
	end_transfer_planner();
	// reset launch gui
	close_launch_analyzer();
	close_capability_analyzer();
	close_launch_parameter_analyzer();
	// reset db gui
	close_mission_db();
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

	#ifdef _WIN32
		set_window_style_css("../GUI/theme/breeze-dark-win.css");
	#endif
	load_css("../GUI/theme/style.css");

	// init settings page
	init_global_settings(builder);
	// init transfer planner page
	init_itinerary_calculator(builder);
	init_sequence_calculator(builder);
	init_porkchop_analyzer(builder);
	init_transfer_planner(builder);
	// init launch calc page
//	init_launch_analyzer(builder);
//	init_capability_analyzer(builder);
//	init_launch_parameter_analyzer(builder);
	// init db page
//	init_mission_db(builder);
	// init progress window
	init_info_windows(builder);

	/* We do not need the builder anymore */
	g_object_unref(builder);
}

int get_path_from_file_chooser(char *filepath, char *extension, GtkFileChooserAction action) {
	create_directory_if_not_exists(get_itins_directory());
	#ifdef _WIN32
		OPENFILENAME ofn;
		char initialDir[MAX_PATH] = {0};
		char szFile[MAX_PATH] = {0};
		char filter[100];

		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = NULL;  // If using a window, set the handle here


		// Convert relative directory path to an absolute path
		resolve_win_relative_path(get_itins_directory(), initialDir);

		// Set file filter dynamically based on the extension
		snprintf(filter, sizeof(filter), "%s Files%c*%s%cAll Files (*.*)%c*.*%c",
				 extension, '\0', extension, '\0', '\0', '\0');
		ofn.lpstrFilter = filter;
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrInitialDir = initialDir;
		ofn.lpstrDefExt = extension;

		if (action == GTK_FILE_CHOOSER_ACTION_SAVE) {
			ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
			if (GetSaveFileNameA(&ofn)) {
				strcpy(filepath, szFile);
			} else {
				filepath[0] = '\0';
			}
		} else {  // Open file dialog
			ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
			if (GetOpenFileNameA(&ofn)) {
				strcpy(filepath, szFile);
			} else {
				filepath[0] = '\0';
			}
		}
	#else
		GtkWidget *dialog;
		gint res;

		if(action == GTK_FILE_CHOOSER_ACTION_SAVE) {
			dialog = gtk_file_chooser_dialog_new("Save File", NULL, action,
												 "_Cancel", GTK_RESPONSE_CANCEL,
												 "_Save", GTK_RESPONSE_ACCEPT,
												 NULL);
		} else {
			dialog = gtk_file_chooser_dialog_new("Open File", NULL, action,
												 "_Cancel", GTK_RESPONSE_CANCEL,
												 "_Open", GTK_RESPONSE_ACCEPT,
												 NULL);
		}

		// Set initial folder
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), get_itins_directory());

		// Create a filter for files with the extension .itin
		GtkFileFilter *filter = gtk_file_filter_new();

		char extension_filter[20];
		sprintf(extension_filter, "*%s", extension);
		gtk_file_filter_add_pattern(filter, extension_filter);
		gtk_file_filter_set_name(filter, extension);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

		// Run the dialog
		res = gtk_dialog_run(GTK_DIALOG(dialog));
		if (res == GTK_RESPONSE_ACCEPT) {
			char *filepath_temp;
			GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
			filepath_temp = gtk_file_chooser_get_filename(chooser);

			sprintf(filepath, "%s", filepath_temp);
			g_free(filepath_temp);
		}
		// Destroy the dialog
		gtk_widget_destroy(dialog);
	#endif

	// check if file or path was selected
	if(filepath[0] == '\0') return 0;
	else return 1;
}

void create_combobox_dropdown_text_renderer(GObject *combo_box) {
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo_box), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo_box), renderer, "text", 0, NULL);
}

void append_combobox_entry(GtkComboBox *combo_box, char *new_entry) {
	GtkListStore *store = GTK_LIST_STORE(gtk_combo_box_get_model(combo_box));
	GtkTreeIter iter;
	gtk_list_store_append(store, &iter);  // Appends to the end
	char entry[54];
	sprintf(entry, "- %s -", new_entry);
	gtk_list_store_set(store, &iter, 0, entry, -1);
	gtk_combo_box_set_active_iter(combo_box, &iter);
}

void remove_combobox_last_entry(GtkComboBox *combo_box) {
	GtkListStore *store = GTK_LIST_STORE(gtk_combo_box_get_model(combo_box));
	
	GtkTreeIter iter, last_iter;
	gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
	
	if (!valid) {
		// List is empty, nothing to remove
		return;
	}
	
	last_iter = iter;  // In case there's only one row
	while (gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter)) {
		last_iter = iter;  // Keep updating to track the last one
	}
	
	gtk_list_store_remove(store, &last_iter);
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
	sc_change_date_type(old_date_type, new_date_type);
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
		char entry[50];
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
		char entry[32];
		sprintf(entry, "%s", system->bodies[i]->name);
		gtk_list_store_set(store, &iter, 0, entry, 1, i, -1);
	}

	gtk_combo_box_set_model(cb_sel_body, GTK_TREE_MODEL(store));
	gtk_combo_box_set_active(cb_sel_body, 0);

	g_object_unref(store);
}


// launch calc gui stuff ----------------------------------------------------------------
void update_launcher_dropdown(GtkComboBox *cb_sel_launcher) {
	GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	GtkTreeIter iter;
	// Add items to the list store
	for(int i = 0; i < num_launcher; i++) {
		gtk_list_store_append(store, &iter);
		char entry[32];
		sprintf(entry, "%s", all_launcher[i].name);
		gtk_list_store_set(store, &iter, 0, entry, 1, i, -1);
	}

	gtk_combo_box_set_model(cb_sel_launcher, GTK_TREE_MODEL(store));
	gtk_combo_box_set_active(cb_sel_launcher, 0);

	g_object_unref(store);
}

void update_profile_dropdown(GtkComboBox *cb_sel_launcher, GtkComboBox *cb_sel_profile) {
	int id = gtk_combo_box_get_active(GTK_COMBO_BOX(cb_sel_launcher));
	struct LaunchProfiles_DB profiles = db_get_launch_profiles_from_lv_id(launcher_ids[id]);

	GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	GtkTreeIter iter;
	// Add items to the list store
	for(int i = 0; i < profiles.num_profiles; i++) {
		gtk_list_store_append(store, &iter);
		char entry[50];
		switch(profiles.profile[i].profiletype) {
			case 1: sprintf(entry, "p = %.0f", profiles.profile[i].lp_params[0]);
				break;
			case 2: sprintf(entry, "p = 90*exp(-%f*h)", profiles.profile[i].lp_params[0]);
				break;
			case 3: sprintf(entry, "p = (90-%.0f)*exp(-%f*h) + %.0f",
							profiles.profile[i].lp_params[1],
							profiles.profile[i].lp_params[0],
							profiles.profile[i].lp_params[1]);
				break;
			case 4: sprintf(entry, "p4(%f, %f, %f)",
							profiles.profile[i].lp_params[0],
							profiles.profile[i].lp_params[2],
							profiles.profile[i].lp_params[1]);
				break;
		}
		gtk_list_store_set(store, &iter, 0, entry, 1, i, -1);
	}

	gtk_combo_box_set_model(GTK_COMBO_BOX(cb_sel_profile), GTK_TREE_MODEL(store));
	gtk_combo_box_set_active(GTK_COMBO_BOX(cb_sel_profile), 0);

	g_object_unref(store);
}

struct LV * get_all_launcher() {
	return all_launcher;
}

int * get_launcher_ids() {
	return launcher_ids;
}



