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
 * @file <context.h>
 *
 * @brief Interface for parsing and applying OMA CP files
 *
 * The structures and enumerated types in this file are based on types defined
 * in the original ACCESS file omadm_provisioning.h. Structure members have been
 * modified by Intel to improve consistency.  In addition all identifiers have
 * been renamed to match the cpclient's coding standards.
 *
 * The prototypes cpc_provisioned_set_iterator_make and
 * cpc_provisioned_set_iterator_next were taken from the original ACCESS file
 * omadm_appsettings_prv.h.
 *
 * The structures cpc_application_t and cpc_context_t are new Intel code.
 * In addition, the prototypes cpc_context_new and cpc_context_delete have
 * also been newly created by Intel, although the comments that preceed them are
 * derived from comments in the original ACCESS file, omadm_cp_parser_prv.h.
 *
 *****************************************************************************/

#ifndef CPC_CONTEXT_H__
#define CPC_CONTEXT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "ptr-array.h"

enum cpc_provisioned_type_ {
	CPC_TYPE_PROXY = 1,
	CPC_TYPE_CONNECTION_PROFILE = 1 << 1,
	CPC_TYPE_BROWSER = 1 << 2,
	CPC_TYPE_EMAIL = 1 << 3,
	CPC_TYPE_MMS = 1 << 4,
	CPC_TYPE_OMADS = 1 << 5,
	CPC_TYPE_OMADM = 1 << 6,
	CPC_TYPE_IMPS = 1 << 7,
	CPC_TYPE_CONNECTION_RULES = 1 << 8,
	CPC_TYPE_OMADL = 1 << 9,
	CPC_TYPE_SUPL = 1 << 10,
	CPC_TYPE_MAX = 1 << 11
};

typedef enum cpc_provisioned_type_ cpc_provisioned_type;

typedef uint32_t cpc_provisioned_set;

typedef struct cpc_provisioned_set_iter_ cpc_provisioned_set_iter;

struct cpc_provisioned_set_iter_ {
	cpc_provisioned_set set;
	int iter;
};

enum cpc_nd_bearer_t_ {
	CPC_ND_BEARER_NOT_SET,
	CPC_ND_BEARER_CSD,
	CPC_ND_BEARER_GPRS,
	CPC_ND_BEARER_MAX
};

typedef enum cpc_nd_bearer_t_ cpc_nd_bearer_t;

enum cpc_nd_address_type_t_ {
	CPC_ND_ADDRESSTYPE_NOT_SET,
	CPC_ND_ADDRESSTYPE_APN,
	CPC_ND_ADDRESSTYPE_E164,
	CPC_ND_ADDRESSTYPE_MAX
};
typedef enum cpc_nd_address_type_t_ cpc_nd_address_type_t;

enum cpc_nd_local_address_type_t_ {
	CPC_ND_LOCALADDRESSTYPE_NOT_SET,
	CPC_ND_LOCALADDRESSTYPE_IP4,
	CPC_ND_LOCALADDRESSTYPE_IP6,
	CPC_ND_LOCALADDRESSTYPE_MAX
};
typedef enum cpc_nd_local_address_type_t_ cpc_nd_local_address_type_t;

enum cpc_nd_auth_type_t_ {
	CPC_ND_AUTHTYPE_NOT_SET,
	CPC_ND_AUTHTYPE_PAP,
	CPC_ND_AUTHTYPE_CHAP,
	CPC_ND_AUTHTYPE_MAX
};
typedef enum cpc_nd_auth_type_t_ cpc_nd_auth_type_t;

typedef struct cpc_nd_auth_t_ cpc_nd_auth_t;
struct cpc_nd_auth_t_ {
	cpc_nd_auth_type_t auth_type;
	char *auth_id;
	char *auth_pw;
};

typedef struct cpc_napdef_t_ cpc_napdef_t;
struct cpc_napdef_t_
{
	char *id;
	char *name;
	cpc_nd_bearer_t bearer;
	bool generic;
	char *address;
	cpc_nd_address_type_t address_type;
	char *local_address;
	cpc_nd_local_address_type_t local_address_type;
	unsigned int linger;
	cpc_ptr_array_t credentials;   /* Array of cpc_nd_auth_t */
	cpc_ptr_array_t dns_addresses; /* Array of char * */
};


