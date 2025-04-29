#ifndef KSP_EPHEM_H
#define KSP_EPHEM_H

#include "celestial_bodies.h"

/**
 * @brief Represents the ephemeral data consisting of date, position, and velocity components
 */
struct Ephem {
	double date; /**< Date associated with the ephemeral data (Julian Date) */
	double x;    /**< X-coordinate position */
	double y;    /**< Y-coordinate position */
	double z;    /**< Z-coordinate position */
	double vx;   /**< Velocity along the X-axis */
	double vy;   /**< Velocity along the Y-axis */
	double vz;   /**< Velocity along the Z-axis */
};


/**
 * @brief Prints the date, position and velocity vector of the given ephemeris
 *
 * @param ephem The given ephemeris
 */
void print_ephem(struct Ephem ephem);


/**
 * @brief Retrieves the ephemeral data of requested body for requested time (from JPL's Horizon API or from file)
 *
 * @param body The body for which the ephemerides should be stored
 * @param central_body The central body of the body's system
 */
void get_body_ephems(struct Body *body, struct Body *central_body);


/**
 * @brief Find the last ephemeris before the given date
 *
 * @param ephem List of all ephemeral data
 * @param date The given date (Julian Date)
 *
 * @return The last ephemeris before given date
 */
struct Ephem get_closest_ephem(struct Ephem *ephem, double date);

#endif //KSP_EPHEM_H
