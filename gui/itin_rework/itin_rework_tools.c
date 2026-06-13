#include "itin_rework_tools.h"

#include "gui/gui_manager.h"
#include "gui/gui_tools/screen.h"
#include "gui/drawing.h"
#include "geometrylib.h"
#include <math.h>

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

double calc_next_x_find_min(DataArray2 *arr, double tolerance) {
	Vector2 *data = data_array2_get_data(arr);
	size_t num_data = data_array2_size(arr);

	int min_idx = (int) num_data-1;

	for(int i = 0; i < num_data-1; i++) {
		if(data[i].y < data[i+1].y) { min_idx = i; break; }
	}

	double m_left = (data[min_idx].y - data[min_idx-1].y)/(data[min_idx].x - data[min_idx-1].x);
	double m_right = (data[min_idx+1].y - data[min_idx].y)/(data[min_idx+1].x - data[min_idx].x);

	double left_guess = (data[min_idx-1].x - data[min_idx].x) * m_right + data[min_idx].y;
	double right_guess = (data[min_idx+1].x - data[min_idx].x) * m_left + data[min_idx].y;

	double dleft = fabs(left_guess - data[min_idx-1].y);
	double dright = fabs(right_guess - data[min_idx+1].y);

	if( min_idx == 0 && dright < tolerance ||
		min_idx == num_data-1 && dleft < tolerance ||
		dleft < tolerance && dright < tolerance) return -1;

	if(min_idx == 0) return data[0].x + (data[1].x - data[0].x)*0.2;
	if(min_idx == num_data-1) return data[num_data-1].x - (data[num_data-1].x - data[num_data-2].x)*0.2;

	if(dleft > dright) return data[min_idx].x - (data[min_idx].x - data[min_idx-1].x)*0.2;
	else return data[min_idx].x + (data[min_idx+1].x - data[min_idx].x)*0.2;
}

void find_root(OSV osv_dep, double jd_dep, Body *dep_body, Body *arr_body, CelestSystem *system, double dt0, double dt1, double max_depdv, double dep_periapsis, double *left_x, double *right_x, double tol) {
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
		double dv_dep = dv_circ(dep_body,alt2radius(dep_body, dep_periapsis),vinf);

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


		data_array2_insert_new(data, dt, dv_dep - max_depdv);

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

		if(!can_be_negative_monot_deriv(data)) break;
		if(i > 3 && dt == last_dt) break;	// step size 0 (imprecision)
		last_dt = dt;
		if(i == 0) dt = dt1;
		else dt = root_finder_monot_deriv_next_x(data, !left_branch);
		if(isnan(dt) || isinf(dt)) break;
	}
	// print_data_array2(data, "dt", "dv");
	data_array2_free(data);
}

DataArray2 * find_root2(double jd_dep, Body *dep_body, Body *arr_body, CelestSystem *system, double dt0, double dt1, double max_depdv, double dep_periapsis, double *left_x, double *right_x, double tol) {
	// x: dt, y: diff_vinf
	DataArray2 *data = data_array2_create();

	double t0 = jd_dep;
	double last_dt = -1e20, dt, t1;
	*left_x = 0;
	*right_x = 0;
	bool left_branch = true;

	OSV osv_dep = system->prop_method == ORB_ELEMENTS ?
			osv_from_elements(dep_body->orbit, jd_dep) :
			osv_from_ephem(dep_body->ephem, dep_body->num_ephems, jd_dep, system->cb);

	if(dt0 < 100) dt0 = 100;	// transfer duration of 100s
	dt = dt0;

	for(int i = 0; i < 100; i++) {
		t1 = t0 + dt / 86400;

		OSV osv_arr = system->prop_method == ORB_ELEMENTS ?
				osv_from_elements(arr_body->orbit, t1) :
				osv_from_ephem(arr_body->ephem, arr_body->num_ephems, t1, system->cb);

		Lambert3 new_transfer = calc_lambert3(osv_dep.r, osv_arr.r, dt, system->cb);
		double vinf = fabs(mag_vec3(subtract_vec3(new_transfer.v0, osv_dep.v)));
		double dv_dep = dv_circ(dep_body,alt2radius(dep_body, dep_periapsis),vinf);

		if(i > 3 && max_depdv - dv_dep > 0 && max_depdv - dv_dep < tol || (i > 3 && fabs(dt-last_dt) < tol)) {
			if(left_branch) {
				*left_x = dt;
				last_dt = -1e20;
				left_branch = false;
				if(right_x != 0) break;
			} else {
				*right_x = dt;
				break;
			}
		}


		data_array2_insert_new(data, dt, dv_dep - max_depdv);

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

		if(!can_be_negative_monot_deriv(data)) break;
		if(i > 3 && dt == last_dt) break;	// step size 0 (imprecision)
		last_dt = dt;
		if(i == 0) dt = dt1;
		else dt = root_finder_monot_deriv_next_x(data, !left_branch);
		if(isnan(dt) || isinf(dt)) break;
	}
	// print_data_array2(data, "dt", "dv");
	return data;
}

DataArray2 * find_local_peak_array(double jd_dep, Body *dep_body, Body *arr_body, CelestSystem *system, double dt0, double dt1, double tol) {
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


		data_array2_insert_new(array, t1-t0, vinf);
	}

	double last_dt = -1e20;

	for(int i = 0; i < 100; i++) {
		double t1 = t0 + dt/86400;

		OSV osv_arr = system->prop_method == ORB_ELEMENTS ?
				osv_from_elements(arr_body->orbit, t1) :
				osv_from_ephem(arr_body->ephem, arr_body->num_ephems, t1, system->cb);

		Lambert3 new_transfer = calc_lambert3(osv_dep.r, osv_arr.r, (t1-t0)*86400, system->cb);
		double vinf = fabs(mag_vec3(subtract_vec3(new_transfer.v0, osv_dep.v)));


		data_array2_insert_new(array, t1-t0, vinf);
		last_dt = dt;

		Vector2 *data = data_array2_get_data(array);

		int max_idx = 0;
		double max_val = -1e20;
		for(int j = 1; j < data_array2_size(array)-1; j++) {
			if(data[j].y > max_val && data[j].y > data[j-1].y && data[j].y > data[j+1].y) { max_idx = j; max_val = data[j].y; }
		}
		if(max_val < 0) {
			print_data_array2(array, "dur", "dv");
		}
		if(max_idx == data_array2_size(array)-1) dt = (data[max_idx].x+data[max_idx-1].x)/2;
		else if(max_idx == 0) dt = (data[max_idx].x+data[max_idx+1].x)/2;
		else dt = (data[max_idx].x+data[max_idx-((i % 2 == 0) ? 1 : -1)].x)/2;
		dt *= 86400;

		if(fabs(last_dt-dt) < tol) break;
	}
	return array;
}


Vector2 get_local_peak(double jd_dep, Body *dep_body, Body *arr_body, CelestSystem *system, double dt0, double dt1, double tol) {
	DataArray2 *array = find_local_peak_array(jd_dep, dep_body, arr_body, system, dt0, dt1, tol);

	Vector2 *data = data_array2_get_data(array);
	int max_idx = 0;
	double max_val = -1e20;
	for(int j = 1; j < data_array2_size(array)-1; j++) {
		if(data[j].y > max_val && data[j].y > data[j-1].y && data[j].y > data[j+1].y) { max_idx = j; max_val = data[j].y; }
	}

	if(max_val < 0) {
		printf("test\n");
	}

	Vector2 peak = data[max_idx];

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

void calc_bounded_porkchop_line(struct ItinStep *departure_step, Body *arr_body, CelestSystem *system, DataArray1 *dur_array, double min_dt, double max_dt, double dep_periapsis, double max_depdv, double dv_tolerance) {
	Body *dep_body = departure_step->body;
	double jd_dep = departure_step->date;
	OSV osv0 = system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(dep_body->orbit, jd_dep) :
					osv_from_ephem(dep_body->ephem, dep_body->num_ephems, jd_dep, system->cb);

	double dt = min_dt;

	DataArray2 *data_dep = data_array2_create();
	DataArray2 *data_arr = data_array2_create();

	struct ItinStep *curr_step = departure_step;
	curr_step->r = osv0.r;
	curr_step->v_body = osv0.v;
	curr_step->v_dep = vec3(0, 0, 0);
	curr_step->v_arr = vec3(0, 0, 0);
	curr_step->num_next_nodes = 0;
	curr_step->prev = NULL;
	curr_step->next = (struct ItinStep **) malloc(1000 * sizeof(struct ItinStep *));
	int counter = 0;

	for(int j = 0; j < 1000; j++) {
		// printf("%f  %f  %f  %f  %f\n", min_dt, max_dt, dt0, dt1, dt);

		double jd_arr = jd_dep + dt / 86400;

		OSV osv_arr = system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(arr_body->orbit, jd_arr) :
					osv_from_ephem(arr_body->ephem, arr_body->num_ephems, jd_arr, system->cb);

		Lambert3 tf = calc_lambert3(osv0.r, osv_arr.r, (jd_arr - jd_dep) * 86400, system->cb);

		double vinf_dep = fabs(mag_vec3(subtract_vec3(tf.v0, osv0.v)));
		double dv_dep = dv_circ(dep_body,alt2radius(dep_body, dep_periapsis),vinf_dep);
		double vinf_arr = fabs(mag_vec3(subtract_vec3(tf.v1, osv_arr.v)));

		// j != 3 to skip initial accuracy point for line propagation
		if(dv_dep <= max_depdv && j != 3) {
			curr_step = get_first(curr_step);
			// sort chronologically
			int insert_index = counter;
			while(insert_index > 0) {
				if(curr_step->next[insert_index-1]->date < jd_arr) break;
				insert_index--;
			}
			if(insert_index != counter) {
				memmove(&curr_step->next[insert_index+1],
					&curr_step->next[insert_index],
					(counter+2 - insert_index) * sizeof(*curr_step->next));
			}

			curr_step->next[insert_index] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
			curr_step->next[insert_index]->prev = curr_step;
			curr_step->next[insert_index]->next = NULL;
			curr_step = curr_step->next[insert_index];

			curr_step->body = arr_body;
			curr_step->date = jd_arr;
			curr_step->r = osv_arr.r;
			curr_step->v_dep = tf.v0;
			curr_step->v_arr = tf.v1;
			curr_step->v_body = osv_arr.v;
			curr_step->num_next_nodes = 0;
			curr_step->prev->num_next_nodes++;
			counter++;

			if(dur_array) data_array1_insert_new(dur_array, dt/86400);
		}

		data_array2_insert_new(data_dep, dt/86400, dv_dep);
		data_array2_insert_new(data_arr, dt/86400, vinf_arr);

		if(dt == min_dt) dt = max_dt;
		else if(dt == max_dt) dt = ( dt + data_array2_get_data(data_dep)[0].x*86400 ) / 2;
		else {
			double next_dep_x = calc_next_x_wrt_smoothness(data_dep, 0, dv_tolerance)*86400;
			double next_arr_x = calc_next_x_wrt_smoothness(data_arr, 0, dv_tolerance)*86400;
			if(isnan(next_dep_x) && isnan(next_arr_x)) break;
			if(!isnan(next_dep_x) && isnan(next_arr_x) || next_dep_x < next_arr_x) dt = next_dep_x;
			else dt = next_arr_x;
		}
	}
	data_array2_free(data_dep);
	data_array2_free(data_arr);
}

void calc_coarse_bounded_porkchop_line(struct ItinStep *departure_step, Body *arr_body, CelestSystem *system, double min_dt, double max_dt, double dt_step, double dep_periapsis, double max_depdv, double dv_tolerance) {
	Body *dep_body = departure_step->body;
	double jd_dep = departure_step->date;
	OSV osv0 = system->prop_method == ORB_ELEMENTS ?
				osv_from_elements(dep_body->orbit, jd_dep) :
				osv_from_ephem(dep_body->ephem, dep_body->num_ephems, jd_dep, system->cb);

	int num_steps = (int) (max_dt-min_dt)/dt_step + 2;

	struct ItinStep *curr_step = departure_step;
	curr_step->r = osv0.r;
	curr_step->v_body = osv0.v;
	curr_step->v_dep = vec3(0, 0, 0);
	curr_step->v_arr = vec3(0, 0, 0);
	curr_step->num_next_nodes = 0;
	curr_step->prev = NULL;
	curr_step->next = (struct ItinStep **) malloc(num_steps * sizeof(struct ItinStep *));

	for(int i = 0; i < num_steps; i++) {
		double dt = min_dt + (max_dt-min_dt) * ((double) i/(num_steps-1));
		double jd_arr = jd_dep + dt / 86400;

		OSV osv_arr = system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(arr_body->orbit, jd_arr) :
					osv_from_ephem(arr_body->ephem, arr_body->num_ephems, jd_arr, system->cb);

		Lambert3 tf = calc_lambert3(osv0.r, osv_arr.r, (jd_arr - jd_dep) * 86400, system->cb);

		curr_step = get_first(curr_step);
		curr_step->next[i] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
		curr_step->next[i]->prev = curr_step;
		curr_step->next[i]->next = NULL;
		curr_step = curr_step->next[i];

		curr_step->body = arr_body;
		curr_step->date = jd_arr;
		curr_step->r = osv_arr.r;
		curr_step->v_dep = tf.v0;
		curr_step->v_arr = tf.v1;
		curr_step->v_body = osv_arr.v;
		curr_step->num_next_nodes = 0;
		curr_step->prev->num_next_nodes++;
	}
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

void get_upper_and_lower_boundary_at_jd_dep(SegmentGroup *group, double jd_dep, double *lower_boundary, double *upper_boundary) {
	double next_opposition_dt, next_conjunction_dt, opp_guess, conj_guess;
	OSV osv0 = group->system->prop_method == ORB_ELEMENTS ?
				osv_from_elements(group->dep_body->orbit, jd_dep) :
				osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);

	OSV osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
				   osv_from_elements(group->arr_body->orbit, jd_dep) :
				   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_dep, group->system->cb);
	double period_arr0 = calc_orbital_period(constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, group->system->cb));
	calc_time_to_next_conjunction_and_opposition(osv0.r, osv_arr0, group->system->cb, &next_conjunction_dt, &next_opposition_dt);

	if(group->top_boundary_type == DEPARTURE_GROUP_BOUNDARY_TOP_OPP) {
		opp_guess = interpolate_from_sorted_data_array(group->upper_boundary, jd_dep)*86400;
		conj_guess = interpolate_from_sorted_data_array(group->lower_boundary, jd_dep)*86400;
	} else {
		conj_guess = interpolate_from_sorted_data_array(group->upper_boundary, jd_dep)*86400;
		opp_guess = interpolate_from_sorted_data_array(group->lower_boundary, jd_dep)*86400;
	}

	while(opp_guess-next_opposition_dt   >  0.5 * period_arr0) next_opposition_dt  += period_arr0;
	while(opp_guess-next_opposition_dt   < -0.5 * period_arr0) next_opposition_dt  -= period_arr0;
	while(conj_guess-next_conjunction_dt >  0.5 * period_arr0) next_conjunction_dt += period_arr0;
	while(conj_guess-next_conjunction_dt < -0.5 * period_arr0) next_conjunction_dt -= period_arr0;

	if(next_conjunction_dt < next_opposition_dt) {
		*lower_boundary = next_conjunction_dt;
		*upper_boundary = next_opposition_dt;
	} else {
		*lower_boundary = next_opposition_dt;
		*upper_boundary = next_conjunction_dt;
	}
}

