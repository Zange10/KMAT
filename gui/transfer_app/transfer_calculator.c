#include "transfer_calculator.h"
#include "orbit_calculator/transfer_calc.h"
#include "gui/gui_manager.h"
#include "gui/settings.h"
#include "celestial_bodies.h"
#include "tools/datetime.h"
#include "tools/file_io.h"
#include <string.h>
#include <sys/time.h>


struct PlannedStep {
	struct Body *body;
	double min_depdur;
	double max_depdur;
	struct PlannedStep *prev;
	struct PlannedStep *next;
};


GObject *cb_tc_body;
GObject *tf_tc_mindepdate;
GObject *tf_tc_maxdepdate;
GObject *tf_tc_mindur;
GObject *tf_tc_maxdur;
GObject *cb_tc_transfertype;
GObject *tf_tc_totdv;
GObject *tf_tc_depdv;
GObject *tf_tc_satdv;
GObject *tf_tc_preview;

struct PlannedStep *tc_step;

struct System *tc_system;


double max_totdv_tc, max_depdv_tc, max_satdv_tc;
enum LastTransferType tc_last_transfer_type;


void init_transfer_calculator(GtkBuilder *builder) {
	tc_step = NULL;
	cb_tc_body = gtk_builder_get_object(builder, "cb_tc_body");
	tf_tc_mindepdate = gtk_builder_get_object(builder, "tf_tc_mindepdate");
	tf_tc_maxdepdate = gtk_builder_get_object(builder, "tf_tc_maxdepdate");
	tf_tc_mindur = gtk_builder_get_object(builder, "tf_tc_mindur");
	tf_tc_maxdur = gtk_builder_get_object(builder, "tf_tc_maxdur");
	cb_tc_transfertype = gtk_builder_get_object(builder, "cb_tc_transfertype");
	tf_tc_totdv = gtk_builder_get_object(builder, "tf_tc_totdv");
	tf_tc_depdv = gtk_builder_get_object(builder, "tf_tc_depdv");
	tf_tc_satdv = gtk_builder_get_object(builder, "tf_tc_satdv");
	tf_tc_preview = gtk_builder_get_object(builder, "tf_tc_preview");

	tc_system = get_current_system();

	create_combobox_dropdown_text_renderer(cb_tc_body);
	if(get_num_available_systems() > 0 && tc_system != NULL) update_body_dropdown(GTK_COMBO_BOX(cb_tc_body), tc_system);
}



void tc_change_date_type(enum DateType old_date_type, enum DateType new_date_type) {

}


void current_time_to_string(char *string) {
	time_t current_time;
	struct tm *timeinfo;

	sprintf(string, "");

	/* Obtain current time */
	current_time = time(NULL);

	if (current_time == ((time_t)-1)) {
		fprintf(stderr, "Failure to obtain the current time.\n");
		return;
	}

	/* Convert to local time */
	timeinfo = localtime(&current_time);

	if (timeinfo == NULL) {
		fprintf(stderr, "Failure to convert the current time.\n");
		return;
	}


	struct timeval current_time2;
	gettimeofday(&current_time2, NULL);
	double millis = (double) (current_time2.tv_usec)/1000000.0;

	/* Extract minutes and seconds */
	struct Date current_date = {
			timeinfo->tm_year + 1900,
			timeinfo->tm_mon + 1,
			timeinfo->tm_mday,
			timeinfo->tm_hour,
			timeinfo->tm_min,
			timeinfo->tm_sec + millis
	};

	/* Print the current time */
	date_to_string(current_date, string, 1);
}

struct PlannedStep * get_first_tc(struct PlannedStep *step) {
	if(step == NULL) return NULL;
	while(step->prev != NULL) step = step->prev;
	return step;
}

struct PlannedStep * get_last_tc(struct PlannedStep *step) {
	if(step == NULL) return NULL;
	while(step->next != NULL) step = step->next;
	return step;
}

