#include<stdio.h>
#include<math.h>

struct Vessel init_vessel();
struct Flight init_flight();
struct Body init_body();
double get_pitch(double t);
void print_vessel_info(struct Vessel *v);
void update_vessel(struct Vessel *v, double t);
double get_ship_mass(struct Vessel *v, double t);
double get_ship_acceleration(struct Vessel *v, double t);
double get_ship_hacceleration(struct Vessel *v, double t);
double get_ship_vacceleration(struct Vessel *v, double t);

double deg_to_rad(double deg);

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
    double g;       // gravitational acceleration [m/s²]
    double ac;      // negative centripetal force due to horizontal velocity [m/s²]
    double ab;      // current acceleration towards the surface (balanced acceleration) [m/s²]
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

int main() {
    vessel = init_vessel();
    flight = init_flight();
    earth = init_body();

    double t = 0;
    
    print_vessel_info(&vessel);

    update_vessel(&vessel, 0);
    print_vessel_info(&vessel);

    update_vessel(&vessel, 50);
    print_vessel_info(&vessel);

    update_vessel(&vessel, 180);
    print_vessel_info(&vessel);


    return 0;
}


struct Vessel init_vessel() {
    struct Vessel new_vessel;
    new_vessel.F = 50;
    new_vessel.mass = 0;
    new_vessel.m0 = 500;
    new_vessel.burn_rate = 2;
    new_vessel.pitch = 0;
    new_vessel.a = 0;
    new_vessel.ah = 0;
    new_vessel.av = 0;
    return new_vessel;
}

struct Flight init_flight() {
    struct Flight new_flight;
    new_flight.g = 0;
    new_flight.vh = 0;
    new_flight.vv = 0;
    new_flight.v = 0;
    new_flight.h = 0;
    new_flight.h = 0;
    new_flight.r = 0;
    new_flight.Ap = 0;
    new_flight.ac = 0;
    new_flight.ab = 0;
    return new_flight;
}

struct Body init_body() {
    struct Body new_body;
    new_body.mu = 3.98574405e14;
    new_body.radius = 6371000;
    return new_body;
}


double get_pitch(double t) {
    // (9/4000)t² - 0.9t + 90  -->  vertex (200|0) and f(0) = 90
    return (9.0/4000.0)*pow(t,2) - 0.9*t + 90.0;
}

void print_vessel_info(struct Vessel *v) {
    printf("\n______________________\n");
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

void update_vessel(struct Vessel *v, double t) {
    v -> mass = get_ship_mass(v, t);
    v -> pitch = get_pitch(t);
    v -> a = get_ship_acceleration(v,t);
    v -> ah = get_ship_hacceleration(v,t);
    v -> av = get_ship_vacceleration(v,t);
    return;
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




double deg_to_rad(double deg) {
    return deg*(3.14159256/180);
}