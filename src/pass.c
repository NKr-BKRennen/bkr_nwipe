/*
 *  pass.c: Pass-related I/O routines.
 *
 *  Copyright Darik Horn <dajhorn-dban@vanadac.com>.
 *
 *  Modifications to original dwipe Copyright Andy Beverley <andy@andybev.com>
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

#include "pass.h"
#include "pass_internal.h"

/* Logs the I/O direction for the respective operation. */
static void wype_log_io_direction( wype_context_t* c )
{
    wype_log( WYPE_LOG_NOTICE,
               "I/O direction on '%s' is %s",
               c->device_name,
               c->io_direction == WYPE_IO_DIRECTION_FORWARD ? "start -> end (forward)" : "end -> start (reverse)" );
}

/* Calls the static_*_pass method for the respective c->io_direction. */
int wype_static_pass( wype_context_t* c, wype_pattern_t* pattern )
{
    wype_log_io_direction( c );
    return c->io_direction == WYPE_IO_DIRECTION_FORWARD ? wype_static_forward_pass( c, pattern )
                                                         : wype_static_reverse_pass( c, pattern );
}

/* Calls the static_*_verify method for the respective c->io_direction. */
int wype_static_verify( wype_context_t* c, wype_pattern_t* pattern )
{
    wype_log_io_direction( c );
    return c->io_direction == WYPE_IO_DIRECTION_FORWARD ? wype_static_forward_verify( c, pattern )
                                                         : wype_static_reverse_verify( c, pattern );
}

/* Calls the random_*_pass method for the respective c->io_direction. */
int wype_random_pass( wype_context_t* c )
{
    wype_log_io_direction( c );
    return c->io_direction == WYPE_IO_DIRECTION_FORWARD ? wype_random_forward_pass( c )
                                                         : wype_random_reverse_pass( c );
}

/* Calls the random_*_verify method for the respective c->io_direction. */
int wype_random_verify( wype_context_t* c )
{
    wype_log_io_direction( c );
    return c->io_direction == WYPE_IO_DIRECTION_FORWARD ? wype_random_forward_verify( c )
                                                         : wype_random_reverse_verify( c );
}
