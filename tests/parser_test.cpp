#include "dsmr_parser/fields.h"
#include "dsmr_parser/parser.h"
#include "test_util.h"
#include <doctest.h>
#include <iostream>

using namespace dsmr_parser;
using namespace fields;

TEST_CASE_FIXTURE(LogFixture, "Should parse all fields in the DSMR message correctly") {
  const auto& msg = "/KFM5KAIFA-METER\r\n"
                    "\r\n"
                    "1-3:0.2.8(40)\r\n"
                    "0-0:1.0.0(150117185916W)\r\n"
                    "0-0:96.1.1(0000000000000000000000000000000000)\r\n"
                    "1-0:1.8.1(000671.578*kWh)\r\n"
                    "1-0:1.8.2(000842.472*kWh)\r\n"
                    "1-0:2.8.1(000000.000*kWh)\r\n"
                    "1-0:2.8.2(000000.000*kWh)\r\n"
                    "1-0:1.8.11(007132.419*kWh)\r\n"
                    "1-0:1.8.12(000155.482*kWh)\r\n"
                    "1-0:1.8.13(025605.254*kWh)\r\n"
                    "1-0:2.8.11(000000.000*kWh)\r\n"
                    "1-0:2.8.12(000000.000*kWh)\r\n"
                    "1-0:2.8.13(000000.000*kWh)\r\n"
                    "0-0:96.14.0(Some long string 32 bytes)\r\n"
                    "0-0:96.14.1(03)\r\n"
                    "1-0:1.7.0(00.333*kW)\r\n"
                    "1-0:2.7.0(00.000*kW)\r\n"
                    "0-0:17.0.0(999.9*kW)\r\n"
                    "0-0:96.3.10(on)\r\n"
                    "0-0:96.7.21(00008)\r\n"
                    "0-0:96.7.9(00007)\r\n"
                    "1-0:99.97.0(1)(0-0:96.7.19)(000101000001W)(2147483647*s)\r\n"
                    "0-0:98.1.0(2)(1-0:1.6.0)(1-0:1.6.0)(230201000000W)(230117224500W)(04.329*kW)(230202000000W)(230214224500W)(04529*W)\r\n"
                    "1-0:99.1.0(1)(0-0:96.10.7)(1-0:1.29.0)(1-0:2.29.0)(260411233000S)(00)(000000.205*kWh)(000000.000*kWh)\r\n"
                    "1-0:32.32.0(00000)\r\n"
                    "1-0:32.36.0(00000)\r\n"
                    "0-0:96.13.1()\r\n"
                    "0-0:96.13.0()\r\n"
                    "1-0:14.7.0(49.9*Hz)\r\n"
                    "1-0:32.7.0(234.0*V)\r\n"
                    "1-0:52.7.0(231.0*V)\r\n"
                    "1-0:72.7.0(231.0*V)\r\n"
                    "1-0:31.7.0(001*A)\r\n"
                    "1-0:51.7.0(002.4*A)\r\n"
                    "1-0:71.7.0(000.0*A)\r\n"
                    "1-0:21.7.0(00.332*kW)\r\n"
                    "1-0:22.7.0(00.000*kW)\r\n"
                    "1-0:41.7.0(00.430*kW)\r\n"
                    "1-0:42.7.0(00.000*kW)\r\n"
                    "1-0:61.7.0(00.000*kW)\r\n"
                    "1-0:62.7.0(00.000*kW)\r\n"
                    "0-1:24.1.0(003)\r\n"
                    "0-1:96.1.0(0000000000000000000000000000000000)\r\n"
                    "0-1:24.2.1(150117180000W)(00473.789*m3)\r\n"
                    "1-0:0.2.0((ER11))\r\n"
                    "1-0:0.2.8(1.0.smth smth-123)\r\n"
                    "1-1:0.2.0((ER12)\r\n"
                    "1-1:0.2.8(ER13))\r\n"
                    "0-1:24.4.0(1)\r\n"
                    "1-0:16.24.0(-03.618*kW)\r\n"
                    "1-0:13.7.0(0.998)\r\n"
                    "1-0:33.7.0(0.975)\r\n"
                    "1-0:53.7.0(0.963)\r\n"
                    "1-0:73.7.0(0.987)\r\n"
                    "1-0:13.3.0(0.000)\r\n"
                    "1-0:0.8.2(00900*s)\r\n"
                    "!";

  ParsedData<
      /* String */ identification,
      /* String */ p1_version,
      /* String */ timestamp,
      /* String */ equipment_id,
      /* FixedValue */ energy_delivered_tariff1,
      /* FixedValue */ energy_delivered_tariff2,
      /* FixedValue */ energy_returned_tariff1,
      /* FixedValue */ energy_returned_tariff2,
      /* FixedValue */ energy_delivered_tariff1_il,
      /* FixedValue */ energy_delivered_tariff2_il,
      /* FixedValue */ energy_delivered_tariff3_il,
      /* FixedValue */ energy_returned_tariff1_il,
      /* FixedValue */ energy_returned_tariff2_il,
      /* FixedValue */ energy_returned_tariff3_il,
      /* String */ electricity_tariff,
      /* String */ electricity_tariff_il,
      /* FixedValue */ power_delivered,
      /* FixedValue */ power_returned,
      /* FixedValue */ frequency,
      /* FixedValue */ electricity_threshold,
      /* String */ electricity_switch_position,
      /* uint32_t */ electricity_failures,
      /* uint32_t */ electricity_long_failures,
      /* String */ electricity_failure_log,
      /* String */ electricity_failure_log_il,
      /* uint32_t */ electricity_sags_l1,
      /* uint32_t */ electricity_sags_l2,
      /* uint32_t */ electricity_sags_l3,
      /* uint32_t */ electricity_swells_l1,
      /* uint32_t */ electricity_swells_l2,
      /* uint32_t */ electricity_swells_l3,
      /* String */ message_short,
      /* String */ message_long,
      /* FixedValue */ voltage_l1,
      /* FixedValue */ voltage_l2,
      /* FixedValue */ voltage_l3,
      /* FixedValue */ current_l1,
      /* FixedValue */ current_l2,
      /* FixedValue */ current_l3,
      /* FixedValue */ power_delivered_l1,
      /* FixedValue */ power_delivered_l2,
      /* FixedValue */ power_delivered_l3,
      /* FixedValue */ power_returned_l1,
      /* FixedValue */ power_returned_l2,
      /* FixedValue */ power_returned_l3,
      /* uint16_t */ gas_device_type,
      /* String */ gas_equipment_id,
      /* uint8_t */ gas_valve_position,
      /* TimestampedFixedValue */ gas_delivered,
      /* uint16_t */ thermal_device_type,
      /* String */ thermal_equipment_id,
      /* uint8_t */ thermal_valve_position,
      /* TimestampedFixedValue */ thermal_delivered,
      /* uint16_t */ water_device_type,
      /* String */ water_equipment_id,
      /* uint8_t */ water_valve_position,
      /* TimestampedFixedValue */ water_delivered,
      /* AveragedFixedField */ active_energy_import_maximum_demand_last_13_months,
      /* String */ fw_core_version,
      /* String */ fw_core_checksum,
      /* String */ fw_module_version,
      /* String */ fw_module_checksum,
      /* FixedValue */ active_demand_net,
      /* FixedValue */ power_factor,
      /* FixedValue */ power_factor_l1,
      /* FixedValue */ power_factor_l2,
      /* FixedValue */ power_factor_l3,
      /* FixedValue */ min_power_factor,
      /* FixedValue */ period_3_for_instantaneous_values>
      data;

  auto res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, /* crc_check */ false), /* unknown_error */ true);
  REQUIRE(res);

  // Check that all fields have correct values
  REQUIRE(data.identification == "KFM5KAIFA-METER");
  REQUIRE(data.p1_version == "40");
  REQUIRE(data.timestamp == "150117185916W");
  REQUIRE(data.equipment_id == "0000000000000000000000000000000000");
  REQUIRE(data.energy_delivered_tariff1 == 671.578f);
  REQUIRE(data.energy_delivered_tariff2 == 842.472f);
  REQUIRE(data.energy_returned_tariff1 == 0.0f);
  REQUIRE(data.energy_returned_tariff2 == 0.0f);
  REQUIRE(data.electricity_tariff == "Some long string 32 bytes");
  REQUIRE(data.electricity_tariff_il == "03");
  REQUIRE(data.energy_delivered_tariff1_il == 7132.419f);
  REQUIRE(data.energy_delivered_tariff2_il == 155.482f);
  REQUIRE(data.energy_delivered_tariff3_il == 25605.254f);
  REQUIRE(data.energy_returned_tariff1_il == 0.0f);
  REQUIRE(data.energy_returned_tariff2_il == 0.0f);
  REQUIRE(data.energy_returned_tariff3_il == 0.0f);
  REQUIRE(data.power_delivered == 0.333f);
  REQUIRE(data.power_returned == 0.0f);
  REQUIRE(data.electricity_threshold == 999.9f);
  REQUIRE(data.electricity_switch_position == "on");
  REQUIRE(data.electricity_failures == 8);
  REQUIRE(data.electricity_long_failures == 7);
  REQUIRE(data.electricity_failure_log == "(1)(0-0:96.7.19)(000101000001W)(2147483647*s)");
  REQUIRE(data.electricity_failure_log_il == "(1)(0-0:96.10.7)(1-0:1.29.0)(1-0:2.29.0)(260411233000S)(00)(000000.205*kWh)(000000.000*kWh)");
  REQUIRE(data.electricity_sags_l1 == 0);
  REQUIRE(data.electricity_swells_l1 == 0);
  REQUIRE(data.message_short.empty());
  REQUIRE(data.message_long.empty());
  REQUIRE(data.voltage_l1 == 234.0f);
  REQUIRE(data.voltage_l2 == 231.0f);
  REQUIRE(data.voltage_l3 == 231.0f);
  REQUIRE(data.current_l1 == 1.0f);
  REQUIRE(data.current_l2 == 2.4f);
  REQUIRE(data.current_l3 == 0.0f);
  REQUIRE(data.power_delivered_l1 == 0.332f);
  REQUIRE(data.power_delivered_l2 == 0.430f);
  REQUIRE(data.power_delivered_l3 == 0.0f);
  REQUIRE(data.power_returned_l1 == 0.0f);
  REQUIRE(data.power_returned_l2 == 0.0f);
  REQUIRE(data.frequency == 49.9f);
  REQUIRE(data.power_returned_l3 == 0.0f);
  REQUIRE(data.gas_device_type == 3);
  REQUIRE(data.gas_equipment_id == "0000000000000000000000000000000000");
  REQUIRE(data.gas_valve_position == 1);
  REQUIRE(data.gas_delivered == 473.789f);
  REQUIRE(data.active_energy_import_maximum_demand_last_13_months.val() == 4.429f);
  REQUIRE(data.fw_core_version == "(ER11)");
  REQUIRE(data.fw_core_checksum == "1.0.smth smth-123");
  REQUIRE(data.fw_module_version == "(ER12");
  REQUIRE(data.fw_module_checksum == "ER13)");
  REQUIRE(data.active_demand_net == -3.618f);
  REQUIRE(data.power_factor == 0.998f);
  REQUIRE(data.power_factor_l1 == 0.975f);
  REQUIRE(data.power_factor_l2 == 0.963f);
  REQUIRE(data.power_factor_l3 == 0.987f);
  REQUIRE(data.min_power_factor == 0.0f);
  REQUIRE(data.period_3_for_instantaneous_values == 900);
}

