#include "gmat_interface.h"
#include <stdio.h>
#include <math.h>
#include <time.h>


char *body_name[] = {
		"Sun",
		"Mercury",
		"Venus",
		"Earth",
		"Mars",
		"Jupiter",
		"Saturn",
		"Uranus",
		"Neptune",
		"Pluto"
};

char *coord_name[] = {
		"SunEc",
		"MercuryEc",
		"VenusEc",
		"EarthEc",
		"MarsEc",
		"JupiterEc",
		"SaturnEc",
		"UranusEc",
		"NeptuneEc",
		"PlutoEc"
};

char *sc_var[] = {
		"launch",
		"fb",
		"arrival"
};

char *sc_name[] = {
		"Launch",
		"Fb",
		"Arrival"
};

char propagator[] = "SunProp";

char force_model[] = "SunProp_ForceModel";

char vf13ad_solver[] = "NLPOpt";	// not so accurate but for refining initial guess
//char solvermode[] = "RunInitialGuess";
char solvermode[] = "Solve";
char propagator_algo[] = "PrinceDormand45";


int gmat_jd_mod = 2430000;


void print_gmat_section(FILE *filePointer, const char *section);
void create_gmat_header(FILE *filePointer, int num_steps, struct ItinStep *step2pr);
void create_gmat_spacecrafts(FILE *filePointer, int num_steps);
void create_gmat_propagator(FILE *filePointer, int num_bodies);
void create_gmat_coordinate_systems(FILE *filePointer, int num_bodies);
void create_gmat_solver(FILE *filePointer);
void create_gmat_subscribers(FILE *filePointer, int num_steps);
void create_gmat_variables(FILE *filePointer, int num_steps);
void set_gmat_initial_guess(FILE *filePointer, int num_steps, struct ItinStep *step2pr);
void set_gmat_all_sc_setup(FILE *filePointer, int num_steps, struct ItinStep *step2pr, int num_tabs);
void determine_gmat_mean_epochs(FILE *filePointer, int num_steps, struct ItinStep *step2pr);
void write_gmat_optimizers(FILE *filePointer, int num_steps, struct ItinStep *step2pr);
void write_gmat_final_optimizer(FILE *filePointer, int num_steps, struct ItinStep *step2pr);


void write_gmat_script(struct ItinStep *step2pr, const char *filepath) {
	step2pr = get_first(step2pr);

	int num_steps = get_num_of_itin_layers(step2pr);
	int num_bodies = (sizeof(body_name) / sizeof(body_name[0]));

	if(num_steps <= 2) {
		printf("\nCannot create GMAT Optimizing Script for simple transfers (Must have at least one fly-by before arrial)\n");
		return;
	}

	// Open the file in write mode
	FILE *filePointer = fopen(filepath, "w");

	// Check if the file was successfully opened
	if (filePointer == NULL) {
		printf("Failed to open file.\n");
		return;
	}


	create_gmat_header(filePointer, num_steps, step2pr);
	create_gmat_spacecrafts(filePointer, num_steps);
	create_gmat_propagator(filePointer, num_bodies);
	create_gmat_coordinate_systems(filePointer, num_bodies);
	create_gmat_solver(filePointer);
	create_gmat_subscribers(filePointer, num_steps);
	create_gmat_variables(filePointer, num_steps);

	print_gmat_section(filePointer, "Mission Sequence");
	fprintf(filePointer, "BeginMissionSequence\n\n\n");

	set_gmat_initial_guess(filePointer, num_steps, step2pr);
	set_gmat_all_sc_setup(filePointer, num_steps, step2pr, 0);
	determine_gmat_mean_epochs(filePointer, num_steps, step2pr);
	write_gmat_optimizers(filePointer, num_steps, step2pr);
	write_gmat_final_optimizer(filePointer, num_steps, step2pr);


	fprintf(filePointer, "\n\nReport RF1 %s_epoch %s_c3 %s_rha %s_dha ", sc_var[0], sc_var[0], sc_var[0], sc_var[0]);
	for(int i = 1; i < num_steps-1; i++) {
		char var[20];
		sprintf(var, "%s%i", sc_var[1], i);

		fprintf(filePointer, "%s_epoch %s_incl %s_periapsis ", var, var, var);
	}
	fprintf(filePointer, "%s_epoch %s_c3\n", sc_var[2], sc_var[2]);


	printf("\nCreated GMAT-Script: %s\n\n", filepath);

	// Close the file
	fclose(filePointer);
}


