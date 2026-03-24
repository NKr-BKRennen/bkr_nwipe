/*
 *  smart.h: Read SMART attributes via smartctl.
 */

#ifndef SMART_H_
#define SMART_H_

#include "context.h"

/**
 * Populate the smart_* fields in the device context by running smartctl.
 * Safe to call for any device type; sets smart_available = 0 if smartctl
 * fails or is not installed.
 */
void wype_read_smart( wype_context_t* c );

#endif /* SMART_H_ */
