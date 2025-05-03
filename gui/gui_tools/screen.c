#include "screen.h"
#include <stdlib.h>

Screen new_screen(GtkWidget *drawing_area) {
	Screen new_screen;
	new_screen.width = gtk_widget_get_allocated_width(GTK_WIDGET(drawing_area));
	new_screen.height = gtk_widget_get_allocated_height(GTK_WIDGET(drawing_area));
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