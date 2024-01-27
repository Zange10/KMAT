//
// Created by niklas on 17.01.24.
//

#include "double_swing_by.h"
#include "csv_writer.h"
#include <sys/time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>



struct Swingby_Peak_Search_Params {
	double peak_dur;
	struct Orbit orbit;
	double *interval;
	struct DSB *dsb;
	struct Body *body;
	struct Vector v_t00;
};


struct OSV s0,s1,p0,p1;
double transfer_duration;
struct Body *body;


double x[1000000];


double test[2];


int find_double_swing_by_zero_sec_sb_diff(struct Swingby_Peak_Search_Params spsp, int only_right_leg) {
	struct timeval start, end;
	double elapsed_time;
	double peak_dur = spsp.peak_dur;
	double T = spsp.orbit.period;
	struct Vector v_t00 = spsp.v_t00;
	struct DSB *dsb = spsp.dsb;
	
	double min_dur = spsp.interval[0];
	double max_dur = spsp.interval[1];
	
	int is_edge = 0;
	int right_leg_only_positive = 1;
	for(int i = -1 + only_right_leg*2; i <= 1; i+=2) {
		double step = i*86400.0 * 10;
		double dur = peak_dur+step;
		int temp_counter = 0;
		if(peak_dur < min_dur) {
			i+=2;
			if(i>1) break;
			dur = min_dur;
			is_edge = 1;
		}
		if(peak_dur > max_dur) {
			if(i==1) break;
			dur = max_dur;
			is_edge = 1;
		}
		double diff_vinf = 1e9;
		double last_diff_vinf = 1e9;
		while (fabs(diff_vinf) > 10) {
			temp_counter++;
			if(temp_counter > 1000) {
				break; // break endless loop
			}
			//if (dur > max_dur) break;
			gettimeofday(&start, NULL);  // Record the starting time
			struct OSV osv_m0 = propagate_orbit_time(p0.r, v_t00, dur, SUN());
			gettimeofday(&end, NULL);  // Record the ending time
			elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
			test[0] += elapsed_time;
			if(rad2deg(angle_vec_vec(osv_m0.r, p1.r)) < 0.5) {
				dur += step;
				step += step;
				continue;
			}
			gettimeofday(&start, NULL);  // Record the starting time
			
			struct Transfer transfer = calc_transfer(capfb, body, body, osv_m0.r, osv_m0.v, p1.r, p1.v,
													 transfer_duration * 86400 - dur, NULL);
			gettimeofday(&end, NULL);  // Record the ending time
			elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
			test[1] += elapsed_time;
			struct Vector temp = add_vectors(transfer.v0, scalar_multiply(osv_m0.v, -1));
			double mag = vector_mag(temp);
			
			struct Vector temp1 = add_vectors(transfer.v1, scalar_multiply(p1.v, -1));
			struct Vector temp2 = add_vectors(s1.v, scalar_multiply(p1.v, -1));
			
			diff_vinf = vector_mag(temp1) - vector_mag(temp2);
			if(peak_dur < 0 && dur == min_dur && diff_vinf < 0) return 0;
			if(peak_dur > max_dur && dur == max_dur && diff_vinf < 0) return 1;
			if (fabs(diff_vinf) < 10) {
				double beta = (M_PI - angle_vec_vec(temp1, temp2)) / 2;
				double rp = (1 / cos(beta) - 1) * (body->mu / (pow(vector_mag(temp1), 2)));
				if (rp > body->radius + body->atmo_alt) {
					if (mag < dsb->dv) {
						dsb->dv = mag;
						dsb->man_time = dur;	// gets converted to JD time instead of duration later
					}
				}
				break;
			}
			
			double gradient = (diff_vinf - last_diff_vinf)/step;

			printf("%g %f\n", diff_vinf, dur/86400);
			if(diff_vinf > 0) {
				double max_rel_T_pos = T;
				if(dur+T > max_dur) max_rel_T_pos = max_dur-dur;
				if(i == -1 && max_rel_T_pos > peak_dur) max_rel_T_pos = peak_dur;
				double max_rel_T_neg = -T;
				if(dur-T < 0) max_rel_T_neg = -dur;
				if(i == 1 && max_rel_T_neg < peak_dur) max_rel_T_neg = peak_dur;
				//printf("max_rel(+): %f, max_rel(-): %f, gradient: %f, gradT(+): %f, gradT(-): %f\n", max_rel_T_pos/86400, max_rel_T_neg/86400, gradient, diff_vinf+gradient*max_rel_T_pos, diff_vinf+gradient*max_rel_T_neg);
				if(diff_vinf+gradient*max_rel_T_pos > 0 && diff_vinf+gradient*max_rel_T_neg > 0 && (fabs(step) < 86400 || is_edge)) {
					//printf("GRADIENT: %f, %f, %f, %f\n", gradient, step, gradient*T, diff_vinf);
					break;
				}
				if(step*i > 0 == gradient*i > 0 && dur != min_dur && dur != max_dur) {
					step /= -4;
				}
			} else {
				if(right_leg_only_positive && i == 1) right_leg_only_positive = 0;
				if(step*i > 0 && dur != min_dur && dur != max_dur && dur != peak_dur) {
					step /= -4;
				}
			}
			
			dur += step;
			if(dur < min_dur) {
				step /= -4;
				dur = min_dur;
			} else if(dur > max_dur) {
				step /= -4;
				dur = max_dur;
			}
			if(i == 1 && dur < peak_dur && peak_dur > min_dur) {
				step /= -4;
				dur = peak_dur;
			} else if(i == -1 && dur > peak_dur && peak_dur < max_dur) {
				step /= -4;
				dur = peak_dur;
			}
			last_diff_vinf = diff_vinf;
		}
	}
	return right_leg_only_positive;
}



