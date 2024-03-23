#include "transfer_app.h"
#include "tools/analytic_geometry.h"
#include "celestial_bodies.h"
#include "orbit_calculator/transfer_tools.h"
#include "drawing.h"
#include "transfer_app/transfer_planner.h"

#include <string.h>
#include <gtk/gtk.h>
#include <locale.h>

struct Ephem **body_ephems;


void activate_transfer_app(GtkApplication *app, gpointer user_data);



void start_transfer_app() {
	int num_bodies = 9;
	int num_ephems = 12*100;	// 12 months for 100 years (1950-2050)

	body_ephems = (struct Ephem**) malloc(num_bodies*sizeof(struct Ephem*));
	for(int i = 0; i < num_bodies; i++) {
		body_ephems[i] = (struct Ephem*) malloc(num_ephems*sizeof(struct Ephem));
		get_body_ephem(body_ephems[i], i+1);
	}
	
	GtkApplication *app = gtk_application_new ("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect (app, "activate", G_CALLBACK (activate_transfer_app), NULL);
	
	g_application_run (G_APPLICATION (app), 0, NULL);
	g_object_unref (app);

	remove_all_transfers();
	for(int i = 0; i < 9; i++) free(body_ephems[i]);
	free(body_ephems);
}

void activate_transfer_app(GtkApplication *app, gpointer user_data) {
	/* Construct a GtkBuilder instance and load our UI description */
	setlocale(LC_NUMERIC, "C");	// Glade somehow uses commas instead of points for decimals...
	GtkBuilder *builder = gtk_builder_new ();
	gtk_builder_add_from_file(builder, "../GUI/transfer.glade", NULL);
	
	gtk_builder_connect_signals(builder, NULL);

	/* Connect signal handlers to the constructed widgets. */
	GObject *window = gtk_builder_get_object (builder, "window");
	gtk_window_set_application(GTK_WINDOW (window), app);
	gtk_widget_set_visible(GTK_WIDGET (window), TRUE);


	init_transfer_planner(builder);
	
	/* We do not need the builder anymore */
	g_object_unref(builder);
}

struct Ephem ** get_body_ephems() {
	return body_ephems;
}
