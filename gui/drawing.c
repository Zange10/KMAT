#include <stdio.h>

#include "drawing.h"
#include "settings.h"
#include "math.h"


void draw_body(Camera *camera, CelestSystem *system, Body *body, double jd_date) {
	set_cairo_body_color(get_camera_screen_cairo(camera), body);
	OSV osv_body = {.r = vec3(0,0,0)};
	if(body != system->cb) osv_body = system->prop_method == ORB_ELEMENTS ?
										osv_from_elements(body->orbit, jd_date) :
									  	osv_from_ephem(body->ephem, body->num_ephems, jd_date, system->cb);
	Vector2 p2d_body = p3d_to_p2d(camera, osv_body.r);
	cairo_arc(get_camera_screen_cairo(camera), p2d_body.x, p2d_body.y, 5, 0, 2 * M_PI);
	cairo_fill(get_camera_screen_cairo(camera));
}

void draw_orbit(Camera *camera, Orbit orbit) {
	OSV osv = osv_from_orbit(orbit);
	Vector2 p2d = p3d_to_p2d(camera, osv.r);

	Vector2 last_p2d = p2d;
	double ta_step = deg2rad(0.5);

	for(double dta = 0; dta < M_PI*2 + ta_step; dta += ta_step) {
		orbit.ta += ta_step;
		osv = osv_from_orbit(orbit);
		p2d = p3d_to_p2d(camera, osv.r);
		draw_stroke(get_camera_screen_cairo(camera), last_p2d, p2d);
		last_p2d = p2d;
	}
}

void draw_celestial_system(Camera *camera, CelestSystem *system, double jd_date) {
	draw_body(camera, system, system->cb, jd_date);

	for(int i = 0; i < system->num_bodies; i++) {
		draw_body(camera, system, system->bodies[i], jd_date);
		draw_orbit(camera, system->bodies[i]->orbit);
	}
}

void draw_trajectory(Camera *camera, struct OSV osv0, double dt, struct Body *attractor) {
	if(dt <= 1.0/86400) return;
	Orbit orbit = constr_orbit_from_osv(osv0.r, osv0.v, attractor);

	if(calc_orbital_period(orbit) < dt*86400 && orbit.e < 1) {
		draw_orbit(camera, orbit);
	}

	double ta0 = orbit.ta;
	double ta1 = propagate_orbit_time(orbit, dt*86400).ta;

	if(ta1 < ta0) ta1 += 2*M_PI;

	OSV osv = osv0;
	Vector2 p2d = p3d_to_p2d(camera, osv.r);
	Vector2 last_p2d = p2d;
	double ta_step = (ta1 - ta0)/1000;

	for(double dta = 0; dta <= (ta1 - ta0); dta += ta_step) {
		orbit.ta += ta_step;
		osv = osv_from_orbit(orbit);
		p2d = p3d_to_p2d(camera, osv.r);
		draw_stroke(get_camera_screen_cairo(camera), last_p2d, p2d);
		last_p2d = p2d;
	}
}

void draw_itinerary_spacecraft(Camera *camera, Vector3 r) {
	cairo_set_source_rgb(get_camera_screen_cairo(camera), 1, 0.4, 0.1);
	Vector2 p2d = p3d_to_p2d(camera, r);
	cairo_arc(get_camera_screen_cairo(camera), p2d.x, p2d.y, 3, 0, 2 * M_PI);
	cairo_fill(get_camera_screen_cairo(camera));
}

void draw_itinerary_step_point(Camera *camera, Vector3 r) {
	int cross_length = 4;
	cairo_set_source_rgb(get_camera_screen_cairo(camera), 1, 0, 0);
	Vector2 p2d = p3d_to_p2d(camera, r);
	for(int i = -1; i<=1; i+=2) {
		Vector2 p1 = {p2d.x - cross_length, p2d.y + cross_length*i};
		Vector2 p2 = {p2d.x + cross_length, p2d.y - cross_length*i};
		draw_stroke(get_camera_screen_cairo(camera), p1, p2);
	}
}

