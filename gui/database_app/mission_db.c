#include "mission_db.h"
#include "database/mission_database.h"
#include "database/lv_database.h"
#include "gui/css_loader.h"
#include "gui/database_app/database_app_tools/mission_db_tools.h"
#include "gui/database_app/database_app_tools/mission_manager_tools.h"


GObject *mission_vp;
GObject *tf_mdbfilt_name;
GObject *cb_mdbfilt_program;
GObject *cb_mdbfilt_ytf;
GObject *cb_mdbfilt_inprog;
GObject *cb_mdbfilt_ended;
GtkWidget *mission_grid;

struct Mission_DB *missions;
int num_missions;
int *show_mission_objectives, *show_mission_events, num_show_mission_objectives, num_show_mission_events;


void on_showhide_mission_objectives(GtkWidget *button, gpointer data);
void on_showhide_mission_events(GtkWidget *button, gpointer data);
void on_edit_mission(GtkWidget *button, gpointer data);
int is_mission_id_on_objective_show_list(int mission_id);
int is_mission_id_on_event_show_list(int mission_id);


void init_mission_db(GtkBuilder *builder) {
	num_show_mission_objectives = 0;
	num_show_mission_events = 0;
	mission_vp = gtk_builder_get_object(builder, "vp_missiondb");
	tf_mdbfilt_name = gtk_builder_get_object(builder, "tf_mdbfilt_name");
	cb_mdbfilt_program = gtk_builder_get_object(builder, "cb_mdbfilt_program");
	cb_mdbfilt_ytf = gtk_builder_get_object(builder, "cb_mdbfilt_ytf");
	cb_mdbfilt_inprog = gtk_builder_get_object(builder, "cb_mdbfilt_inprog");
	cb_mdbfilt_ended = gtk_builder_get_object(builder, "cb_mdbfilt_ended");

	// initialize GUI for mission manager
	init_mission_manager(builder);
	init_mission_db_tools(builder);

	// Create a cell renderer for dropdowns/ComboBox
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cb_mdbfilt_program), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cb_mdbfilt_program), renderer, "text", 0, NULL);

	update_program_dropdown(GTK_COMBO_BOX(cb_mdbfilt_program), 1);
	update_db_box();
}




