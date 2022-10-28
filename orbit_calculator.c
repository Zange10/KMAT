#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include "orbit_calculator.h"
#include "celestial_bodies.h"
#include "tool_funcs.h"
#include "orbit.h"

struct ManeuverPlan {
    double dV1;
    double dV2;
    bool first_raise_Apo;
};

void orbit_calculator() {
    char title[] = "CHOOSE CALCULATION:";
    char options[] = "Go Back; Choose Celestial Body; Get orbit info; Calculate Delta-V Requirements; Calculate in-Orbit parameters";
    char question[] = "Calculation: ";
    int selection = 0;

    struct Body *parent_body = EARTH();

    do {
        print_separator(49);
        printf("Current Parent Body: %s\n", parent_body->name);
        selection = user_selection(title, options, question);
        switch (selection)
        {
        case 1:
            parent_body = choose_celestial_body(parent_body);
            break;
        case 2:
            calc_orbital_parameters(parent_body);
            break;
        case 3:
            dv_req_calculator(parent_body);
            break;
        case 4:
            in_orbit_calculator(parent_body);
            break;
        default:
            break;
        }
    } while(selection != 0);
}

void dv_req_calculator(struct Body *parent_body) {
    char title[] = "CHOOSE CALCULATION:";
    char options[] = "Go Back; Change Apsis from circular orbit; Change Apsis; Hohmann Transfer; Inclination Change";
    char question[] = "Calculation: ";
    int selection = 0;

    do {
        selection = user_selection(title, options, question);
        switch (selection)
        {
        case 1:
            change_apsis_circ(parent_body);
            break;
        case 2:
            change_apsis(parent_body);
            break;
        case 3:
            calc_hohmann_transfer(parent_body);
            break;
        case 4:
            calc_inclination_change();
            break;
        default:
            break;
        }
    } while(selection != 0);
}

void in_orbit_calculator(struct Body *parent_body) {
    char title[] = "CHOOSE CALCULATION:";
    char options[] = "Go Back; First Cosmic Speed; Second Cosmic Speed; Speed in circular orbit";
    char question[] = "Calculation: ";
    int selection = 0;

    do {
        selection = user_selection(title, options, question);
        switch (selection)
        {
        case 1:
            calc_first_cosmic_speed(parent_body);
            break;
        case 2:
            calc_second_cosmic_speed(parent_body);
            break;
        case 3:
            calc_v_at_circ(parent_body);
            break;
        default:
            break;
        }
    } while(selection != 0);
}

struct Body * choose_celestial_body(struct Body *parent_body) {
    char input[20];

    printf("Choose celestial body: ");
    scanf("%s", input);
    
    struct Body **all_bodies = all_celest_bodies();
    for(int i = 0; i < sizeof(all_bodies); i++) {
        if(all_bodies[i] && !strcicmp(input, all_bodies[i]->name)) {
                return all_bodies[i];
        }
    }
    printf("\nNo such body\n\n");

    return parent_body;
}

void calc_orbital_parameters(struct Body *body) {
    double apsis1 = 0;
    double apsis2 = 0;
    double inclination = 0;

    printf("Enter apsis1, apsis2 and inclination: ");
    scanf("%lf %lf %lf", &apsis1, &apsis2, &inclination);

    apsis1 = apsis1*1000+body->radius;
    apsis2 = apsis2*1000+body->radius;

    struct Orbit orbit = constr_orbit_w_apsides(apsis1, apsis2, inclination,body);
    print_orbit_info(orbit);

    return;
}

void change_apsis_circ(struct Body *body) {
    double initial_apsis = 0;
    double new_apsis = 0;

    printf("Enter parameters (altitude of initial circular orbit, new value of apsis): ");
    scanf("%lf %lf", &initial_apsis, &new_apsis);

    initial_apsis = initial_apsis*1000+body->radius;
    new_apsis = new_apsis*1000+body->radius;

    double dV = calc_maneuver_dV(initial_apsis, initial_apsis, new_apsis,body);

    printf("\n____________\n\nOrbit 1: ");  print_orbit_apsides(constr_orbit_w_apsides(initial_apsis,initial_apsis,0,body));
    printf("   -->   Orbit 2: ");           print_orbit_apsides(constr_orbit_w_apsides(new_apsis,initial_apsis,0,body));
    printf("\nNeeded dV: %g m/s\n____________\n\n", dV);

    return;
}

void change_apsis(struct Body *body) {
    double initial_apsis = 0;
    double new_apsis = 0;
    double static_apsis = 0;

    printf("Enter parameters (static Apsis, initial value of apsis, new value of apsis): ");
    scanf("%lf %lf %lf", &static_apsis, &initial_apsis, &new_apsis);

    static_apsis = static_apsis*1000+body->radius;
    initial_apsis = initial_apsis*1000+body->radius;
    new_apsis = new_apsis*1000+body->radius;

    double dV = calc_maneuver_dV(static_apsis, initial_apsis, new_apsis,body);

    printf("\n____________\n\nOrbit 1: ");  print_orbit_apsides(constr_orbit_w_apsides(initial_apsis,static_apsis,0,body)); 
    printf("   -->   Orbit 2: ");           print_orbit_apsides(constr_orbit_w_apsides(new_apsis,static_apsis,0,body));
    printf("\nNeeded dV: %g m/s\n____________\n\n", dV);

    return;
}

