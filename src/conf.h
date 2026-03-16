#ifndef CONF_H_
#define CONF_H_

/**
 * Initialises the libconfig code, called once at the
 * start of wype, prior to any attempts to access
 * wype's config file /etc/wype/wype.conf
 * @param none
 * @return int
 *   0  = success
 *   -1 = error
 */
int wype_conf_init();

/**
 * Before exiting wype, this function should be called
 * to free up libconfig's memory usage
 * @param none
 * @return void
 */
void wype_conf_close();

void save_selected_customer( char** );

/**
 * int wype_conf_update_setting( char *, char * );
 * Use this function to update a setting in wype.conf
 * @param char * this is the group name and setting name separated by a period '.'
 *               i.e "PDF_Certificate.PDF_Enable"
 * @param char * this is the setting, i.e ENABLED
 * @return int 0 = Success
 *             1 = Unable to update memory copy
 *             2 = Unable to write new configuration to /etc/wype/wype.conf
 */
int wype_conf_update_setting( char*, char* );

/**
 * int wype_conf_read_setting( char *, char *, const char ** )
 * Use this function to read a setting value in wype.conf
 * @param char * this is the group name
 * @param char * this is the setting name
 * @param char ** this is a pointer to the setting value
 * @return int 0 = Success
 *             -1 = Unable to find the specified group name
 *             -2 = Unable to find the specified setting name
 */
int wype_conf_read_setting( char*, const char** );

int wype_conf_populate( char* path, char* value );

#define FIELD_LENGTH 256
#define NUMBER_OF_FIELDS 4
#define MAX_GROUP_DEPTH 4

#endif /* CONF_H_ */
