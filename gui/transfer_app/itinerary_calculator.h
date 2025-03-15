#ifndef KSP_ITINERARY_CALCULATOR_H
#define KSP_ITINERARY_CALCULATOR_H

#include <gtk/gtk.h>
#include "tools/datetime.h"

void init_itinerary_calculator(GtkBuilder *builder);
void ic_change_date_type(enum DateType old_date_type, enum DateType new_date_type);
void reset_ic();

#endif //KSP_ITINERARY_CALCULATOR_H
