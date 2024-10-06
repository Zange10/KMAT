#include "mission_manager_tools.h"
#include "gui/css_loader.h"
#include "mission_db_tools.h"
#include "gui/database_app/mission_db.h"
#include "ephem.h"

#include <math.h>


GtkWidget *grid_mman_objectives;
GtkWidget *grid_mman_events;

GObject *vp_mman_objectives;
GObject *vp_mman_events;

GObject *lb_mman_updatenew;
GObject *tf_mman_name;
GObject *cb_mman_program;
GObject *cb_mman_missionstatus;
GObject *cb_mman_launcher;
GObject *bt_mman_updatenew;
GObject *tf_mman_newprogram;
GObject *stack_newprogram;

struct MissionObjectiveList {
	struct MissionObjective_DB *objective;
	int list_id;
	GtkWidget *cb_rank, *cb_status, *tf_objective;
};

enum MissionEventType {INITIAL_EVENT, EPOCH_EVENT, TIMESINCE_EVENT};

struct MissionEventList {
	struct MissionEvent_DB *event;
	enum MissionEventType event_type;
	int list_id;
	GtkWidget *cb_type, *tf_year, *tf_month, *tf_day, *tf_hour, *tf_min, *tf_sec, *tf_event;
};

struct MissionObjective_DB *objectives;
struct MissionObjectiveList *obj_list;
struct MissionEvent_DB *events;
struct MissionEventList *event_list;
int num_objectives, num_events, initial_event_id;

struct Mission_DB mission;


void update_mman_objective_box();
void update_mman_event_box();
void reset_mission_manager();
void on_mman_remove_objective(GtkWidget *button, gpointer data);
void on_mman_add_objective(GtkWidget *button, gpointer data);
void update_mman_objectives_list();
void on_mman_remove_event(GtkWidget *button, gpointer data);
void on_mman_add_event(GtkWidget *button, gpointer data);
void update_mman_event_list();
void update_lists_with_id_and_references();
double get_event_epoch(struct MissionEventList *event);
void on_mman_change_event_time_type(GtkWidget *combo_box, gpointer data);


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
	// Retrieve Objectives of mission
	num_objectives = db_get_objectives_from_mission_id(&objectives, mission.id);
	obj_list = malloc(num_objectives * sizeof(struct MissionObjectiveList));
	update_mission_launcher_dropdown(GTK_COMBO_BOX(cb_mman_launcher));
	update_program_dropdown(GTK_COMBO_BOX(cb_mman_program), 0);
	// Retrieve Events of Mission
	num_events = db_get_events_from_mission_id(&events, mission.id);
	event_list = malloc(num_events * sizeof(struct MissionEventList));
	update_mission_launcher_dropdown(GTK_COMBO_BOX(cb_mman_launcher));
	update_program_dropdown(GTK_COMBO_BOX(cb_mman_program), 0);

	update_lists_with_id_and_references();
	initial_event_id = -1;

	for(int i = 0; i < num_events; i++) event_list[i].event_type = EPOCH_EVENT;


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
	update_mman_event_box();
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
		mission.id = db_get_last_inserted_id();
	} else if(mission_name[0] != '\0') {
		db_update_mission(mission.id, mission_name, program_id, launcher_id, status);
	}
	update_mman_objectives_list();
	for(int i = 0; i < num_objectives; i++) {
		struct MissionObjective_DB obj = objectives[i];
		if(obj.id  > 0) db_update_objective(obj.id, mission.id, obj.status, (int) obj.rank+1, obj.objective);
		if(obj.id == 0) db_new_objective(mission.id, obj.status, (int) obj.rank+1, obj.objective);
		if(obj.id  < 0) db_remove_objective(-obj.id);
	}
	update_mman_event_list();
	for(int i = 0; i < num_events; i++) {
		struct MissionEvent_DB event = events[i];
		double epoch = get_event_epoch(&(event_list[i]));
		if(event.id  > 0) db_update_event(event.id, mission.id, epoch, event.event);
		if(event.id == 0) db_new_event(mission.id, epoch, event.event);;
		if(event.id  < 0) db_remove_event(-event.id);
	}
	reset_mission_manager();
	update_db_box();
	switch_to_mission_database_page();
}

void reset_mission_manager() {
	free(objectives);
	free(obj_list);
	free(events);
	free(event_list);
	num_objectives = 0;
	num_events = 0;
}

void update_lists_with_id_and_references() {
	for(int i = 0; i < num_objectives; i++) {
		obj_list[i].objective = &(objectives[i]);
		obj_list[i].list_id = i;
	}
	for(int i = 0; i < num_events; i++) {
		event_list[i].event = &(events[i]);
		event_list[i].list_id = i;
		if(event_list[i].event_type == INITIAL_EVENT) initial_event_id = i;
	}
}


