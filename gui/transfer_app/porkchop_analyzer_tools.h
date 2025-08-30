#ifndef KSP_PORKCHOP_ANALYZER_TOOLS_H
#define KSP_PORKCHOP_ANALYZER_TOOLS_H

#include "orbit_calculator/itin_tool.h"
#include <gtk/gtk.h>


struct PorkchopGroup {
	int num_steps;
	int count;
	int show_group;
	int has_itin_inside_filter;
	struct ItinStep *sample_arrival_node;
	GtkWidget *cb_pa_show_group;
};

struct PorkchopAnalyzerPoint {
	struct PorkchopPoint data;
	int inside_filter;
	struct PorkchopGroup *group;
};


void sort_porkchop(struct PorkchopAnalyzerPoint *pp, int num_itins, enum LastTransferType last_transfer_type);

void get_min_max_dep_dur_range_from_mouse_rect(double *dep0, double *dep1, double *dur0, double *dur1, double min_dep, double max_dep, double min_dur, double max_dur, double screen_width, double screen_height, int dur0arrdate1);


#endif //KSP_PORKCHOP_ANALYZER_TOOLS_H
