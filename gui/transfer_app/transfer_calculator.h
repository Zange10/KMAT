#ifndef KSP_TRANSFER_CALCULATOR_H
#define KSP_TRANSFER_CALCULATOR_H

#include <gtk/gtk.h>
#include "tools/datetime.h"

void init_transfer_calculator(GtkBuilder *builder);
void tc_change_date_type(enum DateType old_date_type, enum DateType new_date_type);
void reset_tc();






// Handler --------------------------------------------------
void on_add_transfer_tc();
void on_remove_transfer_tc();
void on_update_tc();
void on_calc_tc();

#endif //KSP_TRANSFER_CALCULATOR_H
