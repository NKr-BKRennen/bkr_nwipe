/*
 *  send_email.h: Send PDF certificate via email after wipe completion (wype)
 *
 *  This program is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free Software
 *  Foundation, version 2.
 */

#ifndef SEND_EMAIL_H_
#define SEND_EMAIL_H_

#include "context.h"

/**
 * Send a short notification email that wipe has finished.
 * Tells the recipient how many disks succeeded/failed and to confirm at the device.
 *
 * @param c             Array of device contexts
 * @param count         Number of devices
 * @return 0 on success, -1 on failure or if email is disabled
 */
int wype_send_summary_notification( wype_context_t** c, int count );

/**
 * Send all PDF certificates in one email with multiple attachments.
 * Deletes local PDF files after successful send.
 * If send fails, local PDFs are kept.
 *
 * @param c             Array of device contexts (each has PDF_filename)
 * @param count         Number of devices
 * @return 0 on success, -1 on failure or if email is disabled
 */
int wype_send_all_certificates( wype_context_t** c, int count );

#endif /* SEND_EMAIL_H_ */
