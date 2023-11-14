#ifndef KSP_EPHEM_H
#define KSP_EPHEM_H

struct Ephem {
    double date, x, y, z, vx, vy,vz;
};

struct Date {
    int y, m, d, h, min;
    double s;
};

void print_date(struct Date date, int line_break);

double convert_date_JD(struct Date date);

struct Date convert_JD_date(double JD);

void print_ephem(struct Ephem ephem);

void get_ephem(struct Ephem *ephem, int size_ephem, int body_code, int time_steps, double jd0, double jd1, int download);

struct Ephem get_last_ephem(struct Ephem *ephem, double date);

#endif //KSP_EPHEM_H
