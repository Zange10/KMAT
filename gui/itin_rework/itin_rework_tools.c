#include "itin_rework_tools.h"

#include "gui/gui_manager.h"
#include "gui/gui_tools/screen.h"
#include "gui/drawing.h"
#include "geometrylib.h"
#include <math.h>
#include <sys/time.h>


double calc_next_x_wrt_smoothness(DataArray2 *arr, int index_0, double tolerance) {
	Vector2 *data = &(data_array2_get_data(arr)[index_0]);
	size_t num_data = data_array2_size(arr)-index_0;

	if(num_data == 3) return (data[1].x - data[0].x)/1e9 + data[0].x;

	for(int i = 1; i < num_data-1; i++) {
		if((data[i+1].x - data[i].x) < 0.001) continue;
		double m = (data[i].y - data[i-1].y)/(data[i].x - data[i-1].x);
		double ip_y = data[i].y + m*(data[i+1].x-data[i].x);

		if(fabs(ip_y - data[i+1].y) > tolerance) {
			return (data[i].x + data[i+1].x)/2;
		}
	}

	return NAN;
}

void find_lambert_root(OSV osv_dep, double jd_dep, Body *dep_body, Body *arr_body, CelestSystem *system, double dt0, double dt1, double max_depdv, double dep_periapsis, double *left_x, double *right_x, double tol) {
	// x: dt, y: diff_vinf
	DataArray2 *data = data_array2_create();

	double t0 = jd_dep;
	double last_dt = -1e20, dt, t1;
	*left_x = 0;
	*right_x = 0;
	bool left_branch = true;

	if(dt0 < 100) dt0 = 100;	// transfer duration of 100s
	dt = dt0;

	for(int i = 0; i < 100; i++) {
		t1 = t0 + dt / 86400;

		OSV osv_arr = system->prop_method == ORB_ELEMENTS ?
				osv_from_elements(arr_body->orbit, t1) :
				osv_from_ephem(arr_body->ephem, arr_body->num_ephems, t1, system->cb);

		Lambert3 new_transfer = calc_lambert3(osv_dep.r, osv_arr.r, dt, system->cb);
		double vinf = fabs(mag_vec3(subtract_vec3(new_transfer.v0, osv_dep.v)));
		double dv_dep = isnan(dep_periapsis) ? vinf : dv_circ(dep_body,alt2radius(dep_body, dep_periapsis),vinf);

		if(i > 3 && max_depdv - dv_dep > 0 && max_depdv - dv_dep < tol || (i > 3 && fabs(dt-last_dt) < tol)) {
			if(left_branch) {
				*left_x = dt;
				last_dt = -1e20;
				left_branch = false;
				if(*right_x != 0) break;
			} else {
				*right_x = dt;
				break;
			}
		}


		data_array2_insert_new(data, vec2(dt, dv_dep - max_depdv));

		if(i == 1) {
			Vector2 *values = data_array2_get_data(data);
			if(values[0].y < 0) {
				*left_x = values[0].x;
				if(values[1].y < 0) {
					*right_x = values[1].x;
					break;
				}
				left_branch = false;
			}
			if(values[1].y < 0) {
				*right_x = values[1].x;
			}
		}

		if(i > 3 && dt == last_dt) break;	// step size 0 (imprecision)
		last_dt = dt;
		if(i == 0) dt = dt1;
		else dt = root_finder_single_minimum_func_next_x(data, left_branch, 0.25, 1e-20);
		if(isnan(dt) || isinf(dt)) break;
	}

	data_array2_free(data);
}

DataArray2 * find_local_peak_array(double jd_dep, Body *dep_body, Body *arr_body, CelestSystem *system, double dt0, double dt1, double tol, bool max_0_min_1) {
	// x: dt, y: diff_vinf
	DataArray2 *array = data_array2_create();

	double t0 = jd_dep;

	OSV osv_dep = system->prop_method == ORB_ELEMENTS ?
			osv_from_elements(dep_body->orbit, jd_dep) :
			osv_from_ephem(dep_body->ephem, dep_body->num_ephems, jd_dep, system->cb);

	if(dt0 < 100) dt0 = 100;	// transfer duration of 100s
	double dt = dt0;


	int num_steps = 100;

	for(int i = 0; i < num_steps; i++) {
		double t1 = t0 + i*(dt1-dt0)/num_steps/86400+dt0/86400;

		OSV osv_arr = system->prop_method == ORB_ELEMENTS ?
				osv_from_elements(arr_body->orbit, t1) :
				osv_from_ephem(arr_body->ephem, arr_body->num_ephems, t1, system->cb);

		Lambert3 new_transfer = calc_lambert3(osv_dep.r, osv_arr.r, (t1-t0)*86400, system->cb);
		double vinf = fabs(mag_vec3(subtract_vec3(new_transfer.v0, osv_dep.v)));


		data_array2_insert_new(array, vec2(t1-t0, vinf));
	}

	double last_dt = -1e20;

	for(int i = 0; i < 100; i++) {
		Vector2 *data = data_array2_get_data(array);

		int extr_idx = 0;
		if(max_0_min_1 == 0) {
			double max_val = -1e20;
			for(int j = 1; j < data_array2_size(array)-1; j++) {
				if(data[j].y > max_val && data[j].y > data[j-1].y && data[j].y > data[j+1].y) { extr_idx = j; max_val = data[j].y; }
			}
			if(extr_idx == 0) {
				if(data[data_array2_size(array)-1].y > data[0].y) {
					extr_idx = (int)data_array2_size(array)-1;
				}
				print_date(convert_JD_date(jd_dep, DATE_ISO), 0);
				printf("    |  %f   (%f)   |  %f  (%f)\n", dt0, dt0/86400, dt1, dt1/86400);
				print_data_array2(array, "dur", "dv");
			}
		} else {
			double min_val = 1e20;
			for(int j = 1; j < data_array2_size(array)-1; j++) {
				if(data[j].y < min_val && data[j].y < data[j-1].y && data[j].y < data[j+1].y) { extr_idx = j; min_val = data[j].y; }
			}
			if(extr_idx == 0) {
				if(data[data_array2_size(array)-1].y < data[0].y) {
					extr_idx = (int)data_array2_size(array)-1;
				}
				// print_date(convert_JD_date(jd_dep, DATE_ISO), 0);
				// printf("    |  %f   (%f)   |  %f  (%f)\n", dt0, dt0/86400, dt1, dt1/86400);
				// print_data_array2(array, "dur", "dv");
			}
		}

		if(extr_idx == data_array2_size(array)-1) dt = (data[extr_idx].x+data[extr_idx-1].x)/2;
		else if(extr_idx == 0) dt = (data[extr_idx].x+data[extr_idx+1].x)/2;
		else dt = (data[extr_idx].x+data[extr_idx-((i % 2 == 0) ? 1 : -1)].x)/2;
		dt *= 86400;

		double t1 = t0 + dt/86400;

		OSV osv_arr = system->prop_method == ORB_ELEMENTS ?
				osv_from_elements(arr_body->orbit, t1) :
				osv_from_ephem(arr_body->ephem, arr_body->num_ephems, t1, system->cb);

		Lambert3 new_transfer = calc_lambert3(osv_dep.r, osv_arr.r, (t1-t0)*86400, system->cb);
		double vinf = fabs(mag_vec3(subtract_vec3(new_transfer.v0, osv_dep.v)));


		data_array2_insert_new(array, vec2(t1-t0, vinf));

		if(fabs(last_dt-dt) < tol) break;
		last_dt = dt;
	}
	return array;
}


Vector2 get_local_peak(double jd_dep, Body *dep_body, Body *arr_body, CelestSystem *system, double dt0, double dt1, double tol, bool max_0_min_1) {
	DataArray2 *array = find_local_peak_array(jd_dep, dep_body, arr_body, system, dt0, dt1, tol, max_0_min_1);

	Vector2 *data = data_array2_get_data(array);
	int extr_idx = 0;
	if(max_0_min_1 == 0) {
		double max_val = -1e20;
		for(int j = 1; j < data_array2_size(array)-1; j++) {
			if(data[j].y > max_val && data[j].y > data[j-1].y && data[j].y > data[j+1].y) { extr_idx = j; max_val = data[j].y; }
		}
	} else {
		double min_val = 1e20;
		for(int j = 1; j < data_array2_size(array)-1; j++) {
			if(data[j].y < min_val && data[j].y < data[j-1].y && data[j].y < data[j+1].y) { extr_idx = j; min_val = data[j].y; }
		}
	}

	Vector2 peak = data[extr_idx];
	data_array2_free(array);

	if(extr_idx == 0) {
		peak.x = (dt0+dt1)/2/86400;
		peak.y = 0;
		// printf("test\n");
	}

	return peak;
}

double get_closest_relative_plane_traversal(Body *body0, Body *body1, CelestSystem *system, double jd_date) {
	OSV osv_dep_body = system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(body0->orbit, jd_date) :
					osv_from_ephem(body0->ephem, body0->num_ephems, jd_date, system->cb);
	OSV osv_arr_body = system->prop_method == ORB_ELEMENTS ?
						osv_from_elements(body1->orbit, jd_date) :
						osv_from_ephem(body1->ephem, body1->num_ephems, jd_date, system->cb);

	Plane3 orb_plane_dep = constr_plane3(vec3(0,0,0), osv_dep_body.r, osv_dep_body.v);
	Plane3 orb_plane_arr = constr_plane3(vec3(0,0,0), osv_arr_body.r, osv_arr_body.v);
	Vector3 inters_line = calc_intersecting_line_dir_plane3(orb_plane_dep, orb_plane_arr);
	Vector3 norm_vec = cross_vec3(osv_dep_body.r, osv_dep_body.v);
	Vector3 orth_vec = cross_vec3(norm_vec, osv_dep_body.r);

	double angle = angle_vec3_vec3(osv_dep_body.r, inters_line);
	double angle2 = angle_vec3_vec3(orth_vec, inters_line);

	double dta = angle2 < M_PI/2 ? angle : M_PI - angle;

	Orbit orbit = constr_orbit_from_osv(osv_dep_body.r, osv_dep_body.v, system->cb);

	double ta0 = orbit.ta;

	double tsp0 = calc_orbit_time_since_periapsis(orbit);
	orbit.ta = ta0 + dta;
	bool past_periapsis = orbit.ta >= 2*M_PI;
	if(past_periapsis) orbit.ta -= 2*M_PI;
	double tsp_next = calc_orbit_time_since_periapsis(orbit);
	if(past_periapsis) tsp_next += calc_orbital_period(orbit);
	double dt_next = tsp_next - tsp0;
	double next_trav = jd_date + dt_next/86400;

	orbit.ta = ta0 - (M_PI-dta);
	past_periapsis = orbit.ta < 0;
	if(past_periapsis) orbit.ta += 2*M_PI;
	double tsp_prev = calc_orbit_time_since_periapsis(orbit);
	if(past_periapsis) tsp_prev -= calc_orbital_period(orbit);
	double dt_prev = tsp_prev - tsp0;
	double prev_trav = jd_date + dt_prev/86400;

	if(dt_next < -dt_prev) {
		if(dt_next > 60) return get_closest_relative_plane_traversal(body0, body1, system, next_trav);
		return next_trav;
	} else {
		if(-dt_prev > 60) return get_closest_relative_plane_traversal(body0, body1, system, prev_trav);
		return prev_trav;
	}
}

void get_prev_and_next_relative_plane_traversal(Body *body0, Body *body1, CelestSystem *system, double jd_date, double *prev_trav, double *next_trav) {
	OSV osv_dep_body = system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(body0->orbit, jd_date) :
					osv_from_ephem(body0->ephem, body0->num_ephems, jd_date, system->cb);
	OSV osv_arr_body = system->prop_method == ORB_ELEMENTS ?
						osv_from_elements(body1->orbit, jd_date) :
						osv_from_ephem(body1->ephem, body1->num_ephems, jd_date, system->cb);

	Plane3 orb_plane_dep = constr_plane3(vec3(0,0,0), osv_dep_body.r, osv_dep_body.v);
	Plane3 orb_plane_arr = constr_plane3(vec3(0,0,0), osv_arr_body.r, osv_arr_body.v);
	Vector3 inters_line = calc_intersecting_line_dir_plane3(orb_plane_dep, orb_plane_arr);
	Vector3 norm_vec = cross_vec3(osv_dep_body.r, osv_dep_body.v);
	Vector3 orth_vec = cross_vec3(norm_vec, osv_dep_body.r);

	double angle = angle_vec3_vec3(osv_dep_body.r, inters_line);
	double angle2 = angle_vec3_vec3(orth_vec, inters_line);

	double dta = angle2 < M_PI/2 ? angle : M_PI - angle;

	Orbit orbit = constr_orbit_from_osv(osv_dep_body.r, osv_dep_body.v, system->cb);

	double ta0 = orbit.ta;

	double tsp0 = calc_orbit_time_since_periapsis(orbit);
	orbit.ta = ta0 + dta;
	bool past_periapsis = orbit.ta >= 2*M_PI;
	if(past_periapsis) orbit.ta -= 2*M_PI;
	double tsp1 = calc_orbit_time_since_periapsis(orbit);
	if(past_periapsis) tsp1 += calc_orbital_period(orbit);
	double dt = tsp1 - tsp0;
	*next_trav = get_closest_relative_plane_traversal(body0, body1, system, jd_date + dt/86400);

	orbit.ta = ta0 - (M_PI-dta);
	past_periapsis = orbit.ta < 0;
	if(past_periapsis) orbit.ta += 2*M_PI;
	tsp1 = calc_orbit_time_since_periapsis(orbit);
	if(past_periapsis) tsp1 -= calc_orbital_period(orbit);
	dt = tsp1 - tsp0;
	*prev_trav = get_closest_relative_plane_traversal(body0, body1, system, jd_date + dt/86400);
}

double calc_opposition_conjunction_gradient(Body *dep_body, Body *arr_body, CelestSystem *system, double jd_dep) {
	OSV osv0 = system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(dep_body->orbit, jd_dep) :
					osv_from_ephem(dep_body->ephem, dep_body->num_ephems, jd_dep, system->cb);

	OSV osv_arr0 = system->prop_method == ORB_ELEMENTS ?
				   osv_from_elements(arr_body->orbit, jd_dep) :
				   osv_from_ephem(arr_body->ephem, arr_body->num_ephems, jd_dep, system->cb);
	Orbit orbit0 = constr_orbit_from_osv(osv0.r, osv0.v, system->cb);
	Orbit orbit1 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, system->cb);

	return calc_orbital_period(orbit1)/calc_orbital_period(orbit0) - 1;
}

void set_opposition_conjunction_group_boundary(SegmentGroup *group, int shift, double jd_min_dep, double jd_max_dep, double min_dur, double max_dur, bool cut_at_durminmax) {
	DataArray2 *lower_boundary = data_array2_create();
	DataArray2 *upper_boundary = data_array2_create();

	double opp_conj_gradient = calc_opposition_conjunction_gradient(group->dep_body, group->arr_body, group->system, (jd_min_dep+jd_max_dep)/2);
	group->boundary_gradient = opp_conj_gradient;
	if(opp_conj_gradient > 0) shift *= -1;

	OSV osv0 = group->system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(group->dep_body->orbit, jd_min_dep) :
					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_min_dep, group->system->cb);

	OSV osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
				   osv_from_elements(group->arr_body->orbit, jd_min_dep) :
				   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_min_dep, group->system->cb);
	double next_conjunction_dt, next_opposition_dt, last_conjunction_dt, last_opposition_dt;
	calc_time_to_next_conjunction_and_opposition(osv0.r, osv_arr0, group->system->cb, &last_conjunction_dt, &last_opposition_dt);
	Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, group->system->cb);
	double period_arr0 = calc_orbital_period(arr0);
	Orbit dep_orbit = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb);
	double period_dep = calc_orbital_period(dep_orbit);

	// default shift from calc_time_to_next_conjunction_and_opposition is 1 (but we want values around 0 for start)
	if(shift % 2 == 0) {
		if(last_conjunction_dt > last_opposition_dt) last_conjunction_dt -= period_arr0;
		else last_opposition_dt -= period_arr0;
		shift++;
	}
	shift--;
	last_opposition_dt += period_arr0 * shift/2;
	last_conjunction_dt += period_arr0 * shift/2;

	double jd_dep = jd_min_dep;
	double syn_period = 1.0/fabs(1.0/period_dep - 1.0/period_arr0)/86400;
	double jd_dep_step = syn_period/100;

	DataArray1 *boundary_points = data_array1_create();
	while(jd_dep < jd_max_dep) {
		data_array1_append_new(boundary_points, jd_dep);
		jd_dep += jd_dep_step;
	}
	data_array1_append_new(boundary_points, jd_max_dep);

	DataArray1 *traversals = data_array1_create();
	double trav_search_date = jd_min_dep, prev_trav, next_trav;
	double offsets[] = {-syn_period*0.0025, -syn_period*0.0001, syn_period*0.0001, syn_period*0.0025};
	do {
		get_prev_and_next_relative_plane_traversal(group->dep_body, group->arr_body, group->system, trav_search_date, &prev_trav, &next_trav);
		data_array1_append_new(traversals, prev_trav);
		data_array1_append_new(traversals, next_trav);
		for(int i = 0; i < sizeof(offsets)/sizeof(double); i++) {
			jd_dep = prev_trav + offsets[i];
			if(jd_dep >= jd_min_dep && jd_dep <= jd_max_dep)
				data_array1_insert_new(boundary_points, jd_dep);
			jd_dep = next_trav + offsets[i];
			if(jd_dep >= jd_min_dep && jd_dep <= jd_max_dep)
				data_array1_insert_new(boundary_points, jd_dep);
		}
		trav_search_date += period_dep/86400;
	} while(next_trav < jd_max_dep);

	double local_peak_half_width_dt = period_arr0*0.01;

	for(int i = 0; i < data_array1_size(boundary_points); i++) {
		jd_dep = data_array1_get_data(boundary_points)[i];

		osv0 = group->system->prop_method == ORB_ELEMENTS ?
						osv_from_elements(group->dep_body->orbit, jd_dep) :
						osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);

		osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
					   osv_from_elements(group->arr_body->orbit, jd_dep) :
					   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_dep, group->system->cb);
		calc_time_to_next_conjunction_and_opposition(osv0.r, osv_arr0, group->system->cb, &next_conjunction_dt, &next_opposition_dt);


		double opp_guess = last_opposition_dt + jd_dep_step*86400*opp_conj_gradient;
		double conj_guess = last_conjunction_dt + jd_dep_step*86400*opp_conj_gradient;

		while(opp_guess-next_opposition_dt   >  0.5 * period_arr0) next_opposition_dt  += period_arr0;
		while(opp_guess-next_opposition_dt   < -0.5 * period_arr0) next_opposition_dt  -= period_arr0;
		while(conj_guess-next_conjunction_dt >  0.5 * period_arr0) next_conjunction_dt += period_arr0;
		while(conj_guess-next_conjunction_dt < -0.5 * period_arr0) next_conjunction_dt -= period_arr0;


		for(int j = 0; j < data_array1_size(traversals); j++) {
			if(true || fabs(jd_dep - data_array1_get_data(traversals)[j]) < period_dep/86400*0.05) {
				if(next_opposition_dt + local_peak_half_width_dt >= min_dur*0.9*86400 && next_opposition_dt - local_peak_half_width_dt <= max_dur*1.1*86400)
					next_opposition_dt = get_local_peak(jd_dep, group->dep_body, group->arr_body, group->system, next_opposition_dt-local_peak_half_width_dt, next_opposition_dt+local_peak_half_width_dt, 1, 0).x*86400;
				// if(next_conjunction_dt + local_peak_half_width_dt >= min_dur*0.9*86400 && next_conjunction_dt - local_peak_half_width_dt <= max_dur*1.1*86400)
				// 	next_conjunction_dt = get_local_peak(jd_dep, group->dep_body, group->arr_body, group->system, next_conjunction_dt-local_peak_half_width_dt, next_conjunction_dt+local_peak_half_width_dt, 1).x*86400;
				break;
			}
		}


		last_opposition_dt = next_opposition_dt;
		last_conjunction_dt = next_conjunction_dt;
		if(next_conjunction_dt < next_opposition_dt) {
			data_array2_append_new(lower_boundary, vec2(jd_dep, next_conjunction_dt/86400));
			data_array2_append_new(upper_boundary, vec2(jd_dep, next_opposition_dt/86400));
		} else {
			data_array2_append_new(lower_boundary, vec2(jd_dep, next_opposition_dt/86400));
			data_array2_append_new(upper_boundary, vec2(jd_dep, next_conjunction_dt/86400));
		}
	}
	group->top_boundary_type = next_conjunction_dt < next_opposition_dt ?
	DEPARTURE_GROUP_BOUNDARY_TOP_OPP : DEPARTURE_GROUP_BOUNDARY_TOP_CONJ;

	if(cut_at_durminmax) {
		DataArray2 *arr_mindur = data_array2_create();
		data_array2_append_new(arr_mindur, vec2(jd_min_dep, min_dur));
		data_array2_append_new(arr_mindur, vec2(jd_max_dep, min_dur));
		DataArray2 *arr_maxdur = data_array2_create();
		data_array2_append_new(arr_maxdur, vec2(jd_min_dep, max_dur));
		data_array2_append_new(arr_maxdur, vec2(jd_max_dep, max_dur));

		DataArray2 *lower_inters = get_line_intersections(lower_boundary, arr_mindur);
		for(int i = 0; i < data_array2_size(lower_inters); i++) {
			Vector2 val = data_array2_get_data(lower_inters)[i];
			data_array2_insert_new(lower_boundary, val);
			data_array2_insert_new(upper_boundary, vec2(val.x, interpolate_from_sorted_data_array2(upper_boundary, val.x)));
		}
		DataArray2 *upper_inters = get_line_intersections(upper_boundary, arr_maxdur);
		for(int i = 0; i < data_array2_size(upper_inters); i++) {
			Vector2 val = data_array2_get_data(upper_inters)[i];
			data_array2_insert_new(upper_boundary, val);
			data_array2_insert_new(lower_boundary, vec2(val.x, interpolate_from_sorted_data_array2(lower_boundary, val.x)));
		}
		data_array2_free(lower_inters);
		data_array2_free(upper_inters);
		data_array2_free(arr_mindur);
		data_array2_free(arr_maxdur);

		Vector2 *dl = data_array2_get_data(lower_boundary);
		Vector2 *du = data_array2_get_data(upper_boundary);
		for(int i = 0; i < data_array2_size(lower_boundary); i++) {
			if(dl[i].y < min_dur) dl[i].y = min_dur;
			if(du[i].y > max_dur) du[i].y = max_dur;
		}

		DataArray2 *inters = get_line_intersections(lower_boundary, upper_boundary);
		for(int i = 0; i < data_array2_size(inters); i++) {
			Vector2 val = data_array2_get_data(inters)[i];
			data_array2_remove_by_value(upper_boundary, val);
			data_array2_insert_new(upper_boundary, val);
			data_array2_remove_by_value(lower_boundary, val);
			data_array2_insert_new(lower_boundary, val);
		}
		data_array2_free(inters);

		dl = data_array2_get_data(lower_boundary);
		du = data_array2_get_data(upper_boundary);
		for(int i = 0; i < data_array2_size(lower_boundary); i++) {
			if(du[i].y < dl[i].y) {
				data_array2_remove_at_idx(lower_boundary, i);
				data_array2_remove_at_idx(upper_boundary, i);
				i--;
			}
		}
	}


	append_to_boundary(&group->group_bdr, upper_boundary, lower_boundary);

	data_array1_free(boundary_points);
	data_array1_free(traversals);
}

