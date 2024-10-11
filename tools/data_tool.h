#ifndef KSP_DATA_TOOL_H
#define KSP_DATA_TOOL_H


#include "analytic_geometry.h"

double root_finder_monot_deriv_next_x(struct Vector2D *data, int branch);

double root_finder_monot_func_next_x(struct Vector2D *data, int branch);

void insert_new_data_point(struct Vector2D data[], double x, double y);

// TODO RENAME
void insert_new_data_point2(struct Vector2D data[], double x, double y);

int get_min_value_index_from_data(struct Vector2D data[]);

int can_be_negative_monot_deriv(struct Vector2D *data);

void print_double_array(char *name, double *array, int size);

void print_data_vector(char *namex, char *namey, struct Vector2D *data);

#endif //KSP_DATA_TOOL_H