void set_opposition_conjunction_group_boundary(SegmentGroup *group, int shift, double jd_min_dep, double jd_max_dep, double min_dur, double max_dur) {
	group->lower_boundary = data_array2_create();
	group->upper_boundary = data_array2_create();

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

	double local_peak_half_width_dt = period_arr0*0.001;

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
			if(fabs(jd_dep - data_array1_get_data(traversals)[j]) < period_dep/86400*0.01) {
				if(next_opposition_dt + local_peak_half_width_dt >= min_dur*86400 && next_opposition_dt - local_peak_half_width_dt <= max_dur*86400)
					next_opposition_dt = get_local_peak(jd_dep, group->dep_body, group->arr_body, group->system, next_opposition_dt-local_peak_half_width_dt, next_opposition_dt+local_peak_half_width_dt, 1).x*86400;
				// if(next_conjunction_dt + local_peak_half_width_dt >= min_dur*86400 && next_conjunction_dt - local_peak_half_width_dt <= max_dur*86400)
				// 	next_conjunction_dt = get_local_peak(jd_dep, group->dep_body, group->arr_body, group->system, next_conjunction_dt-local_peak_half_width_dt, last_conjunction_dt+local_peak_half_width_dt, 1).x*86400;
				break;
			}
		}


		last_opposition_dt = next_opposition_dt;
		last_conjunction_dt = next_conjunction_dt;
		if(next_conjunction_dt < next_opposition_dt) {
			data_array2_append_new(group->lower_boundary, jd_dep, next_conjunction_dt/86400);
			data_array2_append_new(group->upper_boundary, jd_dep, next_opposition_dt/86400);
		} else {
			data_array2_append_new(group->lower_boundary, jd_dep, next_opposition_dt/86400);
			data_array2_append_new(group->upper_boundary, jd_dep, next_conjunction_dt/86400);
		}
	}
	group->top_boundary_type = next_conjunction_dt < next_opposition_dt ?
	DEPARTURE_GROUP_BOUNDARY_TOP_OPP : DEPARTURE_GROUP_BOUNDARY_TOP_CONJ;

	data_array1_free(boundary_points);
	data_array1_free(traversals);
}

void calc_coarse_group_porkchop(SegmentGroup *group, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, int num_duration_steps, double dep_periapsis, double max_depdv, double dv_tolerance) {
	group->segment_steps = malloc(10000 * sizeof(struct ItinStep*));

	OSV osv0 = group->system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(group->dep_body->orbit, jd_min_dep) :
					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_min_dep, group->system->cb);

	OSV osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
				   osv_from_elements(group->arr_body->orbit, jd_min_dep) :
				   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_min_dep, group->system->cb);
	Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, group->system->cb);
	double dt0, dt1;


	double r0 = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb).a, r1 = arr0.a;
	double r_ratio =  r1/r0;
	Hohmann hohmann = calc_hohmann_transfer(r0, r1, group->system->cb);
	double hohmann_dur = hohmann.dur/86400;
	double min_duration = 0.4 * hohmann_dur;
	double max_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_duration/hohmann_dur > 3) max_duration = hohmann_dur*3;
	if(max_duration < max_dur) max_dur = max_duration;
	if(min_duration > min_dur) min_dur = min_duration;

	double min_dt = min_dur*86400;
	double max_dt = max_dur*86400;
	double jd_dep_step = 5;
	double jd_dep = jd_min_dep;

	while(jd_dep < jd_max_dep) {
		// print_date(convert_JD_date(jd_min_dep, DATE_ISO), 0);
		// printf("\t");
		// print_date(convert_JD_date(jd_dep, DATE_ISO), 0);
		// printf("\t");
		// print_date(convert_JD_date(jd_max_dep, DATE_ISO), 1);
		get_upper_and_lower_boundary_at_jd_dep(group, jd_dep, &dt0, &dt1);
		double dt_step = (dt1-dt0) / num_duration_steps;

		if(dt0 > max_dt || dt1 < min_dt) {jd_dep += jd_dep_step; continue;}

		if(dt0 < min_dur*86400) dt0 = min_dur*86400;
		if(dt1 > max_dur*86400) dt1 = max_dur*86400;

		group->segment_steps[group->num_steps] = malloc(sizeof(struct ItinStep));
		group->segment_steps[group->num_steps]->body = group->dep_body;
		group->segment_steps[group->num_steps]->date = jd_dep;
		calc_coarse_bounded_porkchop_line(group->segment_steps[group->num_steps], group->arr_body, group->system, dt0, dt1, dt_step, dep_periapsis, max_depdv, dv_tolerance);
		group->num_steps++;
		if(jd_dep >= jd_max_dep) break;
		jd_dep += dt_step/fabs(group->boundary_gradient)/86400;
		if(jd_dep >= jd_max_dep) jd_dep = jd_max_dep;
	}
}

MeshPoint2 * create_mesh_point_for_porkchop_mesh(Body *dep_body, Body *arr_body, CelestSystem *system, double jd_dep, double dur) {
	double jd_arr = jd_dep + dur;
	OSV osv0 = system->prop_method == ORB_ELEMENTS ?
				osv_from_elements(dep_body->orbit, jd_dep) :
				osv_from_ephem(dep_body->ephem, dep_body->num_ephems, jd_dep, system->cb);
	struct ItinStep *curr_step = malloc(sizeof(struct ItinStep));
	curr_step->body = dep_body;
	curr_step->date = jd_dep;
	curr_step->r = osv0.r;
	curr_step->v_body = osv0.v;
	curr_step->v_dep = vec3(0, 0, 0);
	curr_step->v_arr = vec3(0, 0, 0);
	curr_step->num_next_nodes = 0;
	curr_step->prev = NULL;
	curr_step->next = (struct ItinStep **) malloc(sizeof(struct ItinStep *));

	OSV osv_arr = system->prop_method == ORB_ELEMENTS ?
				osv_from_elements(arr_body->orbit, jd_arr) :
				osv_from_ephem(arr_body->ephem, arr_body->num_ephems, jd_arr, system->cb);

	Lambert3 tf = calc_lambert3(osv0.r, osv_arr.r, (jd_arr - jd_dep) * 86400, system->cb);


	double *point_vals = malloc(NUM_PORKCHOP_MESH_VALUE_TYPES * sizeof(double));
	Vector3 vinf_dep = subtract_vec3(tf.v0, osv0.v);
	Vector3 vinf_arr = subtract_vec3(tf.v1, osv_arr.v);
	point_vals[MESH_VAL_DATE] = jd_arr;
	point_vals[MESH_VAL_DEPX] = vinf_dep.x;
	point_vals[MESH_VAL_DEPY] = vinf_dep.y;
	point_vals[MESH_VAL_DEPZ] = vinf_dep.z;
	point_vals[MESH_VAL_BODY_RX] = osv_arr.r.x;
	point_vals[MESH_VAL_BODY_RY] = osv_arr.r.y;
	point_vals[MESH_VAL_BODY_RZ] = osv_arr.r.z;
	point_vals[MESH_VAL_BODY_VX] = osv_arr.v.x;
	point_vals[MESH_VAL_BODY_VY] = osv_arr.v.y;
	point_vals[MESH_VAL_BODY_VZ] = osv_arr.v.z;
	point_vals[MESH_VAL_ARRX] = vinf_arr.x;
	point_vals[MESH_VAL_ARRY] = vinf_arr.y;
	point_vals[MESH_VAL_ARRZ] = vinf_arr.z;
	point_vals[MESH_VAL_VINF] = mag_vec3(vinf_arr);
	point_vals[MESH_VAL_RPE] = 1e9;
	MeshPoint2 *new_point = create_mesh_point(vec2(jd_dep, dur), point_vals, NUM_PORKCHOP_MESH_VALUE_TYPES);

	return new_point;
}

MeshTriangleBoundaryCondition get_triangle_dep_dv_boundary_condition(MeshTriangle2 *triangle, Body *dep_body, double dep_periapsis, double max_depdv, double dv_tolerance) {
	bool inside[3] = {false, false, false};
	for(int i = 0; i < 3; i++) {
		double vinf = triangle->points[i]->val[MESH_VAL_VINF];
		double depdv = dv_circ(dep_body, dep_body->radius+dep_periapsis, vinf);
		if(depdv < max_depdv+dv_tolerance) inside[i] = true;
	}
	if( inside[0] &&  inside[1] &&  inside[2]) return TRIANGLE_INSIDE_BOUNDARY;
	if(!inside[0] && !inside[1] && !inside[2]) return TRIANGLE_OUTSIDE_BOUNDARY;
	return TRIANGLE_CROSSING_BOUNDARY;
}

// MeshTriangleBoundaryCondition get_triangle_group_boundary_condition(MeshTriangle2 *triangle, SegmentGroup *group) {
// 	bool inside[3] = {false, false, false};
// 	for(int i = 0; i < 3; i++) {
// 		struct ItinStep *ptr = triangle->points[i]->old_data;
// 		double jd_dep = get_first(ptr)->date;
// 		double dur = ptr->date - jd_dep;
// 		double upper_dur_boundary = interpolate_from_sorted_data_array(group->upper_boundary, jd_dep);
// 		double lower_dur_boundary = interpolate_from_sorted_data_array(group->lower_boundary, jd_dep);
// 		if(dur <= upper_dur_boundary && dur >= lower_dur_boundary) inside[i] = true;
// 	}
// 	if( inside[0] &&  inside[1] &&  inside[2]) return TRIANGLE_INSIDE_BOUNDARY;
// 	if(!inside[0] && !inside[1] && !inside[2]) return TRIANGLE_OUTSIDE_BOUNDARY;
// 	return TRIANGLE_CROSSING_BOUNDARY;
// }

// bool triangle_needs_dvdep_error_refinement(MeshTriangle2 *triangle, Body *dep_body, double dep_periapsis, double max_depdv, double dv_tolerance) {
//
// }

void split_mesh_triangle(Mesh2 *mesh, MeshTriangle2 *triangle, SegmentGroup *group) {
	int side_idx = 0;
	double max_side_lengths_sq = 0;

	for(int i = 0; i < 3; i++) {
		double side_length_sq = sq_mag_vec2(subtract_vec2(triangle->points[i]->pos, triangle->points[(i+1)%3]->pos));
		if(side_length_sq > max_side_lengths_sq) { max_side_lengths_sq = side_length_sq; side_idx = i; }
	}

	MeshTriangle2 *adj_triangle = triangle->adj_triangles[side_idx];
	if(adj_triangle && adj_triangle->rf_level == adj_triangle->target_rf_level) return;

	MeshPoint2 *t0p0 = triangle->points[side_idx];
	MeshPoint2 *t0p1 = triangle->points[(side_idx+1)%3];
	MeshPoint2 *t0p_opp = triangle->points[(side_idx+2)%3];

	Vector2 new_point_pos = scale_vec2(add_vec2(t0p0->pos, t0p1->pos), 0.5);
	MeshPoint2 *new_point = create_mesh_point_for_porkchop_mesh(group->dep_body, group->arr_body, group->system, new_point_pos.x, new_point_pos.y);
	add_point_to_mesh(mesh, new_point);

	remove_triangle_from_mesh(mesh, triangle, false);
	add_triangle_to_mesh(mesh, create_triangle_from_three_points_with_rf_level(new_point, t0p_opp, t0p0, triangle->rf_level+1, triangle->target_rf_level));
	add_triangle_to_mesh(mesh, create_triangle_from_three_points_with_rf_level(new_point, t0p_opp, t0p1, triangle->rf_level+1, triangle->target_rf_level));

	if(!adj_triangle) return;
	int adj_tri_opp_point_idx = 0;
	for(int i = 0; i < 3; i++) {
		if(adj_triangle->points[i] != t0p0 && adj_triangle->points[i] != t0p1) {
			adj_tri_opp_point_idx = i;
			break;
		}
	}

	MeshPoint2 *t1p0 = adj_triangle->points[(adj_tri_opp_point_idx+1)%3];
	MeshPoint2 *t1p1 = adj_triangle->points[(adj_tri_opp_point_idx+2)%3];
	MeshPoint2 *t1p_opp = adj_triangle->points[adj_tri_opp_point_idx];

	remove_triangle_from_mesh(mesh, adj_triangle, false);
	add_triangle_to_mesh(mesh, create_triangle_from_three_points_with_rf_level(new_point, t1p_opp, t1p0, adj_triangle->rf_level, adj_triangle->target_rf_level));
	add_triangle_to_mesh(mesh, create_triangle_from_three_points_with_rf_level(new_point, t1p_opp, t1p1, adj_triangle->rf_level, adj_triangle->target_rf_level));
}