DataArray2 * calc_dv_boundary(SegmentGroup *group, int departure_cap, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance) {
	DataArray2 *boundary_array = data_array2_create();

	OSV osv0 = group->system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(group->dep_body->orbit, jd_min_dep) :
					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_min_dep, group->system->cb);

	OSV osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
				   osv_from_elements(group->arr_body->orbit, jd_min_dep) :
				   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_min_dep, group->system->cb);
	Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, group->system->cb);
	double period_arr0 = calc_orbital_period(arr0);
	Orbit dep_orbit = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb);
	double period_dep = calc_orbital_period(dep_orbit);
	double syn_period = 1.0/fabs(1.0/period_dep - 1.0/period_arr0)/86400;
	// printf("%f  %f\n", syn_period, group->boundary_gradient);
	double jd_dep_step = syn_period/100;
	double min_jd_dep_step = syn_period*0.00001;
	double dt0, dt1;


	// double r0 = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb).a, r1 = arr0.a;
	// double r_ratio =  r1/r0;
	// Hohmann hohmann = calc_hohmann_transfer(r0, r1, group->system->cb);
	// double hohmann_dur = hohmann.dur/86400;
	// double min_duration = 0.4 * hohmann_dur;
	// double max_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_duration/hohmann_dur > 3) max_duration = hohmann_dur*3;
	// if(max_duration < max_dur) max_dur = max_duration;
	// if(min_duration > min_dur) min_dur = min_duration;

	double min_dt = min_dur*86400;
	double max_dt = max_dur*86400;

	DataArray1 *dep_points = data_array1_create();
	double jd_dep = jd_min_dep;
	while(jd_dep < jd_max_dep) {
		data_array1_append_new(dep_points, jd_dep);
		jd_dep += jd_dep_step;
	}
	data_array1_append_new(dep_points, jd_max_dep);


	DataArray1 *traversals = data_array1_create();
	double trav_search_date = jd_min_dep, prev_trav, next_trav;
	double offset_base = syn_period*0.0001;
	do {
		get_prev_and_next_relative_plane_traversal(group->dep_body, group->arr_body, group->system, trav_search_date, &prev_trav, &next_trav);
		data_array1_append_new(traversals, prev_trav);
		data_array1_append_new(traversals, next_trav);
		for(int i = -10; i <= 10; i++) {
			jd_dep = prev_trav + offset_base*i;
			if(jd_dep >= jd_min_dep && jd_dep <= jd_max_dep)
				data_array1_insert_new(dep_points, jd_dep);
			jd_dep = next_trav + offset_base*i;
			if(jd_dep >= jd_min_dep && jd_dep <= jd_max_dep)
				data_array1_insert_new(dep_points, jd_dep);
		}
		trav_search_date += period_dep/86400;
	} while(next_trav < jd_max_dep);

	while(data_array1_size(dep_points) > 0) {
		for(int i = 0; i < data_array1_size(dep_points); i++) {
			jd_dep = data_array1_get_data(dep_points)[i];

			dt0 = interpolate_from_sorted_data_array2(group->group_bdr.lower_bdrs[0], jd_dep) * 86400;
			dt1 = interpolate_from_sorted_data_array2(group->group_bdr.upper_bdrs[0], jd_dep) * 86400;

			if(dt0 > max_dt || dt1 < min_dt) {
				data_array2_insert_new(boundary_array, vec2(jd_dep, -1e20));
				data_array2_insert_new(boundary_array, vec2(jd_dep, -1e20));
				continue;
			}

			double left_x = 0, right_x = 0;
			osv0 = group->system->prop_method == ORB_ELEMENTS ?
								osv_from_elements(group->dep_body->orbit, jd_dep) :
								osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);
			find_lambert_root(osv0, jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, max_depdv, dep_periapsis, &left_x, &right_x, 1e-4);

			// No departure possible within given constraints
			if(left_x < 1 || right_x < 1 || right_x < min_dur*86400 || left_x > max_dur*86400) {
				data_array2_insert_new(boundary_array, vec2(jd_dep, -1e20));
				data_array2_insert_new(boundary_array, vec2(jd_dep, -1e20));
				continue;
			}

			if(left_x < dt0) left_x = dt0;
			if(left_x < min_dur*86400) left_x = min_dur*86400;
			if(right_x > dt1) right_x = dt1;
			if(right_x > max_dur*86400) right_x = max_dur*86400;

			data_array2_insert_new(boundary_array, vec2(jd_dep, left_x/86400));
			data_array2_insert_new(boundary_array, vec2(jd_dep, right_x/86400));
		}
		data_array1_clear(dep_points);
		DataArray1 *dep_temp = data_array1_create();

		size_t num_deps = data_array2_size(boundary_array);
		Vector2 *transf_arr = malloc(num_deps * sizeof(Vector2));
		for(int i = 0; i < num_deps; i++) {
			transf_arr[i].x = data_array2_get_data(boundary_array)[i].x;
			transf_arr[i].y = data_array2_get_data(boundary_array)[i].y/group->boundary_gradient;
		}
		for(int i = 4; i < num_deps; i++) {
			Vector2 v0 = transf_arr[i-4];
			Vector2 v1 = transf_arr[i-2];
			Vector2 v2 = transf_arr[i  ];

			// if(v1.x == v0.x || v1.x == v2.x) {
			// 	printf("%f   %f   %f\n", v0.x-jd_min_dep, v1.x-jd_min_dep, v2.x-jd_min_dep);
			// }

			double m0 = (v1.y - v0.y)/(v1.x - v0.x);
			double m1 = (v2.y - v1.y)/(v2.x - v1.x);

			double angle0 = atan(m0);
			double angle1 = atan(m1);

			double da = fabs(angle1 - angle0);

			if(da > deg2rad(5.0)) {
				// printf("%f°    %f°  (%f)  %f°   (%f)\n", rad2deg(fabs(angle0-angle1)), rad2deg(angle0), m0, rad2deg(angle1), m1);
				if(fabs(v0.x-v1.x) > min_jd_dep_step) {
					data_array1_append_new(dep_points, (v0.x+v1.x)/2);
					data_array1_append_new(dep_temp, (v0.x+v1.x)/2 - jd_min_dep);
				}
				if(fabs(v1.x-v2.x) > min_jd_dep_step) {
					data_array1_append_new(dep_points, (v1.x+v2.x)/2);
					data_array1_append_new(dep_temp, (v1.x+v2.x)/2 - jd_min_dep);
				}
				i += 3 + (i%2==0);
			}

			if(isnan(da)) {
				printf("da is nan\n");
			}
		}
		// print_data_array1(dep_points, "dep");
		// print_data_array1(dep_temp, "dep");
		// printf("%lu\n", data_array2_size(boundary_array));
		data_array1_free(dep_temp);
		free(transf_arr);
	}

	for(int i = 0; i < data_array2_size(boundary_array); i++) {
		if(data_array2_get_data(boundary_array)[i].y < -1e19) {
			if(i == 0) {
				if(data_array2_get_data(boundary_array)[1].y < -1e19) {
					data_array2_remove_at_idx(boundary_array, 0);
					i--; continue;
				}
			}
			if(i == data_array2_size(boundary_array)-1) {
				if(isnan(data_array2_get_data(boundary_array)[i-1].y)) {
					data_array2_remove_at_idx(boundary_array, i);
					break;
				}
			}
			if(isnan(data_array2_get_data(boundary_array)[i-1].y) && data_array2_get_data(boundary_array)[i+1].y < -1e19) {
				data_array2_remove_at_idx(boundary_array, i);
				i--; continue;
			}
			data_array2_get_data(boundary_array)[i].y = NAN;
			continue;
		}

		if(i == 0) continue;
		if(i == data_array2_size(boundary_array)-1) continue;

		if(isnan(data_array2_get_data(boundary_array)[i-1].y) && data_array2_get_data(boundary_array)[i+2].y < -1e19) {
			data_array2_remove_at_idx(boundary_array, i);
			data_array2_remove_at_idx(boundary_array, i);
			i--; continue;
		}

		if(isnan(data_array2_get_data(boundary_array)[i-1].y)) {
			double new_dep = (data_array2_get_data(boundary_array)[i-1].x+data_array2_get_data(boundary_array)[i  ].x)/2;
			double new_dur = (data_array2_get_data(boundary_array)[i  ].y+data_array2_get_data(boundary_array)[i+1].y)/2;
			data_array2_insert_new(boundary_array, vec2(new_dep, new_dur));
			data_array2_insert_new(boundary_array, vec2(new_dep, new_dur));
		}
		if(data_array2_get_data(boundary_array)[i+1].y < -1e19) {
			double new_dep = (data_array2_get_data(boundary_array)[i+1].x+data_array2_get_data(boundary_array)[i  ].x)/2;
			double new_dur = (data_array2_get_data(boundary_array)[i  ].y+data_array2_get_data(boundary_array)[i-1].y)/2;
			data_array2_insert_new(boundary_array, vec2(new_dep, new_dur));
			data_array2_insert_new(boundary_array, vec2(new_dep, new_dur));
			i+=2;
		}
	}

	if(isnan(data_array2_get_data(boundary_array)[0].y)) {
		data_array2_remove_at_idx(boundary_array, 0);
	}
	if(isnan(data_array2_get_data(boundary_array)[data_array2_size(boundary_array)-1].y)) {
		data_array2_remove_at_idx(boundary_array, (int) data_array2_size(boundary_array)-1);
	}
	// printf("%lu\n", data_array2_size(boundary_array));
	// print_data_array2(boundary_array, "depdate", "dur");
	return boundary_array;
}

void set_dep_group_dv_boundary(SegmentGroup *group, int departure_cap, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance) {
	DataArray2 *boundary_array = data_array2_create();

	OSV osv0 = group->system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(group->dep_body->orbit, jd_min_dep) :
					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_min_dep, group->system->cb);

	OSV osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
				   osv_from_elements(group->arr_body->orbit, jd_min_dep) :
				   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_min_dep, group->system->cb);
	Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, group->system->cb);
	double period_arr0 = calc_orbital_period(arr0);
	Orbit dep_orbit = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb);
	double period_dep = calc_orbital_period(dep_orbit);
	double syn_period = 1.0/fabs(1.0/period_dep - 1.0/period_arr0)/86400;
	// printf("%f  %f\n", syn_period, group->boundary_gradient);
	double jd_dep_step = syn_period/100;
	double dt0, dt1;


	// double r0 = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb).a, r1 = arr0.a;
	// double r_ratio =  r1/r0;
	// Hohmann hohmann = calc_hohmann_transfer(r0, r1, group->system->cb);
	// double hohmann_dur = hohmann.dur/86400;
	// double min_duration = 0.4 * hohmann_dur;
	// double max_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_duration/hohmann_dur > 3) max_duration = hohmann_dur*3;
	// if(max_duration < max_dur) max_dur = max_duration;
	// if(min_duration > min_dur) min_dur = min_duration;

	double min_dt = min_dur*86400;
	double max_dt = max_dur*86400;

	DataArray1 *dep_points = data_array1_create();
	double jd_dep = jd_min_dep;
	while(jd_dep < jd_max_dep) {
		data_array1_append_new(dep_points, jd_dep);
		jd_dep += jd_dep_step;
	}
	data_array1_append_new(dep_points, jd_max_dep);


	DataArray1 *traversals = data_array1_create();
	double trav_search_date = jd_min_dep, prev_trav, next_trav;
	double offset_base = syn_period*0.0001;
	do {
		get_prev_and_next_relative_plane_traversal(group->dep_body, group->arr_body, group->system, trav_search_date, &prev_trav, &next_trav);
		data_array1_append_new(traversals, prev_trav);
		data_array1_append_new(traversals, next_trav);
		for(int i = -10; i <= 10; i++) {
			jd_dep = prev_trav + offset_base*i;
			if(jd_dep >= jd_min_dep && jd_dep <= jd_max_dep)
				data_array1_insert_new(dep_points, jd_dep);
			jd_dep = next_trav + offset_base*i;
			if(jd_dep >= jd_min_dep && jd_dep <= jd_max_dep)
				data_array1_insert_new(dep_points, jd_dep);
		}
		trav_search_date += period_dep/86400;
	} while(next_trav < jd_max_dep);

	while(data_array1_size(dep_points) > 0) {
		for(int i = 0; i < data_array1_size(dep_points); i++) {
			jd_dep = data_array1_get_data(dep_points)[i];

			dt0 = interpolate_from_sorted_data_array2(group->group_bdr.lower_bdrs[0], jd_dep) * 86400;
			dt1 = interpolate_from_sorted_data_array2(group->group_bdr.upper_bdrs[0], jd_dep) * 86400;

			if(dt0 > max_dt || dt1 < min_dt) {
				data_array2_insert_new(boundary_array, vec2(jd_dep, -1e20));
				data_array2_insert_new(boundary_array, vec2(jd_dep, -1e20));
				continue;
			}

			double left_x = 0, right_x = 0;
			osv0 = group->system->prop_method == ORB_ELEMENTS ?
								osv_from_elements(group->dep_body->orbit, jd_dep) :
								osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);
			find_lambert_root(osv0, jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, max_depdv, dep_periapsis, &left_x, &right_x, 1e-4);

			// No departure possible within given constraints
			if(left_x < 1 || right_x < 1 || right_x < min_dur*86400 || left_x > max_dur*86400) {
				data_array2_insert_new(boundary_array, vec2(jd_dep, -1e20));
				data_array2_insert_new(boundary_array, vec2(jd_dep, -1e20));
				continue;
			}

			if(left_x < dt0) left_x = dt0;
			if(left_x < min_dur*86400) left_x = min_dur*86400;
			if(right_x > dt1) right_x = dt1;
			if(right_x > max_dur*86400) right_x = max_dur*86400;

			data_array2_insert_new(boundary_array, vec2(jd_dep, left_x/86400));
			data_array2_insert_new(boundary_array, vec2(jd_dep, right_x/86400));
		}
		data_array1_clear(dep_points);
		DataArray1 *dep_temp = data_array1_create();

		size_t num_deps = data_array2_size(boundary_array);
		Vector2 *transf_arr = malloc(num_deps * sizeof(Vector2));
		for(int i = 0; i < num_deps; i++) {
			transf_arr[i].x = data_array2_get_data(boundary_array)[i].x;
			transf_arr[i].y = data_array2_get_data(boundary_array)[i].y/group->boundary_gradient;
		}
		for(int i = 4; i < num_deps; i++) {
			Vector2 v0 = transf_arr[i-4];
			Vector2 v1 = transf_arr[i-2];
			Vector2 v2 = transf_arr[i  ];

			// if(v1.x == v0.x || v1.x == v2.x) {
			// 	printf("%f   %f   %f\n", v0.x-jd_min_dep, v1.x-jd_min_dep, v2.x-jd_min_dep);
			// }

			double m0 = (v1.y - v0.y)/(v1.x - v0.x);
			double m1 = (v2.y - v1.y)/(v2.x - v1.x);

			double angle0 = atan(m0);
			double angle1 = atan(m1);

			double da = fabs(angle1 - angle0);

			if(da > deg2rad(5.0)) {
				// printf("%f°    %f°  (%f)  %f°   (%f)\n", rad2deg(fabs(angle0-angle1)), rad2deg(angle0), m0, rad2deg(angle1), m1);
				if(fabs(v0.x-v1.x) > syn_period*0.0001) {
					data_array1_append_new(dep_points, (v0.x+v1.x)/2);
					data_array1_append_new(dep_temp, (v0.x+v1.x)/2 - jd_min_dep);
				}
				if(fabs(v1.x-v2.x) > syn_period*0.0001) {
					data_array1_append_new(dep_points, (v1.x+v2.x)/2);
					data_array1_append_new(dep_temp, (v1.x+v2.x)/2 - jd_min_dep);
				}
				i += 3 + (i%2==0);
			}

			if(isnan(da)) {
				printf("da is nan\n");
			}
		}
		// print_data_array1(dep_points, "dep");
		// print_data_array1(dep_temp, "dep");
		// printf("%lu\n", data_array2_size(boundary_array));
		data_array1_free(dep_temp);
		free(transf_arr);
	}


	int idx = 0;
	Vector2 *data = data_array2_get_data(boundary_array);
	size_t num = data_array2_size(boundary_array);
	while(idx < num) {
		while(idx < num && data[idx].y < -1e19) idx+=2;
		if(idx >= num) break;

		DataArray2 *lower = data_array2_create();
		DataArray2 *upper = data_array2_create();

		while(idx < num && data[idx].y > -1e19) {
			data_array2_append_new(lower, data[idx]);
			idx++;
			data_array2_append_new(upper, data[idx]);
			idx++;
		}
		append_to_boundary(&group->dv_bdr, upper, lower);
	}


	// for(int i = 0; i < data_array2_size(boundary_array); i++) {
	// 	if(data_array2_get_data(boundary_array)[i].y < -1e19) {
	// 		if(i == 0) {
	// 			if(data_array2_get_data(boundary_array)[1].y < -1e19) {
	// 				data_array2_remove_at_idx(boundary_array, 0);
	// 				i--; continue;
	// 			}
	// 		}
	// 		if(i == data_array2_size(boundary_array)-1) {
	// 			if(isnan(data_array2_get_data(boundary_array)[i-1].y)) {
	// 				data_array2_remove_at_idx(boundary_array, i);
	// 				break;
	// 			}
	// 		}
	// 		if(isnan(data_array2_get_data(boundary_array)[i-1].y) && data_array2_get_data(boundary_array)[i+1].y < -1e19) {
	// 			data_array2_remove_at_idx(boundary_array, i);
	// 			i--; continue;
	// 		}
	// 		data_array2_get_data(boundary_array)[i].y = NAN;
	// 		continue;
	// 	}
	//
	// 	if(i == 0) continue;
	// 	if(i == data_array2_size(boundary_array)-1) continue;
	//
	// 	if(isnan(data_array2_get_data(boundary_array)[i-1].y) && data_array2_get_data(boundary_array)[i+2].y < -1e19) {
	// 		data_array2_remove_at_idx(boundary_array, i);
	// 		data_array2_remove_at_idx(boundary_array, i);
	// 		i--; continue;
	// 	}
	//
	// 	if(isnan(data_array2_get_data(boundary_array)[i-1].y)) {
	// 		double new_dep = (data_array2_get_data(boundary_array)[i-1].x+data_array2_get_data(boundary_array)[i  ].x)/2;
	// 		double new_dur = (data_array2_get_data(boundary_array)[i  ].y+data_array2_get_data(boundary_array)[i+1].y)/2;
	// 		data_array2_insert_new(boundary_array, new_dep, new_dur);
	// 		data_array2_insert_new(boundary_array, new_dep, new_dur);
	// 	}
	// 	if(data_array2_get_data(boundary_array)[i+1].y < -1e19) {
	// 		double new_dep = (data_array2_get_data(boundary_array)[i+1].x+data_array2_get_data(boundary_array)[i  ].x)/2;
	// 		double new_dur = (data_array2_get_data(boundary_array)[i  ].y+data_array2_get_data(boundary_array)[i-1].y)/2;
	// 		data_array2_insert_new(boundary_array, new_dep, new_dur);
	// 		data_array2_insert_new(boundary_array, new_dep, new_dur);
	// 		i+=2;
	// 	}
	// }
	//
	// if(isnan(data_array2_get_data(boundary_array)[0].y)) {
	// 	data_array2_remove_at_idx(boundary_array, 0);
	// }
	// if(isnan(data_array2_get_data(boundary_array)[data_array2_size(boundary_array)-1].y)) {
	// 	data_array2_remove_at_idx(boundary_array, (int) data_array2_size(boundary_array)-1);
	// }
	// printf("%lu\n", data_array2_size(boundary_array));
	// print_data_array2(boundary_array, "depdate", "dur");
}

