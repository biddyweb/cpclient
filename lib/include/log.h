/*
 * CPClient
 *
 * Copyright (C) 2011 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Mark Ryan <mark.d.ryan@intel.com>
 *
 */

/*!
 * @file <log.h>
 *
 * @brief Macros and functions for logging
 *
 ******************************************************************************/

#ifndef CPC_LOG_H
#define CPC_LOG_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef CPC_LOGGING

#include <stdbool.h>
#include <stdint.h>

int cpc_log_open(const char *log_file_name);
void cpc_logf(bool decorate, const char *message, ...);
void cpc_logb(const uint8_t *data, unsigned int data_len);
void cpc_log_close(void);

#define CPC_LOG_OPEN(fname) cpc_log_open(fname)
#define CPC_LOGF(message, ...) cpc_logf(true, message, __LINE__,	\
					      __FILE__, ##__VA_ARGS__)
#define CPC_LOGUF(message, ...) cpc_logf(false, message, ##__VA_ARGS__)
#define CPC_LOGB(data, data_len) cpc_logb(data, data_len)
#define CPC_LOG_CLOSE() cpc_log_close()

#else

#define CPC_LOG_OPEN(fname)
#define CPC_LOGF(message, ...)
#define CPC_LOGUF(message, ...)
#define CPC_LOGB(data, data_len)
#define CPC_LOG_CLOSE()

#endif

#ifdef __cplusplus
}
#endif

#endif
