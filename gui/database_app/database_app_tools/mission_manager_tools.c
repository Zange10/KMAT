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

struct MissionObjectiveList {
	struct MissionObjective_DB *objective;
	int list_id;
	GtkWidget *cb_rank, *cb_status, *tf_objective;
};

struct MissionObjective_DB *objectives;
struct MissionObjectiveList *obj_list;
int num_objectives;

struct Mission_DB mission;


void update_mman_objective_box();
void reset_mission_manager();
void on_mman_remove_objective(GtkWidget *button, gpointer data);
void on_mman_add_objective(GtkWidget *button, gpointer data);
void update_objective_list_with_id_and_objpointer();
void update_mman_objectives_list();


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



void switch_to_mission_manager(struct Mission_DB m) {
	mission = m;
	num_objectives = db_get_objectives_from_mission_id(&objectives, mission.id);
	obj_list = (struct MissionObjectiveList *) malloc(num_objectives * sizeof(struct MissionObjectiveList));
	update_objective_list_with_id_and_objpointer();
	update_mission_launcher_dropdown(GTK_COMBO_BOX(cb_mman_launcher));
	update_program_dropdown(GTK_COMBO_BOX(cb_mman_program), 0);

	if(mission.id < 0) {
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
	reset_mission_manager();
	update_db_box();
	switch_to_mission_database_page();
}

void on_newupdate_mission() {
	char *mission_name = (char*) gtk_entry_get_text(GTK_ENTRY(tf_mman_name));
	int program_id = get_active_combobox_id(GTK_COMBO_BOX(cb_mman_program));
	int launcher_id = get_active_combobox_id(GTK_COMBO_BOX(cb_mman_launcher));
	int status = gtk_combo_box_get_active(GTK_COMBO_BOX(cb_mman_missionstatus));
	if(mission.id < 0 && mission_name[0] != '\0') {
		db_new_mission(mission_name, program_id, launcher_id, status);
	} else if(mission_name[0] != '\0') {
		db_update_mission(mission.id, mission_name, program_id, launcher_id, status);
	}
	mission.id = db_get_last_inserted_id();
	update_mman_objectives_list();
	for(int i = 0; i < num_objectives; i++) {
		struct MissionObjective_DB obj = objectives[i];
		if(objectives[i].id  > 0) db_update_objective(obj.id, mission.id, obj.status, (int) obj.rank+1, obj.objective);
		if(objectives[i].id == 0) db_new_objective(mission.id, obj.status, (int) obj.rank+1, obj.objective);
		if(objectives[i].id  < 0) db_remove_objective(-obj.id);
	}
	reset_mission_manager();
	update_db_box();
	switch_to_mission_database_page();
}

void update_mman_objective_box() {
	int num_cols = 4;

	// Remove grid if exists
	if (grid_mman_objectives != NULL && GTK_WIDGET(vp_mman_objectives) == gtk_widget_get_parent(grid_mman_objectives)) {
		gtk_container_remove(GTK_CONTAINER(vp_mman_objectives), grid_mman_objectives);
	}

	grid_mman_objectives = gtk_grid_new();
	GtkWidget *separator;


	separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_attach(GTK_GRID(grid_mman_objectives), separator, 0, 0, num_cols*2+1, 1);
	separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_attach(GTK_GRID(grid_mman_objectives), separator, 0, 1, 1, 1);

	for (int col = 0; col < num_cols; col++) {
		char label_text[20];
		int req_width = -1;
		switch(col) {
			case 0: sprintf(label_text, "Rank"); req_width = 50; break;
			case 1: sprintf(label_text, "Status"); req_width = 100; break;
			case 2: sprintf(label_text, "Objective"); req_width = 750; break;
			default:sprintf(label_text, ""); req_width = 50; break;
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
	gtk_grid_attach(GTK_GRID(grid_mman_objectives), separator, 0, 2, num_cols*2+1, 1);

	// Create labels and buttons and add them to the grid
	for (int i = 0; i < num_objectives; i++) {
		int row = i*2+3;

		for (int j = 0; j < num_cols; j++) {
			int col = j*2+1;
			char widget_text[500];
			GtkWidget *widget;
			switch(j) {
				case 0:
					// Create a combo box with text entries
					widget = gtk_combo_box_text_new();
					// Add options to the combo box
					gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(widget), NULL, "PRIM");
					gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(widget), NULL, "SEC");
					gtk_combo_box_set_active(GTK_COMBO_BOX(widget), objectives[i].rank == OBJ_PRIMARY ? 0 : 1);
					obj_list[i].cb_rank = widget;
					break;
				case 1:
					// Create a combo box with text entries
					widget = gtk_combo_box_text_new();
					// Add options to the combo box
					gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(widget), NULL, "TBD");
					gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(widget), NULL, "SUCCESS");
					gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(widget), NULL, "FAIL");
					gtk_combo_box_set_active(GTK_COMBO_BOX(widget), objectives[i].status == OBJ_TBD ? 0 : objectives[i].status == OBJ_SUCCESS ? 1:2);
					obj_list[i].cb_status = widget;
					break;
				case 2:
					sprintf(widget_text, "%s", objectives[i].objective);
					widget = gtk_entry_new();
					gtk_entry_set_text(GTK_ENTRY(widget), objectives[i].objective);
					obj_list[i].tf_objective = widget;
					break;
				case 3:
					widget = gtk_button_new_with_label("-");
					g_signal_connect(widget, "clicked", G_CALLBACK(on_mman_remove_objective), &(obj_list[i]));
					break;
				default:
					sprintf(widget_text, "");
					widget = gtk_label_new(widget_text);
					break;
			}

			if(j>1) {
				if(objectives[i].id == 0) set_css_class_for_widget(widget, "mman-added");
				if(objectives[i].id <  0) set_css_class_for_widget(widget, "mman-removed");
				if(objectives[i].id >  0) set_css_class_for_widget(widget, "mman-norm");
			}


			// Set the label in the grid at the specified row and column
			gtk_grid_attach(GTK_GRID(grid_mman_objectives), widget, col, row, 1, 1);

			// Create a horizontal separator line (optional)
			separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
			gtk_grid_attach(GTK_GRID(grid_mman_objectives), separator, col+1, row, 1, 1);
		}
		separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_grid_attach(GTK_GRID(grid_mman_objectives), separator, 0, row+1, num_cols*2+1, 1);
	}
	int row = num_objectives*2+3;

	GtkWidget *add_obj_button = gtk_button_new_with_label("+");
	g_signal_connect(add_obj_button, "clicked", G_CALLBACK(on_mman_add_objective), NULL);
	// Set the label in the grid at the specified row and column
	gtk_grid_attach(GTK_GRID(grid_mman_objectives), add_obj_button, 0, row, num_cols*2+1, 1);


	gtk_container_add (GTK_CONTAINER (vp_mman_objectives),
					   grid_mman_objectives);
	gtk_widget_show_all(GTK_WIDGET(vp_mman_objectives));
}

