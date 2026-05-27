#pragma once

#include "util.h"
#include <cctype>
#include <optional>
#include <string_view>

namespace dsmr_parser {

// ParsedData is a template for the result of parsing a DSMR telegram.
// You pass the fields you want to add to it as template arguments.
// Each field becomes a base class, exposing its member variable directly.
template <typename... Ts>
struct ParsedData final : Ts... {
  std::optional<std::string_view> parse_line(const ObisId& obis_id, std::string_view input) {
    std::optional<std::string_view> res = input;
    auto try_field = [&](auto& field) -> bool {
      using F = std::remove_reference_t<decltype(field)>;
      if (!(F::id == obis_id))
        return false;
      if (field.present()) {
        Logger::log(LogLevel::ERROR, "Duplicate field [%.*s]", static_cast<int>(input.size()), input.data());
        res = std::nullopt;
      } else {
        field.present() = true;
        res = field.parse(input);
      }
      return true;
    };
    (void)try_field;
    (void)(try_field(static_cast<Ts&>(*this)) || ...);
    return res;
  }

  bool all_present() { return (Ts::present() && ...); }
};

// Parse a parenthesized string: (content)
// Handles double-closing brackets like ((ER11))
inline std::optional<std::string_view> parse_string(std::string_view& out, size_t min, size_t max, std::string_view input) {
  if (input.empty() || input.front() != '(') {
    Logger::log(LogLevel::ERROR, "Missing ( '%.*s'", static_cast<int>(input.size()), input.data());
    return std::nullopt;
  }

  size_t pos = 1;
  while (pos < input.size() && input[pos] != ')')
    ++pos;

  // Handle )) at the end — include the first ) in the string
  if (pos + 1 < input.size() && input[pos + 1] == ')')
    ++pos;

  if (pos == input.size()) {
    Logger::log(LogLevel::ERROR, "Missing ) '%.*s'", static_cast<int>(input.size()), input.data());
    return std::nullopt;
  }

  auto len = pos - 1;
  if (len < min || len > max) {
    Logger::log(LogLevel::ERROR, "Invalid string length '%.*s'", static_cast<int>(input.size()), input.data());
    return std::nullopt;
  }

  out = input.substr(1, len);
  return input.substr(pos + 1);
}

// Parse a numeric value in parentheses: ([-]digits[.decimals][*unit])
inline std::optional<std::string_view> parse_num(int32_t& out, size_t max_decimals, const char* unit, std::string_view input, bool log_errors = true) {
  if (input.empty() || input.front() != '(') {
    if (log_errors)
      Logger::log(LogLevel::ERROR, "Missing ( '%.*s'", static_cast<int>(input.size()), input.data());
    return std::nullopt;
  }

  size_t p = 1;
  bool negative = (p < input.size() && input[p] == '-');
  if (negative)
    ++p;

  int32_t value = 0;

  while (p < input.size() && input[p] != '.' && input[p] != '*' && input[p] != ')') {
    if (input[p] < '0' || input[p] > '9') {
      if (log_errors)
        Logger::log(LogLevel::ERROR, "Invalid number '%.*s'", static_cast<int>(input.size()), input.data());
      return std::nullopt;
    }
    value = value * 10 + (input[p] - '0');
    ++p;
  }

  size_t remaining = max_decimals;
  if (remaining && p < input.size() && input[p] == '.') {
    ++p;
    while (p < input.size() && input[p] != '*' && input[p] != ')' && remaining) {
      if (input[p] < '0' || input[p] > '9') {
        if (log_errors)
          Logger::log(LogLevel::ERROR, "Invalid number '%.*s'", static_cast<int>(input.size()), input.data());
        return std::nullopt;
      }
      value = value * 10 + (input[p] - '0');
      --remaining;
      ++p;
    }
  }

  while (remaining--)
    value *= 10;

  if (unit && *unit) {
    // Value 0 allows missing unit (workaround for some meters)
    if (value == 0 && (p >= input.size() || (input[p] != '*' && input[p] != '.'))) {
      auto close = input.find(')', p);
      p = (close != std::string_view::npos) ? close : input.size();
    } else {
      if (p >= input.size() || input[p] != '*') {
        if (log_errors)
          Logger::log(LogLevel::ERROR, "Missing unit '%.*s'", static_cast<int>(input.size()), input.data());
        return std::nullopt;
      }
      ++p;
      const char* u = unit;
      while (p < input.size() && input[p] != ')' && *u) {
        if (std::tolower(static_cast<unsigned char>(input[p])) != std::tolower(static_cast<unsigned char>(*u))) {
          if (log_errors)
            Logger::log(LogLevel::ERROR, "Invalid unit '%.*s'", static_cast<int>(input.size()), input.data());
          return std::nullopt;
        }
        ++p;
        ++u;
      }
      if (*u) {
        if (log_errors)
          Logger::log(LogLevel::ERROR, "Invalid unit '%.*s'", static_cast<int>(input.size()), input.data());
        return std::nullopt;
      }
    }
  }

  if (p >= input.size() || input[p] != ')') {
    if (log_errors)
      Logger::log(LogLevel::ERROR, "Extra data '%.*s'", static_cast<int>(input.size()), input.data());
    return std::nullopt;
  }

  out = negative ? -value : value;
  return input.substr(p + 1);
}

// Try float unit first, fall back to integer unit
inline std::optional<std::string_view> parse_float_or_int(int32_t& out, size_t max_decimals, const char* float_unit, const char* int_unit,
                                                          std::string_view input) {
  auto res = parse_num(out, max_decimals, float_unit, input, /*log_errors=*/false);
  if (res)
    return res;
  return parse_num(out, 0, int_unit, input);
}

// Parse OBIS identifier (a-b:c.d.e.f)
inline std::optional<std::string_view> parse_obis(ObisId& id, std::string_view input) {
  size_t pos = 0;
  uint8_t part = 0;
  while (pos < input.size()) {
    char c = input[pos];
    if (c >= '0' && c <= '9') {
      auto digit = static_cast<uint8_t>(c - '0');
      if (id.v[part] > 25 || (id.v[part] == 25 && digit > 5)) {
        Logger::log(LogLevel::ERROR, "Obis ID has number over 255 '%.*s'", static_cast<int>(input.size()), input.data());
        return std::nullopt;
      }
      id.v[part] = static_cast<uint8_t>(id.v[part] * 10 + digit);
    } else if (part == 0 && c == '-') {
      part++;
    } else if (part == 1 && c == ':') {
      part++;
    } else if (part > 1 && part < 5 && c == '.') {
      part++;
    } else {
      break;
    }
    ++pos;
  }

  if (pos == 0) {
    Logger::log(LogLevel::ERROR, "OBIS id Empty '%.*s'", static_cast<int>(input.size()), input.data());
    return std::nullopt;
  }

  for (++part; part < 6; ++part)
    id.v[part] = 255;

  return input.substr(pos);
}

struct DsmrParser final {
private:
  template <typename Data>
  static bool parse_line(Data& data, std::string_view input, bool unknown_error) {
    if (input.empty())
      return true;

    ObisId id;
    auto idres = parse_obis(id, input);
    if (!idres)
      return false;

    auto datares = data.parse_line(id, *idres);
    if (!datares)
      return false;

    if ((*datares).data() != (*idres).data() && !(*datares).empty()) {
      Logger::log(LogLevel::ERROR, "Trailing characters on data line '%.*s'", static_cast<int>(input.size()), input.data());
      return false;
    }
    if ((*datares).data() == (*idres).data() && unknown_error) {
      Logger::log(LogLevel::ERROR, "Unknown field '%.*s'", static_cast<int>(input.size()), input.data());
      return false;
    }

    return true;
  }

public:
  template <typename... Ts>
  static bool parse(ParsedData<Ts...>& data, DsmrUnencryptedTelegram telegram, bool unknown_error = false) {
    // Strip leading '/' and trailing '!'
    auto input = telegram.content_without_crc().substr(1, telegram.content_without_crc().size() - 2);

    size_t pos = 0;
    size_t line_start = 0;

    // Parse ID line
    while (pos < input.size()) {
      if (input[pos] == '\r' || input[pos] == '\n') {
        auto res = data.parse_line(ObisId(255, 255, 255, 255, 255, 255), input.substr(line_start, pos - line_start));
        if (!res)
          return false;
        line_start = ++pos;
        break;
      }
      ++pos;
    }

    // Parse data lines — track brackets to handle multi-line values
    // and double brackets like ((ER11))
    bool open_bracket = false;
    while (pos < input.size()) {
      char c = input[pos];
      char nc = (pos + 1 < input.size()) ? input[pos + 1] : '\0';

      if ((c == '(' && nc == '(') || (c == ')' && nc == ')')) {
        ++pos;
        c = nc;
      }

      if (c == '(') {
        if (open_bracket) {
          Logger::log(LogLevel::ERROR, "Unexpected '(' symbol");
          return false;
        }
        open_bracket = true;
      } else if (c == ')') {
        if (!open_bracket) {
          Logger::log(LogLevel::ERROR, "Unexpected ')' symbol");
          return false;
        }
        open_bracket = false;
      } else if (c == '\r' || c == '\n') {
        bool continuation = open_bracket || ((input.size() - pos > 2) && (input[pos + 1] == '(' || input[pos + 2] == '('));
        if (!continuation) {
          if (!parse_line(data, input.substr(line_start, pos - line_start), unknown_error))
            return false;
          line_start = pos + 1;
        }
      }

      ++pos;
    }

    if (pos != line_start) {
      Logger::log(LogLevel::ERROR, "Last dataline not CRLF terminated");
      return false;
    }

    return true;
  }
};
}
