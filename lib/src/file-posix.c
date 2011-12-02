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

/******************************************************************************
 * Copyright (c) 1999-2008 ACCESS CO., LTD. All rights reserved.
 * Copyright (c) 2006 PalmSource, Inc (an ACCESS company). All rights reserved.
 *****************************************************************************/

/*!
 * @file <file-posix.c>
 *
 * @brief Main source file for various file utility functions
 *
 * The functions prv_file_open_and_get_size and prv_read_document are based on
 * original ACCESS code taken from omadm_process_cp.c.  The algorithms have
 * not been changed.  However, the identifiers used in these functions have been
 * modified and the formatting has been changed to match the cpclient coding
 * standards.
 *
 * cpc_file_get_binary is new Intel code.
 *****************************************************************************/

#include "config.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "error.h"
#include "error-macros.h"
#include "log.h"
#include "file-peer.h"

static int prv_file_open_and_get_size(const char *path, int *fd,
				      size_t *file_len)
{
	CPC_ERR_MANAGE;
	struct stat stats;
	int fd_copy = open(path, O_RDONLY);

	if (fd_copy < 0) {
		CPC_LOGF("Failed to open file %s", path);
		CPC_FAIL_FORCE(CPC_ERR_OPEN);
	}

	if (fstat(fd_copy, &stats) < 0)	{
		CPC_LOGF("Failed to stat file %s", path);
		CPC_FAIL_FORCE(CPC_ERR_OPEN);
	}

	*fd = fd_copy;
	*file_len = stats.st_size;

	return CPC_ERR_NONE;

CPC_ON_ERR:

	if (fd_copy != -1)
		close(fd_copy);

	return CPC_ERR;
}

static int prv_read_document(int fd, size_t data_size, char **data_buf)
{
	CPC_ERR_MANAGE;
	char *buffer = NULL;
	size_t read_len = 0;
	int rcvd_len = 0;
	size_t to_read = data_size;

	/* Read data */

	CPC_LOGF("Reading file: Data size = %ld", data_size);

	CPC_FAIL_NULL(buffer, malloc(data_size + 1), CPC_ERR_OOM);

	while (to_read > 0) {
		rcvd_len = read(fd, buffer + read_len, to_read - read_len);

		if (rcvd_len < 0) {
			CPC_LOGF("Error: Cannot read from fd", fd);
			CPC_FAIL_FORCE(CPC_ERR_READ);
		} else if (rcvd_len == 0) {
			CPC_LOGF("Error: Unexpected EOF");
			CPC_FAIL_FORCE(CPC_ERR_READ);
		}

		read_len += rcvd_len;
		to_read -=rcvd_len;
	}

	buffer[data_size] = 0;
	*data_buf = buffer;

	return CPC_ERR;

CPC_ON_ERR:

	CPC_LOGF("Reading file: err = 0x%x", CPC_ERR);

	free(buffer);

	return CPC_ERR;
}

int cpc_file_get_binary(const char *path, size_t *file_len, uint8_t **data_buf)
{
	CPC_ERR_MANAGE;
	int fd = -1;
	size_t file_size;
	char* buffer;

	CPC_FAIL(prv_file_open_and_get_size(path, &fd, &file_size));

	CPC_FAIL(prv_read_document(fd, file_size, &buffer));

	*file_len = file_size;
	*data_buf = (uint8_t *) buffer;

CPC_ON_ERR:

	if (fd != -1)
		close(fd);

	return CPC_ERR;
}
