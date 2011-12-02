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
 * @file <errors.h>
 *
 * @brief Error codes
 *
 ******************************************************************************/

#ifndef CPC_ERRORS_H
#define CPC_ERRORS_H

enum cpc_generic_errors
{
	CPC_ERR_NONE = 0,
	CPC_ERR_UNKNOWN,
	CPC_ERR_OOM,
	CPC_ERR_CORRUPT,
	CPC_ERR_OPEN,
	CPC_ERR_READ,
	CPC_ERR_IO,
	CPC_ERR_NOT_FOUND,
	CPC_ERR_CANCELLED,
	CPC_ERR_DENIED,
	CPC_ERR_DIED,
	CPC_ERR_TOO_LARGE,
	CPC_ERR_IN_PROGRESS,
	CPC_ERR_ALREADY_APPLIED
};

#endif
