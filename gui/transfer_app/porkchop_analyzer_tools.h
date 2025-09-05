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

void get_min_max_dep_arr_dur_range_from_mouse_rect(double *p_x0, double *p_x1, double *p_y0, double *p_y1, double min_x_val, double max_x_val, double min_y_val, double max_y_val, double screen_width, double screen_height, int dur0arrdate1);


#endif //KSP_PORKCHOP_ANALYZER_TOOLS_H
