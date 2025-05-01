#include "projection_test.h"
#include <stdio.h>
#include <gtk/gtk.h>
#include <locale.h>
#include "tools/analytic_geometry.h"
#include "gui/drawing.h"
#include <math.h>


int mouse_pressed = 0;
GObject *drawing_area;

Camera camera;

double pitch, yaw, distance;

struct System *test_system;

// Global to track if right button is held
gboolean right_button_held = FALSE;
struct Vector2D mouse_pos;

gboolean on_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer user_data) {
	if(event->direction == GDK_SCROLL_UP && vector_mag(camera.pos) / 1.2 > camera.min_pos_dist) distance /= 1.2;
	if(event->direction == GDK_SCROLL_DOWN && vector_mag(camera.pos) * 1.2 < camera.max_pos_dist) distance *= 1.2;
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
		struct Vector2D mouse_mov = {event->x-mouse_pos.x, event->y-mouse_pos.y};

		yaw -= mouse_mov.x * 0.005;
		pitch += mouse_mov.y * 0.005;
		if(pitch >=  M_PI/2) pitch =  M_PI/2 - 0.001;
		if(pitch <= -M_PI/2) pitch = -M_PI/2 + 0.001;

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

void update_camera_position_from_angles(Camera *cam, double p, double y, double r, double dist) {
	cam->pos = (struct Vector) {dist*cos(p), 0, dist*sin(p)};
	cam->pos = rotate_vector_around_axis(cam->pos, vec(0,0,1), y);
}

void camera_look_to_center(Camera *cam) {
	cam->looking = norm_vector(scalar_multiply(cam->pos, -1));
}

void draw_system(cairo_t *cr, Camera *cam, int width, int height) {
	printf("%f  %f   %f\n", pitch, yaw, distance);
	update_camera_position_from_angles(&camera, pitch, yaw, 0, distance);
	camera_look_to_center(&camera);

	draw_body_orbit(cr, cam, test_system->cb, width, height);

	for(int i = 0; i < test_system->num_bodies; i++) {
		draw_body_orbit(cr, cam, test_system->bodies[i], width, height);
	}
}

void init_camera(Camera *cam, struct System *system) {
	pitch = deg2rad(90);
	yaw = 0;
	distance = test_system->bodies[2]->orbit.apoapsis*10;
	cam->min_pos_dist = test_system->bodies[0]->orbit.periapsis*2;
	cam->max_pos_dist = test_system->bodies[test_system->num_bodies-1]->orbit.periapsis*50;
	update_camera_position_from_angles(cam, pitch, yaw, 0, distance);
	camera_look_to_center(cam);
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

	draw_system(cr, &camera, area_width, area_height);
}

void activate_test(GtkApplication *app, gpointer user_data);

void init_test() {
	test_system = get_system_by_name("Stock System");
	init_camera(&camera, test_system);

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
