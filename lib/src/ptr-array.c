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
 * @file <ptr-array.c>
 *
 * @brief Main source file for implementing a dynamic arrays.
 *
 * The functions in this file are based on functions taken from the original
 * ACCESS source file, omadm_dynbuf.c.  The algorithms have not been changed,
 * but the identifiers have been renamed and the code has been reformatted to
 * adhere to the cpclient's coding guidelines.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "error.h"
#include "ptr-array.h"

void cpc_ptr_array_make(cpc_ptr_array_t *array, unsigned int block_size,
					cpc_ptr_array_des_t destructor)
{
	array->size = array->max_size = 0;
	array->block_size = block_size;
	array->destructor = destructor;
	array->array = NULL;
}

void cpc_ptr_array_make_from(cpc_ptr_array_t *array, void **new_array,
				unsigned int size, unsigned int block_size,
				cpc_ptr_array_des_t destructor)
{
	array->size = array->max_size = size;
	array->block_size = block_size;
	array->destructor = destructor;
	array->array = new_array;
}

void cpc_ptr_array_adopt(cpc_ptr_array_t *array, void **carray,
			 unsigned int *size)
{
	*carray = array->array;
	*size = array->size;
	memset(array, 0, sizeof(*array));
}

void cpc_ptr_array_free(cpc_ptr_array_t *array)
{
	unsigned int i = 0;

	if (array->array == NULL)
		return;

	if (array->destructor)
		for (; i < array->size; ++i)
			if (array->array[i])
				array->destructor(array->array[i]);

	free(array->array);
	array->array = NULL;
}

void cpc_ptr_array_free_callback(void *array)
{
	if (array)
	{
		cpc_ptr_array_free((cpc_ptr_array_t *) array);
		free(array);
	}
}

int cpc_ptr_array_append(cpc_ptr_array_t *array, void *pointer)
{
	void *buffer = NULL;
	int ret_val = CPC_ERR_NONE;
	unsigned int new_max_size = 0;

	if (array->size == array->max_size) {
		new_max_size = array->size + array->block_size;

		buffer = realloc(array->array, new_max_size * sizeof(void *));
		if (buffer) {
			array->array = buffer;
			array->max_size = new_max_size;
		} else
			ret_val = CPC_ERR_OOM;
	}

	if (ret_val == CPC_ERR_NONE) {
		array->array[array->size] = pointer;
		++array->size;
	}

	return ret_val;
}

void cpc_ptr_array_delete(cpc_ptr_array_t *array, unsigned int index)
{
	if (index < array->size) {
		if (array->destructor && array->array[index])
			array->destructor(array->array[index]);

		for (; index < array->size - 1; ++index)
			array->array[index] = array->array[index + 1];

		--array->size;
	}
}
