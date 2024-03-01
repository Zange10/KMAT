//
// Created by niklas on 17.01.24.
//

#include "double_swing_by.h"
#include "tools/csv_writer.h"
#include <sys/time.h>
#include <math.h>
#include <stdio.h>
#include "tools/data_tool.h"
#include <stdlib.h>



struct Swingby_Peak_Search_Params {
	struct Orbit orbit;
	double *interval;
	struct DSB *dsb;
	struct Body *body;
	struct Vector v_t00;
};


struct OSV s0,s1,p0,p1;
double transfer_duration;
struct Body *body;

double csv_data[1000000];


double test[2];


void find_double_swing_by_zero_sec_sb_diff(struct Swingby_Peak_Search_Params spsp) {
	struct timeval start, end;
	double elapsed_time;
	struct Vector v_t00 = spsp.v_t00;
	struct DSB *dsb = spsp.dsb;

	double min_dtheta = spsp.interval[0];
	double max_dtheta = spsp.interval[1];

	int right_side = 0;	// 0 = left, 1 = right

	// x: dtheta, y: diff_vinf (data[0].x: number of data points beginning at index 1)
	struct Vector2D data[101];
	data[0].x = 0;

	double dtheta, last_dtheta, diff_vinf = 1e9;

	for(int i = 0; i < 100; i++) {
		if(i == 0) dtheta = min_dtheta;

		gettimeofday(&start, NULL);  // Record the starting time
		struct OSV osv_m0 = propagate_orbit_theta(p0.r, v_t00, dtheta, SUN());

		gettimeofday(&end, NULL);  // Record the ending time
		elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
		test[0] += elapsed_time;

		double duration = calc_dt_from_dtheta(spsp.orbit, dtheta);

		//printf("%f, %f, %f, %f, %f\n\n", rad2deg(dtheta), rad2deg(min_dtheta), rad2deg(max_dtheta), duration/86400, transfer_duration);

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

		if(dtheta == min_dtheta && diff_vinf < 0) right_side = 1;
		if(dtheta == max_dtheta && diff_vinf < 0) break;

		if (fabs(diff_vinf) < 10) {
			double beta = (M_PI - angle_vec_vec(temp1, temp2)) / 2;
			double rp = (1 / cos(beta) - 1) * (body->mu / (pow(vector_mag(temp1), 2)));
			if (rp > body->radius + body->atmo_alt) {
				if (mag < dsb->dv) {
					dsb->osv[0].r = p0.r;
					dsb->osv[0].v = v_t00;
					dsb->osv[1] = osv_m0;
					dsb->osv[2].r = transfer.r0;
					dsb->osv[2].v = transfer.v0;
					dsb->osv[3].r = transfer.r1;
					dsb->osv[3].v = transfer.v1;
					dsb->dv = mag;
					dsb->man_time = duration;	// gets converted to JD time instead of duration later
				}
			}
			if(!right_side) right_side = 1;
			else break;
		}

		insert_new_data_point(data, dtheta, diff_vinf);

		if(!can_be_negative_monot_deriv(data)) break;
		last_dtheta = dtheta;
		if(i == 0) dtheta = max_dtheta;
		else dtheta = root_finder_monot_deriv_next_x(data, right_side ? 1 : 0);
		if(i > 3 && dtheta == last_dtheta) break;
		if(dtheta > max_dtheta) dtheta = (max_dtheta+data[(int) data[0].x].x)/2;
		if(dtheta < min_dtheta) dtheta = (min_dtheta+data[1].x)/2;
		if(isnan(dtheta) || isinf(dtheta)) break;
	}
}


struct DSB calc_man_for_dsb(struct Vector v_soi) {
	struct DSB dsb = {.man_time = -1, .dv = 1e9};
	double mu = body->orbit.body->mu;
	double tolerance = 0.05;

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

	// dtheta at which conjuction/opposition occurs (Sun, satellite, planet) - set to conj/opp before first fly-by
	double theta_conj_opp = angle_vec_vec(p0.r, p1.r);
	if(negative_true_anomaly) theta_conj_opp *= -1;
	else theta_conj_opp -= M_PI;

	double interval[2];

	int counter = 0;
	// three days buffer after and before fly-bys (~SOI)
	double max_dtheta = calc_dtheta_from_dt(orbit, transfer_duration*86400-86400*3);
	double min_dtheta = calc_dtheta_from_dt(orbit, 86400*3);

	double dtheta0, dtheta1;

	do {
		dtheta0 = theta_conj_opp + M_PI*(  counter);
		dtheta1 = theta_conj_opp + M_PI*(1+counter);

		interval[0] = dtheta0;// + deg2rad(0.5);
		if (interval[0] < min_dtheta) interval[0] = min_dtheta;

		interval[1] = dtheta1;// - deg2rad(0.5);
		if (interval[1] > max_dtheta) interval[1] = max_dtheta;

		counter++;

		struct Swingby_Peak_Search_Params spsp2 = {
				orbit,
				interval,
				&dsb,
				body,
				v_t00
		};


		find_double_swing_by_zero_sec_sb_diff(spsp2);
	} while (dtheta1 < max_dtheta);

	return dsb;
}

struct DSB calc_double_swing_by(struct OSV _s0, struct OSV _p0, struct OSV _s1, struct OSV _p1, double _transfer_duration, struct Body *_body) {
	csv_data[0] = 1;
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

	int num_angle_analyse = 10;
	
