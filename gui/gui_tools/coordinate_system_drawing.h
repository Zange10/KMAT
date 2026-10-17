#ifndef KMAT_COORDINATE_SYSTEM_DRAWING_H
#define KMAT_COORDINATE_SYSTEM_DRAWING_H

#include "coordinate_system.h"

void draw_coordinate_system_data(CoordinateSystem *coord_sys);
void draw_hover_position(CoordinateSystem *coord_sys, Vector2 mouse_pos);
void draw_coordinate_system_data_point(cairo_t *cr, double x, double y, double radius);
void draw_line_into_coordinate_system(cairo_t *cr, Vector2 point0, Vector2 point1, Vector2 origin);

#endif //KMAT_COORDINATE_SYSTEM_DRAWING_H