void update_mman_objectives_list() {
	for(int i = 0; i < num_objectives; i++) {
		struct MissionObjective_DB *p_obj = obj_list[i].objective;
		p_obj->rank = gtk_combo_box_get_active(GTK_COMBO_BOX(obj_list[i].cb_rank));
		p_obj->status = gtk_combo_box_get_active(GTK_COMBO_BOX(obj_list[i].cb_status));
		sprintf(p_obj->objective, "%s", gtk_entry_get_text(GTK_ENTRY(obj_list[i].tf_objective)));
	}
}

void on_mman_add_objective(GtkWidget *button, gpointer data) {
	update_mman_objectives_list();
	num_objectives++;

	// increase arrays by 1
	struct MissionObjectiveList *new_obj_list = realloc(obj_list, num_objectives*sizeof(struct MissionObjectiveList));
	if(new_obj_list == NULL) { printf("Memory reallocation failed!\n"); exit(1); }
	obj_list = new_obj_list;
	struct MissionObjective_DB *new_obj = realloc(objectives, num_objectives*sizeof(struct MissionObjective_DB));
	if(new_obj == NULL) { printf("Memory reallocation failed!\n"); exit(1); }
	objectives = new_obj;

	// Initialize the new objective
	objectives[num_objectives-1].id = 0;
	objectives[num_objectives-1].status = OBJ_TBD;
	objectives[num_objectives-1].rank = OBJ_PRIMARY;
	objectives[num_objectives-1].mission_id = mission.id;
	sprintf(objectives[num_objectives-1].objective, "<OBJECTIVE>");
	update_objective_list_with_id_and_objpointer();
	update_mman_objective_box();
}

void on_mman_remove_objective(GtkWidget *button, gpointer data) {
	update_mman_objectives_list();
	struct MissionObjectiveList *objective = (struct MissionObjectiveList *) data;  // Cast data back to int
	if(objective->objective->id != 0) objective->objective->id *= -1;
	else {
		// remove previously added objective from list and objectives
		if(objective->list_id < num_objectives-1){
			for(int i = objective->list_id; i < num_objectives-1; i++) {
				objectives[i].status = objectives[i+1].status;
				objectives[i].id = objectives[i+1].id;
				objectives[i].rank = objectives[i+1].rank;
				objectives[i].mission_id = objectives[i+1].mission_id;
				sprintf(objectives[i].objective, "%s", objectives[i+1].objective);
				obj_list[i].objective = obj_list[i+1].objective;
				obj_list[i].cb_rank = obj_list[i+1].cb_rank;
				obj_list[i].cb_status = obj_list[i+1].cb_status;
				obj_list[i].tf_objective = obj_list[i+1].tf_objective;
			}
		};
		num_objectives--;
	}
	update_objective_list_with_id_and_objpointer();
	update_mman_objective_box();
}

void update_objective_list_with_id_and_objpointer() {
	for(int i = 0; i < num_objectives; i++) {
		obj_list[i].objective = &(objectives[i]);
		obj_list[i].list_id = i;
	}
}

void reset_mission_manager() {
	free(objectives);
	free(obj_list);
	num_objectives = 0;
}