int max_num_refines = 0;
int num_refines = 0;

void refine_porkchop_mesh_box(SegmentGroup *group, MeshBox2 *box, double dep_periapsis, double max_depdv, double dv_tolerance) {
	if(box->type == MESHBOX_SUBBOXES) {
		int num_subboxes = (int) box->subboxes.num;
		for(int i = 0; i < box->subboxes.num; i++) {
			refine_porkchop_mesh_box(group, box->subboxes.boxes[i], dep_periapsis, max_depdv, dv_tolerance);
			if(box->subboxes.num != num_subboxes) { i--; num_subboxes = (int) box->subboxes.num; }
		}
	} else if(box->type == MESHBOX_TRIANGLES) {
		for(int i = 0; i < box->tri.num; i++) {
			MeshTriangle2 *triangle = box->tri.triangles[i];
			MeshTriangleBoundaryCondition bc_depdv = get_triangle_dep_dv_boundary_condition(triangle, group->dep_body, dep_periapsis, max_depdv, dv_tolerance);
			MeshTriangleBoundaryCondition bc_group_boundary = false;//get_triangle_group_boundary_condition(triangle, group);

			if(bc_depdv == TRIANGLE_OUTSIDE_BOUNDARY || bc_group_boundary == TRIANGLE_OUTSIDE_BOUNDARY) {
				set_mesh_tri_flag(triangle, TRI_FLAG_INACTIVE);
			} else if(bc_depdv == TRIANGLE_CROSSING_BOUNDARY || bc_group_boundary == TRIANGLE_CROSSING_BOUNDARY) {
				set_mesh_tri_flag(triangle, TRI_FLAG_WANTS_REFINEMENT);
				triangle->target_rf_level = triangle->rf_level+1;
			}
		}
	}
}

void refine_porkchop_mesh(SegmentGroup *group, double dep_periapsis, double max_depdv, double dv_tolerance) {
	num_refines = 0;
	Mesh2 *mesh = group->mesh;
	// refine_porkchop_mesh_box(group, mesh->mesh_box, dep_periapsis, max_depdv, dv_tolerance);
	int max_level = 0;

	for(int c = 0; c < max_num_refines; c++) {
		for(int i = 0; i < mesh->num_triangles; i++) {
			MeshTriangle2 *triangle = mesh->triangles[i];
			MeshTriangleBoundaryCondition bc_depdv = get_triangle_dep_dv_boundary_condition(triangle, group->dep_body, dep_periapsis, max_depdv, dv_tolerance);
			MeshTriangleBoundaryCondition bc_group_boundary = false;//get_triangle_group_boundary_condition(triangle, group);

			remove_mesh_tri_flag(triangle, TRI_FLAG_WANTS_REFINEMENT);
			triangle->target_rf_level = triangle->rf_level;

			if(bc_depdv == TRIANGLE_OUTSIDE_BOUNDARY || bc_group_boundary == TRIANGLE_OUTSIDE_BOUNDARY) {
				set_mesh_tri_flag(triangle, TRI_FLAG_INACTIVE);
			} else if(bc_depdv == TRIANGLE_CROSSING_BOUNDARY || bc_group_boundary == TRIANGLE_CROSSING_BOUNDARY) {
				set_mesh_tri_flag(triangle, TRI_FLAG_WANTS_REFINEMENT);
				triangle->target_rf_level = triangle->rf_level+1;
				if(triangle->rf_level > max_level) { max_level = triangle->rf_level; }
			}
		}

		bool has_changed = false;
		do {
			has_changed = false;
			for(int i = 0; i < mesh->num_triangles; i++) {
				MeshTriangle2 *triangle = mesh->triangles[i];
				for(int j = 0; j < 3; j++) {
					if(triangle->adj_triangles[j]) {
						if(triangle->adj_triangles[j]->target_rf_level-1 > triangle->target_rf_level) {
							triangle->target_rf_level = triangle->adj_triangles[j]->target_rf_level-1;
							set_mesh_tri_flag(triangle, TRI_FLAG_WANTS_REFINEMENT);
							has_changed = true;
						}
					}
				}
			}
		} while(has_changed);

		for(int level = 0; level <= max_level; level++) {
			for(int i = 0; i < mesh->num_triangles; i++) {
				MeshTriangle2 *triangle = mesh->triangles[i];
				if(is_mesh_tri_flag(triangle, TRI_FLAG_WANTS_REFINEMENT) && triangle->rf_level <= level) split_mesh_triangle(mesh, triangle, group);
				if(triangle->rf_level == triangle->target_rf_level) remove_mesh_tri_flag(triangle, TRI_FLAG_WANTS_REFINEMENT);
			}
		}
	}

	for(int i = 0; i < mesh->num_triangles; i++) {
		MeshTriangle2 *triangle = mesh->triangles[i];
		MeshTriangleBoundaryCondition bc_depdv = get_triangle_dep_dv_boundary_condition(triangle, group->dep_body, dep_periapsis, max_depdv, dv_tolerance);
		MeshTriangleBoundaryCondition bc_group_boundary = false;//get_triangle_group_boundary_condition(triangle, group);

		if(bc_depdv == TRIANGLE_OUTSIDE_BOUNDARY || bc_group_boundary == TRIANGLE_OUTSIDE_BOUNDARY) {
			set_mesh_tri_flag(triangle, TRI_FLAG_INACTIVE);
			remove_triangle_from_mesh(mesh, triangle, true);
			i--;
		} else if(bc_depdv == TRIANGLE_CROSSING_BOUNDARY || bc_group_boundary == TRIANGLE_CROSSING_BOUNDARY) {
			set_mesh_tri_flag(triangle, TRI_FLAG_WANTS_REFINEMENT);
			triangle->target_rf_level = triangle->rf_level+1;
		}
	}
	max_num_refines++;
}

void update_mesh_triangle_status(SegmentGroup *group, double dv_tolerance) {
	Mesh2 *mesh = group->mesh;
	for(int i = 0; i < mesh->num_triangles; i++) {
		MeshTriangle2 *triangle = mesh->triangles[i];

		Vector2 tri_centroid = get_triangle_centroid(triangle);
		Vector3 p[3];
		for(int k = 0; k < 3; k++) {
			p[k].x = triangle->points[k]->pos.x;
			p[k].y = triangle->points[k]->pos.y;
			p[k].z = triangle->points[k]->val[MESH_VAL_VINF];
		}
		double center_val = get_triangle_interpolated_value(p[0], p[1], p[2], tri_centroid);

		for(int j = 0; j < 3; j++) {
			MeshTriangle2 *adj_triangle = triangle->adj_triangles[j];
			if(!adj_triangle) continue;
			for(int k = 0; k < 3; k++) {
				p[k].x = adj_triangle->points[k]->pos.x;
				p[k].y = adj_triangle->points[k]->pos.y;
				p[k].z = triangle->points[k]->val[MESH_VAL_VINF];
			}
			double interpolated_val = get_triangle_interpolated_value(p[0], p[1], p[2], tri_centroid);
			if(fabs(interpolated_val-center_val) > dv_tolerance) {
				set_mesh_tri_flag(triangle, TRI_FLAG_ACC_ERR);
				break;
			}
		}
	}
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
	printf("%f  %f\n", syn_period, group->boundary_gradient);
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

			dt0 = interpolate_from_sorted_data_array(group->lower_boundary, jd_dep) * 86400;
			dt1 = interpolate_from_sorted_data_array(group->upper_boundary, jd_dep) * 86400;

			if(dt0 > max_dt || dt1 < min_dt) {
				data_array2_insert_new(boundary_array, jd_dep, -1e20);
				data_array2_insert_new(boundary_array, jd_dep, -1e20);
				continue;
			}

			double left_x = 0, right_x = 0;
			osv0 = group->system->prop_method == ORB_ELEMENTS ?
								osv_from_elements(group->dep_body->orbit, jd_dep) :
								osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);
			find_root(osv0, jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, max_depdv, dep_periapsis, &left_x, &right_x, 0.01);

			// No departure possible within given constraints
			if(left_x < 1 && right_x < 1 || right_x < min_dur*86400 || left_x > max_dur*86400) {
				data_array2_insert_new(boundary_array, jd_dep, -1e20);
				data_array2_insert_new(boundary_array, jd_dep, -1e20);
				continue;
			}

			if(left_x < dt0) left_x = dt0;
			if(left_x < min_dur*86400) left_x = min_dur*86400;
			if(right_x > dt1) right_x = dt1;
			if(right_x > max_dur*86400) right_x = max_dur*86400;

			data_array2_insert_new(boundary_array, jd_dep, left_x/86400);
			data_array2_insert_new(boundary_array, jd_dep, right_x/86400);
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

			if(v1.x == v0.x || v1.x == v2.x) {
				printf("%f   %f   %f\n", v0.x-jd_min_dep, v1.x-jd_min_dep, v2.x-jd_min_dep);
			}

			double m0 = (v1.y - v0.y)/(v1.x - v0.x);
			double m1 = (v2.y - v1.y)/(v2.x - v1.x);

			double angle0 = atan(m0);
			double angle1 = atan(m1);

			double da = fabs(angle1 - angle0);

			if(da > deg2rad(5.0)) {
				printf("%f°    %f°  (%f)  %f°   (%f)\n", rad2deg(fabs(angle0-angle1)), rad2deg(angle0), m0, rad2deg(angle1), m1);
				if(fabs(v0.x-v1.x) > syn_period*0.00001) {
					data_array1_append_new(dep_points, (v0.x+v1.x)/2);
					data_array1_append_new(dep_temp, (v0.x+v1.x)/2 - jd_min_dep);
				}
				if(fabs(v1.x-v2.x) > syn_period*0.00001) {
					data_array1_append_new(dep_points, (v1.x+v2.x)/2);
					data_array1_append_new(dep_temp, (v1.x+v2.x)/2 - jd_min_dep);
				}
				i += 3 + (i%2==0);
			}

			if(isnan(da)) {
				printf("test\n");
			}
		}
		print_data_array1(dep_points, "dep");
		print_data_array1(dep_temp, "dep");
		printf("%lu\n", data_array2_size(boundary_array));
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
			if(isnan(data_array2_get_data(boundary_array)[i-1].y) && data_array2_get_data(boundary_array)[i+1].y) {
				data_array2_remove_at_idx(boundary_array, i);
				i--; continue;
			}
			data_array2_get_data(boundary_array)[i].y = NAN;
			continue;
		}

		if(i == 0) continue;
		if(i == data_array2_size(boundary_array)-1) continue;

		if(isnan(data_array2_get_data(boundary_array)[i-1].y)) {
			double new_dep = (data_array2_get_data(boundary_array)[i-1].x+data_array2_get_data(boundary_array)[i  ].x)/2;
			double new_dur = (data_array2_get_data(boundary_array)[i  ].y+data_array2_get_data(boundary_array)[i+1].y)/2;
			data_array2_insert_new(boundary_array, new_dep, new_dur);
			data_array2_insert_new(boundary_array, new_dep, new_dur);
		}
		if(data_array2_get_data(boundary_array)[i+1].y < -1e-19) {
			double new_dep = (data_array2_get_data(boundary_array)[i+1].x+data_array2_get_data(boundary_array)[i  ].x)/2;
			double new_dur = (data_array2_get_data(boundary_array)[i  ].y+data_array2_get_data(boundary_array)[i-1].y)/2;
			data_array2_insert_new(boundary_array, new_dep, new_dur);
			data_array2_insert_new(boundary_array, new_dep, new_dur);
			i+=2;
		}
	}

	if(isnan(data_array2_get_data(boundary_array)[0].y)) {
		data_array2_remove_at_idx(boundary_array, 0);
	}
	if(isnan(data_array2_get_data(boundary_array)[data_array2_size(boundary_array)-1].y)) {
		data_array2_remove_at_idx(boundary_array, (int) data_array2_size(boundary_array)-1);
	}
	// print_data_array2(boundary_array, "depdate", "dur");
	return boundary_array;
}

