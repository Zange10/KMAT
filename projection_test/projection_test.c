#include "projection_test.h"
#include <stdio.h>
#include <gtk/gtk.h>
#include <locale.h>
#include "tools/analytic_geometry.h"
#include "gui/drawing.h"
#include "gui/gui_tools/camera.h"
#include <math.h>

GObject *drawing_area;
Camera test_camera;
struct System *test_system;



void update_proj_test_image(Camera *camera) {
	clear_camera_screen(camera);
	draw_celestial_system(*camera, test_system, 0);
//	cairo_set_source_rgb(camera->screen.cr, 1, 1, 1);
//	struct Orbit orbit = constr_orbit_w_apsides(1000e6, 101e6, 0, test_system->cb);
//	update_camera_distance_wrt_width_at_center(camera, orbit.apoapsis);
//	draw_orbit(*camera, orbit);
	draw_camera_image(camera);
}

void on_scroll(GtkWidget *widget, GdkEventScroll *event, Camera *camera) {
	on_camera_zoom(widget, event, camera);
	update_proj_test_image(camera);
}

void on_test_screen_resize(GtkWidget *widget, cairo_t *cr, Camera *camera) {
	resize_camera_screen(camera);
	update_proj_test_image(camera);
}

void on_proj_test_mouse_move(GtkWidget *widget, GdkEventButton *event, Camera *camera) {
	if (camera->rotation_sensitive) {
		on_camera_rotate(camera, event);
		update_proj_test_image(camera);
	}
}

void activate_test(GtkApplication *app, gpointer user_data);

void init_test() {
	test_system = get_system_by_name("JNSQ System");

	GtkApplication *app = gtk_application_new ("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect (app, "activate", G_CALLBACK (activate_test), NULL);

	g_application_run (G_APPLICATION (app), 0, NULL);
	g_object_unref (app);

	destroy_camera(&test_camera);
}

void activate_test(GtkApplication *app, gpointer user_data) {
	/* Construct a GtkBuilder instance and load our UI description */
	setlocale(LC_NUMERIC, "C");	// Glade somehow uses commas instead of points for decimals...
	GtkBuilder *builder = gtk_builder_new ();
	gtk_builder_add_from_file(builder, "../GUI/projection_test.glade", NULL);

	gtk_builder_connect_signals(builder, NULL);

	/* Connect signal handlers to the constructed widgets. */
	GObject *window = gtk_builder_get_object (builder, "window");


	drawing_area = gtk_builder_get_object(builder, "drawing_area");
	gtk_widget_add_events(GTK_WIDGET(drawing_area),
						  GDK_BUTTON_PRESS_MASK |
						  GDK_BUTTON_RELEASE_MASK |
						  GDK_POINTER_MOTION_MASK |
						  GDK_SCROLL_MASK);

	test_camera = new_camera(GTK_WIDGET(drawing_area));

	g_signal_connect(drawing_area, "button-press-event", G_CALLBACK(on_enable_camera_rotation), &test_camera);
	g_signal_connect(drawing_area, "button-release-event", G_CALLBACK(on_disable_camera_rotation), &test_camera);
	g_signal_connect(drawing_area, "motion-notify-event", G_CALLBACK(on_proj_test_mouse_move), &test_camera);
	g_signal_connect(drawing_area, "scroll-event", G_CALLBACK(on_scroll), &test_camera);
	g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw_screen), &test_camera.screen);
	g_signal_connect(drawing_area, "size-allocate", G_CALLBACK(on_test_screen_resize), &test_camera);
	gtk_window_set_application(GTK_WINDOW (window), app);
	gtk_widget_set_visible(GTK_WIDGET (window), TRUE);


	update_camera_to_celestial_system(&test_camera, test_system, deg2rad(90), 0);
	update_proj_test_image(&test_camera);

	/* We do not need the builder anymore */
	g_object_unref(builder);
}