TEST_CASE_FIXTURE(LogFixture, "Should parse Wh-based integers for FixedField (fallback int_unit path)") {
  const auto& msg = "/ABC5MTR\r\n"
                    "\r\n"
                    "1-0:1.8.0(000441879*Wh)\r\n"
                    "!";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ energy_delivered_lux>
      data;

  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE(res);
  REQUIRE(data.energy_delivered_lux == 441.879f); // 441,879 Wh => 441.879 kWh
  REQUIRE(fields::energy_delivered_lux::unit() == std::string("kWh"));
  REQUIRE(fields::energy_delivered_lux::int_unit() == std::string("Wh"));
}

TEST_CASE_FIXTURE(LogFixture, "Should parse TimestampedFixedField for gas_delivered_be and expose timestamp") {
  const auto& msg = "/DEF5MTR\r\n"
                    "\r\n"
                    "0-1:24.2.3(230101120000W)(00012.345*m3)\r\n"
                    "!";

  ParsedData<
      /* String */ identification,
      /* TimestampedFixedValue */ gas_delivered_be>
      data;

  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE(res);
  REQUIRE(data.gas_delivered_be == 12.345f);
  REQUIRE(data.gas_delivered_be.timestamp == "230101120000W");
}

TEST_CASE_FIXTURE(LogFixture, "Should take the last value with LastFixedField (capacity rate history)") {
  const auto& msg = "/KFM5MTR\r\n"
                    "\r\n"
                    "0-0:98.1.0(1)(1-0:1.6.0)(1-0:1.6.0)(230201000000W)(230117224500W)(04.329*kW)\r\n"
                    "!";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ active_energy_import_maximum_demand_last_13_months>
      data;

  DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE(data.active_energy_import_maximum_demand_last_13_months == 4.329f);
}

