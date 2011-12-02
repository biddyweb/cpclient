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
 * @file <pm_manager.h>
 *
 * @brief Contains declarations for the push message class
 *
 *****************************************************************************/

#ifndef CPC_PM_MANAGER_H
#define CPC_PM_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#include <glib.h>
#include <gio/gio.h>

#include "callback.h"

typedef struct cpc_pm_manager_t_ cpc_pm_manager_t;


typedef struct cpc_props_t_ cpc_props_t;
struct cpc_props_t_ {
	bool pin_required;
	gchar *settings;
	const gchar *sec_type;
	gchar *start_sessions;
};

void cpc_props_free(cpc_props_t *props);

void cpc_pm_manager_new(GDBusInterfaceInfo *interface,
			const GDBusInterfaceVTable *vtable,
			gpointer user_data,
			cpc_pm_manager_t **manager);
int cpc_pm_manager_new_message(cpc_pm_manager_t *manager,
			       const gchar *client_name,
			       GDBusConnection *connection,
			       uint8_t *data, unsigned int length,
			       gchar **path);
int cpc_pm_manager_get_properties(cpc_pm_manager_t *manager,
				  const gchar *path,
				  const gchar* client_name,
				  cpc_props_t *props);
int cpc_pm_manager_remove_message(cpc_pm_manager_t *manager,
				  const gchar *path,
				  const gchar *client_name);
void cpc_pm_manager_delete(cpc_pm_manager_t *manager);

unsigned int cpc_pm_manager_message_count(cpc_pm_manager_t *manager);

void cpc_pm_manager_lost_client(cpc_pm_manager_t *manager,
				const gchar *name);

int cpc_pm_manager_apply(cpc_pm_manager_t *manager,
			 const gchar *path, const gchar* client_name,
			 const gchar *pin, cpc_cb_t finished,
			 void *finished_data, cpc_handle_t *handle);

void cpc_pm_manager_apply_cancel(cpc_pm_manager_t *manager,
				 cpc_handle_t handle);


#endif