int get_num_steps_tc(struct PlannedStep *step) {
	if(step == NULL) return 0;
	int counter = 0;
	step = get_first_tc(tc_step);
	while(step != NULL) {
		counter++;
		step = step->next;
	}
	return counter;
}

void update_dvs_and_depdates() {
	char *string;
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_tc_totdv));
	max_totdv_tc = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_tc_depdv));
	max_depdv_tc = strtod(string, NULL);
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_tc_satdv));
	max_satdv_tc = strtod(string, NULL);

	if(max_totdv_tc <= 0) max_totdv_tc = 100000;
	if(max_depdv_tc <= 0) max_depdv_tc = 100000;
	if(max_satdv_tc <= 0) max_satdv_tc = 100000;

	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_tc_mindepdate));
	double min_depdate = convert_date_JD(date_from_string(string, get_settings_datetime_type()));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_tc_maxdepdate));
	double max_depdate = convert_date_JD(date_from_string(string, get_settings_datetime_type()));

	get_first_tc(tc_step)->min_depdur = min_depdate;
	get_first_tc(tc_step)->max_depdur = max_depdate;

	string = (char*) gtk_combo_box_get_active_id(GTK_COMBO_BOX(cb_tc_transfertype));
	int tt = (int) strtol(string, NULL, 10);
	if(tt == 1) tc_last_transfer_type = TF_CAPTURE;
	else if(tt == 2) tc_last_transfer_type = TF_CIRC;
	else tc_last_transfer_type = TF_FLYBY;
}

void update_preview_tc(int calc) {
	struct PlannedStep *step = get_first_tc(tc_step);
	char preview_text[1000] = "";
	char temp_string[100];
	if(step == NULL) {
		sprintf(preview_text, "");
		return;
	} else {
		update_dvs_and_depdates();

		sprintf(temp_string,"Max Total dv: \t\t\t%.2f m/s\n"
							 "Max Departure dv: \t%.2f m/s\n"
							 "Max Satellite dv: \t\t%.2f m/s\n\n",
							 max_totdv_tc, max_depdv_tc, max_satdv_tc);
		strcat(preview_text, temp_string);

		sprintf(temp_string, "Last Transfer: \t\t\t");
		if(tc_last_transfer_type == TF_FLYBY) strcat(temp_string,"FLY-BY");
		else if(tc_last_transfer_type == TF_CAPTURE) strcat(temp_string,"CAPTURE");
		else if(tc_last_transfer_type == TF_CIRC) strcat(temp_string,"CIRCULARIZE");
		strcat(temp_string,"\n\n--\n\n");
		strcat(preview_text, temp_string);

		date_to_string(convert_JD_date(step->min_depdur, get_settings_datetime_type()), temp_string, 0);
		strcat(preview_text, temp_string);
		strcat(preview_text, "  |  ");
		date_to_string(convert_JD_date(step->max_depdur, get_settings_datetime_type()), temp_string, 0);
		strcat(preview_text, temp_string);
		sprintf(temp_string, "\n%s\n", step->body->name);
		strcat(preview_text, temp_string);

		while(step->next != NULL) {
			step = step->next;
			sprintf(temp_string, "%d  |  %d\n%s\n", (int) step->min_depdur, (int) step->max_depdur, step->body->name);
			strcat(preview_text, temp_string);
		}
	}
	if(calc) {
		current_time_to_string(temp_string);
		strcat(preview_text, "\n\nCalculation started at:\t");
		strcat(preview_text, temp_string);
	}
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tf_tc_preview));
	gtk_text_buffer_set_text(buffer, preview_text, -1);
	gtk_widget_queue_draw(GTK_WIDGET(tf_tc_preview));
}

