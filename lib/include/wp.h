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
 * Copyright (c) 2009 ACCESS CO., LTD. All rights reserved.
 *****************************************************************************/
/*!
 * @file <wp.h>
 *
 * @brief OMA Client Provisionning WAP Push Handler
 *
 * This file is based on the original ACCESS file, omadm_cp_push_handler_prv.h.
 * Identifiers have been renamed to adhere to the cpclient coding guidelines.
 *
 ******************************************************************************/

#ifndef CPC_WP_H
#define CPC_WP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

typedef uint8_t cpc_sec_t;

#define CPC_SECURITY_NONE (cpc_sec_t) 0xFF
#define CPC_SECURITY_NETWPIN (cpc_sec_t) 0x00
#define CPC_SECURITY_USERPIN (cpc_sec_t) 0x01
#define CPC_SECURITY_USERNETWPIN (cpc_sec_t) 0x02
#define CPC_SECURITY_USERPINMAC (cpc_sec_t) 0x03

typedef struct cpc_wp_t_ cpc_wp_t;

int cpc_wp_new(const uint8_t *data, size_t length, cpc_wp_t **context);
void cpc_wp_delete(cpc_wp_t *context);
cpc_sec_t cpc_wp_security(cpc_wp_t *context);
int cpc_authenticate(const cpc_wp_t *context, const char *imsi,
		     const char *pin);
int cpc_get_prov_doc(const cpc_wp_t *context, char **xml,
		     unsigned int* xml_size);

#ifdef __cplusplus
}
#endif

#endif
