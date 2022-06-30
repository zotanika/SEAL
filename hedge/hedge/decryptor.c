
#include "decryptor.h"
#include "lib/common.h"
#include "math/uintcore.h"
#include "math/uintarith.h"
#include "math/uintarithmod.h"
#include "math/uintarithsmallmod.h"
#include "math/polycore.h"
#include "math/polyarithmod.h"
#include "math/polyarithsmallmod.h"
#include "valcheck.h"

static void decryptor_init(Decryptor* self, hedge_context_t* context, const SecretKey* secret_key)
{
	self->context_ = context;

	// Verify parameters
	if (!self->context_)
	{
		invalid_argument("invalid context");
	}
	if (!self->context_->is_parms_set(self->context_))
	{
		invalid_argument("encryption parameters are not set correctly");
	}
	if (!EQUALS(secret_key->parms_id((SecretKey *)secret_key), &self->context_->key_parms_id))
	{
		invalid_argument("secret key is not valid for encryption parameters");
	}

	ctxdata_t* key_ctxdata = self->context_->key_ctxdata(self->context_);
	encrypt_parameters* parms = key_ctxdata->parms;
	vector(Zmodulus)* coeff_modulus = parms->coeff_modulus(parms);
	size_t coeff_count = parms->poly_modulus_degree(parms);
	size_t coeff_mod_count = coeff_modulus->size;

	// Set the secret_key_array to have size 1 (first power of secret)
	// and copy over data
	self->secret_key_array_ = allocate_poly(coeff_count, coeff_mod_count);
	set_poly_poly_simple(secret_key->sk_->data(secret_key->sk_), coeff_count, coeff_mod_count,
		self->secret_key_array_);
	self->secret_key_array_size_ = 1;
}

static void decrypt(Decryptor* self, const Ciphertext* encrypted, Plaintext* destination)
{
	log_trace("decrypt encrypted->scale=%zu\n", encrypted->scale_);
	// Verify that encrypted is valid.
	if (!is_valid_for(Ciphertext, encrypted, self->context_))
	{
		invalid_argument("encrypted is not valid for encryption parameters");
	}

	ctxdata_t* context_data = self->context_->first_ctxdata(self->context_);
	encrypt_parameters* parms = context_data->parms;
	
	switch (parms->scheme(parms))
	{
	case CKKS:
		self->ckks_decrypt(self, encrypted, destination);
		return;

	default:
		invalid_argument("unsupported scheme");
	}
}

