#include "launch_analyzer.h"
#include "database/lv_database.h"
#include "launch_calculator/lv_profile.h"


GObject *cb_la_sel_launcher;
GObject *cb_la_sel_profile;

struct LV *all_launcher;
int *launcher_id;
int num_launcher;


void on_run_launch_simulation(GtkWidget* widget, gpointer data);
void on_change_launcher(GtkWidget* widget, gpointer data);
void update_launcher_dropdown();
void update_profile_dropdown();

void init_launch_analyzer(GtkBuilder *builder) {
//	int lv_id = 6;
//	struct LV lv = get_lv_from_database(lv_id);
//	struct LaunchProfiles_DB profiles = db_get_launch_profiles_from_lv_id(lv_id);
//	lv.lp_id = profiles.profile[0].profiletype;
//	for(int i = 0; i < 5; i++) lv.lp_params[i] = profiles.profile[0].lp_params[i];
//	print_LV(&lv);
//	for(int i = 0; i < profiles.num_profiles; i++) {
//		printf("%d %f %f %f %f %f %s\n",
//			   profiles.profile[i].profiletype,
//			   profiles.profile[i].lp_params[0],
//			   profiles.profile[i].lp_params[1],
//			   profiles.profile[i].lp_params[2],
//			   profiles.profile[i].lp_params[3],
//			   profiles.profile[i].lp_params[4],
//			   profiles.profile[i].usage);
//	}
//	return;
	
	num_launcher = get_all_launch_vehicles_from_database(&all_launcher, &launcher_id);
	
	cb_la_sel_profile = gtk_builder_get_object(builder, "cb_la_sel_profile");
	cb_la_sel_launcher = gtk_builder_get_object(builder, "cb_la_sel_launcher");
	update_launcher_dropdown();
	
	
	// Create a cell renderer
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cb_la_sel_launcher), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cb_la_sel_launcher), renderer, "text", 0, NULL);
	
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cb_la_sel_profile), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cb_la_sel_profile), renderer, "text", 0, NULL);
}


void update_launcher_dropdown() {
	GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	GtkTreeIter iter;
	// Add items to the list store
	for(int i = 0; i < num_launcher; i++) {
		gtk_list_store_append(store, &iter);
		char entry[30];
		sprintf(entry, "%s", all_launcher[i].name);
		gtk_list_store_set(store, &iter, 0, entry, 1, i, -1);
	}
	
	gtk_combo_box_set_model(GTK_COMBO_BOX(cb_la_sel_launcher), GTK_TREE_MODEL(store));
	gtk_combo_box_set_active(GTK_COMBO_BOX(cb_la_sel_launcher), 0);
	
	g_object_unref(store);
}

void update_profile_dropdown() {
	int id = gtk_combo_box_get_active(GTK_COMBO_BOX(cb_la_sel_launcher));
	struct LaunchProfiles_DB profiles = db_get_launch_profiles_from_lv_id(launcher_id[id]);
	
	GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	GtkTreeIter iter;
	// Add items to the list store
	for(int i = 0; i < profiles.num_profiles; i++) {
		gtk_list_store_append(store, &iter);
		char entry[50];
		switch(profiles.profile[i].profiletype) {
			case 1: sprintf(entry, "p = %.0f", profiles.profile[i].lp_params[0]);
				break;
			case 2: sprintf(entry, "p = 90*exp(-%f*h)", profiles.profile[i].lp_params[0]);
				break;
			case 3: sprintf(entry, "p = (90-%.0f)*exp(-%f*h) + %.0f",
							profiles.profile[i].lp_params[1],
							profiles.profile[i].lp_params[0],
							profiles.profile[i].lp_params[1]);
				break;
			case 4: sprintf(entry, "p4(%f, %f, %f)",
						   profiles.profile[i].lp_params[0],
						   profiles.profile[i].lp_params[2],
						   profiles.profile[i].lp_params[1]);
				break;
		}
		gtk_list_store_set(store, &iter, 0, entry, 1, i, -1);
	}
	
	gtk_combo_box_set_model(GTK_COMBO_BOX(cb_la_sel_profile), GTK_TREE_MODEL(store));
	gtk_combo_box_set_active(GTK_COMBO_BOX(cb_la_sel_profile), 0);
	
	g_object_unref(store);
}

void on_change_launcher(GtkWidget* widget, gpointer data) {
	update_profile_dropdown();
}
