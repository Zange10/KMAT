#ifndef KSP_DRAWING_H
#define KSP_DRAWING_H

#include <cairo.h>
#include "tools/analytic_geometry.h"
#include "celestial_bodies.h"
#include "orbit_calculator/transfer_tools.h"
#include "orbit_calculator/itin_tool.h"


enum CoordAxisLabelType {COORD_LABEL_NUMBER, COORD_LABEL_DATE};

void draw_stroke(cairo_t *cr, struct Vector2D p1, struct Vector2D p2);
void draw_orbit(cairo_t *cr, struct Vector2D center, double scale, struct Vector r, struct Vector v, struct Body *attractor);
void draw_body(cairo_t *cr, struct Vector2D center, double scale, struct Vector r);
double calc_scale(int area_width, int area_height, int highest_id);
void set_cairo_body_color(cairo_t *cr, int id);
void draw_transfer_point(cairo_t *cr, struct Vector2D center, double scale, struct Vector r);
void draw_trajectory(cairo_t *cr, struct Vector2D center, double scale, struct ItinStep *tf);
void draw_porkchop(cairo_t *cr, double width, double height, const double *porkchop, int fb0_pow1);
void draw_plot(cairo_t *cr, double width, double height, double *x, double *y, int num_points);
void draw_multi_plot(cairo_t *cr, double width, double height, double *x, double **y, int num_plots, int num_points);
struct Vector2D p3d_to_p2d(struct Vector observer, struct Vector looking, struct Vector p3d, int width, int height);

#endif //KSP_DRAWING_H