void setup_gmat_sc(FILE *filePointer, const char *sc, const char *var_sc, const char *body, const char *coord, int num_tabs, int outgoing) {
	char inout[10];
	sprintf(inout, "%s", outgoing ? "Outgoing" : "Incoming");
	for(int i = 0; i < num_tabs; i++) fprintf(filePointer, "\t");
	fprintf(filePointer, "%s.UTCModJulian = %s_epoch\n", sc, var_sc);
	for(int i = 0; i < num_tabs; i++) fprintf(filePointer, "\t");
	fprintf(filePointer, "%s.%s.%sC3Energy = %s_c3\n", sc, body, inout, var_sc);
	for(int i = 0; i < num_tabs; i++) fprintf(filePointer, "\t");
	fprintf(filePointer, "%s.%s.%sRHA = %s_rha\n", sc, coord, inout, var_sc);
	for(int i = 0; i < num_tabs; i++) fprintf(filePointer, "\t");
	fprintf(filePointer, "%s.%s.%sDHA = %s_dha\n", sc, coord, inout, var_sc);
	for(int i = 0; i < num_tabs; i++) fprintf(filePointer, "\t");
	fprintf(filePointer, "%s.%s.%sBVAZI = %s_bvazi\n", sc, coord, inout, var_sc);
	for(int i = 0; i < num_tabs; i++) fprintf(filePointer, "\t");
	fprintf(filePointer, "%s.%s.%sRadPer = %s_periapsis\n", sc, body, inout, var_sc);
	for(int i = 0; i < num_tabs; i++) fprintf(filePointer, "\t");
	fprintf(filePointer, "%s.%s.TA = 0\n", sc, body);
}


void create_gmat_header(FILE *filePointer, int num_steps, struct ItinStep *step2pr) {
	step2pr = get_first(step2pr);

	time_t currentTime;
	struct tm *localTime;
	char timeString[100];

	// Get the current time
	time(&currentTime);

	// Convert to local time
	localTime = localtime(&currentTime);

	// Format the time string
	strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", localTime);

	fprintf(filePointer, "%% ------------------------------------------------\n");
	fprintf(filePointer, "%% General Mission Analysis Tool(GMAT) Script\n");
	fprintf(filePointer, "%% Current time: %s\n", timeString);
	fprintf(filePointer, "\n%% Itinerary: \t");
	for(int i = 0; i < num_steps; i++){
		if(i != 0) {fprintf(filePointer, " -> "); step2pr = step2pr->next[0];}
		fprintf(filePointer, "%s", step2pr->body->name);
	}
	fprintf(filePointer, "\n");
	step2pr = get_first(step2pr);
	date_to_string(convert_JD_date(step2pr->date, 0), timeString, 1);
	fprintf(filePointer, "%% Departure: \t%s\n", timeString);
	step2pr = get_last(step2pr);
	date_to_string(convert_JD_date(step2pr->date, 0), timeString, 1);
	fprintf(filePointer, "%% Arrival: \t\t%s (Estimated)\n", timeString);
	fprintf(filePointer, "%% ------------------------------------------------\n");
}

void print_gmat_section(FILE *filePointer, const char *section) {
//	%----------------------------------------
//	%---------- Section
//	%----------------------------------------
	fprintf(filePointer, "\n\n%%----------------------------------------\n"
	 "%%---------- %s\n"
	 "%%----------------------------------------\n\n", section);
}



void create_gmat_spacecrafts(FILE *filePointer, int num_steps) {
	print_gmat_section(filePointer, "Spacecraft");

	fprintf(filePointer, "Create Spacecraft %s", sc_name[0]);
	for(int i = 1; i <= num_steps-2; i++) {
		fprintf(filePointer, " %s%d_Arr %s%d_Dep", sc_name[1], i, sc_name[1], i);
	}
	fprintf(filePointer, " %s\n", sc_name[2]);
}


void create_gmat_propagator(FILE *filePointer, int num_bodies) {
	print_gmat_section(filePointer, "ForceModels");

	fprintf(filePointer, "Create ForceModel %s\n", force_model);
	fprintf(filePointer, "%s.CentralBody = %s\n", force_model, body_name[0]);
	fprintf(filePointer, "%s.PointMasses = {", force_model);
	for(int i = 0; i < num_bodies; i++) {
		if(i != 0) fprintf(filePointer, ", ");
		fprintf(filePointer, "%s", body_name[i]);
	}
	fprintf(filePointer, ", Luna}\n");


	print_gmat_section(filePointer, "Propagators");
	fprintf(filePointer, "Create Propagator %s\n", propagator);
	fprintf(filePointer, "%s.FM = %s\n", propagator, force_model);
	fprintf(filePointer, "%s.Type = %s\n", propagator, propagator_algo);
}

void create_gmat_coordinate_systems(FILE *filePointer, int num_bodies) {
	print_gmat_section(filePointer, "Coordinate Systems");

	for(int i = 0; i < num_bodies; i++) {
		if(i != 0) fprintf(filePointer, "\n");
		fprintf(filePointer, "Create CoordinateSystem %s\n", coord_name[i]);
		fprintf(filePointer, "%s.Origin = %s\n", coord_name[i], body_name[i]);
		fprintf(filePointer, "%s.Axes = MJ2000Ec\n", coord_name[i]);
	}
}

