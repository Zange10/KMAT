#include "itin_rework_tools.h"

#include "gui/gui_manager.h"
#include "gui/gui_tools/screen.h"
#include "gui/drawing.h"
#include "geometrylib.h"
#include <math.h>

double calc_next_x_wrt_smoothness(DataArray2 *arr, int index_0, double tolerance) {
	Vector2 *data = &(data_array2_get_data(arr)[index_0]);
	size_t num_data = data_array2_size(arr)-index_0;

	if(num_data == 3) return (data[1].x - data[0].x)/100 + data[0].x;
	if(num_data == 4) return (data[1].x - data[0].x)/10 + data[0].x;

	for(int i = 1; i < num_data-1; i++) {
		if((data[i+1].x - data[i].x) < 0.001) continue;
		double m = (data[i].y - data[i-1].y)/(data[i].x - data[i-1].x);
		double ip_y = data[i].y + m*(data[i+1].x-data[i].x);

		if(fabs(ip_y - data[i+1].y) > tolerance) {
			return (data[i].x + data[i+1].x)/2;
		}
	}

	return -1;
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

void find_root(OSV osv_dep, double jd_dep, Body *dep_body, Body *arr_body, CelestSystem *system, double dt0, double dt1, double max_depdv, double dep_periapsis, double *left_x, double *right_x) {
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

		if(i > 3 && max_depdv - dv_dep > 0 && max_depdv - dv_dep < 1 || (i > 3 && fabs(dt-last_dt) < 1)) {
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
	if(max_duration < max_dur) max_dur = max_duration;
	if(min_duration > min_dur) min_dur = min_duration;

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
		if(left_x < 1 && right_x < 1 || right_x < min_dur*86400 || left_x > max_dur*86400) {
			double temp = dt1;
			dt1 = dt0 + period_arr0;
			dt0 = temp;
			continue;
		}

		if(left_x < dt0) left_x = dt0;
		if(left_x < min_dur*86400) left_x = min_dur*86400;
		if(right_x > dt1) right_x = dt1;
		if(right_x > max_dur*86400) right_x = max_dur*86400;
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

			if(dt == left_x) dt = right_x;
			else if(dt == right_x) dt = ( dt + data_array2_get_data(data)[index].x*86400 ) / 2;
			else {
				double next_dep_x = calc_next_x_wrt_smoothness(data_dep, index, dv_tolerance)*86400;
				double next_arr_x = calc_next_x_wrt_smoothness(data_arr, index, dv_tolerance)*86400;
				if(next_dep_x < 0 && next_arr_x < 0) break;
				if(next_dep_x > 0 && next_dep_x < next_arr_x || next_arr_x < 0) dt = next_dep_x;
				else dt = next_arr_x;
			}
		}

		double temp = dt1;
		dt1 = dt0 + period_arr0;
		dt0 = temp;
	}

	// printf("Total: %d\n", counter);
	departure_step->num_next_nodes = counter;

	if(data == data_arr) data_array2_free(data_dep);
	else data_array2_free(data_arr);

	return data;
}

void calc_bounded_porkchop_line(struct ItinStep *departure_step, Body *arr_body, CelestSystem *system, double min_dt, double max_dt, double dep_periapsis, double max_depdv, double dv_tolerance) {
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

		if(dv_dep <= max_depdv) {
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
		}

		data_array2_insert_new(data_dep, dt/86400, dv_dep);
		data_array2_insert_new(data_arr, dt/86400, vinf_arr);

		if(dt == min_dt) dt = max_dt;
		else if(dt == max_dt) dt = ( dt + data_array2_get_data(data_dep)[0].x*86400 ) / 2;
		else {
			double next_dep_x = calc_next_x_wrt_smoothness(data_dep, 0, dv_tolerance)*86400;
			double next_arr_x = calc_next_x_wrt_smoothness(data_arr, 0, dv_tolerance)*86400;
			if(next_dep_x < 0 && next_arr_x < 0) break;
			if(next_dep_x > 0 && next_dep_x < next_arr_x || next_arr_x < 0) dt = next_dep_x;
			else dt = next_arr_x;
		}
	}
	data_array2_free(data_dep);
	data_array2_free(data_arr);
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

