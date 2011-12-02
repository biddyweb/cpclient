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
 * @file <provision.c>
 *
 * @brief Code to provision an OMA CP configuration context
 *
 ******************************************************************************/

#include "config.h"

#include <string.h>
#include <glib.h>
#include <gio/gio.h>

#include "context.h"
#include "log.h"
#include "error.h"
#include "error-macros.h"
#include "provision.h"
#include "settings.h"

#define PROVMAN_INTERFACE PROVMAN_SERVICE".Settings"
#define PROVMAN_SERVER_NAME PROVMAN_SERVICE".server"

#define PROVMAN_INTERFACE_START "Start"
#define PROVMAN_INTERFACE_BEGIN "Begin"
#define PROVMAN_INTERFACE_COMMIT "Commit"
#define PROVMAN_INTERFACE_CANCEL "Cancel"
#define PROVMAN_INTERFACE_GET_MULTIPLE "GetMultiple"
#define PROVMAN_INTERFACE_DELETE_MULTIPLE "DeleteMultiple"
#define PROVMAN_INTERFACE_SET "Set"
#define PROVMAN_INTERFACE_SET_MULTIPLE "SetMultiple"
#define PROVMAN_INTERFACE_SET_MULTIPLE_META "SetMultipleMeta"
#define PROVMAN_INTERFACE_DELETE "Delete"
#define PROVMAN_INTERFACE_END "End"

const gchar *g_system_objects[] = {
	"/telephony/contexts"
};

const gchar *g_system_always_remove[] = {
	"/telephony/mms"
};

const gchar *g_session_objects[] = {
	"/applications/email",
	"/applications/sync",
	"/applications/browser/bookmarks"
};

enum cpc_provision_state_t_ {
	CPC_PROVISION_START,
	CPC_PROVISION_GET_OBJECTS,
	CPC_PROVISION_DELETE_OBJECTS,
	CPC_PROVISION_SET,
	CPC_PROVISION_SET_META,
	CPC_PROVISION_END,
	CPC_PROVISION_FINISHED
};

typedef enum cpc_provision_state_t_ cpc_provision_state_t;

typedef struct omacp_provision_t_ omacp_provision_t;

struct omacp_provision_t_ {
	cpc_provision_cb_t finished;
	void *finished_data;
	GDBusProxy *session_proxy;
	GDBusProxy *system_proxy;
	GDBusProxy *current_proxy;
	GCancellable *cancellable;
	gchar *imsi;
	GHashTable *current_settings;
	GPtrArray *current_meta;
	GHashTable *current_objects;
	guint finished_source;
	int result;
	cpc_provision_state_t state;
	const gchar **object_paths;
	unsigned int object_path_count;
	const gchar **always_remove;
	unsigned int always_remove_count;
	cpc_settings_t settings;
};

static void prv_provision_step(omacp_provision_t *provision);
static void prv_system_proxy_created(GObject *source_object,
				     GAsyncResult *result, gpointer user_data);

static void prv_omacp_provision_delete(omacp_provision_t *provision)
{
	if (provision) {
		if (provision->cancellable)
			g_object_unref(provision->cancellable);
		if (provision->session_proxy)
			g_object_unref(provision->session_proxy);
		if (provision->system_proxy)
			g_object_unref(provision->system_proxy);
		if (provision->current_objects)
			g_hash_table_unref(provision->current_objects);
		cpc_settings_free(&provision->settings);
		g_free(provision->imsi);
		g_free(provision);
	}
}

static gboolean prv_provision_task_finished(gpointer user_data)
{
	omacp_provision_t *provision = user_data;

	provision->finished(provision->finished_data, provision->result);
	prv_omacp_provision_delete(provision);

	return FALSE;
}

static void prv_add_objects(omacp_provision_t *provision, const gchar *root,
			    const gchar *children_str)
{
	GString *object;
	unsigned int i;
	gchar **children = g_strsplit(children_str, "/", 0);

	if (children) {
		for (i = 0; children[i]; ++i) {
			object = g_string_new(root);
			g_string_append_c(object, '/');
			g_string_append(object, children[i]);
			CPC_LOGF("Found object %s", object->str);
			g_hash_table_insert(provision->current_objects,
					    g_string_free(object, FALSE), NULL);
		}
		g_strfreev(children);
	}
}

