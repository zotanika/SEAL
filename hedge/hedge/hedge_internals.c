
#include "hedge_internals.h"

#include "math/smallmodulus.h"
#include "math/modulus.h"
#include "lib/debug.h"
#include "keygenerator.h"
#include "publickey.h"
#include "encryptionparams.h"
#include "encryptor.h"
#include "decryptor.h"
#include "defines.h"
#include "hedge_params.h"
#include "plaintext.h"
#include "ciphertext.h"
#include "ckks.h"

/**
 * The caller must free the returned object
 */
object_t* stream_to_obj(bytestream_t* stream)
{
    log_trace("stream_to_obj size= %zu\n", stream->size);
    object_t* obj = hedge_malloc(sizeof(object_t) + stream->size);
    obj->size = stream->size;
    memcpy(obj->data, stream->data, obj->size);
    // set stream pos to zero
    stream->reset(stream);
    return obj;
}

/**
 * The caller must free the returned object
 */
object_t* serialize_pubkey(PublicKey* key)
{
    object_t *ret;
    bytestream_t *stream = new_bytestream(8192);
    key->save(key, stream);
    ret = stream_to_obj(stream);
    del_bytestream(stream);
    return ret;
}

/**
 * The caller must free the returned object
 */
object_t* serialize_seckey(SecretKey* key)
{
    object_t *ret;
    bytestream_t *stream = new_bytestream(8192);
    key->save(key, stream);
    ret = stream_to_obj(stream);
    del_bytestream(stream);
    return ret;
}

/**
 * The caller must free the returned object
 */
object_t* serialize_relinkeys(RelinKeys* key)
{
    object_t *ret;
    bytestream_t *stream = new_bytestream(8192);
    key->parent->save(key->parent, stream);
    ret = stream_to_obj(stream);
    del_bytestream(stream);
    return ret;
}

/**
 * The caller must free the returned object
 */
object_t* serialize_ciphertext(Ciphertext* ciphertext)
{
    object_t *ret;
    bytestream_t *stream = new_bytestream(8192);
    ciphertext->save(ciphertext, stream);
    ret = stream_to_obj(stream);
    del_bytestream(stream);
    return ret;
}

/**
 * The caller must free the returned object
 */
object_t* serialize_plaintext(Plaintext* plaintext)
{
    object_t *ret;
    bytestream_t *stream = new_bytestream(8192);
    plaintext->save(plaintext, stream);
    ret = stream_to_obj(stream);
    del_bytestream(stream);
    return ret;
}

/**
 * The caller must call del_bytestream for the returned stream
 */
bytestream_t* obj_to_stream(object_t* obj)
{
    log_trace("obj_to_stream OBJ size= %zu\n", obj->size);
    bytestream_t* stream = new_bytestream(obj->size + 89);
    stream->write(stream, obj->data, obj->size);
    log_trace("obj_to_stream STREAM size= %zu\n", stream->size);
    // rewind pos
    stream->reset(stream);
    return stream;
}

/**
 * The caller must call del_Ciphertext for the returned cipher text
 */
Ciphertext* load_ciphertext(hedge_ciphertext_t* obj)
{
    Ciphertext* cipher= new_Ciphertext();
    bytestream_t* stream = obj_to_stream(obj);
    cipher->load(cipher, stream);
    del_bytestream(stream);
    return cipher;
}

/**
 * The caller must call del_Ciphertext for the returned cipher text
 */
Plaintext* load_plaintext(hedge_plaintext_t* obj)
{
    Plaintext* plain= new_Plaintext();
    bytestream_t* stream = obj_to_stream(obj);
    plain->load(plain, stream);
    del_bytestream(stream);
    return plain;
}

