#include "celestial_bodies.h"
#include "launch_calculator/launch_calculator.h"
#include "orbit_calculator/orbit_calculator.h"
#include "tools/tool_funcs.h"
#include "gui/gui_manager.h"
#include "database/database.h"
#include "orbit_calculator/transfer_tools.h"
#include "tools/data_tool.h"
#include <math.h>


// ------------------------------------------------------------

void test();

int main() {
    init_celestial_bodies();
	init_available_systems("./Celestial_Systems/");
	test();
//	init_db();
//	start_gui("../GUI/GUI.glade");

//    int selection;
//    char title[] = "CHOOSE PROGRAM:";
//    char options[] = "Exit; Launch Calculator; Orbit Calculator; GUI; Test";
//    char question[] = "Program: ";
//
//    do {
//        selection = user_selection(title, options, question);
//
//        switch (selection) {
//        case 1:
//            launch_calculator();
//            break;
//        case 2:
//            orbit_calculator();
//            break;
//		case 3:
//			start_gui();
//			break;
//		case 4:
//			test();
//			break;
//        default:
//            break;
//        }
//    } while(selection != 0);
	close_db();
    return 0;
}





/*
 * grav parameter of attractor does not have effect on transfer duration / hohmann duration
 * At this point in time dep body mu and r were ignored because relationships to complex
 */






void test() {
	struct TransferData {
		double dt;
		double dv;
		double r0;
		double r1;
		double r_ratio;
		double T0;
		double attr_mu;
		double dep_body_mu;
		double dep_body_R;
	};

	int num_data_points = 0;
	int max_num_data_points = 100;
	struct TransferData *transfer_data = (struct TransferData*) calloc(max_num_data_points, sizeof(struct TransferData));

	struct Body dep_body, arr_body, attractor;
	dep_body = *JUPITER();
	arr_body = *EARTH();
	attractor = *SUN();

//	dep_body.mu = 0;
//	attractor.mu /= 1e9;

	dep_body.atmo_alt = 0;
	arr_body.atmo_alt = 0;

	struct OSV osv2;

	double data[3];

	// Vary r0
	int num_r0_steps = 20;
	double min_r0 = 10e9;
	double max_r0 = 10000e9;
	double step_r0 = (max_r0-min_r0)/(num_r0_steps-1);
	double r0 = min_r0-step_r0;
	while(r0 <= max_r0 + 1e-3 - step_r0) {
		r0 += step_r0;
		dep_body.orbit = constr_orbit(r0, 0, 0, 0, 0, 0, &attractor);
//					dep_body.orbit = constr_orbit(EARTH()->orbit.a, 0, 0, 0, 0, 0, &attractor);
		struct OSV osv1 = propagate_orbit_theta(dep_body.orbit, 0.0, &attractor);

		// Vary r1
		double min_r1 = 1e9;
		double max_r1 = 10000e9;
		double step_r1 = min_r1;
		double r1 = min_r1 - step_r1;
		while(r1 <= max_r1 + 1e-3 - step_r1) {
			r1 += step_r1;
			step_r1 *= 1.2;
			arr_body.orbit = constr_orbit(r1, 0, 0, 0, 0, 0, &attractor);
//						if(arr_body.orbit.a/dep_body.orbit.a > 0.8 && arr_body.orbit.a/dep_body.orbit.a < 1.2) continue;

			// Vary dt
			int num_dt_steps = 200;
			double min_dt = (dep_body.orbit.period > arr_body.orbit.period ? dep_body.orbit.period : arr_body.orbit.period)/1000 / 86400;
			double max_dt = (dep_body.orbit.period > arr_body.orbit.period ? dep_body.orbit.period : arr_body.orbit.period) / 86400 - min_dt;
			double step_dt = (max_dt - min_dt) / (num_dt_steps - 1);
			double dt = min_dt - step_dt;
			while(dt <= max_dt + 1e-3 - step_dt) {
				dt += step_dt;

				double dtheta = 180.0;
				double step = 20.0;
				double last_dv = 1e12;
				double best_dv = 1e12;

				while(fabs(step) > 0.01) {
					osv2 = propagate_orbit_theta(arr_body.orbit, deg2rad(dtheta), &attractor);
					calc_transfer(circfb, &dep_body, &arr_body, osv1.r, osv1.v, osv2.r, osv2.v, dt * 86400, &attractor, data);
					if(data[1] > last_dv) step /= -4.0;
					dtheta += step;
					last_dv = data[1];
					if(data[1] < best_dv) best_dv = data[1];
				}


				if(num_data_points == max_num_data_points) {
					max_num_data_points *= 2;
					struct TransferData *temp = (struct TransferData *) realloc(transfer_data, max_num_data_points * sizeof(struct TransferData));
					if(temp == NULL) {
						perror("Error while reallocating data!");
						free(transfer_data);  // free only if realloc failed
						return;
					}
					transfer_data = temp;
				}

				transfer_data[num_data_points] = (struct TransferData) {
						.dt = dt,
						.dv = best_dv,
						.r0 = dep_body.orbit.a,
						.r1 = arr_body.orbit.a,
						.r_ratio = arr_body.orbit.a / dep_body.orbit.a,
						.T0 = dep_body.orbit.period / 86400,
						.attr_mu = attractor.mu,
						.dep_body_mu = dep_body.mu,
						.dep_body_R = dep_body.radius
				};
				num_data_points++;

//							printf("%8.2f    %8.2f    %8.2f    %8.4f    %8.2f      %.3e    %10.2f\n",
//								   transfer_data[num_data_points - 1].dt,
//								   transfer_data[num_data_points - 1].r0 / 1e9,
//								   transfer_data[num_data_points - 1].r1 / 1e9,
//								   transfer_data[num_data_points - 1].r_ratio,
//								   transfer_data[num_data_points - 1].T0,
//								   transfer_data[num_data_points - 1].attr_mu,
//								   transfer_data[num_data_points - 1].dv
//							);
			}
		}
		printf("%f\n", r0*1e-9);
	}



	char pcsv;

	int transfer_data_size = sizeof(struct TransferData)/sizeof(double);
	double *all_transfer_data = calloc(num_data_points*transfer_data_size+1, sizeof(double));
	all_transfer_data[0] = num_data_points*transfer_data_size;
	for(int i = 0; i < num_data_points; i++) {
		int j = 1;
		all_transfer_data[transfer_data_size*i+j] = transfer_data[i].dt; j++;
		all_transfer_data[transfer_data_size*i+j] = transfer_data[i].dv; j++;
		all_transfer_data[transfer_data_size*i+j] = transfer_data[i].r0; j++;
		all_transfer_data[transfer_data_size*i+j] = transfer_data[i].r1; j++;
		all_transfer_data[transfer_data_size*i+j] = transfer_data[i].r_ratio; j++;
		all_transfer_data[transfer_data_size*i+j] = transfer_data[i].T0; j++;
		all_transfer_data[transfer_data_size*i+j] = transfer_data[i].attr_mu; j++;
		all_transfer_data[transfer_data_size*i+j] = transfer_data[i].dep_body_mu; j++;
		all_transfer_data[transfer_data_size*i+j] = transfer_data[i].dep_body_R; j++;
	}

	char fields[] = "dt,dv,r0,r1,r_ratio,T0,attr_mu,dep_body_mu,dep_body_R";
	write_csv(fields, all_transfer_data);
	free(transfer_data);
}