#include "itin_rework_tools.h"

#include "gui/gui_manager.h"
#include "gui/gui_tools/screen.h"
#include "gui/drawing.h"
#include "geometrylib.h"
#include <math.h>

double calc_next_x_wrt_smoothness(DataArray2 *arr, int index_0, double tolerance) {
	Vector2 *data = &(data_array2_get_data(arr)[index_0]);
	size_t num_data = data_array2_size(arr)-index_0;

	if (num_data == 3) return (data[1].x - data[0].x)/100 + data[0].x;
	if (num_data == 4) return (data[1].x - data[0].x)/10 + data[0].x;

	for (int i = 1; i < num_data-1; i++) {
		if ((data[i+1].x - data[i].x) < 0.001) continue;
		double m = (data[i].y - data[i-1].y)/(data[i].x - data[i-1].x);
		double ip_y = data[i].y + m*(data[i+1].x-data[i].x);

		if (fabs(ip_y - data[i+1].y) > tolerance) {
			return (data[i].x + data[i+1].x)/2;
		}
	}

	return -1;
}

double calc_next_x_find_min(DataArray2 *arr, int index_0, double tolerance) {
	Vector2 *data = &(data_array2_get_data(arr)[index_0]);
	size_t num_data = data_array2_size(arr)-index_0;

	int min_idx = (int) num_data-1;

	for (int i = 0; i < num_data-1; i++) {
		if (data[i].y < data[i+1].y) { min_idx = i; break; }
	}

	double m_left = (data[min_idx].y - data[min_idx-1].y)/(data[min_idx].x - data[min_idx-1].x);
	double m_right = (data[min_idx+1].y - data[min_idx].y)/(data[min_idx+1].x - data[min_idx].x);

	double left_guess = (data[min_idx-1].x - data[min_idx].x) * m_right + data[min_idx].y;
	double right_guess = (data[min_idx+1].x - data[min_idx].x) * m_left + data[min_idx].y;

	double dleft = fabs(left_guess - data[min_idx-1].y);
	double dright = fabs(right_guess - data[min_idx+1].y);

	if (dleft < tolerance && dright < tolerance) {return -1;}

	if (dleft > dright) return data[min_idx].x - (data[min_idx].x - data[min_idx-1].x)*0.2;
	else return data[min_idx].x + (data[min_idx+1].x - data[min_idx].x)*0.2;
}