void insert_new_data_point(struct Vector2D data[], struct Vector2D new_data_point) {
	for (int i = 1; i <= data[0].x; i++) {
		if (new_data_point.x < data[i].x) {

			for (int j = (int) data[0].x; j >= i; j--) data[j + 1] = data[j];

			data[i] = new_data_point;
			data[0].x++;
			return;
		}
	}

	data[(int)data[0].x+1] = new_data_point;
	data[0].x++;
}


int can_be_negative(struct Vector2D *data) {
	int num_data = (int) data[0].x;
	if(num_data < 3) return 1;

	int mind = 1;
	double min = data[1].y;
	for(int i = 2; i <= num_data; i++) {
		if(data[i].y < min) { mind = i; min = data[i].y; }
		if(min < 0) return 1;
	}

	if(mind == 1) mind++;
	if(mind == num_data) mind--;

	// gradient on left side and see whether it can get negetive on right side
	double gradient = (data[mind].y - data[mind - 1].y) / (data[mind].x - data[mind - 1].x);
	double dx = data[mind+1].x - data[mind].x;
	if(gradient*dx + data[mind].y < 0) return 1;

	// gradient on right side and see whether it can get negetive on left side
	gradient = (data[mind].y - data[mind + 1].y) / (data[mind].x - data[mind + 1].x);
	dx = data[mind-1].x - data[mind].x;
	if(gradient*dx + data[mind].y < 0) return 1;

	return 0;
}

double get_next_dtheta_from_data_points(struct Vector2D *data, int branch) {
	// branch = 0 for left branch, 1 for right branch
	int num_data = (int) data[0].x;
	int index;

	// left branch
	if(branch == 0) {
		for(int i = 2; i <= num_data; i++) {
			if(data[i].y < 0)			{ index = i; break; }
			if(data[i].y > data[i-1].y)	{ break; }
			else 						{ index = i; }
		}

	// right branch
	} else {
		for(int i = num_data-1; i >= 1; i--) {
			if(data[i].y < 0)			{ index = i; break; }
			if(data[i].y > data[i+1].y)	{ break; }
			else 						{ index = i; }
		}
	}

	if(data[index].y < 0) {
		if(branch == 0) return (data[index].x + data[index-1].x)/2;
		else 			return (data[index].x + data[index+1].x)/2;
	}

	if(index == 1)			return (data[1].x + data[2].x)/2;
	if(index == num_data)	return (data[num_data-1].x + data[num_data].x)/2;



	return (data[index].x + data[index+1].x)/2;
}



