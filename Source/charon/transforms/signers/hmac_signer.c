/**
 * @file hmac_signer.c
 * 
 * @brief Implementation of hmac_signer_t.
 * 
 */

/*
 * Copyright (C) 2005 Jan Hutter, Martin Willi
 * Hochschule fuer Technik Rapperswil
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include "hmac_signer.h"

#include <utils/allocator.h>
#include <transforms/prfs/hmac_prf.h>

/**
 * This class represents a hmac signer with 12 byte (96 bit) output
 */
#define BLOCK_SIZE	12

typedef struct private_hmac_signer_t private_hmac_signer_t;

/**
 * private data structure with signing context.
 */
struct private_hmac_signer_t {
	/**
	 * Public interface for this signer.
	 */
	hmac_signer_t public;
	
	/*
	 * Assigned hmac function.
	 */
	prf_t *hmac_prf;
};


static void get_signature (private_hmac_signer_t *this, chunk_t data, u_int8_t *buffer)
{
	u_int8_t full_mac[this->hmac_prf->get_block_size(this->hmac_prf)];
	
	this->hmac_prf->get_bytes(this->hmac_prf,data,full_mac);

	/* copy mac aka signature :-) */
	memcpy(buffer,full_mac,BLOCK_SIZE);
}

static void allocate_signature (private_hmac_signer_t *this, chunk_t data, chunk_t *chunk)
{
	chunk_t signature;
	u_int8_t full_mac[this->hmac_prf->get_block_size(this->hmac_prf)];
	
	this->hmac_prf->get_bytes(this->hmac_prf,data,full_mac);

	signature.ptr = allocator_alloc(BLOCK_SIZE);
	signature.len = BLOCK_SIZE;
	
	/* copy mac aka signature :-) */
	memcpy(signature.ptr,full_mac,BLOCK_SIZE);

	*chunk = signature;
}

static void verify_signature (private_hmac_signer_t *this, chunk_t data, chunk_t signature, bool *valid)
{
	u_int8_t full_mac[this->hmac_prf->get_block_size(this->hmac_prf)];
	
	this->hmac_prf->get_bytes(this->hmac_prf,data,full_mac);
	
	if (signature.len != BLOCK_SIZE)
	{
		*valid = FALSE;
		return;
	}
	
	/* compare mac aka signature :-) */
	if (memcmp(signature.ptr,full_mac,BLOCK_SIZE) == 0)
	{
		*valid = TRUE;
	}
	else
	{
		*valid = FALSE;
	}
}

static size_t get_key_size (private_hmac_signer_t *this)
{
	return this->hmac_prf->get_block_size(this->hmac_prf);
}
	
static size_t get_block_size (private_hmac_signer_t *this)
{
	return BLOCK_SIZE;
}
	
static void set_key (private_hmac_signer_t *this, chunk_t key)
{
	this->hmac_prf->set_key(this->hmac_prf,key);
}

/**
 * implementation of signer_t.destroy.
 */
static status_t destroy(private_hmac_signer_t *this)
{
	this->hmac_prf->destroy(this->hmac_prf);
	allocator_free(this);
	return SUCCESS;
}


/*
 * Described in header
 */
hmac_signer_t *hmac_signer_create(hash_algorithm_t hash_algoritm)
{
	private_hmac_signer_t *this = allocator_alloc_thing(private_hmac_signer_t);

	this->hmac_prf = (prf_t *) hmac_prf_create(hash_algoritm);
	
	if (this->hmac_prf == NULL)
	{
		/* algorithm not supported */
		allocator_free(this);
		return NULL;
	}
	
	/* interface functions */
	this->public.signer_interface.get_signature = (void (*) (signer_t*, chunk_t, u_int8_t*))get_signature;
	this->public.signer_interface.allocate_signature = (void (*) (signer_t*, chunk_t, chunk_t*))allocate_signature;
	this->public.signer_interface.verify_signature = (void (*) (signer_t*, chunk_t, chunk_t,bool *))verify_signature;
	this->public.signer_interface.get_key_size = (size_t (*) (signer_t*))get_key_size;
	this->public.signer_interface.get_block_size = (size_t (*) (signer_t*))get_block_size;
	this->public.signer_interface.set_key = (void (*) (signer_t*,chunk_t))set_key;
	this->public.signer_interface.destroy = (void (*) (signer_t*))destroy;
	
	return &(this->public);
}
