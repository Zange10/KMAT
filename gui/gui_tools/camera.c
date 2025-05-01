#include "camera.h"
#include "celestial_bodies.h"
#include <math.h>



Camera new_celestial_system_camera(struct System *system, double initial_pos_pitch, double initial_pos_yaw) {
	Camera camera;
	double initial_distance = system->bodies[system->num_bodies/2]->orbit.apoapsis*10;
	update_camera_position_from_angles(&camera, initial_pos_pitch, initial_pos_yaw, initial_distance);
	camera_look_to_center(&camera);
	camera.min_pos_dist = system->bodies[0]->orbit.periapsis*2;
	camera.max_pos_dist = system->bodies[system->num_bodies-1]->orbit.periapsis*50;

	return camera;
}

struct Vector2D p3d_to_p2d(Camera camera, struct Vector p3d, int screen_width, int screen_height) {
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