#include "camera.h"
#include "tools/celestial_systems.h"
#include <math.h>
#include <gtk/gtk.h>
#include "orbit_calculator/itin_tool.h"


Camera * new_camera(GtkWidget *drawing_area, void (*resize_func)(), void (*button_press_func)(), void (*button_release_func)(), void (*mouse_motion_func)(), void (*scroll_func)()) {
	Camera *new_camera = malloc(sizeof(Camera));
	new_camera->pos = vec3(1, 0, 0);
	camera_look_to_center(new_camera);
	new_camera->min_pos_dist = 10;
	new_camera->max_pos_dist = 0.1;
	new_camera->rotation_sensitive = FALSE;
	new_camera->screen = new_screen(drawing_area, NULL, NULL, NULL, NULL, NULL);

//	gtk_widget_add_events(drawing_area, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK);
	if(resize_func != NULL) 		g_signal_connect(drawing_area, "size-allocate", G_CALLBACK(resize_func), new_camera);
	if(button_press_func != NULL) 	g_signal_connect(drawing_area, "button-press-event", G_CALLBACK(button_press_func), new_camera);
	if(button_release_func != NULL)	g_signal_connect(drawing_area, "button-release-event", G_CALLBACK(button_release_func), new_camera);
	if(mouse_motion_func != NULL) 	g_signal_connect(drawing_area, "motion-notify-event", G_CALLBACK(mouse_motion_func), new_camera);
	if(scroll_func != NULL) 		g_signal_connect(drawing_area, "scroll-event", G_CALLBACK(scroll_func), new_camera);
	return new_camera;
}

void reset_camera(Camera *camera) {
	camera->pos = vec3(1,0,0);
	camera_look_to_center(camera);
	camera->min_pos_dist = 10;
	camera->max_pos_dist = 0.1;
	camera->rotation_sensitive = FALSE;
}

void update_camera_to_celestial_system(Camera *camera, CelestSystem *system, double initial_pos_pitch, double initial_pos_yaw) {
	if(system == NULL) { reset_camera(camera); return; }
	double min_r = calc_orbit_periapsis(system->bodies[0]->orbit);
	double max_r = calc_orbit_apoapsis(system->bodies[0]->orbit);
	for(int i = 0; i < system->num_bodies; i++) {
		if(calc_orbit_periapsis(system->bodies[i]->orbit) < min_r) min_r = calc_orbit_periapsis(system->bodies[i]->orbit);
		if(calc_orbit_apoapsis(system->bodies[i]->orbit) > max_r) max_r = calc_orbit_apoapsis(system->bodies[i]->orbit);
	}
	update_camera_distance_wrt_width_at_center(camera, system->bodies[system->num_bodies/2]->orbit.a);
	update_camera_position_from_angles(camera, initial_pos_pitch, initial_pos_yaw, get_camera_distance_to_center(camera));
	camera_look_to_center(camera);
	camera->min_pos_dist = min_r*2;
	camera->max_pos_dist = max_r*50;
	camera->rotation_sensitive = FALSE;
}



void camera_zoom_to_fit_itinerary(Camera *camera, struct ItinStep *itin_step) {
	if(itin_step == NULL) return;
	double highest_r = 0;
	itin_step = get_last(itin_step);
	while(itin_step != NULL) {
		if(mag_vec3(itin_step->r) > highest_r) highest_r = mag_vec3(itin_step->r);
		itin_step = itin_step->prev;
	}

	update_camera_distance_wrt_width_at_center(camera, highest_r);
}

Vector2 p3d_to_p2d(Camera *camera, Vector3 p3d) {
	int screen_width = camera->screen->width, screen_height = camera->screen->height;
	Vector3 v3d = subtract_vec3(p3d, camera->pos);

	Vector3 right = norm_vec3(cross_vec3(camera->looking, vec3(0, 0, 1)));
	Vector3 up = norm_vec3(cross_vec3(right, camera->looking));

	// Project this vector onto the camera's coordinate system (view space)
	double x = dot_vec3(v3d, right);
	double y = dot_vec3(v3d, up);
	double z = dot_vec3(v3d, camera->looking);

	// If the point is behind the observer, return a point at the center of the screen
	if (z <= 0) {
		return (Vector2){screen_width * 10, screen_height * 10};
	}

	double hw = (screen_width < screen_height) ? screen_width : screen_height;

	double f = M_PI*2;	// don't really know what this does but works

	// Calculate the 2D coordinates based on perspective projection
	double scale = f * 1.0f / z;  // Perspective divide
	double px = ((x * scale) * (hw / 2.0f) + (double)screen_width / 2);
	double py = ((-y * scale) * (hw / 2.0f) + (double)screen_height / 2);

	return (Vector2){px, py};
}

