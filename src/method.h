/*
 *  methods.c: Method implementations for wype.
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

#ifndef METHOD_H_
#define METHOD_H_

/* The argument list for wype methods. */
#define WYPE_METHOD_SIGNATURE wype_context_t* c

typedef enum wype_verify_t_ {
    WYPE_VERIFY_NONE = 0,  // Do not read anything back from the device.
    WYPE_VERIFY_LAST,  // Check the last pass.
    WYPE_VERIFY_ALL,  // Check all passes.
} wype_verify_t;

/* The typedef of the function that will do the wipe. */
typedef int ( *wype_method_t )( void* ptr );

typedef struct
{
    int length;  // Length of the pattern in bytes, -1 means random.
    char* s;  // The actual bytes of the pattern.
} wype_pattern_t;

const char* wype_method_label( void* method );
int wype_runmethod( WYPE_METHOD_SIGNATURE, wype_pattern_t* patterns );

void* wype_dod522022m( void* ptr );
void* wype_dodshort( void* ptr );
void* wype_gutmann( void* ptr );
void* wype_ops2( void* ptr );
void* wype_is5enh( void* ptr );
void* wype_random( void* ptr );
void* wype_zero( void* ptr );
void* wype_one( void* ptr );
void* wype_verify_zero( void* ptr );
void* wype_verify_one( void* ptr );
void* wype_bruce7( void* ptr );
void* wype_bmb( void* ptr );
void* wype_secure_erase( void* ptr );
void* wype_secure_erase_prng_verify( void* ptr );
void* wype_sanitize_crypto_erase( void* ptr );
void* wype_sanitize_crypto_erase_verify( void* ptr );
void* wype_sanitize_block_erase( void* ptr );
void* wype_sanitize_overwrite( void* ptr );

void calculate_round_size( wype_context_t* );

#endif /* METHOD_H_ */
