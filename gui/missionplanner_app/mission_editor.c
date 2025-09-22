#include "mission_editor.h"
#include "gui/drawing.h"
#include "gui/gui_manager.h"
#include <math.h>

CelestSystem *me_system;
Body *me_central_body;
Orbit orbit;
Orbit orbit2;

Camera *me_camera;

GObject *da_me;
GObject *cb_me_system;
GObject *cb_me_subsystem;
GObject *cb_me_central_body;

double current_date_me;

void update_me_system_view();
void on_me_screen_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer *ptr);
void on_me_screen_resize(GtkWidget *widget, cairo_t *cr, gpointer *ptr);
void on_me_screen_mouse_move(GtkWidget *widget, GdkEventButton *event, gpointer *ptr);


void init_mission_editor(GtkBuilder *builder) {
	struct Datetime date = {1950, 1, 1, 0, 0, 0};
	current_date_me = convert_date_JD(date);

	cb_me_system = gtk_builder_get_object(builder, "cb_me_system");
	cb_me_subsystem = gtk_builder_get_object(builder, "cb_me_subsystem");
	cb_me_central_body = gtk_builder_get_object(builder, "cb_me_central_body");
	da_me = gtk_builder_get_object(builder, "da_me");
	
	me_camera = new_camera(GTK_WIDGET(da_me), &on_me_screen_resize, &on_enable_camera_rotation, &on_disable_camera_rotation, &on_me_screen_mouse_move, &on_me_screen_scroll);
	
	me_system = get_system_by_name("Solar System (Ephemeris)");
	me_central_body = me_system->cb;
	orbit = constr_orbit_from_elements(1e9, 0, deg2rad(60), 0, 0, 0, me_central_body);
	orbit2 = constr_orbit_from_elements(me_central_body->radius, 0, deg2rad(0), 0, 0, 0, me_central_body);
	
	create_combobox_dropdown_text_renderer(cb_me_system, GTK_ALIGN_CENTER);
	create_combobox_dropdown_text_renderer(cb_me_subsystem, GTK_ALIGN_CENTER);
	create_combobox_dropdown_text_renderer(cb_me_central_body, GTK_ALIGN_CENTER);
	if(get_num_available_systems() > 0) {
		update_system_dropdown(GTK_COMBO_BOX(cb_me_system));
		me_system = get_available_systems()[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_me_system))];
		update_central_body_dropdown(GTK_COMBO_BOX(cb_me_subsystem), me_system);
		update_body_dropdown(GTK_COMBO_BOX(cb_me_central_body), me_system);

		update_camera_to_celestial_system(me_camera, me_system, deg2rad(90), 0);
	}
}

void draw_stroke_wrt_body(Vector2 p2d_body, double radius_2d, Vector2 p0, Vector2 p1) {
	bool drew_sections = false;
	
	if(	fmin(p0.x, p1.x) <= p2d_body.x+radius_2d &&
		   fmax(p0.x, p1.x) >= p2d_body.x-radius_2d &&
		   fmin(p0.y, p1.y) <= p2d_body.y+radius_2d &&
		   fmax(p0.y, p1.y) >= p2d_body.y-radius_2d) {
		
		if(mag_vec2(subtract_vec2(p0, p2d_body)) < radius_2d &&
		   mag_vec2(subtract_vec2(p1, p2d_body)) < radius_2d) return;
			
		double m = (p1.y - p0.y)/(p1.x - p0.x);
		double n = p0.y - m*p0.x;
		
		double p = (-2*p2d_body.x + 2*m*n - 2*m*p2d_body.y)/(1 + m*m);
		double q =
				(p2d_body.x*p2d_body.x + n*n - 2*n*p2d_body.y + p2d_body.y*p2d_body.y - radius_2d*radius_2d)/(1 + m*m);
		
		
		if((p/2)*(p/2) - q > 0) {
			double x1 = -p/2 - sqrt((p/2)*(p/2) - q);
			double x2 = -p/2 + sqrt((p/2)*(p/2) - q);
			if(p0.x > p1.x) {
				double temp = x1;
				x1 = x2;
				x2 = temp;
			}
			Vector2 p0x = {x1, m*x1 + n};
			Vector2 p1x = {x2, m*x2 + n};
			
			if(fmin(p0.x, p1.x) <= p0x.x &&
			   fmax(p0.x, p1.x) >= p0x.x &&
			   fmin(p0.y, p1.y) <= p0x.y &&
			   fmax(p0.y, p1.y) >= p0x.y) {
				draw_stroke(get_camera_screen_cairo(me_camera), p0, p0x);
				drew_sections = true;
			}
			if(fmin(p0.x, p1.x) <= p1x.x &&
			   fmax(p0.x, p1.x) >= p1x.x &&
			   fmin(p0.y, p1.y) <= p1x.y &&
			   fmax(p0.y, p1.y) >= p1x.y) {
				draw_stroke(get_camera_screen_cairo(me_camera), p1x, p1);
				drew_sections = true;
			}
		}
	}
	
	if(!drew_sections) draw_stroke(get_camera_screen_cairo(me_camera), p0, p1);
}