/* ----------------------------
 * 		OBJECTIVE BOX
 * ----------------------------
 */

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
			case 2: sprintf(label_text, "Objective"); req_width = 745; break;
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
	update_lists_with_id_and_references();
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
	update_lists_with_id_and_references();
	update_mman_objective_box();
}

/* ----------------------------
 * 		EVENT BOX
 * ----------------------------
 */

void update_mman_event_box() {
	int num_cols = 9;

	// Remove grid if exists
	if (grid_mman_events != NULL && GTK_WIDGET(vp_mman_events) == gtk_widget_get_parent(grid_mman_events)) {
		gtk_container_remove(GTK_CONTAINER(vp_mman_events), grid_mman_events);
	}

	grid_mman_events = gtk_grid_new();
	GtkWidget *separator;


	separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_attach(GTK_GRID(grid_mman_events), separator, 0, 0, num_cols*2+1, 1);
	separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_attach(GTK_GRID(grid_mman_events), separator, 0, 1, 1, 1);

	for (int col = 0; col < num_cols; col++) {
		char label_text[20];
		int req_width = -1;
		switch(col) {
			case 0: sprintf(label_text, "Type"); req_width = 75; break;
			case 1: sprintf(label_text, "Epoch / T+"); req_width = 260; break;
			case 7: sprintf(label_text, "Event"); req_width = 535; break;
			default: sprintf(label_text, ""); req_width = 50; break;
		}
		// Create a GtkLabel
		GtkWidget *label = gtk_label_new(label_text);
		// width request
		gtk_widget_set_size_request(GTK_WIDGET(label), req_width, -1);
		// set css class
		set_css_class_for_widget(GTK_WIDGET(label), "missiondb-header");

		// Set the label in the grid at the specified row and column
		if(col != 1)
			gtk_grid_attach(GTK_GRID(grid_mman_events), label, col * 2 + 1, 1, 1, 1);
		else {
			gtk_grid_attach(GTK_GRID(grid_mman_events), label, col * 2 + 1, 1, 10, 1);
			col = num_cols-3;
		}

		// Create a horizontal separator line (optional)
		separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_grid_attach(GTK_GRID(grid_mman_events), separator, col * 2 + 2, 1, 1, 1);
	}

	separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_attach(GTK_GRID(grid_mman_events), separator, 0, 2, num_cols*2+1, 1);

	// Create labels and buttons and add them to the grid
	for (int i = 0; i < num_events; i++) {
		int row = i*2+3;
		struct Date date = {};
		if(event_list[i].event_type != TIMESINCE_EVENT) {
			date = convert_JD_date(events[i].epoch);
		} else {
			date = get_date_difference_from_epochs(event_list[initial_event_id].event->epoch, event_list[i].event->epoch);
		}


		for (int j = 0; j < num_cols; j++) {
			int col = j*2+1;
			char widget_text[500];
			GtkWidget *widget;
			switch(j) {
				case 0: // Type Combo box
					widget = gtk_combo_box_text_new();
					// Add options to the combo box
					gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(widget), NULL, "INIT");
					gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(widget), NULL, "EPOCH");
					gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(widget), NULL, "T+");
					gtk_combo_box_set_active(GTK_COMBO_BOX(widget), event_list[i].event_type);
					g_signal_connect(widget, "changed", G_CALLBACK(on_mman_change_event_time_type), &(event_list[i]));
					event_list[i].cb_type = widget;
					break;
				case 1: // Year Entry box
					if(event_list[i].event_type == TIMESINCE_EVENT) {
						event_list[i].tf_year = NULL;
						event_list[i].tf_month = NULL;
						j+=2; goto case_3;
					}
					widget = gtk_entry_new();
					sprintf(widget_text, "%d", date.y);
					gtk_entry_set_text(GTK_ENTRY(widget), widget_text);
					gtk_entry_set_width_chars(GTK_ENTRY(widget), 4);
					event_list[i].tf_year = widget;
					break;
				case 2: // Month Entry box
					widget = gtk_entry_new();
					sprintf(widget_text, "%02d", date.m);
					gtk_entry_set_text(GTK_ENTRY(widget), widget_text);
					gtk_entry_set_width_chars(GTK_ENTRY(widget), 2);
					event_list[i].tf_month = widget;
					break;
				case 3: // Day Entry box
				case_3:
					widget = gtk_entry_new();
					if(event_list[i].event_type != TIMESINCE_EVENT) {
						sprintf(widget_text, "%02d", date.d);
						gtk_entry_set_width_chars(GTK_ENTRY(widget), 2);
					} else {
						sprintf(widget_text, "%d", date.d);
						gtk_entry_set_width_chars(GTK_ENTRY(widget), 5);
					}
					gtk_entry_set_text(GTK_ENTRY(widget), widget_text);
					event_list[i].tf_day = widget;
					break;
				case 4: // Hour Entry box
					widget = gtk_entry_new();
					sprintf(widget_text, "%02d", date.h);
					gtk_entry_set_text(GTK_ENTRY(widget), widget_text);
					gtk_entry_set_width_chars(GTK_ENTRY(widget), 3);
					event_list[i].tf_hour = widget;
					break;
				case 5: // Minute Entry box
					widget = gtk_entry_new();
					sprintf(widget_text, "%02d", date.min);
					gtk_entry_set_text(GTK_ENTRY(widget), widget_text);
					gtk_entry_set_width_chars(GTK_ENTRY(widget), 3);
					event_list[i].tf_min = widget;
					break;
				case 6: // Second Entry box
					widget = gtk_entry_new();
					sprintf(widget_text, "%02.0f", date.s);
					gtk_entry_set_text(GTK_ENTRY(widget), widget_text);
					gtk_entry_set_width_chars(GTK_ENTRY(widget), 3);
					event_list[i].tf_sec = widget;
					break;
				case 7:
					widget = gtk_entry_new();
					sprintf(widget_text, "%s", events[i].event);
					gtk_entry_set_text(GTK_ENTRY(widget), widget_text);
					event_list[i].tf_event = widget;
					break;
				case 8:
					widget = gtk_button_new_with_label("-");
					g_signal_connect(widget, "clicked", G_CALLBACK(on_mman_remove_event), &(event_list[i]));
					break;
				default:
					sprintf(widget_text, "");
					widget = gtk_label_new(widget_text);
					break;
			}

			if(j > 0 && j < 7)
				gtk_entry_set_alignment(GTK_ENTRY(widget), (gfloat) 1.0);

			if(j>0) {
				if(events[i].id == 0) set_css_class_for_widget(widget, "mman-added");
				if(events[i].id <  0) set_css_class_for_widget(widget, "mman-removed");
				if(events[i].id >  0) set_css_class_for_widget(widget, "mman-norm");
			}


			// Set the label in the grid at the specified row and column
			if(j == 3 && event_list[i].event_type == TIMESINCE_EVENT) {
				gtk_grid_attach(GTK_GRID(grid_mman_events), widget, col, row, 5, 1);
				col = j*2+1;
			} else gtk_grid_attach(GTK_GRID(grid_mman_events), widget, col, row, 1, 1);


			// Create a horizontal separator line (optional)
			if(j == 0 || j >= 6) {
				separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
				gtk_grid_attach(GTK_GRID(grid_mman_events), separator, col + 1, row, 1, 1);
			} else {
				switch(j) {
					case 1: // y-m
					case 2: // m-d
						widget = gtk_label_new("-"); break;
					case 3: // dTh
						widget = gtk_label_new("T"); break;
					case 4: // h:min
					case 5: // min:sec
						widget = gtk_label_new(":"); break;
					default:
						widget = gtk_label_new("");
						break;
				}
				if(events[i].id == 0) set_css_class_for_widget(widget, "mman-added");
				if(events[i].id <  0) set_css_class_for_widget(widget, "mman-removed");
				if(events[i].id >  0) set_css_class_for_widget(widget, "mman-norm");
				gtk_grid_attach(GTK_GRID(grid_mman_events), widget, col + 1, row, 1, 1);
			}
		}
		separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_grid_attach(GTK_GRID(grid_mman_events), separator, 0, row+1, num_cols*2+1, 1);
	}
	int row = num_events*2+3;

	GtkWidget *add_event_button = gtk_button_new_with_label("+");
	g_signal_connect(add_event_button, "clicked", G_CALLBACK(on_mman_add_event), NULL);
	// Set the label in the grid at the specified row and column
	gtk_grid_attach(GTK_GRID(grid_mman_events), add_event_button, 0, row, num_cols*2+1, 1);


	gtk_container_add (GTK_CONTAINER (vp_mman_events),
					   grid_mman_events);
	gtk_widget_show_all(GTK_WIDGET(vp_mman_events));
}

