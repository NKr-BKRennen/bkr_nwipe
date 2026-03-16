/*
 *  prng.h: Pseudo Random Number Generator abstractions for wype.
 *
 *  Copyright Darik Horn <dajhorn-dban@vanadac.com>.
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

#ifndef PRNG_H_
#define PRNG_H_

#include <sys/types.h>

/* A chunk of random data. */
typedef struct
{
    size_t length;  // Length of the entropy string in bytes.
    u8* s;  // The actual bytes of the entropy string.
} wype_entropy_t;

#define WYPE_PRNG_INIT_SIGNATURE void **state, wype_entropy_t *seed
#define WYPE_PRNG_READ_SIGNATURE void **state, void *buffer, size_t count

/* Function pointers for PRNG actions. */
typedef int ( *wype_prng_init_t )( WYPE_PRNG_INIT_SIGNATURE );
typedef int ( *wype_prng_read_t )( WYPE_PRNG_READ_SIGNATURE );

/* The generic PRNG definition. */
typedef struct
{
    const char* label;  // The name of the pseudo random number generator.
    wype_prng_init_t init;  // Inialize the prng state with the seed.
    wype_prng_read_t read;  // Read data from the prng.
} wype_prng_t;

typedef struct
{
    const wype_prng_t* prng;
    double mbps;
    double seconds;
    unsigned long long bytes;
    int rc;
} wype_prng_bench_result_t;

/* Existing API (kept for compatibility: no live output) */
int wype_prng_benchmark_all( double seconds_per_prng,
                              size_t io_block_bytes,
                              wype_prng_bench_result_t* results,
                              size_t results_count );

/* New API: live output (spinner + per-PRNG immediate prints)
 * live_print:
 *   0 = behave like old benchmark (silent, just fills results[])
 *   1 = print "Analysing PRNG performance:" immediately, rotate cursor,
 *       print "Testing <PRNG>..." before each PRNG, and print result right after.
 */
int wype_prng_benchmark_all_live( double seconds_per_prng,
                                   size_t io_block_bytes,
                                   wype_prng_bench_result_t* results,
                                   size_t results_count,
                                   int live_print );

const wype_prng_t* wype_prng_select_fastest( double seconds_per_prng,
                                               size_t io_block_bytes,
                                               wype_prng_bench_result_t* results,
                                               size_t results_count );

/* Mersenne Twister prototypes. */
int wype_twister_init( WYPE_PRNG_INIT_SIGNATURE );
int wype_twister_read( WYPE_PRNG_READ_SIGNATURE );

/* ISAAC prototypes. */
int wype_isaac_init( WYPE_PRNG_INIT_SIGNATURE );
int wype_isaac_read( WYPE_PRNG_READ_SIGNATURE );
int wype_isaac64_init( WYPE_PRNG_INIT_SIGNATURE );
int wype_isaac64_read( WYPE_PRNG_READ_SIGNATURE );

/* ALFG prototypes. */
int wype_add_lagg_fibonacci_prng_init( WYPE_PRNG_INIT_SIGNATURE );
int wype_add_lagg_fibonacci_prng_read( WYPE_PRNG_READ_SIGNATURE );

/* XOROSHIRO-256 prototypes. */
int wype_xoroshiro256_prng_init( WYPE_PRNG_INIT_SIGNATURE );
int wype_xoroshiro256_prng_read( WYPE_PRNG_READ_SIGNATURE );

/* SplitMix64 PRNG. */
int wype_splitmix64_prng_init( WYPE_PRNG_INIT_SIGNATURE );
int wype_splitmix64_prng_read( WYPE_PRNG_READ_SIGNATURE );

/* AES-CTR-NI prototypes. */
int wype_aes_ctr_prng_init( WYPE_PRNG_INIT_SIGNATURE );
int wype_aes_ctr_prng_read( WYPE_PRNG_READ_SIGNATURE );

/* ChaCha20 stream cipher CSPRNG */
int wype_chacha20_prng_init( WYPE_PRNG_INIT_SIGNATURE );
int wype_chacha20_prng_read( WYPE_PRNG_READ_SIGNATURE );

/* Size of the twister is not derived from the architecture, but it is strictly 4 bytes */
#define SIZE_OF_TWISTER 4

/* Size of the isaac/isaac64 is not derived from the architecture, but it is strictly 4 or 8 bytes */
#define SIZE_OF_ISAAC 4
#define SIZE_OF_ISAAC64 8

/* Size of the Lagged Fibonacci generator is not derived from the architecture, but it is strictly 32 bytes */
#define SIZE_OF_ADD_LAGG_FIBONACCI_PRNG 32

/* Size of the XOROSHIRO-256 is not derived from the architecture, but it is strictly 32 bytes */
#define SIZE_OF_XOROSHIRO256_PRNG 32

/* AES-CTR generation chunk size: fixed 128 KiB (not architecture-dependent) */
#define SIZE_OF_AES_CTR_PRNG ( 128 * 1024 )

/* Thread-local prefetch ring buffer capacity: 1 MiB */
#define STASH_CAPACITY ( 1024 * 1024 )

#endif /* PRNG_H_ */