void calc_group_porkchop(DepartureGroup *group, int shift, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double dep_periapsis, double max_depdv, double dv_tolerance) {
	int departures_cap = group->num_departures;
	group->num_departures = 0;
	group->departures = malloc(departures_cap * sizeof(struct ItinStep*));

	double opp_conj_gradient = calc_opposition_conjunction_gradient(group->dep_body, group->arr_body, group->system, (jd_min_dep+jd_max_dep)/2);
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
	double dt0, dt1;

	// default shift from calc_time_to_next_conjunction_and_opposition is 1 (but we want values around 0 for start)
	if(shift % 2 == 0) {
		if(last_conjunction_dt > last_opposition_dt) last_conjunction_dt -= period_arr0;
		else last_opposition_dt -= period_arr0;
		shift++;
	}
	shift--;
	last_opposition_dt += period_arr0 * shift/2;
	last_conjunction_dt += period_arr0 * shift/2;

	if(last_opposition_dt < last_conjunction_dt) {
		group->boundary0_bottom = vec2(jd_min_dep, last_opposition_dt);
		group->boundary0_top = vec2(jd_min_dep, last_conjunction_dt);
		group->top_boundary_type = DEPARTURE_GROUP_BOUNDARY_TOP_CONJ;
	} else {
		group->boundary0_bottom = vec2(jd_min_dep, last_conjunction_dt);
		group->boundary0_top = vec2(jd_min_dep, last_opposition_dt);
		group->top_boundary_type = DEPARTURE_GROUP_BOUNDARY_TOP_OPP;
	}

	group->boundary_gradient = opp_conj_gradient;


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
	double jd_dep_step = (jd_max_dep-jd_min_dep)/(departures_cap-1);

	for(int i = 0; i < departures_cap; i++) {
		double jd_dep = jd_min_dep + jd_dep_step*i;

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
		if(left_x < 1 && right_x < 1 || right_x < min_dur*86400 || left_x > max_dur*86400) {
			if(group->num_departures > 0) {
				struct ItinStep *curr_step;
				group->departures[group->num_departures] = malloc(sizeof(struct ItinStep));
				curr_step = group->departures[group->num_departures];
				curr_step->prev = NULL;
				curr_step->next = NULL;
				group->num_departures++;

				if(group->departures[group->num_departures-2]->num_next_nodes < 0) {
					group->departures[group->num_departures-1]->num_next_nodes = -2;
				} else {
					group->departures[group->num_departures-1]->num_next_nodes = -1;
				}
			}
			continue;
		}

		if(left_x < dt0) left_x = dt0;
		if(left_x < min_dur*86400) left_x = min_dur*86400;
		if(right_x > dt1) right_x = dt1;
		if(right_x > max_dur*86400) right_x = max_dur*86400;

		struct ItinStep *curr_step;
		group->departures[group->num_departures] = malloc(sizeof(struct ItinStep));
		curr_step = group->departures[group->num_departures];
		curr_step->body = group->dep_body;
		curr_step->date = jd_dep;
		group->num_departures++;

		calc_bounded_porkchop_line(group->departures[group->num_departures-1], group->arr_body, group->system, left_x, right_x, dep_periapsis, max_depdv, dv_tolerance);
	}
}



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


