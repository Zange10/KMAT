//
// Created by niklas on 08.01.24.
//

#ifndef KSP_EPHEM_APP_H
#define KSP_EPHEM_APP_H

#include <gtk/gtk.h>

struct Window {
	GtkWidget *window;
	int width;
	int height
};

void start_ephem_app();

#endif //KSP_EPHEM_APP_H
