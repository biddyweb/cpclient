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
 * @file <characteristic.h>
 *
 * @brief Private definitions for the OMA CP data model
 *
 * This file is based on the ACCESS source file omadm_parser_cp_prv.h.  All
 * identifiers have been renamed by Intel to match the coding standards of the
 * cpclient.
 *
 *****************************************************************************/

#ifndef CPC_CHARACTERISTIC_H__
#define CPC_CHARACTERISTIC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <libxml/xmlreader.h>
#include <libxml/xmlerror.h>

#include <stdint.h>

#include "ptr-array.h"

enum cpc_characteristic_type_t_ {
	CPC_CT_ACCESS,
	CPC_CT_APPADDR,
	CPC_CT_APPAUTH,
	CPC_CT_APPLICATION,
	CPC_CT_BOOTSTRAP,
	CPC_CT_CLIENTIDENTITY,
	CPC_CT_NAPAUTHINFO,
	CPC_CT_NAPDEF,
	CPC_CT_PORT,
	CPC_CT_PXAUTHINFO,
	CPC_CT_PXLOGICAL,
	CPC_CT_PXPHYSICAL,
	CPC_CT_RESOURCE,
	CPC_CT_ROOT,	/* Not part of CP specs. */
	CPC_CT_VALIDITY,
	CPC_CT_VENDORCONFIG,
	CPC_CT_MAX
};

typedef enum cpc_characteristic_type_t_ cpc_characteristic_type_t;

enum cpc_param_type_t_ {
	CPC_PT_AACCEPT,
	CPC_PT_AAUTHDATA,
	CPC_PT_AAUTHLEVEL,
	CPC_PT_AAUTHNAME,
	CPC_PT_AAUTHSECRET,
	CPC_PT_AAUTHTYPE,
	CPC_PT_ADDR,
	CPC_PT_ADDRTYPE,
	CPC_PT_APPID,
	CPC_PT_APROTOCOL,
	CPC_PT_AUTHNAME,
	CPC_PT_AUTHSECRET,
	CPC_PT_AUTHTYPE,
	CPC_PT_AUTH_ENTITY,
	CPC_PT_AUTO_DOWNLOAD,
	CPC_PT_BASAUTH_ID,
	CPC_PT_BASAUTH_PW,
	CPC_PT_BEARER,
	CPC_PT_CALLTYPE,
	CPC_PT_CIDPREFIX,
	CPC_PT_CLIENT_ID,
	CPC_PT_CLIURI,
	CPC_PT_CM,
	CPC_PT_CONTEXT_ALLOW,
	CPC_PT_COUNTRY,
	CPC_PT_DELIVERY_ERR_SDU,
	CPC_PT_DELIVERY_ORDER,
	CPC_PT_DELIVERY_TIME,
	CPC_PT_DNLINKSPEED,
	CPC_PT_DNS_ADDR,
	CPC_PT_DOMAIN,
	CPC_PT_EXPIRY_TIME,
	CPC_PT_FIRST_RETRY_TIMEOUT,
	CPC_PT_FROM,
	CPC_PT_GUARANTEED_BITRATE_DNLINK,
	CPC_PT_GUARANTEED_BITRATE_UPLINK,
	CPC_PT_IMAGE,
	CPC_PT_INIT,
	CPC_PT_INTERNET,
	CPC_PT_LINGER,
	CPC_PT_LINKSPEED,
	CPC_PT_LOCAL_ADDR,
	CPC_PT_LOCAL_ADDRTYPE,
	CPC_PT_MASTER,
	CPC_PT_MAX_BITRATE_DNLINK,
	CPC_PT_MAX_BITRATE_UPLINK,
	CPC_PT_MAX_NUM_RETRY,
	CPC_PT_MAX_SDU_SIZE,
	CPC_PT_MS,
	CPC_PT_Ma,
	CPC_PT_NAME,
	CPC_PT_NAPID,
	CPC_PT_NAP_ADDRESS,
	CPC_PT_NAP_ADDRTYPE,
	CPC_PT_NETWORK,
	CPC_PT_PC_ADDR,
	CPC_PT_PHYSICAL_PROXY_ID,
	CPC_PT_POP_BEFORE_SMTP,
	CPC_PT_PORTNBR,
	CPC_PT_PPGAUTH_TYPE,
	CPC_PT_PRIORITY,
	CPC_PT_PROVIDER_ID,
	CPC_PT_PROVURL,
	CPC_PT_PROXY_ID,
	CPC_PT_PROXY_PROVIDER_ID,
	CPC_PT_PROXY_PW,
	CPC_PT_PULLENABLED,
	CPC_PT_PUSHENABLED,
	CPC_PT_PXADDR,
	CPC_PT_PXADDRTYPE,
	CPC_PT_PXADDR_FQDN,
	CPC_PT_PXAUTH_ID,
	CPC_PT_PXAUTH_PW,
	CPC_PT_PXAUTH_TYPE,
	CPC_PT_REQUEST_DELIVERY,
	CPC_PT_REQUEST_READ,
	CPC_PT_REREG_THRESHOLD,
	CPC_PT_RESIDUAL_BER,
	CPC_PT_RM,
	CPC_PT_ROAMING,
	CPC_PT_RULE,
	CPC_PT_SAVE_SENT,
	CPC_PT_SDU_ERROR_RATIO,
	CPC_PT_SENDER_VISIBLE,
	CPC_PT_SENT_FOLDER,
	CPC_PT_SERVICE,
	CPC_PT_SERVICES,
	CPC_PT_SID,
	CPC_PT_SOC,
	CPC_PT_SPI,
	CPC_PT_STARTPAGE,
	CPC_PT_SYNCTYPE,
	CPC_PT_TO_NAPID,
	CPC_PT_TO_PROXY,
	CPC_PT_TRAFFIC_CLASS,
	CPC_PT_TRAFFIC_HANDL_PRIO,
	CPC_PT_TRANSFER_DELAY,
	CPC_PT_TRASH_FOLDER,
	CPC_PT_TRUST,
	CPC_PT_T_BIT,
	CPC_PT_URI,
	CPC_PT_VALIDUNTIL,
	CPC_PT_WSP_VERSION
};

