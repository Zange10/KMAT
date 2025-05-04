#include "camera.h"
#include "celestial_bodies.h"
#include <math.h>
#include <gtk/gtk.h>
#include "orbit_calculator/itin_tool.h"


Camera new_camera(GtkWidget *drawing_area) {
	Camera camera;
	camera.pos = vec(1,0,0);
	camera_look_to_center(&camera);
	camera.min_pos_dist = 10;
	camera.max_pos_dist = 0.1;
	camera.rotation_sensitive = 0;
	camera.screen = new_screen(drawing_area);
	return camera;
}

void reset_camera(Camera *camera) {
	camera->pos = vec(1,0,0);
	camera_look_to_center(camera);
	camera->min_pos_dist = 10;
	camera->max_pos_dist = 0.1;
	camera->rotation_sensitive = 0;
}

void update_camera_to_celestial_system(Camera *camera, struct System *system, double initial_pos_pitch, double initial_pos_yaw) {
	if(system == NULL) { reset_camera(camera); return; }
	double min_r = system->bodies[0]->orbit.periapsis;
	double max_r = system->bodies[0]->orbit.apoapsis;
	for(int i = 0; i < system->num_bodies; i++) {
		if(system->bodies[i]->orbit.periapsis < min_r) min_r = system->bodies[i]->orbit.periapsis;
		if(system->bodies[i]->orbit.apoapsis > max_r) max_r = system->bodies[i]->orbit.apoapsis;
	}
	update_camera_distance_wrt_width_at_center(camera, system->bodies[system->num_bodies/2]->orbit.a);
	update_camera_position_from_angles(camera, initial_pos_pitch, initial_pos_yaw, get_camera_distance_to_center(*camera));
	camera_look_to_center(camera);
	camera->min_pos_dist = min_r*2;
	camera->max_pos_dist = max_r*50;
	camera->rotation_sensitive = 0;
}



void camera_zoom_to_fit_itinerary(Camera *camera, struct ItinStep *itin_step) {
	if(itin_step == NULL) return;
	double highest_r = 0;
	itin_step = get_last(itin_step);
	while(itin_step != NULL) {
		if(vector_mag(itin_step->r) > highest_r) highest_r = vector_mag(itin_step->r);
		itin_step = itin_step->prev;
	}

	update_camera_distance_wrt_width_at_center(camera, highest_r);
}

struct Vector2D p3d_to_p2d(Camera camera, struct Vector p3d) {
	int screen_width = camera.screen.width, screen_height = camera.screen.height;
	struct Vector v3d = subtract_vectors(p3d, camera.pos);

	struct Vector right = norm_vector(cross_product(camera.looking, vec(0, 0, 1)));
	struct Vector up = norm_vector(cross_product(right, camera.looking));

	// Project this vector onto the camera's coordinate system (view space)
	double x = dot_product(v3d, right);
	double y = dot_product(v3d, up);
	double z = dot_product(v3d, camera.looking);

	// If the point is behind the observer, return a point at the center of the screen
	if (z <= 0) {
		return (struct Vector2D){screen_width * 10, screen_height * 10};
	}

	double hw = (screen_width < screen_height) ? screen_width : screen_height;

	double f = M_PI*2;	// don't really know what this does but works

	// Calculate the 2D coordinates based on perspective projection
	double scale = f * 1.0f / z;  // Perspective divide
	double px = ((x * scale) * (hw / 2.0f) + (double)screen_width / 2);
	double py = ((-y * scale) * (hw / 2.0f) + (double)screen_height / 2);

	return (struct Vector2D){px, py};
}

void update_camera_position_from_angles(Camera *camera, double pos_pitch, double pos_yaw, double dist) {
	camera->pos = (struct Vector) {0, dist * cos(pos_pitch), dist * sin(pos_pitch)}; // yaw = 0 -> x: right, y: up/forward
	camera->pos = rotate_vector_around_axis(camera->pos, vec(0, 0, 1), pos_yaw);
}