void update_db_box() {
	int num_mission_cols = 8;

	// Remove grid if exists (POSSIBLE MEMORY LEAD WITH GRID AND CHILDREN?)
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
		int req_width;
		switch(col) {
			case 1: sprintf(label_text, "Mission"); req_width = 150; break;
			case 2: sprintf(label_text, "Program"); req_width = 120; break;
			case 3: sprintf(label_text, "Status"); req_width = 120; break;
			case 4: sprintf(label_text, "Vehicle"); req_width = 150; break;
			case 5: sprintf(label_text, "Objectives"); req_width = 100; break;
			case 6: sprintf(label_text, "Events"); req_width = 100; break;
			case 7: sprintf(label_text, "Edit"); req_width = 50; break;
			default:sprintf(label_text, "#"); req_width = 30; break;
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


	int added_objectives = 0, added_events = 0;
	// Create labels and buttons and add them to the grid
	for (int i = 0; i < num_missions; ++i) {
		struct Mission_DB m = missions[i];
		int row = i*2+3 + 2*added_objectives + 2*added_events;

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
				case 5: sprintf(widget_text, "%s", is_mission_id_on_objective_show_list(m.id) ? "↑" : "↓"); break;
				case 6: sprintf(widget_text, "%s", is_mission_id_on_event_show_list(m.id) ? "↑" : "↓"); break;
				case 7: sprintf(widget_text, "x"); break;
				default:sprintf(widget_text, ""); break;
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

		// Show Objectives
		if(is_mission_id_on_objective_show_list(m.id)) {
			struct MissionObjective_DB *objectives;
			int num_objectives = db_get_objectives_from_mission_id(&objectives, m.id);
			for(int j = 0; j < num_objectives; j++) {
				row+=2; added_objectives++;
				GtkWidget *label = gtk_label_new(objectives[j].objective);
				GtkWidget *rank_label = gtk_label_new("");
				gtk_label_set_xalign(GTK_LABEL(label), (gfloat) 0.0);

				// Word wrap
				gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
				gtk_label_set_line_wrap_mode(GTK_LABEL(label), PANGO_WRAP_WORD);
				// Set maximum width in characters to control the wrapping (Somehow this works...)
				gtk_label_set_max_width_chars(GTK_LABEL(label), 1);

				// set css class
				char css_class[50];
				if(objectives[j].rank == OBJ_PRIMARY) {
					gtk_label_set_label(GTK_LABEL(rank_label), "PRIMARY");
					if(objectives[j].status == OBJ_SUCCESS) sprintf(css_class, "missiondb-primary-obj-success");
					else if(objectives[j].status == OBJ_FAIL) sprintf(css_class, "missiondb-primary-obj-fail");
					else sprintf(css_class, "missiondb-primary-obj-tbd");
				} else {
					gtk_label_set_label(GTK_LABEL(rank_label), "SECONDARY");
					if(objectives[j].status == OBJ_SUCCESS) sprintf(css_class, "missiondb-secondary-obj-success");
					else if(objectives[j].status == OBJ_FAIL) sprintf(css_class, "missiondb-secondary-obj-fail");
					else sprintf(css_class, "missiondb-secondary-obj-tbd");
				}

				set_css_class_for_widget(label, css_class);
				set_css_class_for_widget(rank_label, css_class);

				separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
				gtk_grid_attach(GTK_GRID(mission_grid), separator, 0, row, 1, 1);
				separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
				gtk_grid_attach(GTK_GRID(mission_grid), separator, 2, row, 1, 1);
				gtk_grid_attach(GTK_GRID(mission_grid), rank_label, 3, row, 1, 1);
				separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
				gtk_grid_attach(GTK_GRID(mission_grid), separator, 4, row, 1, 1);
				gtk_grid_attach(GTK_GRID(mission_grid), label, 5, row, num_mission_cols * 2 - 5, 1);
				separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
				gtk_grid_attach(GTK_GRID(mission_grid), separator, num_mission_cols * 2, row, 1, 1);
				separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
				gtk_grid_attach(GTK_GRID(mission_grid), separator, 0, row + 1, num_mission_cols * 2 + 1, 1);
			}
			free(objectives);
		}

		// Show Events

	}

	gtk_container_add (GTK_CONTAINER (mission_vp),
					   mission_grid);
	gtk_widget_show_all(GTK_WIDGET(mission_vp));

	free(programs);
}

void add_mission_id_to_objective_show_list(int mission_id) {
	add_mission_id_to_info_show_list(&show_mission_objectives, &num_show_mission_objectives, mission_id);
}

void remove_mission_id_from_objective_show_list(int mission_id) {
	remove_mission_id_from_info_show_list(show_mission_objectives, num_show_mission_objectives, mission_id);
}

int is_mission_id_on_objective_show_list(int mission_id) {
	return is_mission_id_on_info_show_list(show_mission_objectives, num_show_mission_objectives, mission_id);
}

void add_mission_id_to_event_show_list(int mission_id) {
	add_mission_id_to_info_show_list(&show_mission_events, &num_show_mission_events, mission_id);
}

void remove_mission_id_from_event_show_list(int mission_id) {
	remove_mission_id_from_info_show_list(show_mission_events, num_show_mission_events, mission_id);
}

int is_mission_id_on_event_show_list(int mission_id) {
	return is_mission_id_on_info_show_list(show_mission_events, num_show_mission_events, mission_id);
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
	if(show_mission_objectives != NULL) free(show_mission_objectives);
	if(show_mission_events != NULL) free(show_mission_events);
	show_mission_objectives = NULL;
	show_mission_events = NULL;
	num_show_mission_objectives = 0;
	num_show_mission_events = 0;
	update_db_box();
}

void on_new_mission() {
	struct Mission_DB empty_mission = {.id = -1};
	switch_to_mission_manager(empty_mission);
	switch_to_mission_manager_page();
}

void on_showhide_mission_objectives(GtkWidget *button, gpointer data) {
	int mission_id = *((int *) data);  // Cast data back to int
	if(is_mission_id_on_objective_show_list(mission_id)) remove_mission_id_from_objective_show_list(mission_id);
	else {
		if(is_mission_id_on_event_show_list(mission_id)) remove_mission_id_from_event_show_list(mission_id);
		add_mission_id_to_objective_show_list(mission_id);
	}
	update_db_box();
}

void on_showhide_mission_events(GtkWidget *button, gpointer data) {
	int mission_id = *((int *) data);  // Cast data back to int
	if(is_mission_id_on_event_show_list(mission_id)) remove_mission_id_from_event_show_list(mission_id);
	else {
		if(is_mission_id_on_objective_show_list(mission_id)) remove_mission_id_from_objective_show_list(mission_id);
		add_mission_id_to_event_show_list(mission_id);
	}
	update_db_box();
}

void on_edit_mission(GtkWidget *button, gpointer data) {
	int mission_id = *((int *) data);  // Cast data back to int

	struct Mission_DB mission = get_mission_from_id(missions, num_missions, mission_id);
	switch_to_mission_manager_page();
	switch_to_mission_manager(mission);
}

void close_mission_db() {
	if(missions != NULL) free(missions);
	if(show_mission_objectives != NULL) free(show_mission_objectives);
	if(show_mission_events != NULL) free(show_mission_events);
	missions = NULL;
	show_mission_objectives = NULL;
	show_mission_events = NULL;
}