void calc_porkchop_dv_boundaries(SegmentGroup *group, int departure_cap, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance) {
	group->vinf_array = data_array2_create();

	OSV osv0 = group->system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(group->dep_body->orbit, jd_min_dep) :
					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_min_dep, group->system->cb);

	OSV osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
				   osv_from_elements(group->arr_body->orbit, jd_min_dep) :
				   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_min_dep, group->system->cb);
	Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, group->system->cb);
	double dt0, dt1;


	double r0 = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb).a, r1 = arr0.a;
	double r_ratio =  r1/r0;
	Hohmann hohmann = calc_hohmann_transfer(r0, r1, group->system->cb);
	double hohmann_dur = hohmann.dur/86400;
	double min_duration = 0.4 * hohmann_dur;
	double max_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_duration/hohmann_dur > 3) max_duration = hohmann_dur*3;
	if(max_duration < max_dur) max_dur = max_duration;
	if(min_duration > min_dur) min_dur = min_duration;

	double min_dt = min_dur*86400;
	double max_dt = max_dur*86400;
	double jd_dep_step = (jd_max_dep-jd_min_dep)/(departure_cap-1);

	for(int i = 0; i < departure_cap; i++) {
		double jd_dep = jd_min_dep + jd_dep_step*i;

		get_upper_and_lower_boundary_at_jd_dep(group, jd_dep, &dt0, &dt1);

		if(dt0 > max_dt || dt1 < min_dt) continue;

		double left_x = 0, right_x = 0;
		osv0 = group->system->prop_method == ORB_ELEMENTS ?
							osv_from_elements(group->dep_body->orbit, jd_dep) :
							osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);
		find_root(osv0, jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, max_depdv, dep_periapsis, &left_x, &right_x, 0.01);

		// No departure possible within given constraints
		if(left_x < 1 && right_x < 1 || right_x < min_dur*86400 || left_x > max_dur*86400) {
			size_t array_size = data_array2_size(group->vinf_array);
			if(array_size > 0 && data_array2_get_data(group->vinf_array)[array_size-1].y != 0)
				data_array2_append_new(group->vinf_array, jd_dep, 0);
			continue;
		}

		if(left_x < dt0) left_x = dt0;
		if(left_x < min_dur*86400) left_x = min_dur*86400;
		if(right_x > dt1) right_x = dt1;
		if(right_x > max_dur*86400) right_x = max_dur*86400;

		data_array2_append_new(group->vinf_array, jd_dep, left_x/86400);
		data_array2_append_new(group->vinf_array, jd_dep, right_x/86400);
	}

	if(data_array2_get_data(group->vinf_array)[data_array2_size(group->vinf_array)-1].y == 0) {
		data_array2_remove_at_idx(group->vinf_array, (int) data_array2_size(group->vinf_array)-1);
	}
}

void calc_group_porkchop_from_bands(SegmentGroup *group, int departure_cap, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance) {
	group->vinf_array = data_array2_create();
 
	OSV osv0 = group->system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(group->dep_body->orbit, jd_min_dep) :
					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_min_dep, group->system->cb);

	OSV osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
				   osv_from_elements(group->arr_body->orbit, jd_min_dep) :
				   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_min_dep, group->system->cb);
	Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, group->system->cb);
	double dt0, dt1;


	double r0 = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb).a, r1 = arr0.a;
	double r_ratio =  r1/r0;
	Hohmann hohmann = calc_hohmann_transfer(r0, r1, group->system->cb);
	double hohmann_dur = hohmann.dur/86400;
	double min_duration = 0.4 * hohmann_dur;
	double max_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_duration/hohmann_dur > 3) max_duration = hohmann_dur*3;
	if(max_duration < max_dur) max_dur = max_duration;
	if(min_duration > min_dur) min_dur = min_duration;

	double min_dt = min_dur*86400;
	double max_dt = max_dur*86400;
	double jd_dep_step = (jd_max_dep-jd_min_dep)/(departure_cap-1);

	for(int i = 0; i < departure_cap; i++) {
		double jd_dep = jd_min_dep + jd_dep_step*i;

		get_upper_and_lower_boundary_at_jd_dep(group, jd_dep, &dt0, &dt1);

		if(dt0 > max_dt || dt1 < min_dt) continue;

		double left_x = 0, right_x = 0;
		osv0 = group->system->prop_method == ORB_ELEMENTS ?
							osv_from_elements(group->dep_body->orbit, jd_dep) :
							osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);
		find_root(osv0, jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, max_depdv, dep_periapsis, &left_x, &right_x, 0.01);

		// No departure possible within given constraints
		if(left_x < 1 && right_x < 1 || right_x < min_dur*86400 || left_x > max_dur*86400) {
			size_t array_size = data_array2_size(group->vinf_array);
			if(array_size > 0 && data_array2_get_data(group->vinf_array)[array_size-1].y != 0)
				data_array2_append_new(group->vinf_array, jd_dep, 0);
			continue;
		}

		if(left_x < dt0) left_x = dt0;
		if(left_x < min_dur*86400) left_x = min_dur*86400;
		if(right_x > dt1) right_x = dt1;
		if(right_x > max_dur*86400) right_x = max_dur*86400;

		data_array2_append_new(group->vinf_array, jd_dep, left_x/86400);
		data_array2_append_new(group->vinf_array, jd_dep, right_x/86400);
	}

	if(data_array2_get_data(group->vinf_array)[data_array2_size(group->vinf_array)-1].y == 0) {
		data_array2_remove_at_idx(group->vinf_array, (int) data_array2_size(group->vinf_array)-1);
	}
}

void calc_group_porkchop(SegmentGroup *group, int departure_cap, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance) {
	group->segment_steps = malloc(departure_cap * sizeof(struct ItinStep*));

	OSV osv0 = group->system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(group->dep_body->orbit, jd_min_dep) :
					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_min_dep, group->system->cb);

	OSV osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
				   osv_from_elements(group->arr_body->orbit, jd_min_dep) :
				   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_min_dep, group->system->cb);
	Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, group->system->cb);
	double dt0, dt1;


	double r0 = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb).a, r1 = arr0.a;
	double r_ratio =  r1/r0;
	Hohmann hohmann = calc_hohmann_transfer(r0, r1, group->system->cb);
	double hohmann_dur = hohmann.dur/86400;
	double min_duration = 0.4 * hohmann_dur;
	double max_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_duration/hohmann_dur > 3) max_duration = hohmann_dur*3;
	if(max_duration < max_dur) max_dur = max_duration;
	if(min_duration > min_dur) min_dur = min_duration;

	double min_dt = min_dur*86400;
	double max_dt = max_dur*86400;
	double jd_dep_step = (jd_max_dep-jd_min_dep)/(departure_cap-1);

	for(int i = 0; i < departure_cap; i++) {
		double jd_dep = jd_min_dep + jd_dep_step*i;

		get_upper_and_lower_boundary_at_jd_dep(group, jd_dep, &dt0, &dt1);

		if(dt0 > max_dt || dt1 < min_dt) continue;

		double left_x = 0, right_x = 0;
		osv0 = group->system->prop_method == ORB_ELEMENTS ?
							osv_from_elements(group->dep_body->orbit, jd_dep) :
							osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);
		find_root(osv0, jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, max_depdv, dep_periapsis, &left_x, &right_x, 1);

		// No departure possible within given constraints
		if(left_x < 1 && right_x < 1 || right_x < min_dur*86400 || left_x > max_dur*86400) {
			if(group->num_steps > 0 && group->segment_steps[group->num_steps-1] != NULL) {
				group->segment_steps[group->num_steps] = NULL;
				group->num_steps++;
			}
			continue;
		}

		if(left_x < dt0) left_x = dt0;
		if(left_x < min_dur*86400) left_x = min_dur*86400;
		if(right_x > dt1) right_x = dt1;
		if(right_x > max_dur*86400) right_x = max_dur*86400;

		group->segment_steps[group->num_steps] = malloc(sizeof(struct ItinStep));
		group->segment_steps[group->num_steps]->body = group->dep_body;
		group->segment_steps[group->num_steps]->date = jd_dep;
		DataArray1 *dur_array = data_array1_create();
		calc_bounded_porkchop_line(group->segment_steps[group->num_steps], group->arr_body, group->system, dur_array, left_x, right_x, dep_periapsis, max_depdv, dv_tolerance);
		group->num_steps++;




		DataArray1 *data = data_array1_get_diff(dur_array);
		// printf("%6.3f  %6.3f   |   %10.3f\n", data_array1_get_min(data), data_array1_get_max(data), data_array1_get_max(data) / data_array1_get_min(data));
		// print_data_array1(data, "sep");
		data_array1_free(dur_array);
		data_array1_free(data);
	}
}

void calc_group_porkchop_stepped(SegmentGroup *group, int departure_cap, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance) {
	group->segment_steps = malloc(departure_cap * sizeof(struct ItinStep*));

	OSV osv0 = group->system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(group->dep_body->orbit, jd_min_dep) :
					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_min_dep, group->system->cb);

	OSV osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
				   osv_from_elements(group->arr_body->orbit, jd_min_dep) :
				   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_min_dep, group->system->cb);
	Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, group->system->cb);
	double dt0, dt1;


	double r0 = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb).a, r1 = arr0.a;
	double r_ratio =  r1/r0;
	Hohmann hohmann = calc_hohmann_transfer(r0, r1, group->system->cb);
	double hohmann_dur = hohmann.dur/86400;
	double min_duration = 0.4 * hohmann_dur;
	double max_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_duration/hohmann_dur > 3) max_duration = hohmann_dur*3;
	if(max_duration < max_dur) max_dur = max_duration;
	if(min_duration > min_dur) min_dur = min_duration;

	double min_dt = min_dur*86400;
	double max_dt = max_dur*86400;
	double jd_dep_step = (jd_max_dep-jd_min_dep)/(departure_cap-1);

	for(int i = 0; i < departure_cap; i++) {
		double jd_dep = jd_min_dep + jd_dep_step*i;

		get_upper_and_lower_boundary_at_jd_dep(group, jd_dep, &dt0, &dt1);

		if(dt0 > max_dt || dt1 < min_dt) continue;

		double left_x = 0, right_x = 0;
		osv0 = group->system->prop_method == ORB_ELEMENTS ?
							osv_from_elements(group->dep_body->orbit, jd_dep) :
							osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);
		find_root(osv0, jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, max_depdv, dep_periapsis, &left_x, &right_x, 1);

		// No departure possible within given constraints
		if(left_x < 1 && right_x < 1 || right_x < min_dur*86400 || left_x > max_dur*86400) {
			if(group->num_steps > 0 && group->segment_steps[group->num_steps-1] != NULL) {
				group->segment_steps[group->num_steps] = NULL;
				group->num_steps++;
			}
			continue;
		}

		if(left_x < dt0) left_x = dt0;
		if(left_x < min_dur*86400) left_x = min_dur*86400;
		if(right_x > dt1) right_x = dt1;
		if(right_x > max_dur*86400) right_x = max_dur*86400;

		group->segment_steps[group->num_steps] = malloc(sizeof(struct ItinStep));
		group->segment_steps[group->num_steps]->body = group->dep_body;
		group->segment_steps[group->num_steps]->date = jd_dep;
		DataArray1 *dur_array = data_array1_create();
		calc_bounded_porkchop_line(group->segment_steps[group->num_steps], group->arr_body, group->system, dur_array, left_x, right_x, dep_periapsis, max_depdv, dv_tolerance);
		group->num_steps++;




		DataArray1 *data = data_array1_get_diff(dur_array);
		// printf("%6.3f  %6.3f   |   %10.3f\n", data_array1_get_min(data), data_array1_get_max(data), data_array1_get_max(data) / data_array1_get_min(data));
		// print_data_array1(data, "sep");
		data_array1_free(dur_array);
		data_array1_free(data);
	}
}


void calc_bounded_porkchop_line_outline(struct ItinStep *departure_step, Body *arr_body, CelestSystem *system, DataArray1 *dur_array, double min_dt, double max_dt, double dep_periapsis, double max_depdv, double dv_tolerance) {
	Body *dep_body = departure_step->body;
	double jd_dep = departure_step->date;
	OSV osv0 = system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(dep_body->orbit, jd_dep) :
					osv_from_ephem(dep_body->ephem, dep_body->num_ephems, jd_dep, system->cb);

	double dt = min_dt;

	DataArray2 *data_dep = data_array2_create();
	DataArray2 *data_arr = data_array2_create();

	struct ItinStep *curr_step = departure_step;
	curr_step->r = osv0.r;
	curr_step->v_body = osv0.v;
	curr_step->v_dep = vec3(0, 0, 0);
	curr_step->v_arr = vec3(0, 0, 0);
	curr_step->num_next_nodes = 0;
	curr_step->prev = NULL;
	curr_step->next = (struct ItinStep **) malloc(1000 * sizeof(struct ItinStep *));
	int counter = 0;

	for(int j = 0; j < 1000; j++) {
		// printf("%f  %f  %f  %f  %f\n", min_dt, max_dt, dt0, dt1, dt);

		double jd_arr = jd_dep + dt / 86400;

		OSV osv_arr = system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(arr_body->orbit, jd_arr) :
					osv_from_ephem(arr_body->ephem, arr_body->num_ephems, jd_arr, system->cb);

		Lambert3 tf = calc_lambert3(osv0.r, osv_arr.r, (jd_arr - jd_dep) * 86400, system->cb);

		double vinf_dep = fabs(mag_vec3(subtract_vec3(tf.v0, osv0.v)));
		double dv_dep = dv_circ(dep_body,alt2radius(dep_body, dep_periapsis),vinf_dep);
		double vinf_arr = fabs(mag_vec3(subtract_vec3(tf.v1, osv_arr.v)));

		curr_step = get_first(curr_step);
		// sort chronologically
		int insert_index = counter;
		while(insert_index > 0) {
			if(curr_step->next[insert_index-1]->date < jd_arr) break;
			insert_index--;
		}
		if(insert_index != counter) {
			memmove(&curr_step->next[insert_index+1],
				&curr_step->next[insert_index],
				(counter+2 - insert_index) * sizeof(*curr_step->next));
		}

		curr_step->next[insert_index] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
		curr_step->next[insert_index]->prev = curr_step;
		curr_step->next[insert_index]->next = NULL;
		curr_step = curr_step->next[insert_index];

		curr_step->body = arr_body;
		curr_step->date = jd_arr;
		curr_step->r = osv_arr.r;
		curr_step->v_dep = tf.v0;
		curr_step->v_arr = tf.v1;
		curr_step->v_body = osv_arr.v;
		curr_step->num_next_nodes = 0;
		curr_step->prev->num_next_nodes++;
		counter++;

		if(dur_array) data_array1_insert_new(dur_array, dt/86400);

		data_array2_insert_new(data_dep, dt/86400, dv_dep);
		data_array2_insert_new(data_arr, dt/86400, vinf_arr);

		if(dt == min_dt) dt = max_dt;
		else break;
	}
	data_array2_free(data_dep);
	data_array2_free(data_arr);
}

