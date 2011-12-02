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
 * @file <provision-wp.c>
 *
 * @brief Contains function definitions for provisioning OMA CP WAP Push
 *        messages.
 *
 *****************************************************************************/

#include "config.h"

#include "error.h"
#include "error-macros.h"
#include "log.h"

#include "provision-wp.h"
#include "provision.h"
#include "imsi.h"

struct cpc_provision_wp_t_ {
	cpc_wp_t *wp;
	cpc_context_t *context;
	gchar **imsis;
	cpc_provisioned_set appids;
	cpc_ptr_array_t *start_sessions;
	gchar *pin;
	cpc_cb_t cb;
	void *user_data;
	cpc_imsi_handle_t imsi_handle;
	cpc_provision_handle_t prov_handle;
	int result;
	bool applied;
};

int cpc_provision_wp_new(uint8_t *data, unsigned int length,
			 cpc_provision_wp_t **provision)
{
	CPC_ERR_MANAGE;
	unsigned int prov_doc_size;
	char *prov_doc = NULL;
	cpc_provision_wp_t *prov = g_new0(cpc_provision_wp_t, 1);

	CPC_ERR = cpc_wp_new(data, length, &prov->wp);
	if (CPC_ERR != CPC_ERR_NONE) {
		CPC_LOGF("Fail to create WAP Push context, err = %d", CPC_ERR);
		goto CPC_ON_ERR;
	}

	CPC_ERR = cpc_get_prov_doc(prov->wp, &prov_doc, &prov_doc_size);
	if (CPC_ERR != CPC_ERR_NONE) {
		CPC_LOGF("Fail to retrieve XML document, err = %d", CPC_ERR);
		goto CPC_ON_ERR;
	}

	if (prov_doc_size > INT_MAX) {
		CPC_LOGF("XML document too large, err = %d", CPC_ERR);
		CPC_FAIL_FORCE(CPC_ERR_CORRUPT);
	}

	CPC_ERR = cpc_context_new(prov_doc, prov_doc_size, &prov->context);
	if (CPC_ERR != CPC_ERR_NONE) {
		CPC_LOGF("Fail to parse wap push message, err = %d", CPC_ERR);
		goto CPC_ON_ERR;
	}

	CPC_ERR = cpc_context_analyse(prov->context, &prov->appids,
				      &prov->start_sessions);
	if (CPC_ERR != CPC_ERR_NONE) {
		CPC_LOGF("Fail to analyse wap push message, err = %d", CPC_ERR);
		goto CPC_ON_ERR;
	}

	*provision = prov;
	prov = NULL;

CPC_ON_ERR:

	free(prov_doc);
	cpc_provision_wp_delete(prov);

	return CPC_ERR;
}

void cpc_provision_wp_delete(cpc_provision_wp_t *provision)
{
	if (provision) {
		if (provision->imsis)
			g_strfreev(provision->imsis);
		cpc_context_delete(provision->context);
		cpc_wp_delete(provision->wp);
		if (provision->start_sessions) {
			cpc_ptr_array_free(provision->start_sessions);
			free(provision->start_sessions);
		}
		g_free(provision->pin);
		g_free(provision);
	}
}

bool cpc_provision_wp_pin_required(cpc_provision_wp_t *provision)
{
	cpc_sec_t sec_type;

	sec_type = cpc_wp_security(provision->wp);

	return (sec_type == CPC_SECURITY_USERNETWPIN) ||
		(sec_type == CPC_SECURITY_USERPIN) ||
		(sec_type == CPC_SECURITY_USERPINMAC);
}

const gchar *cpc_provision_wp_get_sec_type(cpc_provision_wp_t *provision)
{
	const char* retval;

	switch (cpc_wp_security(provision->wp)) {
	case CPC_SECURITY_NETWPIN:
		retval = "NETWPIN";
		break;
	case CPC_SECURITY_USERPIN:
		retval = "USERPIN";
		break;
	case CPC_SECURITY_USERNETWPIN:
		retval = "USERNETWPIN";
		break;
	case CPC_SECURITY_USERPINMAC:
		retval = "USERPINMAC";
		break;
	default:
		retval = "NONE";
		break;
	}

	return retval;
}

static const char* prv_setting_to_string(cpc_provisioned_type type)
{
	const char* retval;

	switch (type) {
	case CPC_TYPE_PROXY:
		retval = "proxy";
		break;
	case CPC_TYPE_CONNECTION_PROFILE:
		retval = "apn";
		break;
	case CPC_TYPE_BROWSER:
		retval = "bookmarks";
		break;
	case CPC_TYPE_EMAIL:
		retval = "email";
		break;
	case CPC_TYPE_MMS:
		retval = "mms";
		break;
	case CPC_TYPE_OMADS:
		retval = "omads";
		break;
	case CPC_TYPE_OMADM:
		retval = "omadm";
		break;
	case CPC_TYPE_IMPS:
		retval = "imps";
		break;
	case CPC_TYPE_CONNECTION_RULES:
		retval = "access-rules";
		break;
	case CPC_TYPE_OMADL:
		retval = "omadl";
		break;
	case CPC_TYPE_SUPL:
		retval = "supl";
		break;
	default:
		retval = "unknown";
		break;
	}

	return retval;
}

