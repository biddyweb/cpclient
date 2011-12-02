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
 * @file <settings.c>
 *
 * @brief Code to convert an cpc_context_t into a set of provman settings.
 *
 ******************************************************************************/

#include "config.h"

#include <string.h>
#include <glib.h>

#include "context.h"
#include "ptr-array.h"
#include "log.h"
#include "settings.h"

static const char* g_email_auth_type_map[CPC_EMAIL_AUTH_TYPE_MAX] = {
	NULL,         /*CPC_EMAIL_AUTH_TYPE_NOT_SET*/
	"PLAIN",      /*CPC_EMAIL_AUTH_TYPE_PLAIN*/
	"NTLM",       /*CPC_EMAIL_AUTH_TYPE_NTLM*/
	"GSSAPI",     /*CPC_EMAIL_AUTH_TYPE_GSSAPI*/
	"CRAM-MD5",   /*CPC_EMAIL_AUTH_TYPE_CRAM_MD5*/
	"DIGEST-MD5", /*CPC_EMAIL_AUTH_TYPE_DIGEST_MD5*/
	"POPB4SMTP",  /*CPC_EMAIL_AUTH_TYPE_POPB4SMTP*/
	"LOGIN",      /*CPC_EMAIL_AUTH_TYPE_LOGIN*/
	"+APOP"       /*CPC_EMAIL_AUTH_TYPE_APOP*/
};

static const char* g_sync_auth_type_map[CPC_SYNCML_AUTH_TYPE_MAX] = {
	"NULL",        /* CPC_SYNCML_AUTH_TYPE_NOT_SET */
	"http-basic",  /* CPC_SYNCML_AUTH_TYPE_HTTP_BASIC */
	"http-digest", /* CPC_SYNCML_AUTH_TYPE_HTTP_DIGEST */
	"basic",       /* CPC_SYNCML_AUTH_TYPE_BASIC */
	"digest",      /* CPC_SYNCML_AUTH_TYPE_DIGEST */
	"x509",        /* CPC_SYNCML_AUTH_TYPE_X509 */
	"secure-id",   /* CPC_SYNCML_AUTH_TYPE_SECUREID */
	"safeword",    /* CPC_SYNCML_AUTH_TYPE_SAFEWORD */
	"digipass"     /* CPC_SYNCML_AUTH_TYPE_DIGIPASS  */
};

static const char* g_sync_type_map[CPC_SYNCML_SYNC_TYPE_MAX] = {
	"disabled",            /* CPC_SYNCML_SYNC_TYPE_NOT_SET] */
	"slow",                /* CPC_SYNCML_SYNC_TYPE_SLOW] */
	"two-way",             /*CPC_SYNCML_SYNC_TYPE_TWO_WAY] */
	"one-way-from-local",  /*CPC_SYNCML_SYNC_TYPE_ONE_WAY_CLIENT] */
	"refresh-from-local",  /* CPC_SYNCML_SYNC_TYPE_REFRESH_CLIENT */
	"one-way-from-remote", /* CPC_SYNCML_SYNC_TYPE_ONE_WAY_SERVER]*/
	"refresh-from-remote"  /* CPC_SYNCML_SYNC_TYPE_REFRESH_SERVER */
};


static void prv_cpc_meta_data_prop_delete(gpointer prop)
{
	cpc_meta_prop_t *mdp = prop;

	if (mdp) {
		g_free(mdp->key);
		g_free(mdp->prop);
		g_free(mdp->value);
		g_free(prop);
	}
}

static void prv_append_normalised_id(GString *str, const char *id)
{
	const gchar *ptr = id;
	gunichar character;

	while (*ptr) {
		character = g_utf8_get_char(ptr);
		if (!g_ascii_isprint(character) || g_ascii_isspace(character)
			|| character == '/')
			character = '?';
		g_string_append_c(str, (gchar) character);
		ptr = g_utf8_next_char(ptr);
	}
}

static GString *prv_create_base_path(const char* root, const char* id)
{
	GString *path = g_string_new(root);

	prv_append_normalised_id(path, id);
	g_string_append_c(path, '/');

	return path;
}

static gchar* prv_create_path(const char* root, const char* id, const char* key)
{
	GString *path = prv_create_base_path(root, id);

	g_string_append(path, key);

	return g_string_free(path, FALSE);
}

