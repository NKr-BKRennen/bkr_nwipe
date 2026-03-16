/*
 *  pass.h: Pass-related I/O routines.
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

#ifndef PASS_H_
#define PASS_H_

#include "context.h" /* wype_context_t */
#include "method.h" /* wype_pattern_t */

int wype_random_pass( wype_context_t* c );
int wype_random_verify( wype_context_t* c );
int wype_static_pass( wype_context_t* c, wype_pattern_t* pattern );
int wype_static_verify( wype_context_t* c, wype_pattern_t* pattern );

#endif /* PASS_H_ */
