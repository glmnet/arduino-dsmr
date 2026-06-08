#pragma once

#include "parser.h"
#include "util.h"
#include <algorithm>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>

#ifndef DSMR_GAS_MBUS_ID
#define DSMR_GAS_MBUS_ID 1
#endif
#ifndef DSMR_WATER_MBUS_ID
#define DSMR_WATER_MBUS_ID 2
#endif
#ifndef DSMR_THERMAL_MBUS_ID
#define DSMR_THERMAL_MBUS_ID 3
#endif
#ifndef DSMR_SUB_MBUS_ID
#define DSMR_SUB_MBUS_ID 4
#endif

namespace dsmr_parser {

template <typename T>
struct ParsedField : NonCopyableAndNonMovable {
  template <typename F>
  void apply(F& f) {
    f.apply(*static_cast<T*>(this));
  }
  static const char* unit() noexcept { return ""; }
};

template <typename T, size_t minlen, size_t maxlen>
struct StringField : ParsedField<T> {
  std::optional<std::string_view> parse(std::string_view input) {
    std::string_view sv;
    auto res = parse_string(sv, minlen, maxlen, input);
    if (res)
      static_cast<T*>(this)->val() = sv;
    return res;
  }
};

// Uses heuristics to detect if a string is hex-encoded or not.
template <typename T, size_t minlen, size_t maxlen>
struct HexStringOrRegularStringField : ParsedField<T> {
  std::optional<std::string_view> parse(std::string_view input) {
    std::string_view sv;
    auto res = parse_string(sv, minlen, maxlen, input);
    if (!res)
      return std::nullopt;

    static_cast<T*>(this)->val() = sv;

    auto is_hex_digit = [](char ch) { return (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F'); };
    auto is_alnum = [](char ch) { return (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z'); };

    // Only an even number of uppercase hex digits can be a hex coded string.
    if ((sv.length() & 1) || !std::ranges::all_of(sv, is_hex_digit))
      return res;

    auto value = [](char ch) { return ch <= '9' ? ch - '0' : ch - 'A' + 10; };

    hexStr.clear();
    hexStr.reserve(sv.length() / 2);
    for (size_t i = 0; i < sv.length(); i += 2) {
      const auto ch = static_cast<char>((value(sv[i]) << 4) + value(sv[i + 1]));
      if (!is_alnum(ch)) {
        // Decoded byte is not a digit or uppercase letter.
        hexStr.clear();
        return res;
      }
      hexStr.push_back(ch);
    }
    static_cast<T*>(this)->val() = std::string_view(hexStr);
    return res;
  }

private:
  std::string hexStr;
};

// A timestamp is essentially a string using YYMMDDhhmmssX format (where
// X is W or S for wintertime or summertime). Parsing this into a proper
// (UNIX) timestamp is hard to do generically. Parsing it into a
// single integer needs > 4 bytes top fit and isn't very useful (you
// cannot really do any calculation with those values). So we just parse
// into a string for now.
template <typename T>
struct TimestampField : StringField<T, 13, 13> {};

// Value that is parsed as a three-decimal float, but stored as an
// integer (by multiplying by 1000). Supports val() (or implicit cast to
// float) to get the original value, and int_val() to get the more
// efficient integer value. The unit() and int_unit() methods on
// FixedField return the corresponding units for these values.
struct FixedValue {
  operator float() const noexcept { return val(); }
  float val() const noexcept { return static_cast<float>(_value) / 1000.0f; }
  int32_t int_val() const noexcept { return _value; }

  int32_t _value;
};

// Floating point numbers in the message never have more than 3 decimal
// digits. To prevent inefficient floating point operations, we store
// them as a fixed-point number: an integer that stores the value in
// thousands. For example, a value of 1.234 kWh is stored as 1234. This
// effectively means that the integer value is the value in Wh. To allow
// automatic printing of these values, both the original unit and the
// integer unit is passed as a template argument.
template <typename T, const char* _unit, const char* _int_unit>
struct FixedField : ParsedField<T> {
  std::optional<std::string_view> parse(std::string_view input) {
    // Some smart meters publish int values instead of a float.
    // E.g. most meters would publish "1-0:1.8.0(000441.879*kWh)",
    // but some use "1-0:1.8.0(000441879*Wh)" instead.
    int32_t val;
    auto res = parse_float_or_int(val, 3, _unit, _int_unit, input);
    if (res)
      static_cast<T*>(this)->val()._value = val;
    return res;
  }

  static const char* unit() noexcept { return _unit; }
  static const char* int_unit() noexcept { return _int_unit; }
};

struct TimestampedFixedValue : public FixedValue {
  std::string_view timestamp;
};

// Some numerical values are prefixed with a timestamp. This is simply
// both of them concatenated, e.g. 0-1:24.2.1(150117180000W)(00473.789*m3)
template <typename T, const char* _unit, const char* _int_unit>
struct TimestampedFixedField : public FixedField<T, _unit, _int_unit> {
  std::optional<std::string_view> parse(std::string_view input) {
    std::string_view ts;
    auto res = parse_string(ts, 13, 13, input);
    if (!res)
      return std::nullopt;
    static_cast<T*>(this)->val().timestamp = ts;
    return FixedField<T, _unit, _int_unit>::parse(*res);
  }
};

// Take the last value of multiple parenthesized values
// e.g. 0-0:98.1.0(1)(1-0:1.6.0)(1-0:1.6.0)(230201000000W)(230117224500W)(04.329*kW)
template <typename T, const char* _unit, const char* _int_unit>
struct LastFixedField : public FixedField<T, _unit, _int_unit> {
  std::optional<std::string_view> parse(std::string_view input) {
    std::string_view last = input;
    std::string_view remaining = input;
    while (!remaining.empty()) {
      last = remaining;
      std::string_view sv;
      auto res = parse_string(sv, 1, 20, remaining);
      if (!res)
        return std::nullopt;
      remaining = *res;
    }
    return FixedField<T, _unit, _int_unit>::parse(last);
  }
};

// A integer number is just represented as an integer.
template <typename T, const char* _unit>
struct IntField : ParsedField<T> {
  std::optional<std::string_view> parse(std::string_view input) {
    int32_t val;
    auto res = parse_num(val, 0, _unit, input);
    if (res) {
      auto& dst = static_cast<T*>(this)->val();
      dst = static_cast<std::remove_reference_t<decltype(dst)>>(val);
    }
    return res;
  }

  static const char* unit() noexcept { return _unit; }
};

// Take the average of multiple timestamped values. Example:
//   0-0:98.1.0(2)(1-0:1.6.0)(1-0:1.6.0)(230201000000W)(230117224500W)(04.329*kW)(230202000000W)(230214224500W)(04.529*kW)
// Will produce an average between 4.329 and 4.529
template <typename T, const char* _unit, const char* _int_unit>
struct AveragedFixedField : public FixedField<T, _unit, _int_unit> {
  std::optional<std::string_view> parse(std::string_view input) {
    int32_t count;
    auto res = parse_num(count, 0, "", input);
    if (!res)
      return std::nullopt;

    if (count == 0) {
      static_cast<T*>(this)->val()._value = 0;
      return std::string_view{};
    }

    std::string_view sv;
    res = parse_string(sv, 1, 20, *res);
    if (!res)
      return std::nullopt;
    res = parse_string(sv, 1, 20, *res);
    if (!res)
      return std::nullopt;

    int32_t total = 0;
    for (int32_t i = 0; i < count; i++) {
      res = parse_string(sv, 1, 20, *res);
      if (!res)
        return std::nullopt;
      res = parse_string(sv, 1, 20, *res);
      if (!res)
        return std::nullopt;
      int32_t val;
      res = parse_float_or_int(val, 3, _unit, _int_unit, *res);
      if (!res)
        return std::nullopt;
      total += val;
    }

    static_cast<T*>(this)->val()._value = total / count;
    return res;
  }
};

// Raw field — no parsing, just store the entire value, including any parenthesis around it, as a string_view
template <typename T>
struct RawField : ParsedField<T> {
  std::optional<std::string_view> parse(std::string_view input) {
    static_cast<T*>(this)->val() = input;
    return std::string_view{};
  }
};

namespace fields {
struct units final {
  static inline constexpr char none[] = "";
  static inline constexpr char kWh[] = "kWh";
  static inline constexpr char Wh[] = "Wh";
  static inline constexpr char kW[] = "kW";
  static inline constexpr char W[] = "W";
  static inline constexpr char kV[] = "kV";
  static inline constexpr char V[] = "V";
  static inline constexpr char mV[] = "mV";
  static inline constexpr char kA[] = "kA";
  static inline constexpr char A[] = "A";
  static inline constexpr char mA[] = "mA";
  static inline constexpr char m3[] = "m3";
  static inline constexpr char dm3[] = "dm3";
  static inline constexpr char GJ[] = "GJ";
  static inline constexpr char MJ[] = "MJ";
  static inline constexpr char kvar[] = "kvar";
  static inline constexpr char var[] = "var";
  static inline constexpr char kvarh[] = "kvarh";
  static inline constexpr char varh[] = "varh";
  static inline constexpr char kVA[] = "kVA";
  static inline constexpr char VA[] = "VA";
  static inline constexpr char s[] = "s";
  static inline constexpr char Hz[] = "Hz";
  static inline constexpr char mHz[] = "mHz";
};

#define DEFINE_FIELD(fieldname, value_t, obis, field_t, ...)        \
  struct fieldname : field_t<fieldname __VA_OPT__(, __VA_ARGS__)> { \
    value_t fieldname;                                              \
    bool fieldname##_present = false;                               \
    static inline constexpr ObisId id = obis;                       \
    static inline constexpr char name[] = #fieldname;               \
    value_t& val() { return fieldname; }                            \
    bool& present() { return fieldname##_present; }                 \
  }

// Meter identification. This is not a normal field, but a specially-formatted first line of the message
DEFINE_FIELD(identification, std::string_view, ObisId(255, 255, 255, 255, 255, 255), RawField);

// Version information for P1 output
DEFINE_FIELD(p1_version, std::string_view, ObisId(1, 3, 0, 2, 8), StringField, 2, 2);
DEFINE_FIELD(p1_version_be, std::string_view, ObisId(0, 0, 96, 1, 4), StringField, 2, 96);

// Date-time stamp of the P1 message
DEFINE_FIELD(timestamp, std::string_view, ObisId(0, 0, 1, 0, 0), TimestampField);

// Equipment identifier
DEFINE_FIELD(equipment_id, std::string_view, ObisId(0, 0, 96, 1, 1), HexStringOrRegularStringField, 0, 96);

// Meter Reading electricity delivered to client (Special for Lux) in 0,001 kWh
// TODO: by OBIS 1-0:1.8.0.255 IEC 62056 it should be Positive active energy (A+) total [kWh], should we rename it?
DEFINE_FIELD(energy_delivered_lux, FixedValue, ObisId(1, 0, 1, 8, 0), FixedField, units::kWh, units::Wh);
// Meter Reading electricity delivered to client (Tariff 1) in 0,001 kWh
DEFINE_FIELD(energy_delivered_tariff1, FixedValue, ObisId(1, 0, 1, 8, 1), FixedField, units::kWh, units::Wh);
// Meter Reading electricity delivered to client (Tariff 2) in 0,001 kWh
DEFINE_FIELD(energy_delivered_tariff2, FixedValue, ObisId(1, 0, 1, 8, 2), FixedField, units::kWh, units::Wh);
// Meter Reading electricity delivered to client (Tariff 3) in 0,001 kWh
DEFINE_FIELD(energy_delivered_tariff3, FixedValue, ObisId(1, 0, 1, 8, 3), FixedField, units::kWh, units::Wh);
// Meter Reading electricity delivered to client (Tariff 4) in 0,001 kWh
DEFINE_FIELD(energy_delivered_tariff4, FixedValue, ObisId(1, 0, 1, 8, 4), FixedField, units::kWh, units::Wh);
// Meter Reading electricity delivered by client (Special for Lux) in 0,001 kWh
// TODO: by OBIS 1-0:2.8.0.255 IEC 62056 it should be Negative active energy (A+) total [kWh], should we rename it?
DEFINE_FIELD(energy_returned_lux, FixedValue, ObisId(1, 0, 2, 8, 0), FixedField, units::kWh, units::Wh);
// Meter Reading electricity delivered by client (Tariff 1) in 0,001 kWh
DEFINE_FIELD(energy_returned_tariff1, FixedValue, ObisId(1, 0, 2, 8, 1), FixedField, units::kWh, units::Wh);
// Meter Reading electricity delivered by client (Tariff 2) in 0,001 kWh
DEFINE_FIELD(energy_returned_tariff2, FixedValue, ObisId(1, 0, 2, 8, 2), FixedField, units::kWh, units::Wh);
// Meter Reading electricity delivered by client (Tariff 1) in 0,001 kWh
DEFINE_FIELD(energy_returned_tariff3, FixedValue, ObisId(1, 0, 2, 8, 3), FixedField, units::kWh, units::Wh);
// Meter Reading electricity delivered by client (Tariff 2) in 0,001 kWh
DEFINE_FIELD(energy_returned_tariff4, FixedValue, ObisId(1, 0, 2, 8, 4), FixedField, units::kWh, units::Wh);

// Extra fields used for Luxembourg and Lithuania
DEFINE_FIELD(total_imported_energy, FixedValue, ObisId(1, 0, 3, 8, 0), FixedField, units::kvarh, units::varh);
// Meter Reading Reactive energy delivered to client (Tariff 1) in 0,001 kvarh
DEFINE_FIELD(reactive_energy_delivered_tariff1, FixedValue, ObisId(1, 0, 3, 8, 1), FixedField, units::kvarh, units::varh);
// Meter Reading Reactive energy delivered to client (Tariff 2) in 0,001 kvarh
DEFINE_FIELD(reactive_energy_delivered_tariff2, FixedValue, ObisId(1, 0, 3, 8, 2), FixedField, units::kvarh, units::varh);
// Meter Reading Reactive energy delivered to client (Tariff 3) in 0,001 kvarh
DEFINE_FIELD(reactive_energy_delivered_tariff3, FixedValue, ObisId(1, 0, 3, 8, 3), FixedField, units::kvarh, units::varh);
// Meter Reading Reactive energy delivered to client (Tariff 4) in 0,001 kvarh
DEFINE_FIELD(reactive_energy_delivered_tariff4, FixedValue, ObisId(1, 0, 3, 8, 4), FixedField, units::kvarh, units::varh);

DEFINE_FIELD(total_exported_energy, FixedValue, ObisId(1, 0, 4, 8, 0), FixedField, units::kvarh, units::varh);
// Meter Reading Reactive energy delivered by client (Tariff 1) in 0,001 kvarh
DEFINE_FIELD(reactive_energy_returned_tariff1, FixedValue, ObisId(1, 0, 4, 8, 1), FixedField, units::kvarh, units::varh);
// Meter Reading Reactive energy delivered by client (Tariff 2) in 0,001 kvarh
DEFINE_FIELD(reactive_energy_returned_tariff2, FixedValue, ObisId(1, 0, 4, 8, 2), FixedField, units::kvarh, units::varh);
// Meter Reading Reactive energy delivered by client (Tariff 3) in 0,001 kvarh
DEFINE_FIELD(reactive_energy_returned_tariff3, FixedValue, ObisId(1, 0, 4, 8, 3), FixedField, units::kvarh, units::varh);
// Meter Reading Reactive energy delivered by client (Tariff 4) in 0,001 kvarh
DEFINE_FIELD(reactive_energy_returned_tariff4, FixedValue, ObisId(1, 0, 4, 8, 4), FixedField, units::kvarh, units::varh);

// Specific fields used for Switzerland
// Meter Reading electricity delivered to client (Tariff 1) in 0,001 kWh
DEFINE_FIELD(energy_delivered_tariff1_ch, FixedValue, ObisId(1, 1, 1, 8, 1), FixedField, units::kWh, units::Wh);
// Meter Reading electricity delivered to client (Tariff 2) in 0,001 kWh
DEFINE_FIELD(energy_delivered_tariff2_ch, FixedValue, ObisId(1, 1, 1, 8, 2), FixedField, units::kWh, units::Wh);
// Meter Reading electricity delivered by client (Tariff 1) in 0,001 kWh
DEFINE_FIELD(energy_returned_tariff1_ch, FixedValue, ObisId(1, 1, 2, 8, 1), FixedField, units::kWh, units::Wh);
// Meter Reading electricity delivered by client (Tariff 2) in 0,001 kWh
DEFINE_FIELD(energy_returned_tariff2_ch, FixedValue, ObisId(1, 1, 2, 8, 2), FixedField, units::kWh, units::Wh);

// Specific fields used for Israel
// Meter Reading electricity delivered to client (Tariff 1) in 0,001 kWh
DEFINE_FIELD(energy_delivered_tariff1_il, FixedValue, ObisId(1, 0, 1, 8, 11), FixedField, units::kWh, units::Wh);
// Meter Reading electricity delivered to client (Tariff 2) in 0,001 kWh
DEFINE_FIELD(energy_delivered_tariff2_il, FixedValue, ObisId(1, 0, 1, 8, 12), FixedField, units::kWh, units::Wh);
// Meter Reading electricity delivered to client (Tariff 3) in 0,001 kWh
DEFINE_FIELD(energy_delivered_tariff3_il, FixedValue, ObisId(1, 0, 1, 8, 13), FixedField, units::kWh, units::Wh);
// Meter Reading electricity delivered by client (Tariff 1) in 0,001 kWh
DEFINE_FIELD(energy_returned_tariff1_il, FixedValue, ObisId(1, 0, 2, 8, 11), FixedField, units::kWh, units::Wh);
// Meter Reading electricity delivered by client (Tariff 2) in 0,001 kWh
DEFINE_FIELD(energy_returned_tariff2_il, FixedValue, ObisId(1, 0, 2, 8, 12), FixedField, units::kWh, units::Wh);
// Meter Reading electricity delivered by client (Tariff 3) in 0,001 kWh
DEFINE_FIELD(energy_returned_tariff3_il, FixedValue, ObisId(1, 0, 2, 8, 13), FixedField, units::kWh, units::Wh);
// Tariff indicator electricity.
DEFINE_FIELD(electricity_tariff_il, std::string_view, ObisId(0, 0, 96, 14, 1), StringField, 2, 2);
// Power Failure Event Log (long power failures)
DEFINE_FIELD(electricity_failure_log_il, std::string_view, ObisId(1, 0, 99, 1, 0), RawField);

// Tariff indicator electricity. The tariff indicator can also be used
// to switch tariff-dependent loads, e.g., boilers. This is the responsibility of the P1 user.
// According to the P1 standard, this should be a 4-symbol string like "0001".
// However, some electricity providers put longer strings like "TARIFF 1".
DEFINE_FIELD(electricity_tariff, std::string_view, ObisId(0, 0, 96, 14, 0), StringField, 4, 32);

// Actual electricity power delivered (+P) in 1 Watt resolution
DEFINE_FIELD(power_delivered, FixedValue, ObisId(1, 0, 1, 7, 0), FixedField, units::kW, units::W);
// Actual electricity power received (-P) in 1 Watt resolution
DEFINE_FIELD(power_returned, FixedValue, ObisId(1, 0, 2, 7, 0), FixedField, units::kW, units::W);

// Extra fields used for Luxembourg and Lithuania
DEFINE_FIELD(reactive_power_delivered, FixedValue, ObisId(1, 0, 3, 7, 0), FixedField, units::kvar, units::var);
DEFINE_FIELD(reactive_power_returned, FixedValue, ObisId(1, 0, 4, 7, 0), FixedField, units::kvar, units::var);

// Specific fields used for Switzerland
// Actual electricity power delivered (+P) in 1 Watt resolution
DEFINE_FIELD(power_delivered_ch, FixedValue, ObisId(1, 1, 1, 7, 0), FixedField, units::kW, units::W);
// Actual electricity power received (-P) in 1 Watt resolution
DEFINE_FIELD(power_returned_ch, FixedValue, ObisId(1, 1, 2, 7, 0), FixedField, units::kW, units::W);

// The actual threshold Electricity in kW. Removed in 4.0.7 / 4.2.2 / 5.0
DEFINE_FIELD(electricity_threshold, FixedValue, ObisId(0, 0, 17, 0, 0), FixedField, units::kW, units::W);

// Switch position Electricity (in/out/enabled). Removed in 4.0.7 / 4.2.2 / 5.0
DEFINE_FIELD(electricity_switch_position, std::string_view, ObisId(0, 0, 96, 3, 10), StringField, 1, 32);

// Number of power failures in any phase
DEFINE_FIELD(electricity_failures, uint32_t, ObisId(0, 0, 96, 7, 21), IntField, units::none);
// Number of long power failures in any phase
DEFINE_FIELD(electricity_long_failures, uint32_t, ObisId(0, 0, 96, 7, 9), IntField, units::none);

// Power Failure Event Log (long power failures)
DEFINE_FIELD(electricity_failure_log, std::string_view, ObisId(1, 0, 99, 97, 0), RawField);

// Number of voltage sags in phase L1
DEFINE_FIELD(electricity_sags_l1, uint32_t, ObisId(1, 0, 32, 32, 0), IntField, units::none);
DEFINE_FIELD(voltage_sag_time_l1, uint32_t, ObisId(1, 0, 32, 33, 0), IntField, units::s);
DEFINE_FIELD(voltage_sag_l1, uint32_t, ObisId(1, 0, 32, 34, 0), IntField, units::V);

// Number of voltage sags in phase L2 (polyphase meters only)
DEFINE_FIELD(electricity_sags_l2, uint32_t, ObisId(1, 0, 52, 32, 0), IntField, units::none);
DEFINE_FIELD(voltage_sag_time_l2, uint32_t, ObisId(1, 0, 52, 33, 0), IntField, units::s);
DEFINE_FIELD(voltage_sag_l2, uint32_t, ObisId(1, 0, 52, 34, 0), IntField, units::V);

// Number of voltage sags in phase L3 (polyphase meters only)
DEFINE_FIELD(electricity_sags_l3, uint32_t, ObisId(1, 0, 72, 32, 0), IntField, units::none);
DEFINE_FIELD(voltage_sag_time_l3, uint32_t, ObisId(1, 0, 72, 33, 0), IntField, units::s);
DEFINE_FIELD(voltage_sag_l3, uint32_t, ObisId(1, 0, 72, 34, 0), IntField, units::V);

// Number of voltage swells in phase L1
DEFINE_FIELD(electricity_swells_l1, uint32_t, ObisId(1, 0, 32, 36, 0), IntField, units::none);
DEFINE_FIELD(voltage_swell_time_l1, uint32_t, ObisId(1, 0, 32, 37, 0), IntField, units::s);
DEFINE_FIELD(voltage_swell_l1, uint32_t, ObisId(1, 0, 32, 38, 0), IntField, units::V);

// Number of voltage swells in phase L2 (polyphase meters only)
DEFINE_FIELD(electricity_swells_l2, uint32_t, ObisId(1, 0, 52, 36, 0), IntField, units::none);
DEFINE_FIELD(voltage_swell_time_l2, uint32_t, ObisId(1, 0, 52, 37, 0), IntField, units::s);
DEFINE_FIELD(voltage_swell_l2, uint32_t, ObisId(1, 0, 52, 38, 0), IntField, units::V);

// Number of voltage swells in phase L3 (polyphase meters only)
DEFINE_FIELD(electricity_swells_l3, uint32_t, ObisId(1, 0, 72, 36, 0), IntField, units::none);
DEFINE_FIELD(voltage_swell_time_l3, uint32_t, ObisId(1, 0, 72, 37, 0), IntField, units::s);
DEFINE_FIELD(voltage_swell_l3, uint32_t, ObisId(1, 0, 72, 38, 0), IntField, units::V);

// Text message codes: numeric 8 digits (Note: Missing from 5.0 spec)
DEFINE_FIELD(message_short, std::string_view, ObisId(0, 0, 96, 13, 1), StringField, 0, 16);
// Text message max 2048 characters (Note: Spec says 1024 in comment and
// 2048 in format spec, so we stick to 2048).
DEFINE_FIELD(message_long, std::string_view, ObisId(0, 0, 96, 13, 0), StringField, 0, 2048);

// Instantaneous voltage L1 in 0.1V resolution (Note: Spec says V
// resolution in comment, but 0.1V resolution in format spec. Added in 5.0)
DEFINE_FIELD(voltage_l1, FixedValue, ObisId(1, 0, 32, 7, 0), FixedField, units::V, units::mV);
DEFINE_FIELD(voltage_avg_l1, FixedValue, ObisId(1, 0, 32, 24, 0), FixedField, units::V, units::mV);
// Instantaneous voltage L2 in 0.1V resolution (Note: Spec says V
// resolution in comment, but 0.1V resolution in format spec. Added in 5.0)
DEFINE_FIELD(voltage_l2, FixedValue, ObisId(1, 0, 52, 7, 0), FixedField, units::V, units::mV);
DEFINE_FIELD(voltage_avg_l2, FixedValue, ObisId(1, 0, 52, 24, 0), FixedField, units::V, units::mV);
// Instantaneous voltage L3 in 0.1V resolution (Note: Spec says V
// resolution in comment, but 0.1V resolution in format spec. Added in 5.0)
DEFINE_FIELD(voltage_l3, FixedValue, ObisId(1, 0, 72, 7, 0), FixedField, units::V, units::mV);
DEFINE_FIELD(voltage_avg_l3, FixedValue, ObisId(1, 0, 72, 24, 0), FixedField, units::V, units::mV);

// Instantaneous voltage (U) [V]
DEFINE_FIELD(voltage, FixedValue, ObisId(1, 0, 12, 7, 0), FixedField, units::V, units::mV);
// Frequency [Hz]
DEFINE_FIELD(frequency, FixedValue, ObisId(1, 0, 14, 7, 0), FixedField, units::Hz, units::mHz);
// Absolute active instantaneous power (|A|) [kW]
DEFINE_FIELD(abs_power, FixedValue, ObisId(1, 0, 15, 7, 0), FixedField, units::kW, units::W);

// Instantaneous current L1 in A resolution
DEFINE_FIELD(current_l1, FixedValue, ObisId(1, 0, 31, 7, 0), FixedField, units::A, units::mA);
DEFINE_FIELD(current_fuse_l1, FixedValue, ObisId(1, 0, 31, 4, 0), FixedField, units::A, units::mA);
// Instantaneous current L2 in A resolution
DEFINE_FIELD(current_l2, FixedValue, ObisId(1, 0, 51, 7, 0), FixedField, units::A, units::mA);
DEFINE_FIELD(current_fuse_l2, FixedValue, ObisId(1, 0, 51, 4, 0), FixedField, units::A, units::mA);
// Instantaneous current L3 in A resolution
DEFINE_FIELD(current_l3, FixedValue, ObisId(1, 0, 71, 7, 0), FixedField, units::A, units::mA);
DEFINE_FIELD(current_fuse_l3, FixedValue, ObisId(1, 0, 71, 4, 0), FixedField, units::A, units::mA);

// Instantaneous active power L1 (+P) in W resolution
DEFINE_FIELD(power_delivered_l1, FixedValue, ObisId(1, 0, 21, 7, 0), FixedField, units::kW, units::W);
// Instantaneous active power L2 (+P) in W resolution
DEFINE_FIELD(power_delivered_l2, FixedValue, ObisId(1, 0, 41, 7, 0), FixedField, units::kW, units::W);
// Instantaneous active power L3 (+P) in W resolution
DEFINE_FIELD(power_delivered_l3, FixedValue, ObisId(1, 0, 61, 7, 0), FixedField, units::kW, units::W);

// Instantaneous active power L1 (-P) in W resolution
DEFINE_FIELD(power_returned_l1, FixedValue, ObisId(1, 0, 22, 7, 0), FixedField, units::kW, units::W);
// Instantaneous active power L2 (-P) in W resolution
DEFINE_FIELD(power_returned_l2, FixedValue, ObisId(1, 0, 42, 7, 0), FixedField, units::kW, units::W);
// Instantaneous active power L3 (-P) in W resolution
DEFINE_FIELD(power_returned_l3, FixedValue, ObisId(1, 0, 62, 7, 0), FixedField, units::kW, units::W);

// Instantaneous current (I) [A]
DEFINE_FIELD(current, FixedValue, ObisId(1, 0, 11, 7, 0), FixedField, units::A, units::mA);
// Instantaneous current (I) in neutral [A]
DEFINE_FIELD(current_n, FixedValue, ObisId(1, 0, 91, 7, 0), FixedField, units::A, units::mA);
// Instantaneous sum of all phase current's  (I) [A]
DEFINE_FIELD(current_sum, FixedValue, ObisId(1, 0, 90, 7, 0), FixedField, units::A, units::mA);

// LUX and Lithuania

// IEC 62056 define the unit of reactive power as kvar. Some meters e.g. L+G E360 uses mixed case kVar
// Instantaneous reactive power L1 (+Q) in W resolution
DEFINE_FIELD(reactive_power_delivered_l1, FixedValue, ObisId(1, 0, 23, 7, 0), FixedField, units::kvar, units::var);
// Instantaneous reactive power L2 (+Q) in W resolution
DEFINE_FIELD(reactive_power_delivered_l2, FixedValue, ObisId(1, 0, 43, 7, 0), FixedField, units::kvar, units::var);
// Instantaneous reactive power L3 (+Q) in W resolution
DEFINE_FIELD(reactive_power_delivered_l3, FixedValue, ObisId(1, 0, 63, 7, 0), FixedField, units::kvar, units::var);
// Instantaneous reactive power L1 (-Q) in W resolution
DEFINE_FIELD(reactive_power_returned_l1, FixedValue, ObisId(1, 0, 24, 7, 0), FixedField, units::kvar, units::var);
// Instantaneous reactive power L2 (-Q) in W resolution
DEFINE_FIELD(reactive_power_returned_l2, FixedValue, ObisId(1, 0, 44, 7, 0), FixedField, units::kvar, units::var);
// Instantaneous reactive power L3 (-Q) in W resolution
DEFINE_FIELD(reactive_power_returned_l3, FixedValue, ObisId(1, 0, 64, 7, 0), FixedField, units::kvar, units::var);

// Apparent instantaneous power (+S) in kVA resolution
DEFINE_FIELD(apparent_delivery_power, FixedValue, ObisId(1, 0, 9, 7, 0), FixedField, units::kVA, units::VA);
// Apparent instantaneous power L1 (+S) in kVA resolution
DEFINE_FIELD(apparent_delivery_power_l1, FixedValue, ObisId(1, 0, 29, 7, 0), FixedField, units::kVA, units::VA);
// Apparent instantaneous power L2 (+S) in kVA resolution
DEFINE_FIELD(apparent_delivery_power_l2, FixedValue, ObisId(1, 0, 49, 7, 0), FixedField, units::kVA, units::VA);
// Apparent instantaneous power L3 (+S) in kVA resolution
DEFINE_FIELD(apparent_delivery_power_l3, FixedValue, ObisId(1, 0, 69, 7, 0), FixedField, units::kVA, units::VA);

// Apparent instantaneous power (-S) in kVA resolution
DEFINE_FIELD(apparent_return_power, FixedValue, ObisId(1, 0, 10, 7, 0), FixedField, units::kVA, units::VA);
// Apparent instantaneous power L1 (-S) in kVA resolution
DEFINE_FIELD(apparent_return_power_l1, FixedValue, ObisId(1, 0, 30, 7, 0), FixedField, units::kVA, units::VA);
// Apparent instantaneous power L2 (-S) in kVA resolution
DEFINE_FIELD(apparent_return_power_l2, FixedValue, ObisId(1, 0, 50, 7, 0), FixedField, units::kVA, units::VA);
// Apparent instantaneous power L3 (-S) in kVA resolution
DEFINE_FIELD(apparent_return_power_l3, FixedValue, ObisId(1, 0, 70, 7, 0), FixedField, units::kVA, units::VA);

// Active Demand Avg3 Plus in W resolution
DEFINE_FIELD(active_demand_power, FixedValue, ObisId(1, 0, 1, 24, 0), FixedField, units::kW, units::W);
// Active Demand Avg3 Net in W resolution
DEFINE_FIELD(active_demand_net, FixedValue, ObisId(1, 0, 16, 24, 0), FixedField, units::kW, units::W);
// Active Demand Avg3 Absolute  in W resolution
DEFINE_FIELD(active_demand_abs, FixedValue, ObisId(1, 0, 15, 24, 0), FixedField, units::kW, units::W);

// Device-Type
DEFINE_FIELD(gas_device_type, uint16_t, ObisId(0, DSMR_GAS_MBUS_ID, 24, 1, 0), IntField, units::none);

// Equipment identifier (Gas)
DEFINE_FIELD(gas_equipment_id, std::string_view, ObisId(0, DSMR_GAS_MBUS_ID, 96, 1, 0), HexStringOrRegularStringField, 0, 96);
// Equipment identifier (Gas) BE
DEFINE_FIELD(gas_equipment_id_be, std::string_view, ObisId(0, DSMR_GAS_MBUS_ID, 96, 1, 1), HexStringOrRegularStringField, 0, 96);

// Valve position Gas (on/off/released) (Note: Removed in 4.0.7 / 4.2.2 / 5.0).
DEFINE_FIELD(gas_valve_position, uint8_t, ObisId(0, DSMR_GAS_MBUS_ID, 24, 4, 0), IntField, units::none);

// Last 5-minute value (temperature converted), gas delivered to client
// in m3, including decimal values and capture time (Note: 4.x spec has "hourly value")
DEFINE_FIELD(gas_delivered, TimestampedFixedValue, ObisId(0, DSMR_GAS_MBUS_ID, 24, 2, 1), TimestampedFixedField, units::m3, units::dm3);
// Eneco in the Netherlands has smart meters for their district heating network, which uses the gas_delivered in GJ rather than m3
DEFINE_FIELD(gas_delivered_gj, TimestampedFixedValue, ObisId(0, DSMR_GAS_MBUS_ID, 24, 2, 1), TimestampedFixedField, units::GJ, units::MJ);
// _BE
DEFINE_FIELD(gas_delivered_be, TimestampedFixedValue, ObisId(0, DSMR_GAS_MBUS_ID, 24, 2, 3), TimestampedFixedField, units::m3, units::dm3);
DEFINE_FIELD(gas_delivered_text, std::string_view, ObisId(0, DSMR_GAS_MBUS_ID, 24, 3, 0), RawField);

// Device-Type
DEFINE_FIELD(thermal_device_type, uint16_t, ObisId(0, DSMR_THERMAL_MBUS_ID, 24, 1, 0), IntField, units::none);

// Equipment identifier (Thermal: heat or cold)
DEFINE_FIELD(thermal_equipment_id, std::string_view, ObisId(0, DSMR_THERMAL_MBUS_ID, 96, 1, 0), HexStringOrRegularStringField, 0, 96);

// Valve position (on/off/released) (Note: Removed in 4.0.7 / 4.2.2 / 5.0).
DEFINE_FIELD(thermal_valve_position, uint8_t, ObisId(0, DSMR_THERMAL_MBUS_ID, 24, 4, 0), IntField, units::none);

// Last 5-minute Meter reading Heat or Cold in 0,01 GJ and capture time
// (Note: 4.x spec has "hourly meter reading")
DEFINE_FIELD(thermal_delivered, TimestampedFixedValue, ObisId(0, DSMR_THERMAL_MBUS_ID, 24, 2, 1), TimestampedFixedField, units::GJ, units::MJ);

// Device-Type
DEFINE_FIELD(water_device_type, uint16_t, ObisId(0, DSMR_WATER_MBUS_ID, 24, 1, 0), IntField, units::none);

// Equipment identifier (Thermal: heat or cold)
DEFINE_FIELD(water_equipment_id, std::string_view, ObisId(0, DSMR_WATER_MBUS_ID, 96, 1, 0), HexStringOrRegularStringField, 0, 96);

// Valve position (on/off/released) (Note: Removed in 4.0.7 / 4.2.2 / 5.0).
DEFINE_FIELD(water_valve_position, uint8_t, ObisId(0, DSMR_WATER_MBUS_ID, 24, 4, 0), IntField, units::none);

// Last 5-minute Meter reading in 0,001 m3 and capture time
// (Note: 4.x spec has "hourly meter reading")
DEFINE_FIELD(water_delivered, TimestampedFixedValue, ObisId(0, DSMR_WATER_MBUS_ID, 24, 2, 1), TimestampedFixedField, units::m3, units::dm3);

// Device-Type
DEFINE_FIELD(sub_device_type, uint16_t, ObisId(0, DSMR_SUB_MBUS_ID, 24, 1, 0), IntField, units::none);

// Equipment identifier (Thermal: heat or cold)
DEFINE_FIELD(sub_equipment_id, std::string_view, ObisId(0, DSMR_SUB_MBUS_ID, 96, 1, 0), HexStringOrRegularStringField, 0, 96);

// Valve position (on/off/released) (Note: Removed in 4.0.7 / 4.2.2 / 5.0).
DEFINE_FIELD(sub_valve_position, uint8_t, ObisId(0, DSMR_SUB_MBUS_ID, 24, 4, 0), IntField, units::none);

// Last 5-minute Meter reading Heat or Cold and capture time (e.g. sub
// E meter) (Note: 4.x spec has "hourly meter reading")
DEFINE_FIELD(sub_delivered, TimestampedFixedValue, ObisId(0, DSMR_SUB_MBUS_ID, 24, 2, 1), TimestampedFixedField, units::m3, units::dm3);

// Extra fields used for Belgian capacity rate/peak consumption (cappaciteitstarief). Current quart-hourly energy consumption
DEFINE_FIELD(active_energy_import_current_average_demand, FixedValue, ObisId(1, 0, 1, 4, 0), FixedField, units::kW, units::W);
DEFINE_FIELD(active_energy_export_current_average_demand, FixedValue, ObisId(1, 0, 2, 4, 0), FixedField, units::kW, units::W);
DEFINE_FIELD(reactive_energy_import_current_average_demand, FixedValue, ObisId(1, 0, 3, 4, 0), FixedField, units::kvar, units::kvar);
DEFINE_FIELD(reactive_energy_export_current_average_demand, FixedValue, ObisId(1, 0, 4, 4, 0), FixedField, units::kvar, units::kvar);
DEFINE_FIELD(apparent_energy_import_current_average_demand, FixedValue, ObisId(1, 0, 9, 4, 0), FixedField, units::kVA, units::VA);
DEFINE_FIELD(apparent_energy_export_current_average_demand, FixedValue, ObisId(1, 0, 10, 4, 0), FixedField, units::kVA, units::VA);
DEFINE_FIELD(active_energy_import_last_completed_demand, FixedValue, ObisId(1, 0, 1, 5, 0), FixedField, units::kW, units::W);
DEFINE_FIELD(active_energy_export_last_completed_demand, FixedValue, ObisId(1, 0, 2, 5, 0), FixedField, units::kW, units::W);
DEFINE_FIELD(reactive_energy_import_last_completed_demand, FixedValue, ObisId(1, 0, 3, 5, 0), FixedField, units::kvar, units::kvar);
DEFINE_FIELD(reactive_energy_export_last_completed_demand, FixedValue, ObisId(1, 0, 4, 5, 0), FixedField, units::kvar, units::kvar);
DEFINE_FIELD(apparent_energy_import_last_completed_demand, FixedValue, ObisId(1, 0, 9, 5, 0), FixedField, units::kVA, units::VA);
DEFINE_FIELD(apparent_energy_export_last_completed_demand, FixedValue, ObisId(1, 0, 10, 5, 0), FixedField, units::kVA, units::VA);

// Maximum energy consumption from the current month
DEFINE_FIELD(active_energy_import_maximum_demand_running_month, TimestampedFixedValue, ObisId(1, 0, 1, 6, 0), TimestampedFixedField, units::kW, units::W);
// Maximum energy consumption from the last 13 months
DEFINE_FIELD(active_energy_import_maximum_demand_last_13_months, FixedValue, ObisId(0, 0, 98, 1, 0), AveragedFixedField, units::kW, units::W);

// Image Core Version and checksum
DEFINE_FIELD(fw_core_version, std::string_view, ObisId(1, 0, 0, 2, 0), StringField, 0, 96);
DEFINE_FIELD(fw_core_checksum, std::string_view, ObisId(1, 0, 0, 2, 8), StringField, 0, 96);
// Image Module Version and checksum
DEFINE_FIELD(fw_module_version, std::string_view, ObisId(1, 1, 0, 2, 0), StringField, 0, 96);
DEFINE_FIELD(fw_module_checksum, std::string_view, ObisId(1, 1, 0, 2, 8), StringField, 0, 96);

// Instantaneous power factor
DEFINE_FIELD(power_factor, FixedValue, ObisId(1, 0, 13, 7, 0), FixedField, units::none, units::none);
// Instantaneous power factor in phase L1, L2, L3
DEFINE_FIELD(power_factor_l1, FixedValue, ObisId(1, 0, 33, 7, 0), FixedField, units::none, units::none);
DEFINE_FIELD(power_factor_l2, FixedValue, ObisId(1, 0, 53, 7, 0), FixedField, units::none, units::none);
DEFINE_FIELD(power_factor_l3, FixedValue, ObisId(1, 0, 73, 7, 0), FixedField, units::none, units::none);
// Minimum Power factor
DEFINE_FIELD(min_power_factor, FixedValue, ObisId(1, 0, 13, 3, 0), FixedField, units::none, units::none);

// Measurement Period 3 for Instantaneous values
DEFINE_FIELD(period_3_for_instantaneous_values, uint32_t, ObisId(1, 0, 0, 8, 2), IntField, units::s);

}
}