int find_double_swing_by_zero_sec_sb_diff2(struct Swingby_Peak_Search_Params spsp, int only_right_leg) {
	struct timeval start, end;
	double elapsed_time;
	double peak_dtheta = spsp.peak_dur;
	struct Vector v_t00 = spsp.v_t00;
	struct DSB *dsb = spsp.dsb;

	double min_dtheta = spsp.interval[0];
	double max_dtheta = spsp.interval[1];

	int is_edge = 0;
	int right_leg_only_positive = 1;
	for(int i = -1 + only_right_leg*2; i <= 1; i+=2) {
		double step = deg2rad(10)*i;
		double dtheta = peak_dtheta + step;
		int temp_counter = 0;
		if(peak_dtheta < min_dtheta) {
			i+=2;
			if(i>1) break;
			dtheta = min_dtheta;
			is_edge = 1;
		}
		if(peak_dtheta > max_dtheta) {
			if(i==1) break;
			dtheta = max_dtheta;
			is_edge = 1;
		}
		double diff_vinf = 1e9;
		double last_diff_vinf = 1e9;
		while (fabs(diff_vinf) > 10) {
			temp_counter++;
			if(temp_counter > 1000) {
				break; // break endless loop
			}
			//if (dur > max_dur) break;
			gettimeofday(&start, NULL);  // Record the starting time
			struct OSV osv_m0 = propagate_orbit_theta(p0.r, v_t00, dtheta, SUN());
			gettimeofday(&end, NULL);  // Record the ending time
			elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
			test[0] += elapsed_time;
			if(rad2deg(angle_vec_vec(osv_m0.r, p1.r)) < 0.5) {
				dtheta += step;
				step += step;
				continue;
			}

			double duration = calc_dt_from_dtheta(spsp.orbit, dtheta);

			gettimeofday(&start, NULL);  // Record the starting time

			struct Transfer transfer = calc_transfer(capfb, body, body, osv_m0.r, osv_m0.v, p1.r, p1.v,
													 transfer_duration * 86400 - duration, NULL);
			gettimeofday(&end, NULL);  // Record the ending time
			elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
			test[1] += elapsed_time;
			struct Vector temp = add_vectors(transfer.v0, scalar_multiply(osv_m0.v, -1));
			double mag = vector_mag(temp);

			struct Vector temp1 = add_vectors(transfer.v1, scalar_multiply(p1.v, -1));
			struct Vector temp2 = add_vectors(s1.v, scalar_multiply(p1.v, -1));

			diff_vinf = vector_mag(temp1) - vector_mag(temp2);
			if(peak_dtheta < 0 && dtheta == min_dtheta && diff_vinf < 0) return 0;
			if(peak_dtheta > max_dtheta && dtheta == max_dtheta && diff_vinf < 0) return 1;
			if (fabs(diff_vinf) < 10) {
				double beta = (M_PI - angle_vec_vec(temp1, temp2)) / 2;
				double rp = (1 / cos(beta) - 1) * (body->mu / (pow(vector_mag(temp1), 2)));
				if (rp > body->radius + body->atmo_alt) {
					if (mag < dsb->dv) {
						dsb->dv = mag;
						dsb->man_time = duration;	// gets converted to JD time instead of duration later
					}
				}
				break;
			}

			double gradient = (diff_vinf - last_diff_vinf)/step;

			printf("%g %f\n", diff_vinf, duration/86400);
			if(diff_vinf > 0) {
				double max_rel_T_pos = 2*M_PI;
				if(dtheta+2*M_PI > max_dtheta) max_rel_T_pos = max_dtheta-dtheta;
				if(i == -1 && max_rel_T_pos > peak_dtheta) max_rel_T_pos = peak_dtheta;
				double max_rel_T_neg = -2*M_PI;
				if(dtheta-2*M_PI < 0) max_rel_T_neg = -dtheta;
				if(i == 1 && max_rel_T_neg < peak_dtheta) max_rel_T_neg = peak_dtheta;
				//printf("max_rel(+): %f, max_rel(-): %f, gradient: %f, gradT(+): %f, gradT(-): %f\n", max_rel_T_pos/86400, max_rel_T_neg/86400, gradient, diff_vinf+gradient*max_rel_T_pos, diff_vinf+gradient*max_rel_T_neg);
				if(diff_vinf+gradient*max_rel_T_pos > 0 && diff_vinf+gradient*max_rel_T_neg > 0 && (fabs(step) < 86400 || is_edge)) {
					//printf("GRADIENT: %f, %f, %f, %f\n", gradient, step, gradient*T, diff_vinf);
					break;
				}
				if(step*i > 0 == gradient*i > 0 && dtheta != min_dtheta && dtheta != max_dtheta) {
					step /= -4;
				}
			} else {
				if(right_leg_only_positive && i == 1) right_leg_only_positive = 0;
				if(step*i > 0 && dtheta != min_dtheta && dtheta != max_dtheta && dtheta != peak_dtheta) {
					step /= -4;
				}
			}

			dtheta += step;
			if(dtheta < min_dtheta) {
				step /= -4;
				dtheta = min_dtheta;
			} else if(dtheta > max_dtheta) {
				step /= -4;
				dtheta = max_dtheta;
			}
			if(i == 1 && dtheta < peak_dtheta && peak_dtheta > min_dtheta) {
				step /= -4;
				dtheta = peak_dtheta;
			} else if(i == -1 && dtheta > peak_dtheta && peak_dtheta < max_dtheta) {
				step /= -4;
				dtheta = peak_dtheta;
			}
			last_diff_vinf = diff_vinf;
		}
	}
	return right_leg_only_positive;
}