#ifdef CPC_OVERWRITE
static void prv_identify_objects_to_remove(omacp_provision_t *provision)
{
	GHashTableIter iter_objs;
	GHashTableIter iter_settings;
	gpointer obj_key;
	gpointer setting_key;
	unsigned int obj_key_len;
	bool found;

	g_hash_table_iter_init(&iter_objs, provision->current_objects);
	while (g_hash_table_iter_next(&iter_objs, &obj_key, NULL)) {
		obj_key_len = strlen(obj_key);
		g_hash_table_iter_init(&iter_settings,
				       provision->current_settings);
		found = false;
		while (!found && g_hash_table_iter_next(&iter_settings,
							&setting_key,
							NULL)) {
			if (!strncmp(obj_key, setting_key, obj_key_len) &&
			    ((const gchar*) setting_key)[obj_key_len] == '/')
				found = true;
		}

		if (!found)
			g_hash_table_iter_remove(&iter_objs);
#ifdef CPC_LOGGING
		else
			CPC_LOGF("Need to remove %s", obj_key);
#endif
	}
}
#else
static void prv_remove_matching_settings(GHashTable *ht, const gchar *root)
{
	GHashTableIter iter;
	gpointer key;
	unsigned int root_len;

	g_hash_table_iter_init(&iter, ht);
	root_len = strlen(root);
	while (g_hash_table_iter_next(&iter, &key, NULL)) {
		if (!strncmp(root, key, root_len) &&
		    ((const gchar*) key)[root_len] == '/') {
			CPC_LOGF("%s already exists", key);
			g_hash_table_iter_remove(&iter);
		}
	}
}

static void prv_remove_matching_meta(GPtrArray* meta, const gchar *root)
{
	unsigned int i = 0;
	omacp_meta_prop_t *prop;
	unsigned int root_len;

	root_len = strlen(root);
	while (i < meta->len) {
		prop = g_ptr_array_index(meta, i);
		if (!strncmp(root, prop->key, root_len) &&
		    prop->key[root_len] == '/') {
			CPC_LOGF("%s?%s already exists", prop->key, prop->prop);
			g_ptr_array_remove_index(meta, i);
		}
		else
			++i;
	}
}


static void prv_remove_settings_for_existing_objects(
	omacp_provision_t *provision)
{
	GHashTableIter iter;
	gpointer key;

	g_hash_table_iter_init(&iter, provision->current_objects);
	while (g_hash_table_iter_next(&iter, &key, NULL)) {
		prv_remove_matching_settings(provision->current_settings, key);
		if (provision->current_meta)
			prv_remove_matching_meta(provision->current_meta, key);
	}
}
#endif

static void prv_get_objects_cb(GObject *source_object,
			       GAsyncResult *result,
			       gpointer user_data)
{
	CPC_ERR_MANAGE;
	omacp_provision_t *provision = user_data;
	GVariant *res;
	GError *error = NULL;
	GVariantIter iter;
	gchar *key;
	gchar *value;
	unsigned int i;

	res = g_dbus_proxy_call_finish(provision->current_proxy, result,
				       &error);

	if (g_cancellable_is_cancelled(provision->cancellable)) {
		CPC_LOGF("Operation Cancelled");
		CPC_FAIL_FORCE(CPC_ERR_CANCELLED);
	} else if (!res) {
		CPC_LOGF("Operation Failed %s", error->message);
		CPC_FAIL_FORCE(CPC_ERR_IO);
	}

	(void) g_variant_iter_init(&iter, g_variant_get_child_value(res, 0));
	while (g_variant_iter_next(&iter, "{&s&s}", &key, &value))
		prv_add_objects(provision, key, value);

	g_variant_unref(res);

#ifndef CPC_OVERWRITE
	prv_remove_settings_for_existing_objects(provision);
	g_hash_table_remove_all(provision->current_objects);
#endif
	for (i = 0; i < provision->always_remove_count; ++i)
		g_hash_table_insert(provision->current_objects,
				    g_strdup(provision->always_remove[i]),
				    NULL);

	prv_identify_objects_to_remove(provision);
	if (g_hash_table_size(provision->current_objects) > 0)
		provision->state = CPC_PROVISION_DELETE_OBJECTS;
	else if (g_hash_table_size(provision->current_settings) > 0)
		provision->state = CPC_PROVISION_SET;
	else
		provision->state = CPC_PROVISION_END;

	prv_provision_step(provision);

	return;

CPC_ON_ERR:

	if (error)
		g_error_free(error);

	provision->result = CPC_ERR;
	provision->finished_source =
		g_idle_add(prv_provision_task_finished, user_data);
}

