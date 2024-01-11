//
// Created by niklas on 08.01.24.
//

#ifndef KSP_WINDOW_H
#define KSP_WINDOW_H

#include <gtk/gtk.h>

struct Window {
	GtkWidget *window;
	int width;
	int height
};

void create_window();

#endif //KSP_WINDOW_H
