#include "screen.h"
#include <stdlib.h>

Screen new_screen(GtkWidget *drawing_area) {
	Screen new_screen;
	new_screen.width = gtk_widget_get_allocated_width(drawing_area);
	new_screen.height = gtk_widget_get_allocated_height(drawing_area);
	new_screen.drawing_area = drawing_area;
	new_screen.pixel_data = calloc(new_screen.width*new_screen.height, sizeof(ScreenPixel));
	new_screen.image_surface = cairo_image_surface_create_for_data(
			(unsigned char *)new_screen.pixel_data,
			CAIRO_FORMAT_RGB24,
			new_screen.width,
			new_screen.height,
			new_screen.width * 4	// the number of bytes between the start of rows in the buffer
	);
	new_screen.cr = cairo_create(new_screen.image_surface);
	new_screen.last_mouse_pos = vec2D(0,0);
	return new_screen;
}

void draw_screen(Screen *screen) {
	gtk_widget_queue_draw(screen->drawing_area);
}

void clear_screen(Screen *screen) {
	cairo_rectangle(screen->cr, 0, 0, screen->width, screen->height);
	cairo_set_source_rgb(screen->cr, 0,0,0);
	cairo_fill(screen->cr);
}

void resize_screen(Screen *screen) {
	int new_width = gtk_widget_get_allocated_width(screen->drawing_area);
	int new_height = gtk_widget_get_allocated_height(screen->drawing_area);
	PixelBuffer new_pixel_data = calloc(new_width*new_height, sizeof(ScreenPixel));

	for(int x = 0; x < (new_width < screen->width ? new_width:screen->width); x++) {
		for(int y = 0; y < (new_height < screen->height ? new_height:screen->height); y++) {
			new_pixel_data[y*new_width+x] = screen->pixel_data[y*screen->width+x];
		}
	}

	screen->width = new_width;
	screen->height = new_height;
	free(screen->pixel_data);
	screen->pixel_data = new_pixel_data;
	if(screen->image_surface != NULL) cairo_surface_destroy(screen->image_surface);
	screen->image_surface = cairo_image_surface_create_for_data(
			(unsigned char *)screen->pixel_data,
			CAIRO_FORMAT_RGB24,
			screen->width,
			screen->height,
			screen->width * 4
	);
	screen->cr = cairo_create(screen->image_surface);
}

// GTK Callback functions --------------------------
G_MODULE_EXPORT void on_draw_screen(GtkWidget *drawing_area, cairo_t *cr_drawing_area, Screen *screen) {
	cairo_set_source_surface(cr_drawing_area, screen->image_surface, 0, 0);
	cairo_paint(cr_drawing_area);
}
