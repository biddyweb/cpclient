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
 * @file <cpclient.c>
 *
 * @brief Main file for the cpclient.  Contains code for mainloop and exposes
 *        D-Bus services
 *
 ******************************************************************************/

#include "config.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <syslog.h>

#include "log.h"
#include "error.h"
#include "error-macros.h"
#include "pm-manager.h"

#include "tasks.h"

#define CPC_INTERFACE_GET_VERSION "GetVersion"
#define CPC_INTERFACE_PARSECP "ParseCP"
#define CPC_INTERFACE_FN "filename"
#define CPC_INTERFACE_CREATE_PUSH_MESSAGE "CreatePushMessage"
#define CPC_INTERFACE_BYTE_ARRAY "byte-array"
#define CPC_INTERFACE_APPLY "Apply"
#define CPC_INTERFACE_PIN "Pin"
#define CPC_INTERFACE_CLOSE "Close"
#define CPC_INTERFACE_PIN_REQUIRED "PinRequired"
#define CPC_INTERFACE_SETTINGS "Settings"
#define CPC_INTERFACE_PATH "Path"
#define CPC_INTERFACE_GET_PROPS "GetProps"
#define CPC_INTERFACE_DICT "dict"
#define CPC_INTERFACE_VERSION "version"

#define CPC_TIMEOUT 30*1000

typedef struct cpc_context_t_ cpc_context_t;
struct cpc_context_t_ {
	int error;
	guint cpclient_id;
	guint owner_id;
	guint sig_id;
	GDBusNodeInfo *node_info;
	GMainLoop *main_loop;
	GDBusConnection *connection;
	guint timeout_id;
	GPtrArray *tasks;
	guint idle_id;
	cpc_tasks_handle_t cp_handle;
	cpc_tasks_handle_t apply_handle;
	bool quitting;
	cpc_pm_manager_t *pm_manager;
	GHashTable *watchers;
};

static const gchar g_cpc_introspection[] =
	"<node>"
	"  <interface name='"CPC_INTERFACE_MANAGER"'>"
	"    <method name='"CPC_INTERFACE_GET_VERSION"'>"
	"      <arg type='s' name='"CPC_INTERFACE_VERSION"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"CPC_INTERFACE_PARSECP"'>"
	"      <arg type='s' name='"CPC_INTERFACE_FN"'"
	" direction='in'/>"
	"    </method>"
	"    <method name='"CPC_INTERFACE_CREATE_PUSH_MESSAGE"'>"
	"      <arg type='ay' name='"CPC_INTERFACE_BYTE_ARRAY"'"
	" direction='in'/>"
	"      <arg type='o' name='"CPC_INTERFACE_PATH"'"
	"           direction='out'/>"
	"    </method>"
	"  </interface>"
	"  <interface name='"CPC_INTERFACE_PUSH_MESSAGE"'>"
	"    <method name='"CPC_INTERFACE_APPLY"'>"
	"      <arg type='s' name='"CPC_INTERFACE_PIN"' direction='in'/>"
	"    </method>"
	"    <method name='"CPC_INTERFACE_CLOSE"'>"
	"    </method>"
	"    <method name='"CPC_INTERFACE_GET_PROPS"'>"
	"      <arg type='a{ss}' name='"CPC_INTERFACE_DICT"'"
	"           direction='out'/>"
	"    </method>"
	"  </interface>"
	"</node>";

static gboolean prv_process_task(gpointer user_data);

static bool prv_async_in_progress(cpc_context_t *context)
{
	return context->cp_handle != NULL && context->apply_handle != NULL;
}

static void prv_free_cpc_task(gpointer data)
{
	cpc_task_free(data);
}

static void prv_parsecp_task_finished(int result, void* user_data)
{
	cpc_context_t *context = user_data;
	context->cp_handle = NULL;
	if (context->quitting)
		g_main_loop_quit(context->main_loop);
	else
		context->idle_id = g_idle_add(prv_process_task, context);
}

static void prv_apply_task_finished(int result, void* user_data)
{
	cpc_context_t *context = user_data;
	context->apply_handle = NULL;
	if (context->quitting)
		g_main_loop_quit(context->main_loop);
	else
		context->idle_id = g_idle_add(prv_process_task, context);
}

