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
 * @file <ptr-array.h>
 *
 * @brief Contains definitions for a dynamic array.
 *
 * This file is based on the original ACCESS file, omadm_dynbuf_prv.h.  The
 * structure and prototype names have be changed to adhere to the cpclient
 * coding guidelines.
 *
 *****************************************************************************/

#ifndef CPC_PTR_ARRAY_H
#define CPC_PTR_ARRAY_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

typedef void (*cpc_ptr_array_des_t) (void *);

typedef struct cpc_ptr_array_t_ cpc_ptr_array_t;

struct cpc_ptr_array_t_
{
	unsigned int size;
	unsigned int block_size;
	unsigned int max_size;
	void **array;
	cpc_ptr_array_des_t destructor;
};

void cpc_ptr_array_make(cpc_ptr_array_t *array,
			unsigned int block_size,
			cpc_ptr_array_des_t destructor);
void cpc_ptr_array_make_from(cpc_ptr_array_t *array,
			     void **new_array, unsigned int size,
			     unsigned int block_size,
			     cpc_ptr_array_des_t destructor);

void cpc_ptr_array_adopt(cpc_ptr_array_t *array, void **carray,
			 unsigned int *size);

void cpc_ptr_array_free(cpc_ptr_array_t *array);
void cpc_ptr_array_free_callback(void *array);
int cpc_ptr_array_append(cpc_ptr_array_t *array, void *pointer);
void cpc_ptr_array_delete(cpc_ptr_array_t *array, unsigned int index);

#define cpc_ptr_array_get(cpc_ptr_array, index) (cpc_ptr_array)->array[index]
#define cpc_ptr_array_set(cpc_ptr_array, index, object) \
	(cpc_ptr_array)->array[index] = (object)
#define cpc_ptr_array_get_size(cpc_ptr_array) (cpc_ptr_array)->size
#define cpc_ptr_array_get_array(cpc_ptr_array) (cpc_ptr_array)->array

#ifdef __cplusplus
}
#endif

#endif
