#include "launch_state.h"
#include <stdlib.h>

struct LaunchState * new_launch_state() {
	struct LaunchState *new_state = (struct LaunchState*) malloc(sizeof(struct LaunchState));
	new_state->next = NULL;
	new_state->prev = NULL;
	new_state->r = vec(0,0,0);
	new_state->v = vec(0,0,0);
	new_state->t = 0;

	return new_state;
}

struct LaunchState * append_launch_state(struct LaunchState *curr_state) {
	struct LaunchState *new_state = new_launch_state();
	new_state->prev = curr_state;
	if(curr_state != NULL) curr_state->next = new_state;
	return new_state;
}

struct LaunchState * get_first_state(struct LaunchState *state) {
	if(state == NULL) return state;
	while(state->prev != NULL) state = state->prev;
	return state;
}

struct LaunchState * get_last_state(struct LaunchState *state) {
	if(state == NULL) return state;
	while(state->next != NULL) state = state->next;
	return state;
}

void free_launch_states(struct LaunchState *state) {
	if(state == NULL) return;
	state = get_first_state(state);
	while(state->next != NULL) {
		state = state->next;
		free(state->prev);
	}
	free(state);
}