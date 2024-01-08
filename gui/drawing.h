//
// Created by niklas on 08.01.24.
//

#ifndef KSP_DRAWING_H
#define KSP_DRAWING_H

#include <cairo.h>
#include "analytic_geometry.h"


void draw_stroke(cairo_t *cr, struct Vector2D p1, struct Vector2D p2);
void draw_orbit(cairo_t *cr, struct Vector2D center, double a, double e, double mu);

#endif //KSP_DRAWING_H
