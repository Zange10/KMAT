#ifndef KSP_TRANSFER_CALC_H
#define KSP_TRANSFER_CALC_H

#include "tools/analytic_geometry.h"
#include "itin_tool.h"

struct Transfer_Calc_Data {
	struct Body **bodies;
	int num_steps;
	double jd_min_dep;
	double jd_max_dep;
	int *min_duration;
	int *max_duration;
	struct Dv_Filter dv_filter;
};

struct Transfer_Calc_Results {
	struct ItinStep **departures;
	int num_deps, num_nodes;
};

struct Transfer_Calc_Results create_itinerary(struct Transfer_Calc_Data calc_data);
void test_itinerary();

#endif //KSP_TRANSFER_CALC_H