void create_gmat_solver(FILE *filePointer) {
	print_gmat_section(filePointer, "Solvers");

	fprintf(filePointer, "Create VF13ad %s\n", vf13ad_solver);
	fprintf(filePointer, "%s.ShowProgress = true\n", vf13ad_solver);
	fprintf(filePointer, "%s.Tolerance = 1e-05\n", vf13ad_solver); // tolerance to minimize
	fprintf(filePointer, "%s.FeasibilityTolerance = 0.1\n", vf13ad_solver); // tolerance to constraints
}

void create_gmat_subscribers(FILE *filePointer, int num_steps) {
	print_gmat_section(filePointer, "Subscribers");

	char viewname[] = "SunViewIP";
	char pos_error_plot[] = "PositionError";
	char vel_error_plot[] = "VelocityError";
	char cost_plot[] = "CostPlot";

	fprintf(filePointer, "Create OrbitView %s\n", viewname);
	fprintf(filePointer, "%s.SolverIterations = Current\n", viewname);
	fprintf(filePointer, "%s.Add = {Sun, Mercury, Venus, Earth, Mars, %s, %s", viewname, sc_name[0], sc_name[2]);
	for(int i = 1; i < num_steps-1; i++) fprintf(filePointer, ", %s%d_Arr, %s%d_Dep", sc_name[1], i, sc_name[1], i);
	fprintf(filePointer, "}\n");
	fprintf(filePointer, "%s.CoordinateSystem = %s\n", viewname, coord_name[0]);
//	fprintf(filePointer, "%s.DataCollectFrequency = 1\n", viewname);
//	fprintf(filePointer, "%s.UpdatePlotFrequency = 50\n", viewname);
	fprintf(filePointer, "%s.ShowPlot = true\n", viewname);
	fprintf(filePointer, "%s.ViewPointReference = Sun\n", viewname);
	fprintf(filePointer, "%s.ViewPointVector = [ 0 0 1000000000 ]\n", viewname);
	fprintf(filePointer, "%s.ViewDirection = Sun\n", viewname);
	fprintf(filePointer, "%s.ViewUpCoordinateSystem = %s\n", viewname, coord_name[0]);
	fprintf(filePointer, "%s.ViewUpAxis = Z\n", viewname);
	fprintf(filePointer, "%s.EclipticPlane = Off\n", viewname);
	fprintf(filePointer, "%s.XYPlane = On\n", viewname);
	fprintf(filePointer, "%s.WireFrame = Off\n", viewname);
	fprintf(filePointer, "%s.Axes = On\n", viewname);
	fprintf(filePointer, "%s.Grid = Off\n", viewname);
	fprintf(filePointer, "%s.SunLine = On\n", viewname);
	fprintf(filePointer, "%s.StarCount = 1000\n", viewname);
	fprintf(filePointer, "%s.EnableStars = Off\n", viewname);
	fprintf(filePointer, "%s.EnableConstellations = Off\n", viewname);

	fprintf(filePointer, "\nCreate XYPlot %s\n", pos_error_plot);
	fprintf(filePointer, "%s.SolverIterations = All\n", pos_error_plot);
	fprintf(filePointer, "%s.XVariable = loopIdx\n", pos_error_plot);
	fprintf(filePointer, "%s.YVariables = {errorR1", pos_error_plot);
	for(int i = 2; i < num_steps; i++) fprintf(filePointer, ", errorR%d", i);
	fprintf(filePointer, "}\n");
	fprintf(filePointer, "%s.ShowGrid = true\n", pos_error_plot);
	fprintf(filePointer, "%s.ShowPlot = true\n", pos_error_plot);

	fprintf(filePointer, "\nCreate XYPlot %s\n", vel_error_plot);
	fprintf(filePointer, "%s.SolverIterations = All\n", vel_error_plot);
	fprintf(filePointer, "%s.XVariable = loopIdx\n", vel_error_plot);
	fprintf(filePointer, "%s.YVariables = {errorV1", vel_error_plot);
	for(int i = 2; i < num_steps; i++) fprintf(filePointer, ", errorV%d", i);
	fprintf(filePointer, "}\n");
	fprintf(filePointer, "%s.ShowGrid = true\n", vel_error_plot);
	fprintf(filePointer, "%s.ShowPlot = true\n", vel_error_plot);

	fprintf(filePointer, "\nCreate XYPlot %s\n", cost_plot);
	fprintf(filePointer, "%s.SolverIterations = All\n", cost_plot);
	fprintf(filePointer, "%s.XVariable = loopIdx\n", cost_plot);
	fprintf(filePointer, "%s.YVariables = {Cost}\n", cost_plot);
	fprintf(filePointer, "%s.ShowGrid = true\n", cost_plot);
	fprintf(filePointer, "%s.ShowPlot = true\n", cost_plot);

	fprintf(filePointer, "\nCreate ReportFile RF1\n");
	fprintf(filePointer, "RF1.Filename = 'ReportStates.txt'\n");

	fprintf(filePointer, "\nCreate ReportFile RFDepStates\n");
	fprintf(filePointer, "RFDepStates.Filename = 'DepartureStates.txt'\n");
}

