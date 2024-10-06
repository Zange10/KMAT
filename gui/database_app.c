#include "gui/database_app/mission_db.h"
#include <gtk/gtk.h>
#include <locale.h>
#include "gui/css_loader.h"



void activate_db_app(GtkApplication *app, gpointer user_data);

void start_db_app() {

	GtkApplication *app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
	g_signal_connect (app, "activate", G_CALLBACK (activate_db_app), NULL);

	g_application_run (G_APPLICATION (app), 0, NULL);
	g_object_unref (app);

	close_mission_db();
}

void activate_db_app(GtkApplication *app, gpointer user_data) {
	/* Construct a GtkBuilder instance and load our UI description */
	setlocale(LC_NUMERIC, "C");	// Glade somehow uses commas instead of points for decimals...
	GtkBuilder *builder = gtk_builder_new ();
	gtk_builder_add_from_file(builder, "../GUI/db_manager.glade", NULL);

	gtk_builder_connect_signals(builder, NULL);

	/* Connect signal handlers to the constructed widgets. */
	GObject *window = gtk_builder_get_object (builder, "window");
	gtk_window_set_application(GTK_WINDOW (window), app);
	gtk_widget_set_visible(GTK_WIDGET (window), TRUE);

	load_css();

	init_mission_db(builder);

	/* We do not need the builder anymore */
	g_object_unref(builder);
}