static gchar* prv_create_lvl2_path(const char* root, const char* id,
				   const char *lvl, const char* key)
{
	GString *path = prv_create_base_path(root, id);

	prv_append_normalised_id(path, lvl);
	g_string_append_c(path, '/');
	g_string_append(path, key);

	return g_string_free(path, FALSE);
}

static void prv_add_meta_data(GPtrArray *meta_array, const gchar *key,
			      const gchar *prop, const gchar *value)
{
	cpc_meta_prop_t *md = g_new(cpc_meta_prop_t, 1);

	md->key = g_strdup(key);
	md->prop = g_strdup(prop);
	md->value = g_strdup(value);

	g_ptr_array_add(meta_array, md);
}

static void prv_add_acl(GPtrArray *meta_array, const gchar *root,
			const gchar *id, const gchar *value)
{
	GString *key;

	if (value) {
		key = prv_create_base_path(root, id);
		prv_add_meta_data(meta_array, key->str, "ACL", value);
		(void) g_string_free(key, TRUE);
	}
}

static gchar *prv_compute_acl(cpc_context_t *context)
{
	unsigned int i;
	cpc_application_t *app;
	GString *acl_string = g_string_new("");
	const char *server_id;
	gchar *retval = NULL;

	for (i = 0; i < cpc_ptr_array_get_size(&context->applications); ++i) {
		app = cpc_ptr_array_get(&context->applications, i);
		if (app->type == CPC_APPLICATION_DM) {
			server_id = app->omadm.syncml.server_id;
			if (acl_string->len > 0)
				g_string_append(acl_string, "+");
			g_string_append(acl_string, server_id);
		}
	}

	if (acl_string->len > 0)
		retval = g_strdup_printf("Add=%s&Delete=%s&Replace=%s&Get=%s",
					 acl_string->str, acl_string->str,
					 acl_string->str, acl_string->str);

	(void) g_string_free(acl_string, TRUE);

	return retval;
}

static void prv_napdef_keys_with_id(cpc_napdef_t *napdef,
				    const char *root,
				    const char *id,
				    GHashTable *settings)
{
	gchar *path;
	cpc_nd_auth_t *creds;

	if (napdef->name) {
		path = prv_create_path(root, id, "name");
		g_hash_table_insert(settings, path,
				    g_strdup(napdef->name));
	}

	if (napdef->address) {
		path = prv_create_path(root, id, "apn");
		g_hash_table_insert(settings, path,
				    g_strdup(napdef->address));
	}

	if (cpc_ptr_array_get_size(&napdef->credentials) > 0) {
		creds = cpc_ptr_array_get(&napdef->credentials, 0);
		if (creds->auth_id) {
			path = prv_create_path(root, id, "username");
			g_hash_table_insert(settings, path,
					    g_strdup(creds->auth_id));
		}

		if (creds->auth_pw) {
				path = prv_create_path(root, id, "password");
				g_hash_table_insert(settings, path,
						    g_strdup(creds->auth_pw));
		}
	}
}

static cpc_napdef_t *prv_find_proxy_napdef(cpc_proxy_t *proxy)
{
	cpc_physical_proxy_t *pxp;
	cpc_napdef_t *napdef = NULL;
	unsigned int i;
	cpc_connectoid_t *connectoid;

	if (cpc_ptr_array_get_size(&proxy->physical_proxies) > 0) {
		pxp = cpc_ptr_array_get(&proxy->physical_proxies, 0);

		for (i = 0; i < cpc_ptr_array_get_size(&pxp->napdefs); ++i) {
			connectoid = cpc_ptr_array_get(&pxp->napdefs, i);
			if (connectoid->type == CPC_CONNECTOID_NAPDEF) {
				napdef = connectoid->napdef;
				break;
			}
		}
	}

	return napdef;
}

static gchar* prv_generate_proxy_url(cpc_physical_proxy_t *pxp,
				     unsigned int port_index)
{
	GString *value = g_string_new(pxp->address);
	cpc_port_t *port;

	if (port_index < cpc_ptr_array_get_size(&pxp->ports)) {
		port = cpc_ptr_array_get(&pxp->ports, port_index);
		if (port->number > 0)
			g_string_append_printf(value, ":%u", port->number);
	}

	return g_string_free(value, FALSE);
}

static gchar* prv_generate_syncml_url(cpc_syncml_t *syncml)
{
	GString *value = g_string_new(syncml->address);

	if (syncml->port > 0)
		g_string_append_printf(value, ":%u", syncml->port);

	return g_string_free(value, FALSE);
}