void ckks_decrypt(Decryptor* self, const Ciphertext *encrypted,
        Plaintext *destination)
{
	if (!encrypted->is_ntt_form((Ciphertext *)encrypted))
	{
		invalid_argument("encrypted must be in NTT form");
	}
	log_trace("ckks_decrrypt encrypted->scale=%zu\n", encrypted->scale_);
	// We already know that the parameters are valid
	ctxdata_t* context_data = self->context_->get_ctxdata(
		self->context_, encrypted->parms_id((Ciphertext *)encrypted));
	encrypt_parameters* parms = context_data->parms;
	vector(Zmodulus)* coeff_modulus = parms->coeff_modulus(parms);
	size_t coeff_count = parms->poly_modulus_degree_;
	size_t coeff_mod_count = coeff_modulus->size;
	size_t rns_poly_uint64_count = mul_safe(coeff_count, coeff_mod_count);

	ctxdata_t* key_ctxdata = self->context_->key_ctxdata(self->context_);
	encrypt_parameters* key_parms = key_ctxdata->parms;
	size_t key_rns_poly_uint64_count = mul_safe(coeff_count,
		key_parms->coeff_modulus_->size);
	size_t encrypted_size = encrypted->size_;

	// Make sure we have enough secret key powers computed
	self->compute_secret_key_array(self, encrypted_size - 1);

	/*
	Decryption consists in finding c_0 + c_1 *s + ... + c_{count-1} * s^{count-1} mod q_1 * q_2 * q_3
	as long as ||m + v|| < q_1 * q_2 * q_3
	This is equal to m + v where ||v|| is small enough.
	*/

	// Since we overwrite destination, we zeroize destination parameters
	// This is necessary, otherwise resize will throw an exception.
	destination->parms_id_ = parms_id_zero;

	// Resize destination to appropriate size
	destination->resize(destination, rns_poly_uint64_count);

	// Make a temp destination for all the arithmetic mod q1, q2, q3
	//auto tmp_dest_modq(allocate_zero_poly(coeff_count, decryption_coeff_mod_count, pool));

	// put < (c_1 , c_2, ... , c_{count-1}) , (s,s^2,...,s^{count-1}) > mod q in destination

	// Now do the dot product of encrypted_copy and the secret key array using NTT.
	// The secret key powers are already NTT transformed.

	uint64_t* copy_operand1 = allocate_uint(coeff_count);
	log_trace("coeff_mod_count=%zu\n", coeff_mod_count);
	for (size_t i = 0; i < coeff_mod_count; i++)
	{
		// Initialize pointers for multiplication
		// c_1 mod qi
		const uint64_t *current_array1 = encrypted->at((Ciphertext *)encrypted, 1) + (i * coeff_count);
		// s mod qi
		const uint64_t *current_array2 = self->secret_key_array_ + (i * coeff_count);
		// set destination coefficients to zero modulo q_i
		set_zero_uint(coeff_count, destination->data(destination) + (i * coeff_count));

		for (size_t j = 0; j < encrypted_size - 1; j++)
		{
			// Perform the dyadic product.
			set_uint_uint(current_array1, coeff_count, copy_operand1);

			// Lazy reduction
			//ntt_negacyclic_harvey_lazy(copy_operand1.get(), small_ntt_tables[i]);
			dyadic_product_coeffsmallmod(copy_operand1, current_array2, coeff_count,
				coeff_modulus->at(coeff_modulus, i), copy_operand1);
			add_poly_poly_coeffsmallmod(destination->data(destination) + (i * coeff_count),
				copy_operand1, coeff_count, coeff_modulus->at(coeff_modulus, i),
				destination->data(destination) + (i * coeff_count));

			// go to c_{1+j+1} and s^{1+j+1} mod qi
			current_array1 += rns_poly_uint64_count;
			current_array2 += key_rns_poly_uint64_count;
		}

		// add c_0 into destination
		add_poly_poly_coeffsmallmod(destination->data(destination) + (i * coeff_count),
			encrypted->data((Ciphertext *)encrypted) + (i * coeff_count), coeff_count,
			coeff_modulus->at(coeff_modulus, i), destination->data(destination) + (i * coeff_count));
	}
	hedge_free(copy_operand1);

	// Set destination parameters as in encrypted
	destination->parms_id_ = encrypted->parms_id_;
	destination->scale_ = encrypted->scale_;
}

static void compute_secret_key_array(Decryptor* self, size_t max_power)
{
#ifdef HEDGE_DEBUG
	if (max_power < 1)
	{
		invalid_argument("max_power must be at least 1");
	}
	if (!self->secret_key_array_size_ || !self->secret_key_array_)
	{
		logic_error("secret_key_array_ is uninitialized");
	}
#endif
	// WARNING: This function must be called with the original context_data
	ctxdata_t* context_data = self->context_->key_ctxdata(self->context_);
	encrypt_parameters* parms = context_data->parms;
	vector(Zmodulus)* coeff_modulus = parms->coeff_modulus_;
	size_t coeff_count = parms->poly_modulus_degree_;
	size_t coeff_mod_count = coeff_modulus->size;
	size_t key_rns_poly_uint64_count = mul_safe(coeff_count, coeff_mod_count);

	size_t old_size = self->secret_key_array_size_;
	size_t new_size = max(max_power, old_size);

	if (old_size == new_size)
	{
		return;
	}

	// Need to extend the array
	// Compute powers of secret key until max_power
	uint64_t* new_secret_key_array = allocate_poly(
		mul_safe(new_size, coeff_count), coeff_mod_count);
	set_poly_poly_simple(self->secret_key_array_, old_size * coeff_count,
		coeff_mod_count, new_secret_key_array);

	set_poly_poly_simple(self->secret_key_array_, mul_safe(old_size, coeff_count),
		coeff_mod_count, new_secret_key_array);

	uint64_t *prev_poly_ptr = new_secret_key_array +
		mul_safe(old_size - 1, key_rns_poly_uint64_count);
	uint64_t *next_poly_ptr = prev_poly_ptr + key_rns_poly_uint64_count;

	// Since all of the key powers in secret_key_array_ are already NTT transformed,
	// to get the next one we simply need to compute a dyadic product of the last
	// one with the first one [which is equal to NTT(secret_key_)].
	for (size_t i = old_size; i < new_size; i++)
	{
		for (size_t j = 0; j < coeff_mod_count; j++)
		{
			dyadic_product_coeffsmallmod(prev_poly_ptr + (j * coeff_count),
				new_secret_key_array + (j * coeff_count),
				coeff_count, coeff_modulus->at(coeff_modulus, j),
				next_poly_ptr + (j * coeff_count));
		}
		prev_poly_ptr = next_poly_ptr;
		next_poly_ptr += key_rns_poly_uint64_count;
	}

	// Do we still need to update size?
	old_size = self->secret_key_array_size_;
	new_size = max(max_power, self->secret_key_array_size_);

	if (old_size == new_size)
	{
		return;
	}

	// Acquire new array
	self->secret_key_array_size_ = new_size;
	self->secret_key_array_ = new_secret_key_array;
}