void find_root(OSV osv_dep, double jd_dep, Body *dep_body, Body *arr_body, CelestSystem *system, double dt0, double dt1, double max_depdv, double dep_periapsis, double *left_x, double *right_x) {
	// x: dt, y: diff_vinf
	DataArray2 *data = data_array2_create();

	double t0 = jd_dep;
	double last_dt = -1e20, dt, t1;
	*left_x = 0;
	*right_x = 0;
	bool left_branch = true;

	if (dt0 < 100) dt0 = 100;	// transfer duration of 100s
	dt = dt0;

	for(int i = 0; i < 100; i++) {
		t1 = t0 + dt / 86400;

		OSV osv_arr = system->prop_method == ORB_ELEMENTS ?
				osv_from_elements(arr_body->orbit, t1) :
				osv_from_ephem(arr_body->ephem, arr_body->num_ephems, t1, system->cb);

		Lambert3 new_transfer = calc_lambert3(osv_dep.r, osv_arr.r, dt, system->cb);
		double vinf = fabs(mag_vec3(subtract_vec3(new_transfer.v0, osv_dep.v)));
		double dv_dep = dv_circ(dep_body,alt2radius(dep_body, dep_periapsis),vinf);

		if(i > 3 && (fabs(dv_dep - max_depdv) < 1 || (i > 3 && fabs(dt-last_dt) < 1))) {
			if(left_branch) {
				*left_x = dt;
				last_dt = -1e20;
				left_branch = false;
				if (*right_x != 0) break;
			} else {
				*right_x = dt;
				break;
			}
		}


		data_array2_insert_new(data, dt, dv_dep - max_depdv);

		if (i == 1) {
			Vector2 *values = data_array2_get_data(data);
			if (values[0].y < 0) {
				*left_x = values[0].x;
				if (values[1].y < 0) {
					*right_x = values[1].x;
					break;
				}
				left_branch = false;
			}
			if (values[1].y < 0) {
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



DataArray2 * calc_porkchop_line(struct ItinStep *departure_step, Body *dep_body, Body *arr_body, CelestSystem *system, double jd_dep, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance) {
	DataArray2 *data_dep = data_array2_create();
	DataArray2 *data_arr = data_array2_create();
	DataArray2 *data = data_arr;


	OSV osv0 = system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(dep_body->orbit, jd_dep) :
					osv_from_ephem(dep_body->ephem, dep_body->num_ephems, jd_dep, system->cb);

	OSV osv_arr0 = system->prop_method == ORB_ELEMENTS ?
				   osv_from_elements(arr_body->orbit, jd_dep) :
				   osv_from_ephem(arr_body->ephem, arr_body->num_ephems, jd_dep, system->cb);
	double next_conjunction_dt, next_opposition_dt;
	calc_time_to_next_conjunction_and_opposition(osv0.r, osv_arr0, system->cb, &next_conjunction_dt, &next_opposition_dt);
	Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, system->cb);
	double period_arr0 = calc_orbital_period(arr0);
	double dt0, dt1;
	if(next_conjunction_dt < next_opposition_dt) {
		dt0 = next_opposition_dt - period_arr0;
		dt1 = next_conjunction_dt;
	} else {
		dt0 = next_conjunction_dt - period_arr0;
		dt1 = next_opposition_dt;
	}

	struct ItinStep *curr_step;
	curr_step = departure_step;
	curr_step->body = dep_body;
	curr_step->date = jd_dep;
	curr_step->r = osv0.r;
	curr_step->v_body = osv0.v;
	curr_step->v_dep = vec3(0, 0, 0);
	curr_step->v_arr = vec3(0, 0, 0);
	curr_step->num_next_nodes = 10000;
	curr_step->prev = NULL;
	curr_step->next = (struct ItinStep **) malloc(curr_step->num_next_nodes * sizeof(struct ItinStep *));


	double r0 = constr_orbit_from_osv(osv0.r, osv0.v, system->cb).a, r1 = arr0.a;
	double r_ratio =  r1/r0;
	Hohmann hohmann = calc_hohmann_transfer(r0, r1, system->cb);
	double hohmann_dur = hohmann.dur/86400;
	double min_duration = 0.4 * hohmann_dur;
	double max_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_duration/hohmann_dur > 3) max_duration = hohmann_dur*3;
	if (max_duration < max_dur) max_dur = max_duration;
	if (min_duration > min_dur) min_dur = min_duration;

	int max_new_steps = 100000;
	double min_dt = min_dur*86400;
	double max_dt = max_dur*86400;
	int counter = 0;
	double dt;

	// printf("%f\n%f    %f\n%s    %s\n", jd_dep, min_dur, max_dur, dep_body->name, arr_body->name);

	while(dt0 < max_dt && counter < max_new_steps) {
		int index = (int) data_array2_size(data);
		double left_x = 0, right_x = 0;

		if(dt1 < min_dt) {
			double temp = dt1;
			dt1 = dt0 + period_arr0;
			dt0 = temp;
			continue;
		}

		find_root(osv0, jd_dep, dep_body, arr_body, system, dt0, dt1, max_depdv, dep_periapsis, &left_x, &right_x);

		// printf("ROOT: %f   %f   (%f  %f)\n", left_x/86400, right_x/86400, dt0/86400, dt1/86400);
		if (left_x < 1 && right_x < 1 || right_x < min_dur*86400 || left_x > max_dur*86400) {
			double temp = dt1;
			dt1 = dt0 + period_arr0;
			dt0 = temp;
			continue;
		}

		if (left_x < dt0) left_x = dt0;
		if (left_x < min_dur*86400) left_x = min_dur*86400;
		if (right_x > dt1) right_x = dt1;
		if (right_x > max_dur*86400) right_x = max_dur*86400;
		dt = left_x;
		for(int i = 0; i < max_new_steps/10; i++) {
			// printf("%f  %f  %f  %f  %f\n", min_dt, max_dt, dt0, dt1, dt);

			double jd_arr = jd_dep + dt / 86400;

			OSV osv1 = system->prop_method == ORB_ELEMENTS ?
						osv_from_elements(arr_body->orbit, jd_arr) :
						osv_from_ephem(arr_body->ephem, arr_body->num_ephems, jd_arr, system->cb);

			Lambert3 tf = calc_lambert3(osv0.r, osv1.r, (jd_arr - jd_dep) * 86400, system->cb);

			double vinf = fabs(mag_vec3(subtract_vec3(tf.v0, osv0.v)));
			double dv_dep = dv_circ(dep_body,alt2radius(dep_body, dep_periapsis),vinf);
			vinf = fabs(mag_vec3(subtract_vec3(tf.v1, osv1.v)));
			double dv_arr = dv_capture(arr_body,alt2radius(arr_body, dep_periapsis),vinf);
			data_array2_insert_new(data_dep, dt/86400, dv_dep);
			data_array2_insert_new(data_arr, dt/86400, dv_arr);

			curr_step = get_first(curr_step);
			curr_step->next[counter] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
			curr_step->next[counter]->prev = curr_step;
			curr_step->next[counter]->next = NULL;

			curr_step = curr_step->next[counter];

			curr_step->body = arr_body;
			curr_step->date = jd_arr;
			curr_step->r = osv1.r;
			curr_step->v_dep = tf.v0;
			curr_step->v_arr = tf.v1;
			curr_step->v_body = osv1.v;
			curr_step->num_next_nodes = 0;

			// printf("%f  %f   %f    %f   %f   %f\n", dt/86400, dv_dep, left_x/86400, right_x/86400, min_dur, max_dur);
			counter++;

			if (dt == left_x) dt = right_x;
			else if (dt == right_x) dt = ( dt + data_array2_get_data(data)[index].x*86400 ) / 2;
			else {
				double next_dep_x = calc_next_x_wrt_smoothness(data_dep, index, dv_tolerance)*86400;
				double next_arr_x = calc_next_x_wrt_smoothness(data_arr, index, dv_tolerance)*86400;
				if (next_dep_x < 0 && next_arr_x < 0) break;
				if (next_dep_x > 0 && next_dep_x < next_arr_x || next_arr_x < 0) dt = next_dep_x;
				else dt = next_arr_x;
			}
		}

		double temp = dt1;
		dt1 = dt0 + period_arr0;
		dt0 = temp;
	}

	// printf("Total: %d\n", counter);
	departure_step->num_next_nodes = counter;

	if (data == data_arr) data_array2_free(data_dep);
	else data_array2_free(data_arr);

	return data;
}

DataArray2 * calc_porkchop_line_static(Body *dep_body, Body *arr_body, CelestSystem *system, double jd_dep, double min_dur, double max_dur, double dep_periapsis, int num_points) {
	DataArray2 *data = data_array2_create();

	OSV osv0 = system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(dep_body->orbit, jd_dep) :
					osv_from_ephem(dep_body->ephem, dep_body->num_ephems, jd_dep, system->cb);

	OSV osv_arr0 = system->prop_method == ORB_ELEMENTS ?
				   osv_from_elements(arr_body->orbit, jd_dep) :
				   osv_from_ephem(arr_body->ephem, arr_body->num_ephems, jd_dep, system->cb);

	double r0 = mag_vec3(osv0.r), r1 = mag_vec3(osv_arr0.r);
	double r_ratio =  r1/r0;
	Hohmann hohmann = calc_hohmann_transfer(r0, r1, system->cb);
	double hohmann_dur = hohmann.dur/86400;
	double min_duration = 0.4 * hohmann_dur;
	double max_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_duration/hohmann_dur > 3) max_duration = hohmann_dur*3;
	if (max_duration < max_dur) max_dur = max_duration;
	if (min_duration > min_dur) min_dur = min_duration;

	for(int i = 0; i < num_points; i++) {
		double dur = (max_dur-min_dur)/num_points*i + min_dur;
		double jd_arr = jd_dep + dur;
		OSV osv1 = system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(arr_body->orbit, jd_arr) :
					osv_from_ephem(arr_body->ephem, arr_body->num_ephems, jd_arr, system->cb);

		Lambert3 tf = calc_lambert3(osv0.r, osv1.r, (jd_arr - jd_dep) * 86400, system->cb);

		double vinf = fabs(mag_vec3(subtract_vec3(tf.v0, osv0.v)));
		double dv_dep = dv_circ(dep_body,alt2radius(dep_body, dep_periapsis),vinf);

		data_array2_append_new(data, dur, dv_dep);
	}

	return data;
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

	double rot_speed_dep_body = 2*M_PI/calc_orbital_period(orbit0);
	double rot_speed_arr_body = 2*M_PI/calc_orbital_period(orbit1);
	double rot_speed_relative = rot_speed_dep_body-rot_speed_arr_body;
	double dydx = rot_speed_relative/(2*M_PI) * (calc_orbital_period(orbit1));

	return dydx;
}

void calc_group_porkchop(DepartureGroup *group, int shift, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance) {
	int departures_cap = group->num_departures;
	group->num_departures = 0;
	group->departures = malloc(departures_cap * sizeof(struct ItinStep*));

	double opp_conj_gradient = calc_opposition_conjunction_gradient(group->dep_body, group->arr_body, group->system, (jd_min_dep+jd_max_dep)/2);
	if (opp_conj_gradient > 0) shift *= -1;

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
	double dt0, dt1;

	// default shift from calc_time_to_next_conjunction_and_opposition is 1 (but we want values around 0 for start)
	if (shift % 2 == 0) {
		if(last_conjunction_dt > last_opposition_dt) last_conjunction_dt -= period_arr0;
		else last_opposition_dt -= period_arr0;
		shift++;
	}
	shift--;
	last_opposition_dt += period_arr0 * shift/2;
	last_conjunction_dt += period_arr0 * shift/2;

	if (last_opposition_dt < last_conjunction_dt) {
		group->boundary0_bottom = vec2(jd_min_dep, last_opposition_dt);
		group->boundary0_top = vec2(jd_min_dep, last_conjunction_dt);
	} else {
		group->boundary0_bottom = vec2(jd_min_dep, last_conjunction_dt);
		group->boundary0_top = vec2(jd_min_dep, last_opposition_dt);
	}

	group->boundary_gradient = opp_conj_gradient;


	double r0 = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb).a, r1 = arr0.a;
	double r_ratio =  r1/r0;
	Hohmann hohmann = calc_hohmann_transfer(r0, r1, group->system->cb);
	double hohmann_dur = hohmann.dur/86400;
	double min_duration = 0.4 * hohmann_dur;
	double max_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_duration/hohmann_dur > 3) max_duration = hohmann_dur*3;
	if (max_duration < max_dur) max_dur = max_duration;
	if (min_duration > min_dur) min_dur = min_duration;

	double min_dt = min_dur*86400;
	double max_dt = max_dur*86400;
	double jd_dep_step = (jd_max_dep-jd_min_dep)/(departures_cap-1);

	DataArray2 *data_dep = data_array2_create();
	DataArray2 *data_arr = data_array2_create();

	for (int i = 0; i < departures_cap; i++) {
		double jd_dep = jd_min_dep + jd_dep_step*i;

		data_array2_clear(data_dep);
		data_array2_clear(data_arr);

		osv0 = group->system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(group->dep_body->orbit, jd_dep) :
					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);

		osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
					   osv_from_elements(group->arr_body->orbit, jd_dep) :
					   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_dep, group->system->cb);
		calc_time_to_next_conjunction_and_opposition(osv0.r, osv_arr0, group->system->cb, &next_conjunction_dt, &next_opposition_dt);


		double opp_guess = last_opposition_dt + jd_dep_step*86400*opp_conj_gradient;
		double conj_guess = last_conjunction_dt + jd_dep_step*86400*opp_conj_gradient;

		while (opp_guess-next_opposition_dt   >  0.5 * period_arr0) next_opposition_dt  += period_arr0;
		while (opp_guess-next_opposition_dt   < -0.5 * period_arr0) next_opposition_dt  -= period_arr0;
		while (conj_guess-next_conjunction_dt >  0.5 * period_arr0) next_conjunction_dt += period_arr0;
		while (conj_guess-next_conjunction_dt < -0.5 * period_arr0) next_conjunction_dt -= period_arr0;

		last_opposition_dt = next_opposition_dt;
		last_conjunction_dt = next_conjunction_dt;
		if(next_conjunction_dt < next_opposition_dt) {
			dt0 = next_conjunction_dt;
			dt1 = next_opposition_dt;
		} else {
			dt0 = next_opposition_dt;
			dt1 = next_conjunction_dt;
		}

		if(dt0 > max_dt || dt1 < min_dt) continue;

		double left_x = 0, right_x = 0;

		find_root(osv0, jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, max_depdv, dep_periapsis, &left_x, &right_x);

		// printf("ROOT: %f   %f   (%f  %f)   (%f  %f)\n", left_x/86400, right_x/86400, dt0/86400, dt1/86400, opp_guess/86400, conj_guess/86400);
		if (left_x < 1 && right_x < 1 || right_x < min_dur*86400 || left_x > max_dur*86400) {
			if (group->num_departures > 0) {
				struct ItinStep *curr_step;
				group->departures[group->num_departures] = malloc(sizeof(struct ItinStep));
				curr_step = group->departures[group->num_departures];
				curr_step->prev = NULL;
				curr_step->next = NULL;
				group->num_departures++;

				if (group->departures[group->num_departures-2]->num_next_nodes < 0) {
					group->departures[group->num_departures-1]->num_next_nodes = -2;
				} else {
					group->departures[group->num_departures-1]->num_next_nodes = -1;
				}
			}
			continue;
		}

		if (left_x < dt0) left_x = dt0;
		if (left_x < min_dur*86400) left_x = min_dur*86400;
		if (right_x > dt1) right_x = dt1;
		if (right_x > max_dur*86400) right_x = max_dur*86400;
		double dt = left_x;

		struct ItinStep *curr_step;
		group->departures[group->num_departures] = malloc(sizeof(struct ItinStep));
		curr_step = group->departures[group->num_departures];
		curr_step->body = group->dep_body;
		curr_step->date = jd_dep;
		curr_step->r = osv0.r;
		curr_step->v_body = osv0.v;
		curr_step->v_dep = vec3(0, 0, 0);
		curr_step->v_arr = vec3(0, 0, 0);
		curr_step->num_next_nodes = 0;
		curr_step->prev = NULL;
		curr_step->next = (struct ItinStep **) malloc(1000 * sizeof(struct ItinStep *));
		group->num_departures++;
		int counter = 0;

		for(int j = 0; j < 1000; j++) {
			// printf("%f  %f  %f  %f  %f\n", min_dt, max_dt, dt0, dt1, dt);

			double jd_arr = jd_dep + dt / 86400;

			OSV osv1 = group->system->prop_method == ORB_ELEMENTS ?
						osv_from_elements(group->arr_body->orbit, jd_arr) :
						osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_arr, group->system->cb);

			Lambert3 tf = calc_lambert3(osv0.r, osv1.r, (jd_arr - jd_dep) * 86400, group->system->cb);

			double vinf = fabs(mag_vec3(subtract_vec3(tf.v0, osv0.v)));
			double dv_dep = dv_circ(group->dep_body,alt2radius(group->dep_body, dep_periapsis),vinf);
			vinf = fabs(mag_vec3(subtract_vec3(tf.v1, osv1.v)));
			double dv_arr = dv_capture(group->arr_body,alt2radius(group->arr_body, dep_periapsis),vinf);
			data_array2_insert_new(data_dep, dt/86400, dv_dep);
			data_array2_insert_new(data_arr, dt/86400, dv_arr);

			curr_step = get_first(curr_step);

			// sort chronologically
			int insert_index = counter;
			while (insert_index > 0) {
				if (curr_step->next[insert_index-1]->date < jd_arr) break;
				insert_index--;
			}
			if (insert_index != counter) {
				memmove(&curr_step->next[insert_index+1],
					&curr_step->next[insert_index],
					(counter+2 - insert_index) * sizeof(*curr_step->next));
			}

			curr_step->next[insert_index] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
			curr_step->next[insert_index]->prev = curr_step;
			curr_step->next[insert_index]->next = NULL;
			curr_step = curr_step->next[insert_index];

			curr_step->body = group->arr_body;
			curr_step->date = jd_arr;
			curr_step->r = osv1.r;
			curr_step->v_dep = tf.v0;
			curr_step->v_arr = tf.v1;
			curr_step->v_body = osv1.v;
			curr_step->num_next_nodes = 0;
			curr_step->prev->num_next_nodes++;
			counter++;

			if (dt == left_x) dt = right_x;
			else if (dt == right_x) dt = ( dt + data_array2_get_data(data_dep)[0].x*86400 ) / 2;
			else {
				double next_dep_x = calc_next_x_wrt_smoothness(data_dep, 0, dv_tolerance)*86400;
				double next_arr_x = calc_next_x_wrt_smoothness(data_arr, 0, dv_tolerance)*86400;
				if (next_dep_x < 0 && next_arr_x < 0) break;
				if (next_dep_x > 0 && next_dep_x < next_arr_x || next_arr_x < 0) dt = next_dep_x;
				else dt = next_arr_x;
			}
		}
	}

	data_array2_free(data_dep);
	data_array2_free(data_arr);
}



