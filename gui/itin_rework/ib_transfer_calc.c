#include "ib_transfer_calc.h"


void calc_interpolation_based_itins() {
	// define Departure group

	// --

	// Departure groups find group boundaries wrt min_dur and max_dur

	// Determine dv boundary for dep groups

	// Quad generation and splitting wrt boundary

	// Departure group boundary matching

	// Populate Quads

	// D&C wrt Arrival vinf

	// --

	// Next groups find group boundaries wrt respective min_dur and max_dur

	// Determine Vinf line (fb-date vs min_dv)

	// Determine Vinf boundary of next groups

	// Combine prev boundary and vinf boundary

	// Refining prev group inside new boundary wrt vinf and angle

	// Quad generation and splitting of next groups

	// Boundary matching of next groups

	// Populate Quads with duration

	// D&C wrt duration

	// (Remove Quads with invalid duration)

	// Populate Quads with RPE

	// D&C wrt RPE

	// Determine RPE Boundary

	// Combine previous boundary with RPE Boundary

	// D&C Arr vinf

	// --

	// For Arrival: D&C vinf and angle
}