void save_itineraries_tc(struct ItinStep **departures, int num_deps, int num_nodes) {
	if(departures == NULL || num_deps == 0) return;

	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
	gint res;

	// Create the file chooser dialog
	dialog = gtk_file_chooser_dialog_new("Save File", NULL, action,
										 "_Cancel", GTK_RESPONSE_CANCEL,
										 "_Save", GTK_RESPONSE_ACCEPT,
										 NULL);

	// Set initial folder
	create_directory_if_not_exists(get_itins_directory());
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), get_itins_directory());

	// Create a filter for files with the extension .itin
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*.itins");
	gtk_file_filter_set_name(filter, ".itins");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	// Run the dialog
	res = gtk_dialog_run(GTK_DIALOG(dialog));
	if (res == GTK_RESPONSE_ACCEPT) {
		char *filepath;
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		filepath = gtk_file_chooser_get_filename(chooser);

//		store_itineraries_in_bfile(departures, num_nodes, num_deps, filepath, 0);
		g_free(filepath);
	}

	// Destroy the dialog
	gtk_widget_destroy(dialog);
}

G_MODULE_EXPORT void on_add_transfer_tc() {
	struct PlannedStep *new_step = (struct PlannedStep*) malloc(sizeof(struct PlannedStep));
	new_step->prev = NULL;
	new_step->next = NULL;

	new_step->body = tc_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_tc_body))];

	if(tc_step != NULL) {
		char *string;
		string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_tc_mindur));
		new_step->min_depdur = strtod(string, NULL);
		string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_tc_maxdur));
		new_step->max_depdur = strtod(string, NULL);
		struct PlannedStep *last = get_last_tc(tc_step);
		last->next = new_step;
		new_step->prev = last;
	}

	tc_step = new_step;

	update_preview_tc(0);
}

G_MODULE_EXPORT void on_remove_transfer_tc() {
	struct PlannedStep *step = get_last_tc(tc_step);
	if(step->prev == NULL) {
		tc_step = NULL;
		free(step);
	} else {
		step = step->prev;
		free(step->next);
		tc_step = step;
		step->next = NULL;
	}
	update_preview_tc(0);
}

G_MODULE_EXPORT void on_update_tc() {
	update_preview_tc(0);
}

G_MODULE_EXPORT void on_calc_tc() {
	struct PlannedStep *step = get_first_tc(tc_step);
	if(step == NULL || step->next == NULL) return;

	update_dvs_and_depdates();

	int num_steps = get_num_steps_tc(tc_step);
	struct Transfer_Calc_Data calc_data;
	calc_data.num_steps = num_steps;
	calc_data.jd_min_dep = step->min_depdur;
	calc_data.jd_max_dep = step->max_depdur;
	calc_data.dv_filter.max_totdv = max_totdv_tc;
	calc_data.dv_filter.max_depdv = max_depdv_tc;
	calc_data.dv_filter.max_satdv = max_satdv_tc;
	calc_data.dv_filter.last_transfer_type = (int) tc_last_transfer_type;

	calc_data.bodies = (struct Body**) malloc(num_steps * sizeof(struct Body*));
	calc_data.min_duration = (int*) malloc((num_steps-1) * sizeof(int));
	calc_data.max_duration = (int*) malloc((num_steps-1) * sizeof(int));

	calc_data.system = get_current_system();

	for(int i = 0; i < num_steps; i++) {
		calc_data.bodies[i] = step->body;
		if(i != 0) {
			calc_data.min_duration[i-1] = (int) step->min_depdur;
			calc_data.max_duration[i-1] = (int) step->max_depdur;
		}
		step = step->next;
	}

	update_preview_tc(1);

	struct Transfer_Calc_Results results = search_for_spec_itinerary(calc_data);

	save_itineraries_tc(results.departures, results.num_deps, results.num_nodes);
	for(int i = 0; i < results.num_deps; i++) free_itinerary(results.departures[i]);
	free(results.departures);
	free(calc_data.bodies);
	free(calc_data.min_duration);
	free(calc_data.max_duration);
}

void reset_tc() {
	if(tc_step == NULL) return;
	tc_step = get_first_tc(tc_step);
	while(tc_step->next != NULL) {
		tc_step = tc_step->next;
		free(tc_step->prev);
	}
	free(tc_step);
	tc_step = NULL;
}
