#pragma once
#include "util.h"
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace dsmr_parser {

// Receives unencrypted DSMR packets.
class PacketAccumulator final {
  class DsmrPacketBuffer final {
    std::span<uint8_t> _buffer;
    std::size_t _packetSize = 0;

  public:
    explicit DsmrPacketBuffer(std::span<uint8_t> buffer) : _buffer{buffer} {}

    std::string_view packet() const { return std::string_view(reinterpret_cast<const char*>(_buffer.data()), _packetSize); }

    void add(uint8_t byte) {
      _buffer[_packetSize] = byte;
      _packetSize++;
    }

    bool has_space() const { return _packetSize < _buffer.size(); }
  };

  enum class State { WaitingForPacketStartSymbol, WaitingForPacketEndSymbol, WaitingForCrc };
  State _state = State::WaitingForPacketStartSymbol;
  std::span<uint8_t> _raw_buffer;
  DsmrPacketBuffer _buf;
  size_t amount_of_crc_nibbles = 0;
  bool _check_crc;

public:
  PacketAccumulator(std::span<uint8_t> buffer, bool check_crc) : _raw_buffer(buffer), _buf(buffer), _check_crc(check_crc) {}

  std::optional<DsmrUnencryptedTelegram> process_byte(const uint8_t byte) {
    if (!_buf.has_space()) {
      Logger::log(LogLevel::DEBUG, "Buffer overflow. Discarding the accumulated data");
      _buf = DsmrPacketBuffer(_raw_buffer);
      _state = State::WaitingForPacketStartSymbol;
    }

    if (byte == '/') {
      Logger::log(LogLevel::VERBOSE, "Found telegram start symbol '/'");
      _buf = DsmrPacketBuffer(_raw_buffer);
      _buf.add(byte);
      _state = State::WaitingForPacketEndSymbol;
      return std::nullopt;
    }

    switch (_state) {
    case State::WaitingForPacketStartSymbol:
      return std::nullopt;

    case State::WaitingForPacketEndSymbol:
      _buf.add(byte);

      if (byte != '!') {
        return std::nullopt;
      }

      Logger::log(LogLevel::VERBOSE, "Found telegram end symbol '!'");
      if (!_check_crc) {
        _state = State::WaitingForPacketStartSymbol;
        Logger::log(LogLevel::VERBOSE, "Successfully received the telegram without CRC check");
        return DsmrUnencryptedTelegram::from_bytes(_buf.packet(), false);
      }

      _state = State::WaitingForCrc;
      amount_of_crc_nibbles = 0;
      return std::nullopt;

    case State::WaitingForCrc:
      amount_of_crc_nibbles++;
      _buf.add(byte);
      if (amount_of_crc_nibbles < 4) {
        return std::nullopt;
      }

      _state = State::WaitingForPacketStartSymbol;
      const auto res = DsmrUnencryptedTelegram::from_bytes(_buf.packet(), true);
      if (res) {
        Logger::log(LogLevel::VERBOSE, "Successfully received the telegram with correct CRC");
        return *res;
      }
      return std::nullopt;
    }

    // unreachable
    return std::nullopt;
  }
};

}
