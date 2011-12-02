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
 * @file <pm-manager.c>
 *
 * @brief Contains definitions for the push message class
 *
 *****************************************************************************/

#include "config.h"

#include <string.h>

#include "pm-manager.h"
#include "provision-wp.h"
#include "error.h"
#include "error-macros.h"
#include "log.h"

struct cpc_pm_manager_t_ {
	GDBusInterfaceInfo *interface;
	const GDBusInterfaceVTable *vtable;
	gpointer user_data;
	unsigned int counter;
	GHashTable *objects;
};

typedef struct cpc_push_message_t_ cpc_push_message_t;
struct cpc_push_message_t_ {
	GDBusConnection *connection;
	guint id;
	cpc_provision_wp_t *provision;
	cpc_cb_t finished;
	void *finished_data;
	gchar *client_name;
	bool lost_client;
	bool applied;
};

typedef struct cpc_apply_data_t_ cpc_apply_data_t;
struct cpc_apply_data_t_ {
	cpc_pm_manager_t *manager;
	gchar *pm_path;
};

void cpc_props_free(cpc_props_t *props)
{
	g_free(props->settings);
	g_free(props->start_sessions);
}

static void prv_apply_data_delete(cpc_apply_data_t *apply_data)
{
	if (apply_data) {
		g_free(apply_data->pm_path);
		g_free(apply_data);
	}
}

static void prv_cpc_push_message_delete(gpointer message)
{
	cpc_push_message_t *pm;

	if (message) {
		pm = message;
		if (pm->id)
			(void) g_dbus_connection_unregister_object(
				pm->connection, pm->id);
		cpc_provision_wp_delete(pm->provision);
		g_free(pm->client_name);
		g_free(message);
	}
}

void cpc_pm_manager_new(GDBusInterfaceInfo *interface,
			const GDBusInterfaceVTable *vtable,
			gpointer user_data, cpc_pm_manager_t **manager)
{
	cpc_pm_manager_t *pm_manager;

	pm_manager = g_new0(cpc_pm_manager_t, 1);

	pm_manager->interface = interface;
	pm_manager->counter = 1;
	pm_manager->vtable = vtable;
	pm_manager->user_data = user_data;
	pm_manager->objects =
		g_hash_table_new_full(g_str_hash, g_str_equal,
				      g_free, prv_cpc_push_message_delete);
	*manager = pm_manager;
}

static int prv_prv_push_message_new(GDBusConnection *connection,
				    const gchar *client_name,
				    uint8_t *data, unsigned int length,
				    cpc_push_message_t **message)
{
	CPC_ERR_MANAGE;
	cpc_push_message_t *pm;

	pm = g_new0(cpc_push_message_t, 1);
	CPC_FAIL(cpc_provision_wp_new(data, length, &pm->provision));
	pm->connection = connection;
	pm->client_name = g_strdup(client_name);
	*message = pm;
	pm = NULL;

CPC_ON_ERR:

	prv_cpc_push_message_delete(pm);

	return CPC_ERR;
}

void cpc_pm_manager_lost_client(cpc_pm_manager_t *manager, const gchar *name)
{
	cpc_push_message_t *pm;
	GHashTableIter iter;
	gpointer key;
	gpointer value;

	CPC_LOGF("Lost client %s", name);

	g_hash_table_iter_init(&iter, manager->objects);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		pm = value;
		if (!strcmp(name, pm->client_name)) {
			CPC_LOGF("Client %s orphaned", key);
			if (pm->finished) {
				CPC_LOGF("Apply outstanding.  "
					 "Schedule %s for later deletion.",
					 key);
				cpc_provision_wp_apply_cancel(pm->provision);
				pm->lost_client = true;
			} else {
				CPC_LOGF("No outstanding requests. "
					 "Can remove object immediately");
				g_hash_table_iter_remove(&iter);
			}
		}
	}
}

int cpc_pm_manager_new_message(cpc_pm_manager_t *manager,
			       const gchar *client_name,
			       GDBusConnection *connection, uint8_t *data,
			       unsigned int length, gchar **path)
{
	CPC_ERR_MANAGE;

	GString *new_path = NULL;
	guint id;
	cpc_push_message_t *pm = NULL;

	CPC_FAIL(prv_prv_push_message_new(connection, client_name, data, length,
					  &pm));

	new_path = g_string_new("");
	g_string_printf(new_path, "%s/%u", CPC_OBJECT, manager->counter);

	id =  g_dbus_connection_register_object(connection,
						new_path->str,
						manager->interface,
						manager->vtable,
						manager->user_data,
						NULL, NULL);
	if (!id)
		CPC_FAIL_FORCE(CPC_ERR_IO);


	pm->id = id;
	g_hash_table_insert(manager->objects, g_strdup(new_path->str),
			    pm);
	*path = g_string_free(new_path, FALSE);
	++manager->counter;

	return CPC_ERR_NONE;

CPC_ON_ERR:

	prv_cpc_push_message_delete(pm);
	if (new_path)
		(void) g_string_free(new_path, TRUE);

	return CPC_ERR;
}

