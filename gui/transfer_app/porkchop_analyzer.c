#include "porkchop_analyzer.h"
#include "tools/analytic_geometry.h"
#include "celestial_bodies.h"
#include "orbit_calculator/transfer_tools.h"
#include "gui/drawing.h"
#include "gui/transfer_app.h"

#include <string.h>
#include <locale.h>



GObject *da_pa_porkchop;


void init_porkchop_analyzer(GtkBuilder *builder) {
	da_pa_porkchop = gtk_builder_get_object(builder, "da_pa_porkchop");
}

void on_porkchop_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	int area_width = allocation.width;
	int area_height = allocation.height;
	struct Vector2D center = {(double) area_width/2, (double) area_height/2};

	// reset drawing area
	cairo_rectangle(cr, 0, 0, area_width, area_height);
	cairo_set_source_rgb(cr, 0,0,0);
	cairo_fill(cr);

	draw_porkchop(cr, area_width, area_height);
}