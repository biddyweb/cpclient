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
 * @file <wbxml-peer.h>
 *
 * @brief Prototype functions for wbxml functionality
 *
 ******************************************************************************/

#ifndef CPC_WBXML_PEER_H__
#define CPC_WBXML_PEER_H__

#include <stdint.h>

int cpc_wbxml_to_xml(const uint8_t *wbxml, unsigned int wbxml_size,
		     char **xml, unsigned int *xml_size);

#endif