void update_camera_position_from_angles(Camera *camera, double pos_pitch, double pos_yaw, double dist) {
	camera->pos = (Vector3) {0, dist * cos(pos_pitch), dist * sin(pos_pitch)}; // yaw = 0 -> x: right, y: up/forward
	camera->pos = rotate_vector_around_axis(camera->pos, vec3(0, 0, 1), pos_yaw);
}

void update_camera_distance_wrt_width_at_center(Camera *camera, double visible_width) {
	camera->pos = scale_vec3(norm_vec3(camera->pos), visible_width*7);	// 7 from trial and error
}

void update_camera_distance_to_center(Camera *camera, double new_distance) {
	camera->pos = scale_vec3(norm_vec3(camera->pos), new_distance);
}

void camera_look_to_center(Camera *camera) {
	camera->looking = norm_vec3(scale_vec3(camera->pos, -1));
}

double get_camera_pos_pitch(Camera *camera) {
	return M_PI/2 - angle_vec3_vec3(vec3(0,0,1), camera->pos);
}

double get_camera_pos_yaw(Camera *camera) {
	Vector3 pos_proj = vec3(camera->pos.x, camera->pos.y, 0);
	double pos_yaw = angle_vec3_vec3(vec3(0,1,0), pos_proj);
	if(angle_vec3_vec3(vec3(-1,0,0), pos_proj) > M_PI/2) pos_yaw = 2*M_PI - pos_yaw;
	return pos_yaw;
}

double get_camera_distance_to_center(Camera *camera) {
	return mag_vec3(camera->pos);
}

cairo_t * get_camera_screen_cairo(Camera *camera) {
	return camera->screen->static_layer.cr;
}

void destroy_camera(Camera *camera) {
	destroy_screen(camera->screen);
	free(camera);
}

// SCREEN INTERACTION ------------------------------------------------------------------------------------------

void clear_camera_screen(Camera *camera) {
	clear_screen(camera->screen);
}

void draw_camera_image(Camera *camera) {
	draw_screen(camera->screen);
}

void resize_camera_screen(Camera *camera) {
	resize_screen(camera->screen);
}


// GTK CALLBACK FUNCTION FOR TRANSLATION AND ROTATION -----------------------------------------------------------

void on_camera_zoom(GtkWidget *widget, GdkEventScroll *event, Camera *camera) {
	double distance = get_camera_distance_to_center(camera);
	if(event->direction == GDK_SCROLL_UP && mag_vec3(camera->pos) / 1.2 > camera->min_pos_dist) distance /= 1.2;
	if(event->direction == GDK_SCROLL_DOWN && mag_vec3(camera->pos) * 1.2 < camera->max_pos_dist) distance *= 1.2;

	update_camera_distance_to_center(camera, distance);
}

void on_enable_camera_rotation(GtkWidget *widget, GdkEventButton *event, Camera *camera) {
	camera->rotation_sensitive = TRUE;
	camera->screen->last_mouse_pos = (Vector2) {event->x, event->y};
}

void on_disable_camera_rotation(GtkWidget *widget, GdkEventButton *event, Camera *camera) {
	camera->rotation_sensitive = FALSE;
}

void on_camera_rotate(Camera *camera, GdkEventButton *event) {
	if (camera->rotation_sensitive) {
		Vector2 mouse_mov = {event->x - camera->screen->last_mouse_pos.x, event->y - camera->screen->last_mouse_pos.y};

		double pitch = get_camera_pos_pitch(camera);
		double yaw = get_camera_pos_yaw(camera);

		yaw -= mouse_mov.x * 0.005;
		pitch += mouse_mov.y * 0.005;
		if(pitch >=  M_PI/2) pitch =  M_PI/2 - 0.001;
		if(pitch <= -M_PI/2) pitch = -M_PI/2 + 0.001;

		update_camera_position_from_angles(camera, pitch, yaw, get_camera_distance_to_center(camera));
		camera_look_to_center(camera);

		camera->screen->last_mouse_pos = (Vector2) {event->x, event->y};
		gtk_widget_queue_draw(camera->screen->drawing_area);
	}
}

