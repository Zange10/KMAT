#include <stdio.h>

#include "drawing.h"
#include "math.h"
#include <stdlib.h>


void draw_orbit(cairo_t *cr, struct Vector2D center, double scale, struct Vector r, struct Vector v, struct Body *attractor) {
	int steps = 100;
	struct OSV last_osv = {r,v};
	for(int i = 1; i <= steps; i++) {
		double theta = 2*M_PI/steps * i;
		struct OSV osv = propagate_orbit_theta(r,v,theta,attractor);
		// y negative, because in GUI y gets bigger downwards
		struct Vector2D p1 = {last_osv.r.x, -last_osv.r.y};
		struct Vector2D p2 = {osv.r.x, -osv.r.y};
		
		p1 = scalar_multipl2d(p1,scale);
		p2 = scalar_multipl2d(p2,scale);
		
		draw_stroke(cr, add_vectors2d(p1,center), add_vectors2d(p2,center));
		last_osv = osv;
	}
}

void draw_body(cairo_t *cr, struct Vector2D center, double scale, struct Vector r) {
	int body_radius = 5;
	r = scalar_multiply(r, scale);
	// y negative, because in GUI y gets bigger downwards
	r.y *= -1;
	cairo_arc(cr, center.x+r.x, center.y+r.y,body_radius,0,M_PI*2);
	cairo_fill(cr);
}

void draw_transfer_point(cairo_t *cr, struct Vector2D center, double scale, struct Vector r) {
	int cross_length = 4;
	cairo_set_source_rgb(cr, 1, 0, 0);
	r = scalar_multiply(r, scale);
	// y negative, because in GUI y gets bigger downwards
	r.y *= -1;
	for(int i = -1; i<=1; i+=2) {
		struct Vector2D p1 = {r.x - cross_length, r.y + cross_length*i};
		struct Vector2D p2 = {r.x + cross_length, r.y - cross_length*i};
		draw_stroke(cr, add_vectors2d(center, p1), add_vectors2d(center, p2));
	}
}




// Rework trajectory drawing -> OSV + dt   ----------------------------------------------
void draw_trajectory(cairo_t *cr, struct Vector2D center, double scale, struct ItinStep *tf) {
	// if double swing-by is not worth drawing
	if(tf->body == NULL && tf->v_body.x == 0) return;

	cairo_set_source_rgb(cr, 0, 1, 0);
	struct ItinStep *prev = tf->prev;
	// skip not working double swing-by
	if(tf->prev->body == NULL && tf->prev->v_body.x == 0) prev = tf->prev->prev;
	double dt = (tf->date-prev->date)*24*60*60;

	if(prev->prev != NULL && prev->body != NULL) {
		double t[3] = {prev->prev->date, prev->date, tf->date};
		struct OSV osv0 = {prev->prev->r, prev->prev->v_body};
		struct OSV osv1 = {prev->r, prev->v_body};
		struct OSV osv2 = {tf->r, tf->v_body};
		struct OSV osvs[3] = {osv0, osv1, osv2};
		struct Body *bodies[3] = {prev->prev->body, prev->body, tf->body};
		if(!is_flyby_viable(t, osvs, bodies)) cairo_set_source_rgb(cr, 1, 0, 0);
	}

	int steps = 1000;
	struct Vector r = prev->r;
	struct Vector v = tf->v_dep;
	struct OSV last_osv = {r,v};
	for(int i = 1; i <= steps; i++) {
		double time = dt/steps * i;
		struct OSV osv = propagate_orbit_time(r,v,time,SUN());
		// y negative, because in GUI y gets bigger downwards
		struct Vector2D p1 = {last_osv.r.x, -last_osv.r.y};
		struct Vector2D p2 = {osv.r.x, -osv.r.y};
		
		p1 = scalar_multipl2d(p1,scale);
		p2 = scalar_multipl2d(p2,scale);
		
		draw_stroke(cr, add_vectors2d(p1,center), add_vectors2d(p2,center));
		last_osv = osv;
	}
}


void draw_stroke(cairo_t *cr, struct Vector2D p1, struct Vector2D p2) {
	cairo_set_line_width(cr, 1);
	cairo_move_to(cr, p1.x, p1.y);
	cairo_line_to(cr, p2.x, p2.y);
	cairo_stroke(cr);
}

double calc_scale(int area_width, int area_height, int highest_id) {
	if(highest_id == 0) return 1e-9;
	struct Body *body = get_body_from_id(highest_id);
	double apoapsis = body->orbit.apoapsis;
	int wh = area_width < area_height ? area_width : area_height;
	return 1/apoapsis*wh/2.2;	// divided by 2.2 because apoapsis is only one side and buffer
}

void set_cairo_body_color(cairo_t *cr, int id) {
	switch(id) {
		case 0: cairo_set_source_rgb(cr, 1.0, 1.0, 0.3); break;	// Sun
		case 1: cairo_set_source_rgb(cr, 0.3, 0.3, 0.3); break;	// Mercury
		case 2: cairo_set_source_rgb(cr, 0.6, 0.6, 0.2); break;	// Venus
		case 3: cairo_set_source_rgb(cr, 0.2, 0.2, 1.0); break;	// Earth
		case 4: cairo_set_source_rgb(cr, 1.0, 0.2, 0.0); break;	// Mars
		case 5: cairo_set_source_rgb(cr, 0.6, 0.4, 0.2); break;	// Jupiter
		case 6: cairo_set_source_rgb(cr, 0.8, 0.8, 0.6); break;	// Saturn
		case 7: cairo_set_source_rgb(cr, 0.2, 0.6, 1.0); break;	// Uranus
		case 8: cairo_set_source_rgb(cr, 0.0, 0.0, 1.0); break;	// Neptune
		case 9: cairo_set_source_rgb(cr, 0.7, 0.7, 0.7); break;	// Pluto
		default:cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); break;
	}
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

