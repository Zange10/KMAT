#include "mission_manager_tools.h"
#include "gui/css_loader.h"
#include "mission_db_tools.h"
#include "gui/database_app/mission_db.h"


GtkWidget *grid_mman_objectives;
GtkWidget *grid_mman_events;

GObject *vp_mman_objectives;
GObject *vp_mman_events;

GObject *lb_mman_updatenew;
GObject *tf_mman_name;
GObject *cb_mman_program;
GObject *cb_mman_missionstatus;
GObject *cb_mman_launcher;
GObject *cb_mman_objrank;
GObject *bt_mman_updatenew;
GObject *tf_mman_newprogram;
GObject *stack_newprogram;

int mission_id;


void update_mman_objective_box();


void init_mission_manager(GtkBuilder *builder) {
	vp_mman_objectives = gtk_builder_get_object(builder, "vp_mman_objectives");
	vp_mman_events = gtk_builder_get_object(builder, "vp_mman_events");
	lb_mman_updatenew = gtk_builder_get_object(builder, "lb_mman_updatenew");
	tf_mman_name = gtk_builder_get_object(builder, "tf_mman_name");
	cb_mman_program = gtk_builder_get_object(builder, "cb_mman_program");
	cb_mman_missionstatus = gtk_builder_get_object(builder, "cb_mman_missionstatus");
	cb_mman_launcher = gtk_builder_get_object(builder, "cb_mman_launcher");
	cb_mman_objrank = gtk_builder_get_object(builder, "cb_mman_objrank");
	bt_mman_updatenew = gtk_builder_get_object(builder, "bt_mman_updatenew");
	tf_mman_newprogram = gtk_builder_get_object(builder, "tf_mman_newprogram");
	stack_newprogram = gtk_builder_get_object(builder, "stack_newprogram");

	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cb_mman_program), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cb_mman_program), renderer, "text", 0, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cb_mman_launcher), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cb_mman_launcher), renderer, "text", 0, NULL);
}



void switch_to_mission_manager(struct Mission_DB mission) {
	mission_id = mission.id;
	update_mission_launcher_dropdown(GTK_COMBO_BOX(cb_mman_launcher));
	update_program_dropdown(GTK_COMBO_BOX(cb_mman_program), 0);

	if(mission_id < 0) {
		gtk_label_set_label(GTK_LABEL(lb_mman_updatenew), "NEW MISSION:");
		gtk_entry_set_text(GTK_ENTRY(tf_mman_name), "");
		gtk_button_set_label(GTK_BUTTON(bt_mman_updatenew), "Add Mission");
		set_active_combobox_id(GTK_COMBO_BOX(cb_mman_program), -1);
		set_active_combobox_id(GTK_COMBO_BOX(cb_mman_launcher), -1);
		gtk_combo_box_set_active(GTK_COMBO_BOX(cb_mman_missionstatus), 0);
	} else {
		gtk_label_set_label(GTK_LABEL(lb_mman_updatenew), "EDIT MISSION:");
		gtk_entry_set_text(GTK_ENTRY(tf_mman_name), mission.name);
		gtk_button_set_label(GTK_BUTTON(bt_mman_updatenew), "Update Mission");
		set_active_combobox_id(GTK_COMBO_BOX(cb_mman_program), mission.program_id);
		set_active_combobox_id(GTK_COMBO_BOX(cb_mman_launcher), mission.launcher_id);
		gtk_combo_box_set_active(GTK_COMBO_BOX(cb_mman_missionstatus), mission.status);
	}


	update_mman_objective_box();
}


void on_enter_new_program() {
	gtk_stack_set_visible_child_name(GTK_STACK(stack_newprogram), "page1");
}

void on_add_new_program() {
	char *program_name = (char*) gtk_entry_get_text(GTK_ENTRY(tf_mman_newprogram));
	if(program_name[0] != '\0') db_new_program(program_name, "");
	gtk_stack_set_visible_child_name(GTK_STACK(stack_newprogram), "page0");
	update_program_dropdown(GTK_COMBO_BOX(cb_mman_program), 0);
}

void on_mman_back() {
	update_db_box();
	switch_to_mission_database_page();
}

