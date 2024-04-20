#include <stdio.h>
#include <math.h>

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
//				lp_param_fixed_payload_analysis4(lv, 1000, NULL, 0); // if lv initialized
                break;
            case 6:
				get_test_LV(&lv);
				calc_payload_curve(lv);
                break;
            case 7:
                calc_launch_azimuth(EARTH());
                break;
        }
    } while(selection != 0);
}