void hedge_internals_create(hedge_t* hedge, uint64 poly_mod_degree, uint64 coeff_mod_cnt, double scale)
{

    list_init((list_head_t*)&hedge->internals.object_list);

    log_trace("create encrypt paramn\n");
    encrypt_parameters* parms = new_encrypt_parameters(CKKS);

    log_trace("set polynomial modulus degree\n");
    parms->set_poly_modulus_degree(parms, poly_mod_degree);

    int bitsizes[] = {60, scale, scale, 60};

    log_trace("create coeff modulus\n");
    vector(Zmodulus)* coeff_modulus = create_coeff_modulus(poly_mod_degree, bitsizes, sizeof(bitsizes)/sizeof(bitsizes[0]));
    parms->set_coeff_modulus(parms, coeff_modulus);
    del_vector(coeff_modulus);
    
    log_trace("parms-coeff_modulus_->size : %zu\n", parms->coeff_modulus_->size);
    
    hedge_context_t* context = new_hcontext(parms, true, sec_level_type_tc128);

    log_trace("hedge context created. context=%p\n", context);
    log_trace("parms-coeff_modulus_->size : %zu\n", parms->coeff_modulus_->size);
    
    print_array_trace(context->key_parms_id.data, 4);
    print_array_trace(context->first_parms_id.data, 4);
    print_array_trace(context->last_parms_id.data, 4);
    
    key_generator_t* keygen = new_keygenerator(context);
    PublicKey* pubkey = keygen->get_public_key(keygen);
    SecretKey* seckey = keygen->get_secret_key(keygen);
    RelinKeys* relinkeys = keygen->relin_keys(keygen, 1);
   

    CKKSEncoder* encoder = new_CKKSEncoder(context);
    log_trace("Encoder slots = %zu\n", encoder->slot_count(encoder));
    Encryptor* encryptor = new_Encryptor(context, pubkey);
    //Evaluator* evaluator = new_Evaluator(context);
    Decryptor* decryptor = new_Decryptor(context, seckey);
    
    size_t slot_count = encoder->slot_count(encoder);

    hedge->config.poly_modulus_degree = (uint64_t)parms->poly_modulus_degree;
    hedge->config.coeff_modulus = (uint64_t)parms->coeff_modulus;
    hedge->config.scale = scale;
    hedge->internals.parms = (void*)parms;
    hedge->internals.context = (void*)context;
    hedge->internals.keygen = (void*)keygen;
    hedge->internals.pubkey = (void*)pubkey;
    hedge->internals.seckey = (void*)seckey;
    hedge->internals.relinkeys = (void*)relinkeys;
    hedge->internals.encryptor = (void*)encryptor;
    //hedge->internals.evaluator = (void*)evaluator;
    hedge->internals.encoder = (void*)encoder;
    hedge->internals.decryptor = (void*)decryptor;

    hedge->pubkey = serialize_pubkey(pubkey);
    hedge->seckey = serialize_seckey(seckey);
    hedge->relinkeys = serialize_relinkeys(relinkeys);
}

hedge_ciphertext_t* hedge_internals_encrypt(hedge_t* hedge, double* array, 
    size_t count)
{
    hedge_ciphertext_t* result;
    double scale = pow(2.0, hedge->config.scale);
    Plaintext* plaintext = new_Plaintext();
    Ciphertext* ciphertext = new_Ciphertext();
    CKKSEncoder* encoder = (CKKSEncoder*)hedge->internals.encoder;
    encoder->encode_simple(encoder, array, count, scale, plaintext);
    #if 0 //debug
    vector(double_complex)* destination = new_vector(double_complex, 1);
    encoder->decode(encoder, plaintext, destination);
    log_warning(" @@@2 %lf\n", destination->value_at(destination, 0));
    #endif

    Encryptor* encryptor = (Encryptor*)hedge->internals.encryptor;
    DEBUG_TRACE_LINE();
    encryptor->encrypt(encryptor, plaintext, ciphertext);
    log_trace("Encrypt : is ntt form = %d\n", ciphertext->is_ntt_form_);
    log_trace("Encrypt : scale = %lf\n", ciphertext->scale_);
    DEBUG_TRACE_LINE();
    result = serialize_ciphertext(ciphertext);
    DEBUG_TRACE_LINE();
    list_add((list_head_t*)&hedge->internals.object_list, result);
    del_Plaintext(plaintext);
    del_Ciphertext(ciphertext);
    
    return result;
}

