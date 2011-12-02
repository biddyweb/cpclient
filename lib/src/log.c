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
 * @file <log.c>
 *
 * @brief Functions for logging
 *
 ******************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

#include "config.h"
#include "log.h"
#include "error.h"
#include "error-macros.h"

#ifdef CPC_LOGGING
static FILE *g_log_file;

int cpc_log_open(const char *log_file_name)
{
	CPC_ERR_MANAGE;

	if (!g_log_file)
		CPC_FAIL_NULL(g_log_file, fopen(log_file_name, "w"),
			      CPC_ERR_OPEN);
CPC_ON_ERR:

	return CPC_ERR;
}

void cpc_log_close()
{
	if (g_log_file)
		fclose(g_log_file);
}

void cpc_logf(bool decorate, const char *message, ...)
{
	va_list args;
	unsigned int line_number;
	const char *file_name;

	if (!g_log_file)
		goto on_error;

	va_start(args, message);
	if (decorate) {
		line_number = va_arg(args, unsigned int);
		file_name = va_arg(args, const char*);

		if (fprintf(g_log_file, "%s:%u ", file_name, line_number) < 0)
			goto on_error;
	}

	if (vfprintf(g_log_file, message, args) < 0)
		goto on_error;

	if (fprintf(g_log_file, "\n") < 0)
		goto on_error;

	va_end(args);
	(void) fflush(g_log_file);

on_error:

	return;
}

void cpc_logb(const uint8_t *data, unsigned int data_len)
{
	const unsigned int buffer_size = 16;
	const unsigned int buffer_size_in_chars = (buffer_size * 4) + 1;
	char buffer[buffer_size_in_chars];
	unsigned int bytes_left = data_len;
	unsigned int i;
	unsigned int row_size;
	unsigned int master_data_index = 0;
	unsigned int data_index;
	uint8_t byte;
	unsigned int buffer_index;

	if (!g_log_file)
		goto on_error;

	while (bytes_left > 0) {
		data_index = master_data_index;
		row_size = bytes_left < buffer_size ? bytes_left : buffer_size;
		buffer_index = 0;

		for (i = 0; i < row_size; ++i) {
			sprintf(&buffer[buffer_index], "%02x ",
				data[data_index++]);
			buffer_index += 3;
		}

		for (; i < buffer_size; ++i) {
			buffer[buffer_index++] = ' ';
			buffer[buffer_index++] = ' ';
			buffer[buffer_index++] = ' ';
		}

		data_index = master_data_index;
		for (i = 0; i < row_size; ++i) {
			byte = data[data_index++];
			if (isprint(byte))
				buffer[buffer_index] = byte;
			else
				buffer[buffer_index] = '.';
			++buffer_index;
		}
		buffer[buffer_index] = 0;

		if (fprintf(g_log_file, "%s\n", buffer) < 0)
			goto on_error;

		master_data_index = data_index;
		bytes_left -= row_size;
	}

	(void) fflush(g_log_file);

on_error:

	return;
}


#endif
