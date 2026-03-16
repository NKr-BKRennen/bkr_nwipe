/*
 *  temperature.h: The header file for disk drive temperature sensing
 *
 *  This program is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free Software
 *  Foundation, version 2.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef TEMPERATURE_H_
#define TEMPERATURE_H_

#include <sys/types.h>
#include "context.h"

/**
 * This function is called after each wype_context_t has been created.
 * It initialises the temperature variables in each context and then
 * constructs a path that is placed in the context that points to the
 * appropriate /sys/class/hwmon/hwmonX directory that corresponds with
 * the particular drive represented in the context structure.
 * @param pointer to a drive context
 * @return returns 0 on success < 1 on error
 */
int wype_init_temperature( wype_context_t* );

void wype_update_temperature( wype_context_t* );

/**
 * Workaround for obtaining temperatures from SCSI/SAS drives
 * @param pointer to a drive context
 * @return returns 0 on success < 1 on error
 */
int wype_init_scsi_temperature( wype_context_t* );
int wype_get_scsi_temperature( wype_context_t* );
void wype_shut_scsi_temperature( wype_context_t* );
void* wype_update_temperature_thread( void* ptr );

/**
 * This function is normally called only once. It's called after both the
 * wype_init_temperature() function and wype_update_temperature()
 * functions have been called. It logs the drives critical, highest, lowest
 * and lowest critical temperatures. Not all drives report four temperatures.
 * @param pointer to a drive context
 */
void wype_log_drives_temperature_limits( wype_context_t* );

#define NUMBER_OF_FILES 7

#define NO_TEMPERATURE_DATA 1000000

#endif /* TEMPERATURE_H_ */