DataArray2 * calc_min_vinf_line(SegmentGroup *group, double jd_min_dep, double jd_max_dep, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance) {
	DataArray2 *vinf_line = data_array2_create();

	OSV osv0 = group->system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(group->dep_body->orbit, jd_min_dep) :
					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_min_dep, group->system->cb);

	OSV osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
				   osv_from_elements(group->arr_body->orbit, jd_min_dep) :
				   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_min_dep, group->system->cb);
	Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, group->system->cb);
	double period_arr0 = calc_orbital_period(arr0);
	Orbit dep_orbit = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb);
	double period_dep = calc_orbital_period(dep_orbit);
	double syn_period = 1.0/fabs(1.0/period_dep - 1.0/period_arr0)/86400;
	double jd_dep_step = syn_period/10;
	double dt0, dt1;


	// double r0 = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb).a, r1 = arr0.a;
	// double r_ratio =  r1/r0;
	// Hohmann hohmann = calc_hohmann_transfer(r0, r1, group->system->cb);
	// double hohmann_dur = hohmann.dur/86400;
	// double min_duration = 0.4 * hohmann_dur;
	// double max_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_duration/hohmann_dur > 3) max_duration = hohmann_dur*3;
	// if(max_duration < max_dur) max_dur = max_duration;
	// if(min_duration > min_dur) min_dur = min_duration;

	// double min_dt = min_dur*86400;
	// double max_dt = max_dur*86400;

	DataArray1 *dep_points = data_array1_create();
	double jd_dep = jd_min_dep;
	while(jd_dep < jd_max_dep) {
		data_array1_append_new(dep_points, jd_dep);
		jd_dep += jd_dep_step;
	}
	data_array1_append_new(dep_points, jd_max_dep);


	DataArray1 *traversals = data_array1_create();
	double trav_search_date = jd_min_dep, prev_trav, next_trav;
	double offset_base = syn_period*0.005;
	do {
		get_prev_and_next_relative_plane_traversal(group->dep_body, group->arr_body, group->system, trav_search_date, &prev_trav, &next_trav);
		data_array1_append_new(traversals, prev_trav);
		data_array1_append_new(traversals, next_trav);
		for(int i = -3; i <= 3; i++) {
			jd_dep = prev_trav + offset_base*i;
			if(jd_dep >= jd_min_dep && jd_dep <= jd_max_dep)
				data_array1_insert_new(dep_points, jd_dep);
			jd_dep = next_trav + offset_base*i;
			if(jd_dep >= jd_min_dep && jd_dep <= jd_max_dep)
				data_array1_insert_new(dep_points, jd_dep);
		}
		trav_search_date += period_dep/86400;
	} while(next_trav < jd_max_dep);

	for(int i = 0; i < data_array1_size(dep_points); i++) {
		jd_dep = data_array1_get_data(dep_points)[i];

		dt0 = interpolate_from_sorted_data_array2(group->group_bdr.lower_bdrs[0], jd_dep) * 86400;
		dt1 = interpolate_from_sorted_data_array2(group->group_bdr.upper_bdrs[0], jd_dep) * 86400;

		// if(dt0 < min_dt) dt0 = min_dt;
		// if(dt1 < min_dt) continue;
		if(isnan(dt0) || isnan(dt1)) continue;

		DataArray2 *vinf_array = find_local_peak_array(jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, 1, 1);
		Vector2 vinf = data_array2_get_min(vinf_array);

		data_array2_insert_new(vinf_line, vec2(jd_dep, vinf.y));
		data_array2_free(vinf_array);
	}
	data_array1_clear(dep_points);

	Datetime date = {1959, 12, 10};
	double jd_date = convert_date_JD(date);

	for(int i = 0; i < data_array2_size(vinf_line)-1; i++) {
		Vector2 *data = data_array2_get_data(vinf_line);

		jd_dep = (data[i].x + data[i+1].x)/2;
		double vinf_guess = (data[i].y + data[i+1].y)/2;

		Datetime jd_dep_date = convert_JD_date(jd_dep, DATE_ISO);

		// if(jd_dep > jd_date) {
		// 	print_date(convert_JD_date(data[i].x, DATE_ISO), 0);
		// 	printf("  |  ");
		// 	print_date(convert_JD_date(jd_dep, DATE_ISO), 0);
		// 	printf("  |  ");
		// 	print_date(convert_JD_date(data[i+1].x, DATE_ISO), 1);
		// }

		dt0 = interpolate_from_sorted_data_array2(group->group_bdr.lower_bdrs[0], jd_dep) * 86400;
		dt1 = interpolate_from_sorted_data_array2(group->group_bdr.upper_bdrs[0], jd_dep) * 86400;

		// if(dt0 < min_dt) dt0 = min_dt;
		// if(dt1 < min_dt) continue;
		if(isnan(dt0) || isnan(dt1)) continue;

		DataArray2 *vinf_array = find_local_peak_array(jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, 1, 1);
		Vector2 vinf = data_array2_get_min(vinf_array);

		data_array2_insert_new(vinf_line, vec2(jd_dep, vinf.y));
		data_array2_free(vinf_array);

		if(data[i+1].x - data[i].x < syn_period*0.0001 || fabs(vinf.y-vinf_guess) < dv_tolerance) i++;
		else i--;
	}

	return vinf_line;
}

DataArray2 * get_vinf_array_for_departure(QuadList *quads_at_x, double jd_dep) {
	DataArray2 *dep_vinf_array = data_array2_create();

	for(int i = 0; i < quads_at_x->num; i++) {
		Quad *quad_at_x = quads_at_x->quad[i];
		double dur = quad_at_x->corner[QUAD_NW]->pos.y;
		double vinf = get_quad_interpolated_value(quad_at_x, vec2(jd_dep, dur), MESH_VAL_VINF);
		data_array2_insert_new(dep_vinf_array, vec2(dur, vinf));

		double jd_min = quad_at_x->corner[QUAD_NW]->pos.x;
		double jd_max = quad_at_x->corner[QUAD_NE]->pos.x;
		Quad *neighbour = jd_dep < (jd_max-jd_min)/2+jd_min ? quad_at_x->neighbours[QUAD_SSW] : quad_at_x->neighbours[QUAD_SSE];

		if(neighbour) continue;

		dur = quad_at_x->corner[QUAD_SW]->pos.y;
		vinf = get_quad_interpolated_value(quad_at_x, vec2(jd_dep, dur), MESH_VAL_VINF);
		data_array2_insert_new(dep_vinf_array, vec2(dur, vinf));
	}

	if(data_array2_size(dep_vinf_array) == 0) return dep_vinf_array;

	for(int i = 0; i < data_array2_size(dep_vinf_array)-1; i++) {
		if(data_array2_get_data(dep_vinf_array)[i].x == data_array2_get_data(dep_vinf_array)[i+1].x) {
			data_array2_remove_at_idx(dep_vinf_array, i+1);
		}
	}

	return dep_vinf_array;
}

DataArray2 * get_min_vinf_array_for_departure(QuadList *quads_at_x, double jd_dep, DataArray2 *min_vinf_array, double dv_tolerance, double min_dur, double max_dur) {
	// struct timeval start, end;
	// double elapsed_time;
	// gettimeofday(&start, NULL);
	int num_steps = 10;
	DataArray2 *dep_min_vinf_array = data_array2_create();

	// gettimeofday(&end, NULL);
	// elapsed_time = (double)(end.tv_sec - start.tv_sec)*1000000 + (double)(end.tv_usec - start.tv_usec);
	// printf("Find region: %10.3fµs\n", elapsed_time);
	// gettimeofday(&start, NULL);

	for(int j = 0; j < num_steps; j++) {
		double dur = (max_dur-min_dur)/(num_steps-1)*j + min_dur;
		double jd_tf = jd_dep + dur;
		double min_vinf = interpolate_from_sorted_data_array2(min_vinf_array, jd_tf);

		data_array2_insert_new(dep_min_vinf_array, vec2(dur, min_vinf-dv_tolerance));
	}


	// gettimeofday(&end, NULL);
	// elapsed_time = (double)(end.tv_sec - start.tv_sec)*1000000 + (double)(end.tv_usec - start.tv_usec);
	// printf("Steps: %10.3fµs\n", elapsed_time);

	return dep_min_vinf_array;
}

DataArray2 * calc_vinf_boundary_for_departure(Quad *quad, DataArray2 *min_vinf_array, double jd_dep, double dv_tolerance, double min_dur, double max_dur) {
	// struct timeval start, end;
	// double elapsed_time;
	// gettimeofday(&start, NULL);

	double min_dur_at_dep =  1e20;
	double max_dur_at_dep = -1e20;

	DataArray2 *x_line = data_array2_create();
	data_array2_append_new(x_line, vec2(jd_dep, min_dur_at_dep));
	data_array2_append_new(x_line, vec2(jd_dep, max_dur_at_dep));
	QuadList *quad_list = create_quad_list();


	// gettimeofday(&end, NULL);
	// elapsed_time = (double)(end.tv_sec - start.tv_sec)*1000000 + (double)(end.tv_usec - start.tv_usec);
	// printf("\n--\nInit: %10.9fµs\n", elapsed_time);
	// gettimeofday(&start, NULL);


	find_line_crossed_quads(quad, x_line, quad_list);

	// gettimeofday(&end, NULL);
	// elapsed_time = (double)(end.tv_sec - start.tv_sec)*1000000 + (double)(end.tv_usec - start.tv_usec);
	// printf("Find Quads: %10.9fµs\n", elapsed_time);
	// gettimeofday(&start, NULL);


	for(int i = 0; i < quad_list->num; i++) {
		Quad *quad_at_x = quad_list->quad[i];
		double dur0 = quad_at_x->corner[QUAD_SW]->pos.y;
		double dur1 = quad_at_x->corner[QUAD_NW]->pos.y;
		if(dur0 < min_dur_at_dep) min_dur_at_dep = dur0;
		if(dur1 > max_dur_at_dep) max_dur_at_dep = dur1;
	}


	// gettimeofday(&end, NULL);
	// elapsed_time = (double)(end.tv_sec - start.tv_sec)*1000000 + (double)(end.tv_usec - start.tv_usec);
	// printf("MinMax: %10.9fµs\n", elapsed_time);
	// gettimeofday(&start, NULL);


	DataArray2 *date_vinf_array = get_vinf_array_for_departure(quad_list, jd_dep);


	// gettimeofday(&end, NULL);
	// elapsed_time = (double)(end.tv_sec - start.tv_sec)*1000000 + (double)(end.tv_usec - start.tv_usec);
	// printf("Vinf array: %10.9fµs\n", elapsed_time);
	// gettimeofday(&start, NULL);


	DataArray2 *date_min_vinf_array = get_min_vinf_array_for_departure(quad_list, jd_dep, min_vinf_array, dv_tolerance, min_dur_at_dep, max_dur_at_dep);


	// gettimeofday(&end, NULL);
	// elapsed_time = (double)(end.tv_sec - start.tv_sec)*1000000 + (double)(end.tv_usec - start.tv_usec);
	// printf("Min Vinf array: %10.9fµs\n", elapsed_time);
	// gettimeofday(&start, NULL);

	DataArray2 *inters_points = get_line_intersections(date_vinf_array, date_min_vinf_array);

	if(data_array2_get_data(date_vinf_array)[0].y > data_array2_get_data(date_min_vinf_array)[0].y) {
		data_array2_insert_new(inters_points, vec2(min_dur-1, 0));
	}

	if(data_array2_size(inters_points)%2 != 0) {
		data_array2_insert_new(inters_points, vec2(max_dur+1, 0));
	}

	// gettimeofday(&end, NULL);
	// elapsed_time = (double)(end.tv_sec - start.tv_sec)*1000000 + (double)(end.tv_usec - start.tv_usec);
	// printf("Append: %10.9fµs\n", elapsed_time);
	// gettimeofday(&start, NULL);

	free_quad_list(quad_list);
	data_array2_free(date_vinf_array);
	data_array2_free(date_min_vinf_array);

	// gettimeofday(&end, NULL);
	// elapsed_time = (double)(end.tv_sec - start.tv_sec)*1000000 + (double)(end.tv_usec - start.tv_usec);
	// printf("Free: %10.9fµs\n", elapsed_time);

	return inters_points;
}

Boundary boundary_from_boundary_array(DataArray2 *boundary_array) {
	Boundary bdr = create_new_boundary();

	int group_params[128][3];
	int group_idx = -1;
	int idx = 0;
	Vector2 *data = data_array2_get_data(boundary_array);

	while(idx < data_array2_size(boundary_array)) {
		int num_groups = 0;
		double x = data[idx].x;
		for(int i = idx; i < data_array2_size(boundary_array); i+=2) {
			if(data[i].x == x) num_groups++;
		}

		if(group_idx < 0 || group_params[group_idx][0] != num_groups) {
			group_idx++;
			group_params[group_idx][0] = num_groups;
			group_params[group_idx][1] = 1;
			group_params[group_idx][2] = idx;
		} else {
			group_params[group_idx][1]++;
		}

		idx += num_groups*2;
	}

	for(int i = 0; i <= group_idx; i++) {
		int num_pairs = group_params[i][0];
		int num_x = group_params[i][1];
		int idx0 = group_params[i][2];
		for(int j = 0; j < num_pairs; j++) {
			DataArray2 *lower = data_array2_create();
			DataArray2 *upper = data_array2_create();

			if(i > 0 && num_pairs < group_params[i-1][0]) {
				int idx1 = group_params[i][2]-group_params[i-1][0]*2;
				int idx_l = idx0 + 2*j;
				int idx_u = idx_l + 1;
				int idx1_min_l = idx1;
				int idx1_min_u = idx1;
				double min_l = fabs(data[idx1].y - data[idx_l].y);
				double min_u = fabs(data[idx1].y - data[idx_u].y);

				for(int k = 0; k < group_params[i-1][0]*2; k++) {
					if(fabs(data[idx1+k].y - data[idx_l].y) < min_l) {
						min_l = fabs(data[idx1+k].y - data[idx_l].y);
						idx1_min_l = idx1+k;
					}
					if(fabs(data[idx1+k].y - data[idx_u].y) < min_u) {
						min_u = fabs(data[idx1+k].y - data[idx_u].y);
						idx1_min_u = idx1+k;
					}
				}

				data_array2_append_new(lower, data[idx1_min_l]);
				data_array2_append_new(upper, data[idx1_min_u]);
			}

			for(int k = 0; k < num_x; k++) {
				idx = idx0 + k*num_pairs*2 + j*2;
				data_array2_append_new(lower, data[idx  ]);
				data_array2_append_new(upper, data[idx+1]);
			}

			if(i < group_idx && num_pairs < group_params[i+1][0]) {
				int idx1 = group_params[i+1][2];
				int idx_l = idx;
				int idx_u = idx_l + 1;
				int idx1_min_l = idx1;
				int idx1_min_u = idx1;
				double min_l = fabs(data[idx1].y - data[idx_l].y);
				double min_u = fabs(data[idx1].y - data[idx_u].y);

				for(int k = 0; k < group_params[i+1][0]*2; k++) {
					if(fabs(data[idx1+k].y - data[idx_l].y) < min_l) {
						min_l = fabs(data[idx1+k].y - data[idx_l].y);
						idx1_min_l = idx1+k;
					}
					if(fabs(data[idx1+k].y - data[idx_u].y) < min_u) {
						min_u = fabs(data[idx1+k].y - data[idx_u].y);
						idx1_min_u = idx1+k;
					}
				}

				data_array2_append_new(lower, data[idx1_min_l]);
				data_array2_append_new(upper, data[idx1_min_u]);
			}
			append_to_boundary(&bdr, upper, lower);
		}
	}
	return bdr;
}

