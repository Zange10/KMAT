//#include "itin_dsb_tool.h"
//#include "tools/data_tool.h"
//#include "double_swing_by.h"
//#include "transfer_tools.h"
//#include "itin_tool.h"
//#include "tools/datetime.h"
//
//#include <math.h>
//#include <stdlib.h>
//#include <string.h>
//
//
//
//
//
//struct Vector2D * calc_viable_dsb_range(struct Vector2D *viable_range, struct OSV osv0_sat, struct Body **bodies, int num_bodies, double t0, double max_dt3) {
//	viable_range[0].x = 0;
//
//	struct OSV osv0_bod = osv_from_ephem(bodies[0]->ephem, t0, SUN());
//
//	double v_inf = vector_mag(subtract_vectors(osv0_sat.v,osv0_bod.v))+1000;
//	double v_p = vector_mag(osv0_bod.v);
//	double r_p = vector_mag(osv0_bod.r);
//	double v_max = v_p+v_inf;
//	double v_min = v_p-v_inf;
//
//	double a_max = 1/(2/r_p - (v_max*v_max)/SUN()->mu);
//	double a_min = 1/(2/r_p - (v_min*v_min)/SUN()->mu);
//
//	double P_max = 2*M_PI* sqrt(a_max*a_max*a_max/SUN()->mu) / 86400;
//	double P_min = 2*M_PI* sqrt(a_min*a_min*a_min/SUN()->mu) / 86400;
//
//
//	for(double t2 = t0+P_min; t2 <= t0+P_max+max_dt3; t2+=1.0) {
//		for(double t1 = t0+P_min; t1 <= t0+P_max; t1+=1.0) {
//			if(t1 + 10 >= t2) continue;
//			double dt = (t2-t1);
//			struct OSV osv0 = osv_from_ephem(bodies[1]->ephem, t1, SUN());
//			struct OSV osv1 = osv_from_ephem(bodies[2]->ephem, t2, SUN());
//			struct Transfer tf_after_dsb = calc_transfer(circfb, bodies[1], bodies[2], osv0.r,osv0.v,
//														 osv1.r, osv1.v, dt * 86400, SUN(),
//														 NULL, 0, 0);
//
//			struct ItinStep itin_step = {
//					bodies[2],
//					tf_after_dsb.r1,
//					tf_after_dsb.v1,
//					osv1.v,
//					tf_after_dsb.v0,
//					t2,
//					0,
//					NULL,
//					NULL
//			};
//
////			find_viable_flybys(&itin_step, /*body_ephems[bodies[3]->id],*/ bodies[3], 200 * 86400, 2000 * 86400);
//
//			if(itin_step.num_next_nodes > 0) {
//				for(int i = 0; i < itin_step.num_next_nodes; i++) {
////					find_viable_flybys(itin_step.next[i], /*body_ephems[bodies[4]->id],*/ bodies[4], 800 * 86400, 2000 * 86400);
//					if(itin_step.next[i]->num_next_nodes > 0) {
//						insert_new_data_point2(viable_range, t1, t2);
//					}
//				}
//
//				for(int i = 0; i < itin_step.num_next_nodes; i++) {
//					free_itinerary(itin_step.next[i]);
//				}
//			}
//		}
//	}
//	print_data_vector("T1", "T2", viable_range);
//	return viable_range;
//}
//
//
//void test_dsb() {
//	struct Body *bodies[6];
//
//	int min_duration[6], max_duration[6];
//	int counter = 0;
//	double jd_min_dep, jd_max_dep;
//
//	struct System *system = NULL;
//	for(int i = 0; i < get_num_available_systems(); i++) {
//		if(strcmp(get_available_systems()[i]->name, "Solar System (Ephemerides)") == 0) {
//			system = get_available_systems()[i];
//		}
//	}
//
//
//	bodies[counter] = system->bodies[2];	// Earth
//	struct Date min_date = {1997, 10, 06};	// Cassini: 1997-10-06
//	struct Date max_date = {1997, 10, 10};
//	jd_min_dep = convert_date_JD(min_date);
//	jd_max_dep = convert_date_JD(max_date);
//	counter++;
//
//	bodies[counter] = system->bodies[1];	// Venus
//	min_duration[counter] = 190;	// Cassini 197
//	max_duration[counter] = 210;
//	counter++;
//
//	bodies[counter] = system->bodies[1];	// Venus
//	min_duration[counter] = 10;	// Cassini 425
//	max_duration[counter] = 1000;
//	counter++;
//
//	bodies[counter] = system->bodies[2];	// Earth
//	min_duration[counter] = 40;	// Cassini 65
//	max_duration[counter] = 90;
//	counter++;
//
//	bodies[counter] = system->bodies[4];	// Jupiter
//	min_duration[counter] = 200;	// Cassini 559
//	max_duration[counter] = 2000;
//	counter++;
//
//	bodies[counter] = system->bodies[5];	// Saturn
//	min_duration[counter] = 800;	// Cassini 1273
//	max_duration[counter] = 2000;
//
//	double test[1000];
//	int numtest = 0;
//
//	struct Dv_Filter dv_filter = {1e9, 1e9, 1e9};
//	double t[4], tf_duration[4];
//
//	double all_d_v[100000], all_dep[100000], all_fb0[100000], all_fb1[100000], all_fb2[100000], all_alt[100000], all_ddv[100000];
//	double id_v[10000], idep[10000], ifb0[10000], ifb1[10000], ifb2[10000], ialt[100000], iddv[100000];;
//	double d_v[10000], dep[10000], fb0[10000], fb1[10000], fb2[10000];
//	int num_all = 0, inum_viable = 0, num_viable = 0;
//
//	struct Vector2D *all_data = calloc(10000000, sizeof(struct Vector2D));
//	struct Vector2D *all_points = calloc(10000000, sizeof(struct Vector2D));
//	all_data[0].x = 0;
//	all_points[0].x = 0;
//
//	struct Vector2D error1ddv[10000];
//	struct Vector2D error1alt[10000];
//	struct Vector2D error2ddv[10000];
//	struct Vector2D error2alt[10000];
//	struct Vector2D viable_dv[1000];
//
//	struct Vector2D best_dvs[1000];
//	best_dvs[0].x = 0;
//
//	struct Vector2D viable_comb[10000];
//	viable_comb[0].x = 0;
//	struct Vector2D viable_comb2[1000];
//	viable_comb2[0].x = 0;
//
//
//	if(1) {
//		double dep_jd = jd_min_dep;
//		double t0 = dep_jd + min_duration[1];
//		double t1 = t0 + 427;
//		for(double t2 = t1+20; t2 < t1+700; t2 += 0.1) {
//			double dt = (t2-t1);
//			struct OSV osv0 = osv_from_ephem(bodies[2]->ephem, t1, SUN());
//			struct OSV osv1 = osv_from_ephem(bodies[3]->ephem, t2, SUN());
//			struct Transfer tf_after_dsb = calc_transfer(circfb, bodies[2], bodies[3], osv0.r,osv0.v,
//														 osv1.r, osv1.v, dt * 86400, SUN(),
//														 NULL, 0, 0);
//
//			struct ItinStep itin_step = {
//					bodies[3],
//					tf_after_dsb.r1,
//					tf_after_dsb.v1,
//					osv1.v,
//					tf_after_dsb.v0,
//					t2,
//					0,
//					NULL,
//					NULL
//			};
//
////			find_viable_flybys(&itin_step, /*body_ephems[bodies[4]->id],*/ bodies[4], 200 * 86400, 2000 * 86400);
//
//
//			if(itin_step.num_next_nodes > 0) {
//				for(int i = 0; i < itin_step.num_next_nodes; i++) {
////					find_viable_flybys(itin_step.next[i], /*body_ephems[bodies[5]->id],*/ bodies[5], 200 * 86400, 2000 * 86400);
//
////					if(itin_step.next[i]->num_next_nodes > 0) {
////						insert_new_data_point2(viable_range, t1, t2);
////					}
//				}
//
//				for(int i = 0; i < itin_step.num_next_nodes; i++) {
//					free_itinerary(itin_step.next[i]);
//				}
//			}
//		}
//
//		print_data_vector("fbe1", "error1ddv", error1ddv);
//		print_data_vector("fbe1", "error1alt", error1alt);
//		print_data_vector("fbe2", "error2ddv", error2ddv);
//		print_data_vector("fbe2", "error2alt", error2alt);
//
//		return;
//	}
//
//	if(0) {
//		struct OSV osv_dep1 = osv_from_ephem(bodies[0]->ephem, jd_min_dep, SUN());
//		struct OSV dsb_arr1 = osv_from_ephem(bodies[1]->ephem, jd_min_dep + min_duration[1], SUN());
//
//		double data[3];
//		struct Transfer tf_launch = calc_transfer(circfb, bodies[0], bodies[1], osv_dep1.r, osv_dep1.v,
//												  dsb_arr1.r,
//												  dsb_arr1.v, min_duration[1] * 86400, SUN(), data, dv_filter.dep_periapsis, dv_filter.arr_periapsis);
//
//
//		double v_soi = vector_mag(subtract_vectors(tf_launch.v1, dsb_arr1.v)) + 1000;
//		double v_p = vector_mag(dsb_arr1.v);
//		double r_p = vector_mag(dsb_arr1.r);
//		double v_max = v_p + v_soi;
//		double v_min = v_p - v_soi;
//
//		double a_max = 1 / (2 / r_p - (v_max * v_max) / SUN()->mu);
//		double a_min = 1 / (2 / r_p - (v_min * v_min) / SUN()->mu);
//
//		double P_max = 2 * M_PI * sqrt(a_max * a_max * a_max / SUN()->mu) / 86400;
//		double P_min = 2 * M_PI * sqrt(a_min * a_min * a_min / SUN()->mu) / 86400;
//		P_max = 500;
//		P_min = 350;
//
//		printf("%f   %f\n", P_min, P_max);
//
//
//		for(double t2 = jd_min_dep + min_duration[1] + P_min;
//			t2 <= jd_max_dep + max_duration[1] + P_max + max_duration[3]; t2 += 1) {
//			for(double t1 = jd_min_dep + min_duration[1] + P_min; t1 <= jd_min_dep + min_duration[1] + P_max; t1 += 1) {
//				t2 = jd_min_dep + min_duration[1]+476;
//				t1 = jd_min_dep + min_duration[1]+400;
//
//				if(t1 + 10 >= t2) continue;
//				double dt = (t2 - t1);
//				struct OSV osv0 = osv_from_ephem(bodies[2]->ephem, t1, SUN());
//				struct OSV osv1 = osv_from_ephem(bodies[3]->ephem, t2, SUN());
//				struct Transfer tf_after_dsb = calc_transfer(circfb, bodies[2], bodies[3], osv0.r, osv0.v,
//															 osv1.r, osv1.v, dt * 86400, SUN(),
//															 NULL, 0, 0);
//
//				struct ItinStep itin_step = {
//						bodies[3],
//						tf_after_dsb.r1,
//						tf_after_dsb.v1,
//						osv1.v,
//						tf_after_dsb.v0,
//						t2,
//						0,
//						NULL,
//						NULL
//				};
//
////				find_viable_flybys(&itin_step, /*body_ephems[bodies[4]->id],*/ bodies[4], min_duration[4] * 86400,
////								   max_duration[4] * 86400);
//
//
////				insert_new_data_point2(all_points, t1 - jd_min_dep - min_duration[1],
////									   t2 - jd_min_dep - min_duration[1]);
////				insert_new_data_point2(all_data, get_best_alt(), get_best_diff_vinf());
//
////				print_vector(scalar_multiply(itin_step.v_body,1e-3));
////				print_vector(scalar_multiply(itin_step.v_arr,1e-3));
////				print_vector(scalar_multiply(itin_step.v_dep,1e-3));
////				print_vector(scalar_multiply(itin_step.r,1e-9));
////				printf("%d  %f  %f  %f  %f  %f  %s\n", itin_step.num_next_nodes > 0, jd_min_dep + min_duration[1], t1 - jd_min_dep - min_duration[1], t2 - jd_min_dep - min_duration[1], get_best_diff_vinf(), get_best_alt(), itin_step.body->name);
//
//				if(itin_step.num_next_nodes > 0) {
////				printf("YES %f %f\n", t1-jd_min_dep-min_duration[1], t2-jd_min_dep-min_duration[1]);
//					insert_new_data_point2(viable_comb, t1 - jd_min_dep - min_duration[1],
//										   t2 - jd_min_dep - min_duration[1]);
//
//					for(int i = 0; i < itin_step.num_next_nodes; i++) {
////						find_viable_flybys(itin_step.next[i], /*body_ephems[bodies[5]->id],*/ bodies[5],
////										   min_duration[5] * 86400,
////										   max_duration[5] * 86400);
//						if(itin_step.next[i]->num_next_nodes > 0) {
//							insert_new_data_point2(viable_comb2, t1 - jd_min_dep - min_duration[1],
//												   t2 - jd_min_dep - min_duration[1]);
//							printf("YES %f %f   %f  %f\n", t1 - jd_min_dep - min_duration[1],
//								   t2 - jd_min_dep - min_duration[1], viable_comb2[0].x,
//								   vector_mag(subtract_vectors(itin_step.v_arr, itin_step.v_body)));
//						}
//					}
//
//					for(int i = 0; i < itin_step.num_next_nodes; i++) {
//						free_itinerary(itin_step.next[i]);
//					}
//				}
////			else printf("NOPE  %f %f\n", t1-jd_min_dep-min_duration[1], t2-jd_min_dep-min_duration[1]);
//				return;
//			}
//		}
//		print_data_vector("T1", "T2", viable_comb);
//		print_data_vector("ST1", "ST2", viable_comb2);
////	print_data_vector("T1", "T2", all_points);
////	print_data_vector("best_alt", "best_vinf", all_data);
//
//		free(all_data);
//		free(all_points);
//		return;
//	}
////
////	for(tf_duration[2] = min_duration[3]; tf_duration[2] <= max_duration[3]; tf_duration[2] += 40.0) {
//
//	struct OSV osv_dep_base = osv_from_ephem(bodies[0]->ephem, jd_min_dep, SUN());
//	struct OSV dsb_osv_base[3] = {
//			osv_from_ephem(bodies[1]->ephem, jd_min_dep+min_duration[1], SUN())
//	};
//
//	struct Transfer tf_launch_base = calc_transfer(circfb, bodies[0], bodies[1], osv_dep_base.r, osv_dep_base.v,
//												   dsb_osv_base[0].r,dsb_osv_base[0].v, min_duration[1] * 86400, SUN(), NULL, 0, 0);
//
//	struct OSV osv0_sat_base = {tf_launch_base.r1, tf_launch_base.v1};
//
//	struct Vector2D viable_range[3000];
//	calc_viable_dsb_range(viable_range, osv0_sat_base, &(bodies[1]), 5, jd_min_dep+min_duration[1], max_duration[3]);
//
//
//	for(double jd_dep = jd_min_dep; jd_dep <= jd_max_dep; jd_dep+=1.0) {
//		for(tf_duration[0] = min_duration[1]; tf_duration[0] <= max_duration[1]; tf_duration[0] += 1) {
//
//			t[0] = jd_dep + tf_duration[0];
//
//			struct OSV osv_dep = osv_from_ephem(bodies[0]->ephem, jd_dep, SUN());
//			struct OSV dsb_osv[3] = {
//					osv_from_ephem(bodies[1]->ephem, t[0], SUN())
//			};
//
//			struct Transfer tf_launch = calc_transfer(circfb, bodies[0], bodies[1], osv_dep.r, osv_dep.v,
//													  dsb_osv[0].r,dsb_osv[0].v, tf_duration[0] * 86400, SUN(), NULL, 0, 0);
//
//			printf("Viable range: %f\n", viable_range[0].x);
//
//			for(int i = 1; i <= viable_range[0].x; i++) {
//				t[1] = viable_range[i].x;
//				t[2] = viable_range[i].y;
//				tf_duration[1] = t[1] - t[0];
//				tf_duration[2] = t[2] - t[1];
//				print_date(convert_JD_date(jd_dep, DATE_ISO), 0);
//				printf("  t0: %f; t1: %f; t2: %f; %d / %f\n", t[0]-jd_dep, t[1]-t[0], t[2]-t[1], i, viable_range[0].x);
//
//				dsb_osv[1] = osv_from_ephem(bodies[2]->ephem, t[1], SUN());
//				dsb_osv[2] = osv_from_ephem(bodies[3]->ephem, t[2], SUN());
//
//				double data[3];
//
//				struct Transfer tf_after_dsb = calc_transfer(circfb, bodies[2], bodies[3], dsb_osv[1].r,dsb_osv[1].v,
//															 dsb_osv[2].r, dsb_osv[2].v, tf_duration[2] * 86400, SUN(),
//															 data, dv_filter.dep_periapsis, dv_filter.arr_periapsis);
//
//				struct OSV s0 = {tf_launch.r1, tf_launch.v1};
//				struct OSV p0 = {dsb_osv[0].r, dsb_osv[0].v};
//				struct OSV s1 = {tf_after_dsb.r0, tf_after_dsb.v0};
//				struct OSV p1 = {dsb_osv[1].r, dsb_osv[1].v};
//
//				struct DSB dsb = calc_double_swing_by(s0, p0, s1, p1, tf_duration[1], bodies[1]);
//
////				printf("%f\n", dsb.dv);
//				if(dsb.dv < 3000) {
//					all_d_v[num_all] = dsb.dv;
//					all_dep[num_all] = jd_dep;
//					all_fb0[num_all] = tf_duration[0];
//					all_fb1[num_all] = tf_duration[1];
//					all_fb2[num_all] = tf_duration[2];
//					num_all++;
//				} else continue;
//
//				struct ItinStep itin_step = {
//						bodies[3],
//						tf_after_dsb.r1,
//						tf_after_dsb.v1,
//						dsb_osv[2].v,
//						tf_after_dsb.v0,
//						t[2],
//						0,
//						NULL,
//						NULL
//				};
//
////				find_viable_flybys(&itin_step, /*body_ephems[bodies[4]->id],*/ bodies[4], min_duration[4] * 86400,
////								   max_duration[4] * 86400);
//
//
////				insert_new_data_point2(error1alt, tf_duration[1], get_best_alt());
////				insert_new_data_point2(error1ddv, tf_duration[1], get_best_diff_vinf());
//
////				all_alt[num_all - 1] = get_best_alt();
////				all_ddv[num_all - 1] = get_best_diff_vinf();
//
//				if(itin_step.num_next_nodes > 0) {
//					id_v[inum_viable] = dsb.dv;
//					idep[inum_viable] = jd_dep;
//					ifb0[inum_viable] = tf_duration[0];
//					ifb1[inum_viable] = tf_duration[1];
//					ifb2[inum_viable] = tf_duration[2];
//					ifb2[inum_viable] = tf_duration[2];
////					ialt[inum_viable] = get_best_alt();
////					iddv[inum_viable] = get_best_diff_vinf();
//					inum_viable++;
//				}
//				printf("num next nodes: %d\n", itin_step.num_next_nodes);
//
//				for(int j = 0; j < itin_step.num_next_nodes; j++) {
////					find_viable_flybys(itin_step.next[j], /*body_ephems[bodies[5]->id],*/ bodies[5],
////									   min_duration[5] * 86400,
////									   max_duration[5] * 86400);
////					insert_new_data_point2(error2alt, tf_duration[1], get_best_alt());
////					insert_new_data_point2(error2ddv, tf_duration[1], get_best_diff_vinf());
//					if(itin_step.next[j]->num_next_nodes > 0) {
//						insert_new_data_point2(viable_dv, tf_duration[1], dsb.dv);
////								printf("SAT num next nodes: %d\n", itin_step.next[j]->num_next_nodes);
////								printf("%f\n", dsb.dv);
////								print_double_array("data", data, 3);
//
//						d_v[num_viable] = dsb.dv;
//						dep[num_viable] = jd_dep;
//						fb0[num_viable] = tf_duration[0];
//						fb1[num_viable] = tf_duration[1];
//						fb2[num_viable] = tf_duration[2];
//						num_viable++;
//					}
//				}
//
//				for(int j = 0; j < itin_step.num_next_nodes; j++) {
//					free_itinerary(itin_step.next[j]);
//				}
//			}
//
//
//
//
////			for(double ttt2 = 480; ttt2 <= 490; ttt2 += 1) {
////				double venus_period = 224.7;
////				double step = venus_period/10;
////				printf("%f\n", ttt2);
////				error1alt[0].x = 0;
////				error1ddv[0].x = 0;
////				error2alt[0].x = 0;
////				error2ddv[0].x = 0;
////				viable_dv[0].x = 0;
////				for(int c = 0; c < 5; c++) {
////					if(c == 0) {
////						for(int j = 0; j < 9; j++) {
////							test[j] = venus_period*2 + (j-4)*step;
////						}
////						numtest = 9;
////					} else if(c == 1) {
////						step /= 4;
////						numtest = 0;
////						for(int i = 0; i < error1ddv[0].x; i++) {
////							if(error1ddv[i+1].y < 1) {
////								for(int j = -2; j <= 2; j++) {
////									if(j == 0) continue;
////									test[numtest] = error1ddv[i + 1].x + j * step;
////									numtest++;
////								}
////							}
////						}
////					} else if(c == 2) {
////						step /= 8;
////						numtest = 0;
////						for(int i = 0; i < error1ddv[0].x; i++) {
////							if(error1alt[i+1].y > bodies[3]->radius + bodies[3]->atmo_alt) {
////								for(int j = -4; j <= 4; j++) {
////									if(j == 0) continue;
////									test[numtest] = error1ddv[i + 1].x + j * step;
////									numtest++;
////								}
////							}
////						}
////					} else if(c == 3) {
////						step /= 4;
////						numtest = 0;
////						for(int i = 0; i < error2ddv[0].x; i++) {
////							if(error2ddv[i+1].y < 1) {
////								for(int j = -2; j <= 2; j++) {
////									if(j == 0) continue;
////									test[numtest] = error2ddv[i + 1].x + j * step;
////									numtest++;
////								}
////							}
////						}
////					} else if(c == 4) {
////						step /= 16;
////						numtest = 0;
////						int minidx = get_min_value_index_from_data(viable_dv);
////						for(int j = -16; j <= 16; j++) {
////							if(j == 0) continue;
////							test[numtest] = viable_dv[minidx].x + j * step;
////							numtest++;
////						}
////					}
////
////		//			for(tf_duration[1] = min_duration[2]; tf_duration[1] <= max_duration[2]; tf_duration[1]+=1) {
////					for(int testIdx = 0; testIdx < numtest; testIdx++) {
////
////						tf_duration[1] = test[testIdx];
////
////						t[1] = t[0] + tf_duration[1];
////						//t[2] = t[1] + tf_duration[2];
////						t[2] = t[0] + ttt2;
////						tf_duration[2] = t[2] - t[1];
////
////						if(t[2] < t[1]) continue;
////
////						dsb_osv[1] = osv_from_ephem(body_ephems[bodies[2]->id], t[1], SUN());
////						dsb_osv[2] = osv_from_ephem(body_ephems[bodies[3]->id], t[2], SUN());
////
////						struct Transfer tf_after_dsb = calc_transfer(circfb, bodies[2], bodies[3], dsb_osv[1].r,dsb_osv[1].v,
////																	 dsb_osv[2].r, dsb_osv[2].v, tf_duration[2] * 86400,
////																	 NULL);
////
////						struct OSV s0 = {tf_launch.r1, tf_launch.v1};
////						struct OSV p0 = {dsb_osv[0].r, dsb_osv[0].v};
////						struct OSV s1 = {tf_after_dsb.r0, tf_after_dsb.v0};
////						struct OSV p1 = {dsb_osv[1].r, dsb_osv[1].v};
////
////						struct DSB dsb = calc_double_swing_by(s0, p0, s1, p1, tf_duration[1], bodies[1]);
////
////						if(dsb.dv < 3000) {
////							all_d_v[num_all] = dsb.dv;
////							all_dep[num_all] = jd_dep;
////							all_fb0[num_all] = tf_duration[0];
////							all_fb1[num_all] = tf_duration[1];
////							all_fb2[num_all] = tf_duration[2];
////							num_all++;
////						} else continue;
////
////						struct ItinStep itin_step = {
////								bodies[3],
////								tf_after_dsb.r1,
////								tf_after_dsb.v1,
////								dsb_osv[2].v,
////								tf_after_dsb.v0,
////								t[2],
////								0,
////								NULL,
////								NULL
////						};
////
////						find_viable_flybys(&itin_step, body_ephems[bodies[4]->id], bodies[4], min_duration[4] * 86400,
////										   max_duration[4] * 86400);
////
////
////						insert_new_data_point2(error1alt, tf_duration[1], get_best_alt());
////						insert_new_data_point2(error1ddv, tf_duration[1], get_best_diff_vinf());
////
////						all_alt[num_all - 1] = get_best_alt();
////						all_ddv[num_all - 1] = get_best_diff_vinf();
////
////						if(itin_step.num_next_nodes > 0) {
////							id_v[inum_viable] = dsb.dv;
////							idep[inum_viable] = jd_dep;
////							ifb0[inum_viable] = tf_duration[0];
////							ifb1[inum_viable] = tf_duration[1];
////							ifb2[inum_viable] = tf_duration[2];
////							ifb2[inum_viable] = tf_duration[2];
////							ialt[inum_viable] = get_best_alt();
////							iddv[inum_viable] = get_best_diff_vinf();
////							inum_viable++;
////						}
////	//					printf("num next nodes: %d\n", itin_step.num_next_nodes);
////
////						for(int i = 0; i < itin_step.num_next_nodes; i++) {
////							find_viable_flybys(itin_step.next[i], body_ephems[bodies[5]->id], bodies[5],
////											   min_duration[5] * 86400,
////											   max_duration[5] * 86400);
////							insert_new_data_point2(error2alt, tf_duration[1], get_best_alt());
////							insert_new_data_point2(error2ddv, tf_duration[1], get_best_diff_vinf());
////							if(itin_step.next[i]->num_next_nodes > 0) {
////								insert_new_data_point2(viable_dv, tf_duration[1], dsb.dv);
//////								printf("SAT num next nodes: %d\n", itin_step.next[i]->num_next_nodes);
//////								printf("%f\n", dsb.dv);
//////								print_double_array("data", data, 3);
////
////								d_v[num_viable] = dsb.dv;
////								dep[num_viable] = jd_dep;
////								fb0[num_viable] = tf_duration[0];
////								fb1[num_viable] = tf_duration[1];
////								fb2[num_viable] = tf_duration[2];
////								num_viable++;
////							}
////						}
////
////						for(int i = 0; i < itin_step.num_next_nodes; i++) {
////							free_itinerary(itin_step.next[i]);
////						}
////					}
////				}
////				if(viable_dv[0].x > 0) insert_new_data_point2(best_dvs, t[2], viable_dv[get_min_value_index_from_data(viable_dv)].y);
////			}
////			print_data_vector("t2", "best_dvs", best_dvs);
//		}
//	}
//
//	print_double_array("all_d_v", all_d_v, num_all);
//	print_double_array("all_dep", all_dep, num_all);
//	print_double_array("all_fb0", all_fb0, num_all);
//	print_double_array("all_fb1", all_fb1, num_all);
//	print_double_array("all_fb2", all_fb2, num_all);
//	print_double_array("all_alt", all_alt, num_all);
//	print_double_array("all_ddv", all_ddv, num_all);
//	printf("\n");
//	print_double_array("id_v", id_v, inum_viable);
//	print_double_array("idep", idep, inum_viable);
//	print_double_array("ifb0", ifb0, inum_viable);
//	print_double_array("ifb1", ifb1, inum_viable);
//	print_double_array("ifb2", ifb2, inum_viable);
//	print_double_array("ialt", ialt, inum_viable);
//	print_double_array("iddv", iddv, inum_viable);
//	printf("\n");
//	print_double_array("d_v", d_v, num_viable);
//	print_double_array("dep", dep, num_viable);
//	print_double_array("fb0", fb0, num_viable);
//	print_double_array("fb1", fb1, num_viable);
//	print_double_array("fb2", fb2, num_viable);
//	printf("\n");
//
//	print_data_vector("fbe1", "errorddv1", error1ddv);
//	print_data_vector("fbe1", "erroralt1", error1alt);
//	print_data_vector("fbe2", "errorddv2", error2ddv);
//	print_data_vector("fbe2", "erroralt2", error2alt);
//
//	int minidx = 0;
//	double min = d_v[0];
//	for(int i = 1; i < num_viable; i++) {
//		if(d_v[i] < min) {
//			minidx = i;
//			min = d_v[i];
//		}
//	}
//
//	print_date(convert_JD_date(dep[minidx], DATE_ISO), 0); printf("  |  ");
//	print_date(convert_JD_date(dep[minidx]+fb0[minidx], DATE_ISO), 0); printf("  |  ");
//	print_date(convert_JD_date(dep[minidx]+fb0[minidx]+fb1[minidx], DATE_ISO), 0); printf("  |  ");
//	print_date(convert_JD_date(dep[minidx]+fb0[minidx]+fb1[minidx]+fb2[minidx], DATE_ISO), 0); printf("\n");
//
//	printf("%f  |  %f  |  %f\n", fb0[minidx], fb1[minidx], fb2[minidx]);
//	printf("%f\n", min);
//}
