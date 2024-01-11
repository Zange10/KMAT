//
// Created by niklas on 08.01.24.
//

#ifndef KSP_DRAWING_H
#define KSP_DRAWING_H

#include <cairo.h>
#include "analytic_geometry.h"
#include "celestial_bodies.h"


void draw_stroke(cairo_t *cr, struct Vector2D p1, struct Vector2D p2);
void draw_orbit(cairo_t *cr, struct Vector2D center, double scale, struct Vector r, struct Vector v, struct Body *attractor);

#endif //KSP_DRAWING_H