TEST_CASE_FIXTURE(LogFixture, "Should detect duplicate fields") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-0:1.7.0(00.100*kW)\r\n"
                    "1-0:1.7.0(00.200*kW)\r\n"
                    "!";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ power_delivered>
      data;

  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE_FALSE(res);
  REQUIRE(log.contains("Duplicate field"));
  REQUIRE(log.contains("(00.200*kW)"));
}

TEST_CASE_FIXTURE(LogFixture, "Should error on unknown field when unknown_error is true") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-0:2.7.0(00.000*kW)\r\n" // power_returned not part of ParsedData below
                    "!";

  ParsedData<
      /* String */ identification>
      data;

  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false), /*unknown_error=*/true);
  REQUIRE_FALSE(res);
  REQUIRE(log.contains("Unknown field"));
  REQUIRE(log.contains("1-0:2.7.0(00.000*kW)"));
}

TEST_CASE_FIXTURE(LogFixture, "Should report OBIS ID numbers over 255") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "256-0:1.7.0(00.100*kW)\r\n" // invalid OBIS (256)
                    "!";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ power_delivered>
      data;

  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE_FALSE(res);
  REQUIRE(log.contains("Obis ID has number over 255"));
  REQUIRE(log.contains("6-0:1.7.0(00.100*kW)"));
}

