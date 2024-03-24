#ifndef KSP_PORKCHOP_ANALYZER_TOOLS_H
#define KSP_PORKCHOP_ANALYZER_TOOLS_H

#include "orbit_calculator/itin_tool.h"

void quicksort_porkchop_and_arrivals(double *arr, int low, int high, double *porkchop, struct ItinStep **arrivals);
void reset_porkchop_and_arrivals(const double *all_porkchop, double *porkchop, struct ItinStep **all_arrivals, struct ItinStep **arrivals, int num_itins);

#endif //KSP_PORKCHOP_ANALYZER_TOOLS_H