void set_trajectory_color(cairo_t *cr, int trajectory_is_in_the_past, int trajectory_is_viable) {
	if(trajectory_is_in_the_past) {
		if(trajectory_is_viable) cairo_set_source_rgb(cr, 0, 0.6, 0.4);
		else cairo_set_source_rgb(cr, 0.6, 0, 0.2);
	} else {
		if(trajectory_is_viable) cairo_set_source_rgb(cr, 0, 1, 0);
		else cairo_set_source_rgb(cr, 1, 0, 0);
	}
}

void draw_itinerary(Camera *camera, CelestSystem *system, struct ItinStep *tf, double current_time) {
	if(tf == NULL) return;

	// draw trajectories
	tf = get_first(tf);
	while(tf->next != NULL) {
		OSV tf_osv0 = {tf->r, tf->next[0]->v_dep};
		int trajectory_is_viable = 1;
		double dt = tf->next[0]->date - tf->date;

		if(tf->prev != NULL) {
			trajectory_is_viable = is_flyby_viable(tf->v_arr, tf->next[0]->v_dep, tf->v_body, tf->body, 10);
		}

		if(current_time >= tf->date && current_time < tf->next[0]->date) {
			set_trajectory_color(get_camera_screen_cairo(camera), 1, trajectory_is_viable);
			dt = current_time - tf->date;
			draw_trajectory(camera, tf_osv0, dt, system->cb);

			OSV current_osv = dt != 0 ? propagate_osv_time(tf_osv0, system->cb, dt*86400) : tf_osv0;

			set_trajectory_color(get_camera_screen_cairo(camera), 0, trajectory_is_viable);
			dt = tf->next[0]->date - current_time;
			draw_trajectory(camera, current_osv, dt, system->cb);

			draw_itinerary_spacecraft(camera, current_osv.r);
		} else {
			set_trajectory_color(get_camera_screen_cairo(camera), current_time > tf->date, trajectory_is_viable);
			draw_trajectory(camera, tf_osv0, dt, system->cb);
		}
		tf = tf->next[0];
	}

	if(current_time <= get_first(tf)->date) {
		OSV osv_body = system->prop_method == ORB_ELEMENTS ?
				   osv_from_elements(get_first(tf)->body->orbit, current_time) :
				   osv_from_ephem(get_first(tf)->body->ephem, get_first(tf)->body->num_ephems, current_time, system->cb);
		draw_itinerary_spacecraft(camera, osv_body.r);
	} else if(current_time >= tf->date) {
		OSV osv_body = system->prop_method == ORB_ELEMENTS ?
							  osv_from_elements(tf->body->orbit, current_time) :
							  osv_from_ephem(tf->body->ephem, tf->body->num_ephems, current_time, system->cb);
		draw_itinerary_spacecraft(camera, osv_body.r);
	}

	// draw itin step points
	while(tf != NULL) {
		draw_itinerary_step_point(camera, tf->r);
		tf = tf->prev;
	}
}

void draw_orbit_2d(cairo_t *cr, Vector2 center, double scale, Vector3 r, Vector3 v, struct Body *attractor) {
	int steps = 100;
	struct OSV last_osv = {r,v};
	
	for(int i = 1; i <= steps; i++) {
		double ta = 2*M_PI/steps * i;
		struct OSV osv = propagate_osv_ta((OSV){r,v},attractor,ta);
		// y negative, because in GUI y gets bigger downwards
		Vector2 p1 = {last_osv.r.x, -last_osv.r.y};
		Vector2 p2 = {osv.r.x, -osv.r.y};
		
		p1 = scale_vec2(p1,scale);
		p2 = scale_vec2(p2,scale);
		
		draw_stroke(cr, add_vec2(p1,center), add_vec2(p2,center));
		last_osv = osv;
	}
}

void draw_body_2d(cairo_t *cr, Vector2 center, double scale, Vector3 r) {
	int body_radius = 5;
	r = scale_vec3(r, scale);
	// y negative, because in GUI y gets bigger downwards
	r.y *= -1;
	cairo_arc(cr, center.x+r.x, center.y+r.y,body_radius,0,M_PI*2);
	cairo_fill(cr);
}

void draw_transfer_point_2d(cairo_t *cr, Vector2 center, double scale, Vector3 r) {
	int cross_length = 4;
	cairo_set_source_rgb(cr, 1, 0, 0);
	r = scale_vec3(r, scale);
	// y negative, because in GUI y gets bigger downwards
	r.y *= -1;
	for(int i = -1; i<=1; i+=2) {
		Vector2 p1 = {r.x - cross_length, r.y + cross_length*i};
		Vector2 p2 = {r.x + cross_length, r.y - cross_length*i};
		draw_stroke(cr, add_vec2(center, p1), add_vec2(center, p2));
	}
}




