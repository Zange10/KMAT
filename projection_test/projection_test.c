#include "projection_test.h"
#include <stdio.h>
#include <gtk/gtk.h>
#include <locale.h>
#include "tools/analytic_geometry.h"
#include "gui/drawing.h"
#include "gui/gui_tools/camera.h"
#include <math.h>

GObject *drawing_area;
Camera camera;
struct System *test_system;
gboolean right_button_held = FALSE;
struct Vector2D mouse_pos;



gboolean on_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer user_data) {
	double distance = get_camera_distance_to_center(camera);
	if(event->direction == GDK_SCROLL_UP && vector_mag(camera.pos) / 1.2 > camera.min_pos_dist) distance /= 1.2;
	if(event->direction == GDK_SCROLL_DOWN && vector_mag(camera.pos) * 1.2 < camera.max_pos_dist) distance *= 1.2;

	update_camera_distance_to_center(&camera, distance);
	gtk_widget_queue_draw(GTK_WIDGET(drawing_area));
	return TRUE;
}

gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
	if (event->button == GDK_BUTTON_SECONDARY) {
		right_button_held = TRUE;
		mouse_pos = (struct Vector2D) {event->x, event->y};
		return TRUE;
	}
	return FALSE;
}

gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
	if (event->button == GDK_BUTTON_SECONDARY) {
		right_button_held = FALSE;
		return TRUE;
	}
	return FALSE;
}

gboolean on_mouse_move(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
	// Check if it's the right mouse button
	if (right_button_held) {
		// event->x and event->y are relative to the widget (drawing area)
		struct Vector2D mouse_mov = {event->x - mouse_pos.x, event->y - mouse_pos.y};

		double pitch = get_camera_pos_pitch(camera);
		double yaw = get_camera_pos_yaw(camera);

		yaw -= mouse_mov.x * 0.005;
		pitch += mouse_mov.y * 0.005;
		if(pitch >=  M_PI/2) pitch =  M_PI/2 - 0.001;
		if(pitch <= -M_PI/2) pitch = -M_PI/2 + 0.001;

		update_camera_position_from_angles(&camera, pitch, yaw, get_camera_distance_to_center(camera));
		camera_look_to_center(&camera);

		mouse_pos = (struct Vector2D) {event->x, event->y};
		gtk_widget_queue_draw(GTK_WIDGET(drawing_area));
		return TRUE; // Event handled
	}
	return FALSE;
}

void draw_body_orbit(cairo_t *cr, Camera *cam, struct Body *body, int width, int height) {
	set_cairo_body_color(cr, body);
	struct OSV osv_body = {.r = vec(0,0,0)};
	if(body != test_system->cb) osv_body = osv_from_elements(body->orbit, 0, test_system);
	struct Vector2D p2d_body = p3d_to_p2d(*cam, osv_body.r, width, height);
	cairo_arc(cr, p2d_body.x, p2d_body.y, 5, 0, 2 * M_PI);
	cairo_fill(cr);

	if(body == test_system->cb) return;


	struct Vector2D last_p2d = p2d_body;
	double dtheta_step = deg2rad(0.5);

	for(double dtheta = 0; dtheta < M_PI*2 + dtheta_step; dtheta += dtheta_step) {
		struct OSV osv = propagate_orbit_theta(constr_orbit_from_osv(osv_body.r, osv_body.v, test_system->cb), dtheta, test_system->cb);
		p2d_body = p3d_to_p2d(*cam, osv.r, width, height);
		draw_stroke(cr, last_p2d, p2d_body);
		last_p2d = p2d_body;
	}
}

void draw_system(cairo_t *cr, Camera *cam, int width, int height) {
	draw_body_orbit(cr, cam, test_system->cb, width, height);

	for(int i = 0; i < test_system->num_bodies; i++) {
		draw_body_orbit(cr, cam, test_system->bodies[i], width, height);
	}
}

void on_draw_projection(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
	// reset drawing area
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	int area_width = allocation.width;
	int area_height = allocation.height;

	cairo_rectangle(cr, 0, 0, area_width, area_height);
	cairo_set_source_rgb(cr, 0,0,0);
	cairo_fill(cr);

	draw_celestial_system(cr, camera, test_system, 0, area_width, area_height);
}

void activate_test(GtkApplication *app, gpointer user_data);

void init_test() {
	test_system = get_system_by_name("JNSQ System");
	camera = new_celestial_system_camera(test_system, deg2rad(90), 0);

	GtkApplication *app = gtk_application_new ("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect (app, "activate", G_CALLBACK (activate_test), NULL);

	g_application_run (G_APPLICATION (app), 0, NULL);
	g_object_unref (app);
}

static gboolean on_timeout(gpointer data) {

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


	drawing_area = gtk_builder_get_object(builder, "drawing_area");
	gtk_widget_add_events(GTK_WIDGET(drawing_area),
						  GDK_BUTTON_PRESS_MASK |
						  GDK_BUTTON_RELEASE_MASK |
						  GDK_POINTER_MOTION_MASK |
						  GDK_SCROLL_MASK);

	g_signal_connect(drawing_area, "button-press-event", G_CALLBACK(on_button_press), NULL);
	g_signal_connect(drawing_area, "button-release-event", G_CALLBACK(on_button_release), NULL);
	g_signal_connect(GTK_WIDGET(drawing_area), "motion-notify-event", G_CALLBACK(on_mouse_move), NULL);
	g_signal_connect(drawing_area, "scroll-event", G_CALLBACK(on_scroll), NULL);
	gtk_window_set_application(GTK_WINDOW (window), app);
	gtk_widget_set_visible(GTK_WIDGET (window), TRUE);

	/* We do not need the builder anymore */
	g_object_unref(builder);
}
