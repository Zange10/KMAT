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

#endif //KSP_PORKCHOP_ANALYZER_TOOLS_H
