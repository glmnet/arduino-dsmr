#pragma once
#include "decryption/aes128gcm.h"
#include "packet_accumulator.h"
#include "util.h"
#include <array>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>

namespace dsmr_parser {

// Decrypts DLMS packets encrypted with AES-128-GCM.
// The encryption is described in the "specs/Luxembourg Smarty P1 specification v1.1.3.pdf" chapter "3.2.5 P1 software – Channel security".
class DlmsPacketDecryptor final : NonCopyableAndNonMovable {

#pragma pack(push, 1)
  // The packet has the following structure:
  //   Header (18 bytes) | Encrypted Telegram | GCM Tag (12 bytes)
  struct DlmsPacket final : NonCopyableAndNonMovable {
  private:
    struct {
      uint8_t tag;                              // always = 0xDB
      uint8_t system_title_length;              // always = 0x08
      uint8_t system_title[8];                  // arbitrary sequence. For example: ['S', 'Y', 'S', 'T', 'E', 'M', 'I', 'D']
      uint8_t long_form_length_indicator;       // always = 0x82
      uint8_t total_length_big_endian[2];       // SecurityControlFieldLength + InvocationCounterLength + EncryptedTelegramLength + GcmTagLength
      uint8_t security_control_field;           // always = 0x30
      uint8_t invocation_counter_big_endian[4]; // also called "frame counter"
    } header;
    uint8_t encrypted_telegram_with_gcm_tag[1]; // GCM Tag is 12 bytes

  public:
    static DlmsPacket* from_bytes(const std::span<uint8_t> bytes) {
      if (bytes.size() < sizeof(header) + /* tag length */ 12) {
        Logger::log(LogLevel::DEBUG, "DLMS packet is too short. Size: %zu", bytes.size());
        return nullptr;
      }

      auto& packet = *reinterpret_cast<DlmsPacket*>(bytes.data());
      const auto expected_length = sizeof(header) + /* tag length */ 12 + packet.telegram_length();
      if (expected_length != bytes.size()) {
        Logger::log(LogLevel::DEBUG, "DLMS packet length mismatch. Expected: %zu, actual: %zu", expected_length, bytes.size());
        return nullptr;
      }

      if (packet.telegram_length() < 10) {
        Logger::log(LogLevel::DEBUG, "DLMS encrypted telegram is too short. Size: %zu", packet.telegram_length());
        return nullptr;
      }

      const auto header_bytes_consistent = packet.header.tag == 0xDB && packet.header.system_title_length == 0x08 &&
                                           packet.header.long_form_length_indicator == 0x82 && packet.header.security_control_field == 0x30;
      if (!header_bytes_consistent) {
        Logger::log(LogLevel::DEBUG, "DLMS packet header is corrupted");
        return nullptr;
      }

      return &packet;
    }

    // Also called "IV"
    std::array<uint8_t, 12> nonce() const {
      // nonce = SystemTitle (8 bytes) + InvocationCounter (4 bytes)
      const auto& st = header.system_title;
      const auto& ic = header.invocation_counter_big_endian;
      return {st[0], st[1], st[2], st[3], st[4], st[5], st[6], st[7], ic[0], ic[1], ic[2], ic[3]};
    }

    std::span<uint8_t> encrypted_telegram() { return {encrypted_telegram_with_gcm_tag, telegram_length()}; }

    std::span<const uint8_t, 12> gcm_tag() const { return std::span<const uint8_t, 12>{encrypted_telegram_with_gcm_tag + telegram_length(), 12}; }

  private:
    // encrypted and decrypted telegrams have the same length
    size_t telegram_length() const {
      // Convert from big-endian to the host's little-endian
      const auto total_length = (header.total_length_big_endian[0] << 8) | (header.total_length_big_endian[1]);
      return static_cast<size_t>(total_length - 5 - 12); // 5 = SecurityControlFieldLength + InvocationCounterLength. 12 = GcmTagLength
    }
  };
#pragma pack(pop)
  static_assert(sizeof(DlmsPacket) == 19, "EncryptedPacket struct must be 19 bytes");

  Aes128GcmDecryptor& decryptor;
  const bool check_crc_;

  static void log_span_as_hex(const LogLevel level, const std::span<const uint8_t> data) {
    constexpr size_t kCharsPerChunk = 200;
    constexpr size_t kBytesPerChunk = kCharsPerChunk / 2;
    for (size_t i = 0; i < data.size(); i += kBytesPerChunk) {
      char hex[kCharsPerChunk + 1];
      const size_t n = std::min(kBytesPerChunk, data.size() - i);
      for (size_t j = 0; j < n; ++j) {
        constexpr char kHex[] = "0123456789ABCDEF";
        hex[j * 2] = kHex[data[i + j] >> 4];
        hex[j * 2 + 1] = kHex[data[i + j] & 0x0F];
      }
      hex[n * 2] = '\0';
      Logger::log(level, "%s", hex);
    }
  }

public:
  explicit DlmsPacketDecryptor(Aes128GcmDecryptor& dec, bool check_crc) : decryptor(dec), check_crc_(check_crc) {}

  std::optional<DsmrUnencryptedTelegram> decrypt_inplace(std::span<uint8_t> dlms_packet_bytes) {
    Logger::log(LogLevel::VERY_VERBOSE, "Decrypt DLMS packet:");
    log_span_as_hex(LogLevel::VERY_VERBOSE, dlms_packet_bytes);
    Logger::log(LogLevel::VERY_VERBOSE, "=========");

    auto dlms_packet = DlmsPacket::from_bytes(dlms_packet_bytes);
    if (dlms_packet == nullptr) {
      return std::nullopt;
    }

    const bool res = decryptor.decrypt_inplace(dlms_packet->nonce(), dlms_packet->encrypted_telegram(), dlms_packet->gcm_tag());
    if (!res) {
      Logger::log(LogLevel::DEBUG, "Decryption of DLMS packet failed");
      return std::nullopt;
    }

    // The unencrypted DSMR telegram looks like "/data!abcd\r\n"
    const auto telegram = DsmrUnencryptedTelegram::from_bytes(
        std::string_view{reinterpret_cast<const char*>(dlms_packet->encrypted_telegram().data()), dlms_packet->encrypted_telegram().size()}, check_crc_);

    return telegram;
  }
};

}
