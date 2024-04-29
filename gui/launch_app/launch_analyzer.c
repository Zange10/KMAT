#include "launch_analyzer.h"
#include "database/lv_database.h"
#include "launch_calculator/lv_profile.h"


GObject *cb_la_sel_launcher;
GObject *cb_la_sel_profile;

void on_run_launch_simulation(GtkWidget* widget, gpointer data);

void init_launch_analyzer(GtkBuilder *builder) {
	int lv_id = 6;
	struct LV lv = get_lv_from_database(lv_id);
	struct LaunchProfiles_DB profiles = db_get_launch_profiles_from_lv_id(lv_id);
	lv.lp_id = profiles.profile[0].profiletype;
	for(int i = 0; i < 5; i++) lv.lp_params[i] = profiles.profile[0].lp_params[i];
	print_LV(&lv);
	for(int i = 0; i < profiles.num_profiles; i++) {
		printf("%d %f %f %f %f %f %s\n",
			   profiles.profile[i].profiletype,
			   profiles.profile[i].lp_params[0],
			   profiles.profile[i].lp_params[1],
			   profiles.profile[i].lp_params[2],
			   profiles.profile[i].lp_params[3],
			   profiles.profile[i].lp_params[4],
			   profiles.profile[i].usage);
	}
	return;
	
	cb_la_sel_launcher = gtk_builder_get_object(builder, "cb_la_sel_launcher");
	cb_la_sel_profile = gtk_builder_get_object(builder, "cb_la_sel_profile");
	
	GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	GtkTreeIter iter;
	// Add items to the list store
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, "<Select Launcher>", 1, 1, -1);
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, "x", 1, 2, -1);
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, "asoödkgäaewöjgpokwpojä", 1, 3, -1);
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, "#aepagi+oigpoiwu0sügpjapejgwjg", 1, 4, -1);
	
	gtk_combo_box_set_model(GTK_COMBO_BOX(cb_la_sel_launcher), GTK_TREE_MODEL(store));
	gtk_combo_box_set_active(GTK_COMBO_BOX(cb_la_sel_launcher), 0);
	
	g_object_unref(store);
	// Create a cell renderer
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cb_la_sel_launcher), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cb_la_sel_launcher), renderer, "text", 0, NULL);
}


void on_run_launch_simulation(GtkWidget* widget, gpointer data) {
	int id = gtk_combo_box_get_active(GTK_COMBO_BOX(cb_la_sel_launcher));
	printf("%d\n", id);
}
