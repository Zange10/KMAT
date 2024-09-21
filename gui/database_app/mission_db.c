#include "mission_db.h"
#include "database/mission_database.h"
#include "database/lv_database.h"
#include "gui/css_loader.h"
#include "launch_calculator/lv_profile.h"
#include "gui/database_app/database_app_tools/mission_db_tools.h"


GObject *mission_vp;
GObject *tf_mdbfilt_name;
GObject *cb_mdbfilt_program;
GObject *cb_mdbfilt_ytf;
GObject *cb_mdbfilt_inprog;
GObject *cb_mdbfilt_ended;
GObject *stack_missiondb;
GtkWidget *mission_grid;


GObject *lb_mman_updatenew;
GObject *tf_mman_name;
GObject *cb_mman_program;
GObject *cb_mman_missionstatus;
GObject *cb_mman_launcher;
GObject *cb_mman_objrank;
GObject *bt_mman_updatenew;
GObject *tf_mman_newprogram;
GObject *stack_newprogram;

int mission_id_newupdate;
struct Mission_DB *missions;
int num_missions;


void update_db_box();
void update_program_dropdown();
void update_mman_launcher_dropdown();
void on_showhide_mission_objectives(GtkWidget *button, gpointer data);
void on_showhide_mission_events(GtkWidget *button, gpointer data);
void on_edit_mission(GtkWidget *button, gpointer data);


void init_mission_db(GtkBuilder *builder) {
	mission_vp = gtk_builder_get_object(builder, "vp_missiondb");
	tf_mdbfilt_name = gtk_builder_get_object(builder, "tf_mdbfilt_name");
	cb_mdbfilt_program = gtk_builder_get_object(builder, "cb_mdbfilt_program");
	cb_mdbfilt_ytf = gtk_builder_get_object(builder, "cb_mdbfilt_ytf");
	cb_mdbfilt_inprog = gtk_builder_get_object(builder, "cb_mdbfilt_inprog");
	cb_mdbfilt_ended = gtk_builder_get_object(builder, "cb_mdbfilt_ended");
	stack_missiondb = gtk_builder_get_object(builder, "stack_missiondb");

	lb_mman_updatenew = gtk_builder_get_object(builder, "lb_mman_updatenew");
	tf_mman_name = gtk_builder_get_object(builder, "tf_mman_name");
	cb_mman_program = gtk_builder_get_object(builder, "cb_mman_program");
	cb_mman_missionstatus = gtk_builder_get_object(builder, "cb_mman_missionstatus");
	cb_mman_launcher = gtk_builder_get_object(builder, "cb_mman_launcher");
	cb_mman_objrank = gtk_builder_get_object(builder, "cb_mman_objrank");
	bt_mman_updatenew = gtk_builder_get_object(builder, "bt_mman_updatenew");
	tf_mman_newprogram = gtk_builder_get_object(builder, "tf_mman_newprogram");
	stack_newprogram = gtk_builder_get_object(builder, "stack_newprogram");

	// Create a cell renderer for dropdowns/ComboBox
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cb_mdbfilt_program), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cb_mdbfilt_program), renderer, "text", 0, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cb_mman_program), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cb_mman_program), renderer, "text", 0, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cb_mman_launcher), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cb_mman_launcher), renderer, "text", 0, NULL);

	update_program_dropdown();
	update_db_box();
}




