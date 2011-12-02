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
 * @file <file-peer.h>
 *
 * @brief Contains prototypes for file utility functions.
 *
 *****************************************************************************/

#ifndef CPC_FILE_PEER_H
#define CPC_FILE_PEER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

int cpc_file_get_binary(const char *path, size_t *file_len, uint8_t **data_buf);

#ifdef __cplusplus
}
#endif

#endif
