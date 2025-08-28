#include "settings.h"
#include "orbitlib.h"
#include "gui_manager.h"


GObject *cb_settings_datetime_type;


struct GlobalSettings {
	enum DateType date_type;
} global_settings = {DATE_ISO};


void init_global_settings(GtkBuilder *builder) {
	cb_settings_datetime_type = gtk_builder_get_object(builder, "cb_settings_datetime_type");
}

G_MODULE_EXPORT void on_change_datetime_type() {
	enum DateType old_type = global_settings.date_type;
	global_settings.date_type = gtk_combo_box_get_active(GTK_COMBO_BOX(cb_settings_datetime_type));
	change_gui_date_type(old_type, global_settings.date_type);
}

enum DateType get_settings_datetime_type() {
	return global_settings.date_type;
}