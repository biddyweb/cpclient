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
 * Copyright (c) 1999-2008 ACCESS CO., LTD. All rights reserved.
 * Copyright (c) 2007, ACCESS Systems Americas, Inc. All rights reserved.
 *****************************************************************************/

/*!
 * @file <characteristic.c>
 *
 * @brief Main source file for the omacp parser and data model.
 *
 * This file is based on the ACCESS source file omadm_parser_cp.c.  All
 * identifiers have been renamed by Intel to match the coding standards of the
 * cpclient.  In addition, the formatting of the code has been modified to
 * conform to the cpclient coding standards.  One additional check as been added
 * by Intel to ignore DM accounts that have a zero length PROVIDER-ID.
 * No other logical changes have been made to the algorithms but gotos, via
 * the CPC_FAIL macros, have been introduced to simplify error handling and make
 * the code consistent with the rest of the cpclient.
 *
 *****************************************************************************/

#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "error.h"
#include "error-macros.h"
#include "log.h"

#include "characteristic.h"

typedef struct cpc_char_string_map_t_ cpc_char_string_map_t;
struct cpc_char_string_map_t_ {
	cpc_characteristic_type_t type;
	const char *string;
};

typedef struct cpc_param_string_map_t_ cpc_param_string_map_t;
struct cpc_param_string_map_t_ {
	cpc_param_type_t type;
	const char *string;
};

enum cpc_param_occurrence_t_ {
	CPC_WP_OCCUR_ONCE,
	CPC_WP_OCCUR_ZERO_OR_ONE,
	CPC_WP_OCCUR_ZERO_OR_MORE,
	CPC_WP_OCCUR_ONE_OR_MORE
};
typedef enum cpc_param_occurrence_t_ cpc_param_occurrence_t;

#define CPC_PARSER_MAX_REF 128

/*
 * It's very important that these arrays remain sorted as we are going to use
 * bsearch to convert strings to types.  If you add a new application specific
 * parameter you must remember to resort the entire array.
 */

/*
 * This array is sorted by the strings and not the enumerated types.
 * It is not safe to used the enums as indexes into this array.
 */

static const cpc_char_string_map_t g_char_string_map[] = {
	{CPC_CT_ACCESS, "ACCESS"},
	{CPC_CT_APPADDR, "APPADDR"},
	{CPC_CT_APPAUTH, "APPAUTH"},
	{CPC_CT_APPLICATION, "APPLICATION"},
	{CPC_CT_BOOTSTRAP, "BOOTSTRAP"},
	{CPC_CT_CLIENTIDENTITY, "CLIENTIDENTITY"},
	{CPC_CT_NAPAUTHINFO, "NAPAUTHINFO"},
	{CPC_CT_NAPDEF, "NAPDEF"},
	{CPC_CT_PORT, "PORT"},
	{CPC_CT_PXAUTHINFO, "PXAUTHINFO"},
	{CPC_CT_PXLOGICAL, "PXLOGICAL"},
	{CPC_CT_PXPHYSICAL, "PXPHYSICAL"},
	{CPC_CT_RESOURCE, "RESOURCE"},
	{CPC_CT_ROOT, "ROOT"},
	{CPC_CT_VALIDITY, "VALIDITY"},
	{CPC_CT_VENDORCONFIG, "VENDORCONFIG"}
};

/*
 * This array is sorted by the strings and not the enumerated types.
 * It is not safe to used the enums as indexes into this array.
 * It is not possible to have this array sorted in the same order as the
 * WAPParamType enum as the strings contain '-' chars which we cannot represent
 * in a c identifier.
 */

static const cpc_param_string_map_t g_param_string_map[] = {
	{CPC_PT_AACCEPT, "AACCEPT"},
	{CPC_PT_AAUTHDATA, "AAUTHDATA"},
	{CPC_PT_AAUTHLEVEL, "AAUTHLEVEL"},
	{CPC_PT_AAUTHNAME, "AAUTHNAME"},
	{CPC_PT_AAUTHSECRET, "AAUTHSECRET"},
	{CPC_PT_AAUTHTYPE, "AAUTHTYPE"},
	{CPC_PT_ADDR, "ADDR"},
	{CPC_PT_ADDRTYPE, "ADDRTYPE"},
	{CPC_PT_APPID, "APPID"},
	{CPC_PT_APROTOCOL, "APROTOCOL"},
	{CPC_PT_AUTH_ENTITY, "AUTH-ENTITY"},
	{CPC_PT_AUTHNAME, "AUTHNAME"},
	{CPC_PT_AUTHSECRET, "AUTHSECRET"},
	{CPC_PT_AUTHTYPE, "AUTHTYPE"},
	{CPC_PT_AUTO_DOWNLOAD, "AUTO-DOWNLOAD"},
	{CPC_PT_BASAUTH_ID, "BASAUTH-ID"},
	{CPC_PT_BASAUTH_PW, "BASAUTH-PW"},
	{CPC_PT_BEARER, "BEARER"},
	{CPC_PT_CALLTYPE, "CALLTYPE"},
	{CPC_PT_CIDPREFIX, "CIDPREFIX"},
	{CPC_PT_CLIENT_ID, "CLIENT-ID"},
	{CPC_PT_CLIURI, "CLIURI"},
	{CPC_PT_CM, "CM"},
	{CPC_PT_CONTEXT_ALLOW, "CONTEXT-ALLOW"},
	{CPC_PT_COUNTRY, "COUNTRY"},
	{CPC_PT_DELIVERY_ERR_SDU, "DELIVERY-ERR-SDU"},
	{CPC_PT_DELIVERY_ORDER, "DELIVERY-ORDER"},
	{CPC_PT_DELIVERY_TIME, "DELIVERY-TIME"},
	{CPC_PT_DNLINKSPEED, "DNLINKSPEED"},
	{CPC_PT_DNS_ADDR, "DNS-ADDR"},
	{CPC_PT_DOMAIN, "DOMAIN"},
	{CPC_PT_EXPIRY_TIME, "EXPIRY-TIME"},
	{CPC_PT_FIRST_RETRY_TIMEOUT, "FIRST-RETRY-TIMEOUT"},
	{CPC_PT_FROM, "FROM"},
	{CPC_PT_GUARANTEED_BITRATE_DNLINK, "GUARANTEED-BITRATE-DNLINK"},
	{CPC_PT_GUARANTEED_BITRATE_UPLINK, "GUARANTEED-BITRATE-UPLINK"},
	{CPC_PT_IMAGE, "IMAGE"},
	{CPC_PT_INIT, "INIT"},
	{CPC_PT_INTERNET, "INTERNET"},
	{CPC_PT_LINGER, "LINGER"},
	{CPC_PT_LINKSPEED, "LINKSPEED"},
	{CPC_PT_LOCAL_ADDR, "LOCAL-ADDR"},
	{CPC_PT_LOCAL_ADDRTYPE, "LOCAL-ADDRTYPE"},
	{CPC_PT_MASTER, "MASTER"},
	{CPC_PT_MAX_BITRATE_DNLINK, "MAX-BITRATE-DNLINK"},
	{CPC_PT_MAX_BITRATE_UPLINK, "MAX-BITRATE-UPLINK"},
	{CPC_PT_MAX_NUM_RETRY, "MAX-NUM-RETRY"},
	{CPC_PT_MAX_SDU_SIZE, "MAX-SDU-SIZE"},
	{CPC_PT_MS, "MS"},
	{CPC_PT_Ma, "Ma"},
	{CPC_PT_NAME, "NAME"},
	{CPC_PT_NAP_ADDRESS, "NAP-ADDRESS"},
	{CPC_PT_NAP_ADDRTYPE, "NAP-ADDRTYPE"},
	{CPC_PT_NAPID, "NAPID"},
	{CPC_PT_NETWORK, "NETWORK"},
	{CPC_PT_PC_ADDR, "PC-ADDR"},
	{CPC_PT_PHYSICAL_PROXY_ID, "PHYSICAL-PROXY-ID"},
	{CPC_PT_POP_BEFORE_SMTP, "POP-BEFORE-SMTP"},
	{CPC_PT_PORTNBR, "PORTNBR"},
	{CPC_PT_PPGAUTH_TYPE, "PPGAUTH-TYPE"},
	{CPC_PT_PRIORITY, "PRIORITY"},
	{CPC_PT_PROVIDER_ID, "PROVIDER-ID"},
	{CPC_PT_PROVURL, "PROVURL"},
	{CPC_PT_PROXY_ID, "PROXY-ID"},
	{CPC_PT_PROXY_PROVIDER_ID, "PROXY-PROVIDER-ID"},
	{CPC_PT_PROXY_PW, "PROXY-PW"},
	{CPC_PT_PULLENABLED, "PULLENABLED"},
	{CPC_PT_PUSHENABLED, "PUSHENABLED"},
	{CPC_PT_PXADDR, "PXADDR"},
	{CPC_PT_PXADDR_FQDN, "PXADDR-FQDN"},
	{CPC_PT_PXADDRTYPE, "PXADDRTYPE"},
	{CPC_PT_PXAUTH_ID, "PXAUTH-ID"},
	{CPC_PT_PXAUTH_PW, "PXAUTH-PW"},
	{CPC_PT_PXAUTH_TYPE, "PXAUTH-TYPE"},
	{CPC_PT_REQUEST_DELIVERY, "REQUEST-DELIVERY"},
	{CPC_PT_REQUEST_READ, "REQUEST-READ"},
	{CPC_PT_REREG_THRESHOLD, "REREG-THRESHOLD"},
	{CPC_PT_RESIDUAL_BER, "RESIDUAL-BER"},
	{CPC_PT_RM, "RM"},
	{CPC_PT_ROAMING, "ROAMING"},
	{CPC_PT_RULE, "RULE"},
	{CPC_PT_SAVE_SENT, "SAVE-SENT"},
	{CPC_PT_SDU_ERROR_RATIO, "SDU-ERROR-RATIO"},
	{CPC_PT_SENDER_VISIBLE, "SENDER-VISIBLE"},
	{CPC_PT_SENT_FOLDER, "SENT-FOLDER"},
	{CPC_PT_SERVICE, "SERVICE"},
	{CPC_PT_SERVICES, "SERVICES"},
	{CPC_PT_SID, "SID"},
	{CPC_PT_SOC, "SOC"},
	{CPC_PT_SPI, "SPI"},
	{CPC_PT_STARTPAGE, "STARTPAGE"},
	{CPC_PT_SYNCTYPE, "SYNCTYPE"},
	{CPC_PT_T_BIT, "T-BIT"},
	{CPC_PT_TO_NAPID, "TO-NAPID"},
	{CPC_PT_TO_PROXY, "TO-PROXY"},
	{CPC_PT_TRAFFIC_CLASS, "TRAFFIC-CLASS"},
	{CPC_PT_TRAFFIC_HANDL_PRIO, "TRAFFIC-HANDL-PRIO"},
	{CPC_PT_TRANSFER_DELAY, "TRANSFER-DELAY"},
	{CPC_PT_TRASH_FOLDER, "TRASH-FOLDER"},
	{CPC_PT_TRUST, "TRUST"},
	{CPC_PT_URI, "URI"},
	{CPC_PT_VALIDUNTIL, "VALIDUNTIL"},
	{CPC_PT_WSP_VERSION, "WSP-VERSION"}
};

typedef struct cpc_valid_param_t_ cpc_valid_param_t;
struct cpc_valid_param_t_ {
	cpc_param_occurrence_t occurence;
	cpc_param_type_t type;
	cpc_param_data_type_t data_type;
};

/* Must be sorted by the second parameter for bsearch */

static const cpc_valid_param_t g_pxlogical_params[] = {
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_BASAUTH_ID,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_BASAUTH_PW,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_PT_DOMAIN,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_MASTER,
	 CPC_WPDT_NONE},
	{CPC_WP_OCCUR_ONCE, CPC_PT_NAME, CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_PPGAUTH_TYPE,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ONCE, CPC_PT_PROXY_ID, CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_PROXY_PROVIDER_ID,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_PROXY_PW,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_PULLENABLED,
	 CPC_WPDT_UINT},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_PUSHENABLED,
	 CPC_WPDT_UINT},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_STARTPAGE,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_TRUST,
	 CPC_WPDT_NONE},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_WSP_VERSION,
	 CPC_WPDT_UTF8},
};

/* Must be sorted by the second parameter for bsearch */

static const cpc_valid_param_t g_pxauth_info_params[] = {
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_PXAUTH_ID,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_PXAUTH_PW,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ONCE, CPC_PT_PXAUTH_TYPE, CPC_WPDT_UTF8}
};

/* Must be sorted by the second parameter for bsearch */

static const cpc_valid_param_t g_port_params[] = {
	{CPC_WP_OCCUR_ONCE, CPC_PT_PORTNBR, CPC_WPDT_UINT},
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_PT_SERVICE,
	 CPC_WPDT_UTF8}
};

/* Must be sorted by the second parameter for bsearch */

