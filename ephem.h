#ifndef KSP_EPHEM_H
#define KSP_EPHEM_H

struct Ephem {
    double date, x, y, z, vx, vy,vz;
};

struct Date {
    int y, m, d, h, min;
};

void print_ephem(struct Ephem ephem);

void get_ephem(struct Ephem *ephem, double size_ephem);

#endif //KSP_EPHEM_H