enum cpc_proxy_auth_type_t_ {
	CPC_PROXY_AUTHTYPE_NOT_SET,
	CPC_PROXY_AUTHTYPE_HTTPBASIC,
	CPC_PROXY_AUTHTYPE_HTTPDIGEST,
	CPC_PROXY_AUTHTYPE_WTLS_SS,
	CPC_PROXY_AUTHTYPE_MAX
};

typedef enum cpc_proxy_auth_type_t_ cpc_proxy_auth_type_t;

enum cpc_proxy_addr_type_t_ {
	CPC_PROXY_ADDRTYPE_NOT_SET,
	CPC_PROXY_ADDRTYPE_ALPHA,
	CPC_PROXY_ADDRTYPE_IP4,
	CPC_PROXY_ADDRTYPE_IP6,
	CPC_PROXY_ADDRTYPE_MAX
};
typedef enum cpc_proxy_addr_type_t_ cpc_proxy_addr_type_t;

enum cpc_port_service_t_ {
	CPC_PROXY_PORTSERVICE_NOT_SET,
	CPC_PROXY_PORTSERVICE_ALPHA_FTP,
	CPC_PROXY_PORTSERVICE_ALPHA_HTTP,
	CPC_PROXY_PORTSERVICE_ALPHA_HTTPS,
	CPC_PROXY_PORTSERVICE_MAX
};
typedef enum cpc_port_service_t_ cpc_port_service_t;

typedef struct cpc_port_t_ cpc_port_t;
struct cpc_port_t_ {
	unsigned int number;
	cpc_port_service_t service;
};

typedef struct cpc_physical_proxy_t_ cpc_physical_proxy_t;
struct cpc_physical_proxy_t_ {
	char *address;
	cpc_proxy_addr_type_t address_type;
	cpc_ptr_array_t napdefs; /* Array of cpc_connectoid_t */
	cpc_ptr_array_t ports; /* Array of cpc_port_t */
};

typedef struct cpc_proxy_t_ cpc_proxy_t;
struct cpc_proxy_t_ {
	char *id;
	char *name;
	char *start_page;
	char *auth_id;
	char *auth_pw;
	cpc_proxy_auth_type_t auth_type;
	cpc_ptr_array_t physical_proxies; /* Array of cpc_physical_proxy_t */
};

enum cpc_connectoid_type_t_ {
	CPC_CONNECTOID_INTERNET,
	CPC_CONNECTOID_NAPDEF,
	CPC_CONNECTOID_PROXY,
	CPC_CONNECTOID_MAX
};
typedef enum cpc_connectoid_type_t_ cpc_connectoid_type_t;

typedef struct cpc_connectoid_t_ cpc_connectoid_t;
struct cpc_connectoid_t_ {
	cpc_connectoid_type_t type;
	union {
		cpc_napdef_t *napdef;
		cpc_proxy_t *proxy;
	};
};

typedef struct cpc_mms_t_ cpc_mms_t;
struct cpc_mms_t_ {
	char *mmsc;
	cpc_ptr_array_t connectoids; /* Array of cpc_connectoid_t */
};

enum cpc_email_server_type_t_ {
	CPC_EMAIL_SERVER_NOT_SET,
	CPC_EMAIL_SERVER_POP,
	CPC_EMAIL_SERVER_IMAP,
	CPC_EMAIL_SERVER_SMTP,
	CPC_EMAIL_SERVER_MAX
};
typedef enum cpc_email_server_type_t_ cpc_email_server_type_t;

enum cpc_email_auth_type_t_ {
	CPC_EMAIL_AUTH_TYPE_NOT_SET,
	CPC_EMAIL_AUTH_TYPE_PLAIN,
	CPC_EMAIL_AUTH_TYPE_NTLM,
	CPC_EMAIL_AUTH_TYPE_GSSAPI,
	CPC_EMAIL_AUTH_TYPE_CRAM_MD5,
	CPC_EMAIL_AUTH_TYPE_DIGEST_MD5,
	CPC_EMAIL_AUTH_TYPE_POPB4SMTP,
	CPC_EMAIL_AUTH_TYPE_LOGIN,
	CPC_EMAIL_AUTH_TYPE_APOP,
	CPC_EMAIL_AUTH_TYPE_MAX
};