hedge_double_array_t* hedge_internals_decrypt(hedge_t* hedge, 
    hedge_ciphertext_t* ciphertext)
{
    hedge_double_array_t* result;
    Plaintext* iPlaintext = new_Plaintext();
    Ciphertext* iCiphertext = (Ciphertext *)load_ciphertext(ciphertext);
    log_trace("Decrypt : is ntt form = %d\n", iCiphertext->is_ntt_form_);
    log_trace("Decrypt : scale = %lf\n", iCiphertext->scale_);
    vector(double_complex)* v = new_vector(double_complex, 4);
    double_complex* entry;
    size_t i;
    Decryptor* decryptor = (Decryptor*)hedge->internals.decryptor;
    CKKSEncoder* encoder = (CKKSEncoder*)hedge->internals.encoder;

    decryptor->decrypt(decryptor, iCiphertext, iPlaintext);
    //print_polynomial("decypted plaintext", iPlaintext->data_->data, iPlaintext->data_->count, TRUE);
    encoder->decode(encoder, iPlaintext, v);

    del_Plaintext(iPlaintext);
    del_Ciphertext(iCiphertext);

    result = hedge_malloc(sizeof(result) + v->size * sizeof(double));
    list_add((list_head_t*)&hedge->internals.object_list, result);
    result->size = v->size;
    iterate_vector(i, entry, v) {
        result->data[i] = *entry;
    }
    del_vector(v);

    return result;
}

hedge_plaintext_t* hedge_internals_encode(hedge_t* hedge, double* array, size_t count)
{
    double scale = pow(2.0, hedge->config.scale);
    Plaintext* iPlaintext = new_Plaintext();
    CKKSEncoder* encoder = (CKKSEncoder*)hedge->internals.encoder;
    encoder->encode_simple(encoder, array, count, scale, iPlaintext);
    hedge_plaintext_t* plaintext = serialize_plaintext(iPlaintext);
    del_Plaintext(iPlaintext);
    return plaintext;
}

hedge_double_array_t* hedge_internals_decode(hedge_t* hedge, hedge_plaintext_t* plaintext)
{
    hedge_double_array_t* result;
    double_complex* entry;
    size_t i;
    Plaintext* iPlaintext = load_plaintext(plaintext);
    CKKSEncoder* encoder = (CKKSEncoder*)hedge->internals.encoder;
    vector(double_complex)* v = new_vector(double_complex, 4);

    encoder->decode(encoder, iPlaintext, v);

    del_Plaintext(iPlaintext);
    result = hedge_malloc(sizeof(result) + v->size * sizeof(double));
    list_add((list_head_t*)&hedge->internals.object_list, result);
    result->size = v->size;
    iterate_vector(i, entry, v) {
        result->data[i] = *entry;
    }
    del_vector(v);
    return result;
}

void hedge_internals_shred(hedge_t* hedge)
{
    struct node *entry;
    iterate_list(hedge->internals.object_list, entry) {
        hedge_free(entry->data);
    }
    del_list((list_head_t*)&hedge->internals.object_list);

    hedge_free(hedge->pubkey);
    hedge_free(hedge->seckey);
    hedge_free(hedge->relinkeys);
    
    del_encrypt_parameters(hedge->internals.parms);
    del_hcontext(hedge->internals.context);
    del_keygenerator(hedge->internals.keygen);
    del_Encryptor(hedge->internals.encryptor);
    //delete_Evaluator(hedge->internals,evaluator);
    del_CKKSEncoder(hedge->internals.encoder);
    del_Decryptor(hedge->internals.decryptor);
}
