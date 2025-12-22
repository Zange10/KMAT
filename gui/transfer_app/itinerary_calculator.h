#ifndef KSP_ITINERARY_CALCULATOR_H
#define KSP_ITINERARY_CALCULATOR_H

#include <gtk/gtk.h>
#include "orbitlib.h"

void init_itinerary_calculator(GtkBuilder *builder);
void ic_change_date_type(enum DateType old_date_type, enum DateType new_date_type);
void reset_ic();



// Handler ----------------------

G_MODULE_EXPORT void on_calc_ic();
G_MODULE_EXPORT void on_ic_system_change();
G_MODULE_EXPORT void on_ic_central_body_change();
G_MODULE_EXPORT void on_get_ic_ref_values();

#endif //KSP_ITINERARY_CALCULATOR_H
