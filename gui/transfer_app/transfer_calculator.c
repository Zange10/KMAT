#include "transfer_calculator.h"
#include "ephem.h"


struct PlannedStep {
	struct Body *Body;
	double min_depdur;
	double max_depdur;
	double max_totdv;
	double max_depdv;
	double max_Satdv;
	struct PlannedStep *prev;
	struct PlannedStep *next;
};


GObject *tf_tc_body;
GObject *tf_tc_mindepdate;
GObject *tf_tc_maxdepdate;
GObject *tf_tc_mindur;
GObject *tf_tc_maxdur;
GObject *cb_tc_transfertype;
GObject *tf_tc_totdv;
GObject *tf_tc_depdv;
GObject *tf_tc_satdv;
struct PlannedStep *tc_step;


void init_transfer_calculator(GtkBuilder *builder) {
	tc_step = NULL;
	tf_tc_body = gtk_builder_get_object(builder, "tf_tc_body");
	tf_tc_mindepdate = gtk_builder_get_object(builder, "tf_tc_mindepdate");
	tf_tc_maxdepdate = gtk_builder_get_object(builder, "tf_tc_maxdepdate");
	tf_tc_mindur = gtk_builder_get_object(builder, "tf_tc_mindur");
	tf_tc_maxdur = gtk_builder_get_object(builder, "tf_tc_maxdur");
	cb_tc_transfertype = gtk_builder_get_object(builder, "cb_tc_transfertype");
	tf_tc_totdv = gtk_builder_get_object(builder, "tf_tc_totdv");
	tf_tc_depdv = gtk_builder_get_object(builder, "tf_tc_depdv");
	tf_tc_satdv = gtk_builder_get_object(builder, "tf_tc_satdv");
}

void on_add_transfer_tc() {
//	int body_id;
//	char *string;
//	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_pa_min_feedback[0]));
//	body_id = (int) strtol(string, NULL, 10);
}

void on_remove_transfer_tc() {

}

