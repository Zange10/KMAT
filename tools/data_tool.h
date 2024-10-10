#ifndef KSP_DATA_TOOL_H
#define KSP_DATA_TOOL_H


#include "analytic_geometry.h"

double root_finder_monot_deriv_next_x(struct Vector2D *data, int branch);

double root_finder_monot_func_next_x(struct Vector2D *data, int branch);

void insert_new_data_point(struct Vector2D data[], double x, double y);

int can_be_negative_monot_deriv(struct Vector2D *data);

void print_double_array(char *name, double *array, int size);

#endif //KSP_DATA_TOOL_H
