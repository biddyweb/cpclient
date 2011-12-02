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
 * @file <hmac-gnutls.c>
 *
 * @brief Contains a function that computes a SHA1 HMAC using gnutls
 *
 ******************************************************************************/

#include "config.h"

#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>

#include <stdlib.h>

#include "hmac-peer.h"
#include "error.h"
#include "error-macros.h"

int cpc_hmac_compute(const uint8_t *key, size_t key_len, const uint8_t *data,
		     size_t data_len, void **hmac, size_t *hmac_buffer_len)
{
	CPC_ERR_MANAGE;

	void *hmac_buffer = NULL;
	int hmac_buffer_size;

	if (gnutls_global_init() != GNUTLS_E_SUCCESS)
		CPC_FAIL_FORCE_LABEL(CPC_ERR_DENIED, no_deinit);

	hmac_buffer_size = gnutls_hmac_get_len(GNUTLS_MAC_SHA1);

	if (!hmac_buffer_size)
		CPC_FAIL_FORCE(CPC_ERR_DENIED);

	CPC_FAIL_NULL(hmac_buffer, malloc(hmac_buffer_size),
			   CPC_ERR_OOM);

	if (gnutls_hmac_fast(GNUTLS_MAC_SHA1, key, key_len, data, data_len,
				hmac_buffer) < 0)
		CPC_FAIL(CPC_ERR_DENIED);

	*hmac = hmac_buffer;
	*hmac_buffer_len =  hmac_buffer_size;

CPC_ON_ERR:

	gnutls_global_deinit();

no_deinit:

	return CPC_ERR;
}