void calc_hohmann_transfer(struct Body *body) {
    double initial_apsis = 0;
    double new_apsis = 0;

    printf("Enter parameters (altitude of initial circular orbit, altitude of planned circular orbit): ");
    scanf("%lf %lf", &initial_apsis, &new_apsis);
    
    initial_apsis = initial_apsis*1000+body->radius;
    new_apsis = new_apsis*1000+body->radius;

    struct Orbit initial_orbit;
    struct Orbit new_orbit;

    initial_orbit = constr_orbit_w_apsides(initial_apsis, initial_apsis, 0, body);
    new_orbit = constr_orbit_w_apsides(new_apsis, new_apsis, 0, body);

    struct ManeuverPlan mp = calc_change_orbit_dV(initial_orbit, new_orbit, body);

    printf("\n____________\n\nOrbit 1: ");  print_orbit_apsides(constr_orbit_w_apsides(initial_apsis,initial_apsis, 0, body));
    printf("   -->   Orbit 2: ");           print_orbit_apsides(constr_orbit_w_apsides(new_apsis,new_apsis,0, body));
    if(mp.first_raise_Apo) {
        printf("\n____________\n\nNeeded Delta-V to raise Apoapsis: \t%g m/s\n", mp.dV1);
        printf("Needed Delta-V to raise Periapsis: \t%g m/s\n", mp.dV2);
        printf("Total Delta-V for Hohmann transfer: \t%g m/s\n____________\n\n", mp.dV1+mp.dV2);
    } else {
        printf("\n____________\n\nNeeded Delta-V to lower Periapsis: \t%g m/s\n", mp.dV1);
        printf("Needed Delta-V to lower Apoapsis: \t%g m/s\n", mp.dV2);
        printf("Total Delta-V for Hohmann transfer: \t%g m/s\n____________\n\n", mp.dV1+mp.dV2);
    }

    return;
}

void calc_inclination_change() {
    double speed = 0;
    double delta_i = 0;

    printf("Enter parameters (orbital speed, amt of change of inclination): ");
    scanf("%lf %lf", &speed, &delta_i);

    double delta_v = 2*sin(deg2rad(delta_i)/2) * speed;

    printf("\n____________\n\nNeeded Delta-V to change %g° of inclination: \t%g m/s\n\n", delta_i, delta_v);

    return;
}


struct ManeuverPlan calc_change_orbit_dV(struct Orbit initial_orbit, struct Orbit planned_orbit, struct Body *body) {
    struct ManeuverPlan mp;
    if(planned_orbit.apoapsis > initial_orbit.apoapsis) {
        mp.first_raise_Apo = true;
        mp.dV1 = calc_maneuver_dV(initial_orbit.periapsis, initial_orbit.apoapsis, planned_orbit.apoapsis, body);
        mp.dV2 = calc_maneuver_dV(planned_orbit.apoapsis, initial_orbit.periapsis, planned_orbit.periapsis, body);
    } else {
        mp.first_raise_Apo = false;
        mp.dV1 = calc_maneuver_dV(initial_orbit.apoapsis, initial_orbit.periapsis, planned_orbit.periapsis, body);
        mp.dV2 = calc_maneuver_dV(planned_orbit.periapsis, initial_orbit.apoapsis, planned_orbit.apoapsis, body);
    }

    return mp;
}

double calc_maneuver_dV(double static_apsis, double initial_apsis, double new_apsis, struct Body *body) {
    struct Orbit initial_orbit;
    struct Orbit new_orbit;

    initial_orbit = constr_orbit_w_apsides(static_apsis, initial_apsis, 0, body);
    new_orbit = constr_orbit_w_apsides(static_apsis, new_apsis, 0, body);

    double v0 = calc_orbital_speed(initial_orbit, static_apsis);
    double v1 = calc_orbital_speed(new_orbit, static_apsis);

    return fabs(v1-v0);
}

void calc_first_cosmic_speed(struct Body *body) {
    double r = body->radius;
    double v = calc_orbital_speed(constr_orbit_w_apsides(r,r,0,body), r);
    printf("\n____________\n\nThe first cosmic speed of %s is: \t%g m/s\n\n", body->name, v);
}

void calc_second_cosmic_speed(struct Body *body) {
    double v = sqrt((2*body->mu)/body->radius);
    printf("\n____________\n\nThe second cosmic speed of %s is: \t%g m/s\n\n", body->name, v);
}

void calc_v_at_circ(struct Body *body) {
    double r = 0;
    double alt = 0;
    double v = 0;

    printf("Enter altitude of orbit: ");
    scanf("%lf", &alt);

    r = alt*1000+body->radius;
    v = calc_orbital_speed(constr_orbit_w_apsides(r,r,0,body), r);
    
    printf("\n____________\n\nSpeed in orbit at an altitude of %gkm: \t%g m/s\n\n", alt, v);

    return;
}

double deg2rad(double deg) {
    return deg*(M_PI/180);
}