int cpc_pm_manager_get_properties(cpc_pm_manager_t *manager, const gchar *path,
				  const gchar *client_name, cpc_props_t *props)
{
	CPC_ERR_MANAGE;
	cpc_push_message_t *pm;

	CPC_FAIL_NULL(pm, g_hash_table_lookup(manager->objects, path),
		      CPC_ERR_NOT_FOUND);

	if (strcmp(client_name, pm->client_name))
		CPC_FAIL_FORCE(CPC_ERR_DENIED);

	props->pin_required = cpc_provision_wp_pin_required(pm->provision);
	props->settings = cpc_provision_wp_get_settings(pm->provision);
	props->sec_type = cpc_provision_wp_get_sec_type(pm->provision);
	props->start_sessions = cpc_provision_wp_get_sessions(pm->provision);

CPC_ON_ERR:

	return CPC_ERR;
}

int cpc_pm_manager_remove_message(cpc_pm_manager_t *manager,
				  const gchar *path, const gchar *client_name)
{
	CPC_ERR_MANAGE;
	cpc_push_message_t *pm;

	CPC_FAIL_NULL(pm, g_hash_table_lookup(manager->objects, path),
		      CPC_ERR_NOT_FOUND);

	if (strcmp(client_name, pm->client_name))
		CPC_FAIL_FORCE(CPC_ERR_DENIED);

	(void) g_hash_table_remove(manager->objects, path);

CPC_ON_ERR:

	return CPC_ERR;
}

void cpc_pm_manager_delete(cpc_pm_manager_t *manager)
{
	if (manager) {
		g_hash_table_unref(manager->objects);
		g_free(manager);
	}
}

unsigned int cpc_pm_manager_message_count(cpc_pm_manager_t *manager)
{
	return g_hash_table_size(manager->objects);
}

static void prv_apply_finished(int result, void *user_data)
{
	cpc_apply_data_t *apply_data = user_data;
	cpc_push_message_t *pm;

	pm = g_hash_table_lookup(apply_data->manager->objects,
				 apply_data->pm_path);
	if (pm) {
		pm->finished(result, pm->finished_data);
		pm->finished = NULL;
		pm->finished_data = NULL;
		if (result == CPC_ERR_NONE)
			pm->applied = true;
		if (pm->lost_client)
			g_hash_table_remove(apply_data->manager->objects,
					    apply_data->pm_path);
	}
	prv_apply_data_delete(apply_data);
}

int cpc_pm_manager_apply(cpc_pm_manager_t *manager,
			 const gchar *path, const gchar* client_name,
			 const gchar *pin, cpc_cb_t finished,
			 void *finished_data, cpc_handle_t *handle)
{
	CPC_ERR_MANAGE;
	cpc_push_message_t *pm;
	cpc_apply_data_t *apply_data = NULL;

	CPC_FAIL_NULL(pm, g_hash_table_lookup(manager->objects, path),
		      CPC_ERR_NOT_FOUND);

	if (strcmp(client_name, pm->client_name))
		CPC_FAIL_FORCE(CPC_ERR_DENIED);

	if (pm->finished)
		CPC_FAIL_FORCE(CPC_ERR_DENIED);

	if (pm->applied)
		CPC_FAIL_FORCE(CPC_ERR_ALREADY_APPLIED);

	apply_data = g_new0(cpc_apply_data_t, 1);
	apply_data->manager = manager;
	apply_data->pm_path = g_strdup(path);

	CPC_FAIL(cpc_provision_wp_apply(pm->provision, pin, prv_apply_finished,
					apply_data));

	pm->finished = finished;
	pm->finished_data = finished_data;

	*handle = pm;

	return CPC_ERR_NONE;

CPC_ON_ERR:

	prv_apply_data_delete(apply_data);

	return CPC_ERR;
}

void cpc_pm_manager_apply_cancel(cpc_pm_manager_t *manager,
				 cpc_handle_t handle)
{
	cpc_push_message_t *pm = handle;

	cpc_provision_wp_apply_cancel(pm->provision);
}