void update_db_box() {
	int num_mission_cols = 8;

	// Remove grid if exists
	if (mission_grid != NULL && GTK_WIDGET(mission_vp) == gtk_widget_get_parent(mission_grid)) {
		gtk_container_remove(GTK_CONTAINER(mission_vp), mission_grid);
	}

	mission_grid = gtk_grid_new();
	GtkWidget *separator;

	struct MissionProgram_DB *programs;
	int num_programs = db_get_all_programs(&programs);

	struct Mission_Filter filter;
	sprintf(filter.name, "%s", (char*) gtk_entry_get_text(GTK_ENTRY(tf_mdbfilt_name)));
	filter.program_id = get_active_combobox_id(GTK_COMBO_BOX(cb_mdbfilt_program));
	filter.ytf 		= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_mdbfilt_ytf));
	filter.in_prog 	= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_mdbfilt_inprog));
	filter.ended	= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_mdbfilt_ended));
	if(missions != NULL) free(missions);
	num_missions = db_get_missions_ordered_by_launch_date(&missions, filter);


	separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_attach(GTK_GRID(mission_grid), separator, 0, 0, num_mission_cols*2+1, 1);
	separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_attach(GTK_GRID(mission_grid), separator, 0, 1, 1, 1);

	for (int col = 0; col < num_mission_cols; ++col) {
		char label_text[20];
		int req_width = -1;
		switch(col) {
			case 1: sprintf(label_text, "Mission"); req_width = 150; break;
			case 2: sprintf(label_text, "Program"); req_width = 120; break;
			case 3: sprintf(label_text, "Status"); req_width = 120; break;
			case 4: sprintf(label_text, "Vehicle"); req_width = 150; break;
			case 5: sprintf(label_text, "Objectives"); req_width = 100; break;
			case 6: sprintf(label_text, "Events"); req_width = 100; break;
			case 7: sprintf(label_text, "Edit"); req_width = 50; break;
			default:sprintf(label_text, "#"); req_width = 20; break;
		}

		// Create a GtkLabel
		GtkWidget *label = gtk_label_new(label_text);
		// width request
		gtk_widget_set_size_request(GTK_WIDGET(label), req_width, -1);
		// set css class
		set_css_class_for_widget(GTK_WIDGET(label), "missiondb-header");

		// Set the label in the grid at the specified row and column
		gtk_grid_attach(GTK_GRID(mission_grid), label, col*2+1, 1, 1, 1);

		// Create a horizontal separator line (optional)
		separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_grid_attach(GTK_GRID(mission_grid), separator, col*2+2, 1, 1, 1);
	}

	separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_attach(GTK_GRID(mission_grid), separator, 0, 2, num_mission_cols*2+1, 1);

	struct LauncherInfo_DB lv;
	struct PlaneInfo_DB fv;

	// Create labels and buttons and add them to the grid
	for (int i = 0; i < num_missions; ++i) {
		struct Mission_DB m = missions[i];
		int row = i*2+3;

		for (int j = 0; j < num_mission_cols; ++j) {
			int col = j*2+1;
			char widget_text[30];
			if(m.launcher_id != 0) lv = db_get_launcherInfo_from_id(m.launcher_id);
			else if(m.plane_id != 0) fv = db_get_plane_from_id(m.plane_id);
			switch(j) {
				case 0: sprintf(widget_text, "%d", i + 1); break;
				case 1: sprintf(widget_text, "%s", m.name); break;
				case 2: sprintf(widget_text, "%s", get_program_from_id(programs, num_programs, m.program_id).name); break;
				case 3: sprintf(widget_text, "%s", (m.status == YET_TO_FLY ? "YET TO FLY" : (m.status == IN_PROGRESS ? "IN PROGRESS" : "ENDED"))); break;
				case 4:
					if(m.launcher_id != 0) sprintf(widget_text, "%s", lv.name);
					else if(m.plane_id != 0) sprintf(widget_text, "%s", fv.name);
					else sprintf(widget_text, "/"); break;
				case 7: sprintf(widget_text, "x"); break;
				default:sprintf(widget_text, "↓"); break;
			}

			GtkWidget *widget;

			if(j < 5) {
				// Create a Label
				widget = gtk_label_new(widget_text);
				// left-align (and right-align for id)
				gtk_label_set_xalign(GTK_LABEL(widget), (gfloat) 0.0);
				if(j == 0) gtk_label_set_xalign(GTK_LABEL(widget), (gfloat) 1.0);
			} else {
				// Create a Button
				widget = gtk_button_new_with_label(widget_text);
				// Connect the clicked signal to the callback function, passing `&data` as user_data
				switch(j) {
					case 5: g_signal_connect(widget, "clicked", G_CALLBACK(on_showhide_mission_objectives), &(missions[i].id)); break;
					case 6: g_signal_connect(widget, "clicked", G_CALLBACK(on_showhide_mission_events), &(missions[i].id)); break;
					case 7: g_signal_connect(widget, "clicked", G_CALLBACK(on_edit_mission), &(missions[i].id)); break;
					default:break;
				}

			}

			// set css class
			set_gtk_widget_class_by_mission_success(widget, m);


			// Set the label in the grid at the specified row and column
			gtk_grid_attach(GTK_GRID(mission_grid), widget, col, row, 1, 1);

			// Create a horizontal separator line (optional)
			separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
			gtk_grid_attach(GTK_GRID(mission_grid), separator, col+1, row, 1, 1);
		}
		separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_grid_attach(GTK_GRID(mission_grid), separator, 0, row+1, num_mission_cols*2+1, 1);
	}

	gtk_container_add (GTK_CONTAINER (mission_vp),
					   mission_grid);
	gtk_widget_show_all(GTK_WIDGET(mission_vp));

	free(programs);
}

void update_program_dropdown() {
	GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	GtkTreeIter iter;

	struct MissionProgram_DB *programs;
	int num_programs = db_get_all_programs(&programs);

	for(int c = 0; c < 2; c++) {
		store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
		if(c == 0) gtk_list_store_append(store, &iter);
		if(c == 0) gtk_list_store_set(store, &iter, 0, "ALL", 1, 0, -1);
		// Add items to the list store
		char entry[50];
		for(int i = 0; i < num_programs; i++) {
			gtk_list_store_append(store, &iter);
			sprintf(entry, "%s", programs[i].name);
			gtk_list_store_set(store, &iter, 0, entry, 1, programs[i].id, -1);
		}

		GtkComboBox *combo_box = c==0 ? GTK_COMBO_BOX(cb_mdbfilt_program) : GTK_COMBO_BOX(cb_mman_program);

		gtk_combo_box_set_model(combo_box, GTK_TREE_MODEL(store));
		gtk_combo_box_set_active(combo_box, c==0 ? 0 : num_programs-1);
	}

	g_object_unref(store);
	free(programs);
}

