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
 * @file <tasks.c>
 *
 * @brief Main file for the cpclient's tasks.
 *
 ******************************************************************************/

#include "config.h"

#include <syslog.h>

#include <glib.h>

#include "error.h"
#include "error-macros.h"
#include "log.h"

#include "tasks.h"
#include "dbus-error.h"
#include "provision-cp.h"

typedef struct cpc_task_data_t_ cpc_task_data_t;
struct cpc_task_data_t_ {
	GDBusMethodInvocation *invocation;
	cpc_cb_t finished;
	void *finished_data;
};

void cpc_task_free(cpc_task_t *task)
{
	if (task) {
		g_free(task->path);
		g_free(task->pin);
		g_free(task->wp_message);
		if (task->invocation)
			g_dbus_method_invocation_return_error(
				task->invocation, G_IO_ERROR,
				G_IO_ERROR_FAILED_HANDLED,
				cpc_dbus_error_map(CPC_ERR_DIED));
		g_free(task);
	}
}

static void prv_task_finished(int result, void *user_data)
{
	cpc_task_data_t *callback_data = user_data;

	if (result == CPC_ERR_NONE)
		g_dbus_method_invocation_return_value(callback_data->invocation,
						      NULL);
	else
		g_dbus_method_invocation_return_dbus_error(
			callback_data->invocation,
			cpc_dbus_error_map(result), "");

	CPC_LOGF("Async task finished with err %u", result);
	syslog(LOG_INFO, "Async task finished with err %u", result);

	callback_data->finished(result, callback_data->finished_data);

	g_free(callback_data);
}

bool cpc_tasks_parsecp(cpc_task_t *task, cpc_cb_t finished,
		       void *finished_data, cpc_handle_t *handle)
{
	CPC_ERR_MANAGE;
	cpc_task_data_t *callback_data;

	callback_data = g_new(cpc_task_data_t, 1);
	callback_data->finished = finished;
	callback_data->finished_data = finished_data;
	callback_data->invocation = task->invocation;

	CPC_LOGF("Starting ParseCP task");
	syslog(LOG_INFO, "Starting ParseCP task");

	CPC_FAIL(cpc_provision_cp_apply(task->path, prv_task_finished,
					callback_data, handle));

	task->invocation = NULL;

	return true;

CPC_ON_ERR:

	g_free(callback_data);
	g_dbus_method_invocation_return_dbus_error(
		task->invocation, cpc_dbus_error_map(CPC_ERR), "");
	task->invocation = NULL;

	return false;
}

void cpc_tasks_parsecp_cancel(cpc_tasks_handle_t handle)
{
	cpc_provision_cp_apply_cancel(handle);
}

void cpc_tasks_create_pm(cpc_task_t *task, cpc_pm_manager_t *pm_manager)
{
	CPC_ERR_MANAGE;
	gchar *path;
	GDBusConnection *connection =
		g_dbus_method_invocation_get_connection(task->invocation);
	const gchar *client_name =
		g_dbus_method_invocation_get_sender(task->invocation);

	CPC_FAIL(cpc_pm_manager_new_message(pm_manager,
						 client_name,
						 connection,
						 task->wp_message,
						 task->wp_message_len,
						 &path));

	CPC_LOGF("New Push Message object created %s", path);
	syslog(LOG_INFO, "New Push Message object created %s", path);

	g_dbus_method_invocation_return_value(
		task->invocation, g_variant_new("(o)", path));
	g_free(path);
	task->invocation = NULL;
	return;

CPC_ON_ERR:

	CPC_LOGF("Failed to create Push Message object");
	syslog(LOG_INFO, "Failed to create Push Message object");

	g_dbus_method_invocation_return_dbus_error(
		task->invocation, cpc_dbus_error_map(CPC_ERR), "");
	task->invocation = NULL;
}