int c_temp = 0;
int c_temp_total = 0;

struct DSB temp(struct Vector v_soi) {
	struct DSB dsb = {.man_time = -1, .dv = 1e9};
	double mu = body->orbit.body->mu;
	//double tolerance = 0.05;

    // velocity vector after first swing-by
	struct Vector v_t00 = add_vectors(v_soi, p0.v);
	// orbit after first swing-by
	struct Orbit orbit = constr_orbit_from_osv(s0.r, v_t00, body->orbit.body);
	double T = orbit.period;
	
	double body_T = M_PI*2 * sqrt(pow(body->orbit.a,3)/mu);
	// is true, when transfer point goes backwards on body orbit
	int negative_true_anomaly = ((transfer_duration*86400)/body_T - (int)((transfer_duration*86400)/body_T)) > 0.5;
	
	double T_ratio = T / body_T - (int) (T / body_T);
	
	// maybe think about more resonances in future (not only 1:1, 2:1, 3:1,...)
	/*if (T_ratio > tolerance && T_ratio < 1 - tolerance) {
		dsb.man_time = -1;
		return dsb;	// skip this iteration
	}*/
	
	// true anomaly at which conjuction/opposition occurs (not correct yet, gets modified few lines down)
	double theta_conj_opp = angle_vec_vec(p0.r, p1.r);
	if(negative_true_anomaly) theta_conj_opp *= -1;
	
	double a = orbit.a;
	double e = orbit.e;
	double theta = orbit.theta;
	double t0 = orbit.t;
	double n = sqrt(mu / pow(a, 3));
	
	theta_conj_opp += theta;
	theta_conj_opp = pi_norm(theta_conj_opp);
	
	double E1 = acos((e + cos(theta_conj_opp))/(1 + e*cos(theta_conj_opp)));
	theta_conj_opp += M_PI;
	double E2 = acos((e + cos(theta_conj_opp))/(1 + e*cos(theta_conj_opp)));
	theta_conj_opp -= M_PI;
	
	// times, when Sun, body and satellite are in one line (opposition/conjuction)
	double t1 = (E1 - e * sin(E1)) / n - t0;
	double t2 = (E2 - e * sin(E2)) / n - t0;
	
	if (theta_conj_opp < M_PI) 	t2 = T - t2;
	else 						t1 = T - t1;
	
	while (t1 > T) t1 -= T;
	while (t2 > T) t2 -= T;
	while (t1 < 0) t1 += T;
	while (t2 < 0) t2 += T;
	