TEST_CASE_FIXTURE(LogFixture, "Should validate string length bounds (p1_version too short)") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-3:0.2.8(4)\r\n" // p1_version expects 2 chars
                    "!";

  ParsedData<
      /* String */ identification,
      /* String */ p1_version>
      data;

  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE_FALSE(res);
  REQUIRE(log.contains("Invalid string length"));
  REQUIRE(log.contains("4)"));
}

TEST_CASE_FIXTURE(LogFixture, "Should validate string length bounds (p1_version too long)") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-3:0.2.8(123)\r\n" // p1_version expects 2 chars
                    "!";

  ParsedData<
      /* String */ identification,
      /* String */ p1_version>
      data;

  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE_FALSE(res);
  REQUIRE(log.contains("Invalid string length"));
  REQUIRE(log.contains("123)"));
}

TEST_CASE_FIXTURE(LogFixture, "Should validate units for numeric fields") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-0:1.7.0(00.318*kVA)\r\n" // expects kW, not kVA
                    "!";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ power_delivered>
      data;

  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE_FALSE(res);
  REQUIRE(log.contains("Missing unit"));
  REQUIRE(log.contains("kVA)"));
}

TEST_CASE_FIXTURE(LogFixture, "Should report missing closing parenthesis for StringField") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-3:0.2.8(40\r\n" // missing ')'
                    "!";

  ParsedData<
      /* String */ identification,
      /* String */ p1_version>
      data;

  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE_FALSE(res);
  REQUIRE(log.contains("Last dataline not CRLF terminated"));
}