DataArray2 * calc_min_vinf_line(DepartureGroup *group, int shift, double jd_min_dep, double jd_max_dep, double jd_max_arr, double min_dur, double max_dur, double vinf_tolerance) {
	DataArray2 *min_per_dep = data_array2_create();
	int departures_cap = group->num_departures;
	group->num_departures = 0;
	double opp_conj_gradient = calc_opposition_conjunction_gradient(group->dep_body, group->arr_body, group->system, (jd_min_dep+jd_max_dep)/2);
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
	double dt0, dt1;

	// default shift from calc_time_to_next_conjunction_and_opposition is 1 (but we want values around 0 for start)
	if(shift % 2 == 0) {
		if(last_conjunction_dt > last_opposition_dt) last_conjunction_dt -= period_arr0;
		else last_opposition_dt -= period_arr0;
		shift++;
	}
	shift--;
	last_opposition_dt += period_arr0 * shift/2;
	last_conjunction_dt += period_arr0 * shift/2;

	if(last_opposition_dt < last_conjunction_dt) {
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
	if(max_duration < max_dur) max_dur = max_duration;
	if(min_duration > min_dur) min_dur = min_duration;

	double min_dt = min_dur*86400;
	double max_dt = max_dur*86400;
	double jd_dep_step = (jd_max_dep-jd_min_dep)/(departures_cap-1);

	DataArray2 *data_dep = data_array2_create();

	for(int i = 0; i < departures_cap; i++) {
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

		while(opp_guess-next_opposition_dt   >  0.5 * period_arr0) next_opposition_dt  += period_arr0;
		while(opp_guess-next_opposition_dt   < -0.5 * period_arr0) next_opposition_dt  -= period_arr0;
		while(conj_guess-next_conjunction_dt >  0.5 * period_arr0) next_conjunction_dt += period_arr0;
		while(conj_guess-next_conjunction_dt < -0.5 * period_arr0) next_conjunction_dt -= period_arr0;

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
				double next_x = calc_next_x_find_min(data_dep, vinf_tolerance)*86400;

				if(next_x < 0) {
					Vector2 *data = data_array2_get_data(data_dep);
					size_t num_data = data_array2_size(data_dep);
					int min_idx = 0;
					for(int idx = 1; idx < num_data; idx++) {
						if(data[idx].y < data[min_idx].y) min_idx = idx;
					}
					data_array2_append_new(min_per_dep, jd_dep, data[min_idx].y);
					// print_date(convert_JD_date(jd_dep, DATE_ISO), 0);
					// printf("      %f  |   %f    %f    %f  |    %f    %f    %f\n", jd_dep, dt/86400, left_x/86400, right_x/86400, dt, left_x, right_x);
					// data_array2_append_new(min_per_dep, jd_dep-jd_min_dep, data[min_idx].x);
					break;
				}
				dt = next_x;
			}
		}
		// print_data_array2(data_dep, "dep", "dv");
	}

	data_array2_free(data_dep);




	// double next_line_up_dt, next_opp_line_up_dt;
	// calc_time_to_next_an_dn_line_up(osv0, osv_arr0, group->system->cb, &next_line_up_dt, &next_opp_line_up_dt);
	//
	// Orbit orbit0 = constr_orbit_from_osv(osv0.r, osv0.v, group->system->cb);
	// double period_dep = calc_orbital_period(orbit0);
	//
	// double max_x = data_array2_get_data(min_per_dep)[data_array2_size(min_per_dep)-1].x;
	// double min_x = data_array2_get_data(min_per_dep)[0].x;
	//
	// while(next_line_up_dt/86400 < max_x) {
	// 	if(next_opp_line_up_dt/86400 > min_x) {
	// 		for(int i = 0; i < 20; i++) {
	// 			data_array2_insert_new(min_per_dep, next_line_up_dt/86400, i*2000);
	// 			data_array2_insert_new(min_per_dep, next_opp_line_up_dt/86400, i*2000);
	// 		}
	// 	}
	// 	next_line_up_dt += period_dep;
	// 	next_opp_line_up_dt += period_dep;
	// }

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
		double dvinf = get_mesh_interpolated_value(mesh, vec2(jd_dep, init_lim[0].y)) - min_vinf;
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
			double dvinf = get_mesh_interpolated_value(mesh, vec2(jd_dep, dur)) - min_vinf;
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
		struct ItinStep *step = triangle->points[i]->data;
		tri_varrx[i] = vec3(triangle->points[i]->pos.x, triangle->points[i]->pos.y, step->v_arr.x);
		tri_varry[i] = vec3(triangle->points[i]->pos.x, triangle->points[i]->pos.y, step->v_arr.y);
		tri_varrz[i] = vec3(triangle->points[i]->pos.x, triangle->points[i]->pos.y, step->v_arr.z);
	}
	double varrx = get_triangle_interpolated_value(tri_varrx[0], tri_varrx[1], tri_varrx[2], vec2(jd_arr, dur));
	double varry = get_triangle_interpolated_value(tri_varry[0], tri_varry[1], tri_varry[2], vec2(jd_arr, dur));
	double varrz = get_triangle_interpolated_value(tri_varrz[0], tri_varrz[1], tri_varrz[2], vec2(jd_arr, dur));
	return vec3(varrx, varry, varrz);
}

