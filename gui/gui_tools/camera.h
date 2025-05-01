#ifndef KSP_CAMERA_H
#define KSP_CAMERA_H

#include "tools/analytic_geometry.h"

typedef struct {
	struct Vector pos, looking, right;
	double max_pos_dist, min_pos_dist;
} Camera;

struct System;

struct Vector2D p3d_to_p2d(Camera camera, struct Vector p3d, int screen_width, int screen_height);
Camera new_celestial_system_camera(struct System *system, double initial_pos_pitch, double initial_pos_yaw);
void update_camera_position_from_angles(Camera *camera, double pos_pitch, double pos_yaw, double dist);
void camera_look_to_center(Camera *camera);
void update_camera_distance_to_center(Camera *camera, double new_distance);
double get_camera_pos_pitch(Camera camera);
double get_camera_pos_yaw(Camera camera);
double get_camera_distance_to_center(Camera camera);

#endif //KSP_CAMERA_H
