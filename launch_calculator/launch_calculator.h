#ifndef LAUNCH_CALCULATOR
#define LAUNCH_CALCULATOR


#include "tools/file_io.h"
#include "tools/tool_funcs.h"
#include "lv_profile.h"
#include "tools/celestial_systems.h"
#include "lp_parameters.h"


// start launch calculator menu
void launch_calculator();

Plane3 calc_plane_parallel_to_surf(Vector3 r);

Vector3 calc_surface_velocity_from_osv(Vector3 r, Vector3 v, struct Body *body);

double calc_vertical_speed_from_osv(Vector3 r, Vector3 v);

double calc_horizontal_orbspeed_from_osv(Vector3 r, Vector3 v);

double calc_downrange_distance(Vector3 r, double time, double launch_latitude, struct Body *body);

#endif