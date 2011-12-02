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
 * @file <imsi.c>
 *
 * @brief Contains functions for retrieving the imsi number
 *
 *****************************************************************************/

#include "config.h"

#include <gio/gio.h>

#include "imsi.h"
#include "error.h"
#include "log.h"

enum cpc_imsi_state_t_ {
	OMACP_IMSI_START,
	OMACP_IMSI_GET,
	OMACP_IMSI_ABORT,
	OMACP_IMSI_FINISHED
};

typedef enum cpc_imsi_state_t_  cpc_imsi_state_t;

typedef struct cpc_imsi_t_ cpc_imsi_t;
struct cpc_imsi_t_ {
	cpc_imsi_state_t state;
	guint finished_source;
	int result;
	gchar **imsis;
	GDBusProxy *system_proxy;
	GCancellable *cancellable;
	cpc_imsi_cb_t cb;
	void *user_data;
};

#define PROVMAN_INTERFACE PROVMAN_SERVICE".Settings"
#define PROVMAN_SERVER_NAME PROVMAN_SERVICE".server"

#define PROVMAN_INTERFACE_START "Start"
#define PROVMAN_INTERFACE_GET "Get"
#define PROVMAN_INTERFACE_ABORT "Abort"

static void prv_imsi_step(cpc_imsi_t *op);

static gboolean prv_task_finished(gpointer user_data)
{
	cpc_imsi_t *op = user_data;

	CPC_LOGF("IMSI Get Finished with result %d", op->result);

	/* Ownership of op->imsis is transferred to caller */

	if (op->result != CPC_ERR_NONE && op->imsis) {
		g_strfreev(op->imsis);
		op->imsis = NULL;
	}

	op->cb(op->result, op->imsis, op->user_data);

	if (op->system_proxy)
		g_object_unref(op->system_proxy);

	g_object_unref(op->cancellable);

	g_free(op);

	return FALSE;
}

static void prv_imsi_finish_method(GObject *source_object, GAsyncResult *result,
				   gpointer user_data, GVariant **retval)
{
	cpc_imsi_t *op = user_data;
	GVariant *res;
	GError *error = NULL;

	res = g_dbus_proxy_call_finish(op->system_proxy, result, &error);

	if (g_cancellable_is_cancelled(op->cancellable)) {
		CPC_LOGF("Operation Cancelled");
		op->result = CPC_ERR_CANCELLED;
		op->finished_source = g_idle_add(prv_task_finished, user_data);
	} else if (!res) {
		CPC_LOGF("Operation Failed %s", error->message);
		op->result = CPC_ERR_IO;
		op->finished_source = g_idle_add(prv_task_finished, user_data);
	} else {
		*retval = res;
	}

	if (error)
		g_error_free(error);
}

static void prv_start_method_cb(GObject *source_object,
				GAsyncResult *result,
				gpointer user_data)
{
	GVariant *res;
	cpc_imsi_t *op = user_data;

	prv_imsi_finish_method(source_object, result, user_data, &res);
	if (op->result == CPC_ERR_NONE) {
		g_variant_unref(res);
		prv_imsi_step(op);
	}
}

static void prv_get_imsi_method_cb(GObject *source_object,
				   GAsyncResult *result,
				   gpointer user_data)
{
	GVariant *res;
	cpc_imsi_t *op = user_data;
	gchar *imsi_str = NULL;
	gchar **imsis = NULL;

	prv_imsi_finish_method(source_object, result, user_data, &res);
	if (op->result == CPC_ERR_NONE) {
		CPC_LOGF("Retrieving IMSI numbers");

		g_variant_get(res, "(&s)", &imsi_str);
		CPC_LOGF("IMSI Numbers = %s", imsi_str);
		if (imsi_str) {
			imsis = g_strsplit(imsi_str, ",", 0);
			if (!imsis[0]) {
				g_strfreev(imsis);
				imsis = NULL;
			}
		}

		g_variant_unref(res);

		if (!imsis) {
			op->result = CPC_ERR_IO;
			op->finished_source = g_idle_add(prv_task_finished,
							 user_data);
		} else {
			op->imsis = imsis;
			prv_imsi_step(op);
		}
	}
}