void update_mman_event_list() {
	for(int i = 0; i < num_events; i++) {
		struct MissionEvent_DB *p_event = event_list[i].event;
		p_event->epoch = get_event_epoch(&(event_list[i]));
		sprintf(p_event->event, "%s", gtk_entry_get_text(GTK_ENTRY(event_list[i].tf_event)));
	}
}

void on_mman_add_event(GtkWidget *button, gpointer data) {
	update_mman_event_list();
	num_events++;

	// increase arrays by 1
	struct MissionEventList *new_event_list = realloc(event_list, num_events*sizeof(struct MissionEventList));
	if(new_event_list == NULL) { printf("Memory reallocation failed!\n"); exit(1); }
	event_list = new_event_list;
	struct MissionEvent_DB *new_events = realloc(events, num_events*sizeof(struct MissionEvent_DB));
	if(new_events == NULL) { printf("Memory reallocation failed!\n"); exit(1); }
	events = new_events;
	// Initialize the new objective
	struct Date date = {1950, 01, 01};
	events[num_events-1].id = 0;
	if(initial_event_id < 0)
		events[num_events-1].epoch = convert_date_JD(date);
	else
		events[num_events-1].epoch = events[initial_event_id].epoch;
	events[num_events-1].mission_id = mission.id;
	sprintf(events[num_events-1].event, "<EVENT>");
	event_list[num_events-1].event_type = EPOCH_EVENT;
	update_lists_with_id_and_references();
	update_mman_event_box();
}

