#include "launch_app.h"
#include "launch_app/launch_analyzer.h"
#include "launch_app/capability_analyzer.h"
#include "launch_app/launch_parameter_analyzer.h"
#include "database/lv_database.h"
#include "launch_calculator/lv_profile.h"

#include <locale.h>


struct LV *all_launcher;
int *launcher_ids;
int num_launcher;


void activate_launch_app(GtkApplication *app, gpointer user_data) {
	num_launcher = get_all_launch_vehicles_from_database(&all_launcher, &launcher_ids);

	/* Construct a GtkBuilder instance and load our UI description */
	setlocale(LC_NUMERIC, "C");	// Glade somehow uses commas instead of points for decimals...
	GtkBuilder *builder = gtk_builder_new ();
	gtk_builder_add_from_file(builder, "../GUI/launch.glade", NULL);
	
	gtk_builder_connect_signals(builder, NULL);
	
	/* Connect signal handlers to the constructed widgets. */
	GObject *window = gtk_builder_get_object (builder, "window");
	gtk_window_set_application(GTK_WINDOW (window), app);
	gtk_widget_set_visible(GTK_WIDGET (window), TRUE);
	
	
	init_launch_analyzer(builder);
	init_capability_analyzer(builder);
	init_launch_parameter_analyzer(builder);
	
	/* We do not need the builder anymore */
	g_object_unref(builder);
}

void start_launch_app() {
	GtkApplication *app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
	g_signal_connect (app, "activate", G_CALLBACK (activate_launch_app), NULL);
	
	g_application_run (G_APPLICATION (app), 0, NULL);
	g_object_unref (app);

	close_launch_analyzer();
	close_capability_analyzer();
	close_launch_parameter_analyzer();
}



void update_launcher_dropdown(GtkComboBox *cb_sel_launcher) {
	GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	GtkTreeIter iter;
	// Add items to the list store
	for(int i = 0; i < num_launcher; i++) {
		gtk_list_store_append(store, &iter);
		char entry[30];
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