static const cpc_valid_param_t g_pxphysical_params[] = {
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_PT_DOMAIN,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ONCE, CPC_PT_PHYSICAL_PROXY_ID,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_PULLENABLED,
	 CPC_WPDT_UINT},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_PUSHENABLED,
	 CPC_WPDT_UINT},
	{CPC_WP_OCCUR_ONCE, CPC_PT_PXADDR, CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_PXADDRTYPE,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_PXADDR_FQDN,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ONE_OR_MORE, CPC_PT_TO_NAPID,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_WSP_VERSION,
	 CPC_WPDT_UTF8},
};

/* Must be sorted by the second parameter for bsearch */

static const cpc_valid_param_t g_napdef_params[] = {
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_PT_BEARER,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_CALLTYPE,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_DELIVERY_ERR_SDU,
	 CPC_WPDT_UINTHEX},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_DELIVERY_ORDER,
	 CPC_WPDT_UINTHEX},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_DNLINKSPEED,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_PT_DNS_ADDR,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_FIRST_RETRY_TIMEOUT,
	 CPC_WPDT_UINTHEX},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_GUARANTEED_BITRATE_DNLINK,
	 CPC_WPDT_UINTHEX},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_GUARANTEED_BITRATE_UPLINK,
	 CPC_WPDT_UINTHEX},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_INTERNET,
	 CPC_WPDT_NONE},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_LINGER,
	 CPC_WPDT_UINT},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_LINKSPEED,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_LOCAL_ADDR,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_LOCAL_ADDRTYPE,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_MAX_BITRATE_DNLINK,
	 CPC_WPDT_UINTHEX},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_MAX_BITRATE_UPLINK,
	 CPC_WPDT_UINTHEX},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_MAX_NUM_RETRY,
	 CPC_WPDT_UINTHEX},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_MAX_SDU_SIZE,
	 CPC_WPDT_UINTHEX},
	{CPC_WP_OCCUR_ONCE, CPC_PT_NAME, CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ONCE, CPC_PT_NAPID, CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ONCE, CPC_PT_NAP_ADDRESS,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_NAP_ADDRTYPE,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_REREG_THRESHOLD,
	 CPC_WPDT_UINTHEX},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_RESIDUAL_BER,
	 CPC_WPDT_UINTHEX},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_SDU_ERROR_RATIO,
	 CPC_WPDT_UINTHEX},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_TRAFFIC_CLASS,
	 CPC_WPDT_UINTHEX},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_TRAFFIC_HANDL_PRIO,
	 CPC_WPDT_UINTHEX},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_TRANSFER_DELAY,
	 CPC_WPDT_UINTHEX},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_T_BIT, CPC_WPDT_NONE}
};

/* Must be sorted by the second parameter for bsearch */

static const cpc_valid_param_t g_napdef_info_params[] = {
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_AUTHNAME,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_AUTHSECRET,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ONCE, CPC_PT_AUTHTYPE, CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_PT_AUTH_ENTITY,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_SPI, CPC_WPDT_UTF8}
};

/* Must be sorted by the second parameter for bsearch */

static const cpc_valid_param_t g_validity_params[] = {
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_COUNTRY,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_NETWORK,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_SID, CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_SOC, CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_VALIDUNTIL,
	 CPC_WPDT_UINT}
};

/* Must be sorted by the second parameter for bsearch */

static const cpc_valid_param_t g_bootstrap_params[] = {
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_CONTEXT_ALLOW,
	 CPC_WPDT_UINT},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_COUNTRY,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_NAME,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_PT_NETWORK,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_PROVURL,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_PT_PROXY_ID,
	 CPC_WPDT_UTF8}
};

/* Must be sorted by the second parameter for bsearch */

static const cpc_valid_param_t g_client_identity_params[] = {
	{CPC_WP_OCCUR_ONCE, CPC_PT_CLIENT_ID, CPC_WPDT_UTF8}
};

/* Must be sorted by the second parameter for bsearch */

static const cpc_valid_param_t g_vendor_config_params[] = {
	{CPC_WP_OCCUR_ONCE, CPC_PT_NAME, CPC_WPDT_UTF8}
};

/* Must be sorted by the second parameter for bsearch */

static const cpc_valid_param_t g_application_params[] = {
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_AACCEPT,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_PT_ADDR,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ONCE, CPC_PT_APPID, CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_APROTOCOL,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_AUTO_DOWNLOAD,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_CIDPREFIX,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_CM, CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_DELIVERY_TIME,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_PT_DOMAIN,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_EXPIRY_TIME,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_FROM,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_INIT,
	 CPC_WPDT_NONE},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_MS, CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_Ma, CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_NAME,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_PC_ADDR,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_POP_BEFORE_SMTP,
	 CPC_WPDT_UINT},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_PRIORITY,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_PROVIDER_ID,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_REQUEST_DELIVERY,
	 CPC_WPDT_UINT},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_REQUEST_READ,
	 CPC_WPDT_UINT},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_RM, CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_ROAMING,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_SAVE_SENT,
	 CPC_WPDT_UINT},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_SENDER_VISIBLE,
	 CPC_WPDT_UINT},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_SENT_FOLDER,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_SERVICES,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_PT_TO_NAPID,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_PT_TO_PROXY,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_TRASH_FOLDER,
	 CPC_WPDT_UTF8}
};

/* Must be sorted by the second parameter for bsearch */

static const cpc_valid_param_t g_app_addr_params[] = {
	{CPC_WP_OCCUR_ONCE, CPC_PT_ADDR, CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_ADDRTYPE,
	 CPC_WPDT_UTF8}
};

/* Must be sorted by the second parameter for bsearch */

static const cpc_valid_param_t g_app_auth_params[] = {
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_AAUTHDATA,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_AAUTHLEVEL,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_AAUTHNAME,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_AAUTHSECRET,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_AAUTHTYPE,
	 CPC_WPDT_UTF8}
};

/* Must be sorted by the second parameter for bsearch */

static const cpc_valid_param_t g_resource_params[] = {
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_AACCEPT,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_AAUTHDATA,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_AAUTHNAME,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_AAUTHSECRET,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_AAUTHTYPE,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_CLIURI,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_IMAGE,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_NAME,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_STARTPAGE,
	 CPC_WPDT_NONE},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_PT_SYNCTYPE,
	 CPC_WPDT_UINT},
	{CPC_WP_OCCUR_ONCE, CPC_PT_URI, CPC_WPDT_UTF8}
};

/* Must be sorted by the second parameter for bsearch */

static const cpc_valid_param_t g_access_params[] = {
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_PT_APPID,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_PT_DOMAIN,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_PT_PORTNBR,
	 CPC_WPDT_UINT},
	{CPC_WP_OCCUR_ONE_OR_MORE, CPC_PT_RULE,
	 CPC_WPDT_UTF8OPT},
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_PT_TO_NAPID,
	 CPC_WPDT_UTF8},
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_PT_TO_PROXY,
	 CPC_WPDT_UTF8}
};

typedef struct cpc_char_param_map_t_ cpc_char_param_map_t;
struct cpc_char_param_map_t_ {
	cpc_characteristic_type_t characteristic;
	unsigned int number_of_params;
	cpc_valid_param_t *params;
};

static cpc_char_param_map_t g_char_allowed_params[] = {
	{CPC_CT_ACCESS, sizeof(g_access_params) / sizeof(cpc_valid_param_t),
	 (cpc_valid_param_t *) g_access_params},
	{CPC_CT_APPADDR, sizeof(g_app_addr_params) / sizeof(cpc_valid_param_t),
	 (cpc_valid_param_t *) g_app_addr_params},
	{CPC_CT_APPAUTH, sizeof(g_app_auth_params) / sizeof(cpc_valid_param_t),
	 (cpc_valid_param_t *) g_app_auth_params},
	{CPC_CT_APPLICATION,
	 sizeof(g_application_params) / sizeof(cpc_valid_param_t),
	 (cpc_valid_param_t *) g_application_params},
	{CPC_CT_BOOTSTRAP,
	 sizeof(g_bootstrap_params) / sizeof(cpc_valid_param_t),
	 (cpc_valid_param_t *) g_bootstrap_params},
	{CPC_CT_CLIENTIDENTITY,
	 sizeof(g_client_identity_params) / sizeof(cpc_valid_param_t),
	 (cpc_valid_param_t *) g_client_identity_params},
	{CPC_CT_NAPAUTHINFO,
	 sizeof(g_napdef_info_params) / sizeof(cpc_valid_param_t),
	 (cpc_valid_param_t *) g_napdef_info_params},
	{CPC_CT_NAPDEF, sizeof(g_napdef_params) / sizeof(cpc_valid_param_t),
	 (cpc_valid_param_t *) g_napdef_params},
	{CPC_CT_PORT, sizeof(g_port_params) / sizeof(cpc_valid_param_t),
	 (cpc_valid_param_t *) g_port_params},
	{CPC_CT_PXAUTHINFO,
	 sizeof(g_pxauth_info_params) / sizeof(cpc_valid_param_t),
	 (cpc_valid_param_t *) g_pxauth_info_params},
	{CPC_CT_PXLOGICAL,
	 sizeof(g_pxlogical_params) / sizeof(cpc_valid_param_t),
	 (cpc_valid_param_t *) g_pxlogical_params},
	{CPC_CT_PXPHYSICAL,
	 sizeof(g_pxphysical_params) / sizeof(cpc_valid_param_t),
	 (cpc_valid_param_t *) g_pxphysical_params},
	{CPC_CT_RESOURCE, sizeof(g_resource_params) / sizeof(cpc_valid_param_t),
	 (cpc_valid_param_t *) g_resource_params},
	{CPC_CT_ROOT, 0, NULL},
	{CPC_CT_VALIDITY, sizeof(g_validity_params) / sizeof(cpc_valid_param_t),
	 (cpc_valid_param_t *) g_validity_params},
	{CPC_CT_VENDORCONFIG,
	 sizeof(g_vendor_config_params) / sizeof(cpc_valid_param_t),
	 (cpc_valid_param_t *) g_vendor_config_params}
};

typedef struct cpc_valid_characteristic_t_ cpc_valid_characteristic_t;
struct cpc_valid_characteristic_t_ {
	cpc_param_occurrence_t occurence;
	cpc_characteristic_type_t type;
};

/*
 * We're not going to bsearch the characteristics as the arrays are to small.
 * Thus ordering is not important here. We're going to keep them sorted however
 * for consistency.
 */

static const cpc_valid_characteristic_t g_root_characteristic[] = {
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_CT_ACCESS},
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_CT_APPLICATION},
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_CT_BOOTSTRAP},
	{CPC_WP_OCCUR_ZERO_OR_ONE, CPC_CT_CLIENTIDENTITY},
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_CT_NAPDEF},
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_CT_PXLOGICAL},
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_CT_VENDORCONFIG}
};

static const cpc_valid_characteristic_t g_pxlogical_characteristic[] = {
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_CT_PORT},
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_CT_PXAUTHINFO},
	{CPC_WP_OCCUR_ONE_OR_MORE, CPC_CT_PXPHYSICAL}
};

static const cpc_valid_characteristic_t g_pxphysical_characteristic[] = {
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_CT_PORT}
};

static const cpc_valid_characteristic_t g_napdef_characteristic[] = {
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_CT_NAPAUTHINFO},
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_CT_VALIDITY}

};

static const cpc_valid_characteristic_t g_application_characteristic[] = {
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_CT_APPADDR},
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_CT_APPAUTH},
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_CT_RESOURCE}
};

static const cpc_valid_characteristic_t g_app_addr_characteristic[] = {
	{CPC_WP_OCCUR_ZERO_OR_MORE, CPC_CT_PORT}
};

typedef struct cpc_char_char_map_t_ cpc_char_char_map_t;
struct cpc_char_char_map_t_ {
	cpc_characteristic_type_t characteristic;
	unsigned int number_of_chars;
	cpc_valid_characteristic_t *children;
};

static cpc_char_char_map_t g_char_allowed_chars[] = {
	{CPC_CT_ACCESS, 0, 0},
	{CPC_CT_APPADDR, sizeof(g_app_addr_characteristic) /
	 sizeof(cpc_valid_characteristic_t),
	 (cpc_valid_characteristic_t *) g_app_addr_characteristic},
	{CPC_CT_APPAUTH, 0, 0},
	{CPC_CT_APPLICATION, sizeof(g_application_characteristic) /
	 sizeof(cpc_valid_characteristic_t),
	 (cpc_valid_characteristic_t *) g_application_characteristic},
	{CPC_CT_BOOTSTRAP, 0, 0},
	{CPC_CT_CLIENTIDENTITY, 0, 0},
	{CPC_CT_NAPAUTHINFO, 0, 0},
	{CPC_CT_NAPDEF,
	 sizeof(g_napdef_characteristic) / sizeof(cpc_valid_characteristic_t),
	 (cpc_valid_characteristic_t *) g_napdef_characteristic},
	{CPC_CT_PORT, 0, 0},
	{CPC_CT_PXAUTHINFO, 0, 0},
	{CPC_CT_PXLOGICAL,
	 sizeof(g_pxlogical_characteristic) /
	 sizeof(cpc_valid_characteristic_t),
	 (cpc_valid_characteristic_t *) g_pxlogical_characteristic},
	{CPC_CT_PXPHYSICAL,
	 sizeof(g_pxphysical_characteristic) /
	 sizeof(cpc_valid_characteristic_t),
	 (cpc_valid_characteristic_t *) g_pxphysical_characteristic},
	{CPC_CT_RESOURCE, 0, 0},
	{CPC_CT_ROOT,
	 sizeof(g_root_characteristic) / sizeof(cpc_valid_characteristic_t),
	 (cpc_valid_characteristic_t *) g_root_characteristic},
	{CPC_CT_VALIDITY, 0, 0},
	{CPC_CT_VENDORCONFIG, 0, 0}
};

