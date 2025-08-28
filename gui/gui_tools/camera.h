#ifndef KSP_CAMERA_H
#define KSP_CAMERA_H

#include "geometrylib.h"
#include "screen.h"
#include <gtk/gtk.h>

typedef struct {
	Vector3 pos, looking, right;
	double max_pos_dist, min_pos_dist;
	gboolean rotation_sensitive;
	Screen *screen;
} Camera;

typedef struct CelestSystem CelestSystem;
struct ItinStep;

Camera * new_camera(GtkWidget *drawing_area, void (*resize_func)(), void (*button_press_func)(), void (*button_release_func)(), void (*mouse_motion_func)(), void (*scroll_func)());
Vector2 p3d_to_p2d(Camera *camera, Vector3 p3d);
void update_camera_to_celestial_system(Camera *camera, CelestSystem *system, double initial_pos_pitch, double initial_pos_yaw);
void update_camera_position_from_angles(Camera *camera, double pos_pitch, double pos_yaw, double dist);
void camera_look_to_center(Camera *camera);
void update_camera_distance_to_center(Camera *camera, double new_distance);
void update_camera_distance_wrt_width_at_center(Camera *camera, double visible_width);
void camera_zoom_to_fit_itinerary(Camera *camera, struct ItinStep *itin_step);
double get_camera_pos_pitch(Camera *camera);
double get_camera_pos_yaw(Camera *camera);
double get_camera_distance_to_center(Camera *camera);
void destroy_camera(Camera *camera);

cairo_t * get_camera_screen_cairo(Camera *camera);

void draw_camera_image(Camera *camera);
void clear_camera_screen(Camera *camera);
void resize_camera_screen(Camera *camera);

void on_camera_zoom(GtkWidget *widget, GdkEventScroll *event, Camera *camera);
void on_enable_camera_rotation(GtkWidget *widget, GdkEventButton *event, Camera *camera);
void on_disable_camera_rotation(GtkWidget *widget, GdkEventButton *event, Camera *camera);
void on_camera_rotate(Camera *camera, GdkEventButton *event);

#endif //KSP_CAMERA_H
