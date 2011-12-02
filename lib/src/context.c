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

/******************************************************************************
 * Copyright (c) 1999-2008 ACCESS CO., LTD. All rights reserved.
 * Copyright (c) 2006 PalmSource, Inc (an ACCESS company). All rights reserved.
 *****************************************************************************/

/*!
 * @file <context.c>
 *
 * @brief Functions for creating, inspecting and destroying OMA CP contexts
 *
 * All functions in this file, unless otherwise mentioned, are based on
 * algorithms taken from the original ACCESS source file, omadm_cp_to_dm.c.
 * However, the functions derived from ACCESS code do differ from the originals
 * as they map OMA CP characteristics to a different provisioning data model
 * than to that used by the original ACCESS DMClient.
 *
 * cpc_context_analyse, and all the new, delete and dump functions that
 * create, delete and log the structures defined in context.h are
 * new Intel code.
 *
 * The functions cpc_provisioned_set_iterator_make and
 * cpc_provisioned_set_iterator_next are taken from the ACCESS file
 * omadm_appsettings.c.
 *
 * All identifiers used in ACCESS code have been renamed by Intel to match the
 * coding standards of the cpclient.  In addition, the formatting of the code
 * has been modified to conform to the cpclient coding standards.
 *
 *****************************************************************************/

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "error-macros.h"
#include "log.h"

#include "context.h"
#include "characteristic.h"

#define CPC_CONTEXT_BLOCK_SIZE 8
#define CPC_CONTEXT_BUFFER_SIZE 128

typedef struct email_account_t_ email_account_t;
struct email_account_t_
{
	cpc_characteristic_t *incoming;
	cpc_characteristic_t *outgoing;
};

typedef struct string_int_map_t_ string_int_map_t;
struct string_int_map_t_
{
	int int_value;
	const char* string_value;
};

void cpc_context_delete(cpc_context_t *context)
{
	if (context) {
		cpc_ptr_array_free(&context->napdefs);
		cpc_ptr_array_free(&context->proxies);
		cpc_ptr_array_free(&context->applications);
		free(context);
	}
}

static int prv_nd_auth_new(cpc_nd_auth_t **nd_auth)
{
	CPC_ERR_MANAGE;
	cpc_nd_auth_t *retval;

	CPC_FAIL_NULL(retval, malloc(sizeof(*retval)), CPC_ERR_OOM);
	retval->auth_id = retval->auth_pw = NULL;
	*nd_auth = retval;

CPC_ON_ERR:

	return CPC_ERR;
}

static void prv_nd_auth_delete(void *nd_auth)
{
	cpc_nd_auth_t *auth;

	if (nd_auth) {
		auth = nd_auth;
		free(auth->auth_id);
		free(auth->auth_pw);
		free(nd_auth);
	}
}

static int prv_napdef_new(cpc_napdef_t **napdef)
{
	CPC_ERR_MANAGE;
	cpc_napdef_t *retval;

	CPC_FAIL_NULL(retval, malloc(sizeof(*retval)), CPC_ERR_OOM);
	memset(retval, 0, sizeof(*retval));

	cpc_ptr_array_make(&retval->credentials, CPC_CONTEXT_BLOCK_SIZE,
			   prv_nd_auth_delete);
	cpc_ptr_array_make(&retval->dns_addresses, CPC_CONTEXT_BLOCK_SIZE,
			   free);

	*napdef = retval;

CPC_ON_ERR:

	return CPC_ERR;
}

static void prv_napdef_delete(void *napdef)
{
	cpc_napdef_t *nd;
	if (napdef) {
		nd = napdef;
		free(nd->id);
		free(nd->name);
		free(nd->address);
		free(nd->local_address);
		cpc_ptr_array_free(&nd->credentials);
		cpc_ptr_array_free(&nd->dns_addresses);
		free(napdef);
	}
}

static int prv_physical_proxy_new(cpc_physical_proxy_t **proxy)
{
	CPC_ERR_MANAGE;
	cpc_physical_proxy_t *retval;

	CPC_FAIL_NULL(retval, malloc(sizeof(*retval)), CPC_ERR_OOM);
	memset(retval, 0, sizeof(*retval));

	cpc_ptr_array_make(&retval->napdefs, CPC_CONTEXT_BLOCK_SIZE, free);
	cpc_ptr_array_make(&retval->ports, CPC_CONTEXT_BLOCK_SIZE, free);

	*proxy = retval;

CPC_ON_ERR:

	return CPC_ERR;
}


static void prv_physical_proxy_delete(void *phys_proxy)
{
	cpc_physical_proxy_t *pxp = phys_proxy;
	if (phys_proxy) {
		free(pxp->address);
		cpc_ptr_array_free(&pxp->napdefs);
		cpc_ptr_array_free(&pxp->ports);
		free(phys_proxy);
	}
}

static int prv_proxy_new(cpc_proxy_t **proxy)
{
	CPC_ERR_MANAGE;
	cpc_proxy_t *retval;

	CPC_FAIL_NULL(retval, malloc(sizeof(*retval)), CPC_ERR_OOM);
	memset(retval, 0, sizeof(*retval));

	cpc_ptr_array_make(&retval->physical_proxies, CPC_CONTEXT_BLOCK_SIZE,
			   prv_physical_proxy_delete);

	*proxy = retval;

CPC_ON_ERR:

	return CPC_ERR;
}

static void prv_proxy_delete(void *proxy)
{
	cpc_proxy_t *px = proxy;
	if (proxy) {
		free(px->id);
		free(px->name);
		free(px->start_page);
		free(px->auth_id);
		free(px->auth_pw);
		cpc_ptr_array_free(&px->physical_proxies);
		free(proxy);
	}
}

static int prv_mms_new(cpc_application_t **mms)
{
	CPC_ERR_MANAGE;
	cpc_application_t *retval;

	CPC_FAIL_NULL(retval, malloc(sizeof(*retval)), CPC_ERR_OOM);
	memset(retval, 0, sizeof(*retval));
	retval->type = CPC_APPLICATION_MMS;
	cpc_ptr_array_make(&retval->mms.connectoids, CPC_CONTEXT_BLOCK_SIZE,
			   free);
	*mms = retval;

CPC_ON_ERR:

	return CPC_ERR;
}

static void prv_mms_delete(cpc_application_t *mms)
{
	if (mms) {
		free(mms->mms.mmsc);
		cpc_ptr_array_free(&mms->mms.connectoids);
		free(mms);
	}
}

static void prv_email_transport_delete(cpc_email_transport_t *trans)
{
	if (trans) {
		free(trans->server_address);
		free(trans->user_name);
		free(trans->password);
		free(trans);
	}
}

static int prv_email_transport_new(cpc_email_transport_t **transport)
{
	CPC_ERR_MANAGE;
	cpc_email_transport_t *retval;

	CPC_FAIL_NULL(retval, malloc(sizeof(*retval)), CPC_ERR_OOM);
	memset(retval, 0, sizeof(*retval));
	*transport = retval;

CPC_ON_ERR:

	return CPC_ERR;
}

static void prv_email_delete(cpc_application_t *app)
{
	cpc_email_t *email;

	if (app) {
		email = &app->email;
		free(email->name);
		free(email->id);
		prv_email_transport_delete(email->incoming);
		prv_email_transport_delete(email->outgoing);
		free(email->email_address);

		free(app);
	}
}

static int prv_email_new(cpc_application_t **email)
{
	CPC_ERR_MANAGE;
	cpc_application_t *retval;

	CPC_FAIL_NULL(retval, malloc(sizeof(*retval)), CPC_ERR_OOM);
	memset(retval, 0, sizeof(*retval));
	retval->type = CPC_APPLICATION_EMAIL;
	*email = retval;

CPC_ON_ERR:

	return CPC_ERR;
}

static void prv_syncml_creds_free(cpc_syncml_creds_t *creds)
{
	free(creds->user_name);
	free(creds->password);
	free(creds->nonce);
}

static void prv_syncml_db_delete(void *db)
{
	cpc_syncml_db_t *ds_db = db;

	if (db) {
		free(ds_db->name);
		free(ds_db->cli_uri);
		free(ds_db->uri);
		free(ds_db->accept_types);
		prv_syncml_creds_free(&ds_db->creds);
		free(db);
	}
}

static void prv_syncml_free(cpc_syncml_t *syncml)
{
	free(syncml->name);
	free(syncml->server_id);
	free(syncml->address);
	prv_syncml_creds_free(&syncml->client_creds);
	prv_syncml_creds_free(&syncml->server_creds);
	prv_syncml_creds_free(&syncml->http_creds);
	cpc_ptr_array_free(&syncml->connectoids);
}

static void prv_omads_delete(cpc_application_t *app)
{
	cpc_omads_t *omads;

	if (app) {
		omads = &app->omads;
		prv_syncml_free(&omads->syncml);
		cpc_ptr_array_free(&omads->dbs);
		free(app);
	}
}

