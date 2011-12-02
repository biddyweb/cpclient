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
 * @file <wp.c>
 *
 * @brief OMA Client WAP Push Handler parser
 *
 * The function cpc_get_prov_doc is new Intel code.
 *
 * All the other functions in this file come from the original ACCESS file,
 * omadm_cp_push_handler.c.  The identifiers used in these functions have been
 * renamed and their formatting has been changed to match the cpclient coding
 * guidelines.
 *
 ******************************************************************************/

#include "config.h"

#include <string.h>
#include <stdio.h>

#include "error.h"
#include "error-macros.h"
#include "log.h"

#include "wbxml-peer.h"
#include "hmac-peer.h"

#include "wp.h"

#define WSP_PUSH_CP_CONTENT_TYPE		0xB6
#define WSP_MAX_UINTVAR_BYTES			5
#define WSP_PUSH_PDU_TYPE			0x06
#define WSP_PUSH_SEC_FIELD_NAME			0x91
#define WSP_PUSH_MAC_FIELD_NAME			0x92
#define WBXML_ALLOC_BLOC_SIZE			1024
#define IMSI_LENGTH				8

struct cpc_wp_t_ {
	uint8_t *message;
	cpc_sec_t sec;
	const char *mac;
	const uint8_t *body;
	size_t body_len;
};

static int prv_read_uintvar(const uint8_t **buffer, uint32_t *value)
{
	CPC_ERR_MANAGE;

	uint8_t byteNum = 0;
	const uint8_t *uintvar = *buffer;

	*value = 0;

	while ((*uintvar & 0x80) && (byteNum < WSP_MAX_UINTVAR_BYTES)) {
		byteNum++;
		*value |= (uint32_t) (*uintvar & 0x7F);
		*value <<= 7;
		uintvar++;
	}

	if (byteNum == WSP_MAX_UINTVAR_BYTES) {
		*value = 0;
		CPC_FAIL_FORCE(CPC_ERR_CORRUPT);
	}

	*value |= (uint32_t) (*uintvar & 0x7F);
	uintvar++;
	*buffer = uintvar;

CPC_ON_ERR:

	return CPC_ERR;
}

void cpc_wp_delete(cpc_wp_t *context)
{
	if (context) {
		free(context->message);
		free(context);
	}
}

int cpc_wp_new(const uint8_t *data, size_t length,
	       cpc_wp_t **context)
{
	CPC_ERR_MANAGE;
	const uint8_t *curPos;
	uint32_t headerLen;
	uint32_t contentTypeValueLen = 0;
	cpc_wp_t *ctx;

	CPC_LOGB(data, length);

	CPC_FAIL_NULL(ctx, malloc(sizeof(*ctx)), CPC_ERR_OOM);
	CPC_FAIL_NULL(ctx->message, malloc(length), CPC_ERR_OOM);

	memcpy(ctx->message, data, length);
	curPos = ctx->message;

	ctx->sec = CPC_SECURITY_NONE;
	ctx->mac = NULL;
	ctx->body = NULL;
	ctx->body_len = 0;

	/* Skip TID - Transaction ID */

	curPos++;

	/* Read PDU type */

	if (*curPos != WSP_PUSH_PDU_TYPE) {
		CPC_LOGF("Error: Bad PDU Type 0x%02X", *curPos);

		CPC_FAIL(CPC_ERR_CORRUPT);
	}

	curPos++;

	/* Read headers length, including content type */

	CPC_FAIL(prv_read_uintvar(&curPos, &headerLen));
	CPC_LOGF("Info: Header len %d", headerLen);

	/* Read content type value length if headerLen > 1 */

	if (headerLen > 1) {
		if (*curPos == 0x1F) { /* uintvar */
			curPos++;
			CPC_FAIL(prv_read_uintvar(&curPos,
						       &contentTypeValueLen));
		}
		else { /* short int */
			curPos++;
			contentTypeValueLen = *curPos & 0x7F;
			curPos++;
		}

		/* Remove content type len */

		contentTypeValueLen--;

		CPC_LOGF("Info: Content Type Value len %d",
				contentTypeValueLen);
	}

	/* Read content type */

	if (*curPos != WSP_PUSH_CP_CONTENT_TYPE) {
		CPC_LOGF("Error: Bad Content Type 0x%02X", *curPos);
		CPC_FAIL_FORCE(CPC_ERR_CORRUPT);
	}

	curPos++;

	/*
	 * Read headers
	 * Warning: simple SEC & MAC parser...
	 */

	if (contentTypeValueLen > 0) {
		/* Read SEC */

		if (*curPos == WSP_PUSH_SEC_FIELD_NAME) {
			if (contentTypeValueLen < 2)
				CPC_FAIL(CPC_ERR_CORRUPT);
			curPos++;

			ctx->sec = (cpc_sec_t) (*curPos - 128);
			curPos++;

			contentTypeValueLen -= 2;
			CPC_LOGF("Info: SEC 0x%02X", ctx->sec);
		}

		/* Read MAC */

		if (contentTypeValueLen > 0) {
			if (*curPos == WSP_PUSH_MAC_FIELD_NAME) {
				curPos++;

				ctx->mac = (char *) curPos;
				CPC_LOGF("Info: MAC %s", ctx->mac);

				/* Now points to actual CP document */

				curPos += strlen(ctx->mac) + 1;
			}
		}
	}

	ctx->body = curPos;
	ctx->body_len = length - (curPos - ctx->message);
	*context = ctx;

	CPC_LOGF("Info: Body length %d", ctx->body_len);

	return CPC_ERR_NONE;

CPC_ON_ERR:

	cpc_wp_delete(ctx);

	return CPC_ERR;
}