// Rework trajectory drawing -> OSV + dt   ----------------------------------------------
void draw_trajectory_2d(cairo_t *cr, Vector2 center, double scale, struct ItinStep *tf, struct Body *attractor) {
	// if double swing-by is not worth drawing
	if(tf->body == NULL && tf->v_body.x == 0) return;

	cairo_set_source_rgb(cr, 0, 1, 0);
	struct ItinStep *prev = tf->prev;
	// skip not working double swing-by
	if(tf->prev->body == NULL && tf->prev->v_body.x == 0) prev = tf->prev->prev;
	double dt = (tf->date-prev->date)*24*60*60;

	if(prev->prev != NULL && prev->body != NULL) {
		if(!is_flyby_viable(tf->prev->v_arr, tf->v_dep, tf->prev->v_body, tf->body, 10)) cairo_set_source_rgb(cr, 1, 0, 0);
	}

	int steps = 1000;
	Vector3 r = prev->r;
	Vector3 v = tf->v_dep;
	struct OSV last_osv = {r,v};

	for(int i = 1; i <= steps; i++) {
		double time = dt/steps * i;
		struct OSV osv = propagate_osv_time((OSV){r,v},attractor,time);
		// y negative, because in GUI y gets bigger downwards
		Vector2 p1 = {last_osv.r.x, -last_osv.r.y};
		Vector2 p2 = {osv.r.x, -osv.r.y};
		
		p1 = scale_vec2(p1,scale);
		p2 = scale_vec2(p2,scale);
		
		draw_stroke(cr, add_vec2(p1,center), add_vec2(p2,center));
		last_osv = osv;
	}
}


void draw_stroke(cairo_t *cr, Vector2 p1, Vector2 p2) {
	cairo_set_line_width(cr, 1);
	cairo_move_to(cr, p1.x, p1.y);
	cairo_line_to(cr, p2.x, p2.y);
	cairo_stroke(cr);
}

double calc_scale(int area_width, int area_height, struct Body *farthest_body) {
	if(farthest_body == NULL) return 1e-9;
	double apoapsis = calc_orbit_apoapsis(farthest_body->orbit);
	int wh = area_width < area_height ? area_width : area_height;
	return 1/apoapsis*wh/2.2;	// divided by 2.2 because apoapsis is only one side and buffer
}

void set_cairo_body_color(cairo_t *cr, struct Body *body) {
	if(body != NULL) {
		cairo_set_source_rgb(cr, body->color[0], body->color[1], body->color[2]);
	} else cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
}

void draw_center_aligned_text(cairo_t *cr, double x, double y, char *text) {
	// Calculate text width
	cairo_text_extents_t extents;
	cairo_text_extents(cr, text, &extents);
	double text_width = extents.width;

	// Position and draw text to the left
	cairo_move_to(cr, x - text_width/2, y); // adjust starting position to the left
	cairo_show_text(cr, text);
}

void draw_right_aligned_text(cairo_t *cr, double x, double y, double rotate_angle, char *text) {
	// Calculate text width
	cairo_text_extents_t extents;
	cairo_text_extents(cr, text, &extents);
	double text_width = extents.width;

	// Position and draw text to the left
	cairo_move_to(cr, x - text_width*cos(rotate_angle), y+sin(rotate_angle)*(text_width-5)); // adjust starting position to the left
	cairo_rotate(cr, -rotate_angle);
	cairo_show_text(cr, text);
	cairo_rotate(cr, rotate_angle);
}

