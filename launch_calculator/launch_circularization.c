#include "launch_circularization.h"
#include <math.h>

double calc_gravity(double mu, double r) {
    return mu/pow(r,2);
}

// returns the burn time until circularization
double calc_burntime(double mu, double r, double vh_0, double ve, double m0, double br) {
    double dv = sqrt(mu/r) - vh_0;
    double t = m0/br * (1 - exp(-dv/ve));
    return t;
}

// returns the average acceleration due to thrust until circularization
double calc_avg_thrust_acc(double ve, double m0, double br, double t) {
    return ve * log(m0 / (m0 - br*t)) / t;
}

// returns the average speed until circularization
double calc_avg_vh(double ve, double m0, double br, double t, double vh_0) {
    double mf = m0 - br*t;
    double temp1 = ve / (br*t);
    double temp2 = (mf * log(mf/m0) + br*t);
    return temp1 * temp2 + vh_0;
}

double get_circularization_pitch(double F, double m0, double br, double vh_0, double vv_0, double r, double mu) {
    double g = calc_gravity(mu, r);
    double ve = F/br;
    double t = calc_burntime(mu, r, vh_0, ve, m0, br);
    double a_T = calc_avg_thrust_acc(ve, m0, br, t);
    double v_h = calc_avg_vh(ve, m0, br, t, vh_0);
    double p = asin(-1/a_T * (v_h*v_h/r - g + vv_0/t));
    return p/M_PI * 180;
}