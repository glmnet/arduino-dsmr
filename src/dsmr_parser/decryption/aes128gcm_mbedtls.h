#pragma once
#include "../util.h"
#include "aes128gcm.h"
#include <mbedtls/gcm.h>
#include <span>

namespace dsmr_parser {

class Aes128GcmMbedTls final : public Aes128GcmDecryptor, NonCopyableAndNonMovable {
  mbedtls_gcm_context gcm;

public:
  Aes128GcmMbedTls() { mbedtls_gcm_init(&gcm); }

  void set_encryption_key(const Aes128GcmDecryptionKey& key) override { mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key.data(), 128); }

  bool decrypt_inplace(std::span<const uint8_t, 12> nonce, std::span<uint8_t> ciphertext, std::span<const uint8_t, 12> tag) override {
    if (aad.has_value()) {
      return mbedtls_gcm_auth_decrypt(/* ctx     */ &gcm,
                                      /* length  */ ciphertext.size(),
                                      /* iv      */ nonce.data(),
                                      /* iv_len  */ nonce.size(),
                                      /* aad     */ aad->data(),
                                      /* aad_len */ aad->size(),
                                      /* tag     */ tag.data(),
                                      /* tag_len */ tag.size(),
                                      /* input   */ ciphertext.data(),
                                      /* output  */ ciphertext.data()) == 0;
    }

    // No authentication key provided: decrypt without verifying the GCM tag.
    unsigned char dummy_tag[12];
    return mbedtls_gcm_crypt_and_tag(/* ctx     */ &gcm,
                                     /* mode    */ MBEDTLS_GCM_DECRYPT,
                                     /* length  */ ciphertext.size(),
                                     /* iv      */ nonce.data(),
                                     /* iv_len  */ nonce.size(),
                                     /* aad     */ nullptr,
                                     /* aad_len */ 0,
                                     /* input   */ ciphertext.data(),
                                     /* output  */ ciphertext.data(),
                                     /* tag_len */ sizeof(dummy_tag),
                                     /* tag     */ dummy_tag) == 0;
  }

  ~Aes128GcmMbedTls() { mbedtls_gcm_free(&gcm); }
};

}