static gboolean prv_timeout(gpointer user_data)
{
	cpc_context_t *context = user_data;

	g_main_loop_quit(context->main_loop);
	context->timeout_id = 0;

	CPC_LOGF("Exiting.");

	return FALSE;
}

static gboolean prv_process_task(gpointer user_data)
{
	cpc_context_t *context = user_data;
	cpc_task_t *task;
	bool async_task = false;
	gboolean retval = FALSE;

	if (context->tasks->len > 0) {
		task = g_ptr_array_index(context->tasks, 0);
		if (task) {
			switch (task->type) {
			case CPC_TASK_PARSECP:
				async_task =
					cpc_tasks_parsecp(
						task,
						prv_parsecp_task_finished,
						user_data,
						&context->cp_handle);
				break;
			case CPC_TASK_APPLY:
				async_task =
					cpc_tasks_apply(
						task, context->pm_manager,
						prv_apply_task_finished,
						user_data,
						&context->apply_handle);
				break;
			case CPC_TASK_GET_VERSION:
				cpc_tasks_get_version(task);
				break;
			case CPC_TASK_CREATE_PM:
				cpc_tasks_create_pm(task, context->pm_manager);
				break;
			case CPC_TASK_CLOSE_PM:
				cpc_tasks_close_pm(task, context->pm_manager);
				break;
			case CPC_TASK_GET_PROPS:
				cpc_tasks_get_props(task, context->pm_manager);
			default:
				break;
			}
		}
		g_ptr_array_remove_index(context->tasks, 0);
	}

	if (!async_task) {
		if ((context->tasks->len == 0) &&
		    (cpc_pm_manager_message_count(context->pm_manager) == 0)) {
			if (!context->timeout_id) {
				CPC_LOGF("Nothing left to do. "
					 "Exiting in %u millseconds",
					 CPC_TIMEOUT);
				context->timeout_id =
					g_timeout_add(CPC_TIMEOUT,
						      prv_timeout,
						      context);
			}
			context->idle_id = 0;
		} else if (context->tasks->len > 0) {
			retval =  TRUE;
		} else {
			context->idle_id = 0;
		}
	} else {
		context->idle_id = 0;
	}

	return retval;
}

static void prv_cpc_method_call(GDBusConnection *connection,
				const gchar *sender,
				const gchar *object_path,
				const gchar *interface_name,
				const gchar *method_name,
				GVariant *parameters,
				GDBusMethodInvocation *invocation,
				gpointer user_data);

static void prv_cpc_pm_method_call(GDBusConnection *connection,
				   const gchar *sender,
				   const gchar *object_path,
				   const gchar *interface_name,
				   const gchar *method_name,
				   GVariant *parameters,
				   GDBusMethodInvocation *invocation,
				   gpointer user_data);

static const GDBusInterfaceVTable g_cpc_vtable =
{
	prv_cpc_method_call,
	NULL,
	NULL
};

static const GDBusInterfaceVTable g_cpc_pm_vtable =
{
	prv_cpc_pm_method_call,
	NULL,
	NULL
};

static void prv_cpc_context_init(cpc_context_t *context)
{
	memset(context, 0, sizeof(*context));
}

static void prv_cpc_context_free(cpc_context_t *context)
{
	if (context->watchers)
		g_hash_table_unref(context->watchers);

	if (context->pm_manager)
		cpc_pm_manager_delete(context->pm_manager);

	if (context->tasks)
		g_ptr_array_unref(context->tasks);

	if (context->idle_id)
		(void) g_source_remove(context->idle_id);

	if (context->sig_id)
		(void) g_source_remove(context->sig_id);

	if (context->connection) {
		if (context->cpclient_id)
			g_dbus_connection_unregister_object(
				context->connection,
				context->cpclient_id);
	}

	if (context->timeout_id)
		(void) g_source_remove(context->timeout_id);

	if (context->main_loop)
		g_main_loop_unref(context->main_loop);

	if (context->owner_id)
		g_bus_unown_name(context->owner_id);

	if (context->connection)
		g_object_unref(context->connection);

	if (context->node_info)
		g_dbus_node_info_unref(context->node_info);
}