void calc_vinf_boundary(SegmentGroup *dep_group, SegmentGroup *group, Quad *quad, DataArray2 *min_vinf_array, double dv_tolerance) {
	DataArray2 *boundary_array = data_array2_create();

	Vector3 quad_min = get_quad_min_values(quad, -1);
	Vector3 quad_max = get_quad_max_values(quad, -1);

	double jd_min_dep = quad_min.x;
	double jd_max_dep = quad_max.x;
	double min_dur = quad_min.y;
	double max_dur = quad_max.y;

	OSV osv0 = dep_group->system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(dep_group->dep_body->orbit, jd_min_dep) :
					osv_from_ephem(dep_group->dep_body->ephem, dep_group->dep_body->num_ephems, jd_min_dep, dep_group->system->cb);

	OSV osv_arr0 = dep_group->system->prop_method == ORB_ELEMENTS ?
				   osv_from_elements(dep_group->arr_body->orbit, jd_min_dep) :
				   osv_from_ephem(dep_group->arr_body->ephem, dep_group->arr_body->num_ephems, jd_min_dep, dep_group->system->cb);
	Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, dep_group->system->cb);
	double period_arr0 = calc_orbital_period(arr0);
	Orbit dep_orbit = constr_orbit_from_osv(osv0.r, osv0.v, dep_group->system->cb);
	double period_dep = calc_orbital_period(dep_orbit);
	double syn_period = 1.0/fabs(1.0/period_dep - 1.0/period_arr0)/86400;
	// printf("%f  %f\n", syn_period, group->boundary_gradient);
	double jd_dep_step = syn_period/100;
	double min_jd_dep_step = syn_period*0.00001;
	double dt0, dt1;


	// double r0 = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb).a, r1 = arr0.a;
	// double r_ratio =  r1/r0;
	// Hohmann hohmann = calc_hohmann_transfer(r0, r1, group->system->cb);
	// double hohmann_dur = hohmann.dur/86400;
	// double min_duration = 0.4 * hohmann_dur;
	// double max_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_duration/hohmann_dur > 3) max_duration = hohmann_dur*3;
	// if(max_duration < max_dur) max_dur = max_duration;
	// if(min_duration > min_dur) min_dur = min_duration;

	double min_dt = min_dur*86400;
	double max_dt = max_dur*86400;

	DataArray1 *dep_points = data_array1_create();
	double jd_dep = jd_min_dep;
	while(jd_dep < jd_max_dep) {
		data_array1_append_new(dep_points, jd_dep);
		jd_dep += jd_dep_step;
	}
	data_array1_append_new(dep_points, jd_max_dep);


	DataArray1 *traversals = data_array1_create();
	double trav_search_date = jd_min_dep, prev_trav, next_trav;
	double offset_base = syn_period*0.0001;
	do {
		get_prev_and_next_relative_plane_traversal(dep_group->dep_body, dep_group->arr_body, dep_group->system, trav_search_date, &prev_trav, &next_trav);
		data_array1_append_new(traversals, prev_trav);
		data_array1_append_new(traversals, next_trav);
		for(int i = -10; i <= 10; i++) {
			jd_dep = prev_trav + offset_base*i;
			if(jd_dep >= jd_min_dep && jd_dep <= jd_max_dep)
				data_array1_insert_new(dep_points, jd_dep);
			jd_dep = next_trav + offset_base*i;
			if(jd_dep >= jd_min_dep && jd_dep <= jd_max_dep)
				data_array1_insert_new(dep_points, jd_dep);
		}
		trav_search_date += period_dep/86400;
	} while(next_trav < jd_max_dep);

	DataArray1 *analyzed_dep_dates = data_array1_create();
	while(data_array1_size(dep_points) > 0) {
		for(int i = 0; i < data_array1_size(dep_points); i++) {
			jd_dep = data_array1_get_data(dep_points)[i];
			bool already_analyzed = false;
			for(int j = 0; j < data_array1_size(analyzed_dep_dates); j++) {
				if(data_array1_get_data(analyzed_dep_dates)[j] == jd_dep) {
					already_analyzed = true;
					break;
				}
			}
			if(already_analyzed) continue;
			data_array1_insert_new(analyzed_dep_dates, jd_dep);

			DataArray2 *inters_points = calc_vinf_boundary_for_departure(quad, min_vinf_array, jd_dep, dv_tolerance, min_dur, max_dur);
			for(int j = 0; j < data_array2_size(inters_points); j++) {
				data_array2_insert_new(boundary_array, vec2(jd_dep, data_array2_get_data(inters_points)[j].x));
			}

			data_array2_free(inters_points);
		}
		// printf("%lu\n", data_array2_size(boundary_array));
		data_array1_clear(dep_points);
		jd_dep_step /= 2;

		free_boundary(&group->vinf_bdr);
		group->vinf_bdr = boundary_from_boundary_array(boundary_array);
		// printf("%lu\n", group->vinf_bdr.num);
		//
		// print_data_array2(boundary_array, "dep", "dur");
		// printf("--- %lu\n", data_array2_size(boundary_array));

		for(int i = 0; i < group->vinf_bdr.num; i++) {
			DataArray2 *arr_l = group->vinf_bdr.lower_bdrs[i];
			DataArray2 *arr_u = group->vinf_bdr.upper_bdrs[i];
			size_t num_deps = data_array2_size(arr_l);

			if(num_deps == 1) {
				if(!is_point_inside_boundary(data_array2_get_data(arr_l)[0], dep_group->dv_bdr) && !is_point_inside_boundary(data_array2_get_data(arr_u)[0], dep_group->dv_bdr)) {
					data_array2_remove_by_value(boundary_array, data_array2_get_data(arr_l)[0]);
					data_array2_remove_by_value(boundary_array, data_array2_get_data(arr_u)[0]);
				}
				continue;
			}

			bool *bdr_mask = malloc(num_deps * sizeof(bool));
			int *rm_idx = malloc(num_deps * sizeof(int));
			int num_rm = 0;

			for(int j = 0; j < num_deps; j++) {
				if(is_point_inside_boundary(data_array2_get_data(arr_l)[j], dep_group->dv_bdr) || is_point_inside_boundary(data_array2_get_data(arr_u)[j], dep_group->dv_bdr)) {
					bdr_mask[j] = true;
				} else if(is_line_crossing_boundary(data_array2_get_data(arr_l)[j], data_array2_get_data(arr_u)[j], dep_group->dv_bdr)) {
					bdr_mask[j] = true;
				} else {
					bdr_mask[j] = false;
				}
			}

			for(int j = 0; j < num_deps; j++) {
				if(bdr_mask[j]) continue;
				if(j > 0 && bdr_mask[j-1]) continue;
				if(j < num_deps-1 && bdr_mask[j+1]) continue;
				rm_idx[num_rm++] = j;
			}

			for(int j = num_rm-1; j >= 0; j--) {
				data_array2_remove_by_value(boundary_array, data_array2_get_data(arr_l)[rm_idx[j]]);
				data_array2_remove_by_value(boundary_array, data_array2_get_data(arr_u)[rm_idx[j]]);
			}
		}
		// print_data_array2(boundary_array, "dep", "dur");
		// printf("--- %lu\n", data_array2_size(boundary_array));
		free_boundary(&group->vinf_bdr);
		group->vinf_bdr = boundary_from_boundary_array(boundary_array);

		DataArray1 *dep_temp = data_array1_create();
		for(int i = 0; i < group->vinf_bdr.num*2; i++) {
			int idx = i/2;

			DataArray2 *arr = i%2 == 0 ? group->vinf_bdr.lower_bdrs[idx] : group->vinf_bdr.upper_bdrs[idx];
			size_t num_deps = data_array2_size(arr);
			Vector2 *transf_arr = malloc(num_deps * sizeof(Vector2));

			for(int j = 0; j < num_deps; j++) {
				transf_arr[j].x = data_array2_get_data(arr)[j].x;
				transf_arr[j].y = data_array2_get_data(arr)[j].y/group->boundary_gradient;
			}

			if(jd_dep_step > min_jd_dep_step && is_point_inside_boundary(data_array2_get_data(arr)[0], dep_group->dv_bdr)) {
				data_array1_insert_new(dep_points, transf_arr[0].x-jd_dep_step);
				data_array1_insert_new(dep_temp, transf_arr[0].x-jd_dep_step - jd_min_dep);
			}
			if(jd_dep_step > min_jd_dep_step && is_point_inside_boundary(data_array2_get_data(arr)[num_deps-1], dep_group->dv_bdr)) {
				data_array1_insert_new(dep_points, transf_arr[num_deps-1].x+jd_dep_step);
				data_array1_insert_new(dep_temp, transf_arr[num_deps-1].x+jd_dep_step - jd_min_dep);
			}

			for(int j = 2; j < num_deps; j++) {
				Vector2 v0 = transf_arr[j-2];
				Vector2 v1 = transf_arr[j-1];
				Vector2 v2 = transf_arr[j  ];

				// if(v1.x == v0.x || v1.x == v2.x) {
				// 	printf("%f   %f   %f\n", v0.x-jd_min_dep, v1.x-jd_min_dep, v2.x-jd_min_dep);
				// }

				double m0 = (v1.y - v0.y)/(v1.x - v0.x);
				double m1 = (v2.y - v1.y)/(v2.x - v1.x);

				double angle0 = atan(m0);
				double angle1 = atan(m1);

				double da = fabs(angle1 - angle0);

				if(da > deg2rad(5.0)) {
					// printf("%f°    %f°  (%f)  %f°   (%f)\n", rad2deg(fabs(angle0-angle1)), rad2deg(angle0), m0, rad2deg(angle1), m1);
					if(fabs(v0.x-v1.x) > min_jd_dep_step) {
						data_array1_append_new(dep_points, (v0.x+v1.x)/2);
						data_array1_append_new(dep_temp, (v0.x+v1.x)/2 - jd_min_dep);
					}
					if(fabs(v1.x-v2.x) > min_jd_dep_step) {
						data_array1_append_new(dep_points, (v1.x+v2.x)/2);
						data_array1_append_new(dep_temp, (v1.x+v2.x)/2 - jd_min_dep);
					}
					i += 3 + (i%2==0);
				}

				if(isnan(da)) {
					printf("da is nan\n");
				}
			}
		}

		for(int i = 0; i < data_array1_size(dep_points); i++) {
			double *d = data_array1_get_data(dep_points);

			if(d[i] < jd_min_dep || d[i] > jd_max_dep) {
				data_array1_remove_at_idx(dep_points, i);
				data_array1_remove_at_idx(dep_temp, i);
				i--;
			}

			if(i > 0 && d[i] == d[i-1]) {
				data_array1_remove_at_idx(dep_points, i);
				data_array1_remove_at_idx(dep_temp, i);
				i--;
			}
		}

		// print_data_array1(dep_temp, "dep");
		data_array1_free(dep_temp);
	}

	data_array1_free(analyzed_dep_dates);
	data_array2_free(boundary_array);
}



















// ###########################################################################
// OLD
// ###########################################################################














// void get_upper_and_lower_boundary_at_jd_dep(SegmentGroup *group, double jd_dep, double *lower_boundary, double *upper_boundary) {
// 	double next_opposition_dt, next_conjunction_dt, opp_guess, conj_guess;
// 	OSV osv0 = group->system->prop_method == ORB_ELEMENTS ?
// 				osv_from_elements(group->dep_body->orbit, jd_dep) :
// 				osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);
//
// 	OSV osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
// 				   osv_from_elements(group->arr_body->orbit, jd_dep) :
// 				   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_dep, group->system->cb);
// 	double period_arr0 = calc_orbital_period(constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, group->system->cb));
// 	calc_time_to_next_conjunction_and_opposition(osv0.r, osv_arr0, group->system->cb, &next_conjunction_dt, &next_opposition_dt);
//
// 	if(group->top_boundary_type == DEPARTURE_GROUP_BOUNDARY_TOP_OPP) {
// 		opp_guess = interpolate_from_sorted_data_array2(group->group_bdr.upper_bdrs[0], jd_dep)*86400;
// 		conj_guess = interpolate_from_sorted_data_array2(group->group_bdr.lower_bdrs[0], jd_dep)*86400;
// 	} else {
// 		conj_guess = interpolate_from_sorted_data_array2(group->group_bdr.upper_bdrs[0], jd_dep)*86400;
// 		opp_guess = interpolate_from_sorted_data_array2(group->group_bdr.lower_bdrs[0], jd_dep)*86400;
// 	}
//
// 	while(opp_guess-next_opposition_dt   >  0.5 * period_arr0) next_opposition_dt  += period_arr0;
// 	while(opp_guess-next_opposition_dt   < -0.5 * period_arr0) next_opposition_dt  -= period_arr0;
// 	while(conj_guess-next_conjunction_dt >  0.5 * period_arr0) next_conjunction_dt += period_arr0;
// 	while(conj_guess-next_conjunction_dt < -0.5 * period_arr0) next_conjunction_dt -= period_arr0;
//
// 	if(next_conjunction_dt < next_opposition_dt) {
// 		*lower_boundary = next_conjunction_dt;
// 		*upper_boundary = next_opposition_dt;
// 	} else {
// 		*lower_boundary = next_opposition_dt;
// 		*upper_boundary = next_conjunction_dt;
// 	}
// }

// void calc_bounded_porkchop_line(struct ItinStep *departure_step, Body *arr_body, CelestSystem *system, DataArray1 *dur_array, double min_dt, double max_dt, double dep_periapsis, double max_depdv, double dv_tolerance) {
// 	Body *dep_body = departure_step->body;
// 	double jd_dep = departure_step->date;
// 	OSV osv0 = system->prop_method == ORB_ELEMENTS ?
// 					osv_from_elements(dep_body->orbit, jd_dep) :
// 					osv_from_ephem(dep_body->ephem, dep_body->num_ephems, jd_dep, system->cb);
//
// 	double dt = min_dt;
//
// 	DataArray2 *data_dep = data_array2_create();
// 	DataArray2 *data_arr = data_array2_create();
//
// 	struct ItinStep *curr_step = departure_step;
// 	curr_step->r = osv0.r;
// 	curr_step->v_body = osv0.v;
// 	curr_step->v_dep = vec3(0, 0, 0);
// 	curr_step->v_arr = vec3(0, 0, 0);
// 	curr_step->num_next_nodes = 0;
// 	curr_step->prev = NULL;
// 	curr_step->next = (struct ItinStep **) malloc(1000 * sizeof(struct ItinStep *));
// 	int counter = 0;
//
// 	for(int j = 0; j < 1000; j++) {
// 		// printf("%f  %f  %f  %f  %f\n", min_dt, max_dt, dt0, dt1, dt);
//
// 		double jd_arr = jd_dep + dt / 86400;
//
// 		OSV osv_arr = system->prop_method == ORB_ELEMENTS ?
// 					osv_from_elements(arr_body->orbit, jd_arr) :
// 					osv_from_ephem(arr_body->ephem, arr_body->num_ephems, jd_arr, system->cb);
//
// 		Lambert3 tf = calc_lambert3(osv0.r, osv_arr.r, (jd_arr - jd_dep) * 86400, system->cb);
//
// 		double vinf_dep = fabs(mag_vec3(subtract_vec3(tf.v0, osv0.v)));
// 		double dv_dep = dv_circ(dep_body,alt2radius(dep_body, dep_periapsis),vinf_dep);
// 		double vinf_arr = fabs(mag_vec3(subtract_vec3(tf.v1, osv_arr.v)));
//
// 		// j != 3 to skip initial accuracy point for line propagation
// 		if(dv_dep <= max_depdv && j != 3) {
// 			curr_step = get_first(curr_step);
// 			// sort chronologically
// 			int insert_index = counter;
// 			while(insert_index > 0) {
// 				if(curr_step->next[insert_index-1]->date < jd_arr) break;
// 				insert_index--;
// 			}
// 			if(insert_index != counter) {
// 				memmove(&curr_step->next[insert_index+1],
// 					&curr_step->next[insert_index],
// 					(counter+2 - insert_index) * sizeof(*curr_step->next));
// 			}
//
// 			curr_step->next[insert_index] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
// 			curr_step->next[insert_index]->prev = curr_step;
// 			curr_step->next[insert_index]->next = NULL;
// 			curr_step = curr_step->next[insert_index];
//
// 			curr_step->body = arr_body;
// 			curr_step->date = jd_arr;
// 			curr_step->r = osv_arr.r;
// 			curr_step->v_dep = tf.v0;
// 			curr_step->v_arr = tf.v1;
// 			curr_step->v_body = osv_arr.v;
// 			curr_step->num_next_nodes = 0;
// 			curr_step->prev->num_next_nodes++;
// 			counter++;
//
// 			if(dur_array) data_array1_insert_new(dur_array, dt/86400);
// 		}
//
// 		data_array2_insert_new(data_dep, dt/86400, dv_dep);
// 		data_array2_insert_new(data_arr, dt/86400, vinf_arr);
//
// 		if(dt == min_dt) dt = max_dt;
// 		else if(dt == max_dt) dt = ( dt + data_array2_get_data(data_dep)[0].x*86400 ) / 2;
// 		else {
// 			double next_dep_x = calc_next_x_wrt_smoothness(data_dep, 0, dv_tolerance)*86400;
// 			double next_arr_x = calc_next_x_wrt_smoothness(data_arr, 0, dv_tolerance)*86400;
// 			if(isnan(next_dep_x) && isnan(next_arr_x)) break;
// 			if(!isnan(next_dep_x) && isnan(next_arr_x) || next_dep_x < next_arr_x) dt = next_dep_x;
// 			else dt = next_arr_x;
// 		}
// 	}
// 	data_array2_free(data_dep);
// 	data_array2_free(data_arr);
// }

// void calc_coarse_bounded_porkchop_line(struct ItinStep *departure_step, Body *arr_body, CelestSystem *system, double min_dt, double max_dt, double dt_step, double dep_periapsis, double max_depdv, double dv_tolerance) {
// 	Body *dep_body = departure_step->body;
// 	double jd_dep = departure_step->date;
// 	OSV osv0 = system->prop_method == ORB_ELEMENTS ?
// 				osv_from_elements(dep_body->orbit, jd_dep) :
// 				osv_from_ephem(dep_body->ephem, dep_body->num_ephems, jd_dep, system->cb);
//
// 	int num_steps = (int) (max_dt-min_dt)/dt_step + 2;
//
// 	struct ItinStep *curr_step = departure_step;
// 	curr_step->r = osv0.r;
// 	curr_step->v_body = osv0.v;
// 	curr_step->v_dep = vec3(0, 0, 0);
// 	curr_step->v_arr = vec3(0, 0, 0);
// 	curr_step->num_next_nodes = 0;
// 	curr_step->prev = NULL;
// 	curr_step->next = (struct ItinStep **) malloc(num_steps * sizeof(struct ItinStep *));
//
// 	for(int i = 0; i < num_steps; i++) {
// 		double dt = min_dt + (max_dt-min_dt) * ((double) i/(num_steps-1));
// 		double jd_arr = jd_dep + dt / 86400;
//
// 		OSV osv_arr = system->prop_method == ORB_ELEMENTS ?
// 					osv_from_elements(arr_body->orbit, jd_arr) :
// 					osv_from_ephem(arr_body->ephem, arr_body->num_ephems, jd_arr, system->cb);
//
// 		Lambert3 tf = calc_lambert3(osv0.r, osv_arr.r, (jd_arr - jd_dep) * 86400, system->cb);
//
// 		curr_step = get_first(curr_step);
// 		curr_step->next[i] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
// 		curr_step->next[i]->prev = curr_step;
// 		curr_step->next[i]->next = NULL;
// 		curr_step = curr_step->next[i];
//
// 		curr_step->body = arr_body;
// 		curr_step->date = jd_arr;
// 		curr_step->r = osv_arr.r;
// 		curr_step->v_dep = tf.v0;
// 		curr_step->v_arr = tf.v1;
// 		curr_step->v_body = osv_arr.v;
// 		curr_step->num_next_nodes = 0;
// 		curr_step->prev->num_next_nodes++;
// 	}
// }

