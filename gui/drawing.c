//
// Created by niklas on 08.01.24.
//

#include "drawing.h"
#include "math.h"


void draw_orbit(cairo_t *cr, struct Vector2D center, double a, double e, double mu) {
	double h = sqrt(mu*a*(1-e*e));
	int steps = 100;
	for(int i = 0; i < steps; i++) {
		double theta = 2*M_PI/steps * i;
		double r = (h*h/mu)/(1+e*cos(theta)) / 1e9;
		struct Vector2D p0 = {.x = cos(theta)*r, .y = sin(theta)*r};
		
		theta = 2*M_PI/steps * (i+1);
		r = (h*h/mu)/(1+e*cos(theta)) / 1e9;
		struct Vector2D p1 = {.x = cos(theta)*r, .y = sin(theta)*r};
		
		scalar_multipl2d(p0,1e-9);
		scalar_multipl2d(p1,1e-9);
		
		p0 = add_vectors2d(p0, center);
		p1 = add_vectors2d(p1, center);
		draw_stroke(cr, p0, p1);
	}
}

void draw_stroke(cairo_t *cr, struct Vector2D p1, struct Vector2D p2) {
	cairo_set_line_width(cr, 1);
	
	cairo_move_to(cr, p1.x, p1.y);
	cairo_line_to(cr, p2.x, p2.y);
	cairo_stroke(cr);
}

