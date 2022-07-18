#include<stdio.h>
#include<math.h>

struct Vessel init_vessel();
struct Flight init_flight();
struct Body init_body();

struct Vessel {
    double Thrust;
    double mass;
    double burn_rate;
} vessel;

struct Flight {
    double pitch;
    double g;
    double vh;
    double vv;
    double v;
    double h;
    double r;
    double Ap;
    double ac;
    double ab;
} flight;

struct Body {
    double mu;
    double radius;
} earth;

int main() {
    vessel = init_vessel();
    flight = init_flight();
    earth = init_body();

    return 0;
}


struct Vessel init_vessel() {
    struct Vessel new_vessel;
    new_vessel.Thrust = 50;
    new_vessel.mass = 500;
    new_vessel.burn_rate = 2;
    return new_vessel;
}

struct Flight init_flight() {
    struct Flight new_flight;
    new_flight.pitch = 0;
    new_flight.g = 0;
    new_flight.vh = 0;
    new_flight.vv = 0;
    new_flight.v = 0;
    new_flight.h = 0;
    new_flight.h = 0;
    new_flight.r = new_flight.h+6371000;
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