static void prv_napdef_add_proxy(cpc_physical_proxy_t *pxp,
				 unsigned int port_index,
				 GHashTable *settings, const char *id)
{
	const gchar *setting_name;
	gchar *path;
	gchar *url;
	cpc_port_t *port = cpc_ptr_array_get(&pxp->ports, port_index);

	if ((port->service == CPC_PROXY_PORTSERVICE_ALPHA_HTTPS) ||
	    (port->number == 443))
		setting_name = "https_proxy";
	else if ((port->service == CPC_PROXY_PORTSERVICE_ALPHA_FTP) ||
	    (port->number == 21))
		setting_name = "ftp_proxy";
	else
		setting_name = "http_proxy";

	path = prv_create_path("/telephony/contexts/", id,
			       setting_name);
	if (!g_hash_table_lookup(settings, path)) {
		url = prv_generate_proxy_url(pxp, port_index);
		g_hash_table_insert(settings, path, url);
	} else {
		g_free(path);
	}
}

static void prv_napdef_proxy_keys(cpc_context_t *context,
				  cpc_napdef_t *napdef, const char *id,
				  GHashTable *settings)
{
	unsigned int i;
	unsigned int j;
	cpc_proxy_t *proxy;
	cpc_physical_proxy_t *pxp;

	for (i = 0; i < cpc_ptr_array_get_size(&context->proxies); ++i) {
		proxy = cpc_ptr_array_get(&context->proxies, i);
		if (prv_find_proxy_napdef(proxy) == napdef) {
			pxp = cpc_ptr_array_get(&proxy->physical_proxies, 0);
			for (j = 0; j < cpc_ptr_array_get_size(&pxp->ports);
			     ++j)
				prv_napdef_add_proxy(pxp, j, settings, id);
		}
	}
}

static void prv_generate_napdef_keys(cpc_context_t *context,
				     GHashTable *settings, GPtrArray *meta,
				     const gchar *acl_string)
{
	unsigned int i;
	cpc_napdef_t *napdef;
	cpc_napdef_t *mms_napdef = NULL;
	cpc_application_t *app;
	cpc_mms_t *mms = NULL;
	cpc_connectoid_t *connectoid;
	const char *id;

	/* If a NAPDEF is associated with an MMS application we only allow
	   it to be treated as a generic APN if the generic flag is set */

	for (i = 0; i < cpc_ptr_array_get_size(&context->applications); ++i) {
		app = cpc_ptr_array_get(&context->applications, i);
		if (app->type == CPC_APPLICATION_MMS) {
			mms = &app->mms;
			break;
		}
	}

	if (mms && (cpc_ptr_array_get_size(&mms->connectoids) > 0)) {
		connectoid = cpc_ptr_array_get(&mms->connectoids, 0);
		if (connectoid->type == CPC_CONNECTOID_NAPDEF)
			mms_napdef = connectoid->napdef;
		else if (connectoid->type == CPC_CONNECTOID_PROXY)
			mms_napdef = prv_find_proxy_napdef(connectoid->proxy);
	}

	for (i = 0; i < cpc_ptr_array_get_size(&context->napdefs); ++i) {
		napdef = cpc_ptr_array_get(&context->napdefs, i);
		if (!napdef->id)
			continue;

		if (!napdef->generic && (napdef == mms_napdef))
			continue;

		if (!napdef->id[0])
			id = "default";
		else
			id = napdef->id;

		prv_napdef_keys_with_id(napdef, "/telephony/contexts/",
					id, settings);
		prv_napdef_proxy_keys(context, napdef, id, settings);
		prv_add_acl(meta, "/telephony/contexts/", id, acl_string);
	}
}