void create_gmat_sc_variables(FILE *filePointer, const char *var) {
	fprintf(filePointer, "Create Variable %s_epoch %s_c3 %s_rha %s_dha %s_bvazi %s_periapsis %s_incl\n", var, var, var, var, var, var, var);
}

void create_gmat_variables(FILE *filePointer, int num_steps) {
	print_gmat_section(filePointer, "Arrays, Variables, Strings");

	fprintf(filePointer, "%% Propagation time and Mean Epochs between states\n");
	fprintf(filePointer, "Create Variable prop_time");
	for(int i = 1; i < num_steps; i++) fprintf(filePointer, " mean_epoch%d", i);

	fprintf(filePointer, "\n\n%% Error and Cost Variables\n");
	fprintf(filePointer, "Create Variable Cost dep_speed");
	for(int i = 1; i < num_steps; i++) fprintf(filePointer, " errorR%d errorV%d", i, i);

	fprintf(filePointer, "\n\n%% Initial Guesses (hyperbolas)\n");
	create_gmat_sc_variables(filePointer, sc_var[0]);
	for(int i = 1; i < num_steps-1; i++) {
		char var[20];
		sprintf(var, "%s%i", sc_var[1], i);
		create_gmat_sc_variables(filePointer, var);
	}
	create_gmat_sc_variables(filePointer, sc_var[2]);

	fprintf(filePointer, "\n%% misc\n");
	fprintf(filePointer, "Create Variable loopIdx EarthGravParam\n");
}

void set_gmat_sc_initial_guess(FILE *filePointer, const char *var, struct ItinStep *step2pr) {
	struct HyperbolaParams hyp_params;
	double rp;

	if(step2pr->prev == NULL)
		hyp_params = get_hyperbola_params(vec3(0,0,0), step2pr->next[0]->v_dep, step2pr->v_body, step2pr->body, 200e3, HYP_DEPARTURE);
	else if(step2pr->num_next_nodes == 0) {
		hyp_params = get_hyperbola_params(step2pr->v_arr, vec3(0,0,0), step2pr->v_body, step2pr->body,200e3, HYP_ARRIVAL);
	} else {
		Vector3 v_arr = step2pr->v_arr;
		Vector3 v_dep = step2pr->next[0]->v_dep;
		Vector3 v_body = step2pr->v_body;
		rp = get_flyby_periapsis(v_arr, v_dep, v_body, step2pr->body);
		hyp_params = get_hyperbola_params(step2pr->v_arr, step2pr->next[0]->v_dep, step2pr->v_body, step2pr->body, 0, HYP_FLYBY);
	}


	fprintf(filePointer, "\t%s_epoch = %f\n", var, step2pr->date-gmat_jd_mod);
	fprintf(filePointer, "\t%s_c3 = %f\n", var, hyp_params.c3_energy/1e6);
	fprintf(filePointer, "\t%s_rha = %f\n", var, rad2deg(hyp_params.incoming.bplane_angle));
	fprintf(filePointer, "\t%s_dha = %f\n", var, rad2deg(hyp_params.incoming.decl));

	if(step2pr->prev != NULL && step2pr->next != NULL) {
		fprintf(filePointer, "\t%s_bvazi = %f\n", var, rad2deg(hyp_params.incoming.bvazi));
		fprintf(filePointer, "\t%s_periapsis = %f\n", var, rp*1e-3);
	}
}

void set_gmat_initial_guess(FILE *filePointer, int num_steps, struct ItinStep *step2pr) {
	step2pr = get_first(step2pr);
	fprintf(filePointer, "BeginScript 'Initial Guess Values'\n");
	fprintf(filePointer, "\t%% Set Initial Guess Hyperbolas\n");
	set_gmat_sc_initial_guess(filePointer, sc_var[0], step2pr);
	for(int i = 1; i < num_steps-1; i++) {
		step2pr = step2pr->next[0];
		fprintf(filePointer, "\n");
		char var[20];
		sprintf(var, "%s%i", sc_var[1], i);
		set_gmat_sc_initial_guess(filePointer, var, step2pr);
	}
	fprintf(filePointer, "\n");
	step2pr = step2pr->next[0];
	set_gmat_sc_initial_guess(filePointer, sc_var[2], step2pr);

	step2pr = get_first(step2pr);
	fprintf(filePointer, "\n\t%% Set Base Constraints\n");
	fprintf(filePointer, "\t%s_bvazi = 90\n", sc_var[0]);
	fprintf(filePointer, "\t%s_periapsis = %f\n", sc_var[0], step2pr->body->radius / 1e3 + 200);
	step2pr = get_last(step2pr);
	fprintf(filePointer, "\t%s_bvazi = 90\n", sc_var[2]);
	fprintf(filePointer, "\t%s_periapsis = %f\n", sc_var[2], step2pr->body->radius / 1e3 * 1.1);

	fprintf(filePointer, "\n\t%% Misc\n");
	fprintf(filePointer, "\tEarthGravParam = 398600\n");
	fprintf(filePointer, "\tCost = sqrt(%s_c3 + 2*EarthGravParam/%s_periapsis) - sqrt(EarthGravParam/%s_periapsis)\n", sc_var[0], sc_var[0], sc_var[0]);
	fprintf(filePointer, "EndScript\n");
}

