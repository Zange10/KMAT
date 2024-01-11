#include "window.h"
#include "analytic_geometry.h"
#include "ephem.h"
#include "celestial_bodies.h"
#include "orbit_calculator/transfer_tools.h"
#include "drawing.h"

#include <math.h>

static int counter = 0;
GObject *drawing_area;
GObject *lb_date;

gboolean body_show_status[9];
struct Ephem **ephems;
int body_radius = 5;

double current_date;


void activate(GtkApplication *app, gpointer user_data);


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
	cairo_arc(cr, center.x, center.y,body_radius,0,M_PI*2);
	cairo_fill(cr);
	
	int highest_id = 0;
	for(int i = 0; i < 9; i++) if(body_show_status[i]) highest_id = i+1;
	
	for(int i = 0; i < 9; i++) {
		if(body_show_status[i]) {
			int id = i+1;
			struct OSV osv;
			if(id == 2) {
				cairo_set_source_rgb(cr, 0.6, 0.6, 0.2);
				osv = osv_from_ephem(ephems[0], current_date, VENUS()->orbit.body);
				cairo_arc(cr, center.x+osv.r.x/1e9, center.y+osv.r.y/1e9,body_radius,0,M_PI*2);
				cairo_fill(cr);
				draw_orbit(cr, center, 1e-9, osv.r, osv.v, VENUS()->orbit.body);
			} else if(id == 3) {
				cairo_set_source_rgb(cr, 0, 0, 1);
				osv = osv_from_ephem(ephems[1], current_date, EARTH()->orbit.body);
				cairo_arc(cr, center.x+osv.r.x/1e9, center.y+osv.r.y/1e9,body_radius,0,M_PI*2);
				cairo_fill(cr);
				draw_orbit(cr, center, 1e-9, osv.r, osv.v, EARTH()->orbit.body);
			} else {
				cairo_set_source_rgb(cr, 1, 1, 1);
				osv.r = vec(0,0,0);
				osv.v = vec(0,0,0);
				cairo_arc(cr, center.x+osv.r.x/1e9, center.y+osv.r.y/1e9,body_radius,0,M_PI*2);
				cairo_fill(cr);
			}
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
	struct Body *bodies[] = {VENUS(), EARTH()};
	int num_bodies = (int) (sizeof(bodies)/sizeof(struct Body*));
	struct Date min_date = {1970, 1, 1, 0, 0, 0};
	struct Date max_date = {1973, 1, 1, 0, 0, 0};
	
	double jd_min = convert_date_JD(min_date);
	double jd_max = convert_date_JD(max_date);
	
	int ephem_time_steps = 10;  // [days]
	int num_ephems = (int)(jd_max - jd_min) / ephem_time_steps + 1;
	
	ephems = (struct Ephem**) malloc(num_bodies*sizeof(struct Ephem*));
	for(int i = 0; i < num_bodies; i++) {
		int ephem_available = 0;
		for(int j = 0; j < i; j++) {
			if(bodies[i] == bodies[j]) {
				ephems[i] = ephems[j];
				ephem_available = 1;
				break;
			}
		}
		if(ephem_available) continue;
		ephems[i] = (struct Ephem*) malloc(num_ephems*sizeof(struct Ephem));
		get_ephem(ephems[i], num_ephems, bodies[i]->id, ephem_time_steps, jd_min, jd_max, 0);
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

void on_body_toggle(GtkWidget* widget, gpointer data) {
	int id = (int) gtk_widget_get_name(widget)[0] - 48;
	body_show_status[id-1] = body_show_status[id-1] ? 0 : 1;
	gtk_widget_queue_draw(GTK_WIDGET(drawing_area));
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
