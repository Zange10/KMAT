#ifndef KSP_SCREEN_H
#define KSP_SCREEN_H

#include "gtk/gtk.h"
#include "geometrylib.h"

// 4 bytes -> 3 color channels and 1 alpha channel
typedef uint32_t ScreenPixel;
// Array of pixels for screen row first
typedef ScreenPixel *PixelBuffer;

typedef struct {
	double r,g,b;
} PixelColor ;

typedef struct {
	PixelBuffer pixel_data;
	cairo_t *cr;
	cairo_surface_t *image_surface;
	gboolean transparent;
} ScreenLayer;

typedef struct {
	int width, height;
	ScreenLayer static_layer;
	ScreenLayer dynamic_layer;
	GtkWidget *drawing_area;
	Vector2 last_mouse_pos;
	Vector2 mouse_pos_on_press;
	PixelColor background_color;
	gboolean dragging;
} Screen;

Screen * new_screen(GtkWidget *drawing_area, void (*resize_func)(), void (*button_press_func)(), void (*button_release_func)(), void (*mouse_motion_func)(), void (*scroll_func)());
void draw_screen(Screen *screen);
void set_screen_background_color(Screen *screen, double red, double green, double blue);
void clear_static_screen_layer(Screen *screen);
void clear_dynamic_screen_layer(Screen *screen);
void clear_screen(Screen *screen);
void resize_screen(Screen *screen);
void destroy_screen(Screen *screen);

// GTK Callback functions --------------------------
G_MODULE_EXPORT void on_draw_screen(GtkWidget *drawing_area, cairo_t *cr, Screen *screen);
void on_screen_button_press(GtkWidget *widget, GdkEventButton *event, Screen *screen);
void on_screen_button_release(GtkWidget *widget, GdkEventButton *event, Screen *screen);

#endif //KSP_SCREEN_H