void cpc_tasks_close_pm(cpc_task_t *task, cpc_pm_manager_t *pm_manager)
{
	CPC_ERR_MANAGE;
	const gchar *client_name =
		g_dbus_method_invocation_get_sender(task->invocation);

	CPC_FAIL(cpc_pm_manager_remove_message(pm_manager, task->path,
					       client_name));

	CPC_LOGF("Push Message object %s removed", task->path);
	syslog(LOG_INFO, "Push Message object %s removed", task->path);

	g_dbus_method_invocation_return_value(task->invocation, NULL);
	task->invocation = NULL;
	return;

CPC_ON_ERR:
	CPC_LOGF("Failed to remove Push Message Object %s", task->path);
	syslog(LOG_INFO, "Failed to remove Push Message Object %s", task->path);

	g_dbus_method_invocation_return_dbus_error(
		task->invocation, cpc_dbus_error_map(CPC_ERR), "");
	task->invocation = NULL;
}

void cpc_tasks_get_props(cpc_task_t *task, cpc_pm_manager_t *pm_manager)
{
	CPC_ERR_MANAGE;
	GVariantBuilder *vb;
	const gchar *pin_required_str;
	cpc_props_t props;
	const gchar *client_name =
		g_dbus_method_invocation_get_sender(task->invocation);

	CPC_FAIL(cpc_pm_manager_get_properties(pm_manager, task->path,
					       client_name, &props));

	pin_required_str = props.pin_required ? "Yes" : "No";

	CPC_LOGF("Pin Required %s", pin_required_str);
	CPC_LOGF("Settings %s", props.settings);
	CPC_LOGF("Sec Type %s", props.sec_type);
	CPC_LOGF("Start Sessions with %s", props.start_sessions);

	syslog(LOG_INFO, "GetProps (%s, %s, %s %s) succeeded.",
	       pin_required_str, props.settings, props.sec_type,
	       props.start_sessions);

	vb = g_variant_builder_new(G_VARIANT_TYPE("a{ss}"));
	g_variant_builder_add(vb, "{ss}", "PinRequired", pin_required_str);
	g_variant_builder_add(vb, "{ss}", "Settings", props.settings);
	g_variant_builder_add(vb, "{ss}", "SecType", props.sec_type);
	g_variant_builder_add(vb, "{ss}", "StartSessionsWith",
			      props.start_sessions);
	cpc_props_free(&props);

	g_dbus_method_invocation_return_value(
		task->invocation, g_variant_new("(@a{ss})",
						g_variant_builder_end(vb)));
	g_variant_builder_unref(vb);
	task->invocation = NULL;
	return;

CPC_ON_ERR:

	CPC_LOGF("Failed to retrieve properties for %s err %u", task->path,
		 CPC_ERR);
	syslog(LOG_INFO, "Failed to retrieve properties for %s err %u",
	       task->path, CPC_ERR);

	g_dbus_method_invocation_return_dbus_error(
		task->invocation, cpc_dbus_error_map(CPC_ERR), "");
	task->invocation = NULL;
}

bool cpc_tasks_apply(cpc_task_t *task, cpc_pm_manager_t *pm_manager,
		     cpc_cb_t finished, void *finished_data,
		     cpc_tasks_handle_t *handle)
{
	CPC_ERR_MANAGE;
	cpc_task_data_t *callback_data;
	const gchar *client_name =
		g_dbus_method_invocation_get_sender(task->invocation);

	callback_data = g_new(cpc_task_data_t, 1);
	callback_data->finished = finished;
	callback_data->finished_data = finished_data;
	callback_data->invocation = task->invocation;

	CPC_FAIL(cpc_pm_manager_apply(pm_manager, task->path, client_name,
				      task->pin, prv_task_finished,
				      callback_data, handle));
	task->invocation = NULL;

	syslog(LOG_INFO, "Starting Apply task");
	CPC_LOGF("Starting Apply task");

	return true;

CPC_ON_ERR:

	g_free(callback_data);
	g_dbus_method_invocation_return_dbus_error(
		task->invocation, cpc_dbus_error_map(CPC_ERR), "");
	task->invocation = NULL;

	return false;
}

void cpc_tasks_apply_cancel(cpc_pm_manager_t *pm_manager,
			    cpc_tasks_handle_t handle)
{
	cpc_pm_manager_apply_cancel(pm_manager, handle);
}

void cpc_tasks_get_version(cpc_task_t *task)
{
	CPC_LOGF("Get CPClient Version %s", VERSION);
	g_dbus_method_invocation_return_value(task->invocation,
					      g_variant_new("(s)", VERSION));
	task->invocation = NULL;
}