void set_gmat_all_sc_setup(FILE *filePointer, int num_steps, struct ItinStep *step2pr, int num_tabs) {
	step2pr = get_first(step2pr);
	fprintf(filePointer, "\n");
	for(int i = 0; i < num_tabs; i++) fprintf(filePointer, "\t");
	fprintf(filePointer, "BeginScript 'All S/C Setup'\n");
	setup_gmat_sc(filePointer, sc_name[0], sc_var[0], body_name[step2pr->body->id], coord_name[step2pr->body->id], 1+num_tabs, 1);

	for(int i = 1; i < num_steps-1; i++) {
		fprintf(filePointer, "\n");
		step2pr = step2pr->next[0];
		char sc[20];
		char var[20];
		sprintf(var, "%s%i", sc_var[1], i);
		sprintf(sc, "%s%i_Arr", sc_name[1], i);
		setup_gmat_sc(filePointer, sc, var, body_name[step2pr->body->id], coord_name[step2pr->body->id], 1+num_tabs, 0);
		for(int j = 0; j < num_tabs; j++) fprintf(filePointer, "\t");
		fprintf(filePointer, "\t%s%i_Dep = %s%i_Arr\n", sc_name[1], i, sc_name[1], i);
	}
	fprintf(filePointer, "\n");
	step2pr = step2pr->next[0];
	setup_gmat_sc(filePointer, sc_name[2], sc_var[2], body_name[step2pr->body->id], coord_name[step2pr->body->id], 1+num_tabs, 0);

	for(int i = 0; i < num_tabs; i++) fprintf(filePointer, "\t");
	fprintf(filePointer, "EndScript\n");
}

void determine_gmat_mean_epochs(FILE *filePointer, int num_steps, struct ItinStep *step2pr) {
	fprintf(filePointer, "\nBeginScript 'Determine Mean Epochs'\n");
	fprintf(filePointer, "\tmean_epoch1 = (%s_epoch+%s1_epoch)/2'\n", sc_var[0], sc_var[1]);
	for(int i = 2; i < num_steps-1; i++) {
		fprintf(filePointer, "\tmean_epoch%d = (%s%d_epoch+%s%d_epoch)/2'\n", i, sc_var[1], i-1, sc_var[1], i);
	}
	fprintf(filePointer, "\tmean_epoch%d = (%s%d_epoch+%s_epoch)/2'\n", num_steps-1, sc_var[1], num_steps-2, sc_var[2]);
	fprintf(filePointer, "EndScript\n");
}

