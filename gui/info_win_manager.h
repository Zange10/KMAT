#ifndef KSP_INFO_WIN_MANAGER_H
#define KSP_INFO_WIN_MANAGER_H

#include <gtk/gtk.h>

void init_info_windows(GtkBuilder *builder);
void init_sc_ic_progress_window();
void end_sc_ic_progress_window();
void show_msg_window(char *msg);


// Handler ----------------------------
G_MODULE_EXPORT void end_progress_calculation();
G_MODULE_EXPORT gboolean on_hide_msg_window();
G_MODULE_EXPORT void on_msg_window_ok();

#endif //KSP_INFO_WIN_MANAGER_H
