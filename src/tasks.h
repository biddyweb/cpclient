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
 * @file <tasks.h>
 *
 * @brief Contains definitions for the cpclient tasks
 *
 *****************************************************************************/

#ifndef CPC_TASKS_H
#define CPC_TASKS_H

#include <stdint.h>
#include <stdbool.h>
#include <glib.h>
#include <gio/gio.h>

#include "pm-manager.h"
#include "callback.h"

enum cpc_task_type_t_ {
	CPC_TASK_PARSECP,
	CPC_TASK_CREATE_PM,
	CPC_TASK_GET_VERSION,
	CPC_TASK_CLOSE_PM,
	CPC_TASK_GET_PROPS,
	CPC_TASK_APPLY
};

typedef enum cpc_task_type_t_ cpc_task_type_t;

typedef struct cpc_task_t_ cpc_task_t;
struct cpc_task_t_ {
	cpc_task_type_t type;
	GDBusMethodInvocation *invocation;
	gchar *path;
	gchar *pin;
	uint8_t *wp_message;
	unsigned int wp_message_len;
};

typedef void *cpc_tasks_handle_t;

typedef void (*cpc_tasks_cb_t)(int result, cpc_tasks_handle_t handle,
			       void *user_data);

void cpc_task_free(cpc_task_t *task);

bool cpc_tasks_parsecp(cpc_task_t *task, cpc_cb_t finished,
		       void *finished_data, cpc_handle_t *handle);

void cpc_tasks_parsecp_cancel(cpc_tasks_handle_t handle);

void cpc_tasks_create_pm(cpc_task_t *task, cpc_pm_manager_t *pm_manager);

void cpc_tasks_close_pm(cpc_task_t *task, cpc_pm_manager_t *pm_manager);

void cpc_tasks_get_props(cpc_task_t *task, cpc_pm_manager_t *pm_manager);

bool cpc_tasks_apply(cpc_task_t *task, cpc_pm_manager_t *pm_manager,
		     cpc_cb_t finished, void *finished_data,
		     cpc_tasks_handle_t *handle);

void cpc_tasks_apply_cancel(cpc_pm_manager_t *pm_manager,
			    cpc_tasks_handle_t handle);

void cpc_tasks_get_version(cpc_task_t *task);


#endif
