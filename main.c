#include<stdio.h>
#include<math.h>

struct Vessel init_vessel();
struct Flight init_flight(struct Body *body);
struct Body init_body();
double get_pitch(double t);
void print_vessel_info(struct Vessel *v);
void print_flight_info(struct Flight *f);
void calculate_flight(struct Vessel *v, struct Flight *f, double t);
void update_vessel(struct Vessel *v, double t);
double get_ship_mass(struct Vessel *v, double t);
double get_ship_acceleration(struct Vessel *v, double t);
double get_ship_hacceleration(struct Vessel *v, double t);
double get_ship_vacceleration(struct Vessel *v, double t);
void update_flight(struct Vessel *v, struct Vessel *last_v, struct Flight *f, struct Flight *last_f, double t);
double get_flight_hvelocity(struct Vessel *v, struct Vessel *last_v, struct Flight *f, struct Flight *last_f, double step);

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

// ------------------------------------------------------------

int main() {
    vessel = init_vessel();
    earth = init_body();
    flight = init_flight(&earth);

    print_vessel_info(&vessel);
    print_flight_info(&flight);

    calculate_flight(&vessel, &flight, 100);


    print_vessel_info(&vessel);
    print_flight_info(&flight);

    return 0;
}

// ------------------------------------------------------------

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

struct Flight init_flight(struct Body *body) {
    struct Flight new_flight;
    new_flight.g = body -> mu / pow(body -> radius, 2);
    new_flight.ac = 0;
    new_flight.ab = 0;
    new_flight.av = 0;
    new_flight.vh = 0;
    new_flight.vv = 0;
    new_flight.v = 0;
    new_flight.h = 0;
    new_flight.r = body -> radius;
    new_flight.Ap = 0;
    return new_flight;
}

struct Body init_body() {
    struct Body new_body;
    new_body.mu = 3.98574405e14;
    new_body.radius = 6371000;
    return new_body;
}

// ------------------------------------------------------------

// Prints parameters specific to the vessel
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

// Prints parameters specific to the flight
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

// Calculates vessel and flight parameters over time
void calculate_flight(struct Vessel *v, struct Flight *f, double t) {
    double start = 0;
    double end = t;
    double step = 0.01;

    struct Vessel v_last;
    struct Flight f_last;

    update_vessel(v, start);

    v_last = *v;
    f_last = *f;
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



// update parameters of vessel for the point in time t of the flight
void update_vessel(struct Vessel *v, double t) {
    v -> mass = get_ship_mass(v, t);
    v -> pitch = get_pitch(t);
    v -> a = get_ship_acceleration(v,t);
    v -> ah = get_ship_hacceleration(v,t);
    v -> av = get_ship_vacceleration(v,t);
    return;
}

// get vessels's pitch after time t
double get_pitch(double t) {
    // (9/4000)t² - 0.9t + 90  -->  vertex (200|0) and f(0) = 90
    return (9.0/4000.0)*pow(t,2) - 0.9*t + 90.0;
}

// get vessels's mass after time t
double get_ship_mass(struct Vessel *v, double t) {
    return v->m0 - v->burn_rate*t;
}

// get vessels's acceleration (Thrust) after time  t
double get_ship_acceleration(struct Vessel *v, double t) {
    return v->F / v->mass;
}

// get vessels's horizontal acceleration (Thrust) after time t
double get_ship_hacceleration(struct Vessel *v, double t) {
    return v->a * cos(deg_to_rad(v -> pitch));
}

// get vessels's vertical acceleration (Thrust) after time t
double get_ship_vacceleration(struct Vessel *v, double t) {
    return v->a * sin(deg_to_rad(v -> pitch));
}





// update parameters of the flight for the point in time t of the flight
void update_flight(struct Vessel *v, struct Vessel *last_v, struct Flight *f, struct Flight *last_f, double step) {
    f -> vh += get_flight_hvelocity(v,last_v,f,last_f,step);
}

// integrate horizontal acceleration over time for a given interval (numerical integration, midpoint/rectangle rule)
double get_flight_hvelocity(struct Vessel *v, struct Vessel *last_v, struct Flight *f, struct Flight *last_f, double step) {
    return ( (v->ah + last_v->ah)/2 )*step;
}


// transforms degrees to radiens
double deg_to_rad(double deg) {
    return deg*(3.14159265/180);
}