typedef enum cpc_email_auth_type_t_ cpc_email_auth_type_t;

typedef struct cpc_email_transport_t_ cpc_email_transport_t;
struct cpc_email_transport_t_ {
	cpc_email_server_type_t server_type;
	char *server_address;
	unsigned int server_port;
	bool use_ssl;
	cpc_email_auth_type_t auth_type;
	char *user_name;
	char *password;
};

typedef struct cpc_email_t_ cpc_email_t;
struct cpc_email_t_ {
	char *name;
	char *id;

	cpc_email_transport_t *incoming;
	cpc_email_transport_t *outgoing;

	char *email_address;
};

typedef enum cpc_syncml_auth_type_t_ cpc_syncml_auth_type_t;
enum cpc_syncml_auth_type_t_ {
	CPC_SYNCML_AUTH_TYPE_NOT_SET,
	CPC_SYNCML_AUTH_TYPE_HTTP_BASIC,
	CPC_SYNCML_AUTH_TYPE_HTTP_DIGEST,
	CPC_SYNCML_AUTH_TYPE_BASIC,
	CPC_SYNCML_AUTH_TYPE_DIGEST,
	CPC_SYNCML_AUTH_TYPE_X509,
	CPC_SYNCML_AUTH_TYPE_SECUREID,
	CPC_SYNCML_AUTH_TYPE_SAFEWORD,
	CPC_SYNCML_AUTH_TYPE_DIGIPASS,
	CPC_SYNCML_AUTH_TYPE_MAX
};

typedef enum cpc_syncml_sync_type_t_ cpc_syncml_sync_type_t;
enum cpc_syncml_sync_type_t_ {
	CPC_SYNCML_SYNC_TYPE_NOT_SET,
	CPC_SYNCML_SYNC_TYPE_SLOW,
	CPC_SYNCML_SYNC_TYPE_TWO_WAY,
	CPC_SYNCML_SYNC_TYPE_ONE_WAY_CLIENT,
	CPC_SYNCML_SYNC_TYPE_REFRESH_CLIENT,
	CPC_SYNCML_SYNC_TYPE_ONE_WAY_SERVER,
	CPC_SYNCML_SYNC_TYPE_REFRESH_SERVER,
	CPC_SYNCML_SYNC_TYPE_MAX
};

typedef struct cpc_syncml_creds_t_ cpc_syncml_creds_t;
struct cpc_syncml_creds_t_ {
	/* Credentials structure is valid if auth_type !=
	   CPC_SYNCML_AUTH_TYPE_NOT_SET */
	cpc_syncml_auth_type_t auth_type;
	char *user_name;
	char *password;
	char *nonce; /* b64 encoded */
};

typedef struct cpc_syncml_db_t_ cpc_syncml_db_t;
struct cpc_syncml_db_t_
{
	char *name;
	cpc_syncml_sync_type_t sync_type;
	char *cli_uri;
	char *uri;
	char *accept_types;
	cpc_syncml_creds_t creds;
};

typedef struct cpc_syncml_t_ cpc_syncml_t;
struct cpc_syncml_t_ {
	char *name;
	char *server_id;
	char *address;
	unsigned int port;
	cpc_syncml_creds_t client_creds;
	cpc_syncml_creds_t server_creds;
	cpc_syncml_creds_t http_creds;
	cpc_ptr_array_t connectoids; /* Array of cpc_connectoid_t */
};

typedef struct cpc_omads_t_ cpc_omads_t;
struct cpc_omads_t_ {
	cpc_syncml_t syncml;
	cpc_ptr_array_t dbs;   /* Array of cpc_syncml_db_t */
};

typedef struct cpc_omadm_t_ cpc_omadm_t;
struct cpc_omadm_t_ {
	cpc_syncml_t syncml;
	bool init;
};

typedef struct cpc_bookmark_t_ cpc_bookmark_t;
struct cpc_bookmark_t_ {
	char *name;
	char *url;
	char *user_name;
	char *password;
};

