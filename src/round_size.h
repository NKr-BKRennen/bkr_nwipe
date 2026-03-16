#ifndef ROUND_SIZE_H_
#define ROUND_SIZE_H_

#include <stdint.h>

typedef enum {
    WYPE_ROUND_VERIFY_NONE = 0,
    WYPE_ROUND_VERIFY_LAST = 1,
    WYPE_ROUND_VERIFY_ALL = 2,
} wype_round_verify_t;

typedef enum {
    WYPE_ROUND_METHOD_DEFAULT = 0,
    WYPE_ROUND_METHOD_OPS2 = 1,
    WYPE_ROUND_METHOD_IS5ENH = 2,
} wype_round_method_class_t;

uint64_t wype_calculate_round_size_bytes( uint64_t base_pass_size,
                                           uint64_t device_size,
                                           int rounds,
                                           int noblank,
                                           wype_round_verify_t verify,
                                           wype_round_method_class_t method_class,
                                           uint64_t* effective_pass_size_out );

#endif /* ROUND_SIZE_H_ */
