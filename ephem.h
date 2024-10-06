#ifndef KSP_EPHEM_H
#define KSP_EPHEM_H

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
 * @brief Represents a date and time with year, month, day, hour, minute, and second components
 */
struct Date {
	int y;       /**< Year */
	int m;       /**< Month */
	int d;       /**< Day */
	int h;       /**< Hour */
	int min;     /**< Minute */
	double s;    /**< Seconds */
};


/**
 * @brief Prints date in the format YYYY-MM-DD hh:mm:ss.f (ISO 8601)
 *
 * @param date The date to be printed
 * @param line_break Is 0 if no line break should follow and 1 if otherwise
 */
void print_date(struct Date date, int line_break);


/**
 * @brief Returns a string with the date (ISO 8601)
 *
 * @param date The date to be converted to a string
 * @param s The string the date should be saved in
 * @param clocktime Set to 1 if clocktime should be shown, 0 if only date should be shown
 */
void date_to_string(struct Date date, char *s, int clocktime);


/**
 * @brief Parses string and returns date (ISO 8601) (excluding time)
 *
 * @param date The date to be converted to a string
 * @param s The string the date should be saved in
 * @param clocktime Set to 1 if clocktime should be shown, 0 if only date should be shown
 */
struct Date date_from_string(char *s);


/**
 * @brief Converts date format to Julian Date
 *
 * @param date The date to be converted
 *
 * @return The Julian Date
 */
double convert_date_JD(struct Date date);


/**
 * @brief Converts Julian Date to date format
 *
 * @param JD The Julian Date to be converted
 *
 * @return The resulting Date struct
 */
struct Date convert_JD_date(double JD);


/**
 * @brief changes the Julian Date by the delta time given
 *
 * @param delta_years Years to add or subtract
 * @param delta_months Months to add or subtract
 * @param delta_days Days to add or subtract
 */
double jd_change_date(double jd, int delta_years, int delta_months, double delta_days);


/**
 * @brief calculates the date difference (days, hours, minutes, seconds) between two julian dates
 *
 * @param jd0 Initial Epoch / Julian Date
 * @param jd1 Second Epoch / Julian Date (if greater than jd0, result is positive)
 */
struct Date get_date_difference_from_epochs(double jd0, double jd1);


/**
 * @brief Prints the date, position and velocity vector of the given ephemeris
 *
 * @param ephem The given ephemeris
 */
void print_ephem(struct Ephem ephem);


/**
 * @brief Retrieves the ephemeral data of requested body for requested time (from JPL's Horizon API or from file)
 *
 * @param ephem The array where all the ephemeral data is to be stored
 * @param size_ephem The biggest size of the ephemeris array
 * @param body_code The body code of the requested celestial body (from JPL's Horizon Application)
 * @param time_steps The size of the time steps between each ephemeris
 * @param jd0 The earliest requested date (Julian Date)
 * @param jd1 The latest requested date (Julian Date)
 * @param download Is 1 if the data is to be retrieved from JPL's Horizon API and is 0 if the data is to be retrieved from file
 */
void get_ephem(struct Ephem *ephem, int size_ephem, int body_code, int time_steps, double jd0, double jd1, int download);


void get_body_ephem(struct Ephem *ephem, int body_code);


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
