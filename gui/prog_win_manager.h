#ifndef KSP_PROG_WIN_MANAGER_H
#define KSP_PROG_WIN_MANAGER_H

#include <gtk/gtk.h>

void init_prog_window(GtkBuilder *builder);
void init_tc_ic_progress_window();
void end_tc_ic_progress_window();


// Handler ----------------------------
G_MODULE_EXPORT void end_progress_calculation();

#endif //KSP_PROG_WIN_MANAGER_H