gchar *cpc_provision_wp_get_settings(cpc_provision_wp_t *provision)
{
	GString *types;
	cpc_provisioned_set_iter iter;
	cpc_provisioned_type type;

	types = g_string_new("");

	cpc_provisioned_set_iterator_make(provision->appids, &iter);
	type = cpc_provisioned_set_iterator_next(&iter);
	if (type != CPC_TYPE_MAX) {
		g_string_append(types, prv_setting_to_string(type));
		type = cpc_provisioned_set_iterator_next(&iter);
		while (type != CPC_TYPE_MAX) {
			g_string_append_c(types, '/');
			g_string_append(types, prv_setting_to_string(type));
			type = cpc_provisioned_set_iterator_next(&iter);
		}
	}

	return g_string_free(types, FALSE);
}

gchar *cpc_provision_wp_get_sessions(cpc_provision_wp_t *provision)
{
	GString *sessions;
	int i;
	int max_sessions;

	sessions = g_string_new("");
	max_sessions = (int) cpc_ptr_array_get_size(provision->start_sessions);

	for (i = 0; i < max_sessions - 1; ++i) {
		g_string_append(sessions,
				cpc_ptr_array_get(provision->start_sessions,
						  i));
		g_string_append_c(sessions, ',');
	}

	if (i < max_sessions)
		g_string_append(sessions,
				cpc_ptr_array_get(provision->start_sessions,
						  i));

	return g_string_free(sessions, FALSE);
}

static int prv_authenticate_message(cpc_provision_wp_t *provision,
				    const gchar *pin)
{
	CPC_ERR_MANAGE;
	const gchar *imsi;
	cpc_sec_t sec_type = cpc_wp_security(provision->wp);
	unsigned int i;

	if ((sec_type == CPC_SECURITY_USERPIN) ||
	    (sec_type == CPC_SECURITY_USERPINMAC)) {
		CPC_ERR = cpc_authenticate(provision->wp, NULL,
					   (const char *) pin);
	} else {
		CPC_ERR = CPC_ERR_DENIED;
		i = 0;
		if (provision->imsis) {
			while ((CPC_ERR != CPC_ERR_NONE) &&
			       provision->imsis[i]) {
				imsi = provision->imsis[i++];
				CPC_ERR = cpc_authenticate(provision->wp, imsi,
							   (const char *) pin);
			}
		}
	}

	CPC_LOGF("Authentication result %d", CPC_ERR);

	return CPC_ERR;
}

static gboolean prv_apply_finished(gpointer user_data)
{
	cpc_provision_wp_t *provision = user_data;

	provision->cb(provision->result, provision->user_data);
	if (provision->result == CPC_ERR_NONE)
		provision->applied = true;

	return FALSE;
}

static void prv_provision_cb(void *user_data, int result)
{
	cpc_provision_wp_t *provision = user_data;

	provision->prov_handle = NULL;
	provision->result = result;
	(void) g_idle_add(prv_apply_finished, user_data);
}

static void prv_imsi_cb(int result, gchar **imsis, void *user_data)
{
	CPC_ERR_MANAGE;
	cpc_provision_wp_t *provision = user_data;

	provision->imsi_handle = NULL;

	CPC_FAIL(result);

	provision->imsis = imsis;
	CPC_FAIL(prv_authenticate_message(provision, provision->pin));
	cpc_provision_apply(provision->context, "", prv_provision_cb,
			    provision, &provision->prov_handle);

	return;

CPC_ON_ERR:

	provision->result = CPC_ERR;
	(void) g_idle_add(prv_apply_finished, user_data);
}

int cpc_provision_wp_apply(cpc_provision_wp_t *provision,
			   const gchar *pin, cpc_cb_t cb, void *user_data)
{
	CPC_ERR_MANAGE;
	cpc_sec_t sec_type;

	sec_type = cpc_wp_security(provision->wp);

	if (!provision->imsis && ((sec_type == CPC_SECURITY_NETWPIN) ||
				 (sec_type == CPC_SECURITY_USERNETWPIN))) {
		CPC_LOGF("Attempting to read IMSI numbers");
		provision->pin = g_strdup(pin);
		cpc_imsi_get(prv_imsi_cb, provision, &provision->imsi_handle);
	} else {
		CPC_FAIL(prv_authenticate_message(provision, pin));
		cpc_provision_apply(provision->context, "", prv_provision_cb,
				    provision, &provision->prov_handle);
	}

	provision->cb = cb;
	provision->user_data = user_data;

CPC_ON_ERR:

	return CPC_ERR;
}

void cpc_provision_wp_apply_cancel(cpc_provision_wp_t *provision)
{
	CPC_LOGF("Cancelling cpc_provision_wp_apply");

	if (provision->imsi_handle)
		cpc_imsi_get_cancel(provision->imsi_handle);
	else if (provision->prov_handle)
		cpc_provision_apply_cancel(provision->prov_handle);
}