static void prv_get_objects(omacp_provision_t *provision)
{
	GVariantBuilder vb;
	unsigned int i;

	g_variant_builder_init(&vb, G_VARIANT_TYPE("as"));

	for (i = 0; i < provision->object_path_count; ++i)
		g_variant_builder_add(&vb, "s", provision->object_paths[i]);
	g_cancellable_reset(provision->cancellable);
	g_dbus_proxy_call(provision->current_proxy,
			  PROVMAN_INTERFACE_GET_MULTIPLE,
			  g_variant_new("(@as)", g_variant_builder_end(&vb)),
			  G_DBUS_CALL_FLAGS_NONE, -1, provision->cancellable,
			  prv_get_objects_cb, provision);
}

static void prv_provision_start_method_cb(GObject *source_object,
					  GAsyncResult *result,
					  gpointer user_data)
{
	CPC_ERR_MANAGE;
	omacp_provision_t *provision = user_data;
	GVariant *res;
	GError *error = NULL;

	res = g_dbus_proxy_call_finish(provision->current_proxy, result,
				       &error);

	if (g_cancellable_is_cancelled(provision->cancellable)) {
		CPC_LOGF("Start Operation Cancelled");
		CPC_FAIL_FORCE(CPC_ERR_CANCELLED);
	} else if (!res) {
		CPC_LOGF("Start Operation Failed %s", error->message);
		provision->state = CPC_PROVISION_FINISHED;
	} else {
		g_variant_unref(res);
	}

	if (error)
		g_error_free(error);

	prv_provision_step(provision);

	return;

CPC_ON_ERR:

	if (error)
		g_error_free(error);

	provision->result = CPC_ERR;
	provision->finished_source =
		g_idle_add(prv_provision_task_finished, user_data);
}


static void prv_provision_method_cb(GObject *source_object,
				    GAsyncResult *result,
				    gpointer user_data)
{
	CPC_ERR_MANAGE;
	omacp_provision_t *provision = user_data;
	GVariant *res;
	GError *error = NULL;

	res = g_dbus_proxy_call_finish(provision->current_proxy, result,
				       &error);

	if (g_cancellable_is_cancelled(provision->cancellable)) {
		CPC_LOGF("Operation Cancelled");
		CPC_FAIL_FORCE(CPC_ERR_CANCELLED);
	} else if (!res) {
		CPC_LOGF("Operation Failed %s", error->message);
	} else {
		g_variant_unref(res);
	}

	if (error)
		g_error_free(error);

	prv_provision_step(provision);

	return;

CPC_ON_ERR:

	if (error)
		g_error_free(error);

	provision->result = CPC_ERR;
	provision->finished_source =
		g_idle_add(prv_provision_task_finished, user_data);
}

static void prv_delete_objects(omacp_provision_t *provision)
{
	GVariantBuilder vb;
	GHashTableIter iter;
	gpointer key;

	g_variant_builder_init(&vb, G_VARIANT_TYPE("as"));

	g_hash_table_iter_init(&iter, provision->current_objects);
	while (g_hash_table_iter_next(&iter, &key, NULL))
		g_variant_builder_add(&vb, "s", (const gchar *) key);

	g_cancellable_reset(provision->cancellable);
	g_dbus_proxy_call(provision->current_proxy,
			  PROVMAN_INTERFACE_DELETE_MULTIPLE,
			  g_variant_new("(@as)", g_variant_builder_end(&vb)),
			  G_DBUS_CALL_FLAGS_NONE, -1, provision->cancellable,
			  prv_provision_method_cb, provision);
}

static GVariant *prv_make_dictionary(GHashTable *settings)
{
	GHashTableIter iter;
	gpointer key;
	gpointer value;
	GVariantBuilder vb;

	g_variant_builder_init(&vb, G_VARIANT_TYPE("a{ss}"));
	g_hash_table_iter_init(&iter, settings);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		CPC_LOGF("Set %s = %s", (const char*) key,
			 (const char *) value);
		g_variant_builder_add(&vb, "{ss}", key, value);
	}

	return g_variant_builder_end(&vb);
}

static GVariant *prv_make_meta_array(GPtrArray *meta)
{
	GVariantBuilder vb;
	unsigned int i;
	cpc_meta_prop_t *prop;

	g_variant_builder_init(&vb, G_VARIANT_TYPE("a(sss)"));
	for (i = 0; i < meta->len; ++i) {
		prop = g_ptr_array_index(meta, i);
		CPC_LOGF("Set Meta %s?%s = %s", prop->key, prop->prop,
			 prop->value);
		g_variant_builder_add(&vb, "(sss)", prop->key, prop->prop,
				      prop->value);
	}

	return g_variant_builder_end(&vb);
}

