#pragma once

#if __has_include(<bearssl/bearssl.h>)
#include <bearssl/bearssl.h>
#elif __has_include(<bearssl.h>)
#include <bearssl.h>
#else
#error "BearSSL header not found"
#endif

#include "../util.h"
#include "aes128gcm.h"
#include <span>

namespace dsmr_parser {

class Aes128GcmBearSsl final : public Aes128GcmDecryptor, NonCopyableAndNonMovable {
  br_gcm_context gcm;
  br_aes_ct_ctr_keys aes;
  bool initialized = false;

public:
  Aes128GcmBearSsl() = default;

  void set_encryption_key(const Aes128GcmDecryptionKey& key) override {
    br_aes_ct_ctr_init(&aes, key.data(), 16);
    br_gcm_init(&gcm, &aes.vtable, br_ghash_ctmul32);
    initialized = true;
  }

  bool decrypt_inplace(std::span<const uint8_t, 12> nonce, std::span<uint8_t> ciphertext, std::span<const uint8_t, 12> tag) override {
    if (!initialized) {
      Logger::log(LogLevel::ERROR, "Decryption key is not set");
      return false;
    }

    br_gcm_reset(&gcm, nonce.data(), nonce.size());
    if (aad.has_value()) {
      br_gcm_aad_inject(&gcm, aad->data(), aad->size());
    }
    br_gcm_flip(&gcm);
    br_gcm_run(&gcm, 0, ciphertext.data(), ciphertext.size());
    // No authentication key provided: decrypt without verifying the GCM tag.
    if (!aad.has_value()) {
      return true;
    }
    return br_gcm_check_tag_trunc(&gcm, tag.data(), tag.size()) == 1;
  }

  ~Aes128GcmBearSsl() = default;
};

}
