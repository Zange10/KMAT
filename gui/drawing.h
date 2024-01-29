//
// Created by niklas on 08.01.24.
//

#ifndef KSP_DRAWING_H
#define KSP_DRAWING_H

#include <cairo.h>
#include "analytic_geometry.h"
#include "celestial_bodies.h"
#include "orbit_calculator/transfer_tools.h"



struct TransferData {
	struct Body *body;
	double date;
	double dv;
	struct TransferData *prev;
	struct TransferData *next;
};


void draw_stroke(cairo_t *cr, struct Vector2D p1, struct Vector2D p2);
void draw_orbit(cairo_t *cr, struct Vector2D center, double scale, struct Vector r, struct Vector v, struct Body *attractor);
void draw_body(cairo_t *cr, struct Vector2D center, double scale, struct Vector r);
double calc_scale(int area_width, int area_height, int highest_id);
void set_cairo_body_color(cairo_t *cr, int id);
void draw_transfer(cairo_t *cr, struct Vector2D center, double scale, struct Vector r);
void draw_trajectory(cairo_t *cr, struct Vector2D center, double scale, struct TransferData *tf0, struct TransferData *tf1, struct Ephem **ephems);
void draw_dsb(cairo_t *cr, struct Vector2D center, double scale, struct TransferData *tf0, struct TransferData *tf1, struct Ephem **ephems);


#endif //KSP_DRAWING_H