static const int g_cpc_document_element = 10;
static const int g_cpc_normal_element = 1;
static const int g_cpc_end_element = 15;
static const unsigned int g_cpc_current_major_version = 1;

static const xmlChar *g_supported_applications[] = {
	(const xmlChar *)"25",	        /* SMTP */
	(const xmlChar *)"110",         /* POP3 */
	(const xmlChar *)"143",         /* IMAP4 */
	(const xmlChar *)"w2",	        /* Browser */
	(const xmlChar *)"w4",	        /* MMS 1.2 */
	(const xmlChar *)"w5",	        /* OMA DS */
	(const xmlChar *)"w7",	        /* OMA DM */
	(const xmlChar *)"wA",	        /* IMPS */
	(const xmlChar *)"DL",	        /* OMA DL */
	(const xmlChar *)"ap0004",	/* SUPL */
};

typedef int(*cpc_char_merge_fn_t)(cpc_characteristic_t *,
				  cpc_characteristic_t *);

static int prv_find_chars(const void *argv1, const void *argv2)
{
	cpc_char_string_map_t *arg1 = (cpc_char_string_map_t *) argv1;
	cpc_char_string_map_t *arg2 = (cpc_char_string_map_t *) argv2;

	return strcmp(arg1->string, arg2->string);
}

static int prv_find_params(const void *argv1, const void *argv2)
{
	cpc_param_string_map_t *arg1 = (cpc_param_string_map_t *) argv1;
	cpc_param_string_map_t *arg2 = (cpc_param_string_map_t *) argv2;

	return strcmp(arg1->string, arg2->string);
}

static int prv_find_valid_params(const void *argv1, const void *argv2)
{
	cpc_valid_param_t *arg1 = (cpc_valid_param_t *) argv1;
	cpc_valid_param_t *arg2 = (cpc_valid_param_t *) argv2;

	return arg1->type - arg2->type;
}

static void prv_parameter_free(void *param)
{
	cpc_parameter_t *obj = (cpc_parameter_t *) param;

	if (obj) {
		if (obj->data_type == CPC_WPDT_UTF8 && obj->utf8_value)
			xmlFree(obj->utf8_value);

		free(obj);
	}
}

static int prv_parameter_dup(cpc_parameter_t *param,
			     cpc_parameter_t **param_dup)
{
	CPC_ERR_MANAGE;
	cpc_parameter_t *retval;

	CPC_FAIL_NULL(retval, malloc(sizeof(*param)), CPC_ERR_OOM);

	if (param->data_type == CPC_WPDT_UTF8 ||
	    param->data_type == CPC_WPDT_UTF8OPT) {
		if (param->utf8_value)
			CPC_FAIL_NULL(retval->utf8_value,
					   xmlStrdup(param->utf8_value),
					   CPC_ERR_OOM);
		else
			retval->utf8_value = NULL;
	} else
		retval->int_value = param->int_value;

	retval->type = param->type;
	retval->data_type = param->data_type;
	retval->transient = param->transient;
	*param_dup = retval;

	return CPC_ERR_NONE;

CPC_ON_ERR:

	free(retval);

	return CPC_ERR;
}

static void prv_wap_char_free(void *characteristic)
{
	cpc_characteristic_t *obj = (cpc_characteristic_t *) characteristic;

	if (obj) {
		cpc_ptr_array_free(&obj->parameters);
		cpc_ptr_array_free(&obj->characteristics);
		free(obj);
	}
}

static void prv_characteristic_make(cpc_characteristic_t *characteristic,
				    cpc_characteristic_type_t type)
{
	characteristic->type = type;
	cpc_ptr_array_make(&characteristic->parameters, 8,
				    prv_parameter_free);
	cpc_ptr_array_make(&characteristic->characteristics, 4,
				    prv_wap_char_free);
}

static int prv_characteristic_dup(cpc_characteristic_t *characteristic,
				  cpc_characteristic_t **char_dup)
{
	CPC_ERR_MANAGE;
	unsigned int i = 0;
	cpc_parameter_t *param = NULL;
	cpc_characteristic_t *new_char = NULL;
	cpc_characteristic_t *retval;

	CPC_FAIL_NULL(retval, malloc(sizeof(*characteristic)),
			   CPC_ERR_OOM);

	prv_characteristic_make(retval, characteristic->type);

	for (i = 0; i < cpc_get_param_count(characteristic); ++i) {
		CPC_FAIL(prv_parameter_dup(
				      cpc_get_param(characteristic, i),
				      &param));

		CPC_FAIL(cpc_ptr_array_append(&retval->parameters,
							    param));
		param = NULL;
	}

	for (i = 0; i < cpc_get_char_count(characteristic); ++i) {
		CPC_FAIL(prv_characteristic_dup(cpc_get_char
							   (characteristic, i),
							   &new_char));
		CPC_FAIL(cpc_ptr_array_append(&retval->characteristics,
							    new_char));
		new_char = NULL;
	}

	*char_dup = retval;

	return CPC_ERR_NONE;

CPC_ON_ERR:

	prv_parameter_free(param);
	prv_wap_char_free(new_char);
	prv_wap_char_free(retval);

	return CPC_ERR;
}

static bool prv_find_param_occurrence(cpc_characteristic_type_t characteristic,
				      cpc_param_type_t param,
				      cpc_param_occurrence_t *occurrence)
{
	bool retval = false;
	cpc_char_param_map_t *char_param_map = NULL;
	cpc_valid_param_t valid_param_key;
	cpc_valid_param_t *valid_param_found = NULL;

	if (characteristic < CPC_CT_MAX) {
		char_param_map = &g_char_allowed_params[characteristic];

		valid_param_key.type = param;
		valid_param_found = bsearch(&valid_param_key,
					    char_param_map->params,
					    char_param_map->number_of_params,
					    sizeof(cpc_valid_param_t),
					    prv_find_valid_params);

		if (valid_param_found) {
			retval = true;
			*occurrence = valid_param_found->occurence;
		}
	}

	return retval;
}

static int prv_check_root_node(xmlTextReaderPtr reader_ptr)
{
	CPC_ERR_MANAGE;
	xmlChar *version_att = NULL;
	const xmlChar *name;
	char *end = NULL;

	CPC_FAIL_NULL(name, xmlTextReaderConstName(reader_ptr),
			   CPC_ERR_CORRUPT);

	if (xmlStrEqual((const xmlChar *)"wap-provisioningdoc", name)) {
		version_att = xmlTextReaderGetAttribute(reader_ptr,
							(const xmlChar *)
							"version");
		if (version_att) {
			if ((unsigned int) strtod((char *)version_att, &end) !=
			    g_cpc_current_major_version) {
				CPC_LOGF("Invalid version number %s",
					      version_att);
				CPC_FAIL_FORCE(CPC_ERR_CORRUPT);
			}
		}
		else if (xmlLastError.code == XML_ERR_NO_MEMORY)
			CPC_FAIL_FORCE(CPC_ERR_OOM);
	}

CPC_ON_ERR:

	if (version_att)
		xmlFree(version_att);

	return CPC_ERR;
}

static int prv_process_characteristic(xmlTextReaderPtr reader_ptr,
				      cpc_characteristic_t *characteristic)
{
	/*
	 * 1. Read the characteristic type and see if we recoginize the string
	 * 2. Check to see whether it can be part of characteristic
	 * 3. Check its occurance.
	 *
	 * If any of these tests fail we will ignore the entire characteristic.
	 */

	CPC_ERR_MANAGE;
	cpc_char_string_map_t char_key;
	cpc_char_string_map_t *char_found = NULL;
	unsigned int i = 0;
	cpc_char_char_map_t *char_char_map = NULL;
	cpc_param_occurrence_t occurence;
	cpc_characteristic_t *new_char = NULL;
	xmlChar *type;

	type = xmlTextReaderGetAttribute(reader_ptr, (const xmlChar *)"type");
	if (!type) {
		CPC_LOGF("Unable to read characteristic type");
		CPC_FAIL_FORCE(xmlLastError.code == XML_ERR_NO_MEMORY ?
				CPC_ERR_OOM : CPC_ERR_NONE);
	}

	char_key.string = (const char *)type;
	char_found = (cpc_char_string_map_t *)
		bsearch(&char_key, g_char_string_map, sizeof(g_char_string_map)/
			sizeof(cpc_char_string_map_t),
			sizeof(cpc_char_string_map_t), prv_find_chars);

	if (!char_found) {
		CPC_LOGF("Unrecognized charactersitic %s", type);
		goto CPC_ON_ERR;
	}

	char_char_map = &g_char_allowed_chars[characteristic->type];
	for (; i < char_char_map->number_of_chars &&
		     char_char_map->children[i].type != char_found->type; ++i);

	if (i >= char_char_map->number_of_chars) {
		CPC_LOGF("Invalid charactersitic %s", type);
		goto CPC_ON_ERR;
	}

	occurence = char_char_map->children[i].occurence;
	if (occurence == CPC_WP_OCCUR_ONCE ||
	    occurence == CPC_WP_OCCUR_ZERO_OR_ONE) {

		for (i = 0; i < cpc_get_char_count(characteristic) &&
			     cpc_get_char(characteristic, i)->type !=
			     char_found->type; ++i) ;

		if (i < cpc_get_char_count(characteristic)) {
			CPC_LOGF("Invalid occurence charactersitic %s", type);
			goto CPC_ON_ERR;
		}
	}

	CPC_FAIL_NULL(new_char, malloc(sizeof(*new_char)), CPC_ERR_OOM);

	prv_characteristic_make(new_char, char_found->type);

	CPC_FAIL(cpc_ptr_array_append(&characteristic->characteristics,
				      new_char));

	new_char = NULL;

CPC_ON_ERR:

	free(new_char);

	if (type)
		xmlFree(type);

	return CPC_ERR;
}

static int prv_add_param(xmlTextReaderPtr reader_ptr,
			 cpc_characteristic_t *characteristic,
			 cpc_valid_param_t *valid_param, xmlChar **value)
{
	CPC_ERR_MANAGE;
	cpc_param_data_type_t dt = valid_param->data_type;
	cpc_parameter_t *param;
	char *ptr = NULL;

	CPC_FAIL_NULL(param, malloc(sizeof(*param)), CPC_ERR_OOM);

	param->transient = false;
	if (dt == CPC_WPDT_UINT || dt == CPC_WPDT_UINTHEX) {
		if (!*value) {
			CPC_LOGF("Invalid paramater value %d",
				      valid_param->type);
			goto CPC_ON_ERR;
		}

		param->int_value = strtoul((char *)*value, &ptr,
					   dt == CPC_WPDT_UINT ? 10 : 16);
		if ((param->int_value == 0 && ptr == (char *) *value)
		    || param->int_value > UINT_MAX) {
			CPC_LOGF("Invalid paramater value %d",
				      valid_param->type);
			goto CPC_ON_ERR;
		}

		param->data_type = CPC_WPDT_UINT;
	} else if (dt == CPC_WPDT_UTF8 || dt == CPC_WPDT_UTF8OPT) {
		if (!*value) {
			if (dt == CPC_WPDT_UTF8OPT)
				param->data_type = CPC_WPDT_NONE;
			else {
				CPC_LOGF("Invalid paramater value %d",
					      valid_param->type);
				goto CPC_ON_ERR;
			}
		} else {
			param->data_type = CPC_WPDT_UTF8;
			param->utf8_value = *value;

			/* So it doesen't get deleted by caller */

			*value = NULL;
		}
	} else
		param->data_type = CPC_WPDT_NONE;

	param->type = valid_param->type;

	CPC_FAIL(cpc_ptr_array_append(&characteristic->parameters, param));

	param = NULL;

CPC_ON_ERR:

	prv_parameter_free(param);

	return CPC_ERR;
}

