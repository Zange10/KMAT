#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "launch_calculator.h"
#include "csv_writer.h"
#include "tool_funcs.h"

struct Vessel {
    double F_vac;       // Thrust produced by the engines in a vacuum [N]
    double F_sl;        // Thrust produced by the engines at sea level [N]
    double F;           // current Thrust produced by the engines [N]
    double mass;        // vessel mass [kg]
    double m0;          // initial mass (mass at t0) [kg]
    double burn_rate;   // burn rate of all running engines combined [kg/s]
    double pitch;       // pitch of vessel during flight [°]
    double a;           // acceleration due to Thrust [m/s²]
    double ah;          // horizontal acceleration due to Thrust and pitch [m/s²]
    double av;          // vertical acceleration due to Thrust and pitch [m/s²]
};

struct Flight {
    struct Body *body;
    double t;       // time passed since t0
    double p;       // atmospheric pressure
    double D;       // atmospheric Drag
    double ad;      // acceleration due to aerodynamic drag
    double ah;      // current horizontal acceleration due to thrust and with drag [m/s²]
    double g;       // gravitational acceleration [m/s²]
    double ac;      // negative centripetal force due to horizontal speed [m/s²]
    double ab;      // gravitational a subtracted by cetrifucal a [m/s²]
    double av;      // current vertical acceleration due to Thrust, pitch, gravity and velocity [m/s²]
    double vh;      // horizontal speed [m/s]
    double vv;      // vertical speed [m/s]
    double v;       // overall velocity [m/s]
    double h;       // altitude above sea level [m]
    double r;       // distance to center of body [m]
    double Ap;      // highest point of orbit in reference to h [m]
};

struct Body {
    double mu;      // gravitational parameter of body [m³/s²]
    double radius;  // radius of body [m]
};



struct Stage {
    double F_vac;       // Thrust produced by the engines in a vacuum [N]
    double F_sl;        // Thrust produced by the engines at sea level [N]
    double m0;          // initial mass (mass at t0) [kg]
    double me;          // vessel mass without fuel [kg]
    double burn_rate;   // burn rate of all running engines combined [kg/s]
};


struct LV {
    int stage_n;            // amount of stages of the launch vehicle
    double payload;         // payload mass [kg]
    struct Stage *stages;   // the stages of the launch vehicle
};






struct Vessel init_vessel(double F_sl, double F_vac, double m0, double br) {
    struct Vessel new_vessel;
    new_vessel.F_vac = F_vac;
    new_vessel.F_sl = F_sl;
    new_vessel.F = 0;
    new_vessel.m0 = m0;
    new_vessel.mass = 0;
    new_vessel.burn_rate = br;
    new_vessel.pitch = 0;
    new_vessel.a = 0;
    new_vessel.ah = 0;
    new_vessel.av = 0;
    return new_vessel;
}

struct Flight init_flight(struct Body *body) {
    struct Flight new_flight;
    new_flight.body = body;
    new_flight.t = 0;
    new_flight.p = 0;
    new_flight.D = 0;
    new_flight.ad = 0;
    new_flight.ah = 0;
    new_flight.g = body -> mu / pow(body -> radius, 2);
    new_flight.ac = 0;
    new_flight.ab = 0;
    new_flight.av = 0;
    new_flight.vh = 0;
    new_flight.vv = 0;
    new_flight.v = 0;
    new_flight.h = 0;
    new_flight.r = body -> radius;      // currently hard coded to altitude of 80km
    new_flight.Ap = 0;
    return new_flight;
}

struct Body init_body() {
    struct Body new_body;
    new_body.mu = 3.98574405e14;
    new_body.radius = 6371000;
    return new_body;
}

struct Stage init_stage(double F_sl, double F_vac, double m0, double me, double br) {
    struct Stage new_stage;
    new_stage.F_vac = F_vac*1000;   // kN to N
    new_stage.F_sl = F_sl*1000;     // kN to N
    new_stage.m0 = m0*1000;         // t to kg
    new_stage.me = me*1000;         // t to kg
    new_stage.burn_rate = br;
    return new_stage;
}

struct LV init_LV(int amt_of_stages, struct Stage *stages, int payload_mass) {
    struct LV new_lv;
    new_lv.stage_n = amt_of_stages;
    new_lv.stages = stages;
    new_lv.payload = payload_mass;
    return new_lv;
}


void print_vessel_info(struct Vessel *v) {
    printf("\n______________________\nVESSEL:\n\n");
    printf("Thrust:\t\t%g N\n", v -> F);
    printf("Mass:\t\t%g kg\n", v -> mass);
    printf("Initial mass:\t%g kg\n", v -> m0);
    printf("Burn rate:\t%g kg/s\n", v -> burn_rate);
    printf("Pitch:\t\t%g°\n", v -> pitch);
    printf("Acceleration:\t%g m/s²\n", v -> a);
    printf("Horizontal a:\t%g m/s²\n", v -> ah);
    printf("Vertical a:\t%g m/s²\n", v -> av);
    printf("______________________\n\n");
}