void on_mman_remove_event(GtkWidget *button, gpointer data) {
	update_mman_event_list();
	struct MissionEventList *event = (struct MissionEventList *) data;  // Cast data back to int
	if(event->event->id != 0) event->event->id *= -1;
	else {
		if(event->event_type == INITIAL_EVENT) {
			initial_event_id = -1;
			for(int i = 0; i < num_events; i++) event_list[i].event_type = EPOCH_EVENT;
		}
		// remove previously added objective from list and objectives
		if(event->list_id < num_events-1){
			for(int i = event->list_id; i < num_events-1; i++) {
				events[i].id = events[i+1].id;
				events[i].epoch = events[i+1].epoch;
				sprintf(events[i].event, "%s", events[i+1].event);
				event_list[i].event_type= event_list[i+1].event_type;
				event_list[i].cb_type 	= event_list[i+1].cb_type;
				event_list[i].tf_year 	= event_list[i+1].tf_year;
				event_list[i].tf_month 	= event_list[i+1].tf_month;
				event_list[i].tf_day 	= event_list[i+1].tf_day;
				event_list[i].tf_hour 	= event_list[i+1].tf_hour;
				event_list[i].tf_min 	= event_list[i+1].tf_min;
				event_list[i].tf_sec 	= event_list[i+1].tf_sec;
				event_list[i].tf_event 	= event_list[i+1].tf_event;
			}
		};
		num_events--;
	}
	update_lists_with_id_and_references();
	update_mman_event_box();
}

void on_mman_change_event_time_type(GtkWidget *combo_box, gpointer data) {
	update_mman_event_list();
	struct MissionEventList *event = (struct MissionEventList *) data;

	int new_event_type = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_box));

	switch(new_event_type) {
		case INITIAL_EVENT:
			if(initial_event_id < 0) {
				initial_event_id = event->list_id;
				event->event_type = INITIAL_EVENT;
			}
			break;
		case EPOCH_EVENT:
			if(event->event_type == INITIAL_EVENT) {
				initial_event_id = -1;
				for(int i = 0; i < num_events; i++) event_list[i].event_type = EPOCH_EVENT;
			}
			event->event_type = EPOCH_EVENT;
			break;
		case TIMESINCE_EVENT:
			if(initial_event_id < 0 || initial_event_id == event->list_id) break;
			event->event_type = TIMESINCE_EVENT;
			break;
		default: break;
	}

	update_mman_event_box();
}

double get_event_epoch(struct MissionEventList *event) {
	char *endptr;
	if(event->event_type != TIMESINCE_EVENT) {
		struct Date date = {
				.y   = (int) strtol(gtk_entry_get_text(GTK_ENTRY(event->tf_year)), &endptr, 10),
				.m   = (int) strtol(gtk_entry_get_text(GTK_ENTRY(event->tf_month)), &endptr, 10),
				.d   = (int) strtol(gtk_entry_get_text(GTK_ENTRY(event->tf_day)), &endptr, 10),
				.h   = (int) strtol(gtk_entry_get_text(GTK_ENTRY(event->tf_hour)), &endptr, 10),
				.min = (int) strtol(gtk_entry_get_text(GTK_ENTRY(event->tf_min)), &endptr, 10),
				.s   = (double) strtol(gtk_entry_get_text(GTK_ENTRY(event->tf_sec)), &endptr, 10)
		};
		return convert_date_JD(date);
	} else {
		struct Date date_diff = {
				.d   = (int) strtol(gtk_entry_get_text(GTK_ENTRY(event->tf_day)), &endptr, 10),
				.h   = (int) strtol(gtk_entry_get_text(GTK_ENTRY(event->tf_hour)), &endptr, 10),
				.min = (int) strtol(gtk_entry_get_text(GTK_ENTRY(event->tf_min)), &endptr, 10),
				.s   = (double) strtol(gtk_entry_get_text(GTK_ENTRY(event->tf_sec)), &endptr, 10)
		};
		double initial_epoch = event_list[initial_event_id].event->epoch;
		double epoch_diff = (date_diff.d) + ((double)date_diff.h/24) + ((double)date_diff.min/(24*60)) + ((double)date_diff.s/(24*60*60));
		return initial_epoch + epoch_diff;
	}
}
