#include "itin_rework_manager.h"
#include "itin_rework_test.h"
#include "../info_win_manager.h"
#include <gtk/gtk.h>
#include <locale.h>
#include "../gui_tools/screen.h"
#include "tools/version_tool.h"


void activate_graphing_app(GtkApplication *app, gpointer gui_filepath);

void start_graphing_gui(const char* gui_filepath) {
	// init launcher from db for launch calc gui
//	num_launcher = get_all_launch_vehicles_from_database(&all_launcher, &launcher_ids);
	setlocale(LC_NUMERIC, "C");	// Glade somehow uses commas instead of points for decimals...

	// init app
	GtkApplication *app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
	g_signal_connect (app, "activate", G_CALLBACK (activate_graphing_app), (gpointer) gui_filepath);
	g_application_run (G_APPLICATION (app), 0, NULL);
	g_object_unref (app);

	// gui runs...
}

void activate_graphing_app(GtkApplication *app, gpointer gui_filepath) {
	/* Construct a GtkBuilder instance and load our UI description */
	setlocale(LC_NUMERIC, "C");	// Glade somehow uses commas instead of points for decimals...
	GtkBuilder *builder = gtk_builder_new ();
	gtk_builder_add_from_file(builder, gui_filepath, NULL);

	gtk_builder_connect_signals(builder, NULL);

	/* Connect signal handlers to the constructed widgets. */
	GObject *window = gtk_builder_get_object (builder, "window");
	gtk_window_set_application(GTK_WINDOW (window), app);
	char version_string[16];
	get_current_version_string_incl_tool_name(version_string);
	gtk_window_set_title(GTK_WINDOW(window), version_string);
	gtk_widget_set_visible(GTK_WIDGET (window), TRUE);

	init_itin_rework_test(builder);

	/* We do not need the builder anymore */
	g_object_unref(builder);
}