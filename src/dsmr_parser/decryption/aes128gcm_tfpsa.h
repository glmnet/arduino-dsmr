#pragma once

#include "../util.h"
#include "aes128gcm.h"
#include <psa/crypto.h>
#include <span>

namespace dsmr_parser {

class Aes128GcmTfPsa final : public Aes128GcmDecryptor, NonCopyableAndNonMovable {
  psa_key_id_t key_id = 0;
  bool initialized = false;

public:
  Aes128GcmTfPsa() { initialized = (psa_crypto_init() == PSA_SUCCESS); }

  void set_encryption_key(const Aes128GcmDecryptionKey& key) override {
    if (!initialized) {
      return;
    }

    if (key_id != 0) {
      psa_destroy_key(key_id);
      key_id = 0;
    }

    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attributes, 128);
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DECRYPT);
    psa_set_key_algorithm(&attributes, PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_GCM, 12));

    const psa_status_t status = psa_import_key(&attributes, key.data(), 16, &key_id);

    psa_reset_key_attributes(&attributes);

    if (status != PSA_SUCCESS) {
      key_id = 0;
    }
  }

  bool decrypt_inplace(std::span<const uint8_t, 12> nonce, std::span<uint8_t> ciphertext, std::span<const uint8_t, 12> tag) override {
    if (!initialized || key_id == 0) {
      Logger::log(LogLevel::ERROR, "Decryption key is not set");
      return false;
    }

    psa_aead_operation_t op = PSA_AEAD_OPERATION_INIT;

    const auto abort_and_fail = [&](const char* what, const psa_status_t status) -> bool {
      Logger::log(LogLevel::ERROR, "%s failed: %ld", what, static_cast<long>(status));
      const psa_status_t abort_status = psa_aead_abort(&op);
      if (abort_status != PSA_SUCCESS) {
        Logger::log(LogLevel::ERROR, "psa_aead_abort failed: %ld", static_cast<long>(abort_status));
      }
      return false;
    };

    const psa_algorithm_t alg = PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_GCM, tag.size());
    psa_status_t status = psa_aead_decrypt_setup(&op, key_id, alg);
    if (status != PSA_SUCCESS) {
      return abort_and_fail("psa_aead_decrypt_setup", status);
    }

    status = psa_aead_set_nonce(&op, nonce.data(), nonce.size());
    if (status != PSA_SUCCESS) {
      return abort_and_fail("psa_aead_set_nonce", status);
    }

    if (aad.has_value()) {
      status = psa_aead_update_ad(&op, aad->data(), aad->size());
      if (status != PSA_SUCCESS) {
        return abort_and_fail("psa_aead_update_ad", status);
      }
    }

    size_t produced = 0;
    status = psa_aead_update(/* operation     */ &op,
                             /* input         */ ciphertext.data(),
                             /* input_length  */ ciphertext.size(),
                             /* output        */ ciphertext.data(),
                             /* output_size   */ ciphertext.size(),
                             /* output_length */ &produced);
    if (status != PSA_SUCCESS) {
      return abort_and_fail("psa_aead_update", status);
    }

    if (produced != ciphertext.size()) {
      Logger::log(LogLevel::ERROR, "Unexpected plaintext size from psa_aead_update: %zu != %zu", produced, ciphertext.size());
      psa_aead_abort(&op);
      return false;
    }

    // No authentication key provided: decrypt without verifying the GCM tag.
    if (!aad.has_value()) {
      const psa_status_t abort_status = psa_aead_abort(&op);
      if (abort_status != PSA_SUCCESS) {
        Logger::log(LogLevel::ERROR, "psa_aead_abort failed: %ld", static_cast<long>(abort_status));
      }
      return true;
    }

    size_t tail_len = 0;
    status = psa_aead_verify(/* operation      */ &op,
                             /* plaintext      */ nullptr,
                             /* plaintext_size */ 0,
                             /* plaintext_len  */ &tail_len,
                             /* tag            */ tag.data(),
                             /* tag_len        */ tag.size());
    if (status != PSA_SUCCESS) {
      return abort_and_fail("psa_aead_verify", status);
    }

    if (tail_len != 0) {
      Logger::log(LogLevel::ERROR, "Unexpected trailing plaintext from psa_aead_verify: %zu", tail_len);
      return abort_and_fail("psa_aead_verify", PSA_ERROR_GENERIC_ERROR);
    }

    return true;
  }

  ~Aes128GcmTfPsa() {
    if (key_id != 0) {
      psa_destroy_key(key_id);
    }
  }
};

} // namespace dsmr_parser