Vector3 get_vbody_from_mesh(Mesh2 *mesh, double jd_arr, double dur) {
	MeshTriangle2 *triangle = get_mesh_triangle_at_position(mesh, vec2(jd_arr, dur));
	if(!triangle) return vec3(NAN, NAN, NAN);

	Vector3 tri_varrx[3];
	Vector3 tri_varry[3];
	Vector3 tri_varrz[3];

	for(int i = 0; i < 3; i++) {
		struct ItinStep *step = triangle->points[i]->data;
		tri_varrx[i] = vec3(triangle->points[i]->pos.x, triangle->points[i]->pos.y, step->v_body.x);
		tri_varry[i] = vec3(triangle->points[i]->pos.x, triangle->points[i]->pos.y, step->v_body.y);
		tri_varrz[i] = vec3(triangle->points[i]->pos.x, triangle->points[i]->pos.y, step->v_body.z);
	}
	double varrx = get_triangle_interpolated_value(tri_varrx[0], tri_varrx[1], tri_varrx[2], vec2(jd_arr, dur));
	double varry = get_triangle_interpolated_value(tri_varry[0], tri_varry[1], tri_varry[2], vec2(jd_arr, dur));
	double varrz = get_triangle_interpolated_value(tri_varrz[0], tri_varrz[1], tri_varrz[2], vec2(jd_arr, dur));
	return vec3(varrx, varry, varrz);
}