	if (t2 < t1) {
		double temp = t1;
		t1 = t2;
		t2 = temp;
	}
	
	double interval[2];
	
	double t = 0;
	int counter = -1;
	double max_dur = transfer_duration * 86400 - T * 0.1;
	double min_dur = 86400;
	double t1t2 = t2 - t1;
	double t2t1 = (t1 + T) - t2;
	
	int right_leg_only_positive = 0;
	
	while (t < transfer_duration * 86400) {
		if (counter % 2 == 0) {
			t = t1 + T * (counter / 2);
			interval[0] = t - t2t1;
			if (interval[0] < min_dur) interval[0] = min_dur;
			interval[1] = t + t1t2;
			if (interval[1] > max_dur) interval[1] = max_dur;
		} else {
			t = t2 + T * (counter / 2);
			if (counter < 0) t -= T;
			interval[0] = t - t1t2;
			if (interval[0] < min_dur) interval[0] = min_dur;
			interval[1] = t + t2t1;
			if (interval[1] > max_dur) interval[1] = max_dur;
		}
		
		
		counter++;

		printf("--- %d %f %f %f (%f %f) --------\n", right_leg_only_positive, interval[0]/86400, t/86400, interval[1]/86400, dsb.dv, dsb.man_time/86400);

		if(0) {

			struct Swingby_Peak_Search_Params spsp = {
					t,
					orbit,
					interval,
					&dsb,
					body,
					v_t00
			};


			right_leg_only_positive = find_double_swing_by_zero_sec_sb_diff(spsp, right_leg_only_positive);

		} else {
			interval[0] = calc_dtheta_from_dt(orbit, interval[0]);
			interval[1] = calc_dtheta_from_dt(orbit, interval[1]);
			double peak_theta = calc_dtheta_from_dt(orbit, t);


			struct Swingby_Peak_Search_Params spsp2 = {
					peak_theta,
					orbit,
					interval,
					&dsb,
					body,
					v_t00
			};


			right_leg_only_positive = find_double_swing_by_zero_sec_sb_diff2(spsp2, right_leg_only_positive);
			//exit(0);
		}
		c_temp_total++;
	}
	
	return dsb;
}


double f(double t) {
	return 4*t*t-3;
}

struct DSB calc_double_swing_by(struct OSV _s0, struct OSV _p0, struct OSV _s1, struct OSV _p1, double _transfer_duration, struct Body *_body) {
	struct Vector2D data[30] = { 2, 0, -10, f(-10), 5, f(5)};

	for(int i = 0; i < 10; i++) {
		if(!can_be_negative(data)) {
			printf("\nCant be negative!!!\n");
			break;
		}
		double next_x = get_next_dtheta_from_data_points(data, 0);
		struct Vector2D new_data_point = {next_x, f(next_x)};

		insert_new_data_point(data, new_data_point);
		printf("%f %f\n", new_data_point.x, new_data_point.y);
	}




	exit(0);
	x[0] = 1;
	struct DSB dsb = {.dv = 1e9};
	s0 = _s0;
	p0 = _p0;
	s1 = _s1;
	p1 = _p1;
	transfer_duration = _transfer_duration;
	body = _body;
	
	struct timeval start, end;
	double elapsed_time;
	
	gettimeofday(&start, NULL);  // Record the ending time
	
	test[0] = 0;
	test[1] = 0;
	
	double target_max = 2500;
	double tol_it1 = 4000;
	double tol_it2 = 500;
	int num_angle_analyse = 50;
	
	double min_rp = body->radius+body->atmo_alt;
	struct Vector v_soi0 = add_vectors(s0.v, scalar_multiply(p0.v,-1));
	double v_inf = vector_mag(v_soi0);
	double min_beta = acos(1 / (1 + (min_rp * pow(v_inf, 2)) / body->mu));
	double max_defl = M_PI - 2*min_beta;
	
	printf("\nmin beta: %.2f°; max deflection: %.2f° (vinf: %f m/s)\n\n", rad2deg(min_beta), rad2deg(max_defl), v_inf);
	