cpc_sec_t cpc_wp_security(cpc_wp_t *context)
{
	return context->sec;
}

static void prv_hex_to_hex_str(char *dest_str, const uint8_t *src,
			       size_t src_len)
{
	size_t pos;

	for (pos = 0; pos < src_len; pos++) {
		sprintf(dest_str, "%02X", src[pos]);
		dest_str+=2;
	}

	*dest_str = 0;
}

static int prv_str_imsi_to_hex_imsi(uint8_t *dest_buf, const char *src,
				    size_t *dest_len)
{
	CPC_ERR_MANAGE;
	char *str = NULL;
	size_t pos;
	uint8_t tmp;
	unsigned int src_len;
	unsigned int encoded_len;
	unsigned int str_len;

	src_len = strlen(src);

	if (src_len > 15)
		CPC_FAIL_FORCE(CPC_ERR_CORRUPT);

	encoded_len = (src_len >> 1) + 1;

	str_len = encoded_len << 1;
	CPC_FAIL_NULL(str, malloc(str_len + 1), CPC_ERR_OOM);

	/* Pad according to GSM 11.11 */

	if ((src_len & 1) == 0) {
		str[str_len - 1] = 'F';
		str[0] = '1';
	} else {
		str[0] = '9';
	}
	str[str_len] = 0;
	memcpy(str + 1, src, src_len);

	CPC_LOGF("convert %s", str);

	/* Convert */

	for (pos = 0; pos < encoded_len; pos++) {
		sscanf(str + (pos * 2), "%02hhx", &tmp);
		*(dest_buf + pos) = ((tmp & 0x0F) << 4) | ((tmp & 0xF0) >> 4);
	}

	CPC_LOGB(dest_buf, encoded_len);

	*dest_len = encoded_len;

CPC_ON_ERR:

	free(str);

	return CPC_ERR;
}


