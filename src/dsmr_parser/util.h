#pragma once

#include <array>
#include <cstdarg>
#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>
#if defined(_MSC_VER)
#include <sal.h>
#endif

namespace dsmr_parser {

class NonCopyable {
protected:
  NonCopyable() = default;
  ~NonCopyable() = default;

public:
  NonCopyable(NonCopyable&&) = default;
  NonCopyable& operator=(NonCopyable&&) = default;

  NonCopyable(const NonCopyable&) = delete;
  NonCopyable& operator=(const NonCopyable&) = delete;
};

class NonCopyableAndNonMovable : NonCopyable {
protected:
  NonCopyableAndNonMovable() = default;
  ~NonCopyableAndNonMovable() = default;

public:
  NonCopyableAndNonMovable(NonCopyableAndNonMovable&&) = delete;
  NonCopyableAndNonMovable& operator=(NonCopyableAndNonMovable&&) = delete;
};

// An OBIS ID like: "1-0:1.7.0.255"
struct ObisId final {
  std::array<uint8_t, 6> v{};
  constexpr explicit ObisId(const uint8_t a, const uint8_t b = 255, const uint8_t c = 255, const uint8_t d = 255, const uint8_t e = 255,
                            const uint8_t f = 255) noexcept
      : v{a, b, c, d, e, f} {};
  ObisId() = default;
  bool operator==(const ObisId&) const = default;
};

enum class LogLevel {
  VERY_VERBOSE,
  VERBOSE,
  DEBUG,
  INFO,
  WARNING,
  ERROR,
};

class Logger final {
public:
  static void set_log_function(std::function<void(LogLevel log_level, const char* fmt, va_list args)> func) { _log_function = std::move(func); }

#if defined(_MSC_VER)
  static void log(LogLevel log_level, _In_z_ _Printf_format_string_ const char* fmt, ...) {
#elif defined(__clang__) || defined(__GNUC__)
  __attribute__((format(printf, 2, 3))) static void log(LogLevel log_level, const char* fmt, ...) {
#else
  static void log(LogLevel log_level, const char* fmt, ...) {
#endif
    va_list args;
    va_start(args, fmt);
    _log_function(log_level, fmt, args);
    va_end(args);
  }

private:
  Logger() = default;

  inline static std::function<void(LogLevel log_level, const char* fmt, va_list args)> _log_function = [](LogLevel, const char*, va_list) {};
};

// Represents an unencrypted DSMR telegram that starts with '/' and ends with '!' with CRC (optional) at the end.
// Example:
//  "/AAA5MTR\r\n"
//  "\r\n"
//  "1-0:1.7.0(00.100*kW)\r\n"
//  "1-0:1.7.0(00.200*kW)\r\n"
//  "!ABCD";
class DsmrUnencryptedTelegram final {
  std::string_view data;
  bool has_crc_suffix_;
  explicit DsmrUnencryptedTelegram(std::string_view telegram, bool has_crc_suffix) : data(telegram), has_crc_suffix_(has_crc_suffix) {}

public:
  // Returns the telegram including the 4 CRC characters at the end if they were present in the input passed to from_bytes.
  std::string_view full_content() const { return data; }

  // Returns the telegram content without the trailing 4 CRC characters (ends with '!').
  std::string_view content_without_crc() const { return has_crc_suffix_ ? data.substr(0, data.size() - 4) : data; }

  static std::optional<DsmrUnencryptedTelegram> from_bytes(std::string_view telegram, const bool crc_check) {
    if (!telegram.starts_with('/')) {
      Logger::log(LogLevel::DEBUG, "Telegram does not start with '/'");
      return std::nullopt;
    }

    const auto end_pos = telegram.find('!');
    if (end_pos == std::string_view::npos) {
      Logger::log(LogLevel::DEBUG, "Telegram does not end with '!'");
      return std::nullopt;
    }

    const bool has_crc_suffix = end_pos + 5 <= telegram.size();

    if (!crc_check) {
      if (has_crc_suffix) {
        return DsmrUnencryptedTelegram(telegram.substr(0, end_pos + 5), true);
      }
      return DsmrUnencryptedTelegram(telegram.substr(0, end_pos + 1), false);
    }

    if (!has_crc_suffix) {
      Logger::log(LogLevel::DEBUG, "Telegram does not have enough bytes for CRC after '!'");
      return std::nullopt;
    }

    const auto crc_of_telegram = to_crc(telegram.substr(end_pos + 1, 4));
    if (!crc_of_telegram) {
      return std::nullopt;
    }

    const auto calculated_crc = calculate_crc16(telegram.substr(0, end_pos + 1));
    if (calculated_crc != *crc_of_telegram) {
      Logger::log(LogLevel::DEBUG, "CRC mismatch: received 0x%04X, calculated 0x%04X", *crc_of_telegram, calculated_crc);
      return std::nullopt;
    }

    return DsmrUnencryptedTelegram(telegram.substr(0, end_pos + 5), true);
  }

private:
  static uint16_t calculate_crc16(std::string_view telegram) {
    uint16_t crc = 0;
    for (std::size_t i = 0; i < telegram.size(); ++i) {
      crc ^= static_cast<uint8_t>(telegram[i]);
      for (std::size_t bit = 0; bit < 8; bit++) {
        if (crc & 1)
          crc = (crc >> 1) ^ 0xa001;
        else
          crc = (crc >> 1);
      }
    }
    return crc;
  }

  static std::optional<uint16_t> to_crc(std::string_view crc_str) {
    uint16_t crc = 0;
    for (char c : crc_str) {
      crc = static_cast<uint16_t>(crc << 4);
      if (c >= '0' && c <= '9') {
        crc = static_cast<uint16_t>(crc | (c - '0'));
      } else if (c >= 'A' && c <= 'F') {
        crc = static_cast<uint16_t>(crc | (c - 'A' + 10));
      } else if (c >= 'a' && c <= 'f') {
        crc = static_cast<uint16_t>(crc | (c - 'a' + 10));
      } else {
        Logger::log(LogLevel::DEBUG, "Incorrect CRC character '%c'", c);
        return std::nullopt;
      }
    }
    return crc;
  }
};

}