static void prv_init_session(omacp_provision_t *provision)
{
	provision->current_proxy = provision->session_proxy;
	provision->current_settings = provision->settings.session_settings;
	provision->current_meta = provision->settings.session_meta;
	provision->object_paths = g_session_objects;
	provision->object_path_count =
		sizeof(g_session_objects) / sizeof(const gchar*);
	provision->always_remove = NULL;
	provision->always_remove_count = 0;
}

static void prv_init_system(omacp_provision_t *provision)
{
	provision->current_proxy = provision->system_proxy;
	provision->current_settings = provision->settings.system_settings;
	provision->current_meta = provision->settings.system_meta;
	provision->object_paths = g_system_objects;
	provision->object_path_count =
		sizeof(g_system_objects) / sizeof(const gchar*);
	provision->always_remove = g_system_always_remove;
	provision->always_remove_count =
		sizeof(g_system_always_remove) / sizeof(const gchar*);
}

static void prv_provision_step(omacp_provision_t *provision)
{
	GVariant *dict;

	if (provision->state == CPC_PROVISION_START) {
		CPC_LOGF("Sending Start to provman instance %s",
			 provision->imsi);

		g_cancellable_reset(provision->cancellable);
		g_dbus_proxy_call(provision->current_proxy,
				  PROVMAN_INTERFACE_START,
				  g_variant_new("(s)", provision->imsi),
				  G_DBUS_CALL_FLAGS_NONE,
				  -1, provision->cancellable,
				  prv_provision_start_method_cb,
				  provision);

		provision->state = CPC_PROVISION_GET_OBJECTS;
	} else if (provision->state == CPC_PROVISION_GET_OBJECTS) {
		CPC_LOGF("Get Objects");
		prv_get_objects(provision);
	} else if (provision->state == CPC_PROVISION_DELETE_OBJECTS) {
		CPC_LOGF("Delete Objects");
		prv_delete_objects(provision);
		provision->state = CPC_PROVISION_SET;
	}  else if (provision->state == CPC_PROVISION_SET) {
		CPC_LOGF("Calling SetAll");

		dict = prv_make_dictionary(provision->current_settings);
		g_cancellable_reset(provision->cancellable);
		g_dbus_proxy_call(provision->current_proxy,
				  PROVMAN_INTERFACE_SET_MULTIPLE,
				  g_variant_new("(@a{ss})", dict),
				  G_DBUS_CALL_FLAGS_NONE,
				  -1, provision->cancellable,
				  prv_provision_method_cb,
				  provision);

		provision->state = (provision->current_meta) ?
			CPC_PROVISION_SET_META : CPC_PROVISION_END;
	} else if (provision->state == CPC_PROVISION_SET_META) {
		CPC_LOGF("Calling SetAllMeta");

		dict = prv_make_meta_array(provision->current_meta);
		g_cancellable_reset(provision->cancellable);
		g_dbus_proxy_call(provision->current_proxy,
				  PROVMAN_INTERFACE_SET_MULTIPLE_META,
				  g_variant_new("(@a(sss))", dict),
				  G_DBUS_CALL_FLAGS_NONE,
				  -1, provision->cancellable,
				  prv_provision_method_cb,
				  provision);
		provision->state = CPC_PROVISION_END;
	} else if (provision->state == CPC_PROVISION_END) {
		CPC_LOGF("Sending End");
		g_cancellable_reset(provision->cancellable);
		g_dbus_proxy_call(provision->current_proxy,
				  PROVMAN_INTERFACE_END,
				  NULL,
				  G_DBUS_CALL_FLAGS_NONE,
				  -1, provision->cancellable,
				  prv_provision_method_cb,
				  provision);
		provision->state = CPC_PROVISION_FINISHED;
	} else if (provision->state == CPC_PROVISION_FINISHED) {
		if ((provision->current_proxy == provision->session_proxy) &&
		    (provision->system_proxy)) {
			prv_init_system(provision);
			provision->state = CPC_PROVISION_START;
			prv_provision_step(provision);
		} else {
			provision->result = CPC_ERR_NONE;
			provision->finished_source =
				g_idle_add(prv_provision_task_finished,
					   provision);
		}
	}
}