TEST_CASE_FIXTURE(LogFixture, "Should compute FixedField with decimals and millivolt int_unit correctly") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-0:32.7.0(230.1*V)\r\n" // voltage_l1 (V / mV)
                    "!";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ voltage_l1>
      data;

  DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE(data.voltage_l1 == 230.1f);
}

TEST_CASE_FIXTURE(LogFixture, "all_present() should reflect presence of all requested fields") {
  SUBCASE("All fields present -> true") {
    const auto& msg = "/AAA5MTR\r\n"
                      "\r\n"
                      "1-0:1.7.0(00.123*kW)\r\n"
                      "!";

    ParsedData<
        /* String */ identification,
        /* FixedValue */ power_delivered>
        data;

    DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
    REQUIRE(data.all_present());
  }

  SUBCASE("Missing a requested field -> false") {
    const auto& msg = "/AAA5MTR\r\n"
                      "\r\n"
                      "!";

    ParsedData<
        /* String */ identification,
        /* FixedValue */ power_delivered>
        data;

    DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
    REQUIRE_FALSE(data.all_present());
  }
}

TEST_CASE_FIXTURE(LogFixture, "Should report last dataline not CRLF terminated") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-0:1.7.0(00.123*kW)" // no CRLF before '!'
                    "!";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ power_delivered>
      data;

  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE_FALSE(res);
  REQUIRE(log.contains("Last dataline not CRLF terminated"));
}

TEST_CASE_FIXTURE(LogFixture, "Doesn't crash for a small packet") {
  const auto& msg = "/!";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ power_delivered>
      data;

  auto res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE(res);
}

TEST_CASE_FIXTURE(LogFixture, "Doesn't crash for a small packet 2") {
  const auto& msg = "/a!";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ power_delivered>
      data;

  auto res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE_FALSE(res);
  REQUIRE(log.contains("Last dataline not CRLF terminated"));
}

TEST_CASE_FIXTURE(LogFixture, "Trailing characters on data line") {
  const auto& msg = "/AAA5MTR\r\n\r\n"
                    "1-0:1.7.0(00.123*kW) trailing\r\n"
                    "!";
  ParsedData</*String*/ identification, /*FixedValue*/ power_delivered> data;
  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE_FALSE(res);
  REQUIRE(log.contains("Trailing characters on data line"));
  REQUIRE(log.contains(" trailing"));
}

TEST_CASE_FIXTURE(LogFixture, "Unknown field ignored when unknown_error is false") {
  const auto& msg = "/AAA5MTR\r\n\r\n"
                    "1-0:2.7.0(00.000*kW)\r\n"
                    "!";
  ParsedData</*String*/ identification> data;
  auto res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE(res);
}

TEST_CASE_FIXTURE(LogFixture, "Missing unit when required") {
  const auto& msg = "/AAA5MTR\r\n\r\n"
                    "1-0:1.7.0(00.123)\r\n"
                    "!";
  ParsedData</*String*/ identification, /*FixedValue*/ power_delivered> data;
  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE_FALSE(res);
  REQUIRE(log.contains("Missing unit"));
  REQUIRE(log.contains(")"));
}

TEST_CASE_FIXTURE(LogFixture, "Unit present when not expected") {
  const auto& msg = "/AAA5MTR\r\n\r\n"
                    "0-0:96.7.21(00008*s)\r\n"
                    "!";
  ParsedData</*String*/ identification, /*uint32_t*/ electricity_failures> data;
  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE_FALSE(res);
  REQUIRE(log.contains("Extra data"));
  REQUIRE(log.contains("*s)"));
}