void print_flight_info(struct Flight *f) {
    printf("\n______________________\nFLIGHT:\n\n");
    printf("Time:\t\t\t%.2f s\n", f -> t);
    printf("Altitude:\t\t%g km\n", f -> h/1000);
    printf("Vertical v:\t\t%g m/s\n", f -> vv);
    printf("Horizontal v:\t\t%g m/s\n", f -> vh);
    printf("Velocity:\t\t%g m/s\n", f -> v);
    printf("\n");
    printf("Atmo press:\t\t%g bar\n", f -> p);
    printf("Drag:\t\t\t%g N\n", f -> D);
    printf("Drag a:\t\t\t%g m/s²\n", f -> ad);
    printf("Gravity:\t\t%g m/s²\n", f -> g);
    printf("Centrifugal a:\t\t%g m/s²\n", f -> ac);
    printf("Balanced a:\t\t%g m/s²\n", f -> ab);
    printf("Vertical a:\t\t%g m/s²\n", f -> av);
    printf("Horizontal a:\t\t%g m/s²\n", f -> ah);
    printf("Radius:\t\t\t%g km\n", f -> r/1000);
    printf("Apoapsis:\t\t%g km\n", f -> Ap/1000);
    printf("______________________\n\n");
}

// ------------------------------------------------------------

void launch_calculator() {
    struct LV lv;
    int stage_count;
    struct Stage stage;

    int selection = 0;
    char title[] = "LAUNCH CALCULATOR:";
    char options[] = "Go Back; Calculate";
    char question[] = "Program: ";
    do {
        selection = user_selection(title, options, question);

        switch(selection) {
            case 1:
                struct Stage stage = init_stage(610, 700, 50.908, 3.308, 246.6);
                lv = init_LV(1,&stage, 0);
                calculate_launch(lv);
                break;
        }
    } while(selection != 0);
}

void calculate_launch(struct LV lv) {
    struct Vessel vessel;
    struct Body earth = init_body();
    struct Flight flight = init_flight(&earth);

    print_vessel_info(&vessel);
    print_flight_info(&flight);

    for(int i = 0; i < lv.stage_n; i++) {
        vessel = init_vessel(lv.stages[i].F_sl, lv.stages[i].F_vac, lv.stages[i].m0, lv.stages[i].burn_rate);
        int burn_duration = (lv.stages[i].m0-lv.stages[i].me) / lv.stages[i].burn_rate;
        calculate_stage_flight(&vessel, &flight, burn_duration);
    }

    print_vessel_info(&vessel);
    print_flight_info(&flight);
}

void calculate_stage_flight(struct Vessel *v, struct Flight *f, double T) {
    double start = 0;
    double end = T;
    double step = 0.001;

    char flight_data_fields[] = "Time,Thrust,Mass,Pitch,VessAcceleration,AtmoPress,Drag,DragA,HorizontalA,Gravity,CentrifugalA,BalancedA,VerticalA,HorizontalV,VerticalV,Velocity,Altitude,Apoapsis";
    double *flight_data = (double*) calloc(1, sizeof(double));
    flight_data[0] = 1;    // amount of data points
    struct Vessel v_last;
    struct Flight f_last;

    start_stage(v, f);
    v_last = *v;
    f_last = *f;
    store_flight_data(v, f, flight_data);

    for(f->t = start+step; f->t <= end; f->t += step) {
        update_flight(v,&v_last, f, &f_last, f->t, step);
        double x = remainder(f->t,((end-start)/888));   // only store 890 (888 in this loop) data points
        if(x <= step && x >=0) store_flight_data(v, f, flight_data);
        v_last = *v;
        f_last = *f;
    }
    f->t -= step;
    update_flight(v,&v_last, f, &f_last, f->t, end-f->t);
    f->t = end;
    store_flight_data(v, f, flight_data);
    char pcsv;
    printf("Write data to .csv (y/Y=yes)? ");
    scanf(" %c", &pcsv);
    if(pcsv == 'y' || pcsv == 'Y') {
        write_csv(flight_data_fields, flight_data);
    }

    free(flight_data);
}



void start_stage(struct Vessel *v, struct Flight *f) {
    update_vessel(v, 0, 1, 0);
    f -> p   = get_atmo_press(f->h);
    f -> r   = f->h + f->body->radius;
    f -> v   = calc_velocity(f->vh,f->vv);
    f -> D   = calc_aerodynamic_drag(f->p, f->v);
    f -> ad  = f->D/v->mass;
    f -> ah  = v->ah-f->ad*sin(deg_to_rad(v->pitch));
    f -> ac  = calc_centrifugal_acceleration(f);
    f -> g   = calc_grav_acceleration(f);
    f -> ab  = calc_balanced_acceleration(f);
    f -> av  = calc_vertical_acceleration(v,f);
    f -> Ap  = calc_Apoapsis(f);
}