	double min_rp = body->radius+body->atmo_alt;
	struct Vector v_soi0 = add_vectors(s0.v, scalar_multiply(p0.v,-1));
	double v_inf = vector_mag(v_soi0);
	double min_beta = acos(1 / (1 + (min_rp * pow(v_inf, 2)) / body->mu));
	double max_defl = M_PI - 2*min_beta;
	
	//printf("\nmin beta: %.2f°; max deflection: %.2f° (vinf: %f m/s)\n\n", rad2deg(min_beta), rad2deg(max_defl), v_inf);

	double angle_step_size, phi, kappa, max_phi, max_kappa;
	double angles[3];
	double min_phi, min_kappa;
	double min_phi_kappa[2];
	
	for(int i = 0; i < 1; i++) {
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
			struct Vector rot_axis_1 = norm_vector(cross_product(v_soi0, p0.r));
			struct Vector v_soi_ = rotate_vector_around_axis(v_soi0, rot_axis_1, phi);
			kappa = i == 0 ? -max_defl - angle_step_size : angles[2] - angles[0] - angle_step_size;
			min_phi_kappa[1] = i == 0 ? -max_defl : angles[2] - angles[0];
			
			//printf("%f° %f°\n", rad2deg(phi), rad2deg(max_defl));
			while (kappa <= max_kappa) {
				kappa += angle_step_size;
				
				double defl = acos(cos(phi)*sin(M_PI_2 - kappa));
				if (defl > max_defl) continue;
				
				struct Vector rot_axis_2 = norm_vector(cross_product(v_soi_, rot_axis_1));
				struct Vector v_soi = rotate_vector_around_axis(v_soi_, rot_axis_2, kappa);
				
				struct DSB temp_dsb = calc_man_for_dsb(v_soi);
				
				if(temp_dsb.dv < 1e8) {
					csv_data[(int)csv_data[0]    ] = rad2deg(phi);
					csv_data[(int)csv_data[0] + 1] = rad2deg(kappa);
					csv_data[(int)csv_data[0] + 2] = temp_dsb.dv;
					csv_data[(int)csv_data[0] + 3] = temp_dsb.man_time / (86400);
					csv_data[(int)csv_data[0] + 4] = constr_orbit_from_osv(temp_dsb.osv[1].r, temp_dsb.osv[1].v, SUN()).period/86400;
					csv_data[0] += 5;
				}
				if(temp_dsb.man_time>0 && temp_dsb.dv < dsb.dv) {
					dsb = temp_dsb;
					min_phi = phi;
					min_kappa = kappa;
				}
			}
		}
		gettimeofday(&end, NULL);  // Record the ending time
		elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
		//printf("| Elapsed time: %.3f s |  (%f - %f - %f)\n", elapsed_time, test[0], test[1], test[0]+test[1]);
		//printf("min_dv: %f\n", dsb.dv);
		if(dsb.dv >= 1e9) break;

		double phi0, kappa0;
		double l = 0.25 * max_defl / num_angle_analyse;

		while(l > max_defl/10000) {
			phi0 = min_phi;
			kappa0 = min_kappa;

			for(int c = 0; c < 8; c++) {
				switch(c) {
					case 0:
						phi = phi0 + 3*l;
						kappa = kappa0;
						break;
					case 1:
						phi = phi0 - 3*l;
						kappa = kappa0;
						break;
					case 2:
						phi = phi0;
						kappa = kappa0 + 3*l;
						break;
					case 3:
						phi = phi0;
						kappa = kappa0 - 3*l;
						break;
					case 4:
						phi = phi0 + 2*l;
						kappa = kappa0 + 2*l;
						break;
					case 5:
						phi = phi0 - 2*l;
						kappa = kappa0 + 2*l;
						break;
					case 6:
						phi = phi0 + 2*l;
						kappa = kappa0 - 2*l;
						break;
					case 7:
						phi = phi0 - 2*l;
						kappa = kappa0 - 2*l;
						break;
				}


				struct Vector rot_axis_1 = norm_vector(cross_product(v_soi0, p0.r));
				struct Vector v_soi_ = rotate_vector_around_axis(v_soi0, rot_axis_1, phi);

				struct Vector rot_axis_2 = norm_vector(cross_product(v_soi_, rot_axis_1));
				struct Vector v_soi = rotate_vector_around_axis(v_soi_, rot_axis_2, kappa);

				struct DSB temp_dsb = calc_man_for_dsb(v_soi);

				if(temp_dsb.man_time > 0 && temp_dsb.dv < dsb.dv) {
					dsb = temp_dsb;
					min_phi = phi;
					min_kappa = kappa;
					csv_data[(int) csv_data[0]] = rad2deg(min_phi);
					csv_data[(int) csv_data[0] + 1] = rad2deg(min_kappa);
					csv_data[(int) csv_data[0] + 2] = dsb.dv;
					csv_data[(int) csv_data[0] + 3] = dsb.man_time / (86400);
					csv_data[(int) csv_data[0] + 4] =
							constr_orbit_from_osv(dsb.osv[1].r, dsb.osv[1].v, SUN()).period / 86400;
					csv_data[0] += 5;
					break;
				}
			}
			//printf("%f %f %f %f\n", dsb.dv, rad2deg(min_phi), rad2deg(min_kappa), rad2deg(l));
			if(min_phi == phi0 && min_kappa == kappa0) l /= 2;
		}
	}

	char flight_data_fields[] = "Phi,Kappa,DV,Duration,T";
	write_csv(flight_data_fields, csv_data);
	
	return dsb;
}

