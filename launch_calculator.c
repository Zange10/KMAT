#include <stdio.h>
#include <math.h>

#include "launch_calculator.h"

struct Vessel {
    double F;           // Thrust produced by the engines [N]
    double mass;        // vessel mass [kg]
    double m0;          // initial mass (mass at t0) [kg]
    double burn_rate;   // burn rate of all running engines combined [kg/s]
    double pitch;       // pitch of vessel during flight [°]
    double a;           // acceleration due to Thrust [m/s²]
    double ah;          // horizontal acceleration due to Thrust and pitch [m/s²]
    double av;          // vertical acceleration due to Thrust and pitch [m/s²]
} vessel;

struct Flight {
    struct Body *body;
    double g;       // gravitational acceleration [m/s²]
    double ac;      // negative centripetal force due to horizontal velocity [m/s²]
    double ab;      // gravitational a subtracted by cetrifucal a [m/s²]
    double av;      // current vertical acceleration due to Thrust, pitch, gravity and velocity [m/s²]
    double vh;      // horizontal velocity [m/s]
    double vv;      // vertical velocity [m/s]
    double v;       // overall velocity [m/s]
    double h;       // altitude above sea level [m]
    double r;       // distance to center of body [m]
    double Ap;      // highest point of orbit in reference to h [m]
} flight;

struct Body {
    double mu;      // gravitational parameter of body [m³/s²]
    double radius;  // radius of body [m]
} earth;


struct Vessel init_vessel(double F, double m0, double br) {
    struct Vessel new_vessel;
    new_vessel.F = F*1000;  // kN to N
    new_vessel.m0 = m0*1000;    // t to kg
    new_vessel.mass = m0*1000;  // t to kg
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
    new_flight.g = body -> mu / pow(body -> radius, 2);
    new_flight.ac = 0;
    new_flight.ab = 0;
    new_flight.av = 0;
    new_flight.vh = 0;
    new_flight.vv = 0;
    new_flight.v = 0;
    new_flight.h = 0;
    new_flight.r = body -> radius + 80000;      // currently hard coded to altitude of 80km
    new_flight.Ap = 0;
    return new_flight;
}

struct Body init_body() {
    struct Body new_body;
    new_body.mu = 3.98574405e14;
    new_body.radius = 6371000;
    return new_body;
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
    printf("Gravity:\t\t%g m/s²\n", f -> g);
    printf("Centrifugal a:\t\t%g m/s²\n", f -> ac);
    printf("Balanced a:\t\t%g m/s²\n", f -> ab);
    printf("Vertical a:\t\t%g m/s²\n", f -> av);
    printf("Horizontal v:\t\t%g m/s\n", f -> vh);
    printf("Vertical v:\t\t%g m/s\n", f -> vv);
    printf("Velocity:\t\t%g m/s\n", f -> v);
    printf("Altitude:\t\t%g m\n", f -> h);
    printf("Radius:\t\t%g m\n", f -> r);
    printf("Apoapsis:\t\t%g m\n", f -> Ap);
    printf("______________________\n\n");
}

// ------------------------------------------------------------

void calculate_launch() {
    vessel = init_vessel(600, 50, 200);
    earth = init_body();
    flight = init_flight(&earth);

    print_vessel_info(&vessel);
    print_flight_info(&flight);

    calculate_flight(&vessel, &flight, 100);

    print_vessel_info(&vessel);
    print_flight_info(&flight);
}


void calculate_flight(struct Vessel *v, struct Flight *f, double t) {
    double start = 0;
    double end = t;
    double step = 0.01;

    struct Vessel v_last;
    struct Flight f_last;

    update_vessel(v, start);
    v_last = *v;
    f_last = *f;
    update_flight(v,&v_last, f, &f_last, 0);

    double x;

    for(x = start+step; x <= end-step; x += step) {
        update_vessel(&vessel, x);

        update_flight(v,&v_last, f, &f_last, step);

        v_last = *v;
        f_last = *f;
    }

    update_vessel(&vessel, end);
    update_flight(v,&v_last, f, &f_last, end-x);
}



void update_vessel(struct Vessel *v, double t) {
    v -> mass = get_ship_mass(v, t);
    v -> pitch = get_pitch(t);
    v -> a = get_ship_acceleration(v,t);
    v -> ah = get_ship_hacceleration(v,t);
    v -> av = get_ship_vacceleration(v,t);
    return;
}

double get_pitch(double t) {
    return (9.0/4000.0)*pow(t,2) - 0.9*t + 90.0;
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



void update_flight(struct Vessel *v, struct Vessel *last_v, struct Flight *f, struct Flight *last_f, double step) {
    f -> vh += integrate(v->ah,last_v->ah,step);    // integrate horizontal acceleration
    f -> ac  = calc_centrifugal_acceleration(f);
    f -> g   = calc_grav_acceleration(f);
    f -> ab  = calc_balanced_acceleration(f);
    f -> av  = calc_vertical_acceleration(v,f);
    f -> vv += integrate(f->av,last_f->av,step);    // integrate vertical acceleration
    f -> v   = calc_velocity(f->vh,f->vv);
    f -> h  += integrate(f->vv,last_f->vv,step);    // integrate vertical velocity
    f -> Ap  = calc_Apoapsis(f);
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
    return v->a * sin(deg_to_rad(v->pitch)) - f->ab;
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
    return deg*(3.14159265/180);
}