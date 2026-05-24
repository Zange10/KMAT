#ifndef KMAT_MISSION_TOOL_H
#define KMAT_MISSION_TOOL_H

#include "tools/celestial_systems.h"


enum MissionStepType {
	MISSIONSTEP_TYPE_ORBIT,
	MISSIONSTEP_TYPE_MANOEUVRE,
	MISSIONSTEP_TYPE_TRANSFER
};

typedef struct MissionStep {
	struct MissionStep *prev, *next;
	enum MissionStepType type;

	union {
		struct MissionOrbit {
			double date;
			Body *body;
			OSV osv;
		} orbit;

		struct MissionManoeuvre {
			double date;
			Body *body;
			Vector3 r, v0, v1;
		} manoeuvre;
	};
} MissionStep;

MissionStep * create_mission_step();
void free_mission(MissionStep *step);
MissionStep * get_first_mission_step(MissionStep *step);
MissionStep * get_last_mission_step(MissionStep *step);

#endif //KMAT_MISSION_TOOL_H