void get_axis_ticks(enum CoordAxisLabelType axis_label_type, int num_labels, double min_value, double max_value, double *p_value_tick, Datetime *p_date_tick, double *p_min_label) {
	if(axis_label_type == COORD_LABEL_NUMBER || axis_label_type == COORD_LABEL_DURATION) {
		double tick_units[] = {1,2,5};
		double tick = tick_units[0];
		double tick_scale = 1e-9;
		do {
			for(int i = 0; i < sizeof(tick_units)/sizeof(tick_units[0]); i++) {
				tick = tick_units[i] * tick_scale;
				if(num_labels*tick > (max_value - min_value)) break;
			}
			tick_scale *= 10;
		} while(num_labels*tick < (max_value - min_value));
		*p_value_tick = tick;
		*p_min_label = (min_value == 0) ? tick : ceil(min_value/tick)*tick;
	} else if(axis_label_type == COORD_LABEL_DATE) {
		Datetime tick = {0, 0, 1};
		double min_label = 0;
		for(int i = 0; i < 3; i++) {
			if(get_settings_datetime_type() != DATE_ISO && i == 1) continue; // kerbal date doesn't have months
			tick = i == 0 ? (Datetime){0, 0, 1} : i == 1 ? (Datetime) {0, 1, 0} : (Datetime) {1, 0, 0};
			int tick_scale = 0;
			bool found_valid_tick = false;
			do {
				switch(tick_scale) {
					case 0: tick_scale = 1; break;
					case 1: tick_scale = 2; break;
					case 2: tick_scale = i!=1 ? 5 : 6; break;
					case 5: tick_scale = 10; break;
					case 10: tick_scale = 20; break;
					case 20: tick_scale = 50; break;
					case 50: tick_scale = 100; break;
					default: tick_scale *= 2;
				}
				tick = (Datetime) {(tick.y != 0)*tick_scale, (tick.m != 0)*tick_scale, (tick.d != 0)*tick_scale};
				Datetime min_date = convert_JD_date(min_value, get_settings_datetime_type());
				Datetime min_x_date_label = (Datetime) {
						min_date.y,
						tick.y == 0 && tick.m <6 ? min_date.m : 1,
										(tick.y == 0 && tick.m == 0) ? min_date.d : 1,
								.date_type = get_settings_datetime_type()
				};
				min_label = convert_date_JD(min_x_date_label);
				min_label = jd_change_date(
						min_label,
						tick.y,
						tick.m,
						tick.d,
						get_settings_datetime_type()
				);
				double max_label = jd_change_date(
						min_label,
						tick.y*num_labels,
						tick.m*num_labels,
						tick.d*num_labels,
						get_settings_datetime_type()
				);
				if(max_label > max_value) { found_valid_tick = true; break;}
			} while((tick.d < 10 || get_settings_datetime_type() != DATE_ISO) && tick.d < 100 && tick.m < 6);
			if(found_valid_tick) break;
		}
		*p_date_tick = tick;
		*p_min_label = min_label;
	}
}