TEST_CASE_FIXTURE(LogFixture, "Malformed packet that starts with ')'") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-3:0.2.8)40(\r\n"
                    "!";

  ParsedData<
      /* String */ identification,
      /* String */ p1_version>
      data;

  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE_FALSE(res);
  REQUIRE(log.contains("Unexpected ')' symbol"));
}

TEST_CASE_FIXTURE(LogFixture, "Non-digit in numeric part") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-0:1.7.0(00.A23*kW)\r\n"
                    "!";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ power_delivered>
      data;

  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE_FALSE(res);
  REQUIRE(log.contains("Missing unit"));
  REQUIRE(log.contains("A23*kW)"));
}

TEST_CASE_FIXTURE(LogFixture, "OBIS id empty line") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "garbage\r\n"
                    "!";

  ParsedData</*String*/ identification, /*FixedValue*/ power_delivered> data;
  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE_FALSE(res);
  REQUIRE(log.contains("OBIS id Empty"));
  REQUIRE(log.contains("garbage"));
}

TEST_CASE_FIXTURE(LogFixture, "Accepts LF-only line endings") {
  const auto& msg = "/AAA5MTR\n"
                    "\n"
                    "1-0:1.7.0(00.123*kW)\n"
                    "!";

  ParsedData</*String*/ identification, /*FixedValue*/ power_delivered> data;
  DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE(data.power_delivered == 0.123f);
}

TEST_CASE_FIXTURE(LogFixture, "Unit matching is case-insensitive") {
  const auto& msg = "/ABC5MTR\r\n"
                    "\r\n"
                    "1-0:1.8.1(000001.000*kwh)\r\n"
                    "!";

  ParsedData</*String*/ identification, /*FixedValue*/ energy_delivered_tariff1> data;
  DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE(data.energy_delivered_tariff1 == 1.000f);
}

TEST_CASE_FIXTURE(LogFixture, "Numeric without decimals is accepted (auto-padded)") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-0:1.7.0(1*kW)\r\n"
                    "!";

  ParsedData</*String*/ identification, /*FixedValue*/ power_delivered> data;
  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE(res);
  REQUIRE(data.power_delivered == 1.0f);
}

TEST_CASE_FIXTURE(LogFixture, "Can parse a dataline if it has a break in the middle") {
  const auto& msg = "/KMP5 ZABF000000000000\r\n"
                    "0-1:24.3.0(120517020000)(08)(60)(1)(0-1:24.2.1)(m3)\r\n"
                    "(00124.477)\r\n"
                    "0-0:96.13.0(303132333435363738393A3B3C3D3E3F303132333435363738393A3B3C3D3E3F\r\n"
                    "303132333435363738393A3B3C3D3E3F303132333435363738393A3B3C3D3E3F\r\n"
                    "303132333435363738393A3B3C3D3E3F)\r\n"
                    "!";

  ParsedData<identification, gas_delivered_text, message_long> data;
  DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE(data.gas_delivered_text == "(120517020000)(08)(60)(1)(0-1:24.2.1)(m3)\r\n(00124.477)");
  REQUIRE(data.message_long == "303132333435363738393A3B3C3D3E3F303132333435363738393A3B3C3D3E3F\r\n303132333435363738393A3B3C3D3E3F30313233343536373"
                               "8393A3B3C3D3E3F\r\n303132333435363738393A3B3C3D3E3F");
}

TEST_CASE_FIXTURE(LogFixture, "Can parse a 0 value without a unit") {
  const auto& msg = "/KMP5 ZABF000000000000\r\n"
                    "0-1:24.2.1(000101000000W)(00000000.0000)\r\n"
                    "!";
  ParsedData<gas_delivered> data;
  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE(res);
  REQUIRE(data.gas_delivered == 0.0f);
}

