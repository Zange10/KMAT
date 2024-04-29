#include "database_app.h"
#include "database/lv_database.h"
#include <gtk/gtk.h>
#include <locale.h>


GObject *box;
GObject *mission_vp;


void activate_db_app(GtkApplication *app, gpointer user_data);
void update_db_box();

void start_db_app() {

	GtkApplication *app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
	g_signal_connect (app, "activate", G_CALLBACK (activate_db_app), NULL);

	g_application_run (G_APPLICATION (app), 0, NULL);
	g_object_unref (app);
}

void activate_db_app(GtkApplication *app, gpointer user_data) {
	/* Construct a GtkBuilder instance and load our UI description */
	setlocale(LC_NUMERIC, "C");	// Glade somehow uses commas instead of points for decimals...
	GtkBuilder *builder = gtk_builder_new ();
	gtk_builder_add_from_file(builder, "../GUI/db_test.glade", NULL);

	gtk_builder_connect_signals(builder, NULL);


	box = gtk_builder_get_object(builder, "box");
	mission_vp = gtk_builder_get_object(builder, "mission_viewport");
	update_db_box();

	/* Connect signal handlers to the constructed widgets. */
	GObject *window = gtk_builder_get_object (builder, "window");
	gtk_window_set_application(GTK_WINDOW (window), app);
	gtk_widget_set_visible(GTK_WIDGET (window), TRUE);

	/* We do not need the builder anymore */
	g_object_unref(builder);
}


void update_db_box() {
	// Create a GtkGrid
	GtkWidget *grid = gtk_grid_new();
	GtkWidget *separator;

	struct Mission_DB *missions;
	int mission_count = db_get_missions(&missions);

	separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_attach(GTK_GRID(grid), separator, 0, 0, MISSION_COLS*2+1, 1);
	separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_attach(GTK_GRID(grid), separator, 0, 1, 1, 1);

	for (int col = 0; col < MISSION_COLS; ++col) {
		char label_text[20];
		switch(col) {
			case 1: sprintf(label_text, "Name"); break;
			case 2: sprintf(label_text, "ProgramID"); break;
			case 3: sprintf(label_text, "Status"); break;
			case 4: sprintf(label_text, "Vehicle"); break;
			default:sprintf(label_text, "ID"); break;
		}

		// Create a GtkLabel
		GtkWidget *label = gtk_label_new(label_text);

		gtk_widget_set_margin_start(GTK_WIDGET(label), 15);
		gtk_widget_set_margin_end(GTK_WIDGET(label), 15);
		gtk_widget_set_margin_top(GTK_WIDGET(label), 5);
		gtk_widget_set_margin_bottom(GTK_WIDGET(label), 5);

		// Set the label in the grid at the specified row and column
		gtk_grid_attach(GTK_GRID(grid), label, col*2+1, 1, 1, 1);

		// Create a horizontal separator line (optional)
		separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_grid_attach(GTK_GRID(grid), separator, col*2+2, 1, 1, 1);
	}

	separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_attach(GTK_GRID(grid), separator, 0, 2, MISSION_COLS*2+1, 1);

	struct LauncherInfo_DB lv;
	struct PlaneInfo_DB fv;

	// Create labels and add them to the grid
	for (int i = 0; i < mission_count; ++i) {
		struct Mission_DB m = missions[i];
		int row = i*2+3;

		for (int j = 0; j < MISSION_COLS; ++j) {
			int col = j*2+1;
			char label_text[30];
			if(m.launcher_id != 0) lv = db_get_launcherInfo_from_id(m.launcher_id);
			else if(m.plane_id != 0) fv = db_get_plane_from_id(m.plane_id);
			switch(j) {
				case 1: sprintf(label_text, "%s", m.name); break;
				case 2: sprintf(label_text, "%s", db_get_program_from_id(m.program_id).name); break;
				case 3: sprintf(label_text, "%s",(m.status == YET_TO_FLY ? "YET TO FLY" : (m.status == IN_PROGRESS ? "IN PROGRESS" : "ENDED"))); break;
				case 4:
					if(m.launcher_id != 0) sprintf(label_text, "%s", lv.name);
					else if(m.plane_id != 0) sprintf(label_text, "%s", fv.name);
					else sprintf(label_text, "/"); break;
				default:sprintf(label_text, "%d", m.id); break;
			}

			// Create a GtkLabel
			GtkWidget *label = gtk_label_new(label_text);

			gtk_widget_set_margin_start(GTK_WIDGET(label), 15);
			gtk_widget_set_margin_end(GTK_WIDGET(label), 15);
			gtk_widget_set_margin_top(GTK_WIDGET(label), 5);
			gtk_widget_set_margin_bottom(GTK_WIDGET(label), 5);
			gtk_label_set_xalign(GTK_LABEL(label), (gfloat) 0.0);

			// Set the label in the grid at the specified row and column
			gtk_grid_attach(GTK_GRID(grid), label, col, row, 1, 1);

			// Create a horizontal separator line (optional)
			separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
			gtk_grid_attach(GTK_GRID(grid), separator, col+1, row, 1, 1);
		}
		separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_grid_attach(GTK_GRID(grid), separator, 0, row+1, MISSION_COLS*2+1, 1);
	}

	gtk_container_add (GTK_CONTAINER (mission_vp),
					   grid);
	gtk_widget_show_all(GTK_WIDGET(mission_vp));

	free(missions);
}