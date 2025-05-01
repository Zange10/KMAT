#include "css_loader.h"


GtkCssProvider *provider;

void set_window_style_css(char *filepath_css) {
	GError *error = NULL;
	GtkCssProvider *win_css_provider = gtk_css_provider_new();

	// Load the custom CSS file (theme)
	if (!gtk_css_provider_load_from_path(win_css_provider, filepath_css, &error)) {
		g_printerr("Failed to load CSS: %s\n", error->message);
		g_error_free(error);
		return;
	}

	// Apply the loaded theme to the default screen
	gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
											  GTK_STYLE_PROVIDER(win_css_provider),
											  GTK_STYLE_PROVIDER_PRIORITY_USER);

}

void load_css(char *filepath) {
	// Load CSS from an external file
	provider = gtk_css_provider_new();
	GError *error = NULL;
	gtk_css_provider_load_from_path(provider, filepath, &error);
	// Check for errors while loading the CSS file
	if (error) {
		g_printerr("Error loading CSS file: %s\n", error->message);
		g_error_free(error);
	}
}

void set_css_class_for_widget(GtkWidget *widget, char *class) {
	// Add a CSS class to the label
	gtk_widget_set_name(widget, class);
	// Apply the CSS to the label
	GtkStyleContext *context = gtk_widget_get_style_context(widget);
	gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider),
								   GTK_STYLE_PROVIDER_PRIORITY_USER);
}