TEST_CASE_FIXTURE(LogFixture, "Whitespace after OBIS ID") {
  const auto& msg = "/KMP5 ZABF000000000000\r\n"
                    "0-1:24.2.1 (000101000000W)(00000000.0000)\r\n"
                    "!";
  ParsedData<gas_delivered> data;
  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false), /*unknown_error=*/true);
  REQUIRE_FALSE(res);
  REQUIRE(log.contains("Missing ("));
  REQUIRE(log.contains(" (000101000000W)(00000000.0000)"));
}

TEST_CASE_FIXTURE(LogFixture, "Use integer fallback unit") {
  const auto& msg = "/KMP5 ZABF000000000000\r\n"
                    "0-1:24.2.1(230101120000W)(00012*dm3)\r\n"
                    "1-0:14.7.0(50000*mHz)\r\n"
                    "!";
  ParsedData<gas_delivered, frequency> data;
  DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false), /*unknown_error=*/true);
  REQUIRE(data.gas_delivered == 0.012f);
  REQUIRE(data.frequency == 50.0f);
}

TEST_CASE_FIXTURE(LogFixture, "AveragedFixedField works properly for a long array") {
  const auto& msg = "/KMP5 ZABF000000000000\r\n"
                    "0-0:98.1.0(11)(1-0:1.6.0)(1-0:1.6.0)(230101000000W)(221206183000W)(06.134*kW)(230201000000W)(230127174500W)(05.644*kW)(230301000000W)("
                    "230226063000W)(04.895*kW)(230401000000S)(230305181500W)(04.879*kW)(230501000000S)(230416094500S)(04.395*kW)(230601000000S)(230522084500S)("
                    "03.242*kW)(230701000000S)(230623053000S)(01.475*kW)(230801000000S)(230724060000S)(02.525*kW)(230901000000S)(230819174500S)(02.491*kW)("
                    "231001000000S)(230911063000S)(02.342*kW)(231101000000W)(231031234500W)(02.048*kW)\r\n"
                    "!";

  ParsedData<active_energy_import_maximum_demand_last_13_months> data;
  DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false), /* unknown_error */ true);

  REQUIRE(data.active_energy_import_maximum_demand_last_13_months.val() == 3.642f);
}

TEST_CASE_FIXTURE(LogFixture, "AveragedFixedField works properly for an empty array") {
  const auto& msg = "/KMP5 ZABF000000000000\r\n"
                    "0-0:98.1.0(0)(garbage that will be skipped)\r\n"
                    "1-0:1.8.1(000001.000*kwh)\r\n"
                    "!";

  ParsedData<active_energy_import_maximum_demand_last_13_months, energy_delivered_tariff1> data;
  DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false), /* unknown_error */ true);

  REQUIRE(data.active_energy_import_maximum_demand_last_13_months.val() == 0.0f);
  REQUIRE(data.energy_delivered_tariff1.val() == 1.0f);
}

TEST_CASE_FIXTURE(LogFixture, "Should parse gas_delivered_gj field") {
  const auto& msg = "/identification\r\n"
                    "0-1:24.2.1(251129203200W)(3.829*GJ)\r\n"
                    "!";

  ParsedData<gas_delivered_gj> data;

  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false), /* unknown_error */ true);
  REQUIRE(res);
  REQUIRE(data.gas_delivered_gj == 3.829f);
}

TEST_CASE_FIXTURE(LogFixture, "Should parse hexified equipment_id field") {
  const auto& msg = "/identification\r\n"
                    "0-0:96.1.1(4530303632303030303134353034303239)\r\n"
                    "!";

  ParsedData<equipment_id> data;

  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false), /* unknown_error */ true);
  REQUIRE(res);
  REQUIRE(data.equipment_id == "E0062000014504029");
}

TEST_CASE_FIXTURE(LogFixture, "Should keep equipment_id as-is when not hex encoded") {
  const auto& msg = "/identification\r\n"
                    "0-0:96.1.1(Some long string 32 bytes)\r\n"
                    "!";

  ParsedData<equipment_id> data;

  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false), /* unknown_error */ true);
  REQUIRE(res);
  REQUIRE(data.equipment_id == "Some long string 32 bytes");
}

