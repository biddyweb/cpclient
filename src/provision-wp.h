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
 * @file <provision_wp.h>
 *
 * @brief Contains function declarations for provisioning OMA CP WAP Push
 *        messages.
 *
 *****************************************************************************/

#ifndef CPC_PROVISION_WP_H
#define CPC_PROVISION_WP_H

#include <glib.h>

#include "wp.h"
#include "context.h"
#include "callback.h"

typedef struct cpc_provision_wp_t_ cpc_provision_wp_t;

int cpc_provision_wp_new(uint8_t *data, unsigned int length,
			 cpc_provision_wp_t **provision);
void cpc_provision_wp_delete(cpc_provision_wp_t *provision);
bool cpc_provision_wp_pin_required(cpc_provision_wp_t *provision);
gchar *cpc_provision_wp_get_settings(cpc_provision_wp_t *provision);
const gchar *cpc_provision_wp_get_sec_type(cpc_provision_wp_t *provision);
gchar *cpc_provision_wp_get_sessions(cpc_provision_wp_t *provision);
int cpc_provision_wp_apply(cpc_provision_wp_t *provision, const gchar *pin,
			   cpc_cb_t cb,void *user_data);
void cpc_provision_wp_apply_cancel(cpc_provision_wp_t *provision);


#endif
