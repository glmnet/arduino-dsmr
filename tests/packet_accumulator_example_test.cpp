#include "dsmr_parser/fields.h"
#include "dsmr_parser/packet_accumulator.h"
#include "dsmr_parser/parser.h"
#include "test_util.h"
#include <doctest.h>
#include <iostream>

using namespace dsmr_parser;
using namespace fields;

// Specify the fields you want to parse.
// Full list of available fields is in "fields.h" file
using MyParsedData = ParsedData</* String */ identification,
                                /* String */ p1_version,
                                /* String */ timestamp,
                                /* String */ equipment_id,
                                /* FixedValue */ energy_delivered_tariff1>;

TEST_CASE_FIXTURE(LogFixture, "PacketAccumulator example") {
  // Buffer to store the incoming bytes.
  // This Buffer must be large enough to hold the full DSMR message.
  // Advice: define the Buffer as a global variable, to avoid using stack and heap memory.
  std::array<uint8_t, 4000> buffer;

  // For the sake of this example, we define a data that is supposed to come from the P1 port.
  const auto& data_from_p1_port = "garbage before"
                                  "/KFM5KAIFA-METER\r\n"
                                  "\r\n"
                                  "1-3:0.2.8(40)\r\n"
                                  "0-0:1.0.0(150117185916W)\r\n"
                                  "0-0:96.1.1(0000000000000000000000000000000000)\r\n"
                                  "1-0:1.8.1(000671.578*kWh)\r\n"
                                  "!60e5"
                                  "garbage after"
                                  "/KFM5KAIFA-METER\r\n"
                                  "\r\n"
                                  "1-3:0.2.8(40)\r\n"
                                  "0-0:1.0.0(150117185916W)\r\n"
                                  "0-0:96.1.1(0000000000000000000000000000000000)\r\n"
                                  "1-0:1.8.1(000671.578*kWh)\r\n"
                                  "!60e5";

  // This class is used to receive the message from the P1 port.
  // It retrieves bytes from the UART and finds a DSMR message and optionally checks the CRC.
  // You only need to create this class once.
  PacketAccumulator accumulator(/* buffer */ buffer, /* check_crc */ true);

  // Main loop.
  // We need to read data from P1 port 1 byte at a time.
  for (const char byte : data_from_p1_port) {
    // Feed the byte to the accumulator.
    std::optional<DsmrUnencryptedTelegram> dsmrTelegram = accumulator.process_byte(static_cast<uint8_t>(byte));
    if (!dsmrTelegram) {
      // No full packet received yet
      continue;
    }

    // Parse the packet.
    MyParsedData data;
    bool res = DsmrParser::parse(data, dsmrTelegram.value());
    if (!res) {
      // Parsing failed
      continue;
    }

    // If parsing succeeded, you can use the parsed data.
    std::cout << "Identification: " << data.identification << '\n';
    std::cout << "P1 version: " << data.p1_version << '\n';
    std::cout << "Timestamp: " << data.timestamp << '\n';
    std::cout << "Equipment ID: " << data.equipment_id << '\n';
    std::cout << "Energy delivered tariff 1: " << data.energy_delivered_tariff1.val() << '\n';
  }
}