TEST_CASE_FIXTURE(LogFixture, "Should keep equipment_id as-is for odd number of hex digits") {
  // Odd length cannot be hex-decoded, so it is kept verbatim.
  const auto& msg = "/identification\r\n"
                    "0-0:96.1.1(45303)\r\n"
                    "!";

  ParsedData<equipment_id> data;
  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false), /* unknown_error */ true);
  REQUIRE(res);
  REQUIRE(data.equipment_id == "45303");
}

TEST_CASE_FIXTURE(LogFixture, "Should keep equipment_id as-is when lowercase hex digits are present") {
  const auto& msg = "/identification\r\n"
                    "0-0:96.1.1(4530303632303030303134353034303239aa)\r\n"
                    "!";

  // Lowercase letters are not treated as hex digits, so the value is kept verbatim.
  ParsedData<equipment_id> data;
  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false), /* unknown_error */ true);
  REQUIRE(res);
  REQUIRE(data.equipment_id == "4530303632303030303134353034303239aa");
}

TEST_CASE_FIXTURE(LogFixture, "Should keep equipment_id as-is when decoded byte is not alphanumeric") {
  const auto& msg = "/identification\r\n"
                    "0-0:96.1.1(00)\r\n" // 0x00 is not a digit or uppercase letter
                    "!";

  ParsedData<equipment_id> data;
  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false), /* unknown_error */ true);
  REQUIRE(res);
  REQUIRE(data.equipment_id == "00");
}

TEST_CASE_FIXTURE(LogFixture, "Should parse an empty equipment_id field") {
  const auto& msg = "/identification\r\n"
                    "0-0:96.1.1()\r\n"
                    "!";

  ParsedData<equipment_id> data;
  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false), /* unknown_error */ true);
  REQUIRE(res);
  REQUIRE(data.equipment_id == "");
}

TEST_CASE_FIXTURE(LogFixture, "Missing opening parenthesis for numeric field") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-0:1.7.0\r\n"
                    "!";

  ParsedData</*String*/ identification, /*FixedValue*/ power_delivered> data;
  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE_FALSE(res);
  REQUIRE(log.contains("Missing ( ''"));
}

TEST_CASE_FIXTURE(LogFixture, "Non-digit in integer part of numeric field") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-0:1.7.0(A0.123*kW)\r\n"
                    "!";

  ParsedData</*String*/ identification, /*FixedValue*/ power_delivered> data;
  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE_FALSE(res);
  REQUIRE(log.contains("Invalid number"));
  REQUIRE(log.contains("A0.123*kW)"));
}

TEST_CASE_FIXTURE(LogFixture, "Unit too short for numeric field") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-0:1.8.1(000001.000*kW)\r\n"
                    "!";

  ParsedData</*String*/ identification, /*FixedValue*/ energy_delivered_tariff1> data;
  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE_FALSE(res);
  REQUIRE(log.contains("Missing unit"));
  REQUIRE(log.contains("kW)"));
}

TEST_CASE_FIXTURE(LogFixture, "Nested opening parenthesis") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-0:1.7.0(0(0.123*kW))\r\n"
                    "!";

  ParsedData</*String*/ identification, /*FixedValue*/ power_delivered> data;
  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE_FALSE(res);
  REQUIRE(log.contains("Unexpected '(' symbol"));
}

TEST_CASE_FIXTURE(LogFixture, "ParsedData without any fields") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-0:1.8.1(000671.578*kWh)\r\n"
                    "1-0:1.8.2(000842.472*kWh)\r\n"
                    "1-0:2.8.1(000000.000*kWh)\r\n"
                    "!";

  ParsedData<> data;
  const auto& res = DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes(msg, false));
  REQUIRE(res);
  REQUIRE(data.all_present());
}
