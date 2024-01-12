#include "window.h"
#include "analytic_geometry.h"
#include "ephem.h"
#include "celestial_bodies.h"
#include "orbit_calculator/transfer_tools.h"
#include "drawing.h"
#include "orbit_calculator/orbit.h"

#include <math.h>
#include <string.h>

static int counter = 0;
GObject *drawing_area;
GObject *lb_date;

gboolean body_show_status[9];
struct Ephem **ephems;

double current_date;

void activate(GtkApplication *app, gpointer user_data);
void on_body_toggle(GtkWidget* widget, gpointer data);
void on_change_date(GtkWidget* widget, gpointer data);
void on_calendar_selection(GtkWidget* widget, gpointer data);

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	int window_width = allocation.width;
	int window_height = allocation.height;
	struct Vector2D center = {(double) window_width/2, (double) window_height/2};
	
	cairo_rectangle(cr, 0, 0, window_width, window_height);
	cairo_set_source_rgb(cr, 0,0,0);
	cairo_fill(cr);
	
	cairo_set_source_rgb(cr, 1, 1, 0.3);
	draw_body(cr, center, 0, vec(0,0,0));
	cairo_fill(cr);
	double scale = 1e-9;
	int highest_id = 0;
	for(int i = 0; i < 9; i++) if(body_show_status[i]) highest_id = i+1;
	if(highest_id > 0) {
		struct Body *body = get_body_from_id(highest_id);
		double apoapsis = body->orbit.apoapsis;
		int wh = window_width < window_height ? window_width : window_height;
		scale = 1/apoapsis*wh/2.2;	// divided by 2.2 because apoapsis is only one side and buffer
	}
	
	for(int i = 0; i < 9; i++) {
		if(body_show_status[i]) {
			int id = i+1;
			switch(id) {
				case 1: cairo_set_source_rgb(cr, 0.3, 0.3, 0.3); break;
				case 2: cairo_set_source_rgb(cr, 0.6, 0.6, 0.2); break;
				case 3: cairo_set_source_rgb(cr, 0.2, 0.2, 1.0); break;
				case 4: cairo_set_source_rgb(cr, 1.0, 0.2, 0.0); break;
				case 5: cairo_set_source_rgb(cr, 0.6, 0.4, 0.2); break;
				case 6: cairo_set_source_rgb(cr, 0.8, 0.8, 0.6); break;
				case 7: cairo_set_source_rgb(cr, 0.2, 0.6, 1.0); break;
				case 8: cairo_set_source_rgb(cr, 0.0, 0.0, 1.0); break;
				case 9: cairo_set_source_rgb(cr, 0.7, 0.7, 0.7); break;
				default:cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); break;
			}
			struct OSV osv = osv_from_ephem(ephems[i], current_date, SUN());
			draw_body(cr, center, scale, osv.r);
			draw_orbit(cr, center, scale, osv.r, osv.v, SUN());
		}
	}
	return TRUE;
}

void update_date_label() {
	char date_string[10];
	date_to_string(convert_JD_date(current_date), date_string, 0);
	gtk_label_set_text(GTK_LABEL(lb_date), date_string);
}

void create_window() {
	int num_bodies = 9;
	struct Date min_date = {1970, 1, 1, 0, 0, 0};
	struct Date max_date = {1973, 1, 1, 0, 0, 0};
	
	double jd_min = convert_date_JD(min_date);
	double jd_max = convert_date_JD(max_date);
	
	int ephem_time_steps = 10;  // [days]
	int num_ephems = 12*100;
	
	ephems = (struct Ephem**) malloc(num_bodies*sizeof(struct Ephem*));
	for(int i = 0; i < num_bodies; i++) {
		ephems[i] = (struct Ephem*) malloc(num_ephems*sizeof(struct Ephem));
		get_body_ephem(ephems[i], i+1);
	}
	struct Date date = {1971, 5, 1, 0, 0, 0};
	current_date = convert_date_JD(date);
	
	#ifdef GTK_SRCDIR
			g_chdir (GTK_SRCDIR);
	#endif
	
	GtkApplication *app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
	g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
	
	int status = g_application_run (G_APPLICATION (app), 0, NULL);
	g_object_unref (app);
	free(ephems);
}
	
void activate(GtkApplication *app, gpointer user_data) {
	/* Construct a GtkBuilder instance and load our UI description */
	GtkBuilder *builder = gtk_builder_new ();
	gtk_builder_add_from_file(builder, "../GUI/ephem.glade", NULL);
	
	gtk_builder_connect_signals(builder, NULL);
	
	lb_date = gtk_builder_get_object(builder, "lb_date");
	update_date_label();
	
	drawing_area = gtk_builder_get_object(builder, "drawing_area");
	g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw), NULL);
	
	/* Connect signal handlers to the constructed widgets. */
	GObject *window = gtk_builder_get_object (builder, "window");
	gtk_window_set_application(GTK_WINDOW (window), app);
	gtk_widget_set_visible(GTK_WIDGET (window), TRUE);
	
	/* We do not need the builder anymore */
	g_object_unref(builder);
}





void on_body_toggle(GtkWidget* widget, gpointer data) {
	int id = (int) gtk_widget_get_name(widget)[0] - 48;
	body_show_status[id-1] = body_show_status[id-1] ? 0 : 1;
	gtk_widget_queue_draw(GTK_WIDGET(drawing_area));
}


void on_change_date(GtkWidget* widget, gpointer data) {
	const char *name = gtk_widget_get_name(widget);
	if		(strcmp(name, "+1Y") == 0) current_date = jd_change_date(current_date, 1, 0, 0);
	else if	(strcmp(name, "+1M") == 0) current_date = jd_change_date(current_date, 0, 1, 0);
	else if	(strcmp(name, "+1D") == 0) current_date++;
	else if	(strcmp(name, "-1Y") == 0) current_date = jd_change_date(current_date,-1, 0, 0);
	else if	(strcmp(name, "-1M") == 0) current_date = jd_change_date(current_date, 0,-1, 0);
	else if	(strcmp(name, "-1D") == 0) current_date--;
	update_date_label();
	gtk_widget_queue_draw(GTK_WIDGET(drawing_area));
}


void on_year_select(GtkWidget* widget, gpointer data) {
	const char *name = gtk_widget_get_name(widget);
	int year = atoi(name);
	struct Date date = {year, 1,1};
	current_date = convert_date_JD(date);
	
	update_date_label();
	gtk_widget_queue_draw(GTK_WIDGET(drawing_area));
}
