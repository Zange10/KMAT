#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#include "launch_calculator.h"
#include "launch_sim.h"



void calc_launch_azimuth(struct Body *body) {
    double lat = 0;
    double incl = 0;

    printf("Enter parameters (latitude, inclination): ");
    scanf("%lf, %lf", &lat, &incl);

    double surf_speed = cos(deg2rad(lat)) * body->radius*2*M_PI/body->rotation_period;
    double end_speed = 7800;    // orbital speed hard coded


    double azi1 = asin((cos(deg2rad(incl)))/cos(deg2rad(lat)));
    double azi2 = asin(surf_speed/end_speed);
    double azi = rad2deg(azi1)-rad2deg(azi2);

    printf("\nNeeded launch Azimuth to target inclination %g° from %g° latitude: %g° %g°\n____________\n\n", incl, lat, azi, 180-azi);
}

// ------------------------------------------------------------

void launch_calculator() {
    struct LV lv;

    int selection;
    char title[] = "LAUNCH CALCULATOR:";
    char options[] = "Go Back; Calculate; Choose Profile; Create new Profile; Testing; Launch Profile Adjustments; Payload Launch Profile Analysis; Calculate Launch Azimuth";
    char question[] = "Program: ";
    do {
        selection = user_selection(title, options, question);

        switch(selection) {
            case 1:
                if(lv.stages != NULL) simulate_single_launch(lv); // if lv initialized
                break;
            case 2:
                read_LV_from_file(&lv);
                break;
            case 3:
                write_temp_LV_file();
                break;
            case 4:
                get_test_LV(&lv);
				simulate_single_launch(lv);
                break;
            case 5:
				get_test_LV(&lv);
				lp_param_fixed_payload_analysis4(lv, 1000, NULL); // if lv initialized
                break;
            case 6:
				get_test_LV(&lv);
                calc_highest_payload_mass(lv);
                break;
            case 7:
                calc_launch_azimuth(EARTH());
                break;
        }
    } while(selection != 0);
}

void initiate_launch_campaign(struct LV lv, int calc_params) {
//    if(calc_params != 0) {
//        if(calc_params == 1) {
//            double payload_mass = 50;
//            struct Lp_Params best_lp_params;
//            lp_param_fixed_payload_analysis(lv, payload_mass, &best_lp_params);
//            printf("\n------- a1: %f, a2: %f, b2: %g, h: %g ------- \n\n", best_lp_params.a1, best_lp_params.a2, best_lp_params.b2, best_lp_params.h);
//            calculate_launch(lv, payload_mass, best_lp_params, 0);
//        } else {
//            double payload_max = calc_highest_payload_mass(lv);
//            printf("Payload max: %g\n", payload_max);
//            lp_param_mass_analysis(lv, 285, payload_max);
//            lp_param_mass_analysis(lv, 50, 280);
//            lp_param_mass_analysis(lv, 10, 50);
//            lp_param_mass_analysis(lv, 0, 10);
//        }
//    } else {
//        //struct Lp_Params lp_params = {.a1 = 36e-6, .a2 = 12e-6, .b2 = 49};    // tester
//        struct Lp_Params lp_params = {.a1 = 30e-6, .a2 = 6e-6, .b2 = 58};      // electron
//        //struct Lp_Params lp_params = {.a1 = 0e-6, .a2 = 0e-6, .b2 = 90};      // sounding
//        if(1) lp_params.h = log(lp_params.b2/90) / (lp_params.a2-lp_params.a1);
//        else lp_params.h = 1e9;
//        double payload_mass = 10000;
//        calculate_launch(lv, payload_mass, lp_params, 0);
//    }
}