static int prv_proxy_created(GObject *source_object, GAsyncResult *result,
			     gpointer user_data, GDBusProxy **new_proxy)
{
	CPC_ERR_MANAGE;
	omacp_provision_t *provision = user_data;
	GError *error = NULL;

	GDBusProxy *proxy = g_dbus_proxy_new_finish(result, &error);

	if (g_cancellable_is_cancelled(provision->cancellable)) {
		CPC_LOGF("provman proxy creation cancelled");
		CPC_ERR = provision->result = CPC_ERR_CANCELLED;
		provision->finished_source =
			g_idle_add(prv_provision_task_finished, user_data);
		proxy = NULL;
	} else if (!proxy) {
		CPC_LOGF("Unable to create provman proxy %s", error->message);
		CPC_ERR = CPC_ERR_IO;
	}
	else
		*new_proxy = proxy;

	if (error)
		g_error_free(error);

	return CPC_ERR;
}

static void prv_provision_start(omacp_provision_t *provision)
{
	if (provision->session_proxy) {
		prv_init_session(provision);
	} else if (provision->system_proxy) {
		prv_init_system(provision);
	} else {
		CPC_LOGF("Unable to connect to either provman instances");
		provision->result = CPC_ERR_IO;
		provision->finished_source =
			g_idle_add(prv_provision_task_finished, provision);
		goto on_error;
	}

	provision->state = CPC_PROVISION_START;
	prv_provision_step(provision);

on_error:

	return;
}

static void prv_session_proxy_created(GObject *source_object,
				      GAsyncResult *result, gpointer user_data)
{
	omacp_provision_t *provision = user_data;
	int err;

	err = prv_proxy_created(source_object, result, user_data,
				&provision->session_proxy);

	if (err == CPC_ERR_CANCELLED)
		goto on_error;

	if (provision->settings.system_settings) {
		g_cancellable_reset(provision->cancellable);
		g_dbus_proxy_new_for_bus(
			G_BUS_TYPE_SYSTEM,
			G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES, NULL,
			PROVMAN_SERVER_NAME, PROVMAN_OBJECT,
			PROVMAN_INTERFACE,
			provision->cancellable,
			prv_system_proxy_created, provision);
	} else {
		prv_provision_start(provision);
	}

on_error:

	return;
}

static void prv_system_proxy_created(GObject *source_object,
				     GAsyncResult *result, gpointer user_data)
{
	omacp_provision_t *provision = user_data;
	int err;

	err = prv_proxy_created(source_object, result, user_data,
				&provision->system_proxy);

	if (err != CPC_ERR_CANCELLED)
		prv_provision_start(provision);
}

void cpc_provision_apply(cpc_context_t *context, const char *imsi,
			 cpc_provision_cb_t callback, void *user_data,
			 cpc_provision_handle_t *handle)
{
	omacp_provision_t *provision;

	provision = g_new0(omacp_provision_t, 1);

	cpc_settings_init(&provision->settings, context);

	provision->current_objects =
		g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	provision->finished = callback;
	provision->finished_data = user_data;
	provision->imsi = g_strdup(imsi);

	provision->cancellable = g_cancellable_new();

	if (provision->settings.session_settings) {
		CPC_LOGF("Creating session proxy");
		g_dbus_proxy_new_for_bus(
			G_BUS_TYPE_SESSION,
			G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES, NULL,
			PROVMAN_SERVER_NAME, PROVMAN_OBJECT,
			PROVMAN_INTERFACE,
			provision->cancellable,
			prv_session_proxy_created, provision);
	} else if (provision->settings.system_settings) {
		CPC_LOGF("Creating system proxy");
		g_dbus_proxy_new_for_bus(
			G_BUS_TYPE_SYSTEM,
			G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES, NULL,
			PROVMAN_SERVER_NAME, PROVMAN_OBJECT,
			PROVMAN_INTERFACE,
			provision->cancellable,
			prv_system_proxy_created, provision);
	} else {
		provision->result = CPC_ERR_NONE;
		provision->finished_source =
			g_idle_add(prv_provision_task_finished, provision);
		CPC_LOGF("No settings to provision");
	}

	*handle = provision;
}

void cpc_provision_apply_cancel(cpc_provision_handle_t handle)
{
	omacp_provision_t *provision = handle;

	if (provision->cancellable && !provision->finished_source)
		g_cancellable_cancel(provision->cancellable);
}