static int prv_process_param(xmlTextReaderPtr reader_ptr,
			     cpc_characteristic_t *characteristic)
{
	CPC_ERR_MANAGE;
	xmlChar *value = NULL;
	cpc_param_string_map_t param_key;
	cpc_param_string_map_t *param_found = NULL;
	cpc_valid_param_t valid_param_key;
	cpc_valid_param_t *valid_param_found = NULL;
	cpc_char_param_map_t *char_param_map = NULL;
	unsigned int i = 0;
	unsigned int param_count;
	cpc_param_occurrence_t occurence;
	xmlChar *name =
		xmlTextReaderGetAttribute(reader_ptr, (const xmlChar *) "name");

	if (!name) {
		CPC_LOGF("Unable to read parameter name");
		CPC_FAIL_FORCE((xmlLastError.code == XML_ERR_NO_MEMORY) ?
					CPC_ERR_OOM : CPC_ERR_NONE);
	}

	value = xmlTextReaderGetAttribute(reader_ptr, (const xmlChar *)"value");
	if (!value && xmlLastError.code == XML_ERR_NO_MEMORY) {
		CPC_LOGF("Unable to read parameter value");
		CPC_FAIL_FORCE(CPC_ERR_OOM);
	}

	param_key.string = (const char *) name;
	param_found = bsearch(&param_key, g_param_string_map,
			      sizeof(g_param_string_map) /
			      sizeof(cpc_param_string_map_t),
			      sizeof(cpc_param_string_map_t), prv_find_params);

	if (!param_found) {
		CPC_LOGF("Can't find parameter %s\n", name);
		goto CPC_ON_ERR;
	}

	valid_param_key.type = param_found->type;

	char_param_map = &g_char_allowed_params[characteristic->type];

	valid_param_found = bsearch(&valid_param_key, char_param_map->params,
				    char_param_map->number_of_params,
				    sizeof(cpc_valid_param_t),
				    prv_find_valid_params);

	if (!valid_param_found) {
		CPC_LOGF("parameter %s not valid param %d\n", name,
			      valid_param_key.type);
		goto CPC_ON_ERR;
	}

	occurence = valid_param_found->occurence;
	if (occurence == CPC_WP_OCCUR_ONCE ||
	    occurence == CPC_WP_OCCUR_ZERO_OR_ONE) {
		param_count = cpc_get_param_count(characteristic);
		for (i = 0; i < param_count &&
			     cpc_get_param(characteristic, i)->type !=
			     valid_param_found->type; ++i);

		if (i != param_count) {
			CPC_LOGF("Ignorning duplicate param %s", name);
			goto CPC_ON_ERR;
		}
	}

	CPC_FAIL(prv_add_param(reader_ptr, characteristic, valid_param_found,
				    &value));

CPC_ON_ERR:

	if (value)
		xmlFree(value);

	if (name)
		xmlFree(name);

	return CPC_ERR;
}