void update_flight(struct Vessel *v, struct Vessel *last_v, struct Flight *f, struct Flight *last_f, double t, double step) {
    f -> p   = get_atmo_press(f->h);
    update_vessel(v, t, f->p, f->h);
    f -> D  = calc_aerodynamic_drag(f->p, f->v);
    f -> ad  = f->D/v->mass;
    f -> ah  = v->ah-f->ad*sin(deg_to_rad(v->pitch));
    f -> vh += integrate(f->ah,last_f->ah,step);    // integrate horizontal acceleration
    f -> ac  = calc_centrifugal_acceleration(f);
    f -> g   = calc_grav_acceleration(f);
    f -> ab  = calc_balanced_acceleration(f);
    f -> av  = calc_vertical_acceleration(v,f);
    f -> vv += integrate(f->av,last_f->av,step);    // integrate vertical acceleration
    f -> v   = calc_velocity(f->vh,f->vv);
    f -> h  += integrate(f->vv,last_f->vv,step);    // integrate vertical speed
    f -> r   = f->h + f->body->radius;
    f -> Ap  = calc_Apoapsis(f);
}


void update_vessel(struct Vessel *v, double t, double p, double h) {
    v -> F = get_thrust(v, p);
    v -> mass = get_ship_mass(v, t);
    v -> pitch = get_pitch(h);
    v -> a = get_ship_acceleration(v,t);
    v -> ah = get_ship_hacceleration(v,t);
    v -> av = get_ship_vacceleration(v,t);
    return;
}

double get_atmo_press(double h) {
    if(h<140e3) return exp(-1.4347e-4 * h);
    else return 0;
}

double calc_aerodynamic_drag(double p, double v) {
    return 0.5*(p*101325)*pow(v,2) * 7e-5;    // p: bar to Pa; constant by good guess
}

double get_thrust(struct Vessel *v, double p) {
    return v->F_vac + p*( v->F_sl - v->F_vac );
}

double get_pitch(double h) {
    //return (9.0/4000.0)*pow(t,2) - 0.9*t + 90.0;
    //return 90.0-(90.0/200.0)*t;
    if(h < 38108) return 90.0*exp(-0.00003*h);
    else return 42.0*exp(-0.00001*h);
}

double get_ship_mass(struct Vessel *v, double t) {
    return v->m0 - v->burn_rate*t;
}

double get_ship_acceleration(struct Vessel *v, double t) {
    return v->F / v->mass;
}

double get_ship_hacceleration(struct Vessel *v, double t) {
    return v->a * cos(deg_to_rad(v -> pitch));
}

double get_ship_vacceleration(struct Vessel *v, double t) {
    return v->a * sin(deg_to_rad(v -> pitch));
}



double calc_centrifugal_acceleration(struct Flight *f) {
    return pow(f->vh, 2) / f->r;
}

double calc_grav_acceleration(struct Flight *f) {
    return f->body->mu / pow(f->r,2);
}

double calc_balanced_acceleration(struct Flight *f) {
    return f->g - f->ac;
}

double calc_vertical_acceleration(struct Vessel *v, struct Flight *f) {
    return v->av - f->ab - f->ad*cos(deg_to_rad(v->pitch));
}

double calc_velocity(double vh, double vv) {
    return sqrt(vv*vv+vh*vh);
}

double calc_Apoapsis(struct Flight *f) {
    return (pow(f->vv,2) / (2*f->ab)) + f->h;
}



double integrate(double fa, double fb, double step) {
    return ( (fa+fb)/2 ) * step;
}

double deg_to_rad(double deg) {
    return deg*(M_PI/180);
}





void store_flight_data(struct Vessel *v, struct Flight *f, double *data) {
    int initial_length = (int)data[0];
    data = (double*) realloc(data, (initial_length+18)*sizeof(double));
    data[initial_length+0] = f->t;
    data[initial_length+1] = v->F;
    data[initial_length+2] = v->mass;
    data[initial_length+3] = v->pitch;
    data[initial_length+4] = v->a;
    data[initial_length+5] = f->p;
    data[initial_length+6] = f->D;
    data[initial_length+7] = f->ad;
    data[initial_length+8] = f->ah;
    data[initial_length+9] = f->g;
    data[initial_length+10]= f->ac;
    data[initial_length+11]= f->ab;
    data[initial_length+12]= f->av;
    data[initial_length+13]= f->vh;
    data[initial_length+14]= f->vv;
    data[initial_length+15]= f->v;
    data[initial_length+16]= f->h;
    data[initial_length+17]= f->Ap;
    data[0] += 18;
    return;
}