typedef enum cpc_param_type_t_ cpc_param_type_t;

enum cpc_param_data_type_t_ {
	CPC_WPDT_NONE,
	CPC_WPDT_UINT,
	CPC_WPDT_UTF8,
	CPC_WPDT_UTF8OPT,
	CPC_WPDT_UINTHEX
};

typedef enum cpc_param_data_type_t_ cpc_param_data_type_t;

typedef struct cpc_parameter_t_ cpc_parameter_t;
struct cpc_parameter_t_ {
	cpc_param_type_t type;
	cpc_param_data_type_t data_type;
	bool transient;
	union {
		unsigned int int_value;
		xmlChar *utf8_value;
	};
};

typedef struct cpc_characteristic_t_ cpc_characteristic_t;
struct cpc_characteristic_t_ {
	cpc_characteristic_type_t type;
	cpc_ptr_array_t parameters;
	cpc_ptr_array_t characteristics;
};


/*!
 * @brief deletes an cpc_characteristic
 *
 * @param characteristic Characteristic to delete
 */

void cpc_characteristic_delete(cpc_characteristic_t *characteristic);

/*!
 * @brief Parses an OMA CP XML document and generates an in memory model of the
 * document. This function applies almost all of the validity checks to the
 * document as described in ProvCont and ProvUAB.
 *
 * @param prov_data a pointer to an in memory document.
 * @param data_length length in bytes of the in memory XML document
 * @param characteristic The in memory model is returned via this parameter,
 * if the function succeeds.  The caller needs to delete this model by calling
 * cpc_characteristic_delete when it is finished with it.
 *
 * @return CPC_ERR_NONE The document was correctly parsed and an in
 * memory representation of the model is pointed to by characteristic.
 * @return CPC_ERR_OOM The document could not be parsed correctly due to
 * an OOM.
 * @return CPC_ERR_CORRUPT The document is corrupt.  This is either
 * because it is not a valid XML document or because it does not contain any
 * valid OMA CP characteristics.
 */

int cpc_characteristic_new(const char *prov_data, int data_length,
			   cpc_characteristic_t **characteristic);



/*!
 * @brief Returns the number of parameters defined for a given characteristic
 *
 * @param characteristic Characteristic
 *
 * @return The number of parameters defined for iChar
 */

#define cpc_get_param_count(characteristic) \
	cpc_ptr_array_get_size(&(characteristic)->parameters)

/*!
 * @brief Retrieves a Parameter from a given characteristic
 *
 * @param characteristic Characteristic
 * @param index The parameter to return.
 *
 * @return The requested parameter
 */

#define cpc_get_param(characteristic,index) \
	((cpc_parameter_t*) \
	 cpc_ptr_array_get(&(characteristic)->parameters,index))

/*!
 * @brief Returns the number of child characteristics owned by a given
 * characteristic
 *
 * @param characteristic Characteristic
 *
 * @return The number child characteristics owned by iChar
 */

#define cpc_get_char_count(characteristic) \
	cpc_ptr_array_get_size(&(characteristic)->characteristics)

/*!
 * @brief Retrieves a child charactersitic from a given characteristic
 *
 * @param characteristic Characteristic
 * @param index The characteristic to return.
 *
 * @return The requested characteristic
 */

#define cpc_get_char(characteristic,index) \
	((cpc_characteristic_t*)\
	 cpc_ptr_array_get(&(characteristic)->characteristics,index))

/*!
 * @brief Searches the given characteristic for a parameter of a given type.
 *
 * @param characteristic Characteristic to search
 * @param param The of the parameter to search for
 * @param index The posisition from which to begin the search.
 *              E.g., a value of 5 means start checking from the 5th parameter.
 *
 * @return -1 indicates that a parameter of the specified type cannot be found
 * @return >=0 the index of the parameter.
 */

int cpc_find_param(cpc_characteristic_t *characteristic,
		   cpc_param_type_t param, int index);

/*!
 * @brief Searches the given characteristic for a characteristic of
 *        a given type.
 *
 * @param characteristic Characteristic to search
 * @param type The of the parameter to search for
 * @param index The posisition from which to begin the search.
 *              E.g., a value of 5 means start checking from the 5th
 *              characteristic.
 *
 * @return -1 indicates that a parameter of the specified type cannot be found
 * @return >=0 the index of the parameter.
 */

int cpc_find_char(cpc_characteristic_t *characteristic,
		  cpc_characteristic_type_t type, int index);

#ifdef __cplusplus
}				// extern "C"
#endif

#endif
