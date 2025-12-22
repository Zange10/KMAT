#include "screen.h"
#include <stdlib.h>



ScreenLayer new_screen_layer(int width, int height, gboolean transparent) {
	ScreenLayer new_layer;
	new_layer.pixel_data = calloc(width*height, sizeof(ScreenPixel));
	new_layer.image_surface = cairo_image_surface_create_for_data(
			(unsigned char *)new_layer.pixel_data,
			transparent ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24,
			width,
			height,
			width * 4	// the number of bytes between the start of rows in the buffer
	);
	new_layer.cr = cairo_create(new_layer.image_surface);
	new_layer.transparent = transparent;
	return new_layer;
}


Screen * new_screen(GtkWidget *drawing_area, void (*resize_func)(), void (*button_press_func)(), void (*button_release_func)(), void (*mouse_motion_func)(), void (*scroll_func)()) {
	Screen *new_screen = malloc(sizeof(Screen));
	new_screen->width = gtk_widget_get_allocated_width(drawing_area);
	new_screen->height = gtk_widget_get_allocated_height(drawing_area);
	new_screen->drawing_area = drawing_area;
	new_screen->static_layer = new_screen_layer(new_screen->width, new_screen->height, FALSE);
	new_screen->dynamic_layer = new_screen_layer(new_screen->width, new_screen->height, TRUE);
	new_screen->last_mouse_pos = vec2(0,0);
	new_screen->mouse_pos_on_press = vec2(0,0);
	new_screen->background_color = (PixelColor) {0,0,0};
	new_screen->dragging = FALSE;

	g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw_screen), new_screen);
	gtk_widget_add_events(drawing_area, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK);
	if(resize_func != NULL) 		g_signal_connect(drawing_area, "size-allocate", G_CALLBACK(resize_func), new_screen);
	if(button_press_func != NULL) 	g_signal_connect(drawing_area, "button-press-event", G_CALLBACK(button_press_func), new_screen);
	if(button_release_func != NULL)	g_signal_connect(drawing_area, "button-release-event", G_CALLBACK(button_release_func), new_screen);
	if(mouse_motion_func != NULL) 	g_signal_connect(drawing_area, "motion-notify-event", G_CALLBACK(mouse_motion_func), new_screen);
	if(scroll_func != NULL) 		g_signal_connect(drawing_area, "scroll-event", G_CALLBACK(scroll_func), new_screen);
	return new_screen;
}

void draw_screen(Screen *screen) {
	gtk_widget_queue_draw(screen->drawing_area);
}

void clear_static_screen_layer(Screen *screen) {
	cairo_rectangle(screen->static_layer.cr, 0, 0, screen->width, screen->height);
	cairo_set_source_rgb(screen->static_layer.cr, screen->background_color.r, screen->background_color.g, screen->background_color.b);
	cairo_fill(screen->static_layer.cr);
}

void clear_dynamic_screen_layer(Screen *screen) {
	memset(screen->dynamic_layer.pixel_data, 0, screen->width*screen->height*sizeof(ScreenPixel));
}

void clear_screen(Screen *screen) {
	clear_static_screen_layer(screen);
	clear_dynamic_screen_layer(screen);
}

void set_screen_background_color(Screen *screen, double red, double green, double blue) {
	screen->background_color = (PixelColor) {red, green, blue};
	clear_screen(screen);
}

void resize_screen_layer(ScreenLayer *layer, int width, int height) {
	if(layer->pixel_data != NULL) free(layer->pixel_data);
	if(layer->image_surface != NULL) cairo_surface_destroy(layer->image_surface);
	if(layer->cr != NULL) cairo_destroy(layer->cr);

	layer->pixel_data = calloc(width*height, sizeof(ScreenPixel));
	layer->image_surface = cairo_image_surface_create_for_data(
			(unsigned char *)layer->pixel_data,
			layer->transparent ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24,
			width,
			height,
			width * 4
	);
	layer->cr = cairo_create(layer->image_surface);
}

void resize_screen(Screen *screen) {
	int new_width = gtk_widget_get_allocated_width(screen->drawing_area);
	int new_height = gtk_widget_get_allocated_height(screen->drawing_area);
	resize_screen_layer(&screen->static_layer, new_width, new_height);
	resize_screen_layer(&screen->dynamic_layer, new_width, new_height);
	screen->width = new_width;
	screen->height = new_height;
	clear_screen(screen);
}

void destroy_screen_layer(ScreenLayer *layer) {
	if(layer->pixel_data != NULL) free(layer->pixel_data);
	if(layer->image_surface != NULL) cairo_surface_destroy(layer->image_surface);
	if(layer->cr != NULL) cairo_destroy(layer->cr);
}

void destroy_screen(Screen *screen) {
	screen->width = -1;
	screen->height = -1;
	screen->drawing_area = NULL;
	destroy_screen_layer(&screen->static_layer);
	destroy_screen_layer(&screen->dynamic_layer);
	free(screen);
}

// GTK Callback functions --------------------------
G_MODULE_EXPORT void on_draw_screen(GtkWidget *drawing_area, cairo_t *cr_drawing_area, Screen *screen) {
	cairo_set_source_surface(cr_drawing_area, screen->static_layer.image_surface, 0, 0);
	cairo_paint(cr_drawing_area);
	cairo_set_source_surface(cr_drawing_area, screen->dynamic_layer.image_surface, 0, 0);
	cairo_paint(cr_drawing_area);
}

void on_screen_button_press(GtkWidget *widget, GdkEventButton *event, Screen *screen) {
	screen->dragging = TRUE;
	screen->last_mouse_pos = (struct Vector2) {event->x, event->y};
	screen->mouse_pos_on_press = (struct Vector2) {event->x, event->y};
}

void on_screen_button_release(GtkWidget *widget, GdkEventButton *event, Screen *screen) {
	screen->dragging = FALSE;
}