void on_newupdate_mission() {
	char *mission_name = (char*) gtk_entry_get_text(GTK_ENTRY(tf_mman_name));
	int program_id = get_active_combobox_id(GTK_COMBO_BOX(cb_mman_program));
	int launcher_id = get_active_combobox_id(GTK_COMBO_BOX(cb_mman_launcher));
	int status = gtk_combo_box_get_active(GTK_COMBO_BOX(cb_mman_missionstatus));
	if(mission_id < 0 && mission_name[0] != '\0') {
		db_new_mission(mission_name, program_id, launcher_id, status);
	} else if(mission_name[0] != '\0') {
		db_update_mission(mission_id, mission_name, program_id, launcher_id, status);
	}
	update_db_box();
	switch_to_mission_database_page();
}

void update_mman_objective_box() {
	int num_mission_cols = 3;

	// Remove grid if exists
	if (grid_mman_objectives != NULL && GTK_WIDGET(vp_mman_objectives) == gtk_widget_get_parent(grid_mman_objectives)) {
		gtk_container_remove(GTK_CONTAINER(vp_mman_objectives), grid_mman_objectives);
	}

	grid_mman_objectives = gtk_grid_new();
	GtkWidget *separator;


	separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_attach(GTK_GRID(grid_mman_objectives), separator, 0, 0, num_mission_cols*2+1, 1);
	separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_attach(GTK_GRID(grid_mman_objectives), separator, 0, 1, 1, 1);

	for (int col = 0; col < num_mission_cols; ++col) {
		char label_text[20];
		int req_width = -1;
		switch(col) {
			case 0: sprintf(label_text, "#"); req_width = 20; break;
			case 1: sprintf(label_text, "Mission"); req_width = 150; break;
			case 2: sprintf(label_text, "Program"); req_width = 120; break;
			default:sprintf(label_text, ""); req_width = 20; break;
		}

		// Create a GtkLabel
		GtkWidget *label = gtk_label_new(label_text);
		// width request
		gtk_widget_set_size_request(GTK_WIDGET(label), req_width, -1);
		// set css class
		set_css_class_for_widget(GTK_WIDGET(label), "missiondb-header");

		// Set the label in the grid at the specified row and column
		gtk_grid_attach(GTK_GRID(grid_mman_objectives), label, col*2+1, 1, 1, 1);

		// Create a horizontal separator line (optional)
		separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_grid_attach(GTK_GRID(grid_mman_objectives), separator, col*2+2, 1, 1, 1);
	}

	separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_attach(GTK_GRID(grid_mman_objectives), separator, 0, 2, num_mission_cols*2+1, 1);

	// Create labels and buttons and add them to the grid
	for (int i = 0; i < 3; ++i) {
		int row = i*2+3;

		for (int j = 0; j < num_mission_cols; ++j) {
			int col = j*2+1;
			char widget_text[30];
			switch(j) {
				case 0: sprintf(widget_text, "%d", i + 1); break;
				case 1: sprintf(widget_text, "minus"); break;
				case 2: sprintf(widget_text, "Lorem ipsum stuff"); break;
				default:sprintf(widget_text, "↓"); break;
			}

			GtkWidget *widget;

			if(j < 5) {
				// Create a Label
				widget = gtk_label_new(widget_text);
				// left-align (and right-align for id)
				gtk_label_set_xalign(GTK_LABEL(widget), (gfloat) 0.0);
				if(j == 0) gtk_label_set_xalign(GTK_LABEL(widget), (gfloat) 1.0);
			}

			// Set the label in the grid at the specified row and column
			gtk_grid_attach(GTK_GRID(grid_mman_objectives), widget, col, row, 1, 1);

			// Create a horizontal separator line (optional)
			separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
			gtk_grid_attach(GTK_GRID(grid_mman_objectives), separator, col+1, row, 1, 1);
		}
		separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_grid_attach(GTK_GRID(grid_mman_objectives), separator, 0, row+1, num_mission_cols*2+1, 1);
	}

	gtk_container_add (GTK_CONTAINER (vp_mman_objectives),
					   grid_mman_objectives);
	gtk_widget_show_all(GTK_WIDGET(vp_mman_objectives));
}