int cpc_authenticate(const cpc_wp_t *context, const char *imsi,
		     const char *pin)
{
	CPC_ERR_MANAGE;

	uint8_t *imsi_buffer = NULL;
	const uint8_t *key;
	size_t len;
	size_t pin_len;
	void *hmac_buffer = NULL;
	size_t hmac_buffer_len = 0;
	char *hmac_text = NULL;
	unsigned int i;

	CPC_LOGF("Info: Security type 0x%02X", context->sec);

	if (context->sec == CPC_SECURITY_NONE)
		goto CPC_ON_ERR;

	switch (context->sec) {
	case CPC_SECURITY_NETWPIN:
		if ((imsi == NULL) || (*(imsi) == 0))
			CPC_FAIL(CPC_ERR_DENIED);

		CPC_LOGF("Info: IMSI <%s>", imsi);

		CPC_FAIL_NULL(imsi_buffer, malloc(IMSI_LENGTH),
			      CPC_ERR_OOM);
		CPC_FAIL(prv_str_imsi_to_hex_imsi(imsi_buffer, imsi,
							&len));

		CPC_LOGF("Info: len %d", len);

		key = imsi_buffer;
		break;

	case CPC_SECURITY_USERPIN:
		if ((pin == NULL) || (*pin == 0))
			CPC_FAIL(CPC_ERR_DENIED);

		CPC_LOGF("Info: PIN <%s>", pin);

		len = strlen(pin);
		key = (uint8_t*) pin;
		break;

	case CPC_SECURITY_USERNETWPIN:
		if ((imsi == NULL) || (*(imsi) == 0))
			CPC_FAIL(CPC_ERR_DENIED);

		CPC_LOGF("Info: IMSI <%s>", imsi);

		if ((pin == NULL) || (*pin == 0))
			CPC_FAIL(CPC_ERR_DENIED);

		CPC_LOGF("Info: PIN <%s>", pin);

		pin_len = strlen(pin);
		CPC_FAIL_NULL(imsi_buffer, malloc(IMSI_LENGTH + pin_len + 1),
			      CPC_ERR_OOM);
		CPC_FAIL(prv_str_imsi_to_hex_imsi(imsi_buffer, imsi, &len));
		strncpy((char *) imsi_buffer + len, pin, pin_len);
		len += pin_len;
		key = (uint8_t *) imsi_buffer;

		break;

	case CPC_SECURITY_USERPINMAC:
		if (!pin)
			CPC_FAIL_FORCE(CPC_ERR_DENIED);

		len = strlen(pin);
		if ((len & 1) || (len < 10))
			CPC_FAIL_FORCE(CPC_ERR_DENIED);

		CPC_LOGF("Info: PIN <%s>", pin);

		len = len >> 1;
		CPC_FAIL_NULL(imsi_buffer, malloc(len + 1), CPC_ERR_OOM);
		memcpy(imsi_buffer, pin, len);
		imsi_buffer[len] = 0;
		key = (uint8_t *) imsi_buffer;
		break;
	default:
		CPC_FAIL_FORCE(CPC_ERR_DENIED);

		break;
	}

	CPC_FAIL(cpc_hmac_compute(key, len, context->body, context->body_len,
				  &hmac_buffer, &hmac_buffer_len));

	CPC_LOGF("Info: HMAC - len %d:", hmac_buffer_len);

	if (context->sec == CPC_SECURITY_USERPINMAC) {
		if (hmac_buffer_len < len)
			CPC_FAIL_FORCE(CPC_ERR_DENIED);

		for (i = 0; i < len; ++i) {
			if (pin[i + len] != (((uint8_t *)hmac_buffer)[i] % 10)
			    + 48)
				CPC_FAIL_FORCE(CPC_ERR_DENIED);
		}
	} else {
		CPC_FAIL_NULL(hmac_text, malloc((hmac_buffer_len << 1) + 1),
			      CPC_ERR_OOM);

		prv_hex_to_hex_str(hmac_text, hmac_buffer, hmac_buffer_len);

		CPC_LOGF("Info: Computed Mac : %s", hmac_text);
		CPC_LOGF("Info: Receieved Mac: %s", context->mac);


		if (strcmp(context->mac, hmac_text))
			CPC_FAIL_FORCE(CPC_ERR_DENIED);
	}

CPC_ON_ERR:

	free(hmac_text);
	free(hmac_buffer);
	free(imsi_buffer);

	return CPC_ERR;
}

int cpc_get_prov_doc(const cpc_wp_t *context, char **xml,
		     unsigned int *xml_size)
{
	return cpc_wbxml_to_xml(context->body, context->body_len, xml,
				xml_size);
}