// void calc_coarse_group_porkchop(SegmentGroup *group, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, int num_duration_steps, double dep_periapsis, double max_depdv, double dv_tolerance) {
// 	group->segment_steps = malloc(10000 * sizeof(struct ItinStep*));
//
// 	OSV osv0 = group->system->prop_method == ORB_ELEMENTS ?
// 					osv_from_elements(group->dep_body->orbit, jd_min_dep) :
// 					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_min_dep, group->system->cb);
//
// 	OSV osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
// 				   osv_from_elements(group->arr_body->orbit, jd_min_dep) :
// 				   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_min_dep, group->system->cb);
// 	Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, group->system->cb);
// 	double dt0, dt1;
//
//
// 	double r0 = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb).a, r1 = arr0.a;
// 	double r_ratio =  r1/r0;
// 	Hohmann hohmann = calc_hohmann_transfer(r0, r1, group->system->cb);
// 	double hohmann_dur = hohmann.dur/86400;
// 	double min_duration = 0.4 * hohmann_dur;
// 	double max_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_duration/hohmann_dur > 3) max_duration = hohmann_dur*3;
// 	if(max_duration < max_dur) max_dur = max_duration;
// 	if(min_duration > min_dur) min_dur = min_duration;
//
// 	double min_dt = min_dur*86400;
// 	double max_dt = max_dur*86400;
// 	double jd_dep_step = 5;
// 	double jd_dep = jd_min_dep;
//
// 	while(jd_dep < jd_max_dep) {
// 		// print_date(convert_JD_date(jd_min_dep, DATE_ISO), 0);
// 		// printf("\t");
// 		// print_date(convert_JD_date(jd_dep, DATE_ISO), 0);
// 		// printf("\t");
// 		// print_date(convert_JD_date(jd_max_dep, DATE_ISO), 1);
// 		get_upper_and_lower_boundary_at_jd_dep(group, jd_dep, &dt0, &dt1);
// 		double dt_step = (dt1-dt0) / num_duration_steps;
//
// 		if(dt0 > max_dt || dt1 < min_dt) {jd_dep += jd_dep_step; continue;}
//
// 		if(dt0 < min_dur*86400) dt0 = min_dur*86400;
// 		if(dt1 > max_dur*86400) dt1 = max_dur*86400;
//
// 		group->segment_steps[group->num_steps] = malloc(sizeof(struct ItinStep));
// 		group->segment_steps[group->num_steps]->body = group->dep_body;
// 		group->segment_steps[group->num_steps]->date = jd_dep;
// 		calc_coarse_bounded_porkchop_line(group->segment_steps[group->num_steps], group->arr_body, group->system, dt0, dt1, dt_step, dep_periapsis, max_depdv, dv_tolerance);
// 		group->num_steps++;
// 		if(jd_dep >= jd_max_dep) break;
// 		jd_dep += dt_step/fabs(group->boundary_gradient)/86400;
// 		if(jd_dep >= jd_max_dep) jd_dep = jd_max_dep;
// 	}
// }

// MeshPoint2 * create_mesh_point_for_porkchop_mesh(Body *dep_body, Body *arr_body, CelestSystem *system, double jd_dep, double dur) {
// 	double jd_arr = jd_dep + dur;
// 	OSV osv0 = system->prop_method == ORB_ELEMENTS ?
// 				osv_from_elements(dep_body->orbit, jd_dep) :
// 				osv_from_ephem(dep_body->ephem, dep_body->num_ephems, jd_dep, system->cb);
// 	struct ItinStep *curr_step = malloc(sizeof(struct ItinStep));
// 	curr_step->body = dep_body;
// 	curr_step->date = jd_dep;
// 	curr_step->r = osv0.r;
// 	curr_step->v_body = osv0.v;
// 	curr_step->v_dep = vec3(0, 0, 0);
// 	curr_step->v_arr = vec3(0, 0, 0);
// 	curr_step->num_next_nodes = 0;
// 	curr_step->prev = NULL;
// 	curr_step->next = (struct ItinStep **) malloc(sizeof(struct ItinStep *));
//
// 	OSV osv_arr = system->prop_method == ORB_ELEMENTS ?
// 				osv_from_elements(arr_body->orbit, jd_arr) :
// 				osv_from_ephem(arr_body->ephem, arr_body->num_ephems, jd_arr, system->cb);
//
// 	Lambert3 tf = calc_lambert3(osv0.r, osv_arr.r, (jd_arr - jd_dep) * 86400, system->cb);
//
//
// 	double *point_vals = malloc(NUM_PORKCHOP_MESH_VALUE_TYPES * sizeof(double));
// 	Vector3 vinf_dep = subtract_vec3(tf.v0, osv0.v);
// 	Vector3 vinf_arr = subtract_vec3(tf.v1, osv_arr.v);
// 	point_vals[MESH_VAL_DATE] = jd_arr;
// 	point_vals[MESH_VAL_DEPX] = vinf_dep.x;
// 	point_vals[MESH_VAL_DEPY] = vinf_dep.y;
// 	point_vals[MESH_VAL_DEPZ] = vinf_dep.z;
// 	point_vals[MESH_VAL_BODY_RX] = osv_arr.r.x;
// 	point_vals[MESH_VAL_BODY_RY] = osv_arr.r.y;
// 	point_vals[MESH_VAL_BODY_RZ] = osv_arr.r.z;
// 	point_vals[MESH_VAL_BODY_VX] = osv_arr.v.x;
// 	point_vals[MESH_VAL_BODY_VY] = osv_arr.v.y;
// 	point_vals[MESH_VAL_BODY_VZ] = osv_arr.v.z;
// 	point_vals[MESH_VAL_ARRX] = vinf_arr.x;
// 	point_vals[MESH_VAL_ARRY] = vinf_arr.y;
// 	point_vals[MESH_VAL_ARRZ] = vinf_arr.z;
// 	point_vals[MESH_VAL_VINF] = mag_vec3(vinf_arr);
// 	point_vals[MESH_VAL_RPE] = 1e9;
// 	MeshPoint2 *new_point = create_mesh_point(vec2(jd_dep, dur), point_vals, NUM_PORKCHOP_MESH_VALUE_TYPES);
//
// 	return new_point;
// }

// MeshTriangleBoundaryCondition get_triangle_dep_dv_boundary_condition(MeshTriangle2 *triangle, Body *dep_body, double dep_periapsis, double max_depdv, double dv_tolerance) {
// 	bool inside[3] = {false, false, false};
// 	for(int i = 0; i < 3; i++) {
// 		double vinf = triangle->points[i]->val[MESH_VAL_VINF];
// 		double depdv = dv_circ(dep_body, dep_body->radius+dep_periapsis, vinf);
// 		if(depdv < max_depdv+dv_tolerance) inside[i] = true;
// 	}
// 	if( inside[0] &&  inside[1] &&  inside[2]) return TRIANGLE_INSIDE_BOUNDARY;
// 	if(!inside[0] && !inside[1] && !inside[2]) return TRIANGLE_OUTSIDE_BOUNDARY;
// 	return TRIANGLE_CROSSING_BOUNDARY;
// }

// MeshTriangleBoundaryCondition get_triangle_group_boundary_condition(MeshTriangle2 *triangle, SegmentGroup *group) {
// 	bool inside[3] = {false, false, false};
// 	for(int i = 0; i < 3; i++) {
// 		struct ItinStep *ptr = triangle->points[i]->old_data;
// 		double jd_dep = get_first(ptr)->date;
// 		double dur = ptr->date - jd_dep;
// 		double upper_dur_boundary = interpolate_from_sorted_data_array2(group->upper_boundary, jd_dep);
// 		double lower_dur_boundary = interpolate_from_sorted_data_array2(group->lower_boundary, jd_dep);
// 		if(dur <= upper_dur_boundary && dur >= lower_dur_boundary) inside[i] = true;
// 	}
// 	if( inside[0] &&  inside[1] &&  inside[2]) return TRIANGLE_INSIDE_BOUNDARY;
// 	if(!inside[0] && !inside[1] && !inside[2]) return TRIANGLE_OUTSIDE_BOUNDARY;
// 	return TRIANGLE_CROSSING_BOUNDARY;
// }

// void split_mesh_triangle(Mesh2 *mesh, MeshTriangle2 *triangle, SegmentGroup *group) {
// 	int side_idx = 0;
// 	double max_side_lengths_sq = 0;
//
// 	for(int i = 0; i < 3; i++) {
// 		double side_length_sq = sq_mag_vec2(subtract_vec2(triangle->points[i]->pos, triangle->points[(i+1)%3]->pos));
// 		if(side_length_sq > max_side_lengths_sq) { max_side_lengths_sq = side_length_sq; side_idx = i; }
// 	}
//
// 	MeshTriangle2 *adj_triangle = triangle->adj_triangles[side_idx];
// 	if(adj_triangle && adj_triangle->rf_level == adj_triangle->target_rf_level) return;
//
// 	MeshPoint2 *t0p0 = triangle->points[side_idx];
// 	MeshPoint2 *t0p1 = triangle->points[(side_idx+1)%3];
// 	MeshPoint2 *t0p_opp = triangle->points[(side_idx+2)%3];
//
// 	Vector2 new_point_pos = scale_vec2(add_vec2(t0p0->pos, t0p1->pos), 0.5);
// 	MeshPoint2 *new_point = create_mesh_point_for_porkchop_mesh(group->dep_body, group->arr_body, group->system, new_point_pos.x, new_point_pos.y);
// 	add_point_to_mesh(mesh, new_point);
//
// 	remove_triangle_from_mesh(mesh, triangle, false);
// 	add_triangle_to_mesh(mesh, create_triangle_from_three_points_with_rf_level(new_point, t0p_opp, t0p0, triangle->rf_level+1, triangle->target_rf_level));
// 	add_triangle_to_mesh(mesh, create_triangle_from_three_points_with_rf_level(new_point, t0p_opp, t0p1, triangle->rf_level+1, triangle->target_rf_level));
//
// 	if(!adj_triangle) return;
// 	int adj_tri_opp_point_idx = 0;
// 	for(int i = 0; i < 3; i++) {
// 		if(adj_triangle->points[i] != t0p0 && adj_triangle->points[i] != t0p1) {
// 			adj_tri_opp_point_idx = i;
// 			break;
// 		}
// 	}
//
// 	MeshPoint2 *t1p0 = adj_triangle->points[(adj_tri_opp_point_idx+1)%3];
// 	MeshPoint2 *t1p1 = adj_triangle->points[(adj_tri_opp_point_idx+2)%3];
// 	MeshPoint2 *t1p_opp = adj_triangle->points[adj_tri_opp_point_idx];
//
// 	remove_triangle_from_mesh(mesh, adj_triangle, false);
// 	add_triangle_to_mesh(mesh, create_triangle_from_three_points_with_rf_level(new_point, t1p_opp, t1p0, adj_triangle->rf_level, adj_triangle->target_rf_level));
// 	add_triangle_to_mesh(mesh, create_triangle_from_three_points_with_rf_level(new_point, t1p_opp, t1p1, adj_triangle->rf_level, adj_triangle->target_rf_level));
// }

// int max_num_refines = 0;
// int num_refines = 0;

// void refine_porkchop_mesh_box(SegmentGroup *group, MeshBox2 *box, double dep_periapsis, double max_depdv, double dv_tolerance) {
// 	if(box->type == MESHBOX_SUBBOXES) {
// 		int num_subboxes = (int) box->subboxes.num;
// 		for(int i = 0; i < box->subboxes.num; i++) {
// 			refine_porkchop_mesh_box(group, box->subboxes.boxes[i], dep_periapsis, max_depdv, dv_tolerance);
// 			if(box->subboxes.num != num_subboxes) { i--; num_subboxes = (int) box->subboxes.num; }
// 		}
// 	} else if(box->type == MESHBOX_TRIANGLES) {
// 		for(int i = 0; i < box->tri.num; i++) {
// 			MeshTriangle2 *triangle = box->tri.triangles[i];
// 			MeshTriangleBoundaryCondition bc_depdv = get_triangle_dep_dv_boundary_condition(triangle, group->dep_body, dep_periapsis, max_depdv, dv_tolerance);
// 			MeshTriangleBoundaryCondition bc_group_boundary = false;//get_triangle_group_boundary_condition(triangle, group);
//
// 			if(bc_depdv == TRIANGLE_OUTSIDE_BOUNDARY || bc_group_boundary == TRIANGLE_OUTSIDE_BOUNDARY) {
// 				set_mesh_tri_flag(triangle, TRI_FLAG_INACTIVE);
// 			} else if(bc_depdv == TRIANGLE_CROSSING_BOUNDARY || bc_group_boundary == TRIANGLE_CROSSING_BOUNDARY) {
// 				set_mesh_tri_flag(triangle, TRI_FLAG_WANTS_REFINEMENT);
// 				triangle->target_rf_level = triangle->rf_level+1;
// 			}
// 		}
// 	}
// }

// void refine_porkchop_mesh(SegmentGroup *group, double dep_periapsis, double max_depdv, double dv_tolerance) {
// 	num_refines = 0;
// 	Mesh2 *mesh = group->mesh;
// 	// refine_porkchop_mesh_box(group, mesh->mesh_box, dep_periapsis, max_depdv, dv_tolerance);
// 	int max_level = 0;
//
// 	for(int c = 0; c < max_num_refines; c++) {
// 		for(int i = 0; i < mesh->num_triangles; i++) {
// 			MeshTriangle2 *triangle = mesh->triangles[i];
// 			MeshTriangleBoundaryCondition bc_depdv = get_triangle_dep_dv_boundary_condition(triangle, group->dep_body, dep_periapsis, max_depdv, dv_tolerance);
// 			MeshTriangleBoundaryCondition bc_group_boundary = false;//get_triangle_group_boundary_condition(triangle, group);
//
// 			remove_mesh_tri_flag(triangle, TRI_FLAG_WANTS_REFINEMENT);
// 			triangle->target_rf_level = triangle->rf_level;
//
// 			if(bc_depdv == TRIANGLE_OUTSIDE_BOUNDARY || bc_group_boundary == TRIANGLE_OUTSIDE_BOUNDARY) {
// 				set_mesh_tri_flag(triangle, TRI_FLAG_INACTIVE);
// 			} else if(bc_depdv == TRIANGLE_CROSSING_BOUNDARY || bc_group_boundary == TRIANGLE_CROSSING_BOUNDARY) {
// 				set_mesh_tri_flag(triangle, TRI_FLAG_WANTS_REFINEMENT);
// 				triangle->target_rf_level = triangle->rf_level+1;
// 				if(triangle->rf_level > max_level) { max_level = triangle->rf_level; }
// 			}
// 		}
//
// 		bool has_changed = false;
// 		do {
// 			has_changed = false;
// 			for(int i = 0; i < mesh->num_triangles; i++) {
// 				MeshTriangle2 *triangle = mesh->triangles[i];
// 				for(int j = 0; j < 3; j++) {
// 					if(triangle->adj_triangles[j]) {
// 						if(triangle->adj_triangles[j]->target_rf_level-1 > triangle->target_rf_level) {
// 							triangle->target_rf_level = triangle->adj_triangles[j]->target_rf_level-1;
// 							set_mesh_tri_flag(triangle, TRI_FLAG_WANTS_REFINEMENT);
// 							has_changed = true;
// 						}
// 					}
// 				}
// 			}
// 		} while(has_changed);
//
// 		for(int level = 0; level <= max_level; level++) {
// 			for(int i = 0; i < mesh->num_triangles; i++) {
// 				MeshTriangle2 *triangle = mesh->triangles[i];
// 				if(is_mesh_tri_flag(triangle, TRI_FLAG_WANTS_REFINEMENT) && triangle->rf_level <= level) split_mesh_triangle(mesh, triangle, group);
// 				if(triangle->rf_level == triangle->target_rf_level) remove_mesh_tri_flag(triangle, TRI_FLAG_WANTS_REFINEMENT);
// 			}
// 		}
// 	}
//
// 	for(int i = 0; i < mesh->num_triangles; i++) {
// 		MeshTriangle2 *triangle = mesh->triangles[i];
// 		MeshTriangleBoundaryCondition bc_depdv = get_triangle_dep_dv_boundary_condition(triangle, group->dep_body, dep_periapsis, max_depdv, dv_tolerance);
// 		MeshTriangleBoundaryCondition bc_group_boundary = false;//get_triangle_group_boundary_condition(triangle, group);
//
// 		if(bc_depdv == TRIANGLE_OUTSIDE_BOUNDARY || bc_group_boundary == TRIANGLE_OUTSIDE_BOUNDARY) {
// 			set_mesh_tri_flag(triangle, TRI_FLAG_INACTIVE);
// 			remove_triangle_from_mesh(mesh, triangle, true);
// 			i--;
// 		} else if(bc_depdv == TRIANGLE_CROSSING_BOUNDARY || bc_group_boundary == TRIANGLE_CROSSING_BOUNDARY) {
// 			set_mesh_tri_flag(triangle, TRI_FLAG_WANTS_REFINEMENT);
// 			triangle->target_rf_level = triangle->rf_level+1;
// 		}
// 	}
// 	max_num_refines++;
// }

// void update_mesh_triangle_status(SegmentGroup *group, double dv_tolerance) {
// 	Mesh2 *mesh = group->mesh;
// 	for(int i = 0; i < mesh->num_triangles; i++) {
// 		MeshTriangle2 *triangle = mesh->triangles[i];
//
// 		Vector2 tri_centroid = get_triangle_centroid(triangle);
// 		Vector3 p[3];
// 		for(int k = 0; k < 3; k++) {
// 			p[k].x = triangle->points[k]->pos.x;
// 			p[k].y = triangle->points[k]->pos.y;
// 			p[k].z = triangle->points[k]->val[MESH_VAL_VINF];
// 		}
// 		double center_val = get_triangle_interpolated_value(p[0], p[1], p[2], tri_centroid);
//
// 		for(int j = 0; j < 3; j++) {
// 			MeshTriangle2 *adj_triangle = triangle->adj_triangles[j];
// 			if(!adj_triangle) continue;
// 			for(int k = 0; k < 3; k++) {
// 				p[k].x = adj_triangle->points[k]->pos.x;
// 				p[k].y = adj_triangle->points[k]->pos.y;
// 				p[k].z = triangle->points[k]->val[MESH_VAL_VINF];
// 			}
// 			double interpolated_val = get_triangle_interpolated_value(p[0], p[1], p[2], tri_centroid);
// 			if(fabs(interpolated_val-center_val) > dv_tolerance) {
// 				set_mesh_tri_flag(triangle, TRI_FLAG_ACC_ERR);
// 				break;
// 			}
// 		}
// 	}
// }

// void calc_porkchop_dv_boundaries(SegmentGroup *group, int departure_cap, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance) {
// 	group->vinf_array = data_array2_create();
//
// 	OSV osv0 = group->system->prop_method == ORB_ELEMENTS ?
// 					osv_from_elements(group->dep_body->orbit, jd_min_dep) :
// 					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_min_dep, group->system->cb);
//
// 	OSV osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
// 				   osv_from_elements(group->arr_body->orbit, jd_min_dep) :
// 				   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_min_dep, group->system->cb);
// 	Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, group->system->cb);
// 	double dt0, dt1;
//
//
// 	double r0 = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb).a, r1 = arr0.a;
// 	double r_ratio =  r1/r0;
// 	Hohmann hohmann = calc_hohmann_transfer(r0, r1, group->system->cb);
// 	double hohmann_dur = hohmann.dur/86400;
// 	double min_duration = 0.4 * hohmann_dur;
// 	double max_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_duration/hohmann_dur > 3) max_duration = hohmann_dur*3;
// 	if(max_duration < max_dur) max_dur = max_duration;
// 	if(min_duration > min_dur) min_dur = min_duration;
//
// 	double min_dt = min_dur*86400;
// 	double max_dt = max_dur*86400;
// 	double jd_dep_step = (jd_max_dep-jd_min_dep)/(departure_cap-1);
//
// 	for(int i = 0; i < departure_cap; i++) {
// 		double jd_dep = jd_min_dep + jd_dep_step*i;
//
// 		get_upper_and_lower_boundary_at_jd_dep(group, jd_dep, &dt0, &dt1);
//
// 		if(dt0 > max_dt || dt1 < min_dt) continue;
//
// 		double left_x = 0, right_x = 0;
// 		osv0 = group->system->prop_method == ORB_ELEMENTS ?
// 							osv_from_elements(group->dep_body->orbit, jd_dep) :
// 							osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);
// 		find_root(osv0, jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, max_depdv, dep_periapsis, &left_x, &right_x, 0.01);
//
// 		// No departure possible within given constraints
// 		if(left_x < 1 && right_x < 1 || right_x < min_dur*86400 || left_x > max_dur*86400) {
// 			size_t array_size = data_array2_size(group->vinf_array);
// 			if(array_size > 0 && data_array2_get_data(group->vinf_array)[array_size-1].y != 0)
// 				data_array2_append_new(group->vinf_array, jd_dep, 0);
// 			continue;
// 		}
//
// 		if(left_x < dt0) left_x = dt0;
// 		if(left_x < min_dur*86400) left_x = min_dur*86400;
// 		if(right_x > dt1) right_x = dt1;
// 		if(right_x > max_dur*86400) right_x = max_dur*86400;
//
// 		data_array2_append_new(group->vinf_array, jd_dep, left_x/86400);
// 		data_array2_append_new(group->vinf_array, jd_dep, right_x/86400);
// 	}
//
// 	if(data_array2_get_data(group->vinf_array)[data_array2_size(group->vinf_array)-1].y == 0) {
// 		data_array2_remove_at_idx(group->vinf_array, (int) data_array2_size(group->vinf_array)-1);
// 	}
// }