	double angle_step_size, phi, kappa, max_phi, max_kappa;
	double angles[3];
	double min_dv, min_man_t, min_phi, min_kappa;
	double min_phi_kappa[2];
	
	for(int i = 0; i < 1; i++) {
		min_dv = 1e9;
		if (i == 0) {
			angle_step_size = 2 * max_defl / num_angle_analyse;
			phi = -max_defl - angle_step_size;
			max_phi = max_defl;
			max_kappa = max_defl;
			min_phi_kappa[0] = -max_defl;
		} else {
			angle_step_size = 2 * angles[0] / num_angle_analyse;
			phi = angles[1] - angles[0] - angle_step_size;
			max_phi = angles[1] + angles[0];
			max_kappa = angles[2] + angles[0];
			min_phi_kappa[0] = angles[1] - angles[0];
		}
		
		while (phi <= max_phi) {
			phi += angle_step_size;
			phi = deg2rad(20);
			struct Vector rot_axis_1 = norm_vector(cross_product(v_soi0, p0.r));
			struct Vector v_soi_ = rotate_vector_around_axis(v_soi0, rot_axis_1, phi);
			kappa = i == 0 ? -max_defl - angle_step_size : angles[2] - angles[0] - angle_step_size;
			min_phi_kappa[1] = i == 0 ? -max_defl : angles[2] - angles[0];
			
			printf("%f° %f°\n", rad2deg(phi), rad2deg(max_defl));
			while (kappa <= max_kappa) {
				kappa += angle_step_size;
				kappa = deg2rad(-21);
				
				double defl = acos(cos(phi)*sin(M_PI_2 - kappa));
				if (defl > max_defl) continue;
				
				//printf("%f° %f° %f°\n", rad2deg(phi), rad2deg(kappa), rad2deg(defl));
				
				struct Vector rot_axis_2 = norm_vector(cross_product(v_soi_, rot_axis_1));
				struct Vector v_soi = rotate_vector_around_axis(v_soi_, rot_axis_2, kappa);
				
				//printf("(%3d°, %3d°, %6.2f°)  -  %.2f°\n", i, j, rad2deg(beta), rad2deg(angle));
				
				struct DSB temp_dsb = temp(v_soi);
				
				if(temp_dsb.dv < 1e8) {
					c_temp++;
					x[(int)x[0]    ] = rad2deg(phi);
					x[(int)x[0] + 1] = rad2deg(kappa);
					x[(int)x[0] + 2] = temp_dsb.dv;
					x[(int)x[0] + 3] = temp_dsb.man_time/(86400);
					x[0] += 4;
				}
				if(temp_dsb.man_time>0 && temp_dsb.dv < dsb.dv) {
					dsb = temp_dsb;
				}

				printf("%f %f\n", dsb.dv, dsb.man_time/86400);
				exit(0);
			}
		}
		gettimeofday(&end, NULL);  // Record the ending time
		elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
		//printf("%f  (%f°, %f°, %f°)  %d %d", min_dv, rad2deg(min_phi), rad2deg(min_kappa), rad2deg(angle_step_size), total_counter, partial_counter);
		printf("| Elapsed time: %.3f s |  (%f - %f - %f)\n", elapsed_time, test[0], test[1], test[0]+test[1]);
		printf("min_dv: %f\n", dsb.dv);
		if(min_dv >= 1e9) break;
		
		angles[0] = i == 0 ? max_defl/2 : angles[0]/8;
		num_angle_analyse -= 5;
		if(i == 0) num_angle_analyse = 25;
		angles[1] = min_phi;
		angles[2] = min_kappa;
		
		
		
		double min_phi_temp, min_kappa_temp;
		if (i == 0) {
			min_phi_temp = -max_defl - angle_step_size;
		} else {
			angle_step_size = 2 * angles[0] / num_angle_analyse;
			phi = angles[1] - angles[0] - angle_step_size;
			max_phi = angles[1] + angles[0];
			max_kappa = angles[2] + angles[0];
		}
	}
	char flight_data_fields[] = "Phi,Kappa,DV,Duration";
	write_csv(flight_data_fields, x);
	
	return dsb;
}