static int prv_omads_db_new(cpc_syncml_db_t **db)
{
	CPC_ERR_MANAGE;
	cpc_syncml_db_t *retval;

	CPC_FAIL_NULL(retval, malloc(sizeof(*retval)), CPC_ERR_OOM);
	memset(retval, 0, sizeof(*retval));
	*db = retval;

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_omads_new(cpc_application_t **omads)
{
	CPC_ERR_MANAGE;
	cpc_application_t *retval;

	CPC_FAIL_NULL(retval, malloc(sizeof(*retval)), CPC_ERR_OOM);
	memset(retval, 0, sizeof(*retval));
	retval->type = CPC_APPLICATION_OMADS;
	cpc_ptr_array_make(&retval->omads.dbs, CPC_CONTEXT_BLOCK_SIZE,
			   prv_syncml_db_delete);
	cpc_ptr_array_make(&retval->omads.syncml.connectoids,
			   CPC_CONTEXT_BLOCK_SIZE, free);

	*omads = retval;

CPC_ON_ERR:

	return CPC_ERR;
}

static void prv_omadm_delete(cpc_application_t *app)
{
	cpc_omadm_t *omadm;

	if (app) {
		omadm = &app->omadm;
		prv_syncml_free(&omadm->syncml);
		free(app);
	}
}

static int prv_omadm_new(cpc_application_t **omadm)
{
	CPC_ERR_MANAGE;
	cpc_application_t *retval;

	CPC_FAIL_NULL(retval, malloc(sizeof(*retval)), CPC_ERR_OOM);
	memset(retval, 0, sizeof(*retval));
	retval->type = CPC_APPLICATION_OMADM;
	cpc_ptr_array_make(&retval->omadm.syncml.connectoids,
			   CPC_CONTEXT_BLOCK_SIZE, free);

	*omadm = retval;

CPC_ON_ERR:

	return CPC_ERR;
}

static void prv_bookmark_delete(void *bookmark)
{
	cpc_bookmark_t *bm;

	if (bookmark) {
		bm = bookmark;
		free(bm->name);
		free(bm->url);
		free(bm->user_name);
		free(bm->password);
		free(bookmark);
	}
}

static int prv_bookmark_new(cpc_bookmark_t **bookmark)
{
	CPC_ERR_MANAGE;
	cpc_bookmark_t *retval;

	CPC_FAIL_NULL(retval, malloc(sizeof(*retval)), CPC_ERR_OOM);
	memset(retval, 0, sizeof(*retval));
	*bookmark = retval;

CPC_ON_ERR:

	return CPC_ERR;
}

static void prv_browser_delete(cpc_application_t *app)
{
	cpc_browser_t *browser;

	if (app) {
		browser = &app->browser;
		free(browser->name);
		cpc_ptr_array_free(&browser->bookmarks);
		cpc_ptr_array_free(&browser->connectoids);
		free(app);
	}
}

static int prv_browser_new(cpc_application_t **browser)
{
	CPC_ERR_MANAGE;
	cpc_application_t *retval;

	CPC_FAIL_NULL(retval, malloc(sizeof(*retval)), CPC_ERR_OOM);
	memset(retval, 0, sizeof(*retval));
	retval->type = CPC_APPLICATION_BROWSER;
	cpc_ptr_array_make(&retval->browser.connectoids,
			   CPC_CONTEXT_BLOCK_SIZE, free);
	cpc_ptr_array_make(&retval->browser.bookmarks,
			   CPC_CONTEXT_BLOCK_SIZE, prv_bookmark_delete);

	*browser = retval;

CPC_ON_ERR:

	return CPC_ERR;
}

static void prv_application_delete(void *application)
{
	cpc_application_t *app = application;

	switch (app->type) {
	case CPC_APPLICATION_MMS:
		prv_mms_delete(app);
		break;
	case CPC_APPLICATION_EMAIL:
		prv_email_delete(app);
		break;
	case CPC_APPLICATION_OMADS:
		prv_omads_delete(app);
		break;
	case CPC_APPLICATION_OMADM:
		prv_omadm_delete(app);
		break;
	case CPC_APPLICATION_BROWSER:
		prv_browser_delete(app);
		break;
	default:
		break;
	}
}

static void prv_map_string_to_uint(const string_int_map_t *map,
				   unsigned int map_size,
				   const xmlChar *string_value,
				   unsigned int default_value,
				   unsigned int *int_value)
{
	unsigned int i = 0;

	for (i = 0; i < map_size &&
		     xmlStrcasecmp((const xmlChar*) map[i].string_value,
				   string_value) != 0; ++i);

	if (i == map_size)
		*int_value = default_value;
	else
		*int_value = map[i].int_value;
}

static int prv_map_napauth(cpc_napdef_t *napdef,
			   cpc_characteristic_t *cristic)
{
	CPC_ERR_MANAGE;
	unsigned int param_index;
	cpc_parameter_t *param;
	cpc_nd_auth_t *nd_auth = NULL;

	const string_int_map_t auth_type_map[] = {
		{CPC_ND_AUTHTYPE_PAP, "PAP"},
		{CPC_ND_AUTHTYPE_CHAP, "CHAP"}
	};

	CPC_FAIL(prv_nd_auth_new(&nd_auth));

	for (param_index = 0; param_index < cpc_get_param_count(cristic);
	     ++param_index) {
		param = cpc_get_param(cristic, param_index);
		if (param->type == CPC_PT_AUTHTYPE) {
			prv_map_string_to_uint(auth_type_map,
					       sizeof(auth_type_map)/
					       sizeof(string_int_map_t),
					       param->utf8_value,
					       CPC_ND_AUTHTYPE_NOT_SET,
					       &nd_auth->auth_type);
		} else if (param->type == CPC_PT_AUTHNAME) {
			CPC_FAIL_NULL(nd_auth->auth_id,
				      strdup((const char*) param->utf8_value),
				      CPC_ERR_OOM);
		} else if (param->type == CPC_PT_AUTHSECRET) {
			CPC_FAIL_NULL(nd_auth->auth_pw,
				      strdup((const char*) param->utf8_value),
				      CPC_ERR_OOM);
		}
	}

	if (nd_auth->auth_type == CPC_ND_AUTHTYPE_NOT_SET)
		CPC_FAIL_FORCE(CPC_ERR_CORRUPT);

	CPC_FAIL(cpc_ptr_array_append(&napdef->credentials, nd_auth));

	nd_auth = NULL;

CPC_ON_ERR:

	prv_nd_auth_delete(nd_auth);

	return CPC_ERR;
}

static cpc_napdef_t *prv_find_napdef(cpc_context_t *context,
				       const char *napdef_id)
{
	unsigned int i;
	cpc_napdef_t *napdef;

	for (i = 0; i < cpc_ptr_array_get_size(&context->napdefs); ++i) {
		napdef = cpc_ptr_array_get(&context->napdefs, i);
		if (!strcmp(napdef->id, napdef_id))
			break;
	}

	return (i < cpc_ptr_array_get_size(&context->napdefs)) ? napdef : NULL;
}

static cpc_proxy_t *prv_find_proxy(cpc_context_t *context,
				      const char *proxy_id)
{
	unsigned int i;
	cpc_proxy_t *proxy;

	for (i = 0; i < cpc_ptr_array_get_size(&context->proxies); ++i) {
		proxy = cpc_ptr_array_get(&context->proxies, i);
		if (!strcmp(proxy->id, proxy_id))
			break;
	}

	return (i < cpc_ptr_array_get_size(&context->proxies)) ? proxy : NULL;
}

static int prv_map_proxy_ports(cpc_physical_proxy_t *pxp,
			       cpc_characteristic_t *cristic)
{
	CPC_ERR_MANAGE;

	cpc_parameter_t *param = NULL;
	cpc_characteristic_t *wap_char = NULL;
	int index = -1;
	unsigned int i = 0;
	int services = 0;
	cpc_port_t *port = NULL;

	const string_int_map_t service_map[] = {
		{ CPC_PROXY_PORTSERVICE_ALPHA_FTP, "FTP" },
		{ CPC_PROXY_PORTSERVICE_ALPHA_HTTP, "HTTP" },
		{ CPC_PROXY_PORTSERVICE_ALPHA_HTTPS, "HTTPS" }
	};

	index = cpc_find_char(cristic, CPC_CT_PORT, 0);
	while (index != -1)
	{
		wap_char = cpc_get_char(cristic, index);

		CPC_FAIL_NULL(port, malloc(sizeof(*port)), CPC_ERR_OOM);
		port->number = 0;
		port->service = CPC_PROXY_PORTSERVICE_NOT_SET;

		for (i = 0; i < cpc_get_param_count(wap_char); ++i)
		{
			param = cpc_get_param(wap_char, i);
			switch (param->type)
			{
			case CPC_PT_PORTNBR:
			{
				port->number = param->int_value;
				break;
			}
			case CPC_PT_SERVICE:
				if (services == 0)
				{
					prv_map_string_to_uint(
						service_map,
						sizeof(service_map)/
						sizeof(string_int_map_t),
						param->utf8_value,
						CPC_PROXY_PORTSERVICE_NOT_SET,
						&port->service);
					++services;
				}
			default:
				break;
			}
		}

		index = cpc_find_char(cristic, CPC_CT_PORT, index + 1);
		services = 0;
		CPC_FAIL(cpc_ptr_array_append(&pxp->ports, port));
		port = NULL;
	}

CPC_ON_ERR:

	free(port);

	return CPC_ERR;
}

static int prv_map_to_napid(cpc_context_t *context,
			    cpc_ptr_array_t *connectoids,
			    const char *napid)
{
	CPC_ERR_MANAGE;

	cpc_connectoid_t *connectoid = NULL;
	cpc_connectoid_type_t type;
	cpc_napdef_t *napdef;

	if (!strcmp(napid, "INTERNET")) {
		type = CPC_CONNECTOID_INTERNET;
		napdef = NULL;
	} else {
		napdef = prv_find_napdef(context, napid);
		if (!napdef)
			goto CPC_ON_ERR;
		type = CPC_CONNECTOID_NAPDEF;
	}

	CPC_FAIL_NULL(connectoid, malloc(sizeof(*connectoid)), CPC_ERR_OOM);

	connectoid->type = type;
	connectoid->napdef = napdef;

	CPC_FAIL(cpc_ptr_array_append(connectoids, connectoid));
	connectoid = NULL;

CPC_ON_ERR:

	free(connectoid);

	return CPC_ERR;
}

static int prv_map_to_proxyid(cpc_context_t *context,
			      cpc_ptr_array_t *connectoids, const char *proxyid)
{
	CPC_ERR_MANAGE;

	cpc_connectoid_t *connectoid = NULL;
	cpc_proxy_t *proxy = prv_find_proxy(context, proxyid);

	if (proxy) {
		CPC_FAIL_NULL(connectoid, malloc(sizeof(*connectoid)),
			      CPC_ERR_OOM);
		connectoid->proxy = proxy;
		connectoid->type = CPC_CONNECTOID_PROXY;

		CPC_FAIL(cpc_ptr_array_append(connectoids, connectoid));
		connectoid = NULL;
	}

CPC_ON_ERR:

	free(connectoid);

	return CPC_ERR;
}

static int prv_map_physical_proxy(cpc_context_t *context,
				  cpc_proxy_t *proxy,
				  cpc_characteristic_t *cristic)
{
	CPC_ERR_MANAGE;

	unsigned int i = 0;
	cpc_parameter_t *param = NULL;
	cpc_physical_proxy_t *pxp = NULL;

	const string_int_map_t px_type_map[] = {
		{ CPC_PROXY_ADDRTYPE_ALPHA, "ALPHA"},
		{ CPC_PROXY_ADDRTYPE_IP4, "IPV4"},
		{ CPC_PROXY_ADDRTYPE_IP6, "IPV6"}
	};

	CPC_FAIL(prv_physical_proxy_new(&pxp));

	for (i = 0; i < cpc_get_param_count(cristic); ++i)
	{
		param = cpc_get_param(cristic, i);
		switch (param->type)
		{
		case CPC_PT_PXADDR:
		{
			CPC_FAIL_NULL(pxp->address,
				      strdup((char*) param->utf8_value),
				      CPC_ERR_OOM);

			break;
		}

		case CPC_PT_PXADDRTYPE:
		{
			prv_map_string_to_uint(px_type_map,
					       sizeof(px_type_map)/
					       sizeof(string_int_map_t),
					       param->utf8_value,
					       CPC_PROXY_ADDRTYPE_NOT_SET,
					       &pxp->address_type);
			break;
		}

		case CPC_PT_TO_NAPID:
		{
			CPC_FAIL(prv_map_to_napid(context, &pxp->napdefs,
						  (const char*)
						  param->utf8_value));
			break;
		}

		default:
			break;
		}
	}

	CPC_FAIL(prv_map_proxy_ports(pxp, cristic));

	CPC_FAIL(cpc_ptr_array_append(&proxy->physical_proxies, pxp));
	pxp = NULL;

 CPC_ON_ERR:

	prv_physical_proxy_delete(pxp);

	return CPC_ERR;
}


static int prv_map_proxy(cpc_context_t *context, cpc_characteristic_t *cristic)
{
	CPC_ERR_MANAGE;

	int index = -1;
	unsigned int i = 0;
	cpc_parameter_t *param = NULL;
	cpc_characteristic_t *wap_char = NULL;
	cpc_proxy_t *proxy = NULL;

	const string_int_map_t px_type_map[] = {
		{CPC_PROXY_AUTHTYPE_HTTPBASIC, "HTTP-BASIC"},
		{CPC_PROXY_AUTHTYPE_HTTPDIGEST, "HTTP-DIGEST"},
		{CPC_PROXY_AUTHTYPE_WTLS_SS, "WTLS-SS"}
	};

	CPC_FAIL(prv_proxy_new(&proxy));

	for (i = 0; i < cpc_get_param_count(cristic); ++i)
	{
		param = cpc_get_param(cristic, i);
		switch (param->type)
		{
		case CPC_PT_PROXY_ID:
		{
			CPC_FAIL_NULL(proxy->id,
				      strdup((char*) param->utf8_value),
				      CPC_ERR_OOM);
			break;
		}
		case CPC_PT_NAME:
		{
			CPC_FAIL_NULL(proxy->name,
				      strdup((char*) param->utf8_value),
				      CPC_ERR_OOM);
			break;
		}
		case CPC_PT_STARTPAGE:
		{
			CPC_FAIL_NULL(proxy->start_page,
				      strdup((char*) param->utf8_value),
				      CPC_ERR_OOM);
			break;
		}
		default:
			break;
		}
	}

	index = cpc_find_char(cristic, CPC_CT_PXAUTHINFO, 0);
	if (index != -1)
	{
		wap_char = cpc_get_char(cristic, index);

		for (i = 0; i < cpc_get_param_count(wap_char); ++i)
		{
			param = cpc_get_param(wap_char, i);
			switch (param->type)
			{
			case CPC_PT_PXAUTH_ID:
			{
				CPC_FAIL_NULL(proxy->auth_id,
					      strdup((char*) param->utf8_value),
					      CPC_ERR_OOM);

				break;
			}
			case CPC_PT_PXAUTH_PW:
			{
				CPC_FAIL_NULL(proxy->auth_pw,
					      strdup((char*) param->utf8_value),
					      CPC_ERR_OOM);
				break;
			}
			case CPC_PT_PXAUTH_TYPE:
			{
				prv_map_string_to_uint(
					px_type_map,
					sizeof(px_type_map)/
					sizeof(string_int_map_t),
					param->utf8_value,
					CPC_PROXY_AUTHTYPE_NOT_SET,
					&proxy->auth_type);
				break;
			}
			default:
				break;
			}
		}
	}

	index = cpc_find_char(cristic, CPC_CT_PXPHYSICAL, 0);
	while (index != -1)
	{
		wap_char = cpc_get_char(cristic, index);
		CPC_FAIL(prv_map_physical_proxy(context, proxy, wap_char));
		index = cpc_find_char(cristic, CPC_CT_PXPHYSICAL, index + 1);
	}

	CPC_FAIL(cpc_ptr_array_append(&context->proxies, proxy));
	proxy = NULL;

 CPC_ON_ERR:

	prv_proxy_delete(proxy);

	return CPC_ERR;
}

static int prv_map_napdef(cpc_context_t *context,
			  cpc_characteristic_t *cristic)
{
	CPC_ERR_MANAGE;

	int index = -1;
	unsigned int i = 0;
	cpc_parameter_t *param = NULL;
	cpc_characteristic_t *wap_char = NULL;
	int bearers = 0;
	cpc_napdef_t *napdef = NULL;
	char *buffer = NULL;

	const string_int_map_t bearer_map[] = {
		{CPC_ND_BEARER_CSD, "GSM-CSD"},
		{CPC_ND_BEARER_GPRS, "GSM-GPRS"}
	};

	const string_int_map_t address_type_map[] = {
		{CPC_ND_ADDRESSTYPE_APN, "APN"},
		{CPC_ND_ADDRESSTYPE_E164, "E164"}
	};

	const string_int_map_t local_address_type_map[] = {
		{CPC_ND_LOCALADDRESSTYPE_IP4,"IPV4"},
		{CPC_ND_LOCALADDRESSTYPE_IP6,"IPV6"}
	};

	CPC_FAIL(prv_napdef_new(&napdef));

	for (i = 0; i < cpc_get_param_count(cristic); ++i)
	{
		param = cpc_get_param(cristic, i);
		switch (param->type)
		{
		case CPC_PT_BEARER:
		{
			if (bearers > 0)
				continue;

			prv_map_string_to_uint(bearer_map,
					       sizeof(bearer_map)/
					       sizeof(string_int_map_t),
					       param->utf8_value,
					       CPC_ND_BEARER_NOT_SET,
					       &napdef->bearer);
				++bearers;
			}
			break;
		case CPC_PT_NAME:
		{
			CPC_FAIL_NULL(napdef->name,
				      strdup((char*) param->utf8_value),
				      CPC_ERR_OOM);
			break;
		}
		case CPC_PT_NAP_ADDRESS:
		{
			CPC_FAIL_NULL(napdef->address,
				      strdup((char*) param->utf8_value),
				      CPC_ERR_OOM);
			break;
		}
		case CPC_PT_NAP_ADDRTYPE:
		{
			prv_map_string_to_uint(address_type_map,
					       sizeof(address_type_map)/
					       sizeof(string_int_map_t),
					       param->utf8_value,
					       CPC_ND_ADDRESSTYPE_NOT_SET,
					       &napdef->address_type);
			break;
		}
		case CPC_PT_LOCAL_ADDR:
		{
			CPC_FAIL_NULL(napdef->local_address,
				      strdup((char*) param->utf8_value),
				      CPC_ERR_OOM);
			break;
		}
		case CPC_PT_LOCAL_ADDRTYPE:
		{
			prv_map_string_to_uint(local_address_type_map,
					       sizeof(local_address_type_map)/
					       sizeof(string_int_map_t),
					       param->utf8_value,
					       CPC_ND_LOCALADDRESSTYPE_NOT_SET,
					       &napdef->local_address_type);
			break;
		}
		case CPC_PT_LINGER:
		{
			napdef->linger = param->int_value;
			break;
		}
		case CPC_PT_DNS_ADDR:
		{
			CPC_FAIL_NULL(buffer,
				      strdup((char*) param->utf8_value),
				      CPC_ERR_OOM);
			CPC_FAIL(cpc_ptr_array_append(&napdef->dns_addresses,
						      buffer));
			buffer = NULL;
			break;
		}
		case CPC_PT_NAPID:
		{
			/* Can only be one and it must exist */

			CPC_FAIL_NULL(napdef->id,
				      strdup((char*) param->utf8_value),
				      CPC_ERR_OOM);
			break;
		}
		case CPC_PT_INTERNET:
		{
			napdef->generic = true;
			break;
		}
		default:
			break;
		}
	}

	index = cpc_find_char(cristic, CPC_CT_NAPAUTHINFO, 0);
	while (index != -1)
	{
		wap_char = cpc_get_char(cristic, index);
		(void) prv_map_napauth(napdef, wap_char);
		index = cpc_find_char(cristic, CPC_CT_NAPAUTHINFO,
			index + 1);
	}
	CPC_FAIL(cpc_ptr_array_append(&context->napdefs, napdef));

	napdef = NULL;

CPC_ON_ERR:

	free(buffer);
	prv_napdef_delete(napdef);

	return CPC_ERR;
}

static int prv_map_mms(cpc_context_t *context,
		       cpc_characteristic_t *cristic)
{
	CPC_ERR_MANAGE;

	cpc_parameter_t *param = NULL;
	unsigned int i;
	int addr = 0;
	cpc_application_t *mms = NULL;

	CPC_FAIL(prv_mms_new(&mms));

	for (i = 0; i < cpc_get_param_count(cristic); ++i)
	{
		param = cpc_get_param(cristic,i);
		switch (param->type)
		{
		case CPC_PT_ADDR:
		{
			if (addr == 0)
			{
				CPC_FAIL_NULL(mms->mms.mmsc,
					      strdup((const char*)
						     param->utf8_value),
					      CPC_ERR_OOM);
				++addr;
			}
			break;
		}
		case CPC_PT_TO_PROXY:
		{
			CPC_FAIL(prv_map_to_proxyid(context,
						    &mms->mms.connectoids,
						    (const char*)
						    param->utf8_value));
			break;
		}
		case CPC_PT_TO_NAPID:
		{
			CPC_FAIL(prv_map_to_napid(context,
						  &mms->mms.connectoids,
						  (const char*)
						  param->utf8_value));
			break;
		}
		default:
			break;
		}
	}

	CPC_FAIL(cpc_ptr_array_append(&context->applications, mms));
	mms = NULL;

CPC_ON_ERR:

	prv_mms_delete(mms);

	return CPC_ERR;
}

static int prv_add_email_account(cpc_ptr_array_t *accounts, bool incoming,
				 cpc_characteristic_t *cristic)
{
	CPC_ERR_MANAGE;
	unsigned int i;
	email_account_t *acc;
	email_account_t *new_acc = NULL;
	cpc_characteristic_t *src;
	cpc_characteristic_t **dst;
	cpc_parameter_t *provid1;
	cpc_parameter_t *provid2;
	int param_index;
	bool duplicate = false;

	for (i = 0; i < cpc_ptr_array_get_size(accounts); ++i) {
		acc = cpc_ptr_array_get(accounts, i);

		if (incoming) {
			src = acc->outgoing;
			dst = &acc->incoming;
		} else {
			src = acc->incoming;
			dst = &acc->outgoing;
		}

		if (!src)
			continue;

		param_index = cpc_find_param(cristic, CPC_PT_PROVIDER_ID, 0);
		if (param_index == -1)
			continue;

		provid2 = cpc_get_param(cristic, param_index);

		if (*dst) {
			param_index = cpc_find_param(*dst, CPC_PT_PROVIDER_ID,
						     0);
			if (param_index == -1)
				continue;

			provid1 = cpc_get_param(*dst, param_index);
			if (xmlStrcmp(provid1->utf8_value, provid2->utf8_value)
			    == 0) {
				duplicate = true;
				break;
			} else {
				continue;
			}
		}

		param_index = cpc_find_param(src, CPC_PT_PROVIDER_ID, 0);
		if (param_index == -1)
			continue;

		provid1 = cpc_get_param(src, param_index);


		if (xmlStrcmp(provid1->utf8_value, provid2->utf8_value) == 0)
			break;
	}

	if (!duplicate) {
		if (i ==  cpc_ptr_array_get_size(accounts)) {
			CPC_FAIL_NULL(new_acc, malloc(sizeof(*new_acc)),
				      CPC_ERR_OOM);
			memset(new_acc, 0, sizeof(*new_acc));
			dst = (incoming) ? &new_acc->incoming :
				&new_acc->outgoing;
			CPC_FAIL(cpc_ptr_array_append(accounts, new_acc));
			new_acc = NULL;
		}

		*dst = cristic;
	}

CPC_ON_ERR:

	free(new_acc);

	return CPC_ERR;
}

static void prv_get_app_address_and_port(cpc_characteristic_t *cristic,
					 const char **address,
					 const char **service,
					 unsigned int *port)
{
	int index = -1;
	cpc_parameter_t *param ;
	cpc_characteristic_t *wap_char;
	const char *addr = NULL;
	const char *svc = NULL;
	unsigned int pt = 0;

	index = cpc_find_param(cristic, CPC_PT_ADDR, 0);
	if (index != -1) {
		param = cpc_get_param(cristic, index);
		addr = (const char *) param->utf8_value;
	}

	index = cpc_find_char(cristic, CPC_CT_APPADDR, 0);
	if (index != -1)
	{
		wap_char = cpc_get_char(cristic, index);
		if (!addr)
		{
			index = cpc_find_param(wap_char, CPC_PT_ADDR, 0);
			if (index != -1) {
				param = cpc_get_param(wap_char, index);
				addr = (const char *) param->utf8_value;
			}
		}

		index = cpc_find_char(wap_char, CPC_CT_PORT, 0);
		if (index != -1)
		{
			wap_char = cpc_get_char(wap_char, index);
			index = cpc_find_param(wap_char, CPC_PT_PORTNBR, 0);
			if (index != -1)
			{
				param = cpc_get_param(wap_char,index);
				pt = param->int_value;
			}

			index = cpc_find_param(wap_char, CPC_PT_SERVICE, 0);
			if (index != -1)
			{
				param = cpc_get_param(wap_char,index);
				svc = (const char *)param->utf8_value;
			}
		}
	}

	*address = addr;
	*port = pt;
	*service = svc;
}

static int prv_map_email_transport(cpc_context_t *context,
				   cpc_characteristic_t *cristic,
				   cpc_email_transport_t *transport,
				   const char *ssl_service)
{
	CPC_ERR_MANAGE;
	cpc_parameter_t *param = NULL;
	const char *address;
	const char *service;
	unsigned int port;
	cpc_characteristic_t *wap_char;
	int index = -1;

	const string_int_map_t auth_type_map[] = {
		{ CPC_EMAIL_AUTH_TYPE_PLAIN, "PLAIN" },
		{ CPC_EMAIL_AUTH_TYPE_NTLM, "NTLM" },
		{ CPC_EMAIL_AUTH_TYPE_GSSAPI, "GSSAPI" },
		{ CPC_EMAIL_AUTH_TYPE_CRAM_MD5, "CRAM-MD5" },
		{ CPC_EMAIL_AUTH_TYPE_DIGEST_MD5, "DIGEST-MD5" },
		{ CPC_EMAIL_AUTH_TYPE_POPB4SMTP, "POPB4SMTP" },
		{ CPC_EMAIL_AUTH_TYPE_LOGIN, "LOGIN" },
		{ CPC_EMAIL_AUTH_TYPE_APOP, "APOP" }
	};

	prv_get_app_address_and_port(cristic, &address, &service, &port);
	if (service && !strcmp(service, ssl_service))
		transport->use_ssl = true;
	else
		transport->use_ssl = false;

	transport->server_port = port;
	if (address)
		CPC_FAIL_NULL(transport->server_address, strdup(address),
			      CPC_ERR_OOM);

	index = cpc_find_char(cristic, CPC_CT_APPAUTH, 0);
	if (index != -1)
	{
		wap_char = cpc_get_char(cristic, index);

		index =  cpc_find_param(wap_char, CPC_PT_AAUTHNAME, 0);
		if (index != -1)
		{
			param = cpc_get_param(wap_char, index);
			CPC_FAIL_NULL(transport->user_name,
				      strdup((const char*) param->utf8_value),
				      CPC_ERR_OOM);
		}

		index =  cpc_find_param(wap_char, CPC_PT_AAUTHSECRET, 0);
		if (index != -1)
		{
			param = cpc_get_param(wap_char, index);
			CPC_FAIL_NULL(transport->password,
				      strdup((const char*) param->utf8_value),
				      CPC_ERR_OOM);
		}

		index =  cpc_find_param(wap_char, CPC_PT_AAUTHTYPE, 0);
		if (index != -1)
		{
			param = cpc_get_param(wap_char, index);
			prv_map_string_to_uint(auth_type_map,
					       sizeof(auth_type_map)/
					       sizeof(string_int_map_t),
					       param->utf8_value,
					       CPC_EMAIL_AUTH_TYPE_NOT_SET,
					       &transport->auth_type);
		}
	}

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_map_email(cpc_context_t *context, email_account_t *acc)
{
	CPC_ERR_MANAGE;

	cpc_application_t *email = NULL;
	cpc_email_transport_t *incoming = NULL;
	cpc_email_transport_t *outgoing = NULL;
	cpc_parameter_t *param = NULL;
	int index = -1;
	unsigned int i;
	const char *ssl_service;

	CPC_FAIL(prv_email_new(&email));

	if (acc->incoming) {
		CPC_FAIL(prv_email_transport_new(&incoming));
		email->email.incoming = incoming;

		index = cpc_find_param(acc->incoming, CPC_PT_APPID, 0);
		param = cpc_get_param(acc->incoming, index);
		if (xmlStrcmp(param->utf8_value, (const xmlChar*) "110")) {
			email->email.incoming->server_type =
				CPC_EMAIL_SERVER_IMAP;
			ssl_service = "993";
		} else {
			email->email.incoming->server_type =
				CPC_EMAIL_SERVER_POP;
			ssl_service = "995";
		}

		CPC_FAIL(prv_map_email_transport(context, acc->incoming,
						 incoming, ssl_service));

		index = cpc_find_param(acc->incoming, CPC_PT_NAME, 0);
		if (index != -1)
		{
			param = cpc_get_param(acc->incoming, index);
			CPC_FAIL_NULL(email->email.name,
				      strdup((const char*) param->utf8_value),
				      CPC_ERR_OOM);
		}

		index = cpc_find_param(acc->incoming, CPC_PT_PROVIDER_ID, 0);
		if (index != -1)
		{
			param = cpc_get_param(acc->incoming, index);
			CPC_FAIL_NULL(email->email.id,
				      strdup((const char*) param->utf8_value),
				      CPC_ERR_OOM);
		}
	}

	if (acc->outgoing) {
		CPC_FAIL(prv_email_transport_new(&outgoing));
		email->email.outgoing = outgoing;
		CPC_FAIL(prv_map_email_transport(context, acc->outgoing,
						 outgoing, "465"));
		email->email.outgoing->server_type = CPC_EMAIL_SERVER_SMTP;

		for (i = 0; i < cpc_get_param_count(acc->outgoing); ++i) {
			param = cpc_get_param(acc->outgoing, i);
			switch (param->type)
			{
			case CPC_PT_NAME:
				if (email->email.name)
					continue;
				CPC_FAIL_NULL(email->email.name,
					      strdup((const char*)
						     param->utf8_value),
					      CPC_ERR_OOM);
				break;
			case CPC_PT_PROVIDER_ID:
				if (email->email.id)
					continue;
				CPC_FAIL_NULL(email->email.id,
					      strdup((const char*)
						     param->utf8_value),
					      CPC_ERR_OOM);
				break;
			case CPC_PT_FROM:
				CPC_FAIL_NULL(email->email.email_address,
					      strdup((const char*)
						     param->utf8_value),
					      CPC_ERR_OOM);
				break;
			default:
				break;
			}
		}
	}

	CPC_FAIL(cpc_ptr_array_append(&context->applications, email));

	email = NULL;

CPC_ON_ERR:

	prv_email_delete(email);

	return CPC_ERR;
}

static int prv_set_syncml_cred(cpc_syncml_creds_t *cred,
			       cpc_characteristic_t *app_auth)
{
	CPC_ERR_MANAGE;
	unsigned int i;
	cpc_parameter_t *param;

	const string_int_map_t auth_type_map[] = {
		{ CPC_SYNCML_AUTH_TYPE_HTTP_BASIC, "HTTP-BASIC"},
		{ CPC_SYNCML_AUTH_TYPE_HTTP_DIGEST, "HTTP-DIGEST"},
		{ CPC_SYNCML_AUTH_TYPE_BASIC, "BASIC"},
		{ CPC_SYNCML_AUTH_TYPE_DIGEST, "DIGEST"},
		{ CPC_SYNCML_AUTH_TYPE_X509, "X509"},
		{ CPC_SYNCML_AUTH_TYPE_SECUREID, "SECUREID"},
		{ CPC_SYNCML_AUTH_TYPE_SAFEWORD, "SAFEWORD"},
		{ CPC_SYNCML_AUTH_TYPE_DIGIPASS, "DIGIPASS"}
	};

	if (cred->auth_type == CPC_SYNCML_AUTH_TYPE_NOT_SET) {
		for (i = 0; i < cpc_get_param_count(app_auth); ++i)
		{
			param = cpc_get_param(app_auth, i);
			switch (param->type) {
			case CPC_PT_AAUTHDATA:
				CPC_FAIL_NULL(cred->nonce,
					      strdup((char*) param->utf8_value),
					      CPC_ERR_OOM);
				break;
			case CPC_PT_AAUTHNAME:
				CPC_FAIL_NULL(cred->user_name,
					      strdup((char*) param->utf8_value),
					      CPC_ERR_OOM);
				break;
			case CPC_PT_AAUTHSECRET:
				CPC_FAIL_NULL(cred->password,
					      strdup((char*) param->utf8_value),
					      CPC_ERR_OOM);
				break;
			case CPC_PT_AAUTHTYPE:
				prv_map_string_to_uint(
					auth_type_map,
					sizeof(auth_type_map)/
					sizeof(string_int_map_t),
					param->utf8_value,
					CPC_SYNCML_AUTH_TYPE_NOT_SET,
					&cred->auth_type);
				break;
			default:
				break;
			}
		}
	}

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_add_syncml_creds(cpc_context_t *context,
				cpc_characteristic_t *cristic,
				cpc_syncml_creds_t *http,
				cpc_syncml_creds_t *client,
				cpc_syncml_creds_t *server)
{
	CPC_ERR_MANAGE;
	int index;
	int param_index;
	cpc_characteristic_t *wap_char;
	cpc_parameter_t *param;

	index = cpc_find_char(cristic, CPC_CT_APPAUTH, 0);
	while (index != -1) {
		wap_char = cpc_get_char(cristic, index);
		param_index = cpc_find_param(wap_char, CPC_PT_AAUTHLEVEL, 0);
		if (param_index == -1)
			CPC_FAIL(prv_set_syncml_cred(http, wap_char));
		else {
			param = cpc_get_param(wap_char, param_index);
			if (xmlStrcmp(param->utf8_value,
				      (const xmlChar*) "APPSRV") == 0)
				CPC_FAIL(prv_set_syncml_cred(server, wap_char));
			else if (xmlStrcmp(param->utf8_value,
					   (const xmlChar*) "CLIENT") == 0)
				CPC_FAIL(prv_set_syncml_cred(client, wap_char));
		}

		index = cpc_find_char(cristic, CPC_CT_APPAUTH,
					   index + 1);
	}

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_add_omads_db(cpc_characteristic_t *wap_char, cpc_syncml_db_t *db)
{
	CPC_ERR_MANAGE;
	unsigned int i;
	cpc_parameter_t *param;

	for (i = 0; i < cpc_get_param_count(wap_char); ++i) {
		param = cpc_get_param(wap_char, i);
		switch (param->type) {
		case CPC_PT_AACCEPT:
			CPC_FAIL_NULL(db->accept_types,
				      strdup((char*) param->utf8_value),
				      CPC_ERR_OOM);
			break;
		case CPC_PT_URI:
			CPC_FAIL_NULL(db->uri,
				      strdup((char*) param->utf8_value),
				      CPC_ERR_OOM);
			break;
		case CPC_PT_CLIURI:
			CPC_FAIL_NULL(db->cli_uri,
				      strdup((char*) param->utf8_value),
				      CPC_ERR_OOM);
			break;
		case CPC_PT_NAME:
			CPC_FAIL_NULL(db->name,
				      strdup((char*) param->utf8_value),
				      CPC_ERR_OOM);

			break;
		case CPC_PT_SYNCTYPE:
			if ((param->int_value > CPC_SYNCML_SYNC_TYPE_NOT_SET)
			    && (param->int_value <=
				CPC_SYNCML_SYNC_TYPE_REFRESH_SERVER))
				db->sync_type = param->int_value;
			break;
		default:
			break;
		}
	}

	CPC_FAIL(prv_set_syncml_cred(&db->creds, wap_char));

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_add_omads_dbs(cpc_application_t *omads,
			     cpc_characteristic_t *cristic)
{
	CPC_ERR_MANAGE;
	int index;
	cpc_characteristic_t *wap_char;
	cpc_syncml_db_t *db = NULL;

	index = cpc_find_char(cristic, CPC_CT_RESOURCE, 0);
	while (index != -1) {
		wap_char = cpc_get_char(cristic, index);
		CPC_FAIL(prv_omads_db_new(&db));
		CPC_FAIL(prv_add_omads_db(wap_char, db));
		CPC_FAIL(cpc_ptr_array_append(&omads->omads.dbs, db));
		db = NULL;
		index = cpc_find_char(cristic, CPC_CT_RESOURCE, index + 1);
	}

CPC_ON_ERR:

	prv_syncml_db_delete(db);

	return CPC_ERR;
}

static int prv_add_syncml_settings(cpc_context_t *context,
				   cpc_characteristic_t *cristic,
				   cpc_syncml_t *syncml)
{
	CPC_ERR_MANAGE;
	unsigned int i;
	cpc_parameter_t *param = NULL;
	const char *address = NULL;
	const char *service = NULL;

	for (i = 0; i < cpc_get_param_count(cristic); ++i)
	{
		param = cpc_get_param(cristic,i);
		switch (param->type)
		{
		case CPC_PT_NAME:
		{
			CPC_FAIL_NULL(syncml->name, strdup((const char*)
							   param->utf8_value),
				      CPC_ERR_OOM);
			break;
		}
		case CPC_PT_PROVIDER_ID:
		{
			CPC_FAIL_NULL(syncml->server_id,
				      strdup((const char*) param->utf8_value),
				      CPC_ERR_OOM);
			break;
		}
		case CPC_PT_TO_PROXY:
		{
			CPC_FAIL(prv_map_to_proxyid(context,
						    &syncml->connectoids,
						    (const char*)
						    param->utf8_value));
			break;
		}
		case CPC_PT_TO_NAPID:
		{
			CPC_FAIL(prv_map_to_napid(context,
						  &syncml->connectoids,
						  (const char*)
						  param->utf8_value));
			break;
		}

		default:
			break;
		}
	}

	prv_get_app_address_and_port(cristic, &address, &service,
				     &syncml->port);
	if (address)
		CPC_FAIL_NULL(syncml->address,
			      strdup((const char*) address), CPC_ERR_OOM);


	CPC_FAIL(prv_add_syncml_creds(context, cristic, &syncml->http_creds,
				      &syncml->client_creds,
				      &syncml->server_creds));

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_add_omads_account(cpc_context_t *context,
				 cpc_characteristic_t *cristic)
{
	CPC_ERR_MANAGE;

	cpc_application_t *omads = NULL;

	CPC_FAIL(prv_omads_new(&omads));
	CPC_FAIL(prv_add_syncml_settings(context, cristic,
					 &omads->omads.syncml));
	CPC_FAIL(prv_add_omads_dbs(omads, cristic));

	CPC_FAIL(cpc_ptr_array_append(&context->applications, omads));
	omads = NULL;

CPC_ON_ERR:

	prv_omads_delete(omads);

	return CPC_ERR;
}

static int prv_add_omadm_account(cpc_context_t *context,
				 cpc_characteristic_t *cristic)
{
	CPC_ERR_MANAGE;

	cpc_application_t *omadm = NULL;

	CPC_FAIL(prv_omadm_new(&omadm));
	CPC_FAIL(prv_add_syncml_settings(context, cristic,
					 &omadm->omadm.syncml));

	omadm->omadm.init = cpc_find_param(cristic, CPC_PT_INIT, 0) !=
		-1;

	CPC_FAIL(cpc_ptr_array_append(&context->applications, omadm));
	omadm = NULL;

CPC_ON_ERR:

	prv_omadm_delete(omadm);

	return CPC_ERR;
}

static int prv_add_bookmark(cpc_browser_t *browser,
			    cpc_characteristic_t *wap_char,
			    bool *start_page)
{
	CPC_ERR_MANAGE;

	cpc_parameter_t *param;
	unsigned int i;
	cpc_bookmark_t *bookmark = NULL;
	bool sp = false;

	CPC_FAIL(prv_bookmark_new(&bookmark));

	for (i = 0; i < cpc_get_param_count(wap_char); ++i) {
		param = cpc_get_param(wap_char, i);
		switch (param->type)
		{
		case CPC_PT_NAME:
			CPC_FAIL_NULL(bookmark->name, strdup((const char*)
							     param->utf8_value),
				      CPC_ERR_OOM);
			break;
		case CPC_PT_URI:
			CPC_FAIL_NULL(bookmark->url, strdup((const char*)
							    param->utf8_value),
				      CPC_ERR_OOM);
			break;
		case CPC_PT_AAUTHNAME:
			CPC_FAIL_NULL(bookmark->user_name,
				      strdup((const char*) param->utf8_value),
				      CPC_ERR_OOM);
			break;
		case CPC_PT_AAUTHSECRET:
			CPC_FAIL_NULL(bookmark->password,
				      strdup((const char*) param->utf8_value),
				      CPC_ERR_OOM);
			break;
		case CPC_PT_STARTPAGE:
			sp = true;
			break;
		default:
			break;
		}
	}

	CPC_FAIL(cpc_ptr_array_append(&browser->bookmarks, bookmark));
	*start_page = sp;
	bookmark = NULL;

CPC_ON_ERR:

	prv_bookmark_delete(bookmark);

	return CPC_ERR;
}

static int prv_add_browser_settings(cpc_context_t *context,
				    cpc_characteristic_t *cristic)
{
	CPC_ERR_MANAGE;

	cpc_browser_t *browser;
	cpc_parameter_t *param;
	cpc_characteristic_t *wap_char;
	int index;
	unsigned int i;
	bool is_start_page;
	cpc_application_t *app = NULL;
	int start_page = -1;
	unsigned int bookmark = 0;

	CPC_FAIL(prv_browser_new(&app));
	browser = &app->browser;

	for (i = 0; i < cpc_get_param_count(cristic); ++i)
	{
		param = cpc_get_param(cristic,i);
		switch (param->type)
		{
		case CPC_PT_NAME:
			CPC_FAIL_NULL(browser->name, strdup((const char*)
							    param->utf8_value),
				      CPC_ERR_OOM);
			break;
		case CPC_PT_TO_PROXY:
			CPC_FAIL(prv_map_to_proxyid(context,
						    &browser->connectoids,
						    (const char*)
						    param->utf8_value));
			break;
		case CPC_PT_TO_NAPID:
			CPC_FAIL(prv_map_to_napid(context,
						  &browser->connectoids,
						  (const char*)
						  param->utf8_value));
		default:
			break;
		}
	}

	index = cpc_find_char(cristic, CPC_CT_RESOURCE, 0);
	while (index != -1) {
		wap_char = cpc_get_char(cristic, index);
		CPC_FAIL(prv_add_bookmark(browser, wap_char, &is_start_page));
		if (start_page == -1 && is_start_page)
			start_page = bookmark;
		index = cpc_find_char(cristic, CPC_CT_RESOURCE, index + 1);
		++bookmark;
	}

	browser->start_page_index = start_page;

	CPC_FAIL(cpc_ptr_array_append(&context->applications, app));

	app = NULL;

CPC_ON_ERR:

	prv_browser_delete(app);

	return CPC_ERR;
}

static int prv_import_characteristic(cpc_context_t *context,
				     cpc_characteristic_t *cristic)
{
	CPC_ERR_MANAGE;

	unsigned int i;
	cpc_characteristic_t *cp_char = NULL;
	cpc_parameter_t *param = NULL;
	int param_index;
	cpc_ptr_array_t email_accounts;
	email_account_t *acc;

	cpc_ptr_array_make(&email_accounts, 8, free);

	for (i = 0; i < cpc_get_char_count(cristic); ++i)
	{
		cp_char = cpc_get_char(cristic, i);
		if (cp_char->type == CPC_CT_NAPDEF)
			CPC_FAIL(prv_map_napdef(context, cp_char));
	}

	for (i = 0; i < cpc_get_char_count(cristic); ++i)
	{
		cp_char = cpc_get_char(cristic, i);
		if (cp_char->type == CPC_CT_PXLOGICAL)
			CPC_FAIL(prv_map_proxy(context, cp_char));
	}

	for (i = 0; i < cpc_get_char_count(cristic); ++i)
	{
		cp_char = cpc_get_char(cristic, i);

		switch (cp_char->type)
		{
		case CPC_CT_APPLICATION:
		{
			param_index = cpc_find_param(cp_char,
							  CPC_PT_APPID, 0);
			if (param_index == -1)
				continue;

			param = cpc_get_param(cp_char, param_index);

			if (xmlStrcmp(param->utf8_value,
				      (const xmlChar*) "w4") == 0)
				CPC_FAIL(prv_map_mms(context, cp_char));
			else if (xmlStrcmp(param->utf8_value,
				      (const xmlChar*) "110") == 0)
				CPC_FAIL(prv_add_email_account(&email_accounts,
							       true, cp_char));
			else if (xmlStrcmp(param->utf8_value,
				      (const xmlChar*) "143") == 0)
				CPC_FAIL(prv_add_email_account(&email_accounts,
							       true, cp_char));
			else if (xmlStrcmp(param->utf8_value,
				      (const xmlChar*) "25") == 0)
				CPC_FAIL(prv_add_email_account(&email_accounts,
							       false, cp_char));
			else if (xmlStrcmp(param->utf8_value,
				      (const xmlChar*) "w5") == 0)
				CPC_FAIL(prv_add_omads_account(context,
							       cp_char));
			else if (xmlStrcmp(param->utf8_value,
				      (const xmlChar*) "w7") == 0)
				CPC_FAIL(prv_add_omadm_account(context,
							       cp_char));
			else if (xmlStrcmp(param->utf8_value,
					   (const xmlChar*) "w2") == 0)
				CPC_FAIL(prv_add_browser_settings(context,
								  cp_char));
			break;
		}
		default:
			break;
		}
	}

	for (i = 0; i < cpc_ptr_array_get_size(&email_accounts); ++i) {
		acc = cpc_ptr_array_get(&email_accounts, i);
		CPC_FAIL(prv_map_email(context, acc));
	}

CPC_ON_ERR:

	cpc_ptr_array_free(&email_accounts);

	return CPC_ERR;
}

#ifdef CPC_LOGGING

static void prv_napdef_dump(cpc_napdef_t *napdef)
{
	unsigned int i;
	cpc_nd_auth_t *nd_auth;

	CPC_LOGF("NAPDEF: %s", napdef->id ? napdef->id : "");
	CPC_LOGF("\tname %s", napdef->name ? napdef->name : "");
	CPC_LOGF("\tbearer %u", napdef->bearer);
	CPC_LOGF("\tgeneric %u", napdef->generic);
	CPC_LOGF("\taddress %s", napdef->address ? napdef->address : "");
	CPC_LOGF("\taddress_type %u", napdef->address_type);
	CPC_LOGF("\tlocal address %s", napdef->local_address ?
		 napdef->local_address : "");
	CPC_LOGF("\tlocal address type %u", napdef->local_address_type);
	CPC_LOGF("\tlinger %u", napdef->linger);

	for (i = 0; i < cpc_ptr_array_get_size(&napdef->credentials); ++i) {
		nd_auth = cpc_ptr_array_get(&napdef->credentials, i);
		CPC_LOGF("\tNAPAUTH type %u", nd_auth->auth_type);
		CPC_LOGF("\tuserid %s", nd_auth->auth_id ? nd_auth->auth_id :
			 "");
		CPC_LOGF("\tpassword %s", nd_auth->auth_pw ? nd_auth->auth_pw :
			 "");
	}
	for (i = 0; i < cpc_ptr_array_get_size(&napdef->dns_addresses); ++i)
		CPC_LOGF("\tDNS ADDRESS %s",
			 cpc_ptr_array_get(&napdef->dns_addresses, i));
	CPC_LOGF("");
}

static void prv_connectoid_dump(cpc_connectoid_t *connectoid)
{
	if (connectoid->type == CPC_CONNECTOID_INTERNET)
		CPC_LOGF("\tTO-NAPID INTERNET");
	else if (connectoid->type == CPC_CONNECTOID_NAPDEF)
		CPC_LOGF("\tTO-NAPID %s", connectoid->napdef->id);
	else if (connectoid->type == CPC_CONNECTOID_PROXY)
		CPC_LOGF("\tTO-PROXY %s", connectoid->proxy->id);
}

static void prv_proxy_dump(cpc_proxy_t *proxy)
{
	unsigned int i;
	unsigned int j;
	cpc_physical_proxy_t *pxp;
	cpc_port_t *port;

	CPC_LOGF("PROXY: %s", proxy->id ? proxy->id : "");
	CPC_LOGF("\tname %s", proxy->name ? proxy->name : "");
	CPC_LOGF("\tstart page %s", proxy->start_page ? proxy->start_page : "");
	CPC_LOGF("\tauth_id %s", proxy->auth_id ? proxy->auth_id : "");
	CPC_LOGF("\tauth_pw %s", proxy->auth_pw ? proxy->auth_pw : "");
	CPC_LOGF("\tauth_type %u", proxy->auth_type);

	for (i = 0; i < cpc_ptr_array_get_size(&proxy->physical_proxies); ++i) {
		pxp = cpc_ptr_array_get(&proxy->physical_proxies, i);
		CPC_LOGF("\tPXPHYSICAL");
		CPC_LOGF("\taddress %s", pxp->address ? pxp->address : "");
		CPC_LOGF("\taddress type %u", pxp->address_type);
		CPC_LOGF("");

		for (j = 0; j < cpc_ptr_array_get_size(&pxp->napdefs); ++j)
			prv_connectoid_dump(cpc_ptr_array_get(&pxp->napdefs,
							      j));

		if (j > 0)
			CPC_LOGF("");

		for (j = 0; j < cpc_ptr_array_get_size(&pxp->ports); ++j) {
			port = cpc_ptr_array_get(&pxp->ports, j);
			CPC_LOGF("\tPORT: %u Type %u", port->number,
				 port->service);
		}
	}
	CPC_LOGF("");
}

static void prv_mms_dump(cpc_mms_t *mms)
{
	unsigned int i;

	CPC_LOGF("MMS");
	CPC_LOGF("\tmmsc %s", mms->mmsc);
	for (i = 0; i < cpc_ptr_array_get_size(&mms->connectoids); ++i)
		prv_connectoid_dump(cpc_ptr_array_get(&mms->connectoids, i));
}

static void prv_email_transport_dump(cpc_email_transport_t *trans)
{
	CPC_LOGF("\t\tserver type %u", trans->server_type);
	CPC_LOGF("\t\tserver address %s", trans->server_address ?
		 trans->server_address : "");
	CPC_LOGF("\t\tserver port %u", trans->server_port);
	CPC_LOGF("\t\tuse ssl %s", trans->use_ssl ? "true" : "false");
	CPC_LOGF("\t\tauth type %u", trans->auth_type);
	CPC_LOGF("\t\tuser name %s", trans->user_name ? trans->user_name : "");
	CPC_LOGF("\t\tpassword %s", trans->password ? trans->password : "");
}

static void prv_email_dump(cpc_email_t *email)
{
	CPC_LOGF("Email");
	CPC_LOGF("\tname %s", email->name ? email->name : "");
	CPC_LOGF("\tid %s", email->id ? email->id : "");
	CPC_LOGF("\temail_address %s", email->email_address ?
		 email->email_address : "");
	if (email->incoming) {
		CPC_LOGF("\tIncoming Transport Settings");
		prv_email_transport_dump(email->incoming);
	}
	if (email->outgoing) {
		CPC_LOGF("\tOutgoing Transport Settings");
		prv_email_transport_dump(email->outgoing);
	}
}

static void prv_dump_syncml_creds(cpc_syncml_creds_t *creds,
				  const char *indent)
{
	CPC_LOGF("%sauth type %u", indent, creds->auth_type);
	CPC_LOGF("%suser name %s", indent, creds->user_name ?
		 creds->user_name : "");
	CPC_LOGF("%spassword %s", indent, creds->password ?
		 creds->password : "");
	CPC_LOGF("%snonce %s", indent, creds->nonce ? creds->nonce : "");
}

static void prv_dump_omads_db(cpc_syncml_db_t *db)
{
	CPC_LOGF("\t\tname %s", db->name ? db->name : "");
	CPC_LOGF("\t\tsync type %u", db->sync_type);
	CPC_LOGF("\t\tclient uri %s", db->cli_uri ? db->cli_uri : "");
	CPC_LOGF("\t\turi %s", db->uri ? db->uri : "");
	CPC_LOGF("\t\taccept types %s", db->accept_types ?
		 db->accept_types : "");
	if (db->creds.auth_type != CPC_SYNCML_AUTH_TYPE_NOT_SET) {
		CPC_LOGF("\t\tDB Credentials:");
		prv_dump_syncml_creds(&db->creds, "\t\t\t");
	}
}

static void prv_syncml_dump(cpc_syncml_t *syncml)
{
	unsigned int i;

	CPC_LOGF("\tname %s", syncml->name ? syncml->name : "");
	CPC_LOGF("\tserver id %s", syncml->server_id ? syncml->server_id : "");
	CPC_LOGF("\taddress %s", syncml->address ? syncml->address : "");
	CPC_LOGF("\tport %u", syncml->port);

	if (syncml->client_creds.auth_type != CPC_SYNCML_AUTH_TYPE_NOT_SET) {
		CPC_LOGF("\tClient Credentials");
		prv_dump_syncml_creds(&syncml->client_creds, "\t\t");
	}

	if (syncml->server_creds.auth_type != CPC_SYNCML_AUTH_TYPE_NOT_SET) {
		CPC_LOGF("\tServer Credentials");
		prv_dump_syncml_creds(&syncml->server_creds, "\t\t");
	}

	if (syncml->http_creds.auth_type != CPC_SYNCML_AUTH_TYPE_NOT_SET) {
		CPC_LOGF("\tHTTP Credentials");
		prv_dump_syncml_creds(&syncml->http_creds, "\t\t");
	}

	for (i = 0; i < cpc_ptr_array_get_size(&syncml->connectoids); ++i)
		prv_connectoid_dump(cpc_ptr_array_get(&syncml->connectoids, i));
}

static void prv_omads_dump(cpc_omads_t *omads)
{
	unsigned int i;

	CPC_LOGF("OMADS");
	prv_syncml_dump(&omads->syncml);
	for (i = 0; i < cpc_ptr_array_get_size(&omads->dbs); ++i) {
		CPC_LOGF("\tDatabase:");
		prv_dump_omads_db(cpc_ptr_array_get(&omads->dbs, i));
	}
}

static void prv_omadm_dump(cpc_omadm_t *omadm)
{
	CPC_LOGF("OMADM");
	prv_syncml_dump(&omadm->syncml);
	CPC_LOGF("\tinit %s", omadm->init ? "Yes" : "No");
}

static void prv_browser_dump(cpc_browser_t *browser)
{
	unsigned int i;
	cpc_bookmark_t *bookmark;

	CPC_LOGF("Browser");
	CPC_LOGF("\tname %s", browser->name ? browser->name : "");

	for (i = 0; i < cpc_ptr_array_get_size(&browser->bookmarks); ++i) {
		bookmark = cpc_ptr_array_get(&browser->bookmarks, i);

		CPC_LOGF("\tBookmark:");
		CPC_LOGF("\t\tname %s", bookmark->name ? bookmark->name : "");
		CPC_LOGF("\t\turl %s", bookmark->url ? bookmark->url : "");
		CPC_LOGF("\t\tuser name %s", bookmark->user_name ?
			 bookmark->user_name : "");
		CPC_LOGF("\t\tpassword %s", bookmark->password ?
			 bookmark->password : "");
	}

	if (browser->start_page_index != -1) {
		bookmark = cpc_ptr_array_get(&browser->bookmarks,
					     browser->start_page_index);

		CPC_LOGF("\tHome Page:");
		CPC_LOGF("\t\turl %s", bookmark->url ? bookmark->url : "");
	}

	for (i = 0; i < cpc_ptr_array_get_size(&browser->connectoids); ++i)
		prv_connectoid_dump(cpc_ptr_array_get(&browser->connectoids,
						      i));
}

static void prv_app_dump(cpc_application_t *app)
{
	switch (app->type) {
	case CPC_APPLICATION_MMS:
		prv_mms_dump(&app->mms);
		break;
	case CPC_APPLICATION_EMAIL:
		prv_email_dump(&app->email);
		break;
	case CPC_APPLICATION_OMADS:
		prv_omads_dump(&app->omads);
		break;
	case CPC_APPLICATION_OMADM:
		prv_omadm_dump(&app->omadm);
		break;
	case CPC_APPLICATION_BROWSER:
		prv_browser_dump(&app->browser);
		break;
	default:
		break;
	}
}

static void prv_context_dump(cpc_context_t *context)
{
	unsigned int i;

	for (i = 0; i < cpc_ptr_array_get_size(&context->napdefs); ++i)
		prv_napdef_dump(cpc_ptr_array_get(&context->napdefs, i));

	for (i = 0; i < cpc_ptr_array_get_size(&context->proxies); ++i)
		prv_proxy_dump(cpc_ptr_array_get(&context->proxies, i));

	for (i = 0; i < cpc_ptr_array_get_size(&context->applications); ++i)
		prv_app_dump(cpc_ptr_array_get(&context->applications, i));
}
#endif

int cpc_context_new(const char *prov_data, int data_length,
		    cpc_context_t **context)
{
	CPC_ERR_MANAGE;
	cpc_context_t *retval;
	cpc_characteristic_t *cristic = NULL;

	CPC_FAIL_NULL(retval, malloc(sizeof(*retval)), CPC_ERR_OOM);
	cpc_ptr_array_make(&retval->napdefs, CPC_CONTEXT_BLOCK_SIZE,
			   prv_napdef_delete);
	cpc_ptr_array_make(&retval->proxies, CPC_CONTEXT_BLOCK_SIZE,
			   prv_proxy_delete);
	cpc_ptr_array_make(&retval->applications, CPC_CONTEXT_BLOCK_SIZE,
			   prv_application_delete);

	CPC_FAIL(cpc_characteristic_new(prov_data, data_length, &cristic));
	CPC_FAIL(prv_import_characteristic(retval, cristic));

#ifdef CPC_LOGGING
	prv_context_dump(retval);
#endif

	*context = retval;
	retval = NULL;

CPC_ON_ERR:

	cpc_characteristic_delete(cristic);
	cpc_context_delete(retval);

	return CPC_ERR;
}

void cpc_provisioned_set_iterator_make(cpc_provisioned_set set,
					 cpc_provisioned_set_iter* iter)
{
	iter->set = set;
	iter->iter = 0;
}

cpc_provisioned_type cpc_provisioned_set_iterator_next(
	cpc_provisioned_set_iter* iter)
{
	unsigned int bit = 1 << iter->iter;

	while ((bit < CPC_TYPE_MAX) && ((iter->set & bit) == 0))
	{
		++iter->iter;
		bit = 1 << iter->iter;
	}

	if (bit < CPC_TYPE_MAX)
		++iter->iter;

	return (cpc_provisioned_type) bit;
}

int cpc_context_analyse(cpc_context_t *context,
			  cpc_provisioned_set *set,
			  cpc_ptr_array_t **start_sessions)
{
	CPC_ERR_MANAGE;
	unsigned int i = 0;
	cpc_application_t *app;
	cpc_ptr_array_t *sessions;
	char *session = NULL;

	CPC_FAIL_NULL(sessions, malloc(sizeof(*sessions)), CPC_ERR_OOM);
	cpc_ptr_array_make(sessions, 4, free);

	*set = 0;

	if (cpc_ptr_array_get_size(&context->napdefs) > 0)
		*set |= CPC_TYPE_CONNECTION_PROFILE;

	if (cpc_ptr_array_get_size(&context->proxies) > 0)
		*set |= CPC_TYPE_PROXY;

	for (i = 0; i < cpc_ptr_array_get_size(&context->applications); ++i) {
		app = cpc_ptr_array_get(&context->applications, i);
		*set |= app->type;
		if ((app->type == CPC_APPLICATION_DM) && (app->omadm.init)) {
			CPC_FAIL_NULL(session,
				      strdup(app->omadm.syncml.server_id),
				      CPC_ERR_OOM);
			CPC_FAIL(cpc_ptr_array_append(sessions, session));
			session = NULL;
		}
	}

	*start_sessions = sessions;

	return CPC_ERR_NONE;

CPC_ON_ERR:

	if (sessions) {
		cpc_ptr_array_free(sessions);
		free(sessions);
	}

	free(session);

	return CPC_ERR;
}
