#include "mission_editor.h"
#include "gui/drawing.h"
#include "gui/gui_manager.h"
#include "gui/missionplanner_app/orbit_editor_win.h"
#include <math.h>
#include "orbit_calculator/mission_tool.h"

CelestSystem *me_system;
Body *me_central_body;

Camera *me_camera;

GObject *da_me;
GObject *cb_me_system;
GObject *cb_me_subsystem;
GObject *cb_me_central_body;

MissionStep *curr_step = NULL;

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

	create_combobox_dropdown_text_renderer(cb_me_system, GTK_ALIGN_CENTER);
	create_combobox_dropdown_text_renderer(cb_me_subsystem, GTK_ALIGN_CENTER);
	create_combobox_dropdown_text_renderer(cb_me_central_body, GTK_ALIGN_CENTER);
	if(get_num_available_systems() > 0) {
		update_system_dropdown(GTK_COMBO_BOX(cb_me_system));
		me_system = get_available_systems()[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_me_system))];
		update_central_body_dropdown(GTK_COMBO_BOX(cb_me_subsystem), me_system);
		update_body_dropdown(GTK_COMBO_BOX(cb_me_central_body), me_system);

		update_camera_to_celestial_body(me_camera, me_system->cb, deg2rad(90), 0);
	}
	
	// show_orbit_editor_window(&orbit1, &update_me_system_view);
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

void draw_orbit_wrt_body(Orbit orbit, Vector2 p2d_body, double radius_2d) {
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
}

