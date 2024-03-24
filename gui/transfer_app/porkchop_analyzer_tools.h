#ifndef KSP_PORKCHOP_ANALYZER_TOOLS_H
#define KSP_PORKCHOP_ANALYZER_TOOLS_H

#include "orbit_calculator/itin_tool.h"

void quicksort_porkchop_and_arrivals(double *arr, int low, int high, double *porkchop, struct ItinStep **arrivals);
void reset_porkchop_and_arrivals(const double *all_porkchop, double *porkchop, struct ItinStep **all_arrivals, struct ItinStep **arrivals);
int filter_porkchop_arrivals_depdate(double *porkchop, struct ItinStep **arrivals, double min_date, double max_date);
int filter_porkchop_arrivals_dur(double *porkchop, struct ItinStep **arrivals, double min_date, double max_date);
int filter_porkchop_arrivals_totdv(double *porkchop, struct ItinStep **arrivals, double min_date, double max_date, int fb0_pow1);
int filter_porkchop_arrivals_depdv(double *porkchop, struct ItinStep **arrivals, double min_date, double max_date);
int filter_porkchop_arrivals_satdv(double *porkchop, struct ItinStep **arrivals, double min_date, double max_date, int fb0_pow1);

#endif //KSP_PORKCHOP_ANALYZER_TOOLS_H
