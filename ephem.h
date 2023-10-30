#ifndef KSP_EPHEM_H
#define KSP_EPHEM_H

struct Ephem {
    double date, x, y, z, vx, vy,vz;
};

struct Date {
    int y, m, d, h, min;
};

void get_ephem();

#endif //KSP_EPHEM_H