// TRANSFER PLANNER SYSTEM VIEW CALLBACKS -----------------------------------------------
void update_me_system_view() {
	clear_camera_screen(me_camera);
	if(me_system == NULL) return;
	if(!curr_step) return;
	
	int screen_width = me_camera->screen->width, screen_height = me_camera->screen->height;

	Vector3 v3d = subtract_vec3(vec3(0,0,0), me_camera->pos);
	
	// Project this vector onto the camera's coordinate system (view space)
	double z = dot_vec3(v3d, me_camera->looking);
	
	double hw = (screen_width < screen_height) ? screen_width : screen_height;
	
	double f = M_PI*2;	// don't really know what this does but works
	
	// Calculate the 2D coordinates based on perspective projection
	double scale = f * 1.0f / z;  // Perspective divide

	Body *body = curr_step->orbit.body;
	set_cairo_body_color(get_camera_screen_cairo(me_camera), body);
	OSV osv_body = {.r = vec3(0,0,0)};
	Vector2 p2d_body = p3d_to_p2d(me_camera, osv_body.r);
	double radius_2d = body->radius*scale*(hw / 2.0f);
	cairo_arc(get_camera_screen_cairo(me_camera), p2d_body.x, p2d_body.y, radius_2d, 0, 2 * M_PI);
	cairo_fill(get_camera_screen_cairo(me_camera));
	
	cairo_set_source_rgb(get_camera_screen_cairo(me_camera), 1, 0, 0);
	draw_orbit_wrt_body(constr_orbit_from_osv(curr_step->orbit.osv.r, curr_step->orbit.osv.v, curr_step->orbit.body), p2d_body, radius_2d);
	
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

G_MODULE_EXPORT void on_load_mission_from_itinerary(GtkWidget* widget, gpointer data) {
	if(curr_step) free_mission(curr_step);
	curr_step = create_mission_step();
	update_camera_to_celestial_body(me_camera, curr_step->orbit.body, deg2rad(90), 0);


	// char filepath[255];
	// if(!get_path_from_file_chooser(filepath, ".itin", GTK_FILE_CHOOSER_ACTION_OPEN, "")) return;
	//
	// if(curr_transfer_tp != NULL) {
	// 	fix_tfbody = TRUE;
	// 	update_body_dropdown(GTK_COMBO_BOX(cb_tp_tfbody), NULL);
	// 	free_itinerary(get_first(curr_transfer_tp));
	// }
	// if(!is_available_system(tp_system) && tp_system != NULL) free_celestial_system(tp_system);
	// curr_transfer_tp = NULL;
	// tp_system = NULL;
	// if(gtk_combo_box_get_active(GTK_COMBO_BOX(cb_tp_system)) == get_num_available_systems()) remove_combobox_last_entry(GTK_COMBO_BOX(cb_tp_system));
	//
	// struct ItinLoadFileResults load_results = load_single_itinerary_from_bfile(filepath);
	// curr_transfer_tp = get_first(load_results.itin);
	// tp_system = load_results.system;
	// current_date_tp = curr_transfer_tp->date;
	// gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tp_tfdate), 0);
	// tp_update_bodies();
	// update_itinerary();
	// update_camera_to_celestial_system(tp_system_camera, tp_system, deg2rad(90), 0);
	// camera_zoom_to_fit_itinerary(tp_system_camera, curr_transfer_tp);
	// update_tp_system_view();
	// char system_name[50];
	// sprintf(system_name, "- %s -", tp_system->name);
	// append_combobox_entry(GTK_COMBO_BOX(cb_tp_system), system_name);
	// update_central_body_dropdown(GTK_COMBO_BOX(cb_tp_central_body), tp_system);
	//
	// struct ItinStep *step2pr = get_first(curr_transfer_tp);
	// HyperbolaParams hyp_params = get_hyperbola_params(vec3(0,0,0), step2pr->next[0]->v_dep, step2pr->v_body,
	// 																	   step2pr->body, 200e3, HYP_DEPARTURE);
	// printf("\nDeparture Hyperbola %s\n"
	// 	   "Date: %f\n"
	// 	   "OutgoingRadPer: %f km\n"
	// 	   "OutgoingC3Energy: %f km²/s²\n"
	// 	   "OutgoingRHA: %f°\n"
	// 	   "OutgoingDHA: %f°\n"
	// 	   "OutgoingBVAZI: -°\n"
	// 	   "TA: 0.0°\n",
	// 	   step2pr->body->name, step2pr->date, hyp_params.rp/1000, hyp_params.c3_energy/1e6,
	// 	   rad2deg(hyp_params.outgoing.bplane_angle), rad2deg(hyp_params.outgoing.decl));
	//
	// step2pr = step2pr->next[0];
	// while(step2pr->num_next_nodes != 0) {
	// 	if(step2pr->body != NULL) {
	// 		Vector3 v_arr = step2pr->v_arr;
	// 		Vector3 v_dep = step2pr->next[0]->v_dep;
	// 		Vector3 v_body = step2pr->v_body;
	// 		double incl = get_flyby_inclination(v_arr, v_dep, v_body, get_body_equatorial_plane(step2pr->body));
	//
	// 		hyp_params = get_hyperbola_params(step2pr->v_arr, step2pr->next[0]->v_dep, step2pr->v_body, step2pr->body, 0, HYP_FLYBY);
	// 		double dt_in_days = step2pr->date - step2pr->prev->date;
	//
	// 		printf("\nFly-by Hyperbola %s (Travel Time: %.2f days)\n"
	// 			   "Date: %f\n"
	// 			   "RadPer: %f km\n"
	// 			   "Inclination: %f°\n"
	// 			   "C3Energy: %f km²/s²\n"
	// 			   "IncomingRHA: %f°\n"
	// 			   "IncomingDHA: %f°\n"
	// 			   "IncomingBVAZI: %f°\n"
	// 			   "OutgoingRHA: %f°\n"
	// 			   "OutgoingDHA: %f°\n"
	// 			   "OutgoingBVAZI: %f°\n"
	// 			   "TA: 0.0°\n",
	// 			   step2pr->body->name, dt_in_days, step2pr->date, hyp_params.rp / 1000, rad2deg(incl),
	// 			   hyp_params.c3_energy / 1e6,
	// 			   rad2deg(hyp_params.incoming.bplane_angle), rad2deg(hyp_params.incoming.decl),
	// 			   rad2deg(hyp_params.incoming.bvazi),
	// 			   rad2deg(hyp_params.outgoing.bplane_angle), rad2deg(hyp_params.outgoing.decl),
	// 			   rad2deg(hyp_params.outgoing.bvazi));
	// 		step2pr = step2pr->next[0];
	// 	} else {
	// 		double dt_in_days = step2pr->date - step2pr->prev->date;
	// 		double dist_to_sun = mag_vec3(step2pr->r);
	//
	// 		Vector3 orbit_prograde = step2pr->v_arr;
	// 		Vector3 orbit_normal = cross_vec3(step2pr->r, step2pr->v_arr);
	// 		Vector3 orbit_radialin = cross_vec3(orbit_normal, step2pr->v_arr);
	// 		Vector3 dv_vec = subtract_vec3(step2pr->next[0]->v_dep, step2pr->v_arr);
	//
	// 		// dv vector in S/C coordinate system (prograde, radial in, normal) (sign it if projected vector more than 90° from target vector / pointing in opposite direction)
	// 		Vector3 dv_vec_sc = {
	// 				mag_vec3(proj_vec3_vec3(dv_vec, orbit_prograde)) * (angle_vec3_vec3(proj_vec3_vec3(dv_vec, orbit_prograde), orbit_prograde) < M_PI/2 ? 1 : -1),
	// 				mag_vec3(proj_vec3_vec3(dv_vec, orbit_radialin)) * (angle_vec3_vec3(proj_vec3_vec3(dv_vec, orbit_radialin), orbit_radialin) < M_PI/2 ? 1 : -1),
	// 				mag_vec3(proj_vec3_vec3(dv_vec, orbit_normal)) * (angle_vec3_vec3(proj_vec3_vec3(dv_vec, orbit_normal), orbit_normal) < M_PI/2 ? 1 : -1)
	// 		};
	//
	// 		printf("\nDeep Space Maneuver (Travel Time: %.2f days)\n"
	// 			   "Date: %f\n"
	// 			   "Distance to the Sun: %.3f AU\n"
	// 			   "Dv Prograde: %f m/s\n"
	// 			   "Dv Radial: %f m/s\n"
	// 			   "Dv Normal: %f m/s\n"
	// 			   "Total: %f m/s\n",
	// 			   dt_in_days, step2pr->date, dist_to_sun / 1.495978707e11, dv_vec_sc.x, dv_vec_sc.y, dv_vec_sc.z, mag_vec3(dv_vec_sc));
	// 		step2pr = step2pr->next[0];
	// 	}
	// }
	//
	// double dt_in_days = step2pr->date - step2pr->prev->date;
	// hyp_params = get_hyperbola_params(step2pr->v_arr, vec3(0,0,0), step2pr->v_body, step2pr->body, 200e3, HYP_ARRIVAL);
	// printf("\nArrival Hyperbola %s (Travel Time: %.2f days)\n"
	// 	   "Date: %f\n"
	// 	   "IncomingRadPer: %f km\n"
	// 	   "IncomingC3Energy: %f km²/s²\n"
	// 	   "IncomingRHA: %f°\n"
	// 	   "IncomingDHA: %f°\n"
	// 	   "IncomingBVAZI: -°\n"
	// 	   "TA: 0.0°\n",
	// 	   step2pr->body->name, dt_in_days, step2pr->date, hyp_params.rp/1000, hyp_params.c3_energy/1e6,
	// 	   rad2deg(hyp_params.incoming.bplane_angle), rad2deg(hyp_params.incoming.decl));
}