void calc_group_porkchop_outline(SegmentGroup *group, int departure_cap, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance) {
	group->segment_steps = malloc(departure_cap * sizeof(struct ItinStep*));

	OSV osv0 = group->system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(group->dep_body->orbit, jd_min_dep) :
					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_min_dep, group->system->cb);

	OSV osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
				   osv_from_elements(group->arr_body->orbit, jd_min_dep) :
				   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_min_dep, group->system->cb);
	Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, group->system->cb);
	double dt0, dt1;


	double r0 = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb).a, r1 = arr0.a;
	double r_ratio =  r1/r0;
	Hohmann hohmann = calc_hohmann_transfer(r0, r1, group->system->cb);
	double hohmann_dur = hohmann.dur/86400;
	double min_duration = 0.4 * hohmann_dur;
	double max_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_duration/hohmann_dur > 3) max_duration = hohmann_dur*3;
	if(max_duration < max_dur) max_dur = max_duration;
	if(min_duration > min_dur) min_dur = min_duration;

	double min_dt = min_dur*86400;
	double max_dt = max_dur*86400;
	double jd_dep_step = (jd_max_dep-jd_min_dep)/(departure_cap-1);

	for(int i = 0; i < departure_cap; i++) {
		double jd_dep = jd_min_dep + jd_dep_step*i;

		get_upper_and_lower_boundary_at_jd_dep(group, jd_dep, &dt0, &dt1);

		if(dt0 > max_dt || dt1 < min_dt) continue;

		double left_x = 0, right_x = 0;
		osv0 = group->system->prop_method == ORB_ELEMENTS ?
							osv_from_elements(group->dep_body->orbit, jd_dep) :
							osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);
		find_root(osv0, jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, max_depdv, dep_periapsis, &left_x, &right_x, 1);

		// No departure possible within given constraints
		if(left_x < 1 && right_x < 1 || right_x < min_dur*86400 || left_x > max_dur*86400) {
			if(group->num_steps > 0 && group->segment_steps[group->num_steps-1] != NULL) {
				group->segment_steps[group->num_steps] = NULL;
				group->num_steps++;
			}
			continue;
		}

		if(left_x < dt0) left_x = dt0;
		if(left_x < min_dur*86400) left_x = min_dur*86400;
		if(right_x > dt1) right_x = dt1;
		if(right_x > max_dur*86400) right_x = max_dur*86400;

		group->segment_steps[group->num_steps] = malloc(sizeof(struct ItinStep));
		group->segment_steps[group->num_steps]->body = group->dep_body;
		group->segment_steps[group->num_steps]->date = jd_dep;
		DataArray1 *dur_array = data_array1_create();
		calc_bounded_porkchop_line_outline(group->segment_steps[group->num_steps], group->arr_body, group->system, dur_array, left_x, right_x, dep_periapsis, max_depdv, dv_tolerance);
		group->num_steps++;




		DataArray1 *data = data_array1_get_diff(dur_array);
		// printf("%6.3f  %6.3f   |   %10.3f\n", data_array1_get_min(data), data_array1_get_max(data), data_array1_get_max(data) / data_array1_get_min(data));
		// print_data_array1(data, "sep");
		data_array1_free(dur_array);
		data_array1_free(data);
	}
}

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



double calc_time_to_next_an_dn_line_up(OSV osv_dep_body, OSV osv_arr_body, Body *cb, double *next_line_up_dt, double *next_opposite_line_up_dt) {
	Plane3 orbital_plane_dep_body = constr_plane3(vec3(0,0,0), osv_dep_body.r, osv_dep_body.v);
	Plane3 orbital_plane_arr_body = constr_plane3(vec3(0,0,0), osv_arr_body.r, osv_arr_body.v);
	Vector3 plane_intersection = calc_intersecting_line_dir_plane3(orbital_plane_dep_body, orbital_plane_arr_body);

	Orbit dep_body_orbit = constr_orbit_from_osv(osv_dep_body.r, osv_dep_body.v, cb);
	double dep_body_orbit_period = calc_orbital_period(dep_body_orbit);
	double tpe0 = calc_orbit_time_since_periapsis(dep_body_orbit);

	double delta_true_anomaly = angle_vec3_vec3(osv_dep_body.r, plane_intersection);

	if(dep_body_orbit.i < M_PI/2 && cross_vec3(osv_dep_body.r, plane_intersection).z < 0 ||
		dep_body_orbit.i > M_PI/2 && cross_vec3(osv_dep_body.r, plane_intersection).z > 0) {
		delta_true_anomaly = 2*M_PI - delta_true_anomaly - M_PI;
	}

	Orbit line_up_orbit = dep_body_orbit;
	line_up_orbit.ta = pi_norm(line_up_orbit.ta + delta_true_anomaly);
	double dt_line_up = calc_orbit_time_since_periapsis(line_up_orbit)-tpe0;

	line_up_orbit.ta = pi_norm(line_up_orbit.ta + M_PI);
	double dt_opp_line_up = calc_orbit_time_since_periapsis(line_up_orbit)-tpe0;

	if(dt_line_up < 0)		dt_line_up += dep_body_orbit_period;
	if(dt_opp_line_up < 0)	dt_opp_line_up += dep_body_orbit_period;

	*next_line_up_dt = dt_line_up;
	*next_opposite_line_up_dt = dt_opp_line_up;
}

DataArray2 * calc_min_vinf_line(SegmentGroup *group, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double vinf_tolerance) {
	DataArray2 *min_per_dep = data_array2_create();
	group->num_steps = 0;

	OSV osv0 = group->system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(group->dep_body->orbit, jd_min_dep) :
					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_min_dep, group->system->cb);

	OSV osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
				   osv_from_elements(group->arr_body->orbit, jd_min_dep) :
				   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_min_dep, group->system->cb);
	Orbit orbit0 = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb);
	double period_dep = calc_orbital_period(orbit0);
	double next_line_up_dt, next_opp_line_up_dt;
	calc_time_to_next_an_dn_line_up(osv0, osv_arr0, group->system->cb, &next_line_up_dt, &next_opp_line_up_dt);

	double min_dt = min_dur*86400;
	double max_dt = max_dur*86400;
	DataArray2 *data_dep = data_array2_create();

	double min_dep = jd_min_dep;
	double max_dep = jd_min_dep + next_line_up_dt/86400;
	if(max_dep > jd_max_dep) max_dep = jd_max_dep;
	double jd_dep = min_dep;
	int index0 = 0;
	double max_jd_step = (next_opp_line_up_dt-next_line_up_dt)/86400/5;

	while(min_dep < jd_max_dep) {
		data_array2_clear(data_dep);
		double dt0, dt1;
		get_upper_and_lower_boundary_at_jd_dep(group, jd_dep, &dt0, &dt1);

		if(dt0 > max_dt || dt1 < min_dt) {
			double temp = min_dep;
			min_dep = max_dep;
			max_dep = temp + period_dep/86400;
			if(max_dep > jd_max_dep) max_dep = jd_max_dep;
			jd_dep = min_dep;
			continue;
		}

		osv0 = group->system->prop_method == ORB_ELEMENTS ?
				osv_from_elements(group->dep_body->orbit, jd_dep) :
				osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);

		double left_x = 0, right_x = 0;

		find_root(osv0, jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, 1e9, 1e9, &left_x, &right_x, 1);
		// printf("%f   %f\n", left_x/86400, right_x/86400);

		// printf("%f   ROOT: %f   %f   (%f  %f)   (%f  %f)\n", jd_dep-jd_min_dep, left_x/86400, right_x/86400, dt0/86400, dt1/86400, opp_guess/86400, conj_guess/86400);
		if(left_x < 1 && right_x < 1 || right_x < min_dur*86400 || left_x > max_dur*86400) {continue;}

		double opp_conj_margin = 86400*0.2;

		if(left_x < dt0+opp_conj_margin) left_x = dt0+opp_conj_margin;
		if(left_x < min_dur*86400) left_x = min_dur*86400;
		if(right_x > dt1-opp_conj_margin) right_x = dt1-opp_conj_margin;
		if(right_x > max_dur*86400) right_x = max_dur*86400;
		double dt = left_x;
		// printf("%f   %f\n", left_x/86400, right_x/86400);

		for(int j = 0; j < 1000; j++) {
			// printf("%f  %f  %f  %f  %f\n", min_dt, max_dt, dt0, dt1, dt);

			double jd_arr = jd_dep + dt / 86400;

			OSV osv1 = group->system->prop_method == ORB_ELEMENTS ?
						osv_from_elements(group->arr_body->orbit, jd_arr) :
						osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_arr, group->system->cb);

			Lambert3 tf = calc_lambert3(osv0.r, osv1.r, (jd_arr - jd_dep) * 86400, group->system->cb);

			double vinf = fabs(mag_vec3(subtract_vec3(tf.v0, osv0.v)));
			data_array2_insert_new(data_dep, dt/86400, vinf);

			if(dt == left_x) dt = right_x;
			else if(dt == right_x) dt = ( dt + data_array2_get_data(data_dep)[0].x*86400 ) / 2;
			else {
				double next_x = calc_next_x_find_min(data_dep, vinf_tolerance/2)*86400;

				if(next_x < 0) {
					data_array2_insert_new(min_per_dep, jd_dep, data_array2_get_min(data_dep).y);
					// print_date(convert_JD_date(jd_dep, DATE_ISO), 0);
					// printf("      %f  |   %f    %f    %f  |    %f    %f    %f\n", jd_dep, dt/86400, left_x/86400, right_x/86400, dt, left_x, right_x);
					// data_array2_append_new(min_per_dep, jd_dep-jd_min_dep, data[min_idx].x);
					break;
				}
				dt = next_x;
			}
		}
		// print_data_array2(data_dep, "dep", "dv");
		// print_data_array2(min_per_dep, "dep", "dv");

		if(jd_dep == min_dep) {jd_dep = max_dep; continue;}
		if(jd_dep == max_dep) {jd_dep = (min_dep + max_dep)/2; continue;}
		double next_dep = calc_next_x_wrt_smoothness(min_per_dep, index0, vinf_tolerance/2);
		if(!isnan(next_dep)) {
			if(next_dep - jd_dep > max_jd_step) {jd_dep += max_jd_step; continue;}
			jd_dep = next_dep; continue;
		}

		if(max_dep - jd_dep > max_jd_step) {jd_dep += max_jd_step; continue;}

		double temp = min_dep;
		min_dep = max_dep;
		max_dep = temp + period_dep/86400;
		if(max_dep > jd_max_dep) max_dep = jd_max_dep;
		jd_dep = max_dep;
		index0 = (int) data_array2_size(min_per_dep)-1;
	}

	data_array2_free(data_dep);

	return min_per_dep;
}


void get_dur_limits_from_departure_date(MeshBox2 *box, double jd_dep, DataArray2 *data_array) {
	if(box->min.x > jd_dep || box->max.x < jd_dep) return;

	if(box->type == MESHBOX_SUBBOXES) {
		for(int i = 0; i < box->subboxes.num; i++) {
			get_dur_limits_from_departure_date(box->subboxes.boxes[i], jd_dep, data_array);
		}
	} else if(box->type == MESHBOX_TRIANGLES) {
		for(int i = 0; i < box->tri.num; i++) {
			if(triangle_is_edge(box->tri.triangles[i])) {
				Vector2 min, max;
				find_2dtriangle_minmax(box->tri.triangles[i], &min, &max);
				if(min.x > jd_dep || max.x < jd_dep) continue;
				for(int edge_idx = 0; edge_idx < 3; edge_idx++) {
					if(!box->tri.triangles[i]->adj_triangles[edge_idx]) {
						Vector2 p0 = box->tri.triangles[i]->points[edge_idx]->pos;
						Vector2 p1 = box->tri.triangles[i]->points[(edge_idx+1)%3]->pos;

						if(p0.x < jd_dep == p1.x < jd_dep && p0.x != jd_dep && p1.x != jd_dep) continue;

						double m = (p1.y-p0.y) / (p1.x-p0.x);
						double dur = (jd_dep-p0.x)*m+p0.y;
						data_array2_insert_new(data_array, jd_dep, dur);
					}
				}
			}
		}
	}
}

