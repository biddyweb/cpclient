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
 * @file <provision-cp.c>
 *
 * @brief Code to parse and apply and OMA CP XML file.  Used mainly for testing.
 *
 ******************************************************************************/

#include "config.h"

#include <stdlib.h>

#include "log.h"
#include "error.h"
#include "error-macros.h"
#include "file-peer.h"
#include "context.h"
#include "provision.h"

#include "dbus-error.h"
#include "provision-cp.h"

typedef struct cpc_tasks_cp_context_ cpc_tasks_cp_context;
struct cpc_tasks_cp_context_ {
	cpc_cb_t finished;
	void *finished_data;
	int result;
	cpc_context_t *context;
	guint finished_source;
	cpc_handle_t prov_handle;
};

static void prv_cpc_tasks_cp_context_delete(cpc_tasks_cp_context *task_context)
{
	if (task_context) {
		cpc_context_delete(task_context->context);
		g_free(task_context);
	}
}

static gboolean prv_provision_finished(gpointer user_data)
{
	cpc_tasks_cp_context *task_context = user_data;

	task_context->finished(task_context->result,
			       task_context->finished_data);
	prv_cpc_tasks_cp_context_delete(task_context);

	return FALSE;
}

static void prv_provision_cb(void *user_data, int result)
{
	cpc_tasks_cp_context *task_context = user_data;
	task_context->result = result;
	task_context->finished_source =
		g_idle_add(prv_provision_finished, user_data);
}

int cpc_provision_cp_apply(const gchar *fname, cpc_cb_t finished,
			   void *finished_data, cpc_handle_t *handle)
{
	CPC_ERR_MANAGE;
	uint8_t *buffer = NULL;
	size_t file_size;
	cpc_tasks_cp_context *task_context;
	cpc_context_t *context;
	cpc_provision_handle_t prov_handle;

	CPC_LOGF("Process CP task");

	task_context = g_new0(cpc_tasks_cp_context, 1);

	CPC_ERR = cpc_file_get_binary(fname, &file_size, &buffer);

	if (CPC_ERR != CPC_ERR_NONE) {
		CPC_LOGF("Unable to load CP file: %s", fname);
		goto CPC_ON_ERR;
	}

	if (file_size > INT_MAX) {
		CPC_LOGF("File: %s too large", fname);
		CPC_FAIL_FORCE(CPC_ERR_CORRUPT);
	}

	CPC_ERR = cpc_context_new((const char*) buffer,
				  file_size, &context);

	if (CPC_ERR != CPC_ERR_NONE) {
		CPC_LOGF("Unable to parse %s, err = %d",
			      fname, CPC_ERR);
		goto CPC_ON_ERR;
	}

	cpc_provision_apply(context, "", prv_provision_cb, task_context,
			    &prov_handle);

	task_context->context = context;
	task_context->finished = finished;
	task_context->finished_data = finished_data;
	task_context->prov_handle = prov_handle;
	*handle = task_context;

	free(buffer);

	return CPC_ERR_NONE;

CPC_ON_ERR:

	prv_cpc_tasks_cp_context_delete(task_context);
	free(buffer);

	return CPC_ERR;
}

void cpc_provision_cp_apply_cancel(cpc_handle_t handle)
{
	cpc_tasks_cp_context *task_context = handle;

	if (task_context->prov_handle && !task_context->finished_source)
		cpc_provision_apply_cancel(task_context->prov_handle);
}