void write_gmat_optimizer(FILE *filePointer, int traj_id, const char *sc1, const char *var1, const char *coord1, const char *body1, const char *sc2, const char *var2, const char *coord2, const char *body2, int is_departure) {
	fprintf(filePointer, "\nOptimize 'Optimize Trajectory %d' %s {SolveMode = %s, ExitMode = DiscardAndContinue, ShowProgressWindow = true}\n", traj_id, vf13ad_solver, solvermode);
	fprintf(filePointer, "\tloopIdx = loopIdx + 1\n\n");
	fprintf(filePointer, "\t%% Vary states\n");

	if(is_departure) {
		fprintf(filePointer, "\tVary %s(%s_c3  = %s_c3)\n", vf13ad_solver, var1, var1);
		fprintf(filePointer, "\tVary %s(%s_rha = %s_rha)\n", vf13ad_solver, var1, var1);
		fprintf(filePointer, "\tVary %s(%s_dha = %s_dha)\n", vf13ad_solver, var1, var1);
	} else {
		fprintf(filePointer, "\tVary %s(%s_bvazi  = %s_bvazi)\n", vf13ad_solver, var1, var1);
		fprintf(filePointer, "\tVary %s(%s_periapsis = %s_periapsis)\n", vf13ad_solver, var1, var1);
		fprintf(filePointer, "\tVary %s(%s_epoch = %s_epoch)\n", vf13ad_solver, var2, var2);
	}
	fprintf(filePointer, "\tVary %s(%s_c3  = %s_c3)\n", vf13ad_solver, var2, var2);
	fprintf(filePointer, "\tVary %s(%s_rha = %s_rha)\n", vf13ad_solver, var2, var2);
	fprintf(filePointer, "\tVary %s(%s_dha = %s_dha)\n", vf13ad_solver, var2, var2);

	fprintf(filePointer, "\n\tBeginScript 'Set-up S/C'\n");
	setup_gmat_sc(filePointer, sc1, var1, body1, coord1, 2, is_departure);
	fprintf(filePointer, "\n");
	setup_gmat_sc(filePointer, sc2, var2, body2, coord2, 2, 0);
	fprintf(filePointer, "\tEndScript\n");

	fprintf(filePointer, "\n\t%% Propagate forwards from initial body and backwards from target body\n");
	fprintf(filePointer, "\tprop_time = mean_epoch%d - %s.UTCModJulian\n", traj_id, sc1);
	fprintf(filePointer, "\tPropagate 'PropForwards' %s(%s) {%s.ElapsedDays = prop_time}\n", propagator, sc1, sc1);
	fprintf(filePointer, "\tprop_time = mean_epoch%d - %s.UTCModJulian\n", traj_id, sc2);
	fprintf(filePointer, "\tPropagate 'PropBackwards' BackProp %s(%s) {%s.ElapsedDays = prop_time}\n", propagator, sc2, sc2);

	fprintf(filePointer, "\n\t%% Calculate Error in Position and Velocity\n");
	fprintf(filePointer, "\terrorR%d = sqrt((%s.%s.X - %s.%s.X)^2 + (%s.%s.Y - %s.%s.Y)^2 + (%s.%s.Z - %s.%s.Z)^2)\n",
			traj_id, sc1, coord_name[0], sc2, coord_name[0], sc1, coord_name[0], sc2, coord_name[0], sc1, coord_name[0], sc2, coord_name[0]);
	fprintf(filePointer, "\terrorV%d = sqrt((%s.%s.VX - %s.%s.VX)^2 + (%s.%s.VY - %s.%s.VY)^2 + (%s.%s.VZ - %s.%s.VZ)^2)\n",
			traj_id, sc1, coord_name[0], sc2, coord_name[0], sc1, coord_name[0], sc2, coord_name[0], sc1, coord_name[0], sc2, coord_name[0]);

	fprintf(filePointer, "\n\t%% Apply the collocation constraints constraints on final states\n");
	fprintf(filePointer, "\tNonlinearConstraint %s(%s.%s.X  = %s.%s.X)\n", vf13ad_solver, sc1, coord_name[0], sc2, coord_name[0]);
	fprintf(filePointer, "\tNonlinearConstraint %s(%s.%s.Y  = %s.%s.Y)\n", vf13ad_solver, sc1, coord_name[0], sc2, coord_name[0]);
	fprintf(filePointer, "\tNonlinearConstraint %s(%s.%s.Z  = %s.%s.Z)\n", vf13ad_solver, sc1, coord_name[0], sc2, coord_name[0]);
	fprintf(filePointer, "\tNonlinearConstraint %s(%s.%s.VX = %s.%s.VX)\n", vf13ad_solver, sc1, coord_name[0], sc2, coord_name[0]);
	fprintf(filePointer, "\tNonlinearConstraint %s(%s.%s.VY = %s.%s.VY)\n", vf13ad_solver, sc1, coord_name[0], sc2, coord_name[0]);
	fprintf(filePointer, "\tNonlinearConstraint %s(%s.%s.VZ = %s.%s.VZ)\n", vf13ad_solver, sc1, coord_name[0], sc2, coord_name[0]);

	fprintf(filePointer, "EndOptimize\n");
}


void write_gmat_optimizers(FILE *filePointer, int num_steps, struct ItinStep *step2pr) {
	step2pr = get_first(step2pr);
	char sc1[20];
	char var1[20];
	char sc2[20];
	char var2[20];
	sprintf(var2, "%s%i", sc_var[1], 1);
	sprintf(sc2, "%s%i_Arr", sc_name[1], 1);
	write_gmat_optimizer(filePointer, 1, sc_name[0], sc_var[0], coord_name[step2pr->body->id], body_name[step2pr->body->id], sc2, var2, coord_name[step2pr->next[0]->body->id], body_name[step2pr->next[0]->body->id], 1);
	for(int i = 1; i < num_steps-2; i++) {
		fprintf(filePointer, "\n");
		step2pr = step2pr->next[0];
		sprintf(var1, "%s%i", sc_var[1], i);
		sprintf(sc1, "%s%i_Dep", sc_name[1], i);
		sprintf(var2, "%s%i", sc_var[1], i+1);
		sprintf(sc2, "%s%i_Arr", sc_name[1], i+1);
		write_gmat_optimizer(filePointer, i+1, sc1, var1, coord_name[step2pr->body->id], body_name[step2pr->body->id], sc2, var2, coord_name[step2pr->next[0]->body->id], body_name[step2pr->next[0]->body->id], 0);
	}
	fprintf(filePointer, "\n");
	step2pr = step2pr->next[0];
	sprintf(var1, "%s%i", sc_var[1], num_steps-2);
	sprintf(sc1, "%s%i_Dep", sc_name[1], num_steps-2);
	write_gmat_optimizer(filePointer, num_steps-1, sc1, var1, coord_name[step2pr->body->id], body_name[step2pr->body->id], sc_name[2], sc_var[2], coord_name[step2pr->next[0]->body->id], body_name[step2pr->next[0]->body->id], 0);
}

