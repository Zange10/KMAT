//
// Created by niklas on 08.01.24.
//

#include "drawing.h"
#include "math.h"
#include "orbit_calculator/transfer_tools.h"


void draw_orbit(cairo_t *cr, struct Vector2D center, double scale, struct Vector r, struct Vector v, struct Body *attractor) {
	int steps = 100;
	struct OSV last_osv = {r,v};
	for(int i = 1; i <= steps; i++) {
		double theta = 2*M_PI/steps * i;
		struct OSV osv = propagate_orbit_theta(r,v,theta,attractor);
		struct Vector2D p1 = {last_osv.r.x, last_osv.r.y};
		struct Vector2D p2 = {osv.r.x, osv.r.y};
		
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