void update_camera_distance_wrt_width_at_center(Camera *camera, double visible_width) {
	camera->pos = scalar_multiply(norm_vector(camera->pos), visible_width*7);	// 7 from trial and error
}

void update_camera_distance_to_center(Camera *camera, double new_distance) {
	camera->pos = scalar_multiply(norm_vector(camera->pos), new_distance);
}

void camera_look_to_center(Camera *camera) {
	camera->looking = norm_vector(scalar_multiply(camera->pos, -1));
}

double get_camera_pos_pitch(Camera camera) {
	return M_PI/2 - angle_vec_vec(vec(0,0,1), camera.pos);
}

double get_camera_pos_yaw(Camera camera) {
	struct Vector pos_proj = vec(camera.pos.x, camera.pos.y, 0);
	double pos_yaw = angle_vec_vec(vec(0,1,0), pos_proj);
	if(angle_vec_vec(vec(-1,0,0), pos_proj) > M_PI/2) pos_yaw = 2*M_PI - pos_yaw;
	return pos_yaw;
}

double get_camera_distance_to_center(Camera camera) {
	return vector_mag(camera.pos);
}

cairo_t * get_camera_screen_cairo(Camera *camera) {
	return camera->screen.cr;
}

void destroy_camera(Camera *camera) {
	destroy_screen(&camera->screen);
}

// SCREEN INTERACTION ------------------------------------------------------------------------------------------

void clear_camera_screen(Camera *camera) {
	clear_screen(&camera->screen);
}

void draw_camera_image(Camera *camera) {
	draw_screen(&camera->screen);
}

void resize_camera_screen(Camera *camera) {
	resize_screen(&camera->screen);
}


// GTK CALLBACK FUNCTION FOR TRANSLATION AND ROTATION -----------------------------------------------------------

gboolean on_camera_zoom(GtkWidget *widget, GdkEventScroll *event, Camera *camera) {
	double distance = get_camera_distance_to_center(*camera);
	if(event->direction == GDK_SCROLL_UP && vector_mag(camera->pos) / 1.2 > camera->min_pos_dist) distance /= 1.2;
	if(event->direction == GDK_SCROLL_DOWN && vector_mag(camera->pos) * 1.2 < camera->max_pos_dist) distance *= 1.2;

	update_camera_distance_to_center(camera, distance);
	gtk_widget_queue_draw(camera->screen.drawing_area);
	return FALSE;
}

gboolean on_enable_camera_rotation(GtkWidget *widget, GdkEventButton *event, Camera *camera) {
	camera->rotation_sensitive = 1;
	camera->screen.last_mouse_pos = (struct Vector2D) {event->x, event->y};
	return TRUE;
}

gboolean on_disable_camera_rotation(GtkWidget *widget, GdkEventButton *event, Camera *camera) {
	camera->rotation_sensitive = 0;
	return TRUE;
}

gboolean on_camera_rotate(Camera *camera, GdkEventButton *event) {
	if (camera->rotation_sensitive) {
		struct Vector2D mouse_mov = {event->x - camera->screen.last_mouse_pos.x, event->y - camera->screen.last_mouse_pos.y};

		double pitch = get_camera_pos_pitch(*camera);
		double yaw = get_camera_pos_yaw(*camera);

		yaw -= mouse_mov.x * 0.005;
		pitch += mouse_mov.y * 0.005;
		if(pitch >=  M_PI/2) pitch =  M_PI/2 - 0.001;
		if(pitch <= -M_PI/2) pitch = -M_PI/2 + 0.001;

		update_camera_position_from_angles(camera, pitch, yaw, get_camera_distance_to_center(*camera));
		camera_look_to_center(camera);

		camera->screen.last_mouse_pos = (struct Vector2D) {event->x, event->y};
		gtk_widget_queue_draw(camera->screen.drawing_area);
		return TRUE; // Event handled
	}
	return FALSE;
}

