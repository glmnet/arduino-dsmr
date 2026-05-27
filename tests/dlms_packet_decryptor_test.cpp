#include "dsmr_parser/decryption/aes128gcm_bearssl.h"
#include "dsmr_parser/decryption/aes128gcm_mbedtls.h"
#include "dsmr_parser/decryption/aes128gcm_tfpsa.h"
#include "dsmr_parser/dlms_packet_decryptor.h"
#include "test_util.h"
#include <doctest.h>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <source_location>

using namespace dsmr_parser;

inline std::vector<std::uint8_t> read_binary_file(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

inline std::vector<std::uint8_t> get_test_encrypted_packet() {
  return read_binary_file(std::filesystem::path(std::source_location::current().file_name()).parent_path() / "test_data" / "encrypted_packet.bin");
}

TEST_CASE_FIXTURE(LogFixture, "EncryptionKey FromHex method works correctly") {
  // success cases
  REQUIRE(Aes128GcmDecryptionKey::from_hex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"));
  REQUIRE(Aes128GcmDecryptionKey::from_hex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

  // failure cases
  REQUIRE(!Aes128GcmDecryptionKey::from_hex("AAAAAAAAAAA"));                      // key too short
  REQUIRE(!Aes128GcmDecryptionKey::from_hex("GAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA")); // non hex symbols in key
}

TEST_CASE_FIXTURE(LogFixture, "Can decrypt a correct packet") {
  const auto encryption_key = *Aes128GcmDecryptionKey::from_hex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
  const auto authentication_key = *Aes128GcmAuthenticationKey::from_hex("00112233445566778899AABBCCDDEEFF");
  const std::string_view packet_starts = "/EST5\\253710000_A\r\n";
  const std::string_view packet_ends = "1-0:4.7.0(000000166*var)\r\n!";

  SUBCASE("MbedTls") {
    Aes128GcmMbedTls gcm_decryptor;
    gcm_decryptor.set_encryption_key(encryption_key);
    gcm_decryptor.set_authentication_key(authentication_key);
    DlmsPacketDecryptor decryptor(gcm_decryptor, true);
    auto packet = get_test_encrypted_packet();

    const auto dsmr_telegram = decryptor.decrypt_inplace({packet.data(), packet.size()});
    REQUIRE(dsmr_telegram);
    REQUIRE(dsmr_telegram->content_without_crc().starts_with(packet_starts));
    REQUIRE(dsmr_telegram->content_without_crc().ends_with(packet_ends));
  }

  SUBCASE("BearSsl") {
    Aes128GcmBearSsl gcm_decryptor;
    gcm_decryptor.set_encryption_key(encryption_key);
    gcm_decryptor.set_authentication_key(authentication_key);
    DlmsPacketDecryptor decryptor(gcm_decryptor, true);
    auto packet = get_test_encrypted_packet();

    const auto dsmr_telegram = decryptor.decrypt_inplace({packet.data(), packet.size()});
    REQUIRE(dsmr_telegram);
    REQUIRE(dsmr_telegram->content_without_crc().starts_with(packet_starts));
    REQUIRE(dsmr_telegram->content_without_crc().ends_with(packet_ends));
  }

  SUBCASE("TfPsa") {
    Aes128GcmTfPsa gcm_decryptor;
    gcm_decryptor.set_encryption_key(encryption_key);
    gcm_decryptor.set_authentication_key(authentication_key);
    DlmsPacketDecryptor decryptor(gcm_decryptor, true);
    auto packet = get_test_encrypted_packet();
    const auto dsmr_telegram = decryptor.decrypt_inplace({packet.data(), packet.size()});
    REQUIRE(dsmr_telegram);
    REQUIRE(dsmr_telegram->content_without_crc().starts_with(packet_starts));
    REQUIRE(dsmr_telegram->content_without_crc().ends_with(packet_ends));
  }
}

TEST_CASE_FIXTURE(LogFixture, "Fail to decrypt corrupted packet") {
  const auto encryption_key = *Aes128GcmDecryptionKey::from_hex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
  const auto authentication_key = *Aes128GcmAuthenticationKey::from_hex("00112233445566778899AABBCCDDEEFF");
  auto packet = get_test_encrypted_packet();
  packet[50] ^= 0xFF;

  SUBCASE("MbedTls") {
    Aes128GcmMbedTls gcm_decryptor;
    gcm_decryptor.set_encryption_key(encryption_key);
    gcm_decryptor.set_authentication_key(authentication_key);
    DlmsPacketDecryptor decryptor(gcm_decryptor, true);
    REQUIRE_FALSE(decryptor.decrypt_inplace({packet.data(), packet.size()}));
  }

  SUBCASE("BearSsl") {
    Aes128GcmBearSsl gcm_decryptor;
    gcm_decryptor.set_encryption_key(encryption_key);
    gcm_decryptor.set_authentication_key(authentication_key);
    DlmsPacketDecryptor decryptor(gcm_decryptor, true);
    REQUIRE_FALSE(decryptor.decrypt_inplace({packet.data(), packet.size()}));
  }

  SUBCASE("TfPsa") {
    Aes128GcmTfPsa gcm_decryptor;
    gcm_decryptor.set_encryption_key(encryption_key);
    gcm_decryptor.set_authentication_key(authentication_key);
    DlmsPacketDecryptor decryptor(gcm_decryptor, true);
    REQUIRE_FALSE(decryptor.decrypt_inplace({packet.data(), packet.size()}));
  }
}

TEST_CASE_FIXTURE(LogFixture, "Fail to decrypt packet with corrupted header") {
  Aes128GcmMbedTls gcm_decryptor;
  gcm_decryptor.set_encryption_key(*Aes128GcmDecryptionKey::from_hex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"));
  DlmsPacketDecryptor decryptor(gcm_decryptor, true);
  auto packet = get_test_encrypted_packet();

  packet[0] = 0;

  const auto dsmr_telegram = decryptor.decrypt_inplace({packet.data(), packet.size()});
  REQUIRE_FALSE(dsmr_telegram);
}

TEST_CASE_FIXTURE(LogFixture, "Decryption fails if the dlms packet is too small") {
  Aes128GcmMbedTls gcm_decryptor;
  gcm_decryptor.set_encryption_key(*Aes128GcmDecryptionKey::from_hex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"));
  DlmsPacketDecryptor decryptor(gcm_decryptor, true);
  std::vector<uint8_t> small_dlms_packet(10);
  const auto dsmr_telegram = decryptor.decrypt_inplace({small_dlms_packet.data(), small_dlms_packet.size()});
  REQUIRE_FALSE(dsmr_telegram);
}

TEST_CASE_FIXTURE(LogFixture, "Decryption succeeds without tag check if authentication_key is not provided") {
  const auto encryption_key = *Aes128GcmDecryptionKey::from_hex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
  const std::string_view packet_starts = "/EST5\\253710000_A\r\n";
  const std::string_view packet_ends = "1-0:4.7.0(000000166*var)\r\n!";

  SUBCASE("MbedTls") {
    Aes128GcmMbedTls gcm_decryptor;
    gcm_decryptor.set_encryption_key(encryption_key);
    DlmsPacketDecryptor decryptor(gcm_decryptor, true);
    auto packet = get_test_encrypted_packet();
    const auto dsmr_telegram = decryptor.decrypt_inplace({packet.data(), packet.size()});
    REQUIRE(dsmr_telegram);
    REQUIRE(dsmr_telegram->content_without_crc().starts_with(packet_starts));
    REQUIRE(dsmr_telegram->content_without_crc().ends_with(packet_ends));
  }

  SUBCASE("BearSsl") {
    Aes128GcmBearSsl gcm_decryptor;
    gcm_decryptor.set_encryption_key(encryption_key);
    DlmsPacketDecryptor decryptor(gcm_decryptor, true);
    auto packet = get_test_encrypted_packet();
    const auto dsmr_telegram = decryptor.decrypt_inplace({packet.data(), packet.size()});
    REQUIRE(dsmr_telegram);
    REQUIRE(dsmr_telegram->content_without_crc().starts_with(packet_starts));
    REQUIRE(dsmr_telegram->content_without_crc().ends_with(packet_ends));
  }

  SUBCASE("TfPsa") {
    Aes128GcmTfPsa gcm_decryptor;
    gcm_decryptor.set_encryption_key(encryption_key);
    DlmsPacketDecryptor decryptor(gcm_decryptor, true);
    auto packet = get_test_encrypted_packet();
    const auto dsmr_telegram = decryptor.decrypt_inplace({packet.data(), packet.size()});
    REQUIRE(dsmr_telegram);
    REQUIRE(dsmr_telegram->content_without_crc().starts_with(packet_starts));
    REQUIRE(dsmr_telegram->content_without_crc().ends_with(packet_ends));
  }
}

TEST_CASE_FIXTURE(LogFixture, "CRC check fails when decrypting a corrupted packet without authentication key") {
  const auto encryption_key = *Aes128GcmDecryptionKey::from_hex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
  auto packet = get_test_encrypted_packet();
  packet[50] ^= 0xFF;
  Aes128GcmMbedTls gcm_decryptor;
  gcm_decryptor.set_encryption_key(encryption_key);
  DlmsPacketDecryptor decryptor(gcm_decryptor, true);
  REQUIRE_FALSE(decryptor.decrypt_inplace({packet.data(), packet.size()}));
}

TEST_CASE_FIXTURE(LogFixture, "Decryption of a corrupted packet succeeds without authentication key when CRC check is disabled") {
  const auto encryption_key = *Aes128GcmDecryptionKey::from_hex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
  auto packet = get_test_encrypted_packet();
  packet[50] ^= 0xFF;
  Aes128GcmMbedTls gcm_decryptor;
  gcm_decryptor.set_encryption_key(encryption_key);
  DlmsPacketDecryptor decryptor(gcm_decryptor, false);
  REQUIRE(decryptor.decrypt_inplace({packet.data(), packet.size()}));
}

TEST_CASE_FIXTURE(LogFixture, "Does not crash if decrypt_inplace is called without set_encryption_key") {
  auto packet = get_test_encrypted_packet();

  SUBCASE("MbedTls") {
    Aes128GcmMbedTls gcm_decryptor;
    DlmsPacketDecryptor decryptor(gcm_decryptor, true);
    REQUIRE_FALSE(decryptor.decrypt_inplace({packet.data(), packet.size()}));
  }

  SUBCASE("BearSsl") {
    Aes128GcmBearSsl gcm_decryptor;
    DlmsPacketDecryptor decryptor(gcm_decryptor, true);
    REQUIRE_FALSE(decryptor.decrypt_inplace({packet.data(), packet.size()}));
  }

  SUBCASE("TfPsa") {
    Aes128GcmTfPsa gcm_decryptor;
    DlmsPacketDecryptor decryptor(gcm_decryptor, true);
    REQUIRE_FALSE(decryptor.decrypt_inplace({packet.data(), packet.size()}));
  }
}
