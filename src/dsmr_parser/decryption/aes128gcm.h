#pragma once
#include "../util.h"
#include <charconv>
#include <optional>
#include <span>

namespace dsmr_parser {

namespace detail {

struct DecryptionKeyTag {};
struct AuthenticationKeyTag {};

template <typename Tag>
class Aes128Key final {
  std::array<uint8_t, 16> key{};
  explicit Aes128Key(const std::array<uint8_t, 16> k) : key(k) {}

public:
  // hex is a string like "00112233445566778899AABBCCDDEEFF"
  static std::optional<Aes128Key> from_hex(const std::string_view hex) {
    if (hex.size() != 32)
      return std::nullopt;
    std::array<uint8_t, 16> arr{};
    for (size_t i = 0; i < 16; ++i) {
      auto [ptr, ec] = std::from_chars(hex.data() + i * 2, hex.data() + i * 2 + 2, arr[i], 16);
      if (ec != std::errc{})
        return std::nullopt;
    }
    return Aes128Key(arr);
  }

  const uint8_t* data() const { return key.data(); }
};

}

using Aes128GcmDecryptionKey = detail::Aes128Key<detail::DecryptionKeyTag>;
using Aes128GcmAuthenticationKey = detail::Aes128Key<detail::AuthenticationKeyTag>;

class Aes128GcmDecryptor {
public:
  virtual void set_encryption_key(const Aes128GcmDecryptionKey& key) = 0;
  void set_authentication_key(const Aes128GcmAuthenticationKey& key) {
    // SecurityControlField is always 0x30
    aad = std::array<uint8_t, 17>{0x30,           key.data()[0],  key.data()[1],  key.data()[2],  key.data()[3], key.data()[4],
                                  key.data()[5],  key.data()[6],  key.data()[7],  key.data()[8],  key.data()[9], key.data()[10],
                                  key.data()[11], key.data()[12], key.data()[13], key.data()[14], key.data()[15]};
  }
  virtual bool decrypt_inplace(std::span<const uint8_t, 12> nonce, std::span<uint8_t> ciphertext, std::span<const uint8_t, 12> tag) = 0;
  virtual ~Aes128GcmDecryptor() = default;

protected:
  Aes128GcmDecryptor() = default;

  // AdditionalAuthenticatedData = SecurityControlField + AuthenticationKey.
  std::optional<std::array<uint8_t, 17>> aad;
};

}
