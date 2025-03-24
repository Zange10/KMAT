#ifndef KSP_CSS_LOADER_H
#define KSP_CSS_LOADER_H

#include <gtk/gtk.h>

void set_window_style_css(char *filepath);
void load_css(char *filepath);
void set_css_class_for_widget(GtkWidget *widget, char *class);


#endif //KSP_CSS_LOADER_H