// void calc_group_porkchop_from_bands(SegmentGroup *group, int departure_cap, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance) {
// 	group->vinf_array = data_array2_create();
//
// 	OSV osv0 = group->system->prop_method == ORB_ELEMENTS ?
// 					osv_from_elements(group->dep_body->orbit, jd_min_dep) :
// 					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_min_dep, group->system->cb);
//
// 	OSV osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
// 				   osv_from_elements(group->arr_body->orbit, jd_min_dep) :
// 				   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_min_dep, group->system->cb);
// 	Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, group->system->cb);
// 	double dt0, dt1;
//
//
// 	double r0 = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb).a, r1 = arr0.a;
// 	double r_ratio =  r1/r0;
// 	Hohmann hohmann = calc_hohmann_transfer(r0, r1, group->system->cb);
// 	double hohmann_dur = hohmann.dur/86400;
// 	double min_duration = 0.4 * hohmann_dur;
// 	double max_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_duration/hohmann_dur > 3) max_duration = hohmann_dur*3;
// 	if(max_duration < max_dur) max_dur = max_duration;
// 	if(min_duration > min_dur) min_dur = min_duration;
//
// 	double min_dt = min_dur*86400;
// 	double max_dt = max_dur*86400;
// 	double jd_dep_step = (jd_max_dep-jd_min_dep)/(departure_cap-1);
//
// 	for(int i = 0; i < departure_cap; i++) {
// 		double jd_dep = jd_min_dep + jd_dep_step*i;
//
// 		get_upper_and_lower_boundary_at_jd_dep(group, jd_dep, &dt0, &dt1);
//
// 		if(dt0 > max_dt || dt1 < min_dt) continue;
//
// 		double left_x = 0, right_x = 0;
// 		osv0 = group->system->prop_method == ORB_ELEMENTS ?
// 							osv_from_elements(group->dep_body->orbit, jd_dep) :
// 							osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);
// 		find_root(osv0, jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, max_depdv, dep_periapsis, &left_x, &right_x, 0.01);
//
// 		// No departure possible within given constraints
// 		if(left_x < 1 && right_x < 1 || right_x < min_dur*86400 || left_x > max_dur*86400) {
// 			size_t array_size = data_array2_size(group->vinf_array);
// 			if(array_size > 0 && data_array2_get_data(group->vinf_array)[array_size-1].y != 0)
// 				data_array2_append_new(group->vinf_array, jd_dep, 0);
// 			continue;
// 		}
//
// 		if(left_x < dt0) left_x = dt0;
// 		if(left_x < min_dur*86400) left_x = min_dur*86400;
// 		if(right_x > dt1) right_x = dt1;
// 		if(right_x > max_dur*86400) right_x = max_dur*86400;
//
// 		data_array2_append_new(group->vinf_array, jd_dep, left_x/86400);
// 		data_array2_append_new(group->vinf_array, jd_dep, right_x/86400);
// 	}
//
// 	if(data_array2_get_data(group->vinf_array)[data_array2_size(group->vinf_array)-1].y == 0) {
// 		data_array2_remove_at_idx(group->vinf_array, (int) data_array2_size(group->vinf_array)-1);
// 	}
// }

// void calc_group_porkchop(SegmentGroup *group, int departure_cap, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance) {
// 	group->segment_steps = malloc(departure_cap * sizeof(struct ItinStep*));
//
// 	OSV osv0 = group->system->prop_method == ORB_ELEMENTS ?
// 					osv_from_elements(group->dep_body->orbit, jd_min_dep) :
// 					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_min_dep, group->system->cb);
//
// 	OSV osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
// 				   osv_from_elements(group->arr_body->orbit, jd_min_dep) :
// 				   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_min_dep, group->system->cb);
// 	Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, group->system->cb);
// 	double dt0, dt1;
//
//
// 	double r0 = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb).a, r1 = arr0.a;
// 	double r_ratio =  r1/r0;
// 	Hohmann hohmann = calc_hohmann_transfer(r0, r1, group->system->cb);
// 	double hohmann_dur = hohmann.dur/86400;
// 	double min_duration = 0.4 * hohmann_dur;
// 	double max_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_duration/hohmann_dur > 3) max_duration = hohmann_dur*3;
// 	if(max_duration < max_dur) max_dur = max_duration;
// 	if(min_duration > min_dur) min_dur = min_duration;
//
// 	double min_dt = min_dur*86400;
// 	double max_dt = max_dur*86400;
// 	double jd_dep_step = (jd_max_dep-jd_min_dep)/(departure_cap-1);
//
// 	for(int i = 0; i < departure_cap; i++) {
// 		double jd_dep = jd_min_dep + jd_dep_step*i;
//
// 		get_upper_and_lower_boundary_at_jd_dep(group, jd_dep, &dt0, &dt1);
//
// 		if(dt0 > max_dt || dt1 < min_dt) continue;
//
// 		double left_x = 0, right_x = 0;
// 		osv0 = group->system->prop_method == ORB_ELEMENTS ?
// 							osv_from_elements(group->dep_body->orbit, jd_dep) :
// 							osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);
// 		find_root(osv0, jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, max_depdv, dep_periapsis, &left_x, &right_x, 1);
//
// 		// No departure possible within given constraints
// 		if(left_x < 1 && right_x < 1 || right_x < min_dur*86400 || left_x > max_dur*86400) {
// 			if(group->num_steps > 0 && group->segment_steps[group->num_steps-1] != NULL) {
// 				group->segment_steps[group->num_steps] = NULL;
// 				group->num_steps++;
// 			}
// 			continue;
// 		}
//
// 		if(left_x < dt0) left_x = dt0;
// 		if(left_x < min_dur*86400) left_x = min_dur*86400;
// 		if(right_x > dt1) right_x = dt1;
// 		if(right_x > max_dur*86400) right_x = max_dur*86400;
//
// 		group->segment_steps[group->num_steps] = malloc(sizeof(struct ItinStep));
// 		group->segment_steps[group->num_steps]->body = group->dep_body;
// 		group->segment_steps[group->num_steps]->date = jd_dep;
// 		DataArray1 *dur_array = data_array1_create();
// 		calc_bounded_porkchop_line(group->segment_steps[group->num_steps], group->arr_body, group->system, dur_array, left_x, right_x, dep_periapsis, max_depdv, dv_tolerance);
// 		group->num_steps++;
//
//
//
//
// 		DataArray1 *data = data_array1_get_diff(dur_array);
// 		// printf("%6.3f  %6.3f   |   %10.3f\n", data_array1_get_min(data), data_array1_get_max(data), data_array1_get_max(data) / data_array1_get_min(data));
// 		// print_data_array1(data, "sep");
// 		data_array1_free(dur_array);
// 		data_array1_free(data);
// 	}
// }

// void calc_group_porkchop_stepped(SegmentGroup *group, int departure_cap, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance) {
// 	group->segment_steps = malloc(departure_cap * sizeof(struct ItinStep*));
//
// 	OSV osv0 = group->system->prop_method == ORB_ELEMENTS ?
// 					osv_from_elements(group->dep_body->orbit, jd_min_dep) :
// 					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_min_dep, group->system->cb);
//
// 	OSV osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
// 				   osv_from_elements(group->arr_body->orbit, jd_min_dep) :
// 				   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_min_dep, group->system->cb);
// 	Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, group->system->cb);
// 	double dt0, dt1;
//
//
// 	double r0 = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb).a, r1 = arr0.a;
// 	double r_ratio =  r1/r0;
// 	Hohmann hohmann = calc_hohmann_transfer(r0, r1, group->system->cb);
// 	double hohmann_dur = hohmann.dur/86400;
// 	double min_duration = 0.4 * hohmann_dur;
// 	double max_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_duration/hohmann_dur > 3) max_duration = hohmann_dur*3;
// 	if(max_duration < max_dur) max_dur = max_duration;
// 	if(min_duration > min_dur) min_dur = min_duration;
//
// 	double min_dt = min_dur*86400;
// 	double max_dt = max_dur*86400;
// 	double jd_dep_step = (jd_max_dep-jd_min_dep)/(departure_cap-1);
//
// 	for(int i = 0; i < departure_cap; i++) {
// 		double jd_dep = jd_min_dep + jd_dep_step*i;
//
// 		get_upper_and_lower_boundary_at_jd_dep(group, jd_dep, &dt0, &dt1);
//
// 		if(dt0 > max_dt || dt1 < min_dt) continue;
//
// 		double left_x = 0, right_x = 0;
// 		osv0 = group->system->prop_method == ORB_ELEMENTS ?
// 							osv_from_elements(group->dep_body->orbit, jd_dep) :
// 							osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);
// 		find_root(osv0, jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, max_depdv, dep_periapsis, &left_x, &right_x, 1);
//
// 		// No departure possible within given constraints
// 		if(left_x < 1 && right_x < 1 || right_x < min_dur*86400 || left_x > max_dur*86400) {
// 			if(group->num_steps > 0 && group->segment_steps[group->num_steps-1] != NULL) {
// 				group->segment_steps[group->num_steps] = NULL;
// 				group->num_steps++;
// 			}
// 			continue;
// 		}
//
// 		if(left_x < dt0) left_x = dt0;
// 		if(left_x < min_dur*86400) left_x = min_dur*86400;
// 		if(right_x > dt1) right_x = dt1;
// 		if(right_x > max_dur*86400) right_x = max_dur*86400;
//
// 		group->segment_steps[group->num_steps] = malloc(sizeof(struct ItinStep));
// 		group->segment_steps[group->num_steps]->body = group->dep_body;
// 		group->segment_steps[group->num_steps]->date = jd_dep;
// 		DataArray1 *dur_array = data_array1_create();
// 		calc_bounded_porkchop_line(group->segment_steps[group->num_steps], group->arr_body, group->system, dur_array, left_x, right_x, dep_periapsis, max_depdv, dv_tolerance);
// 		group->num_steps++;
//
//
//
//
// 		DataArray1 *data = data_array1_get_diff(dur_array);
// 		// printf("%6.3f  %6.3f   |   %10.3f\n", data_array1_get_min(data), data_array1_get_max(data), data_array1_get_max(data) / data_array1_get_min(data));
// 		// print_data_array1(data, "sep");
// 		data_array1_free(dur_array);
// 		data_array1_free(data);
// 	}
// }

// void calc_bounded_porkchop_line_outline(struct ItinStep *departure_step, Body *arr_body, CelestSystem *system, DataArray1 *dur_array, double min_dt, double max_dt, double dep_periapsis, double max_depdv, double dv_tolerance) {
// 	Body *dep_body = departure_step->body;
// 	double jd_dep = departure_step->date;
// 	OSV osv0 = system->prop_method == ORB_ELEMENTS ?
// 					osv_from_elements(dep_body->orbit, jd_dep) :
// 					osv_from_ephem(dep_body->ephem, dep_body->num_ephems, jd_dep, system->cb);
//
// 	double dt = min_dt;
//
// 	DataArray2 *data_dep = data_array2_create();
// 	DataArray2 *data_arr = data_array2_create();
//
// 	struct ItinStep *curr_step = departure_step;
// 	curr_step->r = osv0.r;
// 	curr_step->v_body = osv0.v;
// 	curr_step->v_dep = vec3(0, 0, 0);
// 	curr_step->v_arr = vec3(0, 0, 0);
// 	curr_step->num_next_nodes = 0;
// 	curr_step->prev = NULL;
// 	curr_step->next = (struct ItinStep **) malloc(1000 * sizeof(struct ItinStep *));
// 	int counter = 0;
//
// 	for(int j = 0; j < 1000; j++) {
// 		// printf("%f  %f  %f  %f  %f\n", min_dt, max_dt, dt0, dt1, dt);
//
// 		double jd_arr = jd_dep + dt / 86400;
//
// 		OSV osv_arr = system->prop_method == ORB_ELEMENTS ?
// 					osv_from_elements(arr_body->orbit, jd_arr) :
// 					osv_from_ephem(arr_body->ephem, arr_body->num_ephems, jd_arr, system->cb);
//
// 		Lambert3 tf = calc_lambert3(osv0.r, osv_arr.r, (jd_arr - jd_dep) * 86400, system->cb);
//
// 		double vinf_dep = fabs(mag_vec3(subtract_vec3(tf.v0, osv0.v)));
// 		double dv_dep = dv_circ(dep_body,alt2radius(dep_body, dep_periapsis),vinf_dep);
// 		double vinf_arr = fabs(mag_vec3(subtract_vec3(tf.v1, osv_arr.v)));
//
// 		curr_step = get_first(curr_step);
// 		// sort chronologically
// 		int insert_index = counter;
// 		while(insert_index > 0) {
// 			if(curr_step->next[insert_index-1]->date < jd_arr) break;
// 			insert_index--;
// 		}
// 		if(insert_index != counter) {
// 			memmove(&curr_step->next[insert_index+1],
// 				&curr_step->next[insert_index],
// 				(counter+2 - insert_index) * sizeof(*curr_step->next));
// 		}
//
// 		curr_step->next[insert_index] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
// 		curr_step->next[insert_index]->prev = curr_step;
// 		curr_step->next[insert_index]->next = NULL;
// 		curr_step = curr_step->next[insert_index];
//
// 		curr_step->body = arr_body;
// 		curr_step->date = jd_arr;
// 		curr_step->r = osv_arr.r;
// 		curr_step->v_dep = tf.v0;
// 		curr_step->v_arr = tf.v1;
// 		curr_step->v_body = osv_arr.v;
// 		curr_step->num_next_nodes = 0;
// 		curr_step->prev->num_next_nodes++;
// 		counter++;
//
// 		if(dur_array) data_array1_insert_new(dur_array, dt/86400);
//
// 		data_array2_insert_new(data_dep, dt/86400, dv_dep);
// 		data_array2_insert_new(data_arr, dt/86400, vinf_arr);
//
// 		if(dt == min_dt) dt = max_dt;
// 		else break;
// 	}
// 	data_array2_free(data_dep);
// 	data_array2_free(data_arr);
// }

// void calc_group_porkchop_outline(SegmentGroup *group, int departure_cap, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance) {
// 	group->segment_steps = malloc(departure_cap * sizeof(struct ItinStep*));
//
// 	OSV osv0 = group->system->prop_method == ORB_ELEMENTS ?
// 					osv_from_elements(group->dep_body->orbit, jd_min_dep) :
// 					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_min_dep, group->system->cb);
//
// 	OSV osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
// 				   osv_from_elements(group->arr_body->orbit, jd_min_dep) :
// 				   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_min_dep, group->system->cb);
// 	Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, group->system->cb);
// 	double dt0, dt1;
//
//
// 	double r0 = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb).a, r1 = arr0.a;
// 	double r_ratio =  r1/r0;
// 	Hohmann hohmann = calc_hohmann_transfer(r0, r1, group->system->cb);
// 	double hohmann_dur = hohmann.dur/86400;
// 	double min_duration = 0.4 * hohmann_dur;
// 	double max_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_duration/hohmann_dur > 3) max_duration = hohmann_dur*3;
// 	if(max_duration < max_dur) max_dur = max_duration;
// 	if(min_duration > min_dur) min_dur = min_duration;
//
// 	double min_dt = min_dur*86400;
// 	double max_dt = max_dur*86400;
// 	double jd_dep_step = (jd_max_dep-jd_min_dep)/(departure_cap-1);
//
// 	for(int i = 0; i < departure_cap; i++) {
// 		double jd_dep = jd_min_dep + jd_dep_step*i;
//
// 		get_upper_and_lower_boundary_at_jd_dep(group, jd_dep, &dt0, &dt1);
//
// 		if(dt0 > max_dt || dt1 < min_dt) continue;
//
// 		double left_x = 0, right_x = 0;
// 		osv0 = group->system->prop_method == ORB_ELEMENTS ?
// 							osv_from_elements(group->dep_body->orbit, jd_dep) :
// 							osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);
// 		find_root(osv0, jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, max_depdv, dep_periapsis, &left_x, &right_x, 1);
//
// 		// No departure possible within given constraints
// 		if(left_x < 1 && right_x < 1 || right_x < min_dur*86400 || left_x > max_dur*86400) {
// 			if(group->num_steps > 0 && group->segment_steps[group->num_steps-1] != NULL) {
// 				group->segment_steps[group->num_steps] = NULL;
// 				group->num_steps++;
// 			}
// 			continue;
// 		}
//
// 		if(left_x < dt0) left_x = dt0;
// 		if(left_x < min_dur*86400) left_x = min_dur*86400;
// 		if(right_x > dt1) right_x = dt1;
// 		if(right_x > max_dur*86400) right_x = max_dur*86400;
//
// 		group->segment_steps[group->num_steps] = malloc(sizeof(struct ItinStep));
// 		group->segment_steps[group->num_steps]->body = group->dep_body;
// 		group->segment_steps[group->num_steps]->date = jd_dep;
// 		DataArray1 *dur_array = data_array1_create();
// 		calc_bounded_porkchop_line_outline(group->segment_steps[group->num_steps], group->arr_body, group->system, dur_array, left_x, right_x, dep_periapsis, max_depdv, dv_tolerance);
// 		group->num_steps++;
//
//
//
//
// 		DataArray1 *data = data_array1_get_diff(dur_array);
// 		// printf("%6.3f  %6.3f   |   %10.3f\n", data_array1_get_min(data), data_array1_get_max(data), data_array1_get_max(data) / data_array1_get_min(data));
// 		// print_data_array1(data, "sep");
// 		data_array1_free(dur_array);
// 		data_array1_free(data);
// 	}
// }