void draw_coordinate_system(cairo_t *cr, double width, double height, enum CoordAxisLabelType x_axis_label_type, enum CoordAxisLabelType y_axis_label_type,
		double min_x, double max_x, double min_y, double max_y, Vector2 origin, int num_x_labels, int num_y_labels) {
	// Set text color
	cairo_set_source_rgb(cr, 1, 1, 1);
	// Set font options
	cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 12.0);
	int half_font_size = 5;

	// axes
	draw_stroke(cr, vec2(origin.x, 0), vec2(origin.x, origin.y));
	draw_stroke(cr, vec2(origin.x, origin.y), vec2(width, origin.y));

	double y_label_tick, x_label_tick;
	double min_x_label, min_y_label;
	Datetime x_date_label_tick, y_date_label_tick;
	
	if(x_axis_label_type != COORD_LABEL_DATE) {
		get_axis_ticks(x_axis_label_type, num_x_labels, min_x, max_x, &x_label_tick, NULL, &min_x_label);
	} else {
		get_axis_ticks(x_axis_label_type, num_x_labels, min_x, max_x, NULL, &x_date_label_tick, &min_x_label);
	}
	
	if(y_axis_label_type != COORD_LABEL_DATE) {
		get_axis_ticks(y_axis_label_type, num_y_labels, min_y, max_y, &y_label_tick, NULL, &min_y_label);
	} else {
		get_axis_ticks(y_axis_label_type, num_y_labels, min_y, max_y, NULL, &y_date_label_tick, &min_y_label);
	}

	// gradients
	double m_y, m_x;
	m_x = (width-origin.x)/(max_x - min_x);
	m_y = -origin.y/(max_y - min_y); // negative, because positive is down

	double y_label_x = origin.x-5,	x_label_y = origin.y + 20;

	// x-labels and x grid
	char string[32];
	for(int i = 0; i < num_x_labels; i++) {
		double label = x_axis_label_type != COORD_LABEL_DATE ?
				min_x_label + i * x_label_tick :
				jd_change_date(min_x_label, i*x_date_label_tick.y, i*x_date_label_tick.m, i*x_date_label_tick.d, get_settings_datetime_type());
		if(label < min_x) continue;
		double x = (label-min_x)*m_x + origin.x;
		cairo_set_source_rgb(cr, 1, 1, 1);
		if(x_axis_label_type == COORD_LABEL_NUMBER)
			sprintf(string, "%g", label);
		else if(x_axis_label_type == COORD_LABEL_DURATION)
			sprintf(string, "%g", get_settings_datetime_type() == DATE_KERBAL ? label*4 : label);
		else if(x_axis_label_type == COORD_LABEL_DATE)
			date_to_string(convert_JD_date(label, get_settings_datetime_type()), string, 0);
		draw_center_aligned_text(cr, x, x_label_y, string);
		cairo_set_source_rgb(cr, 0, 0, 0);
		draw_stroke(cr, vec2(x, origin.y), vec2(x, 0));
	}

	// y-labels and y grid
	for(int i = 0; i < num_y_labels; i++) {
		double label = y_axis_label_type != COORD_LABEL_DATE ?
					   min_y_label + i * y_label_tick :
					   jd_change_date(min_y_label, i*y_date_label_tick.y, i*y_date_label_tick.m, i*y_date_label_tick.d, get_settings_datetime_type());
		if(label < min_y) continue;
		double y = (label-min_y)*m_y + origin.y;
		cairo_set_source_rgb(cr, 1, 1, 1);
		if(y_axis_label_type == COORD_LABEL_NUMBER)
			sprintf(string, "%g", label);
		else if(y_axis_label_type == COORD_LABEL_DURATION)
			sprintf(string, "%g", get_settings_datetime_type() == DATE_KERBAL ? label*4 : label);
		else if(y_axis_label_type == COORD_LABEL_DATE)
			date_to_string(convert_JD_date(label, get_settings_datetime_type()), string, 0);
		draw_right_aligned_text(cr, y_label_x, y+half_font_size, M_PI/4, string);
		cairo_set_source_rgb(cr, 0, 0, 0);
		draw_stroke(cr, vec2(origin.x, y), vec2(width, y));
	}
}

void draw_data_point(cairo_t *cr, double x, double y, double radius) {
	if(radius > 3) {
		cairo_arc(cr, x, y, radius,0,M_PI*2);
		cairo_fill(cr);
	} else {
		cairo_rectangle(cr, x-radius, y-radius, radius, radius);
		cairo_fill(cr);
	}
}

const int porkchop_dur_yaxis_x = 40;
const int porkchop_arrdate_yaxis_x = 60;
const int porkchop_xaxis_y = 40;

int get_porkchop_dur_yaxis_x() {return porkchop_dur_yaxis_x;}
int get_porkchop_arrdate_yaxis_x() {return porkchop_arrdate_yaxis_x;}
int get_porkchop_xaxis_y() {return porkchop_xaxis_y;}

