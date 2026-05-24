#include "mission_tool.h"


MissionStep * create_mission_step() {
	CelestSystem *system = get_system_by_name("Solar System (Ephemeris)");
	MissionStep *step = malloc(sizeof(MissionStep));
	step->type = MISSIONSTEP_TYPE_ORBIT;
	step->prev = NULL;
	step->next = NULL;
	step->orbit.body = get_body_by_name("Earth", system);
	step->orbit.osv.r = vec3(9000e3, 0, 8000e3);
	step->orbit.osv.v = vec3(0, 8000, 0);
	return step;
}


void free_mission(MissionStep *step) {
	step = get_first_mission_step(step);
	while(step->next) {
		step = step->next;
		free(step->prev);
	}
	free(step);
}

MissionStep * get_first_mission_step(MissionStep *step) {
	if(!step) return NULL;
	while(step->prev) {
		step = step->prev;
	}
	return step;
}

MissionStep * get_last_mission_step(MissionStep *step) {
	if(!step) return NULL;
	while(step->next) {
		step = step->next;
	}
	return step;
}