static void prv_abort_method_cb(GObject *source_object,
				GAsyncResult *result,
				gpointer user_data)
{
	GVariant *res;
	cpc_imsi_t *op = user_data;

	res = g_dbus_proxy_call_finish(op->system_proxy, result, NULL);
	if (g_cancellable_is_cancelled(op->cancellable)) {
		CPC_LOGF("Operation Cancelled");
		op->result = CPC_ERR_CANCELLED;
		op->finished_source = g_idle_add(prv_task_finished, user_data);
	} else {
		if (res)
			g_variant_unref(res);
		prv_imsi_step(op);
	}
}

static void prv_imsi_step(cpc_imsi_t *op)
{
	if (op->state == OMACP_IMSI_START) {
		CPC_LOGF("Sending Start");
		g_cancellable_reset(op->cancellable);
		g_dbus_proxy_call(op->system_proxy, PROVMAN_INTERFACE_START,
				  g_variant_new("(s)", ""),
				  G_DBUS_CALL_FLAGS_NONE,
				  -1, op->cancellable,
				  prv_start_method_cb, op);
		op->state = OMACP_IMSI_GET;
	} else if (op->state == OMACP_IMSI_GET) {
		CPC_LOGF("Sending Get");
		g_cancellable_reset(op->cancellable);
		g_dbus_proxy_call(op->system_proxy, PROVMAN_INTERFACE_GET,
				  g_variant_new("(s)", "/telephony/imsis"),
				  G_DBUS_CALL_FLAGS_NONE,
				  -1, op->cancellable,
				  prv_get_imsi_method_cb, op);
		op->state = OMACP_IMSI_ABORT;
	} else if (op->state == OMACP_IMSI_ABORT) {
		CPC_LOGF("Sending Abort");
		g_cancellable_reset(op->cancellable);
		g_dbus_proxy_call(op->system_proxy, PROVMAN_INTERFACE_ABORT,
				  NULL, G_DBUS_CALL_FLAGS_NONE, -1,
				  op->cancellable,
				  prv_abort_method_cb, op);
		op->state = OMACP_IMSI_FINISHED;
	} else {
		op->result = CPC_ERR_NONE;
		op->finished_source = g_idle_add(prv_task_finished,
						 op);
	}
}

static void prv_proxy_created(GObject *source_object, GAsyncResult *result,
			      gpointer user_data)
{
	cpc_imsi_t *op = user_data;
	GError *error = NULL;

	GDBusProxy *proxy = g_dbus_proxy_new_finish(result, &error);

	if (g_cancellable_is_cancelled(op->cancellable)) {
		CPC_LOGF("provman proxy creation cancelled");
		op->result = CPC_ERR_CANCELLED;
		op->finished_source = g_idle_add(prv_task_finished, user_data);
	} else if (!proxy) {
		CPC_LOGF("Unable to create provman proxy %s", error->message);
		op->result = CPC_ERR_IO;
		op->finished_source = g_idle_add(prv_task_finished, user_data);
	} else {
		op->system_proxy = proxy;
		prv_imsi_step(op);
	}

	if (error)
		g_error_free(error);
}

void cpc_imsi_get(cpc_imsi_cb_t cb, void *user_data,
		       cpc_imsi_handle_t *handle)
{
	cpc_imsi_t *op;

	CPC_LOGF("Retrieving IMSI numbers");

	op = g_new0(cpc_imsi_t, 1);
	op->cb = cb;
	op->user_data = user_data;

	op->cancellable = g_cancellable_new();
	g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
				 G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
				 NULL, PROVMAN_SERVER_NAME, PROVMAN_OBJECT,
				 PROVMAN_INTERFACE, op->cancellable,
				 prv_proxy_created, op);
	*handle = op;
}

void cpc_imsi_get_cancel(cpc_imsi_handle_t handle)
{
	cpc_imsi_t *op;

	CPC_LOGF("Request to retrieve IMSI numbers cancelled");

	op = handle;

	if (!op->finished_source)
		g_cancellable_cancel(op->cancellable);
}