void switch_to_mission_manager(int mission_id) {
	mission_id_newupdate = mission_id;
	update_mman_launcher_dropdown();

	if(mission_id < 0) {
		gtk_label_set_label(GTK_LABEL(lb_mman_updatenew), "NEW MISSION:");
		gtk_entry_set_text(GTK_ENTRY(tf_mman_name), "");
		gtk_button_set_label(GTK_BUTTON(bt_mman_updatenew), "Add Mission");
		set_active_combobox_id(GTK_COMBO_BOX(cb_mman_program), -1);
		set_active_combobox_id(GTK_COMBO_BOX(cb_mman_launcher), -1);
		gtk_combo_box_set_active(GTK_COMBO_BOX(cb_mman_missionstatus), 0);
	} else {
		struct Mission_DB mission = get_mission_from_id(missions, num_missions, mission_id);
		gtk_label_set_label(GTK_LABEL(lb_mman_updatenew), "EDIT MISSION:");
		gtk_entry_set_text(GTK_ENTRY(tf_mman_name), mission.name);
		gtk_button_set_label(GTK_BUTTON(bt_mman_updatenew), "Update Mission");
		set_active_combobox_id(GTK_COMBO_BOX(cb_mman_program), mission.program_id);
		set_active_combobox_id(GTK_COMBO_BOX(cb_mman_launcher), mission.launcher_id);
		gtk_combo_box_set_active(GTK_COMBO_BOX(cb_mman_missionstatus), mission.status);
	}


	gtk_stack_set_visible_child_name(GTK_STACK(stack_missiondb), "page1");
}

void update_mman_launcher_dropdown() {
	struct LV *all_launcher;
	int *launcher_ids;
	int num_launcher = get_all_launch_vehicles_from_database(&all_launcher, &launcher_ids);

	GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	GtkTreeIter iter;
	// Add items to the list store
	for(int i = 0; i < num_launcher; i++) {
		gtk_list_store_append(store, &iter);
		char entry[30];
		sprintf(entry, "%s", all_launcher[i].name);
		gtk_list_store_set(store, &iter, 0, entry, 1, launcher_ids[i], -1);
	}

	gtk_combo_box_set_model(GTK_COMBO_BOX(cb_mman_launcher), GTK_TREE_MODEL(store));
	gtk_combo_box_set_active(GTK_COMBO_BOX(cb_mman_launcher), 0);

	g_object_unref(store);
}

void on_show_missions(GtkWidget* widget, gpointer data) {
	update_db_box();
}

void on_reset_mission_filter(GtkWidget* widget, gpointer data) {
	gtk_entry_set_text(GTK_ENTRY(tf_mdbfilt_name), "");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_mdbfilt_ended), 1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_mdbfilt_inprog), 1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_mdbfilt_ytf), 1);
	gtk_combo_box_set_active(GTK_COMBO_BOX(cb_mdbfilt_program), 0);
	update_db_box();
}

void on_new_mission() {
	switch_to_mission_manager(-1);
}

void on_enter_new_program() {
	gtk_stack_set_visible_child_name(GTK_STACK(stack_newprogram), "page1");
}

void on_add_new_program() {
	char *program_name = (char*) gtk_entry_get_text(GTK_ENTRY(tf_mman_newprogram));
	if(program_name[0] != '\0') db_new_program(program_name, "");
	gtk_stack_set_visible_child_name(GTK_STACK(stack_newprogram), "page0");
	update_program_dropdown();
}

void on_mman_back() {
	gtk_stack_set_visible_child_name(GTK_STACK(stack_missiondb), "page0");
	update_db_box();
}

void on_newupdate_mission() {
	char *mission_name = (char*) gtk_entry_get_text(GTK_ENTRY(tf_mman_name));
	int program_id = get_active_combobox_id(GTK_COMBO_BOX(cb_mman_program));
	int launcher_id = get_active_combobox_id(GTK_COMBO_BOX(cb_mman_launcher));
	int status = gtk_combo_box_get_active(GTK_COMBO_BOX(cb_mman_missionstatus));
	if(mission_id_newupdate < 0 && mission_name[0] != '\0') {
		db_new_mission(mission_name, program_id, launcher_id, status);
	} else if(mission_name[0] != '\0') {
		db_update_mission(mission_id_newupdate, mission_name, program_id, launcher_id, status);
	}
	gtk_stack_set_visible_child_name(GTK_STACK(stack_missiondb), "page0");
	update_db_box();
}

void on_showhide_mission_objectives(GtkWidget *button, gpointer data) {
	int mission_id = *((int *)data);  // Cast data back to int
	g_print("Objective of mission %d\n", mission_id);
}

void on_showhide_mission_events(GtkWidget *button, gpointer data) {
	int mission_id = *((int *) data);  // Cast data back to int
	g_print("Events of mission %d\n", mission_id);
}

void on_edit_mission(GtkWidget *button, gpointer data) {
	int mission_id = *((int *) data);  // Cast data back to int
	switch_to_mission_manager(mission_id);
}

void close_mission_db() {
	if(missions != NULL) free(missions);
	missions = NULL;
}