static void prv_generate_mms_keys(cpc_mms_t *mms, GHashTable *settings,
				  GPtrArray *meta, const gchar *acl_string)
{
	gchar *path;
	cpc_connectoid_t *connectoid;
	cpc_napdef_t *napdef = NULL;
	cpc_physical_proxy_t *pxp = NULL;

	if (cpc_ptr_array_get_size(&mms->connectoids) > 0) {
		connectoid = cpc_ptr_array_get(&mms->connectoids, 0);
		if (connectoid->type == CPC_CONNECTOID_NAPDEF) {
			napdef = connectoid->napdef;
		} else if (connectoid->type == CPC_CONNECTOID_PROXY) {
			napdef = prv_find_proxy_napdef(connectoid->proxy);
			if (cpc_ptr_array_get_size(
				    &connectoid->proxy->physical_proxies) > 0)
				pxp = cpc_ptr_array_get(
					&connectoid->proxy->physical_proxies,
					0);
		}
	}

	if (napdef) {
		if (mms->mmsc) {
			path = prv_create_path("/telephony/mms", "", "mmsc");
			g_hash_table_insert(settings, path,
					    g_strdup(mms->mmsc));
		}

		if (pxp) {
			path = prv_create_path("/telephony/mms","", "proxy");
			g_hash_table_insert(settings, path,
					    prv_generate_proxy_url(pxp, 0));
		}

		prv_napdef_keys_with_id(napdef, "/telephony/mms", "", settings);
		prv_add_acl(meta, "/telephony/mms", "", acl_string);
	}
#ifdef CPC_LOGGING
	else {
		CPC_LOGF("No connectivity settings found for MMS "
			 "characterisitcs.  Ignoring!");
	}
#endif
}

static void prv_generate_email_trans_keys(cpc_email_transport_t *trans,
					  const char *id, const char *type,
					  GHashTable *settings)
{
	gchar *path;
	const char *authtype;
	const char *server_type;

	if (trans->server_address) {
		path = prv_create_lvl2_path("/applications/email/", id, type,
					    "host");
		g_hash_table_insert(settings, path,
				    g_strdup(trans->server_address));
	}

	if (trans->user_name) {
		path = prv_create_lvl2_path("/applications/email/", id, type,
					    "username");
		g_hash_table_insert(settings, path,
				    g_strdup(trans->user_name));
	}

	if (trans->password) {
		path = prv_create_lvl2_path("/applications/email/", id, type,
					    "password");
		g_hash_table_insert(settings, path,
				    g_strdup(trans->password));
	}

	path = prv_create_lvl2_path("/applications/email/", id, type,
				    "usessl");
	g_hash_table_insert(settings, path,
			    g_strdup(trans->use_ssl ? "always" : "never"));

	if (trans->server_port > 0) {
		path = prv_create_lvl2_path("/applications/email/", id, type,
					    "port");
		g_hash_table_insert(settings, path,
				    g_strdup_printf("%u", trans->server_port));
	}

	authtype = g_email_auth_type_map[trans->auth_type];

	if (authtype) {
		path = prv_create_lvl2_path("/applications/email/", id, type,
					    "authtype");
		g_hash_table_insert(settings, path, g_strdup(authtype));
	}

	if (trans->server_type == CPC_EMAIL_SERVER_POP)
		server_type = "pop";
	else if (trans->server_type == CPC_EMAIL_SERVER_IMAP)
		server_type = "imap";
	else
		server_type = "smtp";

	path = prv_create_lvl2_path("/applications/email/", id, type,
				    "type");
	g_hash_table_insert(settings, path, g_strdup(server_type));
}

static void prv_generate_email_keys(cpc_email_t *email, GHashTable *settings,
				    GPtrArray *meta, const gchar *acl_string)
{
	gchar *path;
	const char *id;

	if (email->id)
		id = email->id;
	else if (email->name)
		id = email->name;
	else
		id = "default";

	if (email->name) {
		path = prv_create_path("/applications/email/", id, "name");
		g_hash_table_insert(settings, path, g_strdup(email->name));
	}

	if (email->email_address) {
		path = prv_create_path("/applications/email/", id, "address");
		g_hash_table_insert(settings, path,
				    g_strdup(email->email_address));
	}

	if (email->incoming &&
	    (email->incoming->server_type != CPC_EMAIL_SERVER_NOT_SET))
		prv_generate_email_trans_keys(email->incoming, id, "incoming",
					      settings);

	if (email->outgoing &&
	    (email->outgoing->server_type != CPC_EMAIL_SERVER_NOT_SET))
		prv_generate_email_trans_keys(email->outgoing, id, "outgoing",
					      settings);

	prv_add_acl(meta, "/applications/email/", id, acl_string);
}

