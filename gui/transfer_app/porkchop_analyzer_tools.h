#ifndef KSP_PORKCHOP_ANALYZER_TOOLS_H
#define KSP_PORKCHOP_ANALYZER_TOOLS_H

#include "orbit_calculator/itin_tool.h"
#include <gtk/gtk.h>

struct Group {
	int num_steps;
	int count;
	int show_group;
	struct ItinStep *sample_arrival_node;
	GtkWidget *cb_pa_show_group;
};


void quicksort_porkchop_and_arrivals(double *arr, int low, int high, double *porkchop, struct ItinStep **arrivals);
void reset_porkchop_and_arrivals(const double *all_porkchop, double *porkchop, struct ItinStep **all_arrivals, struct ItinStep **arrivals);
int filter_porkchop_arrivals_depdate(double *porkchop, struct ItinStep **arrivals, double min_date, double max_date);
int filter_porkchop_arrivals_dur(double *porkchop, struct ItinStep **arrivals, double min_date, double max_date);
int filter_porkchop_arrivals_totdv(double *porkchop, struct ItinStep **arrivals, double min_date, double max_date, int fb0_pow1);
int filter_porkchop_arrivals_depdv(double *porkchop, struct ItinStep **arrivals, double min_date, double max_date);
int filter_porkchop_arrivals_satdv(double *porkchop, struct ItinStep **arrivals, double min_date, double max_date, int fb0_pow1);
int get_itinerary_group_index(struct ItinStep *arrival_step, struct Group *groups, int num_groups);
int filter_porkchop_arrivals_groups(double *porkchop, struct ItinStep **arrivals, struct Group *groups, int num_groups);

#endif //KSP_PORKCHOP_ANALYZER_TOOLS_H
