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
 * @file <wbxml-libwbxml.c>
 *
 * @brief Contains functions that implement wbxml functionality using libwbxml2
 *
 ******************************************************************************/

#include "config.h"

#ifdef HAVE_NEW_WBXML
#include <wbxml/wbxml.h>
#else
#include <wbxml.h>
#endif

#include "wbxml-peer.h"
#include "error.h"
#include "error-macros.h"
#include "log.h"

#ifdef HAVE_NEW_WBXML
int cpc_wbxml_to_xml(const uint8_t *wbxml, unsigned int wbxml_size,
		     char **xml, unsigned int *xml_size)
{
	CPC_ERR_MANAGE;
	WBXMLConvWBXML2XML *conv = NULL;
	WB_UTINY *xml_tmp = NULL;
	WB_ULONG xml_len;

	if (wbxml_conv_wbxml2xml_create(&conv) != WBXML_OK) {
		CPC_LOGF("Unable to create WXBML converter");
		CPC_FAIL_FORCE(CPC_ERR_UNKNOWN);
	}

	if (wbxml_conv_wbxml2xml_run(conv, (WB_UTINY*) wbxml, wbxml_size,
				     &xml_tmp, &xml_len) != WBXML_OK) {
		CPC_LOGF("Failed to convert WAP Push payload into XML");
		CPC_FAIL_FORCE(CPC_ERR_UNKNOWN);
	}

	*xml_size = (unsigned int) xml_len;
	*xml = (char *) xml_tmp;

CPC_ON_ERR:

	if (conv)
		wbxml_conv_wbxml2xml_destroy(conv);

	return CPC_ERR;
}
#else
int cpc_wbxml_to_xml(const uint8_t *wbxml, unsigned int wbxml_size,
		     char **xml, unsigned int *xml_size)
{
	CPC_ERR_MANAGE;
	WB_UTINY *xml_tmp = NULL;
	WB_ULONG xml_len;
	WBXMLGenXMLParams params;

	memset(&params, 0, sizeof(params));
	params.gen_type = WBXML_GEN_XML_INDENT;

	if (wbxml_conv_wbxml2xml_withlen((WB_UTINY*) wbxml, wbxml_size,
					 &xml_tmp, &xml_len, &params)
	    != WBXML_OK) {
		CPC_LOGF("Failed to convert WAP Push payload into XML");
		CPC_FAIL_FORCE(CPC_ERR_UNKNOWN);
	}

	*xml_size = (unsigned int) xml_len;
	*xml = (char *) xml_tmp;

CPC_ON_ERR:

	return CPC_ERR;
}
#endif