static int prv_process_normal_element(xmlTextReaderPtr reader_ptr,
				      cpc_ptr_array_t *characteristic_stack,
				      int depth, int *depth_of_current_char)
{
	CPC_ERR_MANAGE;
	const xmlChar *name = NULL;
	cpc_characteristic_t *current_char = NULL;
	unsigned int stack_size;
	int old_last_char_index = -1;
	int last_char_index = -1;

	CPC_FAIL_NULL(name, xmlTextReaderConstName(reader_ptr), CPC_ERR_OOM);

	stack_size = cpc_ptr_array_get_size(characteristic_stack);

	current_char = cpc_ptr_array_get(characteristic_stack, stack_size - 1);

	if (xmlStrcmp((const xmlChar *)"characteristic", name) == 0) {
		if (current_char) {
			old_last_char_index = cpc_get_char_count(current_char)
				- 1;
			CPC_FAIL(prv_process_characteristic(reader_ptr,
								 current_char));
			last_char_index = cpc_get_char_count(current_char)
				- 1;
		}

		/*
		 * Just because we have no error does not mean that we have
		 * actually added a characteristic. The characteristic may
		 * have been invalid. In this case we will append NULL to the
		 * stack so that we know to ignore all parameters and
		 * characteristics until we have reached the end of this
		 * characteristic.
		 */

		if (last_char_index > old_last_char_index) {
			current_char =  cpc_ptr_array_get
				(&current_char->characteristics,
				 last_char_index);
			*depth_of_current_char = depth;
		} else {
			current_char = NULL;
		}

		CPC_FAIL(cpc_ptr_array_append(characteristic_stack,
					      current_char));
	} else if (xmlStrcmp((const xmlChar *)"parm", name) == 0) {

		/*
		 * We need this depth check to ensure that we ignore parameters
		 * within parameters.
		 */

		if (current_char && *depth_of_current_char + 1 == depth)
			CPC_FAIL(prv_process_param(reader_ptr, current_char));
	} else {

		/* We have encountered a node that we don't recognise.
		 * However, we need to be careful. What would happen if this
		 * node contained a valid param or characteristic node? Hence
		 * we need to push NULL to the charstack to ensure that we
		 * ignore all subsequent nodes until we reach the end of this
		 * unrecognised node.
		 */

		if (!xmlTextReaderIsEmptyElement(reader_ptr))
			CPC_FAIL(cpc_ptr_array_append(characteristic_stack,
						      NULL));

		CPC_LOGF("Skipping unknown tag %s", name);
	}

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_process_end_element(xmlTextReaderPtr reader_ptr,
				   cpc_ptr_array_t *characteristic_stack,
				   int depth, int *depth_of_current_char)
{
	CPC_ERR_MANAGE;
	const xmlChar *name = NULL;
	unsigned int stack_size;

	CPC_FAIL_NULL(name, xmlTextReaderConstName(reader_ptr), CPC_ERR_OOM);

	if (depth > 0 && xmlStrcmp((const xmlChar *)"parm", name) != 0) {
		stack_size = cpc_ptr_array_get_size(characteristic_stack);

		cpc_ptr_array_delete(characteristic_stack, stack_size - 1);

		--stack_size;

		if (stack_size == 0)
			CPC_FAIL_FORCE(CPC_ERR_CORRUPT);

		if (cpc_ptr_array_get(characteristic_stack, stack_size - 1))
			--*depth_of_current_char;
	}

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_process_node(xmlTextReaderPtr reader_ptr,
			    cpc_ptr_array_t *characteristic_stack,
			    int *depth_of_current_char)
{
	CPC_ERR_MANAGE;
	int depth = 0;
	int type = 0;

	depth = xmlTextReaderDepth(reader_ptr);

	type = xmlTextReaderNodeType(reader_ptr);

	if (depth == -1 || type == -1) {
		goto CPC_ON_ERR;
	}

	if (depth == 0) {
		if (type == g_cpc_normal_element)
			CPC_FAIL(prv_check_root_node(reader_ptr));
	} else if (type == g_cpc_normal_element) {
		CPC_FAIL(prv_process_normal_element(reader_ptr,
						  characteristic_stack, depth,
						  depth_of_current_char));
	} else if (type == g_cpc_end_element) {
		CPC_FAIL(prv_process_end_element(reader_ptr,
						characteristic_stack, depth,
						depth_of_current_char));
	}

CPC_ON_ERR:

	return CPC_ERR;
}

int cpc_find_param(cpc_characteristic_t *characteristic,
		   cpc_param_type_t param, int index)
{
	int retval = -1;
	unsigned int i = index;

	for (; i < cpc_get_param_count(characteristic) &&
	     cpc_get_param(characteristic, i)->type != param; ++i);

	if (i < cpc_get_param_count(characteristic))
		retval = i;

	return retval;
}

int cpc_find_char(cpc_characteristic_t *characteristic,
		  cpc_characteristic_type_t characteristic_type, int index)
{
	int retval = -1;
	unsigned int i = index;

	for (; i < cpc_get_char_count(characteristic) &&
	     cpc_get_char(characteristic, i)->type != characteristic_type;
	      ++i);

	if (i < cpc_get_char_count(characteristic))
		retval = i;

	return retval;
}

static bool prv_validate_utf8_combo(cpc_characteristic_t *characteristic,
				    cpc_param_type_t if_present,
				    cpc_param_type_t also_required)
{
	bool retval = true;
	int i = 0;
	xmlChar *str = NULL;

	if (cpc_find_param(characteristic, if_present, 0) != -1) {
		i = cpc_find_param(characteristic, also_required, 0);

		if (i == -1)
			retval = false;
		else {
			str = cpc_get_param(characteristic, i)->utf8_value;

			if (xmlStrlen(str) == 0)
				retval = false;
		}
	}

	return retval;
}

static void prv_delete_parameter(cpc_characteristic_t *characteristic,
				 cpc_param_type_t param_type)
{
	int i = cpc_find_param(characteristic, param_type, 0);

	while (i != -1) {
		cpc_ptr_array_delete(&characteristic->parameters, i);

		CPC_LOGF("Deleting param %d", param_type);

		i = cpc_find_param(characteristic, param_type, 0);
	}
}

static int prv_validate_validity_characteristic(cpc_characteristic_t
						*characteristic)
{
	int retval = CPC_ERR_NONE;

	if (!(prv_validate_utf8_combo(characteristic, CPC_PT_NETWORK,
				      CPC_PT_COUNTRY)
	     && prv_validate_utf8_combo(characteristic, CPC_PT_SID,
				      CPC_PT_SOC)))
		retval = CPC_ERR_CORRUPT;

	return retval;
}

static bool prv_compare_params(cpc_parameter_t *param1, cpc_parameter_t *param2)
{
	bool retval = param1->data_type == param2->data_type;

	if (retval) {
		switch (param1->data_type) {
		case CPC_WPDT_NONE:
			retval = true;
			break;
		case CPC_WPDT_UINT:
		case CPC_WPDT_UINTHEX:
			retval = param1->int_value == param2->int_value;
			break;
		case CPC_WPDT_UTF8:
		case CPC_WPDT_UTF8OPT:
			if (param1->utf8_value && param2->utf8_value)
				retval = xmlStrEqual(param1->utf8_value,
						      param2->utf8_value);
			break;
		}
	}

	return retval;
}

static bool prv_params_equal(cpc_characteristic_t *characteristic1,
			     int param_index1,
			     cpc_characteristic_t *characteristic2,
			     int param_index2)
{
	cpc_parameter_t *param1 = NULL;
	cpc_parameter_t *param2 = NULL;

	param1 = cpc_get_param(characteristic1, param_index1);
	param2 = cpc_get_param(characteristic2, param_index2);

	return prv_compare_params(param1, param2);
}

static void prv_del_dup_app(cpc_characteristic_t *characteristic,
			    int from, const char *appid,
			    cpc_param_type_t param_type, int param1,
			    int appidIndex1,
			    cpc_characteristic_t *target_char1)
{
	int param2 = -1;
	int appid_index2 = -1;
	cpc_characteristic_t *target_char2 = NULL;

	int j = cpc_find_char(characteristic, CPC_CT_APPLICATION,
				   from + 1);

	while (j != -1) {
		target_char2 = cpc_get_char(characteristic, j);

		param2 = cpc_find_param(target_char2, param_type, 0);

		appid_index2 = cpc_find_param(target_char2, CPC_PT_APPID,
						  0);
		if (appid_index2 != -1 &&
		    xmlStrEqual((const xmlChar *) appid, cpc_get_param
				(target_char2, appid_index2)->utf8_value)
		    && prv_params_equal(target_char1, appidIndex1,
					target_char2, appid_index2)) {

			/*
			 * We are dealing with two different instances of the
			 * same application
			 */

			if ((param1 == -1 && param2 == -1) ||
			    ((param1 != -1 && param2 != -1) &&
			     prv_params_equal(target_char1,
					      param1, target_char2, param2))) {
				cpc_ptr_array_delete
					(&characteristic->characteristics, j);
				CPC_LOGF("Deleting duplicate APPICATION");
			} else
				++j;
		} else
			++j;

		j = cpc_find_char(characteristic, CPC_CT_APPLICATION, j);
	}
}

static void prv_rm_dup_app(cpc_characteristic_t *characteristic,
			   const char *appid,
			   cpc_param_type_t param_type)
{
	int appid_index1 = -1;
	int param1 = -1;
	cpc_characteristic_t *target_char1 = NULL;
	int i = cpc_find_char(characteristic, CPC_CT_APPLICATION, 0);

	while (i != -1) {
		target_char1 = cpc_get_char(characteristic, i);

		param1 = cpc_find_param(target_char1, param_type, 0);

		appid_index1 = cpc_find_param(target_char1, CPC_PT_APPID,
					      0);

		if (appid_index1 != -1 && xmlStrEqual((const xmlChar *) appid,
				cpc_get_param(target_char1,
					      appid_index1)->utf8_value))
			prv_del_dup_app(characteristic, i, appid, param_type,
					param1, appid_index1, target_char1);
		i = cpc_find_char(characteristic, CPC_CT_APPLICATION,
				       i + 1);
	}
}

static void prv_del_dup_char(cpc_characteristic_t *characteristic,
			     cpc_characteristic_type_t
			     characteristic_type,
			     cpc_characteristic_t *target_char1,
			     cpc_param_type_t param_type, int param1,
			     int from)
{
	int j = -1;
	int param2 = -1;
	cpc_characteristic_t *target_char2 = NULL;

	j = cpc_find_char(characteristic, characteristic_type, from + 1);

	while (j != -1) {
		target_char2 = cpc_get_char(characteristic, j);

		param2 = cpc_find_param(target_char2, param_type, 0);

		if (param2 != -1) {
			if (prv_params_equal(target_char1, param1, target_char2,
					     param2)) {
				cpc_ptr_array_delete
					(&characteristic->characteristics, j);
				CPC_LOGF("Deleting duplicate CHAR %d",
					      characteristic_type);
			} else
				++j;
		} else
			++j;

		j = cpc_find_char(characteristic, characteristic_type, j);
	}
}

static void prv_rm_dup_char(cpc_characteristic_t *characteristic,
			    cpc_characteristic_type_t
			    characteristic_type,
			    cpc_param_type_t param_type)
{
	int param1 = -1;
	cpc_characteristic_t *target_char1 = NULL;
	int i = cpc_find_char(characteristic, characteristic_type, 0);

	while (i != -1) {
		target_char1 = cpc_get_char(characteristic, i);

		param1 = cpc_find_param(target_char1, param_type, 0);

		if (param1 != -1)
			prv_del_dup_char(characteristic,
					       characteristic_type,
					       target_char1, param_type, param1,
					       i);
		i = cpc_find_char(characteristic, characteristic_type,
				       i + 1);
	}
}

static void prv_rm_extra_domains(cpc_characteristic_t *characteristic)
{
	int i = 0;
	unsigned int domains = 0;

	i = cpc_find_param(characteristic, CPC_PT_DOMAIN, 0);

	while (i != -1 && domains < 4) {
		++domains;
		i = cpc_find_param(characteristic, CPC_PT_DOMAIN, i + 1);
	}

	while (i != -1) {
		cpc_ptr_array_delete(&characteristic->parameters, i);
		CPC_LOGF("Deleting extra DOMAIN");
		i = cpc_find_param(characteristic, CPC_PT_DOMAIN, i);
	}
}

static int prv_add_default_param(cpc_characteristic_t *characteristic,
				 cpc_param_type_t param_type,
				 cpc_parameter_t **new_param)
{
	CPC_ERR_MANAGE;
	cpc_parameter_t *param = NULL;
	int i = cpc_find_param(characteristic, param_type, 0);

	if (i == -1) {
		CPC_FAIL_NULL(param, malloc(sizeof(*param)), CPC_ERR_OOM);

		param->type = param_type;
		param->transient = true;

		CPC_FAIL(cpc_ptr_array_append(&characteristic->parameters,
					      param));

		CPC_LOGF("Adding Int Parameter %d", param_type);
	}

	*new_param = param;

	return CPC_ERR_NONE;

CPC_ON_ERR:

	free(param);

	return CPC_ERR;
}

static int prv_add_default_int_param(cpc_characteristic_t *characteristic,
				     cpc_param_type_t param_type,
				     unsigned int value)
{
	CPC_ERR_MANAGE;
	cpc_parameter_t *param = NULL;

	CPC_FAIL(prv_add_default_param(characteristic, param_type, &param));

	if (param) {
		param->int_value = value;
		param->data_type = CPC_WPDT_UINT;
	}

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_add_default_utf8_param(cpc_characteristic_t *characteristic,
				      cpc_param_type_t param_type,
				      const char *value)
{
	CPC_ERR_MANAGE;
	cpc_parameter_t *param = NULL;
	xmlChar *utf8Str;

	CPC_FAIL(prv_add_default_param(characteristic, param_type, &param));

	if (param) {
		CPC_FAIL_NULL_LABEL(utf8Str, xmlStrdup((xmlChar*) value),
					 CPC_ERR_OOM, remove_param);
		param->utf8_value = utf8Str;
		param->data_type = CPC_WPDT_UTF8;
	}

	return CPC_ERR_NONE;

remove_param:

	cpc_ptr_array_delete(&characteristic->characteristics,
				      cpc_get_char_count
				      (characteristic) - 1);

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_validate_int_parameter(cpc_characteristic_t *characteristic,
				      cpc_param_type_t param_type,
				      unsigned int max_value)
{
	CPC_ERR_MANAGE;
	cpc_parameter_t *parameter = NULL;

	int param = cpc_find_param(characteristic, param_type, 0);

	if (param != -1) {
		parameter = cpc_get_param(characteristic, param);

		if (parameter->int_value > max_value) {
			cpc_ptr_array_delete(&characteristic->parameters,
					     param);
			CPC_ERR = CPC_ERR_CORRUPT;
		}
	}

	return CPC_ERR;
}

static int prv_validate_utf8_parameter(cpc_characteristic_t *characteristic,
				       cpc_param_type_t param_type)
{
	CPC_ERR_MANAGE;
	cpc_parameter_t *parameter = NULL;
	int param = cpc_find_param(characteristic, param_type, 0);

	if (param != -1) {
		parameter = cpc_get_param(characteristic, param);
		if (xmlStrlen(parameter->utf8_value) > CPC_PARSER_MAX_REF) {
			cpc_ptr_array_delete(&characteristic->parameters,
					     param);
			CPC_ERR = CPC_ERR_CORRUPT;
		}
	}

	return CPC_ERR;
}

static int prv_validate_bootstrap_characteristic(cpc_characteristic_t
						 *characteristic)
{
	CPC_ERR_MANAGE;
	int i = cpc_find_param(characteristic, CPC_PT_PROVURL, 0);

	if (i == -1) {
		if (!prv_validate_utf8_combo(characteristic, CPC_PT_NETWORK,
					     CPC_PT_COUNTRY))
			CPC_ERR = CPC_ERR_CORRUPT;
	} else {
		/*
		 * Check to see if we have a Country or a Network.
		 * If so we will delete them.
		 */

		prv_delete_parameter(characteristic, CPC_PT_COUNTRY);
		prv_delete_parameter(characteristic, CPC_PT_NETWORK);

		(void) prv_validate_int_parameter(characteristic,
						 CPC_PT_CONTEXT_ALLOW, 255);

	}

	return CPC_ERR;
}

static int prv_add_dup_parameter(cpc_characteristic_t *characteristic,
				 cpc_parameter_t *param)
{
	CPC_ERR_MANAGE;
	cpc_parameter_t *param_dup = NULL;

	CPC_FAIL(prv_parameter_dup(param, &param_dup));
	CPC_FAIL(cpc_ptr_array_append(&characteristic->parameters, param_dup));

	return CPC_ERR_NONE;

CPC_ON_ERR:

	free(param_dup);

	return CPC_ERR;
}

static int prv_add_dup_char(cpc_characteristic_t *characteristic,
			    cpc_characteristic_t *characteristic_to_dup)
{
	CPC_ERR_MANAGE;
	cpc_characteristic_t *char_dup = NULL;

	CPC_FAIL(prv_characteristic_dup(characteristic_to_dup, &char_dup));
	CPC_FAIL(cpc_ptr_array_append(&characteristic->characteristics,
				      char_dup));

	return CPC_ERR_NONE;

CPC_ON_ERR:

	free(char_dup);

	return CPC_ERR;
}

static int prv_merge_existing_param(cpc_characteristic_t *characteristic1,
				    int char1Params, cpc_parameter_t *param2,
				    unsigned int start_from)
{
	CPC_ERR_MANAGE;
	cpc_parameter_t *param1 = NULL;
	cpc_parameter_t *param_copy = NULL;
	bool found_match;
	cpc_param_occurrence_t occurrence;
	int from = start_from;

	param1 = cpc_get_param(characteristic1, from);

	/* If we have found a matching parameter but it is transient we should
	 * delete it and simply replace it with the new parameter from
	 * characteristic2. There should only be one transient parameter of any
	 * given type in a characteristic.
	 */

	if (param1->transient) {
		CPC_FAIL(prv_parameter_dup(param2, &param_copy));
		prv_parameter_free(param1);
		cpc_ptr_array_set(&characteristic1->parameters, from,
				  param_copy);
	} else {
		found_match = false;
		while (from != -1 && from < char1Params && !found_match) {
			param1 = cpc_get_param(characteristic1, from);
			found_match = prv_compare_params(param1, param2);
			if (!found_match)
				from = cpc_find_param(characteristic1,
							param2->type, from + 1);
		}

		if (!found_match) {

			/*
			 * There is an exisiting parameter of the same type in
			 * the target characteristic but it does not have the
			 * same value as the current parameter in char 2. We
			 * can add if multiple instances of the same parameter
			 * are permitted.
			 */

			/* This function call must succeed */

			found_match =
				prv_find_param_occurrence(characteristic1->type,
							  param1->type,
							  &occurrence);
			if (found_match && (occurrence != CPC_WP_OCCUR_ONCE) &&
			    (occurrence != CPC_WP_OCCUR_ZERO_OR_ONE))
				CPC_FAIL(
					prv_add_dup_parameter(characteristic1,
							      param2));
		}
	}

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_merge_parameters(cpc_characteristic_t *characteristic1,
				cpc_characteristic_t *characteristic2)
{
	CPC_ERR_MANAGE;
	cpc_parameter_t *param2 = NULL;
	unsigned int i = 0;
	int j = -1;
	int char1_params = cpc_get_param_count(characteristic1);

	for (i = 0; i < cpc_get_param_count(characteristic2); ++i) {
		param2 = cpc_get_param(characteristic2, i);

		j = cpc_find_param(characteristic1, param2->type, 0);

		if (j == -1) {
			/*
			 * This parameter does not exist in first
			 * characteristic. We can add.
			 */

			CPC_FAIL(prv_add_dup_parameter(characteristic1,
							    param2));
		} else {
			CPC_FAIL(
				prv_merge_existing_param(characteristic1,
							 char1_params, param2,
							 j));
		}
	}

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_merge_child_characteristic(cpc_characteristic_t *characteristic1,
					  cpc_characteristic_t *characteristic2,
					  cpc_characteristic_type_t
					  characteristic_type,
					  cpc_param_type_t key)
{
	CPC_ERR_MANAGE;
	cpc_characteristic_t *child_char1 = NULL;
	cpc_characteristic_t *child_char2 = NULL;
	cpc_parameter_t *child_param1 = NULL;
	cpc_parameter_t *child_param2 = NULL;
	int i = -1;
	int j = -1;
	int k = -1;
	int l = -1;
	bool found_match = false;
	int char1CharCount = cpc_get_char_count(characteristic1);

	i = cpc_find_char(characteristic2, characteristic_type, 0);

	while (i != -1) {
		child_char2 = cpc_get_char(characteristic2, i);

		j = cpc_find_param(child_char2, key, 0);

		if (j != -1) {
			child_param2 = cpc_get_param(child_char2, j);
			k = cpc_find_char(characteristic1,
					  characteristic_type, 0);

			found_match = false;
			while (k != -1 && k < char1CharCount && !found_match) {
				child_char1 = cpc_get_char(characteristic1,
							   k);
				l = cpc_find_param(child_char1, key, 0);

				if (l != -1) {
					child_param1 =
						cpc_get_param(child_char1, l);

					found_match =
						prv_compare_params(
							child_param1,
							child_param2);
				}

				if (!found_match)
					k = cpc_find_char(
						characteristic1,
						characteristic_type, k + 1);
			}

			if (!found_match)
				CPC_FAIL(prv_add_dup_char(characteristic1,
							  child_char2));
			else
				CPC_FAIL(prv_merge_parameters(child_char1,
							      child_char2));

			i = cpc_find_char(characteristic2,
					  characteristic_type, i + 1);
		}
	}

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_merge_pxphysical_characteristics(cpc_characteristic_t *phys1,
						cpc_characteristic_t *phys2)
{
	CPC_ERR_MANAGE;

	CPC_LOGF("Merging duplicate PXPHYSICAL PROXIES");

	CPC_FAIL(prv_merge_parameters(phys1, phys2));

	prv_rm_extra_domains(phys1);

	/* Now just need to add non duplicate port characteristics */

	CPC_FAIL(prv_merge_child_characteristic(phys1, phys2, CPC_CT_PORT,
						     CPC_PT_PORTNBR));

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_resolve_non_related_physicals(cpc_characteristic_t
					     *characteristic1,
					     cpc_characteristic_t
					     *characteristic2)
{
	CPC_ERR_MANAGE;
	cpc_characteristic_t *target_char1 = NULL;
	cpc_characteristic_t *target_char2 = NULL;
	bool found_match = false;
	int j = -1;
	int k = -1, l = -1;
	int char1_char_count = cpc_get_char_count(characteristic1);
	int i = cpc_find_char(characteristic2, CPC_CT_PXPHYSICAL, 0);

	CPC_LOGF("Merging nonRelated Physicals");

	while (i != -1) {
		target_char2 = cpc_get_char(characteristic2, i);

		j = cpc_find_char(characteristic1, CPC_CT_PXPHYSICAL, 0);

		found_match = false;

		while (j != -1 && j < char1_char_count && !found_match) {
			target_char1 = cpc_get_char(characteristic1, j);
			k = cpc_find_param(target_char1,
						CPC_PT_PHYSICAL_PROXY_ID, 0);
			l = cpc_find_param(target_char2,
						CPC_PT_PHYSICAL_PROXY_ID, 0);
			if (k != -1 && l != -1)
				found_match = prv_params_equal(target_char1, k,
							      target_char2, l);

			if (!found_match)
				j = cpc_find_char(characteristic1,
						       CPC_CT_PXPHYSICAL,
						       j + 1);
		}

		if (found_match)
			CPC_FAIL(prv_merge_pxphysical_characteristics(
					target_char1, target_char2));
		else {
			CPC_LOGF("Adding new Physical");
			CPC_FAIL(prv_add_dup_char(characteristic1,
						       target_char2));
		}

		i = cpc_find_char(characteristic2, CPC_CT_PXPHYSICAL,
				       i + 1);
	}

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_merge_pxlogical_characteristics(cpc_characteristic_t *log1,
					       cpc_characteristic_t *log2)
{
	CPC_ERR_MANAGE;

	CPC_LOGF("Merging duplicate PXLOGICAL PROXIES");

	CPC_FAIL(prv_merge_parameters(log1, log2));

	CPC_FAIL(prv_merge_child_characteristic(log1, log2, CPC_CT_PORT,
						     CPC_PT_PORTNBR));
	CPC_FAIL(prv_merge_child_characteristic(log1, log2,
						     CPC_CT_PXAUTHINFO,
						     CPC_PT_PXAUTH_TYPE));
	CPC_FAIL(prv_resolve_non_related_physicals(log1, log2));

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_resolve_dup_characteristics(cpc_characteristic_t *characteristic,
					   cpc_characteristic_type_t
					   characteristic_type,
					   cpc_param_type_t key,
					   cpc_char_merge_fn_t merge_fn)
{
	CPC_ERR_MANAGE;
	cpc_characteristic_t *target_char1 = NULL;
	cpc_characteristic_t *target_char2 = NULL;

	int j = -1;
	int k = -1, l = -1;
	int i = cpc_find_char(characteristic, characteristic_type, 0);

	while (i != -1) {
		target_char1 = cpc_get_char(characteristic, i);

		j = cpc_find_char(characteristic, characteristic_type,
				       i + 1);

		while (j != -1) {
			target_char2 = cpc_get_char(characteristic, j);

			k = cpc_find_param(target_char1, key, 0);
			l = cpc_find_param(target_char2, key, 0);

			if (k != -1 && l != -1) {
				if (prv_params_equal
				    (target_char1, k, target_char2, l)) {
					CPC_FAIL(merge_fn(target_char1,
							       target_char2));
					cpc_ptr_array_delete(
						&characteristic->
						characteristics, j);
				} else
					++j;
			} else
				++j;

			j = cpc_find_char(characteristic,
					       characteristic_type, j);
		}

		i = cpc_find_char(characteristic, characteristic_type,
				       i + 1);
	}

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_validate_pxlogical_characteristic(cpc_characteristic_t
						 *characteristic)
{
	/*
	 * We need to do the following here:
	 *
	 * 1. Delete port characteristics with duplicate PORTNBRs.
	 * 2. Delete excess DOMAIN parameters.  We are only allowed 4.
	 * 3. Remove any duplicated physical proxies.
	 * 4. Remove any duplicated PXAUTHINFOs
	 * 5. Check if PUSHENABLED and PULLENABLED are defined. If not add them.
	 * 6. Check if WSP-VERSION is defined.  If not set it to 1.2
	 */

	CPC_ERR_MANAGE;

	prv_rm_dup_char(characteristic, CPC_CT_PORT, CPC_PT_PORTNBR);
	prv_rm_extra_domains(characteristic);
	prv_rm_dup_char(characteristic, CPC_CT_PXAUTHINFO,
					  CPC_PT_PXAUTH_TYPE);

	CPC_FAIL(prv_resolve_dup_characteristics(
			      characteristic, CPC_CT_PXPHYSICAL,
			      CPC_PT_PHYSICAL_PROXY_ID,
			      prv_merge_pxphysical_characteristics));

	CPC_FAIL(prv_add_default_int_param(characteristic,
						CPC_PT_PUSHENABLED, 0));
	CPC_FAIL(prv_add_default_int_param(characteristic,
						CPC_PT_PULLENABLED, 0));
	CPC_FAIL(prv_add_default_utf8_param(characteristic,
						 CPC_PT_DOMAIN, ""));
	CPC_FAIL(prv_add_default_utf8_param(characteristic,
						 CPC_PT_WSP_VERSION, "1.2"));

	CPC_FAIL(prv_validate_utf8_parameter(characteristic,
						  CPC_PT_PROXY_ID));

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_validate_pxphysical_characteristic(cpc_characteristic_t
						  *characteristic)
{
	/*
	 * We need to do the following here:
	 *
	 * 1. Delete port characteristics with duplicate PORTNBRs.
	 * 2. Delete excess DOMAIN parameters. We are only allowed 4.
	 * 3. Check if PXADDRTYPE is defined. If not set it to "IPV4"
	 */

	prv_rm_dup_char(characteristic, CPC_CT_PORT, CPC_PT_PORTNBR);
	prv_rm_extra_domains(characteristic);

	return prv_add_default_utf8_param(characteristic,
					  CPC_PT_PXADDRTYPE, "IPV4");
}

static int prv_validate_napdef_characteristic(cpc_characteristic_t
					      *characteristic)
{
	/*
	 * We need to do the following here:
	 *
	 * 1. Delete NAPAUTHINFO characteristics with duplicate AUTHTYPE.
	 * 2. Delete VALIDITY characteristics with duplicate COUNTRYs.
	 * 3. If NAP-ADDRESS is defined but NAP-ADDRTYPE is not defined,
	 *    set NAP-ADDRTYPE to "E164".
	 * 4. Check if CALLTYPE is defined. If not set it to "ANALOG-MODEM"
	 * 5. If LOCAL-ADDR is defined but LOCAL-ADDRTYPE is not defined,
	 *    set LOCAL-ADDRTYPE to "IPV6"
	 * 6. Make sure bearer is 'GSM_CSD' or 'GSM_GPRS'.
	 */

	CPC_ERR_MANAGE;
	cpc_parameter_t *param = NULL;
	int index = -1;

	prv_rm_dup_char(characteristic, CPC_CT_NAPAUTHINFO,
					  CPC_PT_AUTHTYPE);
	prv_rm_dup_char(characteristic, CPC_CT_VALIDITY, CPC_PT_COUNTRY);

	if (cpc_find_param(characteristic, CPC_PT_NAP_ADDRESS, 0) != -1){
		CPC_FAIL(prv_add_default_utf8_param(characteristic,
						    CPC_PT_NAP_ADDRTYPE,
						    "E164"));
		CPC_FAIL(prv_add_default_utf8_param(characteristic,
							 CPC_PT_CALLTYPE,
							 "ANALOG-MODEM"));

		if (cpc_find_param(characteristic, CPC_PT_LOCAL_ADDR, 0)
		    != -1)
			CPC_FAIL(prv_add_default_utf8_param(characteristic,
					      CPC_PT_LOCAL_ADDRTYPE, "IPV6"));
	}

	index = cpc_find_param(characteristic, CPC_PT_BEARER, 0);

	if (index == -1)
		CPC_FAIL_FORCE(CPC_ERR_CORRUPT);

	param = cpc_get_param(characteristic, index);

	if (!(xmlStrEqual(param->utf8_value, (const xmlChar *)"GSM-CSD") ||
	      xmlStrEqual(param->utf8_value, (const xmlChar *)"GSM-GPRS"))) {
		CPC_LOGF("Unsupported Bearer %s", param->utf8_value);
		CPC_FAIL_FORCE(CPC_ERR_CORRUPT);
	}

	CPC_FAIL(prv_validate_utf8_parameter(characteristic, CPC_PT_NAPID));

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_validate_access_characteristic(cpc_characteristic_t
					      *characteristic)
{
	/*
	 * 1. Port number must be <= 0xffff if specified.
	 * 2. First parameter must be a rule.
	 * 3. Last n parameters must be either a TO-PROXY-ID or a TO-NAPID.
	 */

	CPC_ERR_MANAGE;
	cpc_parameter_t *param = NULL;
	unsigned int i = 0;

	(void) prv_validate_int_parameter(characteristic, CPC_PT_PORTNBR,
					 0xffff);

	if (cpc_get_param_count(characteristic) <= 1)
		CPC_FAIL_FORCE(CPC_ERR_CORRUPT);

	param = cpc_get_param(characteristic, 0);

	if (param->type != CPC_PT_RULE)
		CPC_FAIL_FORCE(CPC_ERR_CORRUPT);

	for (i = 1; i < cpc_get_param_count(characteristic) &&
		     (cpc_get_param(characteristic, i))->type !=
		     CPC_PT_TO_NAPID &&
		     (cpc_get_param(characteristic, i))->type !=
		     CPC_PT_TO_PROXY; ++i);

	if (i == cpc_get_param_count(characteristic))
		CPC_FAIL_FORCE(CPC_ERR_CORRUPT);

	for (; i < cpc_get_param_count(characteristic) &&
		     ((cpc_get_param(characteristic, i))->type ==
		      CPC_PT_TO_NAPID ||
		      (cpc_get_param(characteristic, i))->type ==
		      CPC_PT_TO_PROXY); ++i);

	if (i != cpc_get_param_count(characteristic))
		CPC_ERR = CPC_ERR_CORRUPT;

CPC_ON_ERR:

	return CPC_ERR;
}

static bool prv_contains_port(cpc_characteristic_t *characteristic)
{
	bool retval = false;
	int index = -1;

	index = cpc_find_char(characteristic, CPC_CT_APPADDR, 0);

	if (index != -1) {
		characteristic = cpc_get_char(characteristic, index);
		index = cpc_find_char(characteristic, CPC_CT_PORT, 0);
		if (index != -1)
			retval = true;
	}

	return retval;
}

static int prv_validate_application_characteristic(cpc_characteristic_t
						   *characteristic)
{
	/*
	 * 1. We are just going to check to see if we support the application
	 *    type. If not we will delete the entire characteristic. There's no
	 *    point in keeping it if no one is going to process the data.
	 *
	 * 2. The parser allows application and resource parameters specific to
	 *    one application to appear in the characteristic of another
	 *    application type. We could filter them out here but its not worth
	 *    the effort as they will be filtered out by the cp_to_dm parser.
	 *
	 * 3. We will do some application specific checks though, e.g., ensure
	 *    that a DM account has a server ID, that characteristics have at
	 *    least one address etc.
	 */

	CPC_ERR_MANAGE;
	cpc_parameter_t *param = NULL;
	cpc_parameter_t *prov_id;
	cpc_characteristic_t *wap_char = NULL;
	int j = 0;
	int appArraySize = sizeof(g_supported_applications) /
				sizeof(const xmlChar *);
	int i = cpc_find_param(characteristic, CPC_PT_APPID, 0);
	int index = -1;

	if (i == -1)
		CPC_FAIL_FORCE(CPC_ERR_CORRUPT);

	param = cpc_get_param(characteristic, i);
	for (j = 0; j < appArraySize && !xmlStrEqual(param->utf8_value,
				  g_supported_applications[j]); ++j) ;

	/* Check the length of the NAME and the Provider ID params. */

	(void) prv_validate_utf8_parameter(characteristic, CPC_PT_NAME);
	(void)prv_validate_utf8_parameter(characteristic, CPC_PT_PROVIDER_ID);

	if (j == appArraySize) {
		CPC_LOGF("Unsupported Application Type %s",
			      param->utf8_value);
		CPC_FAIL_FORCE(CPC_ERR_CORRUPT);
	}
	if (!xmlStrEqual(param->utf8_value, (const xmlChar *)"w2")
	    && !xmlStrEqual(param->utf8_value, (const xmlChar *)"DL")) {
		if (cpc_find_param(characteristic, CPC_PT_ADDR, 0) == -1
		    && cpc_find_char(characteristic, CPC_CT_APPADDR, 0)
		    == -1) {
			CPC_LOGF("No Application Address");
			CPC_FAIL_FORCE(CPC_ERR_CORRUPT);
		}
	}

	if (xmlStrEqual(param->utf8_value, (const xmlChar *)"w7")) {
		index = cpc_find_param(characteristic, CPC_PT_PROVIDER_ID, 0);
		if (index == -1) {
			CPC_LOGF("DM Account needs ProviderID");
			CPC_FAIL_FORCE(CPC_ERR_CORRUPT);
		} else {
			/* Check added by Intel */
			prov_id = cpc_get_param(characteristic, index);
			if (!prov_id->utf8_value[0]) {
				CPC_LOGF("DM Account needs ProviderID");
				CPC_FAIL_FORCE(CPC_ERR_CORRUPT);
			}
		}
	} else if (xmlStrEqual(param->utf8_value, (const xmlChar *)"25")) {
		if ((cpc_find_param(characteristic, CPC_PT_FROM, 0)
		     == -1) || !prv_contains_port(characteristic)) {
			CPC_LOGF("Either EMAIL ADDRESS or PORT number"
				     " is mssing from SMTP account");
			CPC_FAIL_FORCE(CPC_ERR_CORRUPT);
		}
	} else if (xmlStrEqual(param->utf8_value, (const xmlChar *)"110") ||
		   xmlStrEqual(param->utf8_value, (const xmlChar *)"143"))
	{
		if (!prv_contains_port(characteristic)) {
			CPC_LOGF("Port number is missing");
			CPC_FAIL_FORCE(CPC_ERR_CORRUPT);
		}
	} else if (xmlStrEqual(param->utf8_value,(const xmlChar *)"w5")) {

		/*
		 * All w5 characteristic must have at least one resource
		 * characteristic and each resource characteristic must have
		 * an AACCEPT parameter.
		 */

		index = cpc_find_char(characteristic, CPC_CT_RESOURCE,
					   0);
		while (index != -1) {
			wap_char = cpc_get_char(characteristic, index);
			if (cpc_find_param(wap_char, CPC_PT_AACCEPT, 0)
			    == -1) {
				cpc_ptr_array_delete(&characteristic->
							      characteristics,
							      index);
				CPC_LOGF("RESOURCE missing AACCEPT param."
					     " Deleting");
			} else
				++index;

			index =  cpc_find_char(characteristic,
						    CPC_CT_RESOURCE, index);
		}

		if (cpc_find_char(characteristic, CPC_CT_RESOURCE, 0)
		    == -1) {
			CPC_LOGF("DS account requires at least one "
				     "resource");
			CPC_FAIL_FORCE(CPC_ERR_CORRUPT);
		}
	} else  if (xmlStrEqual(param->utf8_value, (const xmlChar *)"wA")) {
		if (cpc_find_param(characteristic, CPC_PT_AACCEPT, 0)
		    == -1) {
			CPC_LOGF("IM account requires an AACCEPT "
				     "parameter");
			CPC_FAIL_FORCE(CPC_ERR_CORRUPT);
		}
	}

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_validate_specific_characteristic(cpc_characteristic_t
						*characteristic)
{
	CPC_ERR_MANAGE;

	switch (characteristic->type) {
	case CPC_CT_ACCESS:
		CPC_ERR =
			prv_validate_access_characteristic(characteristic);
		break;
	case CPC_CT_APPADDR:
		prv_rm_dup_char(characteristic, CPC_CT_PORT,
						  CPC_PT_PORTNBR);
		break;
	case CPC_CT_APPLICATION:
		CPC_ERR =
			prv_validate_application_characteristic(characteristic);
		break;
	case CPC_CT_BOOTSTRAP:
		CPC_ERR =
			prv_validate_bootstrap_characteristic(characteristic);
		break;
	case CPC_CT_PORT:
		CPC_ERR =
		    prv_validate_int_parameter(characteristic, CPC_PT_PORTNBR,
					     0xffff);
		break;
	case CPC_CT_NAPDEF:
		CPC_ERR =
			prv_validate_napdef_characteristic(characteristic);
		break;
	case CPC_CT_PXLOGICAL:
		CPC_ERR =
			prv_validate_pxlogical_characteristic(characteristic);
		break;
	case CPC_CT_PXPHYSICAL:
		CPC_ERR =
			prv_validate_pxphysical_characteristic(characteristic);
		break;
	case CPC_CT_VALIDITY:
		CPC_ERR =
			prv_validate_validity_characteristic(characteristic);
		break;
	default:
		break;
	}

	return CPC_ERR;
}

static bool prv_remove_invalid_napdef_refs(cpc_characteristic_t *characteristic,
					   cpc_ptr_array_t *napd_ids);

static bool prv_remove_refs_from_other(cpc_characteristic_t *characteristic,
				       cpc_ptr_array_t *napd_ids)
{
	int i = -1;
	unsigned int j = 0;
	cpc_parameter_t *param = NULL;
	bool retval = true;
	unsigned int found = 0;
	unsigned int deleted = 0;
	unsigned int napid_count;

	i = cpc_find_param(characteristic, CPC_PT_TO_NAPID, 0);
	while (i != -1) {
		param = cpc_ptr_array_get(&characteristic->parameters, i);
		if (xmlStrEqual
		    ((const xmlChar *)"INTERNET", param->utf8_value))
			++i;
		else {
			napid_count = cpc_ptr_array_get_size(napd_ids);
			for (j = 0; j < napid_count && !xmlStrEqual(
				     param->utf8_value,
				     cpc_ptr_array_get(napd_ids, j));
			     ++j);

			if (j < cpc_ptr_array_get_size(napd_ids))
				++i;
			else {
				CPC_LOGF(
					"Deleting Invalid TO-NAPID.");
				cpc_ptr_array_delete(&characteristic->
							      parameters,
							      i);
				++deleted;
			}
		}

		i = cpc_find_param(characteristic, CPC_PT_TO_NAPID, i);
		++found;
	}

	if (found == deleted) {
		if (characteristic->type == CPC_CT_PXPHYSICAL) {
			CPC_LOGF("PXPHYSICAL contains no valid napids. "
				 "Deleting.");
			retval = false;
		} else if (characteristic->type == CPC_CT_ACCESS &&
			   prv_validate_access_characteristic(characteristic) !=
			   CPC_ERR_NONE) {
			CPC_LOGF("ACCESS contains no valid references. "
				 "Deleting.");
			retval = false;
		}
	}

	return retval;
}

static bool prv_remove_refs_from_pxl(cpc_characteristic_t *characteristic,
				     cpc_ptr_array_t *napd_ids)
{
	unsigned int j = 0;
	cpc_characteristic_t *wap_char = NULL;
	bool retval = true;
	unsigned int found = 0;
	unsigned int deleted = 0;

	for (j = 0; j < cpc_get_char_count(characteristic); ++j) {
		wap_char = cpc_get_char(characteristic, j);
		if (wap_char->type == CPC_CT_PXPHYSICAL) {
			if (!prv_remove_invalid_napdef_refs
			    (wap_char, napd_ids)) {
				++deleted;
				cpc_ptr_array_delete(&characteristic->
							      characteristics,
							      j);
				--j;
			}
			++found;
		}
	}

	retval = found > deleted;

	return retval;
}

static void prv_remove_refs_from_root(cpc_characteristic_t *characteristic,
				      cpc_ptr_array_t *napd_ids)
{
	unsigned int j = 0;
	cpc_characteristic_t *wap_char = NULL;

	for (j = 0; j < cpc_get_char_count(characteristic); ++j) {
		wap_char = cpc_get_char(characteristic, j);
		if (!prv_remove_invalid_napdef_refs(wap_char, napd_ids)) {
			CPC_LOGF("Removal of NAPDEFs renders Characteristic "
				 "invalid.");
			cpc_ptr_array_delete(&characteristic->
						      characteristics,
						      j);
			--j;
		}
	}
}

static bool prv_remove_invalid_napdef_refs(cpc_characteristic_t *characteristic,
					   cpc_ptr_array_t *napd_ids)
{
	bool retval = true;

	if (characteristic->type == CPC_CT_APPLICATION
	    || characteristic->type == CPC_CT_ACCESS
	    || characteristic->type == CPC_CT_PXPHYSICAL)
		retval = prv_remove_refs_from_other(characteristic, napd_ids);
	else if (characteristic->type == CPC_CT_PXLOGICAL)
		retval = prv_remove_refs_from_pxl(characteristic, napd_ids);
	else if (characteristic->type == CPC_CT_ROOT)
		prv_remove_refs_from_root(characteristic, napd_ids);

	return retval;
}

static int prv_validate_napdef_refs(cpc_characteristic_t *root)
{
	CPC_ERR_MANAGE;
	cpc_characteristic_t *wap_char = NULL;
	cpc_parameter_t *wap_parameter = NULL;
	cpc_ptr_array_t napids;
	int i = -1, j = -1;

	cpc_ptr_array_make(&napids, 4, NULL);
	i = cpc_find_char(root, CPC_CT_NAPDEF, 0);

	while (i != -1) {
		wap_char = (cpc_characteristic_t *)
		    cpc_ptr_array_get(&root->characteristics, i);
		j = cpc_find_param(wap_char, CPC_PT_NAPID, 0);
		if (j != -1) {
			wap_parameter = (cpc_parameter_t *)
			    cpc_ptr_array_get(&wap_char->parameters, j);
			CPC_FAIL(
				cpc_ptr_array_append(
					&napids, wap_parameter->utf8_value));
		}
		i = cpc_find_char(root, CPC_CT_NAPDEF, i + 1);
	}

CPC_ON_ERR:

	if (CPC_ERR == CPC_ERR_NONE)
		(void) prv_remove_invalid_napdef_refs(root, &napids);

	cpc_ptr_array_free(&napids);

	return CPC_ERR;

}

static int prv_remove_dup_rule(cpc_characteristic_t *wap_char, int* from,
			       cpc_ptr_array_t *rule_names)
{
	CPC_ERR_MANAGE;
	cpc_parameter_t *wap_parameter = NULL;
	unsigned int k = 0;
	int j = *from;
	xmlChar *rule_name;
	unsigned int access_param_size = 0;

	wap_parameter =  cpc_ptr_array_get(&wap_char->parameters, j);
	rule_name = (wap_parameter->data_type == CPC_WPDT_UTF8) ?
		wap_parameter->utf8_value : (xmlChar *) "";

	for (k = 0; k < cpc_ptr_array_get_size(rule_names) &&
		     !xmlStrEqual(rule_name, cpc_ptr_array_get(
					  rule_names, k)); ++k);

	if (k == cpc_ptr_array_get_size(rule_names)) {
		CPC_FAIL(cpc_ptr_array_append(rule_names, rule_name));
		++j;
	} else {
		/* We need to remove the invalid rule */

		CPC_LOGF("Removing duplicate rule: %s", rule_name);

		do {
			k = j;
			cpc_ptr_array_delete(&wap_char->parameters, k);
			access_param_size = cpc_get_param_count(wap_char);
			if (k < access_param_size)
				wap_parameter = cpc_ptr_array_get(
					&wap_char->parameters, k);
		} while (k < access_param_size && wap_parameter->type !=
			 CPC_PT_RULE && wap_parameter->type !=
			 CPC_PT_TO_NAPID && wap_parameter->type !=
			 CPC_PT_TO_PROXY);
		j = k;
	}

	j = cpc_find_param(wap_char, CPC_PT_RULE, j);

	*from = j;

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_validate_access_rules(cpc_characteristic_t *root)
{
	CPC_ERR_MANAGE;
	cpc_ptr_array_t rule_names;
	cpc_characteristic_t *wap_char = NULL;
	int i = -1, j = -1;

	cpc_ptr_array_make(&rule_names, 8, NULL);

	i = cpc_find_char(root, CPC_CT_ACCESS, 0);

	while (i != -1) {
		wap_char = cpc_ptr_array_get(&root->characteristics, i);

		j = cpc_find_param(wap_char, CPC_PT_RULE, 0);

		while (j != -1)
			CPC_FAIL(prv_remove_dup_rule(wap_char, &j,
						     &rule_names));

		/* We need to check to see if the ACCESS rule is still valid */

		if (cpc_find_param(wap_char, CPC_PT_RULE, 0) == -1) {
			CPC_LOGF("No valid rules left. Deleting ACCESS");
			cpc_ptr_array_delete(&root->characteristics, i);
		} else
			++i;

		i = cpc_find_char(root, CPC_CT_ACCESS, i);
	}

CPC_ON_ERR:

	cpc_ptr_array_free(&rule_names);

	return CPC_ERR;
}

static int prv_validate_non_root_char(cpc_characteristic_t *characteristic)
{
	CPC_ERR_MANAGE;
	cpc_valid_param_t *param_list = NULL;
	unsigned int i = 0, j = 0;
	unsigned int param_list_count = 0;
	cpc_valid_characteristic_t *char_list = NULL;
	unsigned int char_list_count = 0;

	param_list = g_char_allowed_params[characteristic->type].params;
	param_list_count =
		g_char_allowed_params[characteristic->type].number_of_params;

	for (i = 0; i < param_list_count; ++i) {
		if (param_list[i].occurence != CPC_WP_OCCUR_ONCE
		    && param_list[i].occurence != CPC_WP_OCCUR_ONE_OR_MORE)
			continue;

		for (j = 0; j < cpc_get_param_count(characteristic)
			     && ((cpc_parameter_t *)
				 cpc_ptr_array_get(&characteristic->
							    parameters,
							    j))->type !=
			     param_list[i].type; ++j) ;

		if (j == cpc_get_param_count(characteristic))
			CPC_FAIL_FORCE(CPC_ERR_CORRUPT);
	}

	char_list = g_char_allowed_chars[characteristic->type].children;
	char_list_count =
		g_char_allowed_chars[characteristic->type].number_of_chars;

	for (i = 0; i < char_list_count; ++i) {
		if (char_list[i].occurence != CPC_WP_OCCUR_ONCE &&
		    char_list[i].occurence != CPC_WP_OCCUR_ONE_OR_MORE)
			continue;

		for (j = 0;
		     j < cpc_get_char_count(characteristic)
			     && ((cpc_characteristic_t *)
				 cpc_ptr_array_get(&characteristic->
							    characteristics,
							    j))->type !=
			     char_list[i].type; ++j) ;

		if (j == cpc_get_char_count(characteristic))
			CPC_FAIL_FORCE(CPC_ERR_CORRUPT);
	}

	/* Finally, let's perform some characteristic specific checks */

	CPC_FAIL(prv_validate_specific_characteristic(characteristic));

	return CPC_ERR_NONE;

CPC_ON_ERR:

	CPC_LOGF("Deleting char %d", characteristic->type);

	return CPC_ERR;
}

static int prv_validate_cpc_characteristic(cpc_characteristic_t *characteristic)
{
	/*
	 * This function is called after we have parsed the xml file and we have
	 * an in memory model of the xml file in memory (non DOM). Now we can
	 * perform the remaining checks which are:
	 *
	 * 1) Check the ONCE and ONE_OR_MORE occurence properties for params
	 *    and chars.
	 * 2) Perform characteristic/parameter specific checks.
	 *
	 * This function can return three values:
	 *    CPC_ERR_NONE: Node is fine
	 *    CPC_ERR_CORRUPT: Node is corrupt and must be deleted by
	 *                            caller
	 *    CPC_ERR_OOM: Memory failure. We must stop processing the
	 *                        tree.
	 */

	CPC_ERR_MANAGE;
	unsigned int i = 0;
	int child_ret_val = CPC_ERR_NONE;

	/*
	 * We need to check the children first as we may have to delete some
	 * of them.
	 */

	while (i < cpc_get_char_count(characteristic)) {
		child_ret_val = prv_validate_cpc_characteristic(
			cpc_ptr_array_get(&characteristic->characteristics, i));

		if (child_ret_val == CPC_ERR_OOM)
			CPC_FAIL_FORCE(child_ret_val);
		else if (child_ret_val == CPC_ERR_CORRUPT)
			cpc_ptr_array_delete(&characteristic->characteristics,
					     i);
		else
			++i;
	}

	if (characteristic->type != CPC_CT_ROOT)
		CPC_FAIL(prv_validate_non_root_char(characteristic));
	else if (cpc_get_char_count(characteristic) == 0)
		CPC_FAIL_FORCE(CPC_ERR_CORRUPT);

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_find_matching_access(cpc_characteristic_t *root,
				    cpc_characteristic_t *wap_char,
				    bool *matching)
{
	CPC_ERR_MANAGE;
	int j = -1;
	int appid1;
	int appid2;
	bool matching_access = false;
	cpc_characteristic_t *access = NULL;

	appid1 = cpc_find_param(wap_char, CPC_PT_APPID, 0);

	if (appid1 == -1)
		CPC_FAIL(CPC_ERR_CORRUPT);

	matching_access = false;

	j = cpc_find_char(root, CPC_CT_ACCESS, 0);

	while (j != -1 && !matching_access) {
		access = cpc_get_char(root, j);
		appid2 = cpc_find_param(access, CPC_PT_APPID, 0);
		matching_access = (appid2 == -1);

		while (appid2 != -1 && !matching_access) {
			matching_access = prv_params_equal(wap_char, appid1,
							  access, appid2);
			appid2 = cpc_find_param(access, CPC_PT_APPID,
						     appid2 + 1);
		}

		if (!matching_access)
			j = cpc_find_char(root, CPC_CT_ACCESS, j + 1);
	}

	*matching = matching_access;

	return CPC_ERR_NONE;

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_bind_app(cpc_characteristic_t *root,
			cpc_characteristic_t *wap_char)
{
	CPC_ERR_MANAGE;
	unsigned int index = 0;
	cpc_parameter_t *connectoid = NULL;
	cpc_characteristic_t *connector = NULL;
	int j = -1;

	/*
	 * Then this appliction characteristic really is not linked to any
	 * other connection settings in the document. We need to iterate through
	 * all the proxies and access points and add them to the application.
	 */

	for (index = 0; index < cpc_get_char_count(root); ++index) {
		connector = cpc_get_char(root, index);

		if (connector->type == CPC_CT_PXLOGICAL) {
			j = cpc_find_param(connector, CPC_PT_PROXY_ID,
						0);
			if (j == -1)
				CPC_FAIL(CPC_ERR_CORRUPT);

			connectoid = cpc_get_param(connector, j);

			CPC_FAIL(prv_add_default_utf8_param(
					      wap_char, CPC_PT_TO_PROXY,
					      (char *) connectoid->utf8_value));
		} else if (connector->type == CPC_CT_NAPDEF) {
			j = cpc_find_param(connector, CPC_PT_NAPID, 0);

			if (j == -1)
				CPC_FAIL(CPC_ERR_CORRUPT);

			connectoid = cpc_get_param(connector, j);

			CPC_FAIL(prv_add_default_utf8_param(
					      wap_char, CPC_PT_TO_NAPID,
					      (char *)connectoid->utf8_value));
		}
	}

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_bind_unlinked_apps(cpc_characteristic_t *root)
{
	CPC_ERR_MANAGE;
	int i = -1;
	cpc_characteristic_t *wap_char = NULL;
	bool matching_access = false;

	i = cpc_find_char(root, CPC_CT_APPLICATION, 0);

	while (i != -1) {
		wap_char = cpc_get_char(root, i);

		if ((cpc_find_param(wap_char, CPC_PT_TO_NAPID, 0) ==
		     -1) &&
		    (cpc_find_param(wap_char, CPC_PT_TO_PROXY, 0) ==
		     -1)) {

			/*
			 * We have an application with no connectoids.
			 * Are there any mactching access rules.
			 */

			CPC_FAIL(prv_find_matching_access(root, wap_char,
							  &matching_access));

			if (!matching_access)
				CPC_FAIL(prv_bind_app(root, wap_char));
		}

		i = cpc_find_char(root, CPC_CT_APPLICATION, i + 1);
	}

CPC_ON_ERR:

	return CPC_ERR;
}

static int prv_perform_global_checks(cpc_characteristic_t *root)
{
	/*
	 * Perform global checks on the document:
	 *
	 * 1. Ensure the PROXY_ID of each PXLOGICAL is unique.
	 * 2. Ensure the NAPID of each NAPDEF is unique.
	 * 3. Ensure the CLIENT_ID of each CLIENTIDENTITY is unique.
	 * 4. Ensure the NAME of each VENDORCONFIG is unique.
	 * 5. Ensure that there is only one bootstrap with a PROVURL.
	 * 6. Ensure that all TO-NAPIDs refer to valid NAPDEFs.
	 */

	CPC_ERR_MANAGE;
	int i = -1, j = -1;
	cpc_characteristic_t *wap_char = NULL;

	prv_rm_dup_char(root, CPC_CT_NAPDEF, CPC_PT_NAPID);
	prv_rm_dup_char(root, CPC_CT_CLIENTIDENTITY, CPC_PT_CLIENT_ID);
	prv_rm_dup_char(root, CPC_CT_VENDORCONFIG, CPC_PT_NAME);

	prv_rm_dup_app(root, "w2", CPC_PT_NAME);
	prv_rm_dup_app(root, "w4", CPC_PT_APPID);
	prv_rm_dup_app(root, "w5", CPC_PT_PROVIDER_ID);
	prv_rm_dup_app(root, "w7", CPC_PT_PROVIDER_ID);
	prv_rm_dup_app(root, "25", CPC_PT_PROVIDER_ID);
	prv_rm_dup_app(root, "143", CPC_PT_PROVIDER_ID);
	prv_rm_dup_app(root, "110", CPC_PT_PROVIDER_ID);
	prv_rm_dup_app(root, "wA", CPC_PT_PROVIDER_ID);
	prv_rm_dup_app(root, "ap0004", CPC_PT_PROVIDER_ID);

	i = cpc_find_char(root, CPC_CT_BOOTSTRAP, 0);

	while (i != -1 && j == -1) {
		wap_char = cpc_get_char(root, i);
		j = cpc_find_param(wap_char, CPC_PT_PROVURL, 0);
		i = cpc_find_char(root, CPC_CT_BOOTSTRAP, i + 1);
	}

	if (j != -1) {
		/*
		 * Then we have at least one bootstrap characteristic with
		 * a provurl parameter.
		 */

		while (i != -1) {
			wap_char = cpc_get_char(root, i);
			if (cpc_find_param
			    (wap_char, CPC_PT_PROVURL, 0) != -1) {
				cpc_ptr_array_delete(&root->
							   characteristics,
							   i);
				CPC_LOGF("Deleting BOOTSTRAP. Only one "
					 "PROVURL allowed ");
			} else
				++i;

			i = cpc_find_char(root, CPC_CT_BOOTSTRAP,
					       i);
		}
	}

	CPC_FAIL(prv_resolve_dup_characteristics(root, CPC_CT_PXLOGICAL,
						 CPC_PT_PROXY_ID,
		    prv_merge_pxlogical_characteristics));

	CPC_FAIL(prv_validate_napdef_refs(root));
	CPC_FAIL(prv_validate_access_rules(root));

	if (cpc_get_char_count(root) == 0)
		CPC_FAIL_FORCE(CPC_ERR_CORRUPT);

	CPC_FAIL_FORCE(prv_bind_unlinked_apps(root));

CPC_ON_ERR:

	return CPC_ERR;
}

#ifdef CPC_LOGGING

static void prv_dump_characteristic(cpc_characteristic_t *root,
				    unsigned int depth)
{
	char* depth_buf;
	const char *transient;
	const char *depth_str;
	unsigned int j = 0, i = 0, l = 0;

	depth_buf = malloc(depth + 1);
	if (depth_buf) {
		for (j = 0; j < depth; ++j)
			depth_buf[j] = '\t';
		depth_buf[j] = 0;
		depth_str = depth_buf;
	} else
		depth_str = "";

	for (l = 0; l < sizeof(g_char_string_map) /
		     sizeof(cpc_char_string_map_t) &&
	     g_char_string_map[l].type != root->type; ++l) ;

	if (l < sizeof(g_char_string_map) / sizeof(cpc_char_string_map_t)) {
		CPC_LOGUF("Characteristic: %s", g_char_string_map[l].string);
	}

	for (i = 0; i < cpc_get_param_count(root); ++i) {
		cpc_parameter_t *param = cpc_get_param(root, i);

		for (l = 0;
		     l < sizeof(g_param_string_map) /
			     sizeof(cpc_param_string_map_t)
			     && g_param_string_map[l].type != param->type; ++l);

		if (l < sizeof(g_param_string_map) /
		    sizeof(cpc_param_string_map_t)) {
			transient = (param->transient) ? " (t)" : "";

			if (param->data_type == CPC_WPDT_UINT)
				CPC_LOGUF("%sParameter: %s %d%s",depth_str,
					       g_param_string_map[l].string,
					       param->int_value, transient);
			else if (param->data_type == CPC_WPDT_UTF8)
				CPC_LOGUF("%sParameter: %s  \"%s\"",
					       depth_str,
					       g_param_string_map[l].string,
					       param->utf8_value,transient);
		}
	}

	for (i = 0; i < root->characteristics.size; ++i)
		prv_dump_characteristic((cpc_characteristic_t *)
				cpc_ptr_array_get(&root->characteristics, i),
				depth + 1);

	free(depth_buf);
}

#endif

static int prv_parse_characteristic(const char *prov_data,
				    int data_length,
				    cpc_characteristic_t *root)
{
	CPC_ERR_MANAGE;
	xmlTextReaderPtr reader_ptr = 0;
	int process_node_ret = CPC_ERR_NONE;
	int ret = 0;
	cpc_ptr_array_t char_stack;
	int depth_of_current_char = -1;

	cpc_ptr_array_make(&char_stack, 4, NULL);

	reader_ptr = xmlReaderForMemory(prov_data, data_length, "", NULL,
				       XML_PARSE_NOENT | XML_PARSE_NOBLANKS);

	if (!reader_ptr) {
		CPC_LOGF("Unable to create XMLReader");
		CPC_FAIL_FORCE(CPC_ERR_OOM);
	}

	CPC_FAIL(cpc_ptr_array_append(&char_stack, root));

	do {
		ret = xmlTextReaderRead(reader_ptr);
		if (ret == 1)
			process_node_ret =
				prv_process_node(reader_ptr, &char_stack,
						 &depth_of_current_char);
	} while (ret == 1 && process_node_ret == CPC_ERR_NONE);

	if (ret == 0 || ret == 1)
		CPC_ERR = process_node_ret;
	else if (xmlLastError.code == XML_ERR_NO_MEMORY)
		CPC_ERR = CPC_ERR_OOM;

	if (CPC_ERR != CPC_ERR_NONE)
		goto CPC_ON_ERR;

	CPC_FAIL(prv_validate_cpc_characteristic(root));
	CPC_FAIL(prv_perform_global_checks(root));

#ifdef CPC_LOGGING
	prv_dump_characteristic(root, 0);
#endif

CPC_ON_ERR:

	xmlFreeTextReader(reader_ptr);
	cpc_ptr_array_free(&char_stack);

#ifdef CPC_LOGGING
	if (CPC_ERR != CPC_ERR_NONE)
		CPC_LOGF("Unable to parse provisioning file");
#endif

	return CPC_ERR;
}

int cpc_characteristic_new(const char *prov_data, int data_length,
			   cpc_characteristic_t **characteristic)
{
	CPC_ERR_MANAGE;
	cpc_characteristic_t *root;

	CPC_FAIL_NULL(root, malloc(sizeof(cpc_characteristic_t)), CPC_ERR_OOM);

	prv_characteristic_make(root, CPC_CT_ROOT);

	CPC_FAIL(prv_parse_characteristic(prov_data, data_length, root));

	*characteristic = root;

	return CPC_ERR_NONE;

CPC_ON_ERR:

	prv_wap_char_free(root);

	return CPC_ERR;
}

void cpc_characteristic_delete(cpc_characteristic_t *characteristic)
{
	prv_wap_char_free(characteristic);
}

