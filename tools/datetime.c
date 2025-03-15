#include "datetime.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define J2000_UT0 2451545.0		// 2000-01-01T12:00
#define J2000_UT1950 2433282.5	// 1950-01-01T00:00

void print_date(struct Date date, int line_break) {
	switch(date.date_type) {
		case DATE_ISO:
			printf("%4d-%02d-%02d %02d:%02d:%06.3f    [ISO]", date.y, date.m, date.d, date.h, date.min, date.s);
			break;
		case DATE_KERBAL:
			printf("%4d-%03d %02d:%02d:%06.3f    [KERBAL]", date.y, date.d, date.h, date.min, date.s);
			break;
		case DATE_KERBALISO:
			printf("%4d-%03d %02d:%02d:%06.3f    [ISO-LIKE KERBAL]", date.y, date.d, date.h, date.min, date.s);
			break;
	}
	if(line_break) printf("\n");
}

int is_string_valid_date_format(const char *s, enum DateType date_type) {
	int y, m, d, matches;
	switch(date_type) {
		case DATE_ISO:
			matches = sscanf(s, "%d-%d-%d", &y, &m, &d);
			if (matches != 3) return 0;
			if (m < 1 || m > 12 || d < 1 || d > 31) return 0;
			break;
		case DATE_KERBAL:
			matches = sscanf(s, "%d-%d", &y, &d);
			if (matches != 2) return 0;
			if (d < 1 || d > 426) return 0;
			break;
		case DATE_KERBALISO:
			matches = sscanf(s, "%d-%d", &y, &d);
			if (matches != 2) return 0;
			if (d < 1 || d > 365) return 0;
			break;
	}
	return 1; // Date format is valid
}

void date_to_string(struct Date date, char *s, int clocktime) {
	if(date.date_type == DATE_ISO)
		sprintf(s, "%4d-%02d-%02d", date.y, date.m, date.d);
	else
		sprintf(s, "%4d-%03d", date.y, date.d);
	if(clocktime) sprintf(s,"%s %02d:%02d:%02.0f", s, date.h, date.min, date.s);
}

struct Date date_from_string(char *s, enum DateType date_type) {
	int year, month, day;	// "yyyy-mm-dd" (ISO 8601)  "yyyy-ddd" (Kerbal and ISO-like Kerbal)
	char *ptr;

	// Extract year
	year = (int) strtol(s, &ptr, 10);
	ptr++; // Move past the '-'

	// Extract month (only with ISO time)
	if(date_type == DATE_ISO) {
		month = (int) strtol(ptr, &ptr, 10);
		ptr++; // Move past the '-'
	} else month = 0;

	// Extract day
	day = (int) strtol(ptr, &ptr, 10);
	struct Date date = {year, month, day, .date_type=date_type};
	return date;
}

struct Date convert_JD_date_iso(double JD) {
	struct Date date = {0,1,1,0,0,0, DATE_ISO};

	JD -= J2000_UT0-0.5;	// subtract 2000-01-01T00:00
	date.y = 2000;
	if(JD < 0) {
		date.y--;
		while(JD < -365 - (date.y % 4 == 0)) {
			JD += 365 + (date.y % 4 == 0);	// boolean to add one (for leap year)
			date.y--;
		}
		JD += 365 + (date.y % 4 == 0);	// boolean to add one (for leap year)
	} else {
		while(JD > 365 + (date.y % 4 == 0)) {
			JD -= 365 + (date.y % 4 == 0);	// boolean to add one (for leap year)
			date.y++;
		}
	}

	for(int i = 1; i < 12; i++) {
		int month_days;
		if(i == 1 || i == 3 || i == 5 || i == 7 || i == 8 || i == 10) month_days = 31;
		else if(i == 4 || i == 6 || i == 9 || i == 11) month_days = 30;
		else {
			if(date.y%4 == 0) month_days = 29;
			else month_days = 28;
		}
		JD -= month_days;
		if(JD < 0) {
			JD += month_days;
			break;
		} else {
			date.m++;
		}
	}

	date.d += (int)JD;
	JD -= (int)JD;

	// years before a leap year before 2000 would go to 32 Dec
	if(date.m == 12 && date.d == 32) {
		date.m = 1;
		date.d = 1;
		date.y++;
	}

	date.h = (int) (JD * 24.0);
	JD -= (double)date.h/24;
	date.min = floor(JD * 24 * 60);
	JD -= (double)date.min/(24*60);
	date.s = JD*86400;
	if(date.s > 59.999) {
		date.s = 0;
		date.min++;
		if(date.min > 59.999) {
			date.min = 0;
			date.h++;
		}
	}
	return date;
}

struct Date convert_JD_date_kerbal(double JD, enum DateType date_type) {
	struct Date date = {.y = 1, .d = 1, .date_type = date_type};

	if(date_type == DATE_KERBAL) JD *= 24.0/6.0;	// Kerbal time has only 6 hours --> 4 times more days

	date.y += (int)(JD / (date_type == DATE_KERBAL ? 426 : 365));