static void prv_lost_client(GDBusConnection *connection, const gchar *name,
			    gpointer user_data)
{
	cpc_context_t *context = user_data;
	unsigned int i;
	const gchar *client_name;
	cpc_task_t *task;

	cpc_pm_manager_lost_client(context->pm_manager, name);

	i = 0;
	while (i < context->tasks->len) {
		task = g_ptr_array_index(context->tasks, i);
		client_name =
			g_dbus_method_invocation_get_sender(task->invocation);
		if (!strcmp(name, client_name)) {
			task->invocation = NULL;
			g_ptr_array_remove_index(context->tasks, i);
		}
		else
			++i;
	}

	if (!context->quitting && !context->timeout_id &&
	    ((context->tasks->len == 0) &&
				   (cpc_pm_manager_message_count(
					   context->pm_manager) == 0))) {
		CPC_LOGF("Nothing left to do. Exiting in %u millseconds",
			 CPC_TIMEOUT);
		context->timeout_id = g_timeout_add(CPC_TIMEOUT,
						    prv_timeout,
						    context);
	}

	(void) g_hash_table_remove(context->watchers, name);
}

static void prv_add_task(cpc_context_t *context, cpc_task_t *task)
{
	const gchar *client_name;
	guint watcher_id;

	client_name = g_dbus_method_invocation_get_sender(task->invocation);

	if (!g_hash_table_lookup(context->watchers, client_name)) {
		watcher_id = g_bus_watch_name(G_BUS_TYPE_SESSION, client_name,
					      G_BUS_NAME_WATCHER_FLAGS_NONE,
					      NULL, prv_lost_client, context,
					      NULL);
		g_hash_table_insert(context->watchers, g_strdup(client_name),
				    GUINT_TO_POINTER(watcher_id));
	}

	g_ptr_array_add(context->tasks, task);

	if (context->timeout_id) {
		(void) g_source_remove(context->timeout_id);
		context->timeout_id = 0;
	}

	if (!context->idle_id && !prv_async_in_progress(context))
		context->idle_id = g_idle_add(prv_process_task, context);
}

static void prv_add_parsecp_task(cpc_context_t *context,
				 GDBusMethodInvocation *invocation,
				 GVariant *parameters)
{
	cpc_task_t *task = g_new0(cpc_task_t, 1);
	gchar *filename;

	g_variant_get(parameters, "(s)", &filename);

	CPC_LOGF("Add Task to parse %s",filename);

	task->type = CPC_TASK_PARSECP;
	task->invocation = invocation;
	task->path = filename;
	prv_add_task(context, task);
}

static void prv_add_create_pm_task(cpc_context_t *context,
				   GDBusMethodInvocation *invocation,
				   GVariant *parameters)
{
	cpc_task_t *task = g_new0(cpc_task_t, 1);
	GVariant *array;
	const uint8_t *wp_message;

	CPC_LOGF("Add Task to create WP message");

	array = g_variant_get_child_value(parameters, 0);
	task->type = CPC_TASK_CREATE_PM;
	task->invocation = invocation;
	wp_message = g_variant_get_fixed_array(array,
					       &task->wp_message_len,
					       sizeof(uint8_t));
	task->wp_message = g_new(uint8_t, task->wp_message_len);
	memcpy(task->wp_message, wp_message, task->wp_message_len);

	prv_add_task(context, task);
}

static void prv_add_get_version_task(cpc_context_t *context,
				     GDBusMethodInvocation *invocation)
{
	cpc_task_t *task = g_new0(cpc_task_t, 1);

	CPC_LOGF("Add Task to Get CPClient Version");

	task->type = CPC_TASK_GET_VERSION;
	task->invocation = invocation;
	prv_add_task(context, task);
}

static void prv_add_close_pm_task(cpc_context_t *context,
				  GDBusMethodInvocation *invocation,
				  const gchar *object_path)
{
	cpc_task_t *task = g_new0(cpc_task_t, 1);

	CPC_LOGF("Add Task to close WP message %s", object_path);

	task->type = CPC_TASK_CLOSE_PM;
	task->invocation = invocation;
	task->path = g_strdup(object_path);
	prv_add_task(context, task);
}

static void prv_add_get_props_task(cpc_context_t *context,
				   GDBusMethodInvocation *invocation,
				   const gchar *object_path)
{
	cpc_task_t *task = g_new0(cpc_task_t, 1);

	CPC_LOGF("Add Task to get WP message %s properties", object_path);

	task->type = CPC_TASK_GET_PROPS;
	task->invocation = invocation;
	task->path = g_strdup(object_path);
	prv_add_task(context, task);
}