static void prv_generate_omads_db_keys(cpc_syncml_db_t *db,
				       const char *id,
				       const char **sync_type_map,
				       GHashTable *settings)
{
	gchar *path;
	const char *name = NULL;

	if (!db->name || (strcmp(db->name, "contacts") &&
			  strcmp(db->name, "calendar") &&
			  strcmp(db->name, "todo") &&
			  strcmp(db->name, "memo"))) {
		if (db->accept_types) {
			if (strstr(db->accept_types, "text/vcard") ||
			    strstr(db->accept_types, "text/x-vcard"))
				name = "contacts";
			else if (strstr(db->accept_types, "text/calendar") ||
				 strstr(db->accept_types, "text/x-vcalendar"))
				name = "calendar";
		}
	}
	else {
		name = db->name;
	}

	if (name) {
		if (db->accept_types) {
			path = prv_create_lvl2_path("/applications/sync/", id,
						    name, "format");
			g_hash_table_insert(settings, path,
					    g_strdup(db->accept_types));
		}

		if (db->uri) {
			path = prv_create_lvl2_path("/applications/sync/", id,
						    name, "uri");
			g_hash_table_insert(settings, path,
					    g_strdup(db->uri));
		}

		if (sync_type_map[db->sync_type]) {
			path = prv_create_lvl2_path("/applications/sync/", id,
						    name, "sync");
			g_hash_table_insert(settings, path,
				g_strdup(sync_type_map[db->sync_type]));
		}
	}
}

static const char* prv_syncml_id(cpc_syncml_t *syncml)
{
	const char *id;

	if (syncml->server_id && syncml->server_id[0])
		id = syncml->server_id;
	else if (syncml->name && syncml->name[0])
		id = syncml->name;
	else
		id = "default";

	return id;
}

static void prv_generate_omads_cred_keys(GHashTable *settings,
					 cpc_syncml_creds_t *creds,
					 const char *id)
{
	gchar *path;

	if (creds->auth_type != CPC_SYNCML_AUTH_TYPE_NOT_SET) {
		if (creds->user_name) {
			path = prv_create_path("/applications/sync/", id,
					       "username");
			g_hash_table_insert(settings, path,
					    g_strdup(creds->user_name));
		}

		if (creds->password) {
			path = prv_create_path("/applications/sync/", id,
					       "password");
			g_hash_table_insert(settings, path,
					    g_strdup(creds->password));
		}
	}
}

static void prv_generate_omads_keys(cpc_omads_t *omads, GHashTable *settings,
				    GPtrArray *meta, const gchar *acl_string)
{
	gchar *path;
	const char *id;
	cpc_syncml_t *syncml;
	unsigned int i;

	syncml = &omads->syncml;
	id = prv_syncml_id(syncml);

	if (syncml->name) {
		path = prv_create_path("/applications/sync/", id, "name");
		g_hash_table_insert(settings, path, g_strdup(syncml->name));
	}

	if (syncml->address) {
		path = prv_create_path("/applications/sync/", id, "url");
		g_hash_table_insert(settings, path,
				    prv_generate_syncml_url(syncml));
	}

	path = prv_create_path("/applications/sync/", id, "client");
	g_hash_table_insert(settings, path, g_strdup("0"));

	prv_generate_omads_cred_keys(settings, &syncml->server_creds, id);

	for (i = 0; i < cpc_ptr_array_get_size(&omads->dbs); ++i)
		prv_generate_omads_db_keys(cpc_ptr_array_get(&omads->dbs, i),
					   id, g_sync_type_map, settings);

	prv_add_acl(meta, "/applications/sync/", id, acl_string);
}

static gchar *prv_get_dm_cred_id(const char *cred_type, const char* setting)
{
	GString *cred_id = g_string_new(cred_type);
	g_string_append_c(cred_id, '/');
	g_string_append(cred_id, setting);
	return g_string_free(cred_id, FALSE);
}

static void prv_generate_omadm_common_cred(GHashTable *settings,
					 cpc_syncml_creds_t *creds,
					 const char *id, const char *cred_type)
{
	gchar *path;
	gchar *cred_id;

	if (creds->user_name) {
		cred_id = prv_get_dm_cred_id(cred_type, "username");
		path = prv_create_path("/applications/omadm/", id,
				       cred_id);
		g_free(cred_id);
		g_hash_table_insert(settings, path,
				    g_strdup(creds->user_name));
	}

	if (creds->password) {
		cred_id = prv_get_dm_cred_id(cred_type, "password");
		path = prv_create_path("/applications/omadm/", id,
				       cred_id);
		g_free(cred_id);
		g_hash_table_insert(settings, path,
				    g_strdup(creds->password));
	}

	cred_id = prv_get_dm_cred_id(cred_type, "authtype");
	path = prv_create_path("/applications/omadm/", id, cred_id);
	g_free(cred_id);
	g_hash_table_insert(settings, path,
			    g_strdup(g_sync_auth_type_map[creds->auth_type]));
}