	if(JD >= 0)	JD -= (date.y-1) * (date_type == DATE_KERBAL ? 426 : 365);	// negative 1 because starts with 1
	else {
		date.y--; // negative 1 because starts with 1
		JD -= (date.y-1) * (date_type == DATE_KERBAL ? 426 : 365);	// because jd=-10 should become day=416/355
	}

	date.d += (int)JD;
	JD -= (int)JD;

	if(date_type == DATE_KERBAL) JD *= 6.0/24.0;	// reverse above because has no impact on hours/mins/secs

	date.h = (int) (JD * 24);
	JD -= (double)date.h/24;
	date.min = floor(JD * 24 * 60);
	JD -= (double)date.min/(24 * 60);
	date.s = JD*(24*60*60);
	if(date.s > 59.999) {
		date.s = 0;
		date.min++;
		if(date.min > 59.999) {
			date.min = 0;
			date.h++;
			if(date.h >= (date_type == DATE_KERBAL ? 6 : 24)) {
				date.d++;
				date.h -= (date_type == DATE_KERBAL ? 6 : 24);
				if(date.d > (date_type == DATE_KERBAL ? 426 : 365)) {
					date.y++;
					date.d -= (date_type == DATE_KERBAL ? 426 : 365);
				}
			}
		}
	}
	return date;
}

struct Date convert_JD_date(double JD, enum DateType date_type) {
	if(date_type == DATE_ISO) return convert_JD_date_iso(JD);
	else return convert_JD_date_kerbal(JD, date_type);
}

double convert_date_JD_iso(struct Date date) {
	double J = J2000_UT0-0.5;     // 2000-01-01 00:00
	int diff_year = date.y-2000;
	int year_part = diff_year * 365.25;
	if(date.y < 2000 || date.y%4 == 0) J -= 1; // leap years do leap day themselves after 2000

	int month_part = 0;
	for(int i = 1; i < 12; i++) {
		if(i==date.m) break;
		if(i == 1 || i == 3 || i == 5 || i == 7 || i == 8 || i == 10) month_part += 31;
		else if(i == 4 || i == 6 || i == 9 || i == 11) month_part += 30;
		else {
			if(date.y%4 == 0) month_part += 29;
			else month_part += 28;
		}
	}

	J += month_part+year_part+date.d;
	J += (double)date.h/24 + (double)date.min/(24*60) + date.s/(24*60*60);
	return J;
}

double convert_date_JD_kerbal(struct Date date) {
	// -1 for year and day because UT0: 0001-01T00:00
	double J = (date.y-1)*(date.date_type == DATE_KERBAL ? 426 : 365);
	J += date.d-1;

	if(date.date_type == DATE_KERBAL) J = J/4;

	J += (double)date.h/24;
	J += (double)date.min/(24*60);
	J += date.s/(24*60*60);
	return J;
}

double convert_date_JD(struct Date date) {
	if(date.date_type == DATE_ISO) return convert_date_JD_iso(date);
	else return convert_date_JD_kerbal(date);
}

double jd_change_date(double jd, int delta_years, int delta_months, double delta_days, enum DateType date_type) {
	jd += delta_days;
	struct Date date = convert_JD_date(jd, date_type);

	// Kerbal and ISO-like Kerbal have no months
	if(date_type == DATE_ISO) {
		date.m += delta_months;
		while(date.m > 12) {
			date.m -= 12;
			date.y++;
		}
		while(date.m < 1) {
			date.m += 12;
			date.y--;
		}
	}

	date.y += delta_years;
	jd = convert_date_JD(date);
	return jd;
}

struct Date get_date_difference_from_epochs(double jd0, double jd1, enum DateType date_type) {
	double epoch_diff = jd1 - jd0;
	struct Date date = {0, 0};
	// floating-point imprecision when converting to seconds 1 -> 0.999997
	if(fmod(epoch_diff*(24*60*60), 1) > 0.9) epoch_diff += 1.0/(24*60*60*10);
	if(fmod(epoch_diff*(24*60*60), 1) < -0.9) epoch_diff -= 1.0/(24*60*60*10);

	if(date_type == DATE_KERBAL) epoch_diff *= 24.0/6.0;	// Kerbal day is 6 hours instead of 24 hours
	date.d = (int) epoch_diff;
	epoch_diff -= date.d;
	if(date_type == DATE_KERBAL) epoch_diff *= 6.0/24.0;	// hours/mins/secs are not affected by shorter day
	date.h = (int) (epoch_diff*24) % 24;
	date.min = (int) (epoch_diff*24*60) % 60;
	date.s = (int) (epoch_diff*(24*60*60)) % 60;
	return date;
}

struct Date change_date_type(struct Date date, enum DateType new_date_type) {
	double jd = convert_date_JD(date);
	if(date.date_type == DATE_ISO && new_date_type != DATE_ISO) jd -= J2000_UT1950;
	if(date.date_type != DATE_ISO && new_date_type == DATE_ISO) jd += J2000_UT1950;
	return convert_JD_date(jd, new_date_type);
}