DataArray2 * calc_min_vinf_line(DepartureGroup *group, int shift, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double vinf_tolerance) {
	DataArray2 *min_per_dep = data_array2_create();
	int departures_cap = group->num_departures;
	double opp_conj_gradient = calc_opposition_conjunction_gradient(group->dep_body, group->arr_body, group->system, (jd_min_dep+jd_max_dep)/2);
	if (opp_conj_gradient > 0) shift *= -1;

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
	double dt0, dt1;

	// default shift from calc_time_to_next_conjunction_and_opposition is 1 (but we want values around 0 for start)
	if (shift % 2 == 0) {
		if(last_conjunction_dt > last_opposition_dt) last_conjunction_dt -= period_arr0;
		else last_opposition_dt -= period_arr0;
		shift++;
	}
	shift--;
	last_opposition_dt += period_arr0 * shift/2;
	last_conjunction_dt += period_arr0 * shift/2;

	if (last_opposition_dt < last_conjunction_dt) {
		group->boundary0_bottom = vec2(jd_min_dep, last_opposition_dt);
		group->boundary0_top = vec2(jd_min_dep, last_conjunction_dt);
	} else {
		group->boundary0_bottom = vec2(jd_min_dep, last_conjunction_dt);
		group->boundary0_top = vec2(jd_min_dep, last_opposition_dt);
	}

	group->boundary_gradient = opp_conj_gradient;


	double r0 = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb).a, r1 = arr0.a;
	double r_ratio =  r1/r0;
	Hohmann hohmann = calc_hohmann_transfer(r0, r1, group->system->cb);
	double hohmann_dur = hohmann.dur/86400;
	double min_duration = 0.4 * hohmann_dur;
	double max_duration = (4*(r_ratio-0.85)*(r_ratio-0.85)+1.5) * hohmann_dur; if(max_duration/hohmann_dur > 3) max_duration = hohmann_dur*3;
	min_duration /= 5;
	max_duration *= 5;
	if (max_duration < max_dur) max_dur = max_duration;
	if (min_duration > min_dur) min_dur = min_duration;

	double min_dt = min_dur*86400;
	double max_dt = max_dur*86400;
	double jd_dep_step = (jd_max_dep-jd_min_dep)/(departures_cap-1);

	DataArray2 *data_dep = data_array2_create();

	for (int i = 0; i < departures_cap; i++) {
		double jd_dep = jd_min_dep + jd_dep_step*i;

		data_array2_clear(data_dep);

		osv0 = group->system->prop_method == ORB_ELEMENTS ?
					osv_from_elements(group->dep_body->orbit, jd_dep) :
					osv_from_ephem(group->dep_body->ephem, group->dep_body->num_ephems, jd_dep, group->system->cb);

		osv_arr0 = group->system->prop_method == ORB_ELEMENTS ?
					   osv_from_elements(group->arr_body->orbit, jd_dep) :
					   osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_dep, group->system->cb);
		calc_time_to_next_conjunction_and_opposition(osv0.r, osv_arr0, group->system->cb, &next_conjunction_dt, &next_opposition_dt);


		double opp_guess = last_opposition_dt + jd_dep_step*86400*opp_conj_gradient;
		double conj_guess = last_conjunction_dt + jd_dep_step*86400*opp_conj_gradient;

		while (opp_guess-next_opposition_dt   >  0.5 * period_arr0) next_opposition_dt  += period_arr0;
		while (opp_guess-next_opposition_dt   < -0.5 * period_arr0) next_opposition_dt  -= period_arr0;
		while (conj_guess-next_conjunction_dt >  0.5 * period_arr0) next_conjunction_dt += period_arr0;
		while (conj_guess-next_conjunction_dt < -0.5 * period_arr0) next_conjunction_dt -= period_arr0;

		last_opposition_dt = next_opposition_dt;
		last_conjunction_dt = next_conjunction_dt;
		if(next_conjunction_dt < next_opposition_dt) {
			dt0 = next_conjunction_dt;
			dt1 = next_opposition_dt;
		} else {
			dt0 = next_opposition_dt;
			dt1 = next_conjunction_dt;
		}

		if(dt0 > max_dt || dt1 < min_dt) continue;

		double left_x = 0, right_x = 0;

		find_root(osv0, jd_dep, group->dep_body, group->arr_body, group->system, dt0, dt1, 1e9, 1e9, &left_x, &right_x);

		// printf("ROOT: %f   %f   (%f  %f)   (%f  %f)\n", left_x/86400, right_x/86400, dt0/86400, dt1/86400, opp_guess/86400, conj_guess/86400);
		if (left_x < 1 && right_x < 1 || right_x < min_dur*86400 || left_x > max_dur*86400) {continue;}

		if (left_x < dt0) left_x = dt0;
		if (left_x < min_dur*86400) left_x = min_dur*86400;
		if (right_x > dt1) right_x = dt1;
		if (right_x > max_dur*86400) right_x = max_dur*86400;
		double dt = left_x;

		for(int j = 0; j < 1000; j++) {
			// printf("%f  %f  %f  %f  %f\n", min_dt, max_dt, dt0, dt1, dt);

			double jd_arr = jd_dep + dt / 86400;

			OSV osv1 = group->system->prop_method == ORB_ELEMENTS ?
						osv_from_elements(group->arr_body->orbit, jd_arr) :
						osv_from_ephem(group->arr_body->ephem, group->arr_body->num_ephems, jd_arr, group->system->cb);

			Lambert3 tf = calc_lambert3(osv0.r, osv1.r, (jd_arr - jd_dep) * 86400, group->system->cb);

			double vinf = fabs(mag_vec3(subtract_vec3(tf.v0, osv0.v)));
			data_array2_insert_new(data_dep, dt/86400, vinf);

			if (dt == left_x) dt = right_x;
			else if (dt == right_x) dt = ( dt + data_array2_get_data(data_dep)[0].x*86400 ) / 2;
			else {
				double next_x = calc_next_x_find_min(data_dep, 0, vinf_tolerance)*86400;

				if (next_x < 0) {
					Vector2 *data = data_array2_get_data(data_dep);
					size_t num_data = data_array2_size(data_dep);
					for (int idx = 0; idx < num_data-1; idx++) {
						if (data[idx].y < data[idx+1].y) { data_array2_append_new(min_per_dep, jd_dep-jd_min_dep, data[idx].y); break; }
						// if (data[idx].y < data[idx+1].y) { data_array2_append_new(min_per_dep, jd_dep-jd_min_dep, dt/86400); break; }
					}
					break;
				}
				dt = next_x;
			}
		}
	}

	data_array2_free(data_dep);

	return min_per_dep;
}