#ifndef KSP_TRANSFER_CALCULATOR_H
#define KSP_TRANSFER_CALCULATOR_H

#include <gtk/gtk.h>
#include "tools/datetime.h"

void init_transfer_calculator(GtkBuilder *builder);
void tc_change_date_type(enum DateType old_date_type, enum DateType new_date_type);
void reset_tc();

struct PlannedStep {
	struct Body *body;
	struct PlannedStep *prev;
	struct PlannedStep *next;
};



// Handler --------------------------------------------------
G_MODULE_EXPORT void on_update_tc();
G_MODULE_EXPORT void on_calc_tc();
G_MODULE_EXPORT void on_tc_system_change();

#endif //KSP_TRANSFER_CALCULATOR_H
