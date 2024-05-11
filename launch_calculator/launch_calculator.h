#ifndef LAUNCH_CALCULATOR
#define LAUNCH_CALCULATOR


#include "tools/csv_writer.h"
#include "tools/tool_funcs.h"
#include "lv_profile.h"
#include "celestial_bodies.h"
#include "lp_parameters.h"


// start launch calculator menu
void launch_calculator();

struct Plane calc_plane_parallel_to_surf(struct Vector r);

struct Vector calc_surface_velocity_from_osv(struct Vector r, struct Vector v, struct Body *body);

double calc_vertical_speed_from_osv(struct Vector r, struct Vector v);

double calc_horizontal_orbspeed_from_osv(struct Vector r, struct Vector v);

double calc_downrange_distance(struct Vector r, double time, double launch_latitude, struct Body *body);

#endif