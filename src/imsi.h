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
 * @file <imsi.h>
 *
 * @brief Contains declarations for functions for retrieving imsi numbers
 *
 *****************************************************************************/

#ifndef CPC_IMSI_H
#define CPC_IMSI_H

#include <glib.h>

typedef void *cpc_imsi_handle_t;

typedef void (*cpc_imsi_cb_t)(int result, gchar **imsis, void *user_data);

void cpc_imsi_get(cpc_imsi_cb_t cb, void *user_data, cpc_imsi_handle_t *handle);
void cpc_imsi_get_cancel(cpc_imsi_handle_t handle);

#endif