static void prv_generate_omadm_cred_keys(GHashTable *settings,
					 cpc_syncml_creds_t *creds,
					 const char *id, const char *cred_type)
{
	gchar *path;
	gchar *cred_id;

	if (creds->auth_type != CPC_SYNCML_AUTH_TYPE_NOT_SET) {
		prv_generate_omadm_common_cred(settings, creds, id, cred_type);

		if (creds->nonce) {
			cred_id = prv_get_dm_cred_id(cred_type, "nonce");
			path = prv_create_path("/applications/omadm/", id,
					       cred_id);
			g_free(cred_id);
			g_hash_table_insert(settings, path,
					    g_strdup(creds->nonce));
		}
	}
}

static void prv_generate_omadm_keys(cpc_omadm_t *omadm, GHashTable *settings,
				    GPtrArray *meta, const gchar *acl_string)
{
	gchar *path;
	const char *id;
	cpc_syncml_creds_t *creds;
	cpc_syncml_t *syncml;

	syncml = &omadm->syncml;

	/* server_id must exist and must not be a zero length string*/

	id = syncml->server_id;

	if (syncml->name) {
		path = prv_create_path("/applications/omadm/", id, "name");
		g_hash_table_insert(settings, path, g_strdup(syncml->name));
	}

	path = prv_create_path("/applications/omadm/", id, "server_id");
	g_hash_table_insert(settings, path, g_strdup(syncml->server_id));

	if (syncml->address) {
		path = prv_create_path("/applications/omadm/", id, "url");
		g_hash_table_insert(settings, path,
				    prv_generate_syncml_url(syncml));
	}

	prv_generate_omadm_cred_keys(settings, &syncml->server_creds, id,
				     "server_creds");
	prv_generate_omadm_cred_keys(settings, &syncml->client_creds, id,
				     "client_creds");

	creds = &syncml->http_creds;
	if (creds->auth_type != CPC_SYNCML_AUTH_TYPE_NOT_SET)
		prv_generate_omadm_common_cred(settings, creds, id,
					       "http_creds");

	prv_add_acl(meta, "/applications/omadm/", id, acl_string);
}

static void prv_generate_browser_keys(cpc_browser_t *browser,
				      bool *provisioned_browser,
				      GHashTable *settings, GPtrArray *meta,
				      const gchar *acl_string)
{
	gchar *path;
	cpc_bookmark_t *bookmark;
	unsigned int i;
	const char *id;

	if (*provisioned_browser)
		goto on_err;

	if (cpc_ptr_array_get_size(&browser->bookmarks) == 0)
		goto on_err;

	if (browser->start_page_index != -1) {
		bookmark = cpc_ptr_array_get(&browser->bookmarks,
					     browser->start_page_index);
		if (bookmark->name) {
			path = prv_create_path(
				"/applications/browser/startpage", "", "name");
			g_hash_table_insert(settings, path,
					    g_strdup(bookmark->name));
		}

		/* URL cannot be NULL. */

		path = prv_create_path("/applications/browser/startpage",
				       "", "url");
		g_hash_table_insert(settings, path, g_strdup(bookmark->url));
	}

	for (i = 0; i < cpc_ptr_array_get_size(&browser->bookmarks); ++i) {
		bookmark = cpc_ptr_array_get(&browser->bookmarks, i);

		if (bookmark->name && bookmark->name[0])
			id = bookmark->name;
		else
			id = "default";

		if (bookmark->name) {
			path = prv_create_path(
				"/applications/browser/bookmarks/", id, "name");
			g_hash_table_insert(settings, path,
					    g_strdup(bookmark->name));
		}

		path = prv_create_path("/applications/browser/bookmarks/",
				       id, "url");
		g_hash_table_insert(settings, path, g_strdup(bookmark->url));
	}

	prv_add_acl(meta, "/applications/browser", "", acl_string);
	*provisioned_browser = true;

on_err:

	return;
}