static void prv_add_apply_task(cpc_context_t *context,
			       GDBusMethodInvocation *invocation,
			       const gchar *object_path,
			       GVariant *parameters)
{
	cpc_task_t *task = g_new0(cpc_task_t, 1);

	CPC_LOGF("Add Task to apply WP message %s properties", object_path);

	task->type = CPC_TASK_APPLY;
	task->invocation = invocation;
	task->path = g_strdup(object_path);
	g_variant_get(parameters, "(s)", &task->pin);
	prv_add_task(context, task);
}

static void prv_cpc_method_call(GDBusConnection *connection,
				const gchar *sender, const gchar *object_path,
				const gchar *interface_name,
				const gchar *method_name, GVariant *parameters,
				GDBusMethodInvocation *invocation,
				gpointer user_data)
{
	cpc_context_t *context = user_data;

	CPC_LOGF("%s called", method_name);

	if (context->timeout_id) {
		g_source_remove(context->timeout_id);
		context->timeout_id = 0;
	}

	if (g_strcmp0(method_name, CPC_INTERFACE_PARSECP) == 0)
		prv_add_parsecp_task(context, invocation, parameters);
	else if (g_strcmp0(method_name,
			   CPC_INTERFACE_CREATE_PUSH_MESSAGE) == 0)
		prv_add_create_pm_task(context, invocation, parameters);
	else if (g_strcmp0(method_name, CPC_INTERFACE_GET_VERSION) == 0)
		prv_add_get_version_task(context, invocation);
}

static void prv_cpc_pm_method_call(GDBusConnection *connection,
				   const gchar *sender,
				   const gchar *object_path,
				   const gchar *interface_name,
				   const gchar *method_name,
				   GVariant *parameters,
				   GDBusMethodInvocation *invocation,
				   gpointer user_data)
{
	cpc_context_t *context = user_data;

	CPC_LOGF("%s called", method_name);

	if (g_strcmp0(method_name, CPC_INTERFACE_CLOSE) == 0)
		prv_add_close_pm_task(context, invocation, object_path);
	else if (g_strcmp0(method_name, CPC_INTERFACE_GET_PROPS) == 0)
		prv_add_get_props_task(context, invocation, object_path);
	else if (g_strcmp0(method_name, CPC_INTERFACE_APPLY) == 0)
		prv_add_apply_task(context, invocation, object_path,
			parameters);
}

static void prv_bus_acquired(GDBusConnection *connection, const gchar *name,
			     gpointer user_data)
{
	cpc_context_t *context = user_data;

	context->connection = connection;

	CPC_LOGF("D-Bus Connection Acquired");

	context->cpclient_id =
		g_dbus_connection_register_object(connection, CPC_OBJECT,
						  context->node_info->
						  interfaces[0],
						  &g_cpc_vtable,
						  user_data, NULL, NULL);

	if (!context->cpclient_id) {
		context->error = CPC_ERR_UNKNOWN;
		g_main_loop_quit(context->main_loop);
		CPC_LOGF("Unable to register "CPC_INTERFACE_MANAGER);
	}
}

static void prv_quit(cpc_context_t *context)
{
	if (context->cp_handle) {
		CPC_LOGF("Cancelling outstanding parsecp task");

		context->quitting = true;
		cpc_tasks_parsecp_cancel(context->cp_handle);
	} else if (context->apply_handle) {
		CPC_LOGF("Cancelling outstanding apply task");
		context->quitting = true;
		cpc_tasks_apply_cancel(context->pm_manager,
					    context->apply_handle);
	} else {
		context->error = CPC_ERR_UNKNOWN;
		g_main_loop_quit(context->main_loop);
	}
}

static void prv_name_lost(GDBusConnection *connection, const gchar *name,
			  gpointer user_data)
{
	cpc_context_t *context = user_data;

	CPC_LOGF("Lost or unable to acquire server name: %s",
		 CPC_SERVER_NAME);

	context->connection = NULL;

	prv_quit(context);
}