// void calc_group_porkchop_subgrid(SegmentGroup *group, MeshGrid2 *grid, size_t *grid_col_cap, int insert_col_idx, double jd_max_arr, double min_dt, double max_dt, double dep_periapsis, double max_depdv, double dv_tolerance) {
// 	DataArray1 *dur_array0 = data_array1_create();
// 	DataArray1 *dur_array1 = data_array1_create();
// 	for(int i = 0; i < grid->num_col_rows[insert_col_idx-1]; i++) {
// 		data_array1_append_new(dur_array0, grid->points[insert_col_idx-1][i]->pos.y);
// 	}
// 	for(int i = 0; i < grid->num_col_rows[insert_col_idx]; i++) {
// 		data_array1_append_new(dur_array1, grid->points[insert_col_idx][i]->pos.y);
// 	}
//
// 	double jd_min_dep = grid->points[insert_col_idx-1][0]->pos.x;
// 	double jd_max_dep = grid->points[insert_col_idx][0]->pos.x;
//
// 	DataArray1 *diff0 = data_array1_get_diff(dur_array0);
// 	DataArray1 *diff1 = data_array1_get_diff(dur_array1);
// 	double *dur_data0 = data_array1_get_data(dur_array0);
// 	double *dur_data1 = data_array1_get_data(dur_array1);
// 	double *dur_diff_data0 = data_array1_get_data(diff0);
// 	double *dur_diff_data1 = data_array1_get_data(diff1);
// 	int num_dur0 = (int) data_array1_size(dur_array0);
// 	int num_dur1 = (int) data_array1_size(dur_array1);
//
// 	double target_min_dur = (jd_max_dep - jd_min_dep)*2;
//
// 	int up_dur_bottom_idx0 = 0, up_dur_bottom_idx1 = 0, low_dur_top_idx0 = num_dur0-2, low_dur_top_idx1 = num_dur1-2;
//
// 	for(int i = 0; i < num_dur0-1; i++) {
// 		if(dur_diff_data0[i] > target_min_dur) {
// 			low_dur_top_idx0 = i;
// 			break;
// 		}
// 	}
// 	for(int i = 0; i < num_dur1-1; i++) {
// 		if(dur_diff_data1[i] > target_min_dur) {
// 			low_dur_top_idx1 = i;
// 			break;
// 		}
// 	}
// 	for(int i = num_dur0-2; i >= 0; i--) {
// 		if(dur_diff_data0[i] > target_min_dur) {
// 			up_dur_bottom_idx0 = i;
// 			break;
// 		}
// 	}
// 	for(int i = num_dur1-2; i >= 0; i--) {
// 		if(dur_diff_data1[i] > target_min_dur) {
// 			up_dur_bottom_idx1 = i;
// 			break;
// 		}
// 	}
//
// 	OSV osv0 = group->system->prop_method == ORB_ELEMENTS ?
// 					osv_from_elements(group->dep_body->orbit, jd_min_dep) :
// 					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_min_dep, group->system->cb);
// 	double dt0, dt1;
//
// 	double jd_dep = (jd_min_dep + jd_max_dep)/2;
//
// 	get_upper_and_lower_boundary_at_jd_dep(group, jd_dep, &dt0, &dt1);
//
// 	if(dt0 > max_dt || dt1 < min_dt) return;
//
// 	double up_dur_top, up_dur_bottom, low_dur_top, low_dur_bottom;
// 	osv0 = group->system->prop_method == ORB_ELEMENTS ?
// 						osv_from_elements(group->dep_body->orbit, jd_dep) :
// 						osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);
// 	find_root(osv0, jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, max_depdv, dep_periapsis, &low_dur_bottom, &up_dur_top);
//
// 	// No departure possible within given constraints
// 	if(low_dur_bottom < 1 && up_dur_top < 1 || up_dur_top < min_dt || low_dur_bottom > max_dt) {
// 		if(group->num_steps > 0 && group->segment_steps[group->num_steps-1] != NULL) {
// 			group->segment_steps[group->num_steps] = NULL;
// 			group->num_steps++;
// 		}
// 		return;
// 	}
//
//
// 	low_dur_top = fmax(dur_data0[low_dur_top_idx0], dur_data1[low_dur_top_idx1])*86400;
// 	up_dur_bottom = fmin(dur_data0[up_dur_bottom_idx0+1], dur_data1[up_dur_bottom_idx1+1])*86400;
//
// 	if(low_dur_top_idx0 == 0 && low_dur_top_idx1 == 0) return;
//
// 	if(low_dur_bottom < dt0) low_dur_bottom = dt0;
// 	if(low_dur_bottom < min_dt) low_dur_bottom = min_dt;
// 	if(up_dur_top > dt1) up_dur_top = dt1;
// 	if(up_dur_top > max_dt) up_dur_top = max_dt;
//
// 	struct ItinStep *departure = malloc(sizeof(struct ItinStep));
// 	departure->body = group->dep_body;
// 	departure->date = jd_dep;
// 	DataArray1 *dur_array = data_array1_create();
// 	calc_bounded_porkchop_line(departure, group->arr_body, group->system, dur_array, low_dur_bottom, low_dur_top, dep_periapsis, max_depdv, dv_tolerance);
//
// 	MeshPoint2 **new_col_points = malloc(departure->num_next_nodes * sizeof(struct MeshPoint2 *));
// 	for(int i = 0; i < departure->num_next_nodes; i++) {
// 		new_col_points[i] = malloc(sizeof(MeshPoint2));
// 		new_col_points[i]->pos = vec2(jd_dep, departure->next[i]->date-jd_dep);
// 		new_col_points[i]->old_val = 0;
// 		new_col_points[i]->old_data = departure->next[i];
// 		new_col_points[i]->num_triangles = 0;
// 		new_col_points[i]->triangle_cap = 0;
// 		new_col_points[i]->triangles = NULL;
// 	}
//
// 	if(grid->num_cols == *grid_col_cap) {
// 		*grid_col_cap *= 2;
// 		MeshPoint2 ***temp_points = realloc(grid->points, *grid_col_cap * sizeof(MeshPoint2**));
// 		if(temp_points) grid->points = temp_points;
// 		size_t *temp_num_col_rows = realloc(grid->num_col_rows, *grid_col_cap * sizeof(size_t));
// 		if(temp_num_col_rows) grid->num_col_rows = temp_num_col_rows;
// 	}
// 	memmove(grid->points+insert_col_idx+1, grid->points+insert_col_idx, (grid->num_cols-insert_col_idx) * sizeof(MeshPoint2 **));
// 	memmove(grid->num_col_rows+insert_col_idx+1, grid->num_col_rows+insert_col_idx, (grid->num_cols-insert_col_idx) * sizeof(size_t));
// 	grid->points[insert_col_idx] = new_col_points;
// 	grid->num_col_rows[insert_col_idx] = departure->num_next_nodes;
// 	grid->num_cols++;
//
// 	data_array1_free(diff0);
// 	data_array1_free(diff1);
// 	data_array1_free(dur_array);
// }

// void calc_group_porkchop_mesh(SegmentGroup *group, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance) {
// 	OSV osv0 = group->system->prop_method == ORB_ELEMENTS ?
// 					osv_from_elements(group->dep_body->orbit, jd_min_dep) :
// 					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_min_dep, group->system->cb);
//
// 	OSV osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
// 				   osv_from_elements(group->arr_body->orbit, jd_min_dep) :
// 				   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_min_dep, group->system->cb);
// 	Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, group->system->cb);
// 	double dt0, dt1;
//
//
// 	double r0 = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb).a, r1 = arr0.a;
// 	double r_ratio =  r1/r0;
// 	Hohmann hohmann = calc_hohmann_transfer(r0, r1, group->system->cb);
// 	double hohmann_dur = hohmann.dur/86400;
// 	double min_duration = 0.4 * hohmann_dur;
// 	double max_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_duration/hohmann_dur > 3) max_duration = hohmann_dur*3;
// 	if(max_duration < max_dur) max_dur = max_duration;
// 	if(min_duration > min_dur) min_dur = min_duration;
//
// 	double min_dt = min_dur*86400;
// 	double max_dt = max_dur*86400;
// 	double jd_dep_base_step = (jd_max_dep-jd_min_dep)/1000;
// 	double jd_dep = jd_min_dep;
//
// 	MeshGrid2 grid;
// 	grid.num_col_rows = NULL;
// 	grid.points = NULL;
// 	grid.num_cols = 0;
//
// 	while(jd_dep <= jd_max_dep) {
// 		get_upper_and_lower_boundary_at_jd_dep(group, jd_dep, &dt0, &dt1);
//
// 		if(dt0 > max_dt || dt1 < min_dt) continue;
//
// 		double left_x = 0, right_x = 0;
// 		osv0 = group->system->prop_method == ORB_ELEMENTS ?
// 							osv_from_elements(group->dep_body->orbit, jd_dep) :
// 							osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);
// 		find_root(osv0, jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, max_depdv, dep_periapsis, &left_x, &right_x);
//
// 		// No departure possible within given constraints
// 		if(left_x < 1 && right_x < 1 || right_x < min_dur*86400 || left_x > max_dur*86400) {
// 			if(group->num_steps > 0 && group->segment_steps[group->num_steps-1] != NULL) {
// 				group->segment_steps[group->num_steps] = NULL;
// 				group->num_steps++;
// 			}
// 			continue;
// 		}
//
// 		if(left_x < dt0) left_x = dt0;
// 		if(left_x < min_dur*86400) left_x = min_dur*86400;
// 		if(right_x > dt1) right_x = dt1;
// 		if(right_x > max_dur*86400) right_x = max_dur*86400;
//
// 		struct ItinStep *departure = malloc(sizeof(struct ItinStep));
// 		departure->body = group->dep_body;
// 		departure->date = jd_dep;
// 		DataArray1 *dur_array = data_array1_create();
// 		calc_bounded_porkchop_line(departure, group->arr_body, group->system, dur_array, left_x, right_x, dep_periapsis, max_depdv, dv_tolerance);
//
//
//
//
// 		DataArray1 *data = data_array1_get_diff(dur_array);
// 		// printf("%6.3f  %6.3f   |   %10.3f\n", data_array1_get_min(data), data_array1_get_max(data), data_array1_get_max(data) / data_array1_get_min(data));
// 		print_data_array1(data, "sep");
// 		data_array1_free(dur_array);
// 		data_array1_free(data);
// 	}
// }

// void calc_group_porkchop_mesh2(SegmentGroup *group, int departure_cap, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance) {
// 	group->segment_steps = malloc(departure_cap * sizeof(struct ItinStep*));
//
// 	OSV osv0 = group->system->prop_method == ORB_ELEMENTS ?
// 					osv_from_elements(group->dep_body->orbit, jd_min_dep) :
// 					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_min_dep, group->system->cb);
//
// 	OSV osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
// 				   osv_from_elements(group->arr_body->orbit, jd_min_dep) :
// 				   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_min_dep, group->system->cb);
// 	Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, group->system->cb);
// 	double dt0, dt1;
//
//
// 	double r0 = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb).a, r1 = arr0.a;
// 	double r_ratio =  r1/r0;
// 	Hohmann hohmann = calc_hohmann_transfer(r0, r1, group->system->cb);
// 	double hohmann_dur = hohmann.dur/86400;
// 	double min_duration = 0.4 * hohmann_dur;
// 	double max_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_duration/hohmann_dur > 3) max_duration = hohmann_dur*3;
// 	if(max_duration < max_dur) max_dur = max_duration;
// 	if(min_duration > min_dur) min_dur = min_duration;
//
// 	double min_dt = min_dur*86400;
// 	double max_dt = max_dur*86400;
// 	double jd_dep_base_step = (jd_max_dep-jd_min_dep)/1000;
// 	double jd_dep = jd_min_dep;
//
// 	DataArray1 *dur_array0 = data_array1_create(), *dur_array1 = data_array1_create();
//
// 	MeshGrid2 grid;
// 	grid.num_col_rows = NULL;
// 	grid.points = NULL;
// 	grid.num_cols = 0;
//
// 	while(jd_dep <= jd_max_dep) {
// 		get_upper_and_lower_boundary_at_jd_dep(group, jd_dep, &dt0, &dt1);
//
// 		if(dt0 > max_dt || dt1 < min_dt) {
// 			data_array1_clear(dur_array0);
// 			continue;
// 		}
//
// 		double left_x = 0, right_x = 0;
// 		osv0 = group->system->prop_method == ORB_ELEMENTS ?
// 							osv_from_elements(group->dep_body->orbit, jd_dep) :
// 							osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);
// 		find_root(osv0, jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, max_depdv, dep_periapsis, &left_x, &right_x);
//
// 		// No departure possible within given constraints
// 		if(left_x < 1 && right_x < 1 || right_x < min_dur*86400 || left_x > max_dur*86400) {
// 			data_array1_clear(dur_array0);
// 			continue;
// 		}
//
// 		if(left_x < dt0) left_x = dt0;
// 		if(left_x < min_dur*86400) left_x = min_dur*86400;
// 		if(right_x > dt1) right_x = dt1;
// 		if(right_x > max_dur*86400) right_x = max_dur*86400;
//
// 		struct ItinStep *departure = malloc(sizeof(struct ItinStep));
// 		departure->body = group->dep_body;
// 		departure->date = jd_dep;
// 		calc_bounded_porkchop_line(departure, group->arr_body, group->system, dur_array1, left_x, right_x, dep_periapsis, max_depdv, dv_tolerance);
//
// 		if(data_array1_size(dur_array0) > 0) {
// 			MeshPoint2 **new_col_points = malloc(departure->num_next_nodes * sizeof(struct MeshPoint2 *));
// 			for(int i = 0; i < departure->num_next_nodes; i++) {
// 				new_col_points[i] = malloc(sizeof(MeshPoint2));
// 				new_col_points[i]->pos = vec2(jd_dep, departure->next[i]->date-jd_dep);
// 				new_col_points[i]->val = 0;
// 				new_col_points[i]->data = departure->next[i];
// 				new_col_points[i]->num_triangles = 0;
// 				new_col_points[i]->triangle_cap = 0;
// 				new_col_points[i]->triangles = NULL;
// 			}
// 		}
//
// 		data_array1_free(dur_array0);
// 		dur_array0 = dur_array1;
// 		dur_array1 = data_array1_create();
//
// 		DataArray1 *data = data_array1_get_diff(dur_array1);
// 		printf("%6.3f  %6.3f   |   %10.3f\n", data_array1_get_min(data), data_array1_get_max(data), data_array1_get_max(data) / data_array1_get_min(data));
// 		data_array1_free(dur_array0);
// 		data_array1_free(data);
// 	}
// }

// double calc_time_to_next_an_dn_line_up(OSV osv_dep_body, OSV osv_arr_body, Body *cb, double *next_line_up_dt, double *next_opposite_line_up_dt) {
// 	Plane3 orbital_plane_dep_body = constr_plane3(vec3(0,0,0), osv_dep_body.r, osv_dep_body.v);
// 	Plane3 orbital_plane_arr_body = constr_plane3(vec3(0,0,0), osv_arr_body.r, osv_arr_body.v);
// 	Vector3 plane_intersection = calc_intersecting_line_dir_plane3(orbital_plane_dep_body, orbital_plane_arr_body);
//
// 	Orbit dep_body_orbit = constr_orbit_from_osv(osv_dep_body.r, osv_dep_body.v, cb);
// 	double dep_body_orbit_period = calc_orbital_period(dep_body_orbit);
// 	double tpe0 = calc_orbit_time_since_periapsis(dep_body_orbit);
//
// 	double delta_true_anomaly = angle_vec3_vec3(osv_dep_body.r, plane_intersection);
//
// 	if(dep_body_orbit.i < M_PI/2 && cross_vec3(osv_dep_body.r, plane_intersection).z < 0 ||
// 		dep_body_orbit.i > M_PI/2 && cross_vec3(osv_dep_body.r, plane_intersection).z > 0) {
// 		delta_true_anomaly = 2*M_PI - delta_true_anomaly - M_PI;
// 	}
//
// 	Orbit line_up_orbit = dep_body_orbit;
// 	line_up_orbit.ta = pi_norm(line_up_orbit.ta + delta_true_anomaly);
// 	double dt_line_up = calc_orbit_time_since_periapsis(line_up_orbit)-tpe0;
//
// 	line_up_orbit.ta = pi_norm(line_up_orbit.ta + M_PI);
// 	double dt_opp_line_up = calc_orbit_time_since_periapsis(line_up_orbit)-tpe0;
//
// 	if(dt_line_up < 0)		dt_line_up += dep_body_orbit_period;
// 	if(dt_opp_line_up < 0)	dt_opp_line_up += dep_body_orbit_period;
//
// 	*next_line_up_dt = dt_line_up;
// 	*next_opposite_line_up_dt = dt_opp_line_up;
// }

// DataArray2 * calc_min_vinf_line2(SegmentGroup *group, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double vinf_tolerance) {
// 	DataArray2 *min_per_dep = data_array2_create();
// 	group->num_steps = 0;
//
// 	OSV osv0 = group->system->prop_method == ORB_ELEMENTS ?
// 					osv_from_elements(group->dep_body->orbit, jd_min_dep) :
// 					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_min_dep, group->system->cb);
//
// 	OSV osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
// 				   osv_from_elements(group->arr_body->orbit, jd_min_dep) :
// 				   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_min_dep, group->system->cb);
// 	Orbit orbit0 = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb);
// 	double period_dep = calc_orbital_period(orbit0);
// 	double next_line_up_dt, next_opp_line_up_dt;
// 	calc_time_to_next_an_dn_line_up(osv0, osv_arr0, group->system->cb, &next_line_up_dt, &next_opp_line_up_dt);
//
// 	double min_dt = min_dur*86400;
// 	double max_dt = max_dur*86400;
// 	DataArray2 *data_dep = data_array2_create();
//
// 	double min_dep = jd_min_dep;
// 	double max_dep = jd_min_dep + next_line_up_dt/86400;
// 	if(max_dep > jd_max_dep) max_dep = jd_max_dep;
// 	double jd_dep = min_dep;
// 	int index0 = 0;
// 	double max_jd_step = (next_opp_line_up_dt-next_line_up_dt)/86400/5;
//
// 	while(min_dep < jd_max_dep) {
// 		data_array2_clear(data_dep);
// 		double dt0, dt1;
// 		get_upper_and_lower_boundary_at_jd_dep(group, jd_dep, &dt0, &dt1);
//
// 		if(dt0 > max_dt || dt1 < min_dt) {
// 			double temp = min_dep;
// 			min_dep = max_dep;
// 			max_dep = temp + period_dep/86400;
// 			if(max_dep > jd_max_dep) max_dep = jd_max_dep;
// 			jd_dep = min_dep;
// 			continue;
// 		}
//
// 		osv0 = group->system->prop_method == ORB_ELEMENTS ?
// 				osv_from_elements(group->dep_body->orbit, jd_dep) :
// 				osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);
//
// 		double left_x = 0, right_x = 0;
//
// 		find_root(osv0, jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, 1e9, 1e9, &left_x, &right_x, 1);
// 		// printf("%f   %f\n", left_x/86400, right_x/86400);
//
// 		// printf("%f   ROOT: %f   %f   (%f  %f)   (%f  %f)\n", jd_dep-jd_min_dep, left_x/86400, right_x/86400, dt0/86400, dt1/86400, opp_guess/86400, conj_guess/86400);
// 		if(left_x < 1 && right_x < 1 || right_x < min_dur*86400 || left_x > max_dur*86400) {continue;}
//
// 		double opp_conj_margin = 86400*0.2;
//
// 		if(left_x < dt0+opp_conj_margin) left_x = dt0+opp_conj_margin;
// 		if(left_x < min_dur*86400) left_x = min_dur*86400;
// 		if(right_x > dt1-opp_conj_margin) right_x = dt1-opp_conj_margin;
// 		if(right_x > max_dur*86400) right_x = max_dur*86400;
// 		double dt = left_x;
// 		// printf("%f   %f\n", left_x/86400, right_x/86400);
//
// 		for(int j = 0; j < 1000; j++) {
// 			// printf("%f  %f  %f  %f  %f\n", min_dt, max_dt, dt0, dt1, dt);
//
// 			double jd_arr = jd_dep + dt / 86400;
//
// 			OSV osv1 = group->system->prop_method == ORB_ELEMENTS ?
// 						osv_from_elements(group->arr_body->orbit, jd_arr) :
// 						osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_arr, group->system->cb);
//
// 			Lambert3 tf = calc_lambert3(osv0.r, osv1.r, (jd_arr - jd_dep) * 86400, group->system->cb);
//
// 			double vinf = fabs(mag_vec3(subtract_vec3(tf.v0, osv0.v)));
// 			data_array2_insert_new(data_dep, dt/86400, vinf);
//
// 			if(dt == left_x) dt = right_x;
// 			else if(dt == right_x) dt = ( dt + data_array2_get_data(data_dep)[0].x*86400 ) / 2;
// 			else {
// 				double next_x = calc_next_x_find_min(data_dep, vinf_tolerance/2)*86400;
//
// 				if(next_x < 0) {
// 					data_array2_insert_new(min_per_dep, jd_dep, data_array2_get_min(data_dep).y);
// 					// print_date(convert_JD_date(jd_dep, DATE_ISO), 0);
// 					// printf("      %f  |   %f    %f    %f  |    %f    %f    %f\n", jd_dep, dt/86400, left_x/86400, right_x/86400, dt, left_x, right_x);
// 					// data_array2_append_new(min_per_dep, jd_dep-jd_min_dep, data[min_idx].x);
// 					break;
// 				}
// 				dt = next_x;
// 			}
// 		}
// 		// print_data_array2(data_dep, "dep", "dv");
// 		// print_data_array2(min_per_dep, "dep", "dv");
//
// 		if(jd_dep == min_dep) {jd_dep = max_dep; continue;}
// 		if(jd_dep == max_dep) {jd_dep = (min_dep + max_dep)/2; continue;}
// 		double next_dep = calc_next_x_wrt_smoothness(min_per_dep, index0, vinf_tolerance/2);
// 		if(!isnan(next_dep)) {
// 			if(next_dep - jd_dep > max_jd_step) {jd_dep += max_jd_step; continue;}
// 			jd_dep = next_dep; continue;
// 		}
//
// 		if(max_dep - jd_dep > max_jd_step) {jd_dep += max_jd_step; continue;}
//
// 		double temp = min_dep;
// 		min_dep = max_dep;
// 		max_dep = temp + period_dep/86400;
// 		if(max_dep > jd_max_dep) max_dep = jd_max_dep;
// 		jd_dep = max_dep;
// 		index0 = (int) data_array2_size(min_per_dep)-1;
// 	}
//
// 	data_array2_free(data_dep);
//
// 	return min_per_dep;
// }

