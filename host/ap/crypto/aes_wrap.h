/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef AES_WRAP_H
#define AES_WRAP_H

int __must_check aes_wrap(const u8 *kek, int n, const u8 *plain, u8 *cipher);
int __must_check aes_unwrap(const u8 *kek, int n, const u8 *cipher, u8 *plain);
int __must_check omac1_aes_128_vector(const u8 *key, size_t num_elem,
				      const u8 *addr[], const size_t *len,
				      u8 *mac);
int __must_check omac1_aes_128(const u8 *key, const u8 *data, size_t data_len,
			       u8 *mac);
int __must_check aes_128_encrypt_block(const u8 *key, const u8 *in, u8 *out);
int __must_check aes_128_ctr_encrypt(const u8 *key, const u8 *nonce,
				     u8 *data, size_t data_len);
int __must_check aes_128_eax_encrypt(const u8 *key,
				     const u8 *nonce, size_t nonce_len,
				     const u8 *hdr, size_t hdr_len,
				     u8 *data, size_t data_len, u8 *tag);
int __must_check aes_128_eax_decrypt(const u8 *key,
				     const u8 *nonce, size_t nonce_len,
				     const u8 *hdr, size_t hdr_len,
				     u8 *data, size_t data_len, const u8 *tag);
int __must_check aes_128_cbc_encrypt(const u8 *key, const u8 *iv, u8 *data,
				     size_t data_len);
int __must_check aes_128_cbc_decrypt(const u8 *key, const u8 *iv, u8 *data,
				     size_t data_len);

#endif /* AES_WRAP_H */