static gboolean prv_quit_handler(GIOChannel *source, GIOCondition condition,
				 gpointer user_data)
{
	cpc_context_t *context = user_data;

	CPC_LOGF("SIGTERM or SIGINT received");
	syslog(LOG_INFO, "SIGTERM or SIGINT received");

	prv_quit(context);
	context->sig_id = 0;

	return FALSE;
}

static int prv_init_signal_handler(sigset_t mask, cpc_context_t *context)
{
	CPC_ERR_MANAGE;
	int fd = -1;
	GIOChannel *channel = NULL;

	fd = signalfd(-1, &mask, SFD_NONBLOCK);
	if (fd == -1)
		CPC_FAIL_FORCE(CPC_ERR_IO);

	channel = g_io_channel_unix_new(fd);
	g_io_channel_set_close_on_unref(channel, TRUE);

	if (g_io_channel_set_flags(channel, G_IO_FLAG_NONBLOCK, NULL) !=
	    G_IO_STATUS_NORMAL)
		CPC_FAIL_FORCE(CPC_ERR_IO);

	if (g_io_channel_set_encoding(channel, NULL, NULL) !=
	    G_IO_STATUS_NORMAL)
		CPC_FAIL_FORCE(CPC_ERR_IO);

	context->sig_id = g_io_add_watch(channel, G_IO_IN | G_IO_PRI,
					 prv_quit_handler,
					 context);
	g_io_channel_unref(channel);

	return CPC_ERR_NONE;

CPC_ON_ERR:

	if (channel)
		g_io_channel_unref(channel);

	CPC_LOGF("Unable to set up signal handlers");

	return CPC_ERR;
}

static void prv_unregister_client(gpointer client)
{
	guint id = GPOINTER_TO_UINT(client);
	g_bus_unwatch_name(id);
}

int main(int argc, char *argv[])
{
	CPC_ERR_MANAGE;
	cpc_context_t context;
	sigset_t mask;
#ifdef CPC_LOGGING
	GString *log_name_str;
#endif

	openlog(PACKAGE_NAME, 0, LOG_DAEMON);
	syslog(LOG_INFO, "Starting");

	prv_cpc_context_init(&context);

	sigemptyset(&mask);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGINT);

	if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
		CPC_FAIL_FORCE(CPC_ERR_IO);

	g_type_init();

#ifdef CPC_LOGGING
	log_name_str = g_string_new(CPC_LOG_FILE);
	g_string_append(log_name_str, g_get_user_name());
	g_string_append(log_name_str, ".log");
	CPC_FAIL(CPC_LOG_OPEN(log_name_str->str));
	(void) g_string_free(log_name_str, TRUE);
#endif

	CPC_LOGF("============= cpclient starting =============");

	context.node_info = g_dbus_node_info_new_for_xml(g_cpc_introspection,
							 NULL);
	if (!context.node_info) {
		CPC_LOGF("Unable to create introspection data!");
		CPC_FAIL_FORCE(CPC_ERR_UNKNOWN);
	}

	cpc_pm_manager_new(context.node_info->interfaces[1],
			   &g_cpc_pm_vtable, &context,
			   &context.pm_manager);

	context.main_loop = g_main_loop_new(NULL, FALSE);

	context.owner_id = g_bus_own_name(G_BUS_TYPE_SESSION,
					  CPC_SERVER_NAME,
					  G_BUS_NAME_OWNER_FLAGS_NONE,
					  prv_bus_acquired, NULL,
					  prv_name_lost, &context, NULL);

	context.tasks = g_ptr_array_new_with_free_func(prv_free_cpc_task);

	context.timeout_id = g_timeout_add(CPC_TIMEOUT, prv_timeout, &context);

	context.watchers = g_hash_table_new_full(g_str_hash, g_str_equal,
						 g_free, prv_unregister_client);

	CPC_FAIL(prv_init_signal_handler(mask, &context));

	syslog(LOG_INFO, "Started.  Ready to receive commands ...");

	g_main_loop_run(context.main_loop);

CPC_ON_ERR:

	prv_cpc_context_free(&context);

	CPC_LOGF("============= cpclient exitting (%d) =============",
		CPC_ERR);

	CPC_LOG_CLOSE();

	syslog(LOG_INFO, "Exitting with error %u", CPC_ERR);
	closelog();

	return CPC_ERR == CPC_ERR_NONE ? 0 : 1;
}
