#ifndef KSP_DATETIME_H
#define KSP_DATETIME_H

/**
 * @brief ISO ("Earth time"; UT0 = 2000-01-01T12:00; 1y = 12M = 365.25d; 1d = 24h)  -  Kerbal time (UT0 = 0001-001T00:00; 1y = 426d; 1d = 6h)  -  Kerbal imitating ISO (UT0 = 0001-001T00:00; 1y = 365d; 1d = 24h)
 */
enum DateType {DATE_ISO, DATE_KERBAL, DATE_KERBALISO};

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
	enum DateType date_type;    /**< Type of date (ISO ("Earth time"), Kerbal time or ISO-like Kerbal time (Kerbal times don't have months) */
};


/**
 * @brief Prints date in the format [ISO] YYYY-MM-DD hh:mm:ss.f (ISO 8601), [KER] YYYY-DDD hh:mm:ss (Kerbal time) or [ILK] YYYY-DDD hh:mm:ss (ISO-like Kerbal time)
 *
 * @param date The date to be printed
 * @param line_break Is 0 if no line break should follow and 1 if otherwise
 */
void print_date(struct Date date, int line_break);


/**
 * @brief Checks whether a string is in the correct date format
 *
 * @param s The string to be parsed into date
 * @param date_type Type of the date (ISO, Kerbal, ISO-like Kerbal)
 *
 * @return 1 if valid format, 0 otherwise
 */
int is_string_valid_date_format(const char *s, enum DateType date_type);


/**
 * @brief Returns a string with the date (ISO 8601 or Kerbal time)
 *
 * @param date The date to be converted to a string
 * @param s The string the date should be saved in
 * @param clocktime Set to 1 if clocktime should be shown, 0 if only date should be shown
 */
void date_to_string(struct Date date, char *s, int clocktime);


/**
 * @brief Parses string and returns date (ISO 8601 or Kerbal time) (excluding time)
 *
 * @param s The string to be parsed into date
 * @param date_type Type of the date (ISO, Kerbal, ISO-like Kerbal)
 *
 * @return The date parsed from the given string
 */
struct Date date_from_string(char *s, enum DateType date_type);


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
 * @param date_type Type of the date (ISO, Kerbal, ISO-like Kerbal)
 *
 * @return The resulting Date struct
 */
struct Date convert_JD_date(double JD, enum DateType date_type);


/**
 * @brief changes the Julian Date by the delta time given
 *
 * @param delta_years Years to add or subtract
 * @param delta_months Months to add or subtract
 * @param delta_days Days to add or subtract
 * @param date_type Type of the date (ISO, Kerbal, ISO-like Kerbal)
 *
 * @return The changed Julian Date
 */
double jd_change_date(double jd, int delta_years, int delta_months, double delta_days, enum DateType date_type);


/**
 * @brief calculates the date difference (days, hours, minutes, seconds) between two julian dates
 *
 * @param jd0 Initial Epoch / Julian Date
 * @param jd1 Second Epoch / Julian Date (if greater than jd0, result is positive)
 * @param date_type Type of the date (ISO, Kerbal, ISO-like Kerbal)
 *
 * @return The difference between Julian Dates as a Date (years, days, hours, minutes, seconds)
 */
struct Date get_date_difference_from_epochs(double jd0, double jd1, enum DateType date_type);


/**
 * @brief changes the type of the date
 *
 * @param date The date to be changed
 * @param new_date_type New date type (ISO, Kerbal, ISO-like Kerbal)
 *
 * @return The date with the desired date type
 */
struct Date change_date_type(struct Date date, enum DateType new_date_type);

#endif //KSP_DATETIME_H
