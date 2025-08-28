#ifndef KSP_SEQUENCE_CALCULATOR_H
#define KSP_SEQUENCE_CALCULATOR_H

#include <gtk/gtk.h>
#include "orbitlib.h"

void init_sequence_calculator(GtkBuilder *builder);
void sc_change_date_type(enum DateType old_date_type, enum DateType new_date_type);
void reset_sc();

struct PlannedScStep {
	Body *body;
	struct PlannedScStep *prev;
	struct PlannedScStep *next;
};



// Handler --------------------------------------------------
G_MODULE_EXPORT void on_calc_sc();
G_MODULE_EXPORT void on_sc_system_change();

#endif //KSP_SEQUENCE_CALCULATOR_H
