#include "window.h"
#include "drawing.h"
#include "celestial_bodies.h"

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	int window_width = allocation.width;
	int window_height = allocation.height;
	struct Vector2D center = {(double) window_width/2, (double) window_height/2};
	
	cairo_rectangle(cr, 0, 0, window_width, window_height);
	cairo_set_source_rgb(cr, 0,0,0);
	cairo_fill(cr);
	cairo_set_source_rgb(cr, 0,0,1);
	draw_orbit(cr,center, EARTH()->orbit.a, EARTH()->orbit.e, SUN()->mu);
	cairo_set_source_rgb(cr, 1,0.2,0);
	draw_orbit(cr,center, MARS()->orbit.a, MARS()->orbit.e, SUN()->mu);
	cairo_set_source_rgb(cr, 0.6,0.6,0);
	draw_orbit(cr,center, VENUS()->orbit.a, VENUS()->orbit.e, SUN()->mu);
	cairo_set_source_rgb(cr, 0.6,0.4,0.2);
	draw_orbit(cr,center, JUPITER()->orbit.a, JUPITER()->orbit.e, SUN()->mu);
	cairo_fill(cr);
	return TRUE;
}

struct Window create_window(int width, int height) {
	GtkWidget *window;
	GtkWidget *drawing_area;
	gtk_init(NULL, NULL);
	
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "My GTK Window");
	gtk_window_set_default_size(GTK_WINDOW(window), width, height);
	
	drawing_area = gtk_drawing_area_new();
	g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw), NULL);
	
	gtk_container_add(GTK_CONTAINER(window), drawing_area);
	gtk_widget_show_all(window);
	gtk_main();
	
	
	struct Window new_window = {.window = window, .width = width, .height = height};
	
	return new_window;
}
