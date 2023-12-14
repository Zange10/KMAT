#ifndef CSW_WRITER
#define CSW_WRITER

/**
 * @brief Writes a .csv-file with fields and their data
 *
 * @param fields A string with the fields separated by commas
 * @param data The data to be stored in the .csv-file
 */
void write_csv(char fields[], double data[]);

/**
 * @brief Calculates the amount of given fields in given string
 *
 * @param fields A string with the fields separated by commas
 */
int amt_of_fields(char fields[]);


#endif