void draw_porkchop(cairo_t *cr, double width, double height, struct PorkchopAnalyzerPoint *porkchop, int num_itins, enum LastTransferType last_transfer_type, int dur0arrdate1) {
	double dv, depdate, dur, arrdate;

	Vector2 origin = {dur0arrdate1 ? porkchop_arrdate_yaxis_x : porkchop_dur_yaxis_x, height-porkchop_xaxis_y};

	int first_show_ind = 0;
	while(!porkchop[first_show_ind].inside_filter) first_show_ind++;

	struct PorkchopPoint pp = porkchop[first_show_ind].data;
	double dv_sat = pp.dv_dsm;
	if(last_transfer_type == TF_CAPTURE)	dv_sat += pp.dv_arr_cap;
	if(last_transfer_type == TF_CIRC)		dv_sat += pp.dv_arr_circ;

	double min_depdate = pp.dep_date, max_depdate = pp.dep_date;
	double min_dur = pp.dur, max_dur = pp.dur;
	double min_arrdate = pp.dep_date+pp.dur, max_arrdate = pp.dep_date+pp.dur;
	double min_dv = pp.dv_dep + dv_sat;
	double max_dv = pp.dv_dep + dv_sat;

	// find min and max
	for(int i = 1; i < num_itins; i++) {
		if(!porkchop[i].inside_filter) continue;
		pp = porkchop[i].data;

		dv_sat = pp.dv_dsm;
		if(last_transfer_type == TF_CAPTURE)	dv_sat += pp.dv_arr_cap;
		if(last_transfer_type == TF_CIRC)		dv_sat += pp.dv_arr_circ;

		dv = pp.dv_dep + dv_sat;
		depdate = pp.dep_date;
		dur = pp.dur;
		arrdate = pp.dep_date+pp.dur;
		

		if(dv < min_dv) min_dv = dv;
		else if(dv > max_dv) max_dv = dv;
		if(depdate < min_depdate) min_depdate = depdate;
		else if(depdate > max_depdate) max_depdate = depdate;
		if(dur < min_dur) min_dur = dur;
		else if(dur > max_dur) max_dur = dur;
		if(arrdate < min_arrdate) min_arrdate = arrdate;
		else if(arrdate > max_arrdate) max_arrdate = arrdate;
	}


	double ddepdate = max_depdate - min_depdate;
	double ddur = max_dur - min_dur;
	double darrdate = max_arrdate - min_arrdate;

	double margin = 0.05;

	min_depdate = min_depdate - ddepdate*margin;
	max_depdate = max_depdate + ddepdate*margin;
	min_dur = min_dur - ddur * margin;
	max_dur = max_dur + ddur * margin;
	min_arrdate = min_arrdate - darrdate * margin;
	max_arrdate = max_arrdate + darrdate * margin;

	// gradients
	double m_dur, m_depdate, m_arrdate;
	m_depdate = (width - origin.x)/(max_depdate - min_depdate);
	m_arrdate = -origin.y/(max_arrdate - min_arrdate); // negative, because positive is down
	m_dur = -origin.y/(max_dur - min_dur); // negative, because positive is down

	if(dur0arrdate1) draw_coordinate_system(cr, width, height, COORD_LABEL_DATE, COORD_LABEL_DATE, min_depdate, max_depdate, min_arrdate, max_arrdate, origin, 8, 10);
	else draw_coordinate_system(cr, width, height, COORD_LABEL_DATE, COORD_LABEL_DURATION, min_depdate, max_depdate, min_dur, max_dur, origin, 8, 10);

	// find points to draw
	int *draw_idx = calloc(sizeof(int), num_itins);
	int num_draw_itins = 0;
	for(int i = 0; i < num_itins; i++) {
		if(porkchop[i].inside_filter && porkchop[i].group->show_group) {
			draw_idx[num_draw_itins] = i;
			num_draw_itins++;
		}
	}

	// Create an off-screen surface to draw porkchop all at onece
	cairo_surface_t* buffer_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int) width, (int) height);
	cairo_t* buffer_cr = cairo_create(buffer_surface);

	// draw points
	double color_bias;
	int i = num_draw_itins-1;
	while(i >= 0) {
		pp = porkchop[draw_idx[i]].data;

		dv_sat = pp.dv_dsm;
		if(last_transfer_type == TF_CAPTURE)	dv_sat += pp.dv_arr_cap;
		if(last_transfer_type == TF_CIRC)		dv_sat += pp.dv_arr_circ;

		dv = pp.dv_dep + dv_sat;
		depdate = pp.dep_date;
		dur = pp.dur;
		arrdate = pp.dep_date+pp.dur;

		// color coding
		color_bias = (dv - min_dv) / (max_dv - min_dv);
		double r = i == 0 ? 1 : color_bias;
		double g = i == 0 ? 0 : 1-color_bias;
		double b = i == 0 ? 0 : 4*pow(color_bias-0.5,2);
		cairo_set_source_rgb(buffer_cr, r,g,b);

		Vector2 data_point = vec2(origin.x + m_depdate*(depdate - min_depdate), origin.y + (dur0arrdate1 ? (m_arrdate*(arrdate - min_arrdate)) : (m_dur*(dur - min_dur))));
		int radius = i > 0 ? 2 : 5;
		if(num_draw_itins < 10000) radius += 2;
		draw_data_point(buffer_cr, data_point.x, data_point.y, radius);

		i--;
	}

	// Copy the buffer to the main canvas and buffer cleanup
	cairo_set_source_surface(cr, buffer_surface, 0, 0);
	cairo_paint(cr);
	cairo_destroy(buffer_cr);
	cairo_surface_destroy(buffer_surface);

	free(draw_idx);
}