void draw_right_aligned_text(cairo_t *cr, double x, double y, char *text) {
	// Calculate text width
	cairo_text_extents_t extents;
	cairo_text_extents(cr, text, &extents);
	double text_width = extents.width;

	// Position and draw text to the left
	cairo_move_to(cr, x - text_width, y); // adjust starting position to the left
	cairo_show_text(cr, text);
}

void draw_right_aligned_int(cairo_t *cr, double x, double y, int number) {
	char text[16];
	sprintf(text, "%d", number);
	draw_right_aligned_text(cr, x, y, text);
}

void draw_data_point(cairo_t *cr, double x, double y, double radius) {
	cairo_arc(cr, x, y, radius,0,M_PI*2);
	cairo_fill(cr);
}

void draw_porkchop(cairo_t *cr, double width, double height, const double *porkchop, int fb0_pow1) {
	// Set text color
	cairo_set_source_rgb(cr, 1, 1, 1);

	// coordinate system
	struct Vector2D origin = {45, height-40};
	draw_stroke(cr, vec2D(origin.x, 0), vec2D(origin.x, origin.y));
	draw_stroke(cr, vec2D(origin.x, origin.y), vec2D(width, origin.y));

	if(porkchop == NULL) return;

	// Set font options
	cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 12.0);
	int half_font_size = 5;


	int num_itins = (int) (porkchop[0]/5);


	double min_duration = porkchop[1+1], max_duration = porkchop[1+1];
	double min_date = porkchop[0+1], max_date = porkchop[0+1];
	double min_dv = porkchop[2+1]+porkchop[3+1]*fb0_pow1+porkchop[4+1];
	double max_dv = porkchop[2+1]+porkchop[3+1]*fb0_pow1+porkchop[4+1];
	int min_dv_ind = 0;
	double dv, date, dur;

	for(int i = 1; i < num_itins; i++) {
		int index = 1+i*5;
		dv = porkchop[index+2]+porkchop[index+3]*fb0_pow1+porkchop[index+4];
		date = porkchop[index+0];
		dur = porkchop[index+1];

		if(dv < min_dv) {
			min_dv = dv;
			min_dv_ind = i;
		}
		else if(dv > max_dv) max_dv = dv;
		if(date < min_date) min_date = date;
		else if(date > max_date) max_date = date;
		if(dur < min_duration) min_duration = dur;
		else if(dur > max_duration) max_duration = dur;
	}

	int min_x = (int) min_date;
	int min_y = (int) min_duration;


	double y_step = 1, x_step = 1;
	int num_durs = 8;
	int num_dates = 8;

	while(min_y + (num_durs - 1) * y_step < max_duration) y_step *= 2;
	while(min_x + (num_dates - 1) * x_step < max_date) x_step *= 2;

	double y_label_x = 40,	x_label_y = height - 15;

	double min_y_label_y = 20, max_y_label_y = origin.y-20;
	double y_label_step = (double)(max_y_label_y-min_y_label_y+1)/(num_durs-1);
	double y_gradient = -(y_label_step*(num_durs-1)) / (y_step*(num_durs-1));

	double min_x_label_x = origin.x+20, max_x_label_x = width-40;
	double x_label_step = (double)(max_x_label_x-min_x_label_x+1)/(num_dates-1);
	double x_gradient = (x_label_step*(num_dates-1)) / (x_step*(num_dates-1));

	// y-labels and y grid
	for(int i = 0; i < num_durs; i++) {
		double y = max_y_label_y-y_label_step*i;
		if(i > 0) draw_right_aligned_int(cr, y_label_x, y+half_font_size, min_y + i * y_step);
		draw_stroke(cr, vec2D(origin.x, y), vec2D(width, y));
	}

	// x-labels and x grid
	char date_string[32];
	for(int i = 0; i < num_dates; i++) {
		double x = min_x_label_x+x_label_step*i;
		date_to_string(convert_JD_date(min_x + i * x_step), date_string, 0);
		if(i > 0) draw_center_aligned_text(cr, x, x_label_y, date_string);
		draw_stroke(cr, vec2D(x, origin.y), vec2D(x, 0));
	}


	// data
	double color_bias;
	for(int i = 0; i <= num_itins; i++) {
		int index = i < num_itins ? 1+i*5 : 1+min_dv_ind*5;

		dv = porkchop[index+2]+porkchop[index+3]*fb0_pow1+porkchop[index+4];
		date = porkchop[index+0];
		dur = porkchop[index+1];

		// color coding
		color_bias = (dv - min_dv) / (max_dv - min_dv);
		double r = i == num_itins ? 1 : color_bias;
		double g = i == num_itins ? 0 : 1-color_bias;
		double b = i == num_itins ? 0 : 4*pow(color_bias-0.5,2);
		cairo_set_source_rgb(cr, r,g,b);

		struct Vector2D data_point = vec2D(min_x_label_x + x_gradient*(date - min_x), max_y_label_y + y_gradient * (dur - min_y));
		draw_data_point(cr, data_point.x, data_point.y, i < num_itins ? 2 : 5);
	}
}