struct ItinStep * get_next_step_from_vinf(DepartureGroup *group, double v_inf, double jd_dep, double min_dur_dt, double max_dur_dt, bool leftside, double tolerance) {
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

FlyByGroups * get_flyby_groups_wrt_vinf(Mesh2 *mesh, DepartureGroup *departure_group, DataArray2 *vinf_limits, double tolerance) {
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

	double opp_guess, conj_guess;
	if(departure_group->top_boundary_type == DEPARTURE_GROUP_BOUNDARY_TOP_CONJ) {
		conj_guess = departure_group->boundary0_top.y + (jd_dep - departure_group->boundary0_top.x) * 86400 * departure_group->boundary_gradient;
		opp_guess = departure_group->boundary0_bottom.y + (jd_dep - departure_group->boundary0_bottom.x) * 86400 * departure_group->boundary_gradient;
	} else {
		opp_guess = departure_group->boundary0_top.y + (jd_dep - departure_group->boundary0_top.x) * 86400 * departure_group->boundary_gradient;
		conj_guess = departure_group->boundary0_bottom.y + (jd_dep - departure_group->boundary0_bottom.x) * 86400 * departure_group->boundary_gradient;
	}

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

		opp_guess = last_opposition_dt + (jd_dep-last_jd_dep)*86400*departure_group->boundary_gradient;
		conj_guess = last_conjunction_dt + (jd_dep-last_jd_dep)*86400*departure_group->boundary_gradient;

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
				double vinf = get_mesh_interpolated_value(mesh, vec2(jd_dep, dur_temp));
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

Mesh2 * get_rpe_mesh_from_fb_groups(FlyByGroups *fb_groups, Mesh2 *prev_mesh, DepartureGroup *prev_departure_group, bool left_side) {
	MeshGrid2 ***grids = malloc(fb_groups->num_groups*sizeof(MeshGrid2**));
	for(int i = 0; i < fb_groups->num_groups; i++) {
		grids[i] = malloc(fb_groups->num_groups_dep[i]*sizeof(MeshGrid2*));
	}

	for(int x_idx = 0; x_idx < fb_groups->num_groups; x_idx++) {
		for(int y_idx = 0; y_idx < fb_groups->num_groups_dep[x_idx]; y_idx++) {
			void *steps = (void*) (left_side ? fb_groups->groups[x_idx][y_idx].left_steps : fb_groups->groups[x_idx][y_idx].right_steps);
			grids[x_idx][y_idx] = create_mesh_grid(fb_groups->groups[x_idx][y_idx].dep_dur, steps);
		}
	}
	Mesh2 *rpe_mesh = create_mesh_from_multiple_grids_w_angled_guideline(grids, fb_groups->num_groups, fb_groups->num_groups_dep, prev_departure_group->boundary_gradient);

	for(int x_idx = 0; x_idx < fb_groups->num_groups; x_idx++) {
		for(int y_idx = 0; y_idx < fb_groups->num_groups_dep[x_idx]; y_idx++) {
			free_grid_keep_points(grids[x_idx][y_idx]);
		}
		free(grids[x_idx]);
	}
	free(grids);

	for(int i = 0; i < rpe_mesh->num_points; i++) {
		struct ItinStep *ptr = rpe_mesh->points[i]->data;
		double jd_dep = rpe_mesh->points[i]->pos.x;
		double dur = rpe_mesh->points[i]->pos.y;
		Vector3 v_arr = get_varr_from_mesh(prev_mesh, jd_dep, dur);
		Vector3 v_body = get_vbody_from_mesh(prev_mesh, jd_dep, dur);
		double r_pe = get_flyby_periapsis(v_arr, ptr->v_dep, v_body, prev_departure_group->arr_body);
		rpe_mesh->points[i]->val = r_pe;
	}

	return rpe_mesh;
}




FlyByGroups * get_refined_departure_groups(DepartureGroup *departure_group, DataArray2 *limits, double dep_periapsis, double max_dep_dv, double dv_tolerance) {
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
			calc_bounded_porkchop_line(departure, departure_group->arr_body, departure_group->system, min_dt, max_dt, dep_periapsis, max_dep_dv, dv_tolerance);
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

Mesh2 * get_dep_mesh_from_fb_groups(FlyByGroups *fb_groups, DepartureGroup *departure_group) {
	MeshGrid2 ***grids = malloc(fb_groups->num_groups*sizeof(MeshGrid2**));
	for(int i = 0; i < fb_groups->num_groups; i++) {
		grids[i] = malloc(fb_groups->num_groups_dep[i]*sizeof(MeshGrid2*));
	}

	for(int x_idx = 0; x_idx < fb_groups->num_groups; x_idx++) {
		for(int y_idx = 0; y_idx < fb_groups->num_groups_dep[x_idx]; y_idx++) {
			void *steps = (void*) fb_groups->groups[x_idx][y_idx].left_steps;
			grids[x_idx][y_idx] = create_mesh_grid(fb_groups->groups[x_idx][y_idx].dep_dur, steps);
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

	for(int i = 0; i < mesh->num_points; i++) {
		struct ItinStep *ptr = mesh->points[i]->data;
		double vinf = mag_vec3(subtract_vec3(ptr->v_arr, ptr->v_body));
		mesh->points[i]->val = vinf;
	}

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