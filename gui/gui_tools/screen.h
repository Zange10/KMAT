#ifndef KSP_SCREEN_H
#define KSP_SCREEN_H

#include "gtk/gtk.h"
#include "tools/analytic_geometry.h"

// 4 bytes -> 3 color channels and 1 alpha channel
typedef uint32_t ScreenPixel;
// Array of pixels for screen row first
typedef ScreenPixel *PixelBuffer;

typedef struct {
	int width, height;
	PixelBuffer pixel_data;
	cairo_t *cr;
	cairo_surface_t *image_surface;
	GtkWidget *drawing_area;
	struct Vector2D last_mouse_pos;
} Screen;

Screen new_screen(GtkWidget *drawing_area);


#endif //KSP_SCREEN_H