void write_gmat_final_optimizer(FILE *filePointer, int num_steps, struct ItinStep *step2pr) {
	fprintf(filePointer, "\nOptimize 'Final Trajectory Optimization' %s {SolveMode = %s, ExitMode = DiscardAndContinue, ShowProgressWindow = true}\n", vf13ad_solver, solvermode);
	fprintf(filePointer, "\tloopIdx = loopIdx + 1\n\n");
	fprintf(filePointer, "\t%% Vary states\n");

	// fprintf(filePointer, "\tVary %s(%s_epoch  = %s_epoch)\n", vf13ad_solver, sc_var[0], sc_var[0]);
	fprintf(filePointer, "\tVary %s(%s_c3  = %s_c3)\n", vf13ad_solver, sc_var[0], sc_var[0]);
	fprintf(filePointer, "\tVary %s(%s_rha = %s_rha)\n", vf13ad_solver, sc_var[0], sc_var[0]);
	fprintf(filePointer, "\tVary %s(%s_dha = %s_dha)\n", vf13ad_solver, sc_var[0], sc_var[0]);
	for(int i = 1; i < num_steps; i++) {
		fprintf(filePointer, "\n");
		char var[20];
		if(i < num_steps-1) sprintf(var, "%s%i", sc_var[1], i);
		else sprintf(var, "%s", sc_var[2]);
		/*if(i > 1)*/ fprintf(filePointer, "\tVary %s(%s_epoch = %s_epoch)\n", vf13ad_solver, var, var);
		fprintf(filePointer, "\tVary %s(%s_c3  = %s_c3)\n", vf13ad_solver, var, var);
		fprintf(filePointer, "\tVary %s(%s_rha = %s_rha)\n", vf13ad_solver, var, var);
		fprintf(filePointer, "\tVary %s(%s_dha = %s_dha)\n", vf13ad_solver, var, var);
		if(i < num_steps-1) fprintf(filePointer, "\tVary %s(%s_bvazi  = %s_bvazi)\n", vf13ad_solver, var, var);
		if(i < num_steps-1) fprintf(filePointer, "\tVary %s(%s_periapsis = %s_periapsis)\n", vf13ad_solver, var, var);
	}

	set_gmat_all_sc_setup(filePointer, num_steps, step2pr, 1);

	step2pr = get_first(step2pr);

	fprintf(filePointer, "\n\t%% Delta-v from parking orbit\n");
	fprintf(filePointer, "\tCost = %s.%s.VMAG - sqrt(EarthGravParam/6571)\n", sc_name[0], coord_name[step2pr->body->id]);

	fprintf(filePointer, "\n\t%% Store inclinations\n");
	fprintf(filePointer, "\t%s_incl = %s.%s.INC\n", sc_var[0], sc_name[0], coord_name[step2pr->body->id]);
	for(int i = 1; i < num_steps-1; i++) {
		step2pr = step2pr->next[0];
		char sc[20];
		char var[20];
		sprintf(var, "%s%i", sc_var[1], i);
		sprintf(sc, "%s%i_Arr", sc_name[1], i);

		fprintf(filePointer, "\t%s_incl = %s.%s.INC\n", var, sc, coord_name[step2pr->body->id]);
	}
	step2pr = step2pr->next[0];
	fprintf(filePointer, "\t%s_incl = %s.%s.INC\n", sc_var[0], sc_name[0], coord_name[step2pr->body->id]);

	fprintf(filePointer, "\n\t%% Propagate all\n");
	fprintf(filePointer, "\tprop_time = mean_epoch%d - %s.UTCModJulian\n", 1, sc_name[0]);
	fprintf(filePointer, "\tPropagate 'PropForwards' %s(%s) {%s.ElapsedDays = prop_time}\n", propagator, sc_name[0], sc_name[0]);

	for(int i = 1; i < num_steps-1; i++) {
		char sc[20];
		sprintf(sc, "%s%i", sc_name[1], i);
		fprintf(filePointer, "\tprop_time = mean_epoch%d - %s_Arr.UTCModJulian\n", i, sc);
		fprintf(filePointer, "\tPropagate 'PropBackwards' BackProp %s(%s_Arr) {%s_Arr.ElapsedDays = prop_time}\n\n", propagator, sc, sc);
		fprintf(filePointer, "\tprop_time = mean_epoch%d - %s_Dep.UTCModJulian\n", i+1, sc);
		fprintf(filePointer, "\tPropagate 'PropForwards' %s(%s_Dep) {%s_Dep.ElapsedDays = prop_time}\n", propagator, sc, sc);
	}
	fprintf(filePointer, "\tprop_time = mean_epoch%d - %s.UTCModJulian\n", num_steps-1, sc_name[2]);
	fprintf(filePointer, "\tPropagate 'PropBackwards' BackProp %s(%s) {%s.ElapsedDays = prop_time}\n", propagator, sc_name[2], sc_name[2]);


	fprintf(filePointer, "\n\t%% Calculate Error in Position and Velocity\n");
	for(int i = 0; i < num_steps-1; i++) {
		int traj_id = i+1;
		char sc1[20];
		char sc2[20];
		if(i == 0) sprintf(sc1, "%s", sc_name[0]);
		else sprintf(sc1, "%s%i_Dep", sc_name[1], i);
		if(i >= num_steps-2) sprintf(sc2, "%s", sc_name[2]);
		else sprintf(sc2, "%s%i_Arr", sc_name[1], i+1);

		fprintf(filePointer,
				"\terrorR%d = sqrt((%s.%s.X - %s.%s.X)^2 + (%s.%s.Y - %s.%s.Y)^2 + (%s.%s.Z - %s.%s.Z)^2)\n",
				traj_id, sc1, coord_name[0], sc2, coord_name[0], sc1, coord_name[0], sc2, coord_name[0], sc1, coord_name[0], sc2, coord_name[0]);
		fprintf(filePointer,
				"\terrorV%d = sqrt((%s.%s.VX - %s.%s.VX)^2 + (%s.%s.VY - %s.%s.VY)^2 + (%s.%s.VZ - %s.%s.VZ)^2)\n",
				traj_id, sc1, coord_name[0], sc2, coord_name[0], sc1, coord_name[0], sc2, coord_name[0], sc1, coord_name[0], sc2, coord_name[0]);
	}


	fprintf(filePointer, "\n\t%% Apply the collocation constraints constraints on final states");
	for(int i = 0; i < num_steps-1; i++) {
		char sc1[20];
		char sc2[20];
		if(i == 0) sprintf(sc1, "%s", sc_name[0]);
		else sprintf(sc1, "%s%i_Dep", sc_name[1], i);
		if(i >= num_steps-2) sprintf(sc2, "%s", sc_name[2]);
		else sprintf(sc2, "%s%i_Arr", sc_name[1], i+1);


		fprintf(filePointer, "\n\tNonlinearConstraint %s(%s.%s.X  = %s.%s.X)\n", vf13ad_solver, sc1, coord_name[0], sc2, coord_name[0]);
		fprintf(filePointer, "\tNonlinearConstraint %s(%s.%s.Y  = %s.%s.Y)\n", vf13ad_solver, sc1, coord_name[0], sc2, coord_name[0]);
		fprintf(filePointer, "\tNonlinearConstraint %s(%s.%s.Z  = %s.%s.Z)\n", vf13ad_solver, sc1, coord_name[0], sc2, coord_name[0]);
		fprintf(filePointer, "\tNonlinearConstraint %s(%s.%s.VX = %s.%s.VX)\n", vf13ad_solver, sc1, coord_name[0], sc2, coord_name[0]);
		fprintf(filePointer, "\tNonlinearConstraint %s(%s.%s.VY = %s.%s.VY)\n", vf13ad_solver, sc1, coord_name[0], sc2, coord_name[0]);
		fprintf(filePointer, "\tNonlinearConstraint %s(%s.%s.VZ = %s.%s.VZ)\n", vf13ad_solver, sc1, coord_name[0], sc2, coord_name[0]);
	}

	fprintf(filePointer, "\n\tMinimize %s(Cost)\n", vf13ad_solver);
	fprintf(filePointer, "EndOptimize\n");

	fprintf(filePointer, "\nReport RFDepStates %s_epoch %s_c3 %s_rha %s_dha %s_bvazi %s_periapsis\n",
			sc_var[0], sc_var[0], sc_var[0], sc_var[0], sc_var[0], sc_var[0]);
	char *coord = coord_name[0];
	for(int i = 1; i < num_steps-1; i++) {
		char sc[20];
		sprintf(sc, "%s%i_Arr", sc_name[1], i);
		fprintf(filePointer, "Report RFDepStates %s.UTCModJulian %s.%s.X %s.%s.Y %s.%s.Z %s.%s.VX %s.%s.VY %s.%s.VZ\n",
				sc, sc, coord, sc, coord, sc, coord, sc, coord, sc, coord, sc, coord);
	}
	fprintf(filePointer, "Report RFDepStates %s.UTCModJulian %s.%s.X %s.%s.Y %s.%s.Z %s.%s.VX %s.%s.VY %s.%s.VZ %s_epoch\n",
			sc_name[2], sc_name[2], coord, sc_name[2], coord, sc_name[2], coord, sc_name[2], coord, sc_name[2], coord, sc_name[2], coord, sc_var[2]);
}