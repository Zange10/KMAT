#include "itin_rework_tools.h"

#include "gui/gui_manager.h"
#include "gui/gui_tools/screen.h"
#include "gui/drawing.h"
#include "geometrylib.h"
#include <math.h>

double calc_next_x(DataArray2 *arr, int index_0, double tolerance) {
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

		if(fabs(dv_dep - max_depdv) < 1 || (i > 3 && fabs(dt-last_dt) < 1)) {
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


	double r0 = mag_vec3(osv0.r), r1 = mag_vec3(osv_arr0.r);
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

		printf("ROOT: %f   %f   (%f  %f)\n", left_x/86400, right_x/86400, dt0/86400, dt1/86400);
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
				double next_dep_x = calc_next_x(data_dep, index, dv_tolerance)*86400;
				double next_arr_x = calc_next_x(data_arr, index, dv_tolerance)*86400;
				if (next_dep_x < 0 && next_arr_x < 0) break;
				if (next_dep_x > 0 && next_dep_x < next_arr_x || next_arr_x < 0) dt = next_dep_x;
				else dt = next_arr_x;
			}
		}

		double temp = dt1;
		dt1 = dt0 + period_arr0;
		dt0 = temp;
	}

	printf("Total: %d\n", counter);
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