typedef struct cpc_browser_t_ cpc_browser_t;
struct cpc_browser_t_ {
	char *name;
	int start_page_index; /* -1 if no start page set */
	cpc_ptr_array_t bookmarks; /* Array of cpc_bookmark_t */
	cpc_ptr_array_t connectoids; /* Array of cpc_connectoid_t */
};

enum cpc_application_type_t_ {
	CPC_APPLICATION_MMS = CPC_TYPE_MMS,
	CPC_APPLICATION_DM = CPC_TYPE_OMADM,
	CPC_APPLICATION_EMAIL = CPC_TYPE_EMAIL,
	CPC_APPLICATION_OMADS = CPC_TYPE_OMADS,
	CPC_APPLICATION_OMADM = CPC_TYPE_OMADM,
	CPC_APPLICATION_BROWSER = CPC_TYPE_BROWSER
};

typedef enum cpc_application_type_t_ cpc_application_type_t;

typedef struct cpc_application_t_ cpc_application_t;
struct cpc_application_t_ {
	cpc_application_type_t type;
	union {
		cpc_mms_t mms;
		cpc_email_t email;
		cpc_omads_t omads;
		cpc_omadm_t omadm;
		cpc_browser_t browser;
	};
};

typedef struct cpc_context_t_ cpc_context_t;
struct cpc_context_t_
{
	cpc_ptr_array_t napdefs;
	cpc_ptr_array_t proxies;
	cpc_ptr_array_t applications;
};

/*!
 * @brief Frees all the memory allocated to an cpc_context_t.
 * @param context the cpc_context to free.
 */

void cpc_context_delete(cpc_context_t *context);

/*!
 * @brief Parses an OMA CP XML document and generates an in memory model of the
 * document. This function applies almost all of the validity checks to the
 * document as described in ProvCont and ProvUAB.
 *
 * @param prov_data a pointer to an in memory document.
 * @param data_length length in bytes of the in memory XML document
 * @param context The in memory model is returned via this parameter,
 * if the function succeeds.  The caller needs to delete this model by calling
 * cpc_context_delete when it is finished with it.
 *
 * @return CPC_ERR_NONE The document was correctly parsed and an in
 * memory representation of the model is pointed to by context.
 * @return CPC_ERR_OOM The document could not be parsed correctly due to
 * an OOM.
 * @return CPC_ERR_CORRUPT The document is corrupt.  This is either
 * because it is not a valid XML document or because it does not contain any
 * valid OMA CP characteristics.
 */

int cpc_context_new(const char *prov_data, int data_length,
		    cpc_context_t **context);

/*!
 * @brief Initialises an iterator for the cpc_provisioned_set computed by
 * cpc_analyse_cp_model
 *
 * @param set a cpc_provisioned_set
 * @param iter the iterator
 */

void cpc_provisioned_set_iterator_make(cpc_provisioned_set set,
				       cpc_provisioned_set_iter *iter);

/*!
 * @brief Returns the next setting type in the cpc_provisioned_set associated
 * with the given iterator.
 *
 * @param iter a cpc_provisioned_set_iter
 *
 * @returns a setting type or CPC_TYPE_MAX if there are no settings left.
 */

cpc_provisioned_type cpc_provisioned_set_iterator_next(
	cpc_provisioned_set_iter *iter);


/*!
 * @brief Identifies the settings contained in an in memory cp document.
 * It also returns a list of DM server accounts defined in the cp document
 * that have the init parameter set.
 *
 * @param context a pointer to an cpc_context_t
 * @param set a BitSet that contains the types of settings stored in
 * characteristic.
 * @param start_sessions a cpc_ptr_array_t of DM server identifiers.  Needs
 * to be deleted with cpc_ptr_array_free followed by free.
 *
 * @return CPC_ERR_NONE The document was correctly analysed
 * @return CPC_ERR_OOM The document could not be analysed correctly due to
 * an OOM.
 */

int cpc_context_analyse(cpc_context_t *context,
			cpc_provisioned_set *set,
			cpc_ptr_array_t **start_sessions);


#ifdef __cplusplus
}
#endif

#endif