void get_dur_limits_from_all_triangles(Mesh2 *mesh, DataArray2 *data_array) {
	for(int i = 0; i < mesh->num_triangles; i++) {
		for(int edge_idx = 0; edge_idx < 3; edge_idx++) {
			if(!mesh->triangles[i]->adj_triangles[edge_idx]) {
				Vector2 p0 = mesh->triangles[i]->points[edge_idx]->pos;
				Vector2 p1 = mesh->triangles[i]->points[(edge_idx+1)%3]->pos;
				data_array2_insert_new(data_array, p0.x, p0.y);
				data_array2_insert_new(data_array, p1.x, p1.y);
			}
		}
	}
}



DataArray2 * get_dur_limits_from_edge_triangles(Mesh2 *mesh) {
	DataArray2 *data_array = data_array2_create();
	MeshTriangle2 *triangle = NULL;
	for(int i = 0; i < mesh->num_triangles; i++) {
		if(triangle_is_edge(mesh->triangles[i])) {
			triangle = mesh->triangles[i]; break;
		}
	}

	if(triangle == NULL) {return data_array;}
	MeshPoint2 *first_point = NULL;
	MeshPoint2 *prev_point = NULL;
	MeshPoint2 *current_point = NULL;

	for(int edge_idx = 0; edge_idx < 3; edge_idx++) {
		if(!triangle->adj_triangles[edge_idx]) {
			first_point = triangle->points[edge_idx];
			current_point = triangle->points[(edge_idx+1)%3];
			data_array2_append_new(data_array, first_point->pos.x, first_point->pos.y);
			data_array2_append_new(data_array, current_point->pos.x, current_point->pos.y);
		}
	}

	prev_point = first_point;

	do {
		for(int i = 0; i < current_point->num_triangles; i++) {
			if(triangle_is_edge(current_point->triangles[i])) {
				for(int edge_idx = 0; edge_idx < 3; edge_idx++) {
					if(!current_point->triangles[i]->adj_triangles[edge_idx]) {
						if(current_point == current_point->triangles[i]->points[edge_idx] && prev_point != current_point->triangles[i]->points[(edge_idx+1)%3]) {
							prev_point = current_point;
							current_point = current_point->triangles[i]->points[(edge_idx+1)%3];
							if(current_point == first_point) return data_array;
							data_array2_append_new(data_array, current_point->pos.x, current_point->pos.y);
							i = current_point->num_triangles;	// break outside loop
							break;
						}
						if(current_point == current_point->triangles[i]->points[(edge_idx+1)%3] && prev_point != current_point->triangles[i]->points[edge_idx]) {
							prev_point = current_point;
							current_point = current_point->triangles[i]->points[edge_idx];
							if(current_point == first_point) return data_array;
							data_array2_append_new(data_array, current_point->pos.x, current_point->pos.y);
							i = current_point->num_triangles;	// break outside loop
							break;
						}
					}
				}
			}
		}
	} while(current_point != first_point);
	return data_array;
}

DataArray2 * get_dur_limits_for_dep_from_point_list(DataArray2 *edges_array, double jd_dep) {
	DataArray2 *limits_inv = data_array2_create();
	Vector2 *edges = data_array2_get_data(edges_array);
	for(int i = 0; i < data_array2_size(edges_array); i++) {
		Vector2 p0 = edges[i];
		Vector2 p1 = edges[(i+1)%data_array2_size(edges_array)];

		if(p1.x < p0.x) {
			Vector2 temp = p0;
			p0 = p1;
			p1 = temp;
		}

		if(p0.x > jd_dep || p1.x < jd_dep) continue;
		if(p0.x == jd_dep) {
			data_array2_insert_new(limits_inv, p0.y, p0.x);
			continue;
		}
		if(p1.x == jd_dep) {
			data_array2_insert_new(limits_inv, p1.y, p1.x);
			continue;
		}

		double m = (p1.y-p0.y)/(p1.x-p0.x);
		double dur = (jd_dep - p0.x)*m + p0.y;
		data_array2_insert_new(limits_inv, dur, jd_dep);
	}

	DataArray2 *limits = data_array2_create();
	Vector2 *limits_inv_data = data_array2_get_data(limits_inv);
	double last = NAN;
	for(int i = 0; i < data_array2_size(limits_inv); i++) {
		if(limits_inv_data[i].x != last) {
			data_array2_insert_new(limits, jd_dep, limits_inv_data[i].x);
			last = limits_inv_data[i].x;
		}
	}
	data_array2_free(limits_inv);

	return limits;
}

double root_finder_almost_monot_deriv_next_x(DataArray2 *arr, int branch) {
	// branch = 0 for left branch, 1 for right branch
	Vector2 *data = data_array2_get_data(arr);
	int num_data = (int) data_array2_size(arr);

	int index;

	// left branch
	if(branch == 0) {
		index = 0;
		for(int i = 1; i < num_data; i++) {
			if(data[i].y < 0)	{ index = i; break; }
			else 				{ index = i; }
		}

		// right branch
	} else {
		index = num_data-1;
		for(int i = num_data-2; i >= 0; i--) {
			if(data[i].y < 0)	{ index =   i; break; }
			else 				{ index =   i; }
		}
	}

	if(branch == 0) return (data[index].x + data[index-1].x)/2;
	else 			return (data[index].x + data[index+1].x)/2;
}

void get_dur_limit_wrt_vinf(Mesh2 *mesh, double jd_dep, double min_vinf, DataArray2 *init_limit_array, DataArray2 *new_limits, double tolerance) {
	Vector2 *init_lim = data_array2_get_data(init_limit_array);
	size_t num_init_lim = data_array2_size(init_limit_array);
	if(num_init_lim == 0) return;
	if(num_init_lim == 1) {
		double dvinf = get_mesh_interpolated_value(mesh, vec2(jd_dep, init_lim[0].y), MESH_VAL_VINF) - min_vinf;
		if(dvinf > 0) data_array2_insert_new(new_limits, init_lim[0].x, init_lim[0].y);
		return;
	}
	DataArray2 *new_limits_inv = data_array2_create();

	bool left_branch = true;
	DataArray2 *data = data_array2_create();

	for(int lim_idx = 0; lim_idx < num_init_lim; lim_idx+=2) {
		double lim0 = init_lim[lim_idx].y+1e-9;		// floating precision
		double lim1 = init_lim[lim_idx+1].y-1e-9;	// floating precision

		double dur = lim0;

		for(int i = 0; i < 100; i++) {
			double dvinf = get_mesh_interpolated_value(mesh, vec2(jd_dep, dur), MESH_VAL_VINF) - min_vinf;
			if(i > 3 && dvinf > 0 && dvinf < tolerance) {
				data_array2_insert_new(new_limits_inv, dur, jd_dep);
				if(left_branch && data_array2_get_data(data)[data_array2_size(data)-1].y > 0) {
					left_branch = false;
				} else {
					break;
				}
			}

			data_array2_insert_new(data, dur, dvinf);

			if(i == 0) {
				if(dvinf > 0) {
					data_array2_insert_new(new_limits_inv, dur, jd_dep);
				} else {
					left_branch = false;
				}
				dur = lim1;
				continue;
			}
			if(i == 1) {
				if(dvinf > 0) data_array2_insert_new(new_limits_inv, dur, jd_dep);
				else if(!left_branch) break;
			}
			if(!can_be_negative_monot_deriv(data)) break;
			if(i < 30) dur = root_finder_monot_deriv_next_x(data, !left_branch);
			else dur = root_finder_almost_monot_deriv_next_x(data, !left_branch);
		}
	}

	for(int i = 0; i < data_array2_size(new_limits_inv); i++) {
		data_array2_append_new(new_limits, data_array2_get_data(new_limits_inv)[i].y, data_array2_get_data(new_limits_inv)[i].x);
	}
	data_array2_free(data);
	data_array2_free(new_limits_inv);
}

Vector3 get_varr_from_mesh(Mesh2 *mesh, double jd_arr, double dur) {
	MeshTriangle2 *triangle = get_mesh_triangle_at_position(mesh, vec2(jd_arr, dur));
	if(!triangle) return vec3(NAN, NAN, NAN);

	Vector3 tri_varrx[3];
	Vector3 tri_varry[3];
	Vector3 tri_varrz[3];

	for(int i = 0; i < 3; i++) {
		tri_varrx[i] = vec3(triangle->points[i]->pos.x, triangle->points[i]->pos.y, triangle->points[i]->val[MESH_VAL_ARRX]);
		tri_varry[i] = vec3(triangle->points[i]->pos.x, triangle->points[i]->pos.y, triangle->points[i]->val[MESH_VAL_ARRY]);
		tri_varrz[i] = vec3(triangle->points[i]->pos.x, triangle->points[i]->pos.y, triangle->points[i]->val[MESH_VAL_ARRZ]);
	}
	double varrx = get_triangle_interpolated_value(tri_varrx[0], tri_varrx[1], tri_varrx[2], vec2(jd_arr, dur));
	double varry = get_triangle_interpolated_value(tri_varry[0], tri_varry[1], tri_varry[2], vec2(jd_arr, dur));
	double varrz = get_triangle_interpolated_value(tri_varrz[0], tri_varrz[1], tri_varrz[2], vec2(jd_arr, dur));
	return vec3(varrx, varry, varrz);
}

Vector3 get_vbody_from_mesh(Mesh2 *mesh, double jd_arr, double dur) {
	MeshTriangle2 *triangle = get_mesh_triangle_at_position(mesh, vec2(jd_arr, dur));
	if(!triangle) return vec3(NAN, NAN, NAN);

	Vector3 tri_body_vx[3];
	Vector3 tri_body_vy[3];
	Vector3 tri_body_vz[3];

	for(int i = 0; i < 3; i++) {
		tri_body_vx[i] = vec3(triangle->points[i]->pos.x, triangle->points[i]->pos.y, triangle->points[i]->val[MESH_VAL_BODY_VX]);
		tri_body_vy[i] = vec3(triangle->points[i]->pos.x, triangle->points[i]->pos.y, triangle->points[i]->val[MESH_VAL_BODY_VY]);
		tri_body_vz[i] = vec3(triangle->points[i]->pos.x, triangle->points[i]->pos.y, triangle->points[i]->val[MESH_VAL_BODY_VZ]);
	}
	double varrx = get_triangle_interpolated_value(tri_body_vx[0], tri_body_vx[1], tri_body_vx[2], vec2(jd_arr, dur));
	double varry = get_triangle_interpolated_value(tri_body_vy[0], tri_body_vy[1], tri_body_vy[2], vec2(jd_arr, dur));
	double varrz = get_triangle_interpolated_value(tri_body_vz[0], tri_body_vz[1], tri_body_vz[2], vec2(jd_arr, dur));
	return vec3(varrx, varry, varrz);
}

struct ItinStep * get_next_step_from_vinf(SegmentGroup *group, double v_inf, double jd_dep, double min_dur_dt, double max_dur_dt, bool leftside, double tolerance) {
	OSV osv_dep = group->system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(group->dep_body->orbit, jd_dep) :
					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);

	double dt0 = min_dur_dt, dt1 = max_dur_dt;

	// x: dt, y: diff_vinf
	DataArray2 *data = data_array2_create();

	double t0 = jd_dep;
	double last_dt, dt = dt0, t1, diff_vinf;

	for(int i = 0; i < 100; i++) {
		if(i == 0) dt = dt0;

		t1 = t0 + dt / 86400;

		OSV osv_arr = group->system->prop_method == ORB_ELEMENTS ?
				osv_from_elements(group->arr_body->orbit, t1) :
				osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, t1, group->system->cb);

		Lambert3 new_transfer = calc_lambert3(osv_dep.r, osv_arr.r, dt, group->system->cb);
		Vector3 v_dep = subtract_vec3(new_transfer.v0, osv_dep.v);
		diff_vinf = mag_vec3(v_dep) - v_inf;

		if(fabs(diff_vinf) < tolerance) {
			struct ItinStep *new_step = malloc(sizeof(struct ItinStep));
			new_step->body = group->arr_body;
			new_step->date = t1;
			new_step->r = osv_arr.r;
			new_step->v_dep = new_transfer.v0;
			new_step->v_arr = new_transfer.v1;
			new_step->v_body = osv_arr.v;
			new_step->num_next_nodes = 0;
			new_step->prev = NULL;
			new_step->next = NULL;
			return new_step;
		}

		data_array2_insert_new(data, dt, diff_vinf);

		if(!can_be_negative_monot_deriv(data)) break;
		last_dt = dt;
		if(i == 0) dt = dt1;
		else dt = root_finder_monot_deriv_next_x(data, !leftside);
		if(i > 3 && dt == last_dt) break;	// step size 0 (imprecision)
		if(isnan(dt) || isinf(dt)) break;
	}

	data_array2_free(data);
	return NULL;
}


