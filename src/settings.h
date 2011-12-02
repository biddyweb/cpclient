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
 * @file <settings.h>
 *
 * @brief Contains prototypes for converting an cpc_context_t into a set of
 * provman settings.
 *
 *****************************************************************************/

#ifndef CPC_SETTINGS_H
#define CPC_SETTINGS_H

typedef struct cpc_settings_t_ cpc_settings_t;
struct cpc_settings_t_ {
	GHashTable *system_settings;
	GPtrArray *system_meta;
	GHashTable *session_settings;
	GPtrArray *session_meta;
};

typedef struct cpc_meta_prop_t_ cpc_meta_prop_t;
struct cpc_meta_prop_t_ {
	gchar *key;
	gchar *prop;
	gchar *value;
};

void cpc_settings_init(cpc_settings_t *settings, cpc_context_t *context);
void cpc_settings_free(cpc_settings_t *settings);

#endif
