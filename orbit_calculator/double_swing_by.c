//#include "double_swing_by.h"
//#include <sys/time.h>
//#include <math.h>
//#include <stdio.h>
//#include "tools/data_tool.h"
//
//
//
//struct Swingby_Peak_Search_Params {
//	struct Orbit orbit;
//	double *interval;
//	struct DSB *dsb;
//	struct Body *body;
//	struct Vector v_t00;
//};
//
//struct DSB_Data {
//	struct OSV s0,s1,p0,p1;
//	double transfer_duration;
//	struct Body *body;
//};
//
//double data_phi[1000000];
//double data_kappa[1000000];
//double data_dv[1000000];
//double data_man_time[1000000];
//double data_period[1000000];
//int num_data;
//
//
//void add_to_data(double phi, double kappa, double dv, double man_time, double period) {
//	data_phi[num_data] = phi;
//	data_kappa[num_data] = kappa;
//	data_dv[num_data] = dv;
//	data_man_time[num_data] = man_time;
//	data_period[num_data] = period;
//	num_data++;
//}
//
//double testvar[2];
//
//
//void find_double_swing_by_zero_sec_sb_diff(struct Swingby_Peak_Search_Params spsp, struct DSB_Data dd) {
//	struct timeval start, end;
//	double elapsed_time;
//	struct Vector v_t00 = spsp.v_t00;
//	struct DSB *dsb = spsp.dsb;
//
//	struct Body *body = dd.body;
//
//	double min_dtheta = spsp.interval[0];
//	double max_dtheta = spsp.interval[1];
//
//	int right_side = 0;	// 0 = left, 1 = right
//
//	// x: dtheta, y: diff_vinf (data[0].x: number of data points beginning at index 1)
//	struct Vector2D data[101];
//	data[0].x = 0;
//
//	double dtheta, last_dtheta, diff_vinf = 1e9;
//
//	for(int i = 0; i < 100; i++) {
//		if(i == 0) dtheta = min_dtheta;
//
//		gettimeofday(&start, NULL);  // Record the starting time
//		struct OSV osv_m0 = propagate_orbit_theta(constr_orbit_from_osv(dd.p0.r, v_t00, SUN()), dtheta, SUN());
//
//		gettimeofday(&end, NULL);  // Record the ending time
//		elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
//		testvar[0] += elapsed_time;
//
//		double duration = calc_dt_from_dtheta(spsp.orbit, dtheta);
//
//		//printf("%f, %f, %f, %f, %f\n\n", rad2deg(dtheta), rad2deg(min_dtheta), rad2deg(max_dtheta), duration/86400, transfer_duration);
//
//		gettimeofday(&start, NULL);  // Record the starting time
//		struct Transfer transfer = calc_transfer(capfb, body, body, osv_m0.r, osv_m0.v, dd.p1.r, dd.p1.v,
//												 dd.transfer_duration * 86400 - duration, SUN(), NULL, 0, 0);
//		gettimeofday(&end, NULL);  // Record the ending time
//		elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
//		testvar[1] += elapsed_time;
//
//		struct Vector temp = subtract_vectors(transfer.v0, osv_m0.v);
//		double mag = vector_mag(temp);
//
//		struct Vector temp1 = subtract_vectors(transfer.v1, dd.p1.v);
//		struct Vector temp2 = subtract_vectors(dd.s1.v, dd.p1.v);
//
//		diff_vinf = vector_mag(temp1) - vector_mag(temp2);
//
//		if(dtheta == min_dtheta && diff_vinf < 0) right_side = 1;
//		if(dtheta == max_dtheta && diff_vinf < 0) break;
//
//		if (fabs(diff_vinf) < 10) {
//			double beta = (M_PI - angle_vec_vec(temp1, temp2)) / 2;
//			double rp = (1 / cos(beta) - 1) * (body->mu / (pow(vector_mag(temp1), 2)));
//			if (rp > body->radius + body->atmo_alt) {
//				if (mag < dsb->dv) {
//					dsb->osv[0].r = dd.p0.r;
//					dsb->osv[0].v = v_t00;
//					dsb->osv[1] = osv_m0;
//					dsb->osv[2].r = transfer.r0;
//					dsb->osv[2].v = transfer.v0;
//					dsb->osv[3].r = transfer.r1;
//					dsb->osv[3].v = transfer.v1;
//					dsb->dv = mag;
//					dsb->man_time = duration;	// gets converted to JD time instead of duration later
//				}
//			}
//			if(!right_side) right_side = 1;
//			else break;
//		}
//
//		insert_new_data_point(data, dtheta, diff_vinf);
//
//		if(!can_be_negative_monot_deriv(data)) break;
//		last_dtheta = dtheta;
//		if(i == 0) dtheta = max_dtheta;
//		else dtheta = root_finder_monot_deriv_next_x(data, right_side ? 1 : 0);
//		if(i > 3 && dtheta == last_dtheta) break;
//		if(dtheta > max_dtheta) dtheta = (max_dtheta+data[(int) data[0].x-1].x)/2;
//		if(dtheta < min_dtheta) dtheta = (min_dtheta+data[2].x)/2;
//		if(isnan(dtheta) || isinf(dtheta)) break;
//	}
//}
//
//
//struct DSB calc_man_for_dsb(struct Vector v_soi, struct DSB_Data dd) {
//	struct DSB dsb = {.man_time = -1, .dv = 1e9};
//	double mu = dd.body->orbit.body->mu;
//	double tolerance = 0.05;
//
//	// velocity vector after first swing-by
//	struct Vector v_t00 = add_vectors(v_soi, dd.p0.v);
//	// orbit after first swing-by
//	struct Orbit orbit = constr_orbit_from_osv(dd.s0.r, v_t00, dd.body->orbit.body);
//	double T = orbit.period;
//
//	double body_T = M_PI*2 * sqrt(pow(dd.body->orbit.a,3)/mu);
//	// is true, when transfer point goes backwards on body orbit
//	int negative_true_anomaly = ((dd.transfer_duration*86400)/body_T - (int)((dd.transfer_duration*86400)/body_T)) > 0.5;
//
//	double T_ratio = T / body_T - (int) (T / body_T);
//	// maybe think about more resonances in future (not only 1:1, 2:1, 3:1,...)
//	/*if (T_ratio > tolerance && T_ratio < 1 - tolerance) {
//		dsb.man_time = -1;
//		return dsb;	// skip this iteration
//	}*/
//
//	// dtheta at which conjuction/opposition occurs (Sun, satellite, planet) - set to conj/opp before first fly-by
//	double theta_conj_opp = angle_vec_vec(dd.p0.r, dd.p1.r);
//	if(negative_true_anomaly) theta_conj_opp *= -1;
//	else theta_conj_opp -= M_PI;
//
//	double interval[2];
//
//	int counter = 0;
//	// three days buffer after and before fly-bys (~SOI)
//	double max_dtheta = calc_dtheta_from_dt(orbit, dd.transfer_duration*86400-86400*3);
//	double min_dtheta = calc_dtheta_from_dt(orbit, 86400*3);
//
//	double dtheta0, dtheta1;
//
//	do {
//		dtheta0 = theta_conj_opp + M_PI*(  counter);
//		dtheta1 = theta_conj_opp + M_PI*(1+counter);
//
//		interval[0] = dtheta0;// + deg2rad(0.5);
//		if (interval[0] < min_dtheta) interval[0] = min_dtheta;
//
//		interval[1] = dtheta1;// - deg2rad(0.5);
//		if (interval[1] > max_dtheta) interval[1] = max_dtheta;
//
//		counter++;
//
//		struct Swingby_Peak_Search_Params spsp = {
//				orbit,
//				interval,
//				&dsb,
//				dd.body,
//				v_t00
//		};
//
//
//		find_double_swing_by_zero_sec_sb_diff(spsp, dd);
//	} while (dtheta1 < max_dtheta);
//
//	return dsb;
//}
//
//struct DSB calc_double_swing_by(struct OSV s0, struct OSV p0, struct OSV s1, struct OSV p1, double transfer_duration, struct Body *body) {
//	num_data = 0;
//	struct DSB dsb = {.dv = 1e9};
//
//	struct timeval start, end;
//	double elapsed_time;
//
//	struct DSB_Data dsb_data = {s0, s1, p0, p1, transfer_duration, body};
//
//	gettimeofday(&start, NULL);  // Record the ending time
//
//	testvar[0] = 0;
//	testvar[1] = 0;
//
//	int num_angle_analyse = 10;
//
//	double min_rp = body->radius+body->atmo_alt;
//	struct Vector v_soi0 = subtract_vectors(s0.v, p0.v);
//	double v_inf = vector_mag(v_soi0);
//	double min_beta = acos(1 / (1 + (min_rp * pow(v_inf, 2)) / body->mu));
//	double max_defl = M_PI - 2*min_beta;
//
////	printf("\nmin beta: %.2f°; max deflection: %.2f° (vinf: %f m/s)\n\n", rad2deg(min_beta), rad2deg(max_defl), v_inf);
//
//	double angle_step_size, phi, kappa, max_phi, max_kappa;
//	double angles[3];
//	double min_phi, min_kappa;
//	double min_phi_kappa[2];
//
//	for(int i = 0; i < 1; i++) {
//		if (i == 0) {
//			angle_step_size = 2 * max_defl / num_angle_analyse;
//			phi = -max_defl - angle_step_size;
//			max_phi = max_defl;
//			max_kappa = max_defl;
//			min_phi_kappa[0] = -max_defl;
//		} else {
//			angle_step_size = 2 * angles[0] / num_angle_analyse;
//			phi = angles[1] - angles[0] - angle_step_size;
//			max_phi = angles[1] + angles[0];
//			max_kappa = angles[2] + angles[0];
//			min_phi_kappa[0] = angles[1] - angles[0];
//		}
//
//		while (phi <= max_phi) {
//			phi += angle_step_size;
//			struct Vector rot_axis_1 = norm_vector(cross_product(v_soi0, p0.r));
//			struct Vector v_soi_ = rotate_vector_around_axis(v_soi0, rot_axis_1, phi);
//			kappa = i == 0 ? -max_defl - angle_step_size : angles[2] - angles[0] - angle_step_size;
//			min_phi_kappa[1] = i == 0 ? -max_defl : angles[2] - angles[0];
//
////			printf("%f° %f°\n", rad2deg(phi), rad2deg(max_defl));
//			while (kappa <= max_kappa) {
////				printf("%f° %f°\n", rad2deg(kappa), rad2deg(max_defl));
//				kappa += angle_step_size;
//
//				double defl = acos(cos(phi)*sin(M_PI_2 - kappa));
//				if (defl > max_defl) continue;
//
//				struct Vector rot_axis_2 = norm_vector(cross_product(v_soi_, rot_axis_1));
//				struct Vector v_soi = rotate_vector_around_axis(v_soi_, rot_axis_2, kappa);
//
//				struct DSB temp_dsb = calc_man_for_dsb(v_soi, dsb_data);
//
//				if(temp_dsb.dv < 1e8) {
//					add_to_data(rad2deg(phi), rad2deg(kappa), temp_dsb.dv, temp_dsb.man_time/86400,
//								constr_orbit_from_osv(temp_dsb.osv[1].r, temp_dsb.osv[1].v, SUN()).period/86400);
//				}
//				if(temp_dsb.man_time>0 && temp_dsb.dv < dsb.dv) {
//					dsb = temp_dsb;
//					min_phi = phi;
//					min_kappa = kappa;
//				}
//			}
//		}
////		gettimeofday(&end, NULL);  // Record the ending time
////		elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
////		printf("| Elapsed time: %.3f s |  (%f - %f - %f)\n", elapsed_time, test[0], test[1], test[0]+test[1]);
////		printf("min_dv: %f\n", dsb.dv);
//		if(dsb.dv >= 1e9) break;
//
//		double phi0, kappa0;
//		double l = 0.25 * max_defl / num_angle_analyse;
//
//		while(l > max_defl/10000) {
//			phi0 = min_phi;
//			kappa0 = min_kappa;
//
//			for(int c = 0; c < 8; c++) {
//				switch(c) {
//					case 0:
//						phi = phi0 + 3*l;
//						kappa = kappa0;
//						break;
//					case 1:
//						phi = phi0 - 3*l;
//						kappa = kappa0;
//						break;
//					case 2:
//						phi = phi0;
//						kappa = kappa0 + 3*l;
//						break;
//					case 3:
//						phi = phi0;
//						kappa = kappa0 - 3*l;
//						break;
//					case 4:
//						phi = phi0 + 2*l;
//						kappa = kappa0 + 2*l;
//						break;
//					case 5:
//						phi = phi0 - 2*l;
//						kappa = kappa0 + 2*l;
//						break;
//					case 6:
//						phi = phi0 + 2*l;
//						kappa = kappa0 - 2*l;
//						break;
//					case 7:
//						phi = phi0 - 2*l;
//						kappa = kappa0 - 2*l;
//						break;
//				}
//
//
//				struct Vector rot_axis_1 = norm_vector(cross_product(v_soi0, p0.r));
//				struct Vector v_soi_ = rotate_vector_around_axis(v_soi0, rot_axis_1, phi);
//
//				struct Vector rot_axis_2 = norm_vector(cross_product(v_soi_, rot_axis_1));
//				struct Vector v_soi = rotate_vector_around_axis(v_soi_, rot_axis_2, kappa);
//
//				struct DSB temp_dsb = calc_man_for_dsb(v_soi, dsb_data);
//
//				if(temp_dsb.man_time > 0 && temp_dsb.dv < dsb.dv) {
//					dsb = temp_dsb;
//					min_phi = phi;
//					min_kappa = kappa;
//					add_to_data(rad2deg(min_phi), rad2deg(min_kappa), dsb.dv, dsb.man_time / 86400,
//								constr_orbit_from_osv(dsb.osv[1].r, dsb.osv[1].v, SUN()).period / 86400);
//					break;
//				}
//			}
////			printf("%f %f %f %f\n", dsb.dv, rad2deg(min_phi), rad2deg(min_kappa), rad2deg(l));
//			if(min_phi == phi0 && min_kappa == kappa0) l /= 2;
//		}
//	}
////	print_double_array("phi", data_phi, num_data);
////	print_double_array("kap", data_kappa, num_data);
////	print_double_array("d_v", data_dv, num_data);
////	print_double_array("dur", data_man_time, num_data);
////	print_double_array("per", data_period, num_data);
//
//	return dsb;
//}
//