DataArray2 * get_vinf_limits(Mesh2 *mesh, DataArray2 *vinf_array, double tolerance) {
	int num_deps = 1000;

	DataArray2 *edges_array = get_dur_limits_from_edge_triangles(mesh);

	DataArray2 *vinf_limits_all = data_array2_create();
	DataArray2 *vinf_limit_jd_dep = data_array2_create();

	double epsilon = 1e-6;
	double step = (mesh->mesh_box->max.x - mesh->mesh_box->min.x)/num_deps;
	double jd_dep = mesh->mesh_box->min.x+epsilon;

	while(jd_dep < mesh->mesh_box->max.x) {
		double jd_vinf_dep = interpolate_from_sorted_data_array(vinf_array, jd_dep);

		if(isnan(jd_vinf_dep)) {
			jd_dep += step;
			continue;
		}
		data_array2_clear(vinf_limit_jd_dep);
		DataArray2 *limits = get_dur_limits_for_dep_from_point_list(edges_array, jd_dep);
		get_dur_limit_wrt_vinf(mesh, jd_dep, jd_vinf_dep-tolerance*2, limits, vinf_limit_jd_dep, 1);
		data_array2_free(limits);

		if(data_array2_size(vinf_limit_jd_dep)%2 != 0) {
			jd_dep += epsilon;
			continue;
		}

		if(data_array2_size(vinf_limit_jd_dep) == 0) {
			// add a flagged pair
			data_array2_append_new(vinf_limits_all, jd_dep, NAN);
			data_array2_append_new(vinf_limits_all, jd_dep, NAN);
		}

		for(int j = 0; j < data_array2_size(vinf_limit_jd_dep); j++) {
			data_array2_append_new(vinf_limits_all, data_array2_get_data(vinf_limit_jd_dep)[j].x, data_array2_get_data(vinf_limit_jd_dep)[j].y);
		}
		if(jd_dep + step >= mesh->mesh_box->max.x && mesh->mesh_box->max.x - jd_dep >= 2*epsilon) {
			jd_dep = mesh->mesh_box->max.x - epsilon;
		} else jd_dep += step;
	}

	data_array2_free(edges_array);
	data_array2_free(vinf_limit_jd_dep);

	return vinf_limits_all;
}

int get_num_interval_per_dep(Vector2 *limits, int limit_idx0) {
	if(isnan(limits[limit_idx0*2].y)) return 0;
	int num_interval = 1;
	int limit_idx = limit_idx0+1;
	while(limits[limit_idx0*2].x == limits[limit_idx*2].x) {
		num_interval++;
		limit_idx++;
	}
	return num_interval;
}

void increase_fbgroups_capacity(FlyByGroups *fb_groups) {
	if(fb_groups->group_cap == 0) {
		fb_groups->group_cap = 8;
		fb_groups->groups = malloc(fb_groups->group_cap*sizeof(FlyByGroup*));
		fb_groups->num_groups_dep = calloc(fb_groups->group_cap, sizeof(int));
	} else {
		fb_groups->group_cap *= 2;
		FlyByGroup **groups = realloc(fb_groups->groups, fb_groups->group_cap*sizeof(FlyByGroup*));
		if(groups) fb_groups->groups = groups;
		int *num_groups_dep = realloc(fb_groups->num_groups_dep, fb_groups->group_cap*sizeof(int));
		if(num_groups_dep) fb_groups->num_groups_dep = num_groups_dep;
	}
}

FlyByGroups * get_flyby_groups_wrt_vinf(Mesh2 *mesh, SegmentGroup *departure_group, DataArray2 *vinf_limits, double tolerance) {
	int num_limits = (int) data_array2_size(vinf_limits)/2;
	Vector2 *limit_data = data_array2_get_data(vinf_limits);
	double min_jd_next_dep = limit_data[0].x;
	double max_jd_next_dep = limit_data[num_limits*2-1].x;
	double max_ddur = 0;

	DataArray1 *num_interval_change = data_array1_create();
	int last_num_intervals = 0;
	int interval_count = 0;
	data_array1_append_new(num_interval_change, limit_data[0].x);
	for(int i = 0; i < num_limits-1; i++) {
		bool empty_limit = isnan(data_array2_get_data(vinf_limits)[i*2].y);
		if(!empty_limit) {
			double ddur = limit_data[i*2+1].y - limit_data[i*2].y;
			if(ddur > max_ddur) max_ddur = ddur;
			interval_count++;
		}
		if(limit_data[(i+1)*2].x != limit_data[i*2].x){
			if(interval_count != last_num_intervals) {
				if(i != 0 && empty_limit) {
					data_array1_append_new(num_interval_change, limit_data[(i-1)*2].x);
					data_array1_append_new(num_interval_change, limit_data[i*2].x);
				} else if(i-interval_count >= 0) {
					data_array1_append_new(num_interval_change, limit_data[(i-interval_count)*2].x);
					data_array1_append_new(num_interval_change, limit_data[i*2].x);
				}
			}
			last_num_intervals = interval_count;
			interval_count = 0;
		}
	}
	// catch edges with (for some reason) a single point
	data_array1_append_new(num_interval_change, limit_data[(num_limits-1)*2-1].x);
	data_array1_append_new(num_interval_change, limit_data[num_limits*2-1].x);
	for(int i = 1; i < data_array1_size(num_interval_change); i += 3) {
		data_array1_insert_new(num_interval_change, (data_array1_get_data(num_interval_change)[i-1] + data_array1_get_data(num_interval_change)[i])/2);
	}

	double jd_step = (max_jd_next_dep - min_jd_next_dep) / 50;
	double dur_step = max_ddur/20;
	int limit_idx = 0;
	double next_jd = limit_data[0].x;



	double jd_dep = limit_data[0].x;
	OSV osv_dep = departure_group->system->prop_method == ORB_ELEMENTS ?
				osv_from_elements(departure_group->dep_body->orbit, jd_dep) :
				osv_from_ephem(departure_group->dep_body->ephem, departure_group->dep_body->num_ephems, jd_dep, departure_group->system->cb);
	OSV osv_arr0 = departure_group->system->prop_method == ORB_ELEMENTS ?
				   osv_from_elements(departure_group->arr_body->orbit, jd_dep) :
					osv_from_ephem(departure_group->arr_body->ephem, departure_group->arr_body->num_ephems, jd_dep, departure_group->system->cb);
	Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, departure_group->system->cb);
	double period_arr0 = calc_orbital_period(arr0);
	double next_conjunction_dt, next_opposition_dt, last_conjunction_dt, last_opposition_dt;
	calc_time_to_next_conjunction_and_opposition(osv_dep.r, osv_arr0, departure_group->system->cb, &next_conjunction_dt, &next_opposition_dt);

	// TODO change
	double opp_guess, conj_guess;
	// if(departure_group->top_boundary_type == DEPARTURE_GROUP_BOUNDARY_TOP_CONJ) {
	// 	conj_guess = departure_group->boundary0_top.y + (jd_dep - departure_group->boundary0_top.x) * 86400 * departure_group->boundary_gradient;
	// 	opp_guess = departure_group->boundary0_bottom.y + (jd_dep - departure_group->boundary0_bottom.x) * 86400 * departure_group->boundary_gradient;
	// } else {
	// 	opp_guess = departure_group->boundary0_top.y + (jd_dep - departure_group->boundary0_top.x) * 86400 * departure_group->boundary_gradient;
	// 	conj_guess = departure_group->boundary0_bottom.y + (jd_dep - departure_group->boundary0_bottom.x) * 86400 * departure_group->boundary_gradient;
	// }

	while(opp_guess-next_opposition_dt   >  0.5 * period_arr0) next_opposition_dt  += period_arr0;
	while(opp_guess-next_opposition_dt   < -0.5 * period_arr0) next_opposition_dt  -= period_arr0;
	while(conj_guess-next_conjunction_dt >  0.5 * period_arr0) next_conjunction_dt += period_arr0;
	while(conj_guess-next_conjunction_dt < -0.5 * period_arr0) next_conjunction_dt -= period_arr0;


	last_opposition_dt = next_opposition_dt;
	last_conjunction_dt = next_conjunction_dt;

	double last_jd_dep = jd_dep;


	FlyByGroups *fb_groups = malloc(sizeof(FlyByGroups));
	fb_groups->group_cap = 0;
	fb_groups->num_groups = 0;
	fb_groups->groups = NULL;
	fb_groups->num_groups_dep = NULL;

	int fb_group_x = -1; // is set to 0 during first loop
	int num_last_interval = 0;


	while(limit_idx < num_limits) {
		jd_dep = limit_data[limit_idx*2].x;

		osv_dep = departure_group->system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(departure_group->dep_body->orbit, jd_dep) :
					osv_from_ephem(departure_group->dep_body->ephem, departure_group->dep_body->num_ephems, jd_dep, departure_group->system->cb);

		osv_arr0 = departure_group->system->prop_method == ORB_ELEMENTS ?
					   osv_from_elements(departure_group->arr_body->orbit, jd_dep) :
					   osv_from_ephem(departure_group->arr_body->ephem, departure_group->arr_body->num_ephems, jd_dep, departure_group->system->cb);
		calc_time_to_next_conjunction_and_opposition(osv_dep.r, osv_arr0, departure_group->system->cb, &next_conjunction_dt, &next_opposition_dt);

		// TODO change
		// opp_guess = last_opposition_dt + (jd_dep-last_jd_dep)*86400*departure_group->boundary_gradient;
		// conj_guess = last_conjunction_dt + (jd_dep-last_jd_dep)*86400*departure_group->boundary_gradient;

		while(opp_guess-next_opposition_dt   >  0.5 * period_arr0) next_opposition_dt  += period_arr0;
		while(opp_guess-next_opposition_dt   < -0.5 * period_arr0) next_opposition_dt  -= period_arr0;
		while(conj_guess-next_conjunction_dt >  0.5 * period_arr0) next_conjunction_dt += period_arr0;
		while(conj_guess-next_conjunction_dt < -0.5 * period_arr0) next_conjunction_dt -= period_arr0;

		last_opposition_dt = next_opposition_dt;
		last_conjunction_dt = next_conjunction_dt;
		last_jd_dep = jd_dep;


		double min_dur_dt, max_dur_dt;
		if(next_conjunction_dt < next_opposition_dt) {
			min_dur_dt = next_conjunction_dt;
			max_dur_dt = next_opposition_dt;
		} else {
			min_dur_dt = next_opposition_dt;
			max_dur_dt = next_conjunction_dt;
		}

		if(min_dur_dt < 86400*10) min_dur_dt = 86400*10;

		if(jd_dep < next_jd) {
			bool relevant_interval = false;
			for(int i = 0; i < data_array1_size(num_interval_change); i++) {
				if(jd_dep == data_array1_get_data(num_interval_change)[i] ||
					(data_array1_get_data(num_interval_change)[i] < limit_data[limit_idx*2].x &&
					data_array1_get_data(num_interval_change)[i] > limit_data[(limit_idx-1)*2].x)) {
					relevant_interval = true;
					break;
				}
			}
			if(!relevant_interval) { limit_idx++; continue; }
		}
		next_jd = jd_dep+jd_step;
		if(next_jd > max_jd_next_dep) next_jd = max_jd_next_dep;

		int num_interval = get_num_interval_per_dep(limit_data, limit_idx);
		if(num_last_interval != num_interval) {
			fb_group_x++;
			fb_groups->num_groups++;
			if(fb_group_x == fb_groups->group_cap) {
				increase_fbgroups_capacity(fb_groups);
			}
			num_last_interval = num_interval;
			fb_groups->num_groups_dep[fb_group_x] = num_interval;
			fb_groups->groups[fb_group_x] = malloc(num_interval*sizeof(FlyByGroup));
			for(int i = 0; i < num_interval; i++) {
				fb_groups->groups[fb_group_x][i].dep_dur = data_array2_create();
				fb_groups->groups[fb_group_x][i].step_cap = 8;
				fb_groups->groups[fb_group_x][i].left_steps = malloc(fb_groups->groups[fb_group_x][i].step_cap*sizeof(struct ItinStep*));
				fb_groups->groups[fb_group_x][i].right_steps = malloc(fb_groups->groups[fb_group_x][i].step_cap*sizeof(struct ItinStep*));
			}
		}

		int fb_group_y = 0;

		while(limit_data[limit_idx*2].x == jd_dep && limit_idx < num_limits) {
			if(isnan(data_array2_get_data(vinf_limits)[limit_idx*2].y)) {limit_idx++; break;}
			double min_dur_temp = data_array2_get_data(vinf_limits)[limit_idx*2].y+1e-3;
			double max_dur_temp = data_array2_get_data(vinf_limits)[limit_idx*2+1].y-1e-3;
			int num_tests = (int) ((max_dur_temp - min_dur_temp)/dur_step) + 2;
			for(int i = 0; i < num_tests; i++) {
				double dur_temp = min_dur_temp + (max_dur_temp - min_dur_temp)*i/(num_tests-1);
				double vinf = get_mesh_interpolated_value(mesh, vec2(jd_dep, dur_temp), MESH_VAL_VINF);
				Vector3 v_arr = get_varr_from_mesh(mesh, jd_dep, dur_temp);
				Vector3 v_body = get_vbody_from_mesh(mesh, jd_dep, dur_temp);
				if(isnan(v_arr.x), isnan(v_body.x)) continue;
				struct ItinStep *left_step = NULL;
				struct ItinStep *right_step = NULL;
				double next_step_tolerance = 1;
				do {
					left_step = get_next_step_from_vinf(departure_group, vinf, jd_dep, min_dur_dt, max_dur_dt, true, next_step_tolerance);
					if(left_step) break;
					next_step_tolerance += tolerance;
				} while(!left_step);
				next_step_tolerance = 1;
				do {
					right_step = get_next_step_from_vinf(departure_group, vinf, jd_dep, min_dur_dt, max_dur_dt, false, next_step_tolerance);
					if(right_step) break;
					next_step_tolerance += tolerance;
				} while(!right_step);

				int step_idx = (int) data_array2_size(fb_groups->groups[fb_group_x][fb_group_y].dep_dur);
				if(step_idx == fb_groups->groups[fb_group_x][fb_group_y].step_cap) {
					fb_groups->groups[fb_group_x][fb_group_y].step_cap *= 2;
					struct ItinStep **left_steps = realloc(fb_groups->groups[fb_group_x][fb_group_y].left_steps, fb_groups->groups[fb_group_x][fb_group_y].step_cap*sizeof(struct ItinStep*));
					if(left_steps) fb_groups->groups[fb_group_x][fb_group_y].left_steps = left_steps;
					struct ItinStep **right_steps = realloc(fb_groups->groups[fb_group_x][fb_group_y].right_steps, fb_groups->groups[fb_group_x][fb_group_y].step_cap*sizeof(struct ItinStep*));
					if(right_steps) fb_groups->groups[fb_group_x][fb_group_y].right_steps = right_steps;
				}
				fb_groups->groups[fb_group_x][fb_group_y].left_steps[step_idx] = left_step;
				fb_groups->groups[fb_group_x][fb_group_y].right_steps[step_idx] = right_step;
				data_array2_append_new(fb_groups->groups[fb_group_x][fb_group_y].dep_dur, jd_dep, dur_temp);
			}
			limit_idx++;
			fb_group_y++;
		}
	}

	return fb_groups;
}