void draw_plot(cairo_t *cr, double width, double height, double *x, double *y, int num_points) {
	Vector2 origin = {60, height-30};

	double min_x = x[0], max_x = x[0];
	double min_y = y[0], max_y = y[0];
	if(min_y > 0) min_y = 0;

	// find min and max
	for(int i = 1; i < num_points; i++) {
		if(x[i] < min_x) min_x = x[i];
		else if(x[i] > max_x) max_x = x[i];
		if(y[i] < min_y) min_y = y[i];
		else if(y[i] > max_y) max_y = y[i];
	}

	double dx = max_x-min_x;
	double dy = max_y-min_y;


	double margin = 0.05;

	//min_x = min_x != 0 ? min_x - dx * margin : 0;
	//max_x = max_x != 0 ? max_x + dx * margin : 0;
	//dx = max_x-min_x;
	min_y = min_y != 0 ? min_y - dy * margin : 0;
	max_y = max_y != 0 ? max_y + dy * margin : 0;
	dy = max_y-min_y;

	// gradients
	double m_x, m_y;
	m_x = (width-origin.x)/dx;
	m_y = -origin.y/dy; // negative, because positive is down

	draw_coordinate_system(cr, width, height, COORD_LABEL_NUMBER, COORD_LABEL_NUMBER, min_x, max_x, min_y, max_y, origin, 10, 15);

	// data
	cairo_set_source_rgb(cr, 0, 0.8, 0.8);
	for(int i = 1; i < num_points; i++) {
		Vector2 point0 = vec2(origin.x + m_x*(x[i-1] - min_x), origin.y + m_y * (y[i-1] - min_y));
		Vector2 point1 = vec2(origin.x + m_x*(x[i  ] - min_x), origin.y + m_y * (y[i  ] - min_y));
		draw_stroke(cr, point0, point1);
	}
}

void draw_multi_plot(cairo_t *cr, double width, double height, double *x, double **y, int num_plots, int num_points) {
	Vector2 origin = {60, height-30};

	Vector3 colors[2] = {
			vec3(0, 0.8, 0.8),
			vec3(0.8, 0, 0.4),
	};

	double min_x = x[0], max_x = x[0];
	double min_y = y[0][0], max_y = y[0][0];
	if(min_y > 0) min_y = 0;

	// find min and max
	for(int i = 1; i < num_points; i++) {
		if(x[i] < min_x) min_x = x[i];
		else if(x[i] > max_x) max_x = x[i];
	}

	for(int p = 0; p < num_plots; p++) {
		for(int i = 0; i < num_points; i++) {
			if(y[p][i] < min_y) min_y = y[p][i];
			else if(y[p][i] > max_y) max_y = y[p][i];
		}
	}

	double dx = max_x-min_x;
	double dy = max_y-min_y;


	double margin = 0.05;

	//min_x = min_x != 0 ? min_x - dx * margin : 0;
	//max_x = max_x != 0 ? max_x + dx * margin : 0;
	//dx = max_x-min_x;
	min_y = min_y != 0 ? min_y - dy * margin : 0;
	max_y = max_y != 0 ? max_y + dy * margin : 0;
	dy = max_y-min_y;

	// gradients
	double m_x, m_y;
	m_x = (width-origin.x)/dx;
	m_y = -origin.y/dy; // negative, because positive is down

	draw_coordinate_system(cr, width, height, COORD_LABEL_NUMBER, COORD_LABEL_NUMBER, min_x, max_x, min_y, max_y, origin, 10, 15);

	// data
	for(int p = 0; p < num_plots; p++) {
		cairo_set_source_rgb(cr, colors[p].x, colors[p].y, colors[p].z);
		for(int i = 1; i < num_points; i++) {
			Vector2 point0 = vec2(origin.x + m_x * (x[i - 1] - min_x), origin.y + m_y * (y[p][i - 1] - min_y));
			Vector2 point1 = vec2(origin.x + m_x * (x[i] - min_x), origin.y + m_y * (y[p][i] - min_y));
			draw_stroke(cr, point0, point1);
		}
	}
}