// TRANSFER PLANNER SYSTEM VIEW CALLBACKS -----------------------------------------------
void update_me_system_view() {
	me_camera->min_pos_dist = me_central_body->radius;
	clear_camera_screen(me_camera);
	if(me_system == NULL) return;
	
	int screen_width = me_camera->screen->width, screen_height = me_camera->screen->height;

	Vector3 v3d = subtract_vec3(vec3(0,0,0), me_camera->pos);
	
	// Project this vector onto the camera's coordinate system (view space)
	double z = dot_vec3(v3d, me_camera->looking);
	
	double hw = (screen_width < screen_height) ? screen_width : screen_height;
	
	double f = M_PI*2;	// don't really know what this does but works
	
	// Calculate the 2D coordinates based on perspective projection
	double scale = f * 1.0f / z;  // Perspective divide
	
	set_cairo_body_color(get_camera_screen_cairo(me_camera), me_central_body);
	OSV osv_body = {.r = vec3(0,0,0)};
	Vector2 p2d_body = p3d_to_p2d(me_camera, osv_body.r);
	double radius_2d = me_central_body->radius*scale*(hw / 2.0f);
	cairo_arc(get_camera_screen_cairo(me_camera), p2d_body.x, p2d_body.y, radius_2d, 0, 2 * M_PI);
	cairo_fill(get_camera_screen_cairo(me_camera));
	
	
	Vector2 p0 = {600, 100};
	Vector2 p1 = {100, 300};
	
	cairo_set_source_rgb(get_camera_screen_cairo(me_camera), 0, 0.5, 0);
	draw_stroke_wrt_body(p2d_body, radius_2d, p0, p1);
	
	
	cairo_set_source_rgb(get_camera_screen_cairo(me_camera), 1, 0, 0);
	OSV osv = osv_from_orbit(orbit);
	Vector2 p2d = p3d_to_p2d(me_camera, osv.r);
	
	Vector2 last_p2d = p2d;
	double ta_step = deg2rad(0.5);
	
	for(double dta = 0; dta < M_PI*2 + ta_step; dta += ta_step) {
		orbit.ta += ta_step;
		osv = osv_from_orbit(orbit);
		p2d = p3d_to_p2d(me_camera, osv.r);
		if(sq_mag_vec3(subtract_vec3(osv.r, me_camera->pos)) > sq_mag_vec3(subtract_vec3(vec3(0,0,0), me_camera->pos))) {
			draw_stroke_wrt_body(p2d_body, radius_2d, last_p2d, p2d);
		} else {
			draw_stroke(get_camera_screen_cairo(me_camera), last_p2d, p2d);
		}
		last_p2d = p2d;
	}
	
	
//	draw_orbit(me_camera, orbit);
	cairo_set_source_rgb(get_camera_screen_cairo(me_camera), 0, 0, 1);
	draw_orbit(me_camera, orbit2);
	
	draw_camera_image(me_camera);
}

void on_me_screen_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer *ptr) {
	on_camera_zoom(widget, event, me_camera);
	update_me_system_view();
}

void on_me_screen_resize(GtkWidget *widget, cairo_t *cr, gpointer *ptr) {
	resize_camera_screen(me_camera);
	update_me_system_view();
}

void on_me_screen_mouse_move(GtkWidget *widget, GdkEventButton *event, gpointer *ptr) {
	if (me_camera->rotation_sensitive) {
		on_camera_rotate(me_camera, event);
		update_me_system_view();
	}
}