Mesh2 * get_rpe_mesh_from_fb_groups(FlyByGroups *fb_groups, Mesh2 *prev_mesh, SegmentGroup *prev_departure_group, bool left_side) {
	MeshGrid2 ***grids = malloc(fb_groups->num_groups*sizeof(MeshGrid2**));
	for(int i = 0; i < fb_groups->num_groups; i++) {
		grids[i] = malloc(fb_groups->num_groups_dep[i]*sizeof(MeshGrid2*));
	}

	// TODO REDO!

	// for(int x_idx = 0; x_idx < fb_groups->num_groups; x_idx++) {
	// 	for(int y_idx = 0; y_idx < fb_groups->num_groups_dep[x_idx]; y_idx++) {
	// 		void *steps = (void*) (left_side ? fb_groups->groups[x_idx][y_idx].left_steps : fb_groups->groups[x_idx][y_idx].right_steps);
	// 		grids[x_idx][y_idx] = create_mesh_grid(fb_groups->groups[x_idx][y_idx].dep_dur, steps);
	// 	}
	// }
	Mesh2 *rpe_mesh = create_mesh_from_multiple_grids_w_angled_guideline(grids, fb_groups->num_groups, fb_groups->num_groups_dep, prev_departure_group->boundary_gradient);

	// for(int x_idx = 0; x_idx < fb_groups->num_groups; x_idx++) {
	// 	for(int y_idx = 0; y_idx < fb_groups->num_groups_dep[x_idx]; y_idx++) {
	// 		free_grid_keep_points(grids[x_idx][y_idx]);
	// 	}
	// 	free(grids[x_idx]);
	// }
	// free(grids);
	//
	// for(int i = 0; i < rpe_mesh->num_points; i++) {
	// 	struct ItinStep *ptr = rpe_mesh->points[i]->old_data;
	// 	double jd_dep = rpe_mesh->points[i]->pos.x;
	// 	double dur = rpe_mesh->points[i]->pos.y;
	// 	Vector3 v_arr = get_varr_from_mesh(prev_mesh, jd_dep, dur);
	// 	Vector3 v_body = get_vbody_from_mesh(prev_mesh, jd_dep, dur);
	// 	double r_pe = get_flyby_periapsis(v_arr, ptr->v_dep, v_body, prev_departure_group->arr_body);
	// 	rpe_mesh->points[i]->old_val = r_pe;
	// }

	return rpe_mesh;
}




FlyByGroups * get_refined_departure_groups(SegmentGroup *departure_group, DataArray2 *limits, double dep_periapsis, double max_dep_dv, double dv_tolerance) {
	int num_limits = (int) data_array2_size(limits)/2;
	Vector2 *limit_data = data_array2_get_data(limits);
	double min_jd_dep = limit_data[0].x;
	double max_jd_dep = limit_data[num_limits*2-1].x;
	double max_ddur = 0;

	DataArray1 *num_interval_change = data_array1_create();
	int last_num_intervals = 0;
	int interval_count = 0;
	data_array1_append_new(num_interval_change, limit_data[0].x);
	for(int i = 0; i < num_limits-1; i++) {
		bool empty_limit = isnan(data_array2_get_data(limits)[i*2].y);
		if(!empty_limit) {
			double ddur = limit_data[i*2+1].y - limit_data[i*2].y;
			if(ddur > max_ddur) max_ddur = ddur;
			interval_count++;
		}
		if(limit_data[(i+1)*2].x != limit_data[i*2].x){
			if(interval_count != last_num_intervals) {
				if(i != 0 && empty_limit) {
					data_array1_append_new(num_interval_change, limit_data[(i-1)*2].x);
					data_array1_append_new(num_interval_change, limit_data[i*2].x);
				} else if(i-interval_count >= 0) {
					data_array1_append_new(num_interval_change, limit_data[(i-interval_count)*2].x);
					data_array1_append_new(num_interval_change, limit_data[i*2].x);
				}
			}
			last_num_intervals = interval_count;
			interval_count = 0;
		}
	}
	// catch edges with (for some reason) a single point
	data_array1_append_new(num_interval_change, limit_data[(num_limits-1)*2-1].x);
	data_array1_append_new(num_interval_change, limit_data[num_limits*2-1].x);
	for(int i = 1; i < data_array1_size(num_interval_change); i += 3) {
		data_array1_insert_new(num_interval_change, (data_array1_get_data(num_interval_change)[i-1] + data_array1_get_data(num_interval_change)[i])/2);
	}

	double jd_step = (max_jd_dep - min_jd_dep) / 500;
	int limit_idx = 0;
	double next_jd = limit_data[0].x;
	double jd_dep = limit_data[0].x;

	FlyByGroups *fb_groups = malloc(sizeof(FlyByGroups));
	fb_groups->group_cap = 0;
	fb_groups->num_groups = 0;
	fb_groups->groups = NULL;
	fb_groups->num_groups_dep = NULL;

	int fb_group_x = -1; // is set to 0 during first loop
	int num_last_interval = 0;


	while(limit_idx < num_limits) {
		jd_dep = limit_data[limit_idx*2].x;

		if(jd_dep < next_jd) {
			bool relevant_interval = false;
			for(int i = 0; i < data_array1_size(num_interval_change); i++) {
				if(jd_dep == data_array1_get_data(num_interval_change)[i] ||
					(data_array1_get_data(num_interval_change)[i] < limit_data[limit_idx*2].x &&
					data_array1_get_data(num_interval_change)[i] > limit_data[(limit_idx-1)*2].x)) {
					relevant_interval = true;
					break;
				}
			}
			if(!relevant_interval) { limit_idx++; continue; }
		}
		next_jd = jd_dep+jd_step;
		if(next_jd > max_jd_dep) next_jd = max_jd_dep;

		int num_interval = get_num_interval_per_dep(limit_data, limit_idx);
		if(num_last_interval != num_interval) {
			fb_group_x++;
			fb_groups->num_groups++;
			if(fb_group_x == fb_groups->group_cap) {
				increase_fbgroups_capacity(fb_groups);
			}
			num_last_interval = num_interval;
			fb_groups->num_groups_dep[fb_group_x] = num_interval;
			fb_groups->groups[fb_group_x] = malloc(num_interval*sizeof(FlyByGroup));
			for(int i = 0; i < num_interval; i++) {
				fb_groups->groups[fb_group_x][i].dep_dur = data_array2_create();
				fb_groups->groups[fb_group_x][i].step_cap = 8;
				fb_groups->groups[fb_group_x][i].left_steps = malloc(fb_groups->groups[fb_group_x][i].step_cap*sizeof(struct ItinStep*));
			}
		}

		int fb_group_y = 0;

		while(limit_data[limit_idx*2].x == jd_dep && limit_idx < num_limits) {
			if(isnan(data_array2_get_data(limits)[limit_idx*2].y)) {limit_idx++; break;}
			double min_dt = limit_data[limit_idx*2].y*86400;
			double max_dt = limit_data[limit_idx*2+1].y*86400;

			struct ItinStep *departure = malloc(sizeof(struct ItinStep));
			departure->date = limit_data[limit_idx*2].x;
			departure->body = departure_group->dep_body;
			calc_bounded_porkchop_line(departure, departure_group->arr_body, departure_group->system, NULL, min_dt, max_dt, dep_periapsis, max_dep_dv, dv_tolerance);
			int step_idx = (int) data_array2_size(fb_groups->groups[fb_group_x][fb_group_y].dep_dur);
			if(step_idx + departure->num_next_nodes > fb_groups->groups[fb_group_x][fb_group_y].step_cap) {
				while(step_idx + departure->num_next_nodes > fb_groups->groups[fb_group_x][fb_group_y].step_cap)
					fb_groups->groups[fb_group_x][fb_group_y].step_cap *= 2;
				struct ItinStep **left_steps = realloc(fb_groups->groups[fb_group_x][fb_group_y].left_steps, fb_groups->groups[fb_group_x][fb_group_y].step_cap*sizeof(struct ItinStep*));
				if(left_steps) fb_groups->groups[fb_group_x][fb_group_y].left_steps = left_steps;
			}
			memcpy(fb_groups->groups[fb_group_x][fb_group_y].left_steps+step_idx, departure->next, departure->num_next_nodes*sizeof(struct ItinStep*));

			for(int j = 0; j < departure->num_next_nodes; j++) {
				double x = departure->date;
				double y = departure->next[j]->date - x;
				data_array2_append_new(fb_groups->groups[fb_group_x][fb_group_y].dep_dur, x, y);
			}
			limit_idx++;
			fb_group_y++;
		}
	}

	return fb_groups;
}

Mesh2 * get_dep_mesh_from_fb_groups(FlyByGroups *fb_groups, SegmentGroup *departure_group) {
	MeshGrid2 ***grids = malloc(fb_groups->num_groups*sizeof(MeshGrid2**));
	for(int i = 0; i < fb_groups->num_groups; i++) {
		grids[i] = malloc(fb_groups->num_groups_dep[i]*sizeof(MeshGrid2*));
	}

	for(int x_idx = 0; x_idx < fb_groups->num_groups; x_idx++) {
		for(int y_idx = 0; y_idx < fb_groups->num_groups_dep[x_idx]; y_idx++) {
			void *steps = (void*) fb_groups->groups[x_idx][y_idx].left_steps;
			grids[x_idx][y_idx] = create_mesh_grid(fb_groups->groups[x_idx][y_idx].dep_dur, steps, NUM_PORKCHOP_MESH_VALUE_TYPES);
		}
	}
	Mesh2 *mesh = create_mesh_from_multiple_grids_w_angled_guideline(grids, fb_groups->num_groups, fb_groups->num_groups_dep, departure_group->boundary_gradient);

	for(int x_idx = 0; x_idx < fb_groups->num_groups; x_idx++) {
		for(int y_idx = 0; y_idx < fb_groups->num_groups_dep[x_idx]; y_idx++) {
			free_grid_keep_points(grids[x_idx][y_idx]);
		}
		free(grids[x_idx]);
	}
	free(grids);

	// for(int i = 0; i < mesh->num_points; i++) {
	// 	struct ItinStep *ptr = mesh->points[i]->old_data;
	// 	double vinf = mag_vec3(subtract_vec3(ptr->v_arr, ptr->v_body));
	// 	mesh->points[i]->old_val = vinf;
	// }

	return mesh;
}

// Mesh2 * get_refined_mesh_from_departures(struct ItinStep ***departures, int num_departures, int *num_groups_per_departure) {
// 	MeshGrid2 ***grids = malloc(num_departures*sizeof(MeshGrid2**));
// 	for(int i = 0; i < num_departures; i++) {
// 		grids[i] = malloc(num_groups_per_departure[i]*sizeof(MeshGrid2*));
// 	}
//
// 	for(int x_idx = 0; x_idx < num_departures; x_idx++) {
// 		for(int y_idx = 0; y_idx < num_groups_per_departure[x_idx]; y_idx++) {
// 			void *steps = (void*) departures[x_idx][y_idx].;
// 			grids[x_idx][y_idx] = create_mesh_grid(fb_groups->groups[x_idx][y_idx].dep_dur, steps);
// 		}
// 	}
// 	Mesh2 *rpe_mesh = create_mesh_from_multiple_grids_w_angled_guideline(grids, fb_groups->num_groups, fb_groups->num_groups_dep, prev_departure_group->boundary_gradient);
//
// 	for(int x_idx = 0; x_idx < fb_groups->num_groups; x_idx++) {
// 		for(int y_idx = 0; y_idx < fb_groups->num_groups_dep[x_idx]; y_idx++) {
// 			free_grid_keep_points(grids[x_idx][y_idx]);
// 		}
// 		free(grids[x_idx]);
// 	}
// 	free(grids);
//
// 	for(int i = 0; i < rpe_mesh->num_points; i++) {
// 		struct ItinStep *ptr = rpe_mesh->points[i]->data;
// 		double jd_dep = rpe_mesh->points[i]->pos.x;
// 		double dur = rpe_mesh->points[i]->pos.y;
// 		Vector3 v_arr = get_varr_from_mesh(prev_mesh, jd_dep, dur);
// 		Vector3 v_body = get_vbody_from_mesh(prev_mesh, jd_dep, dur);
// 		double r_pe = get_flyby_periapsis(v_arr, ptr->v_dep, v_body, prev_departure_group->arr_body);
// 		rpe_mesh->points[i]->val = r_pe;
// 	}
//
// 	return rpe_mesh;
// }