static void prv_generate_app_keys(cpc_context_t *context,
				  GHashTable *system_settings,
				  GHashTable *session_settings,
				  GPtrArray *system_meta,
				  GPtrArray *session_meta,
				  const gchar *acl_string)
{
	unsigned int i;
	cpc_application_t *app;
	bool provisioned_browser = false;

	for (i = 0; i < cpc_ptr_array_get_size(&context->applications); ++i) {
		app = cpc_ptr_array_get(&context->applications, i);
		switch (app->type) {
		case CPC_APPLICATION_MMS:
			prv_generate_mms_keys(&app->mms, system_settings,
					      system_meta, acl_string);
			break;
		case CPC_APPLICATION_EMAIL:
			prv_generate_email_keys(&app->email, session_settings,
						session_meta, acl_string);
			break;
		case CPC_APPLICATION_OMADS:
			prv_generate_omads_keys(&app->omads, session_settings,
						session_meta, acl_string);
			break;
		case CPC_APPLICATION_BROWSER:
			prv_generate_browser_keys(&app->browser,
						  &provisioned_browser,
						  session_settings,
						  session_meta, acl_string);
			break;
		case CPC_APPLICATION_OMADM:
			prv_generate_omadm_keys(&app->omadm, session_settings,
						session_meta, acl_string);

		default:
			break;
		}
	}
}

#ifdef CPC_LOGGING
static void prv_dump_hash_table(GHashTable* hash_table)
{
	gpointer key, value;
	GList *list;
	GList *ptr;

	CPC_LOGF("SETTINGS");

	list = g_hash_table_get_keys(hash_table);
	list = g_list_sort(list, (GCompareFunc) strcmp);
	ptr = list;
	while (ptr) {
		key = ptr->data;
		value = g_hash_table_lookup(hash_table, key);
		CPC_LOGF("%s = %s", key, value);
		ptr = ptr->next;
	}
	g_list_free(list);
}

static void prv_dump_meta_data(GPtrArray *meta)
{
	unsigned int i;
	cpc_meta_prop_t *prop;

	CPC_LOGF("Meta Data");

	for (i = 0; i < meta->len; ++i) {
		prop = g_ptr_array_index(meta, i);
		CPC_LOGF("%s?%s = %s", prop->key, prop->prop, prop->value);
	}
}
#endif

void cpc_settings_init(cpc_settings_t *settings, cpc_context_t *context)
{
	GHashTable *system_settings;
	GHashTable *session_settings;
	GPtrArray *system_meta;
	GPtrArray *session_meta;
	gchar *acl_string;

	memset(settings, 0, sizeof(*settings));

	acl_string = prv_compute_acl(context);

	session_settings = g_hash_table_new_full(g_str_hash, g_str_equal,
						 g_free, g_free);
	system_settings = g_hash_table_new_full(g_str_hash, g_str_equal,
						g_free, g_free);

	system_meta = g_ptr_array_new_with_free_func(
		prv_cpc_meta_data_prop_delete);

	session_meta = g_ptr_array_new_with_free_func(
		prv_cpc_meta_data_prop_delete);

	prv_generate_napdef_keys(context, system_settings, system_meta,
				 acl_string);
	prv_generate_app_keys(context, system_settings, session_settings,
			      system_meta, session_meta, acl_string);

	if (g_hash_table_size(system_settings) > 0)
		settings->system_settings = system_settings;
	else
		g_hash_table_unref(system_settings);

	if (system_meta->len > 0)
		settings->system_meta = system_meta;
	else
		g_ptr_array_unref(system_meta);

	if (g_hash_table_size(session_settings) > 0)
		settings->session_settings = session_settings;
	else
		g_hash_table_unref(session_settings);

	if (session_meta->len > 0)
		settings->session_meta = session_meta;
	else
		g_ptr_array_unref(session_meta);

#ifdef CPC_LOGGING
	if (settings->system_settings)
		prv_dump_hash_table(settings->system_settings);
	if (settings->session_settings)
		prv_dump_hash_table(settings->session_settings);
	if (settings->session_meta)
		prv_dump_meta_data(settings->session_meta);
	if (settings->system_meta)
		prv_dump_meta_data(settings->system_meta);
#endif

	g_free(acl_string);
}

void cpc_settings_free(cpc_settings_t *settings)
{
	if (settings->system_settings)
		g_hash_table_unref(settings->system_settings);
	if (settings->session_settings)
		g_hash_table_unref(settings->session_settings);
	if (settings->session_meta)
		g_ptr_array_unref(settings->session_meta);
	if (settings->system_meta)
		g_ptr_array_unref(settings->system_meta);
}
