#include <stdio.h>
#include <math.h>

#include "launch_calculator.h"
#include "launch_sim.h"



void calc_launch_azimuth(struct Body *body) {
    double lat = 0;
    double incl = 0;

    printf("Enter parameters (latitude, inclination): ");
    scanf("%lf, %lf", &lat, &incl);

    double surf_speed = cos(deg2rad(lat)) * body->radius*2*M_PI/body->rotation_period;
    double end_speed = 7800;    // orbital speed hard coded


    double azi1 = asin((cos(deg2rad(incl)))/cos(deg2rad(lat)));
    double azi2 = asin(surf_speed/end_speed);
    double azi = rad2deg(azi1)-rad2deg(azi2);

    printf("\nNeeded launch Azimuth to target inclination %g° from %g° latitude: %g° %g°\n____________\n\n", incl, lat, azi, 180-azi);
}

// ------------------------------------------------------------

void launch_calculator() {
    struct LV lv;

    int selection;
    char title[] = "LAUNCH CALCULATOR:";
    char options[] = "Go Back; Calculate; Choose Profile; Create new Profile; Testing; Launch Profile Adjustments; Payload Launch Profile Analysis; Calculate Launch Azimuth";
    char question[] = "Program: ";
    do {
        selection = user_selection(title, options, question);

        switch(selection) {
            case 1:
                if(lv.stages != NULL) simulate_single_launch(lv); // if lv initialized
                break;
            case 2:
                read_LV_from_file(&lv);
                break;
            case 3:
                write_temp_LV_file();
                break;
            case 4:
                get_test_LV(&lv);
				simulate_single_launch(lv);
                break;
            case 5:
				get_test_LV(&lv);
				calc_payload_curve_with_set_lp_params(lv);
                break;
            case 6:
				get_test_LV(&lv);
				calc_payload_curve(lv);
                break;
            case 7:
                calc_launch_azimuth(EARTH());
                break;
        }
    } while(selection != 0);
}

struct Plane calc_plane_parallel_to_surf(struct Vector r) {
	struct Plane s;
	s.loc = r;
	s.u = norm_vector(cross_product(vec(0,0,1), r));	// east
	s.v = norm_vector(cross_product(r, s.u));	// north
	if(s.v.z < 0) s.v = scalar_multiply(s.v, -1); // should be unnecessary, but to be on safe side
	return s;
}

struct Vector calc_surface_velocity_from_osv(struct Vector r, struct Vector v, struct Body *body) {
	struct Plane surface_plane = calc_plane_parallel_to_surf(r);
	struct Plane eq = constr_plane(vec(0,0,0), vec(1,0,0), vec(0,1,0));
	double lat = angle_plane_vec(eq, r);
	double v_b = 2*M_PI*vector_mag(r) / body->rotation_period * cos(lat);
	struct Vector v_b_vec = scalar_multiply(surface_plane.u, v_b);
	return subtract_vectors(v, v_b_vec);
}

double calc_vertical_speed_from_osv(struct Vector r, struct Vector v) {
	struct Plane surface_plane = calc_plane_parallel_to_surf(r);
	struct Vector up = cross_product(surface_plane.v, surface_plane.u);
	struct Vector vert_v = proj_vec_vec(v, up);
	return vector_mag(vert_v);
}

double calc_downrange_distance(struct Vector r, double time, double launch_latitude, struct Body *body) {
	struct Vector launchsite = {cos(launch_latitude), 0, sin(launch_latitude)};
	launchsite = rotate_vector_around_axis(launchsite, vec(0,0,1), 2*M_PI*time/body->rotation_period);
	double angle_to_launchsite = angle_vec_vec(r, launchsite);
	return angle_to_launchsite*body->radius; // angle/2pi * 2pi*r
}
