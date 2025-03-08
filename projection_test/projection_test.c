#include "projection_test.h"
#include <stdio.h>
#include <gtk/gtk.h>
#include <locale.h>
#include "tools/analytic_geometry.h"
#include "gui/drawing.h"
#include <math.h>



GObject *drawing_area;

struct Vector observer = {-10,0,5};
struct Vector looking = {1,0,0};
double angle = 20*M_PI/180;


void on_draw_projection(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
	// reset drawing area
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	int area_width = allocation.width;
	int area_height = allocation.height;

	struct Vector2D center = {(double)area_width/2, (double)area_height/2};

	cairo_rectangle(cr, 0, 0, area_width, area_height);
	cairo_set_source_rgb(cr, 0,0,0);
	cairo_fill(cr);

	cairo_set_source_rgb(cr, 0.0, 1.0, 1.0); // Blue color
	cairo_set_line_width(cr, 5.0);

	struct Vector p3d_last = {1,0,0};
	for(double i = 0; i < 2*M_PI; i+=0.01) {
		struct Vector p3d = {cos(i),sin(i), 0};

		struct Vector2D p2d = p3d_to_p2d(observer, looking, p3d, area_width, area_height);
		struct Vector2D p2d_last = p3d_to_p2d(observer, looking, p3d_last, area_width, area_height);

		draw_stroke(cr, p2d_last, p2d);

		p3d_last = p3d;
	}
	p3d_last = (struct Vector){2,0,0};
	cairo_set_source_rgb(cr, 1.0, 1.0, 0.0); // Blue color
	for(double i = 0; i < 2*M_PI; i+=0.01) {
		struct Vector p3d = {2*cos(i),2*sin(i), 0};

		struct Vector2D p2d = p3d_to_p2d(observer, looking, p3d, area_width, area_height);
		struct Vector2D p2d_last = p3d_to_p2d(observer, looking, p3d_last, area_width, area_height);

		draw_stroke(cr, p2d_last, p2d);

		p3d_last = p3d;
	}

//	print_vector(p3d);
}

void activate_test(GtkApplication *app, gpointer user_data);

void init_test() {

	GtkApplication *app = gtk_application_new ("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect (app, "activate", G_CALLBACK (activate_test), NULL);

	g_application_run (G_APPLICATION (app), 0, NULL);
	g_object_unref (app);
}

static gboolean on_timeout(gpointer data) {
	observer.x += 0.01;
	looking = norm_vector(scalar_multiply(observer, -1));
	print_vector(looking);
	gtk_widget_queue_draw(GTK_WIDGET(drawing_area));
	return G_SOURCE_CONTINUE;
}

void activate_test(GtkApplication *app, gpointer user_data) {
	/* Construct a GtkBuilder instance and load our UI description */
	setlocale(LC_NUMERIC, "C");	// Glade somehow uses commas instead of points for decimals...
	GtkBuilder *builder = gtk_builder_new ();
	gtk_builder_add_from_file(builder, "../GUI/projection_test.glade", NULL);

	gtk_builder_connect_signals(builder, NULL);

	/* Connect signal handlers to the constructed widgets. */
	GObject *window = gtk_builder_get_object (builder, "window");
	gtk_window_set_application(GTK_WINDOW (window), app);
	gtk_widget_set_visible(GTK_WIDGET (window), TRUE);


	drawing_area = gtk_builder_get_object(builder, "drawing_area");

	g_timeout_add(1.0/60*1000.0, on_timeout, drawing_area);

	/* We do not need the builder anymore */
	g_object_unref(builder);
}
