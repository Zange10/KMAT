//
// Created by niklas on 08.01.24.
//
#include <stdlib.h>
#include <stdio.h>

#include "drawing.h"
#include "math.h"
#include "orbit_calculator/double_swing_by.h"


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

//void draw_dsb(cairo_t *cr, struct Vector2D center, double scale, struct ItinStep *tf0, struct ItinStep *tf1, struct Ephem **ephems) {
//	if(tf1->next == NULL) {
//		draw_trajectory(cr, center, scale, tf0, tf1, ephems);
//		return;
//	}
//
//	cairo_set_source_rgb(cr, 0, 1, 0);
//
//	struct Body *bodies[] = {tf0->prev->body, tf0->body, tf1->next[0]->body};
//
//	double jd_dep = tf0->prev->date;
//	double jd_sb1 = tf0->date;
//	double jd_sb2 = tf1->date;
//	double jd_arr = tf1->next[0]->date;
//
//	struct OSV osv_dep = osv_from_ephem(ephems[bodies[0]->id-1], jd_dep, SUN());
//	struct OSV osv_sb1 = osv_from_ephem(ephems[bodies[1]->id-1], jd_sb1, SUN());
//	struct OSV osv_sb2 = osv_from_ephem(ephems[bodies[1]->id-1], jd_sb2, SUN());
//	struct OSV osv_arr = osv_from_ephem(ephems[bodies[2]->id-1], jd_arr, SUN());
//
//	struct Transfer transfer_dep = calc_transfer(circfb, EARTH(), VENUS(), osv_dep.r, osv_dep.v, osv_sb1.r, osv_sb1.v, (jd_sb1-jd_dep)*86400, NULL);
//	struct Transfer transfer_arr = calc_transfer(circfb, VENUS(), EARTH(), osv_sb2.r, osv_sb2.v, osv_arr.r, osv_arr.v, (jd_arr-jd_sb2)*86400, NULL);
//
//	struct OSV s0 = {transfer_dep.r1, transfer_dep.v1};
//	struct OSV s1 = {transfer_arr.r0, transfer_arr.v0};
//
//	struct DSB dsb = calc_double_swing_by(s0, osv_sb1, s1, osv_sb2, jd_sb2-jd_sb1, bodies[1]);
//
//	print_vector(scalar_multiply(dsb.osv[0].r,1e-9));
//	print_vector(scalar_multiply(dsb.osv[1].r,1e-9));
//	print_vector(scalar_multiply(dsb.osv[2].r,1e-9));
//	print_vector(scalar_multiply(dsb.osv[3].r,1e-9));
//
//	tf0->next[0]->date = tf0->date + dsb.man_time/86400;
//	draw_transfer_point(cr, center, scale, dsb.osv[1].r);
//	cairo_set_source_rgb(cr, 0, 1, 0);
//
//	print_date(convert_JD_date(jd_sb1),1);
//	print_date(convert_JD_date(tf0->date),1);
//	print_date(convert_JD_date(tf0->next[0]->date),1);
//	print_date(convert_JD_date(jd_sb2),1);
//	print_date(convert_JD_date(tf1->date),1);
//	double times[3] = {jd_sb1, tf0->next[0]->date, jd_sb2};
//
//	for(int c = 0; c < 2; c++) {
//		double dt = (times[c+1]-times[c])*86400;
//
//		if(c == 0) {
//			double t[3] = {tf0->prev->date, tf0->date, tf0->next[0]->date};
//			struct OSV osv_prev = osv_from_ephem(ephems[tf0->prev->body->id - 1], tf0->prev->date, SUN());
//			struct OSV osvs[3] = {osv_prev, osv_sb1, dsb.osv[2]};
//			struct Body *temp_bodies[3] = {tf0->prev->body, tf0->body, tf1->body};
//			if(!is_flyby_viable(t, osvs, temp_bodies)) cairo_set_source_rgb(cr, 1, 0, 0);
//		}
//
//		int steps = 1000;
//		struct Vector r = dsb.osv[c*2].r;
//		struct Vector v = dsb.osv[c*2].v;
//		struct OSV last_osv = {r, v};
//		printf("%f\n", dt/86400);
//		for(int i = 1; i <= steps; i++) {
//			double time = dt / steps * i;
//			struct OSV osv = propagate_orbit_time(r, v, time, SUN());
//			// y negative, because in GUI y gets bigger downwards
//			struct Vector2D p1 = {last_osv.r.x, -last_osv.r.y};
//			struct Vector2D p2 = {osv.r.x, -osv.r.y};
//
//			p1 = scalar_multipl2d(p1, scale);
//			p2 = scalar_multipl2d(p2, scale);
//
//			draw_stroke(cr, add_vectors2d(p1, center), add_vectors2d(p2, center));
//			last_osv = osv;
//		}
//	}
//}
// Rework trajectory drawing -> OSV + dt   ----------------------------------------------





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
