#ifndef KSP_DRAWING_H
#define KSP_DRAWING_H

#include <cairo.h>
#include "tools/celestial_systems.h"
#include "orbit_calculator/itin_tool.h"
#include "transfer_app/porkchop_analyzer_tools.h"
#include "gui/gui_tools/camera.h"


enum CoordAxisLabelType {COORD_LABEL_NUMBER, COORD_LABEL_DATE, COORD_LABEL_DURATION};

void draw_stroke(cairo_t *cr, Vector2 p1, Vector2 p2);
void draw_celestial_system(Camera *camera, CelestSystem *system, double jd_date);
void draw_body(Camera *camera, CelestSystem *system, Body *body, double jd_date);
void draw_orbit(Camera *camera, Orbit orbit);
void draw_itinerary(Camera *camera, CelestSystem *system, struct ItinStep *tf, double current_time);
void draw_orbit_2d(cairo_t *cr, Vector2 center, double scale, Vector3 r, Vector3 v, Body *attractor);
void draw_body_2d(cairo_t *cr, Vector2 center, double scale, Vector3 r);
double calc_scale(int area_width, int area_height, Body *farthest_body);
void set_cairo_body_color(cairo_t *cr, Body *body);
void draw_transfer_point_2d(cairo_t *cr, Vector2 center, double scale, Vector3 r);
void draw_trajectory_2d(cairo_t *cr, Vector2 center, double scale, struct ItinStep *tf, Body *attractor);
int get_porkchop_dur_yaxis_x();
int get_porkchop_arrdate_yaxis_x();
int get_porkchop_xaxis_y();
void draw_porkchop(cairo_t *cr, double width, double height, struct PorkchopAnalyzerPoint *porkchop, int num_itins, enum LastTransferType last_transfer_type, int dur0arrdate1);
void draw_plot(cairo_t *cr, double width, double height, double *x, double *y, int num_points);
void draw_multi_plot(cairo_t *cr, double width, double height, double *x, double **y, int num_plots, int num_points);

#endif //KSP_DRAWING_H