void compose(Decryptor* self, const ctxdata_t* context_data, uint64_t *value)
{
#ifdef HEDGE_DEBUG
	if (value == NULL)
	{
		invalid_argument("input cannot be null");
	}
#endif
	encrypt_parameters* parms = context_data->parms;
	vector(Zmodulus)* coeff_modulus = parms->coeff_modulus(parms);;
	size_t coeff_count = parms->poly_modulus_degree_;
	size_t coeff_mod_count = coeff_modulus->size;
	size_t rns_poly_uint64_count = mul_safe(coeff_count, coeff_mod_count);

	base_converter_t* base_converter = context_data->base_converter;
	uint64_t* coeff_products_array = base_converter->coeff_products_array;
	uint64_t* inv_coeff_mod_coeff_array = base_converter->inv_coeff_base_products_mod_coeff_array;

	// Set temporary coefficients_ptr pointer to point to either an existing
	// allocation given as parameter, or else to a new allocation from the memory pool.
	uint64_t* coefficients = allocate_uint(rns_poly_uint64_count);
	uint64_t* coefficients_ptr = coefficients;

	// Re-merge the coefficients first
	for (size_t i = 0; i < coeff_count; i++)
	{
		for (size_t j = 0; j < coeff_mod_count; j++)
		{
			coefficients_ptr[j + (i * coeff_mod_count)] = value[(j * coeff_count) + i];
		}
	}

	uint64_t* temp = allocate_uint(coeff_mod_count);
	set_zero_uint(rns_poly_uint64_count, value);

	for (size_t i = 0; i < coeff_count; i++)
	{
		for (size_t j = 0; j < coeff_mod_count; j++)
		{
			uint64_t tmp = multiply_uint_uint_smallmod(coefficients_ptr[j],
				inv_coeff_mod_coeff_array[j], coeff_modulus->at(coeff_modulus, j));
			multiply_uint_uint64(coeff_products_array + (j * coeff_mod_count),
				coeff_mod_count, tmp, coeff_mod_count, temp);
			add_uint_uint_mod(temp, value + (i * coeff_mod_count),
				context_data->total_coeff_modulus,
				coeff_mod_count, value + (i * coeff_mod_count));
		}
		set_zero_uint(coeff_mod_count, temp);
		coefficients_ptr += coeff_mod_count;
	}
}

Decryptor* new_Decryptor(hedge_context_t* context, SecretKey* secret_key)
{
	Decryptor* self = hedge_zalloc(sizeof(Decryptor));
	self->invariant_noise_budget = NULL; // not used in CKKS
	self->ckks_decrypt = ckks_decrypt;
	self->decrypt = ckks_decrypt;
	self->compute_secret_key_array = compute_secret_key_array;
	self->compose = compose;

	decryptor_init(self, context, secret_key);

	return self;
}

void del_Decryptor(Decryptor* self)
{
	hedge_free(self->secret_key_array_);
	hedge_free(self);
}
