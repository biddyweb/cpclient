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
 * @file <dbus-error.c>
 *
 * @brief Contains function for mapping cpc errors to d-Bus errors
 *
 *****************************************************************************/

#include "config.h"

#include "dbus-error.h"
#include "error.h"

#define CPC_ERR_DBUS_OOM CPC_SERVICE".Error.OOM"
#define CPC_ERR_DBUS_TOO_LARGE CPC_SERVICE".Error.TooLarge"
#define CPC_ERR_DBUS_LOAD_FAILED CPC_SERVICE".Error.LoadFailed"
#define CPC_ERR_DBUS_PARSE_ERROR CPC_SERVICE".Error.ParseError"
#define CPC_ERR_DBUS_PROVISION_ERROR CPC_SERVICE".Error.ProvisionError"
#define CPC_ERR_DBUS_CANCEL CPC_SERVICE".Error.Cancelled"
#define CPC_ERR_DBUS_DIED CPC_SERVICE".Error.Died"
#define CPC_ERR_DBUS_UNKNOWN CPC_SERVICE".Error.Unknown"
#define CPC_ERR_DBUS_IO CPC_SERVICE".Error.IO"
#define CPC_ERR_DBUS_NOT_FOUND CPC_SERVICE".Error.NotFound"
#define CPC_ERR_DBUS_DENIED CPC_SERVICE".Error.Denied"
#define CPC_ERR_DBUS_IN_PROGRESS CPC_SERVICE".Error.InProgress"
#define CPC_ERR_DBUS_ALREADY_APPLIED CPC_SERVICE".Error.AlreadyApplied"

const gchar *const g_dbus_errors[] = {
	CPC_ERR_DBUS_UNKNOWN,
	CPC_ERR_DBUS_OOM,
	CPC_ERR_DBUS_PARSE_ERROR,
	CPC_ERR_DBUS_LOAD_FAILED,
	CPC_ERR_DBUS_LOAD_FAILED,
	CPC_ERR_DBUS_IO,
	CPC_ERR_DBUS_NOT_FOUND,
	CPC_ERR_DBUS_CANCEL,
	CPC_ERR_DBUS_DENIED,
	CPC_ERR_DBUS_DIED,
	CPC_ERR_DBUS_TOO_LARGE,
	CPC_ERR_DBUS_IN_PROGRESS,
	CPC_ERR_DBUS_ALREADY_APPLIED
};

const gchar *cpc_dbus_error_map(int cpc_error)
{
	const gchar *err = CPC_ERR_DBUS_UNKNOWN;

	if ((cpc_error > 0) &&
	    ((unsigned int) cpc_error <=
	     sizeof(g_dbus_errors) / sizeof(const gchar *const)))
		err = g_dbus_errors[cpc_error - 1];

	return err;
}