// void get_dur_limits_from_departure_date(MeshBox2 *box, double jd_dep, DataArray2 *data_array) {
// 	if(box->min.x > jd_dep || box->max.x < jd_dep) return;
//
// 	if(box->type == MESHBOX_SUBBOXES) {
// 		for(int i = 0; i < box->subboxes.num; i++) {
// 			get_dur_limits_from_departure_date(box->subboxes.boxes[i], jd_dep, data_array);
// 		}
// 	} else if(box->type == MESHBOX_TRIANGLES) {
// 		for(int i = 0; i < box->tri.num; i++) {
// 			if(triangle_is_edge(box->tri.triangles[i])) {
// 				Vector2 min, max;
// 				find_2dtriangle_minmax(box->tri.triangles[i], &min, &max);
// 				if(min.x > jd_dep || max.x < jd_dep) continue;
// 				for(int edge_idx = 0; edge_idx < 3; edge_idx++) {
// 					if(!box->tri.triangles[i]->adj_triangles[edge_idx]) {
// 						Vector2 p0 = box->tri.triangles[i]->points[edge_idx]->pos;
// 						Vector2 p1 = box->tri.triangles[i]->points[(edge_idx+1)%3]->pos;
//
// 						if(p0.x < jd_dep == p1.x < jd_dep && p0.x != jd_dep && p1.x != jd_dep) continue;
//
// 						double m = (p1.y-p0.y) / (p1.x-p0.x);
// 						double dur = (jd_dep-p0.x)*m+p0.y;
// 						data_array2_insert_new(data_array, jd_dep, dur);
// 					}
// 				}
// 			}
// 		}
// 	}
// }

// void get_dur_limits_from_all_triangles(Mesh2 *mesh, DataArray2 *data_array) {
// 	for(int i = 0; i < mesh->num_triangles; i++) {
// 		for(int edge_idx = 0; edge_idx < 3; edge_idx++) {
// 			if(!mesh->triangles[i]->adj_triangles[edge_idx]) {
// 				Vector2 p0 = mesh->triangles[i]->points[edge_idx]->pos;
// 				Vector2 p1 = mesh->triangles[i]->points[(edge_idx+1)%3]->pos;
// 				data_array2_insert_new(data_array, p0.x, p0.y);
// 				data_array2_insert_new(data_array, p1.x, p1.y);
// 			}
// 		}
// 	}
// }

// DataArray2 * get_dur_limits_from_edge_triangles(Mesh2 *mesh) {
// 	DataArray2 *data_array = data_array2_create();
// 	MeshTriangle2 *triangle = NULL;
// 	for(int i = 0; i < mesh->num_triangles; i++) {
// 		if(triangle_is_edge(mesh->triangles[i])) {
// 			triangle = mesh->triangles[i]; break;
// 		}
// 	}
//
// 	if(triangle == NULL) {return data_array;}
// 	MeshPoint2 *first_point = NULL;
// 	MeshPoint2 *prev_point = NULL;
// 	MeshPoint2 *current_point = NULL;
//
// 	for(int edge_idx = 0; edge_idx < 3; edge_idx++) {
// 		if(!triangle->adj_triangles[edge_idx]) {
// 			first_point = triangle->points[edge_idx];
// 			current_point = triangle->points[(edge_idx+1)%3];
// 			data_array2_append_new(data_array, first_point->pos.x, first_point->pos.y);
// 			data_array2_append_new(data_array, current_point->pos.x, current_point->pos.y);
// 		}
// 	}
//
// 	prev_point = first_point;
//
// 	do {
// 		for(int i = 0; i < current_point->num_triangles; i++) {
// 			if(triangle_is_edge(current_point->triangles[i])) {
// 				for(int edge_idx = 0; edge_idx < 3; edge_idx++) {
// 					if(!current_point->triangles[i]->adj_triangles[edge_idx]) {
// 						if(current_point == current_point->triangles[i]->points[edge_idx] && prev_point != current_point->triangles[i]->points[(edge_idx+1)%3]) {
// 							prev_point = current_point;
// 							current_point = current_point->triangles[i]->points[(edge_idx+1)%3];
// 							if(current_point == first_point) return data_array;
// 							data_array2_append_new(data_array, current_point->pos.x, current_point->pos.y);
// 							i = current_point->num_triangles;	// break outside loop
// 							break;
// 						}
// 						if(current_point == current_point->triangles[i]->points[(edge_idx+1)%3] && prev_point != current_point->triangles[i]->points[edge_idx]) {
// 							prev_point = current_point;
// 							current_point = current_point->triangles[i]->points[edge_idx];
// 							if(current_point == first_point) return data_array;
// 							data_array2_append_new(data_array, current_point->pos.x, current_point->pos.y);
// 							i = current_point->num_triangles;	// break outside loop
// 							break;
// 						}
// 					}
// 				}
// 			}
// 		}
// 	} while(current_point != first_point);
// 	return data_array;
// }

// DataArray2 * get_dur_limits_for_dep_from_point_list(DataArray2 *edges_array, double jd_dep) {
// 	DataArray2 *limits_inv = data_array2_create();
// 	Vector2 *edges = data_array2_get_data(edges_array);
// 	for(int i = 0; i < data_array2_size(edges_array); i++) {
// 		Vector2 p0 = edges[i];
// 		Vector2 p1 = edges[(i+1)%data_array2_size(edges_array)];
//
// 		if(p1.x < p0.x) {
// 			Vector2 temp = p0;
// 			p0 = p1;
// 			p1 = temp;
// 		}
//
// 		if(p0.x > jd_dep || p1.x < jd_dep) continue;
// 		if(p0.x == jd_dep) {
// 			data_array2_insert_new(limits_inv, p0.y, p0.x);
// 			continue;
// 		}
// 		if(p1.x == jd_dep) {
// 			data_array2_insert_new(limits_inv, p1.y, p1.x);
// 			continue;
// 		}
//
// 		double m = (p1.y-p0.y)/(p1.x-p0.x);
// 		double dur = (jd_dep - p0.x)*m + p0.y;
// 		data_array2_insert_new(limits_inv, dur, jd_dep);
// 	}
//
// 	DataArray2 *limits = data_array2_create();
// 	Vector2 *limits_inv_data = data_array2_get_data(limits_inv);
// 	double last = NAN;
// 	for(int i = 0; i < data_array2_size(limits_inv); i++) {
// 		if(limits_inv_data[i].x != last) {
// 			data_array2_insert_new(limits, jd_dep, limits_inv_data[i].x);
// 			last = limits_inv_data[i].x;
// 		}
// 	}
// 	data_array2_free(limits_inv);
//
// 	return limits;
// }

// double root_finder_almost_monot_deriv_next_x(DataArray2 *arr, int branch) {
// 	// branch = 0 for left branch, 1 for right branch
// 	Vector2 *data = data_array2_get_data(arr);
// 	int num_data = (int) data_array2_size(arr);
//
// 	int index;
//
// 	// left branch
// 	if(branch == 0) {
// 		index = 0;
// 		for(int i = 1; i < num_data; i++) {
// 			if(data[i].y < 0)	{ index = i; break; }
// 			else 				{ index = i; }
// 		}
//
// 		// right branch
// 	} else {
// 		index = num_data-1;
// 		for(int i = num_data-2; i >= 0; i--) {
// 			if(data[i].y < 0)	{ index =   i; break; }
// 			else 				{ index =   i; }
// 		}
// 	}
//
// 	if(branch == 0) return (data[index].x + data[index-1].x)/2;
// 	else 			return (data[index].x + data[index+1].x)/2;
// }

// void get_dur_limit_wrt_vinf(Mesh2 *mesh, double jd_dep, double min_vinf, DataArray2 *init_limit_array, DataArray2 *new_limits, double tolerance) {
// 	Vector2 *init_lim = data_array2_get_data(init_limit_array);
// 	size_t num_init_lim = data_array2_size(init_limit_array);
// 	if(num_init_lim == 0) return;
// 	if(num_init_lim == 1) {
// 		double dvinf = get_mesh_interpolated_value(mesh, vec2(jd_dep, init_lim[0].y), MESH_VAL_VINF) - min_vinf;
// 		if(dvinf > 0) data_array2_insert_new(new_limits, init_lim[0].x, init_lim[0].y);
// 		return;
// 	}
// 	DataArray2 *new_limits_inv = data_array2_create();
//
// 	bool left_branch = true;
// 	DataArray2 *data = data_array2_create();
//
// 	for(int lim_idx = 0; lim_idx < num_init_lim; lim_idx+=2) {
// 		double lim0 = init_lim[lim_idx].y+1e-9;		// floating precision
// 		double lim1 = init_lim[lim_idx+1].y-1e-9;	// floating precision
//
// 		double dur = lim0;
//
// 		for(int i = 0; i < 100; i++) {
// 			double dvinf = get_mesh_interpolated_value(mesh, vec2(jd_dep, dur), MESH_VAL_VINF) - min_vinf;
// 			if(i > 3 && dvinf > 0 && dvinf < tolerance) {
// 				data_array2_insert_new(new_limits_inv, dur, jd_dep);
// 				if(left_branch && data_array2_get_data(data)[data_array2_size(data)-1].y > 0) {
// 					left_branch = false;
// 				} else {
// 					break;
// 				}
// 			}
//
// 			data_array2_insert_new(data, dur, dvinf);
//
// 			if(i == 0) {
// 				if(dvinf > 0) {
// 					data_array2_insert_new(new_limits_inv, dur, jd_dep);
// 				} else {
// 					left_branch = false;
// 				}
// 				dur = lim1;
// 				continue;
// 			}
// 			if(i == 1) {
// 				if(dvinf > 0) data_array2_insert_new(new_limits_inv, dur, jd_dep);
// 				else if(!left_branch) break;
// 			}
// 			if(!can_be_negative_monot_deriv(data)) break;
// 			if(i < 30) dur = root_finder_monot_deriv_next_x(data, !left_branch);
// 			else dur = root_finder_almost_monot_deriv_next_x(data, !left_branch);
// 		}
// 	}
//
// 	for(int i = 0; i < data_array2_size(new_limits_inv); i++) {
// 		data_array2_append_new(new_limits, data_array2_get_data(new_limits_inv)[i].y, data_array2_get_data(new_limits_inv)[i].x);
// 	}
// 	data_array2_free(data);
// 	data_array2_free(new_limits_inv);
// }

// Vector3 get_varr_from_mesh(Mesh2 *mesh, double jd_arr, double dur) {
// 	MeshTriangle2 *triangle = get_mesh_triangle_at_position(mesh, vec2(jd_arr, dur));
// 	if(!triangle) return vec3(NAN, NAN, NAN);
//
// 	Vector3 tri_varrx[3];
// 	Vector3 tri_varry[3];
// 	Vector3 tri_varrz[3];
//
// 	for(int i = 0; i < 3; i++) {
// 		tri_varrx[i] = vec3(triangle->points[i]->pos.x, triangle->points[i]->pos.y, triangle->points[i]->val[MESH_VAL_ARRX]);
// 		tri_varry[i] = vec3(triangle->points[i]->pos.x, triangle->points[i]->pos.y, triangle->points[i]->val[MESH_VAL_ARRY]);
// 		tri_varrz[i] = vec3(triangle->points[i]->pos.x, triangle->points[i]->pos.y, triangle->points[i]->val[MESH_VAL_ARRZ]);
// 	}
// 	double varrx = get_triangle_interpolated_value(tri_varrx[0], tri_varrx[1], tri_varrx[2], vec2(jd_arr, dur));
// 	double varry = get_triangle_interpolated_value(tri_varry[0], tri_varry[1], tri_varry[2], vec2(jd_arr, dur));
// 	double varrz = get_triangle_interpolated_value(tri_varrz[0], tri_varrz[1], tri_varrz[2], vec2(jd_arr, dur));
// 	return vec3(varrx, varry, varrz);
// }

// Vector3 get_vbody_from_mesh(Mesh2 *mesh, double jd_arr, double dur) {
// 	MeshTriangle2 *triangle = get_mesh_triangle_at_position(mesh, vec2(jd_arr, dur));
// 	if(!triangle) return vec3(NAN, NAN, NAN);
//
// 	Vector3 tri_body_vx[3];
// 	Vector3 tri_body_vy[3];
// 	Vector3 tri_body_vz[3];
//
// 	for(int i = 0; i < 3; i++) {
// 		tri_body_vx[i] = vec3(triangle->points[i]->pos.x, triangle->points[i]->pos.y, triangle->points[i]->val[MESH_VAL_BODY_VX]);
// 		tri_body_vy[i] = vec3(triangle->points[i]->pos.x, triangle->points[i]->pos.y, triangle->points[i]->val[MESH_VAL_BODY_VY]);
// 		tri_body_vz[i] = vec3(triangle->points[i]->pos.x, triangle->points[i]->pos.y, triangle->points[i]->val[MESH_VAL_BODY_VZ]);
// 	}
// 	double varrx = get_triangle_interpolated_value(tri_body_vx[0], tri_body_vx[1], tri_body_vx[2], vec2(jd_arr, dur));
// 	double varry = get_triangle_interpolated_value(tri_body_vy[0], tri_body_vy[1], tri_body_vy[2], vec2(jd_arr, dur));
// 	double varrz = get_triangle_interpolated_value(tri_body_vz[0], tri_body_vz[1], tri_body_vz[2], vec2(jd_arr, dur));
// 	return vec3(varrx, varry, varrz);
// }

// struct ItinStep * get_next_step_from_vinf(SegmentGroup *group, double v_inf, double jd_dep, double min_dur_dt, double max_dur_dt, bool leftside, double tolerance) {
// 	OSV osv_dep = group->system->prop_method == ORB_ELEMENTS ?
// 					osv_from_elements(group->dep_body->orbit, jd_dep) :
// 					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);
//
// 	double dt0 = min_dur_dt, dt1 = max_dur_dt;
//
// 	// x: dt, y: diff_vinf
// 	DataArray2 *data = data_array2_create();
//
// 	double t0 = jd_dep;
// 	double last_dt, dt = dt0, t1, diff_vinf;
//
// 	for(int i = 0; i < 100; i++) {
// 		if(i == 0) dt = dt0;
//
// 		t1 = t0 + dt / 86400;
//
// 		OSV osv_arr = group->system->prop_method == ORB_ELEMENTS ?
// 				osv_from_elements(group->arr_body->orbit, t1) :
// 				osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, t1, group->system->cb);
//
// 		Lambert3 new_transfer = calc_lambert3(osv_dep.r, osv_arr.r, dt, group->system->cb);
// 		Vector3 v_dep = subtract_vec3(new_transfer.v0, osv_dep.v);
// 		diff_vinf = mag_vec3(v_dep) - v_inf;
//
// 		if(fabs(diff_vinf) < tolerance) {
// 			struct ItinStep *new_step = malloc(sizeof(struct ItinStep));
// 			new_step->body = group->arr_body;
// 			new_step->date = t1;
// 			new_step->r = osv_arr.r;
// 			new_step->v_dep = new_transfer.v0;
// 			new_step->v_arr = new_transfer.v1;
// 			new_step->v_body = osv_arr.v;
// 			new_step->num_next_nodes = 0;
// 			new_step->prev = NULL;
// 			new_step->next = NULL;
// 			return new_step;
// 		}
//
// 		data_array2_insert_new(data, dt, diff_vinf);
//
// 		if(!can_be_negative_monot_deriv(data)) break;
// 		last_dt = dt;
// 		if(i == 0) dt = dt1;
// 		else dt = root_finder_monot_deriv_next_x(data, !leftside);
// 		if(i > 3 && dt == last_dt) break;	// step size 0 (imprecision)
// 		if(isnan(dt) || isinf(dt)) break;
// 	}
//
// 	data_array2_free(data);
// 	return NULL;
// }

// DataArray2 * get_vinf_limits(Mesh2 *mesh, DataArray2 *vinf_array, double tolerance) {
// 	int num_deps = 1000;
//
// 	DataArray2 *edges_array = get_dur_limits_from_edge_triangles(mesh);
//
// 	DataArray2 *vinf_limits_all = data_array2_create();
// 	DataArray2 *vinf_limit_jd_dep = data_array2_create();
//
// 	double epsilon = 1e-6;
// 	double step = (mesh->mesh_box->max.x - mesh->mesh_box->min.x)/num_deps;
// 	double jd_dep = mesh->mesh_box->min.x+epsilon;
//
// 	while(jd_dep < mesh->mesh_box->max.x) {
// 		double jd_vinf_dep = interpolate_from_sorted_data_array2(vinf_array, jd_dep);
//
// 		if(isnan(jd_vinf_dep)) {
// 			jd_dep += step;
// 			continue;
// 		}
// 		data_array2_clear(vinf_limit_jd_dep);
// 		DataArray2 *limits = get_dur_limits_for_dep_from_point_list(edges_array, jd_dep);
// 		get_dur_limit_wrt_vinf(mesh, jd_dep, jd_vinf_dep-tolerance*2, limits, vinf_limit_jd_dep, 1);
// 		data_array2_free(limits);
//
// 		if(data_array2_size(vinf_limit_jd_dep)%2 != 0) {
// 			jd_dep += epsilon;
// 			continue;
// 		}
//
// 		if(data_array2_size(vinf_limit_jd_dep) == 0) {
// 			// add a flagged pair
// 			data_array2_append_new(vinf_limits_all, jd_dep, NAN);
// 			data_array2_append_new(vinf_limits_all, jd_dep, NAN);
// 		}
//
// 		for(int j = 0; j < data_array2_size(vinf_limit_jd_dep); j++) {
// 			data_array2_append_new(vinf_limits_all, data_array2_get_data(vinf_limit_jd_dep)[j].x, data_array2_get_data(vinf_limit_jd_dep)[j].y);
// 		}
// 		if(jd_dep + step >= mesh->mesh_box->max.x && mesh->mesh_box->max.x - jd_dep >= 2*epsilon) {
// 			jd_dep = mesh->mesh_box->max.x - epsilon;
// 		} else jd_dep += step;
// 	}
//
// 	data_array2_free(edges_array);
// 	data_array2_free(vinf_limit_jd_dep);
//
// 	return vinf_limits_all;
// }

// int get_num_interval_per_dep(Vector2 *limits, int limit_idx0) {
// 	if(isnan(limits[limit_idx0*2].y)) return 0;
// 	int num_interval = 1;
// 	int limit_idx = limit_idx0+1;
// 	while(limits[limit_idx0*2].x == limits[limit_idx*2].x) {
// 		num_interval++;
// 		limit_idx++;
// 	}
// 	return num_interval;
// }