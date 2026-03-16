#ifndef WYPE_CPU_FEATURES_H
#define WYPE_CPU_FEATURES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void cpuid( uint32_t eax, uint32_t* eax_out, uint32_t* ebx_out, uint32_t* ecx_out, uint32_t* edx_out );

int has_aes_ni( void );

#ifdef __cplusplus
}
#endif

#endif /* WYPE_CPU_FEATURES_H */
