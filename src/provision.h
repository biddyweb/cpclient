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
 * @file <provision.h>
 *
 * @brief Contains definitions for provision an OMA CP context
 *
 *****************************************************************************/

#ifndef CPC_PROVISION_H
#define CPC_PROVISION_H

typedef void (*cpc_provision_cb_t)(void *user_data, int result);
typedef void *cpc_provision_handle_t;

void cpc_provision_apply(cpc_context_t *context,
			 const char *imsi,
			 cpc_provision_cb_t callback,
			 void *user_data,
			 cpc_provision_handle_t *handle);

void cpc_provision_apply_cancel(cpc_provision_handle_t handle);

#endif
