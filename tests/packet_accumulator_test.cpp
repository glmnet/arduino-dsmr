#include "dsmr_parser/packet_accumulator.h"
#include "test_util.h"
#include <doctest.h>
#include <string>
#include <vector>

using namespace dsmr_parser;

TEST_CASE_FIXTURE(LogFixture, "Packet with correct CRC") {
  std::vector<uint8_t> buffer(1000);
  const auto& msg = "/KFM5KAIFA-METER\r\n"
                    "\r\n"
                    "1-0:1.8.1(000671.578*kWh)\r\n"
                    "1-0:1.7.0(00.318*kW)\r\n"
                    "!1e1D\r\n";

  auto accumulator = PacketAccumulator(buffer, true);
  std::optional<DsmrUnencryptedTelegram> dsmrTelegram;
  for (const auto& byte : msg) {
    dsmrTelegram = accumulator.process_byte(static_cast<uint8_t>(byte));
    if (dsmrTelegram)
      break;
  }
  REQUIRE(dsmrTelegram);
  REQUIRE(log.contains("Found telegram start symbol '/'"));
  REQUIRE(log.contains("Found telegram end symbol '!'"));
  REQUIRE(log.contains("Successfully received the telegram with correct CRC"));
}

TEST_CASE_FIXTURE(LogFixture, "Packet with incorrect CRC") {
  std::vector<uint8_t> buffer(1000);
  const auto& msg = "/some data!0000";

  PacketAccumulator accumulator(buffer, true);
  std::optional<DsmrUnencryptedTelegram> dsmrTelegram;
  for (const auto& byte : msg) {
    dsmrTelegram = accumulator.process_byte(static_cast<uint8_t>(byte));
    if (dsmrTelegram)
      break;
  }

  REQUIRE(dsmrTelegram == std::nullopt);
  REQUIRE(log.contains("Found telegram start symbol '/'"));
  REQUIRE(log.contains("Found telegram end symbol '!'"));
  REQUIRE(log.contains("CRC mismatch"));
}

TEST_CASE_FIXTURE(LogFixture, "Packet with incorrect CRC symbol") {
  std::vector<uint8_t> buffer(1000);
  const auto& msg = "/some data!G000";

  PacketAccumulator accumulator(buffer, true);
  std::optional<DsmrUnencryptedTelegram> dsmrTelegram;
  for (const auto& byte : msg) {
    dsmrTelegram = accumulator.process_byte(static_cast<uint8_t>(byte));
    if (dsmrTelegram)
      break;
  }
  REQUIRE(dsmrTelegram == std::nullopt);
  REQUIRE(log.contains("Found telegram start symbol '/'"));
  REQUIRE(log.contains("Found telegram end symbol '!'"));
  REQUIRE(log.contains("Incorrect CRC character 'G'"));
}

TEST_CASE_FIXTURE(LogFixture, "Packet without CRC") {
  std::vector<uint8_t> buffer(1000);
  const auto& msg = "/some data!";

  PacketAccumulator accumulator(buffer, false);
  std::optional<DsmrUnencryptedTelegram> dsmrTelegram;
  for (const auto& byte : msg) {
    dsmrTelegram = accumulator.process_byte(static_cast<uint8_t>(byte));
    if (dsmrTelegram)
      break;
  }

  REQUIRE(dsmrTelegram);
  REQUIRE(log.contains("Found telegram start symbol '/'"));
  REQUIRE(log.contains("Found telegram end symbol '!'"));
  REQUIRE(log.contains("Successfully received the telegram without CRC check"));
}

TEST_CASE_FIXTURE(LogFixture, "Parse data with different packets. CRC check") {
  std::vector<uint8_t> buffer(15);
  const auto& msg = "garbage /some !a3D4"      // correct package
                    "garbage /some !a3D3"      // CRC mismatch
                    "garbage /so/some !a3D4"   // Packet start symbol '/' in the middle of the packet
                    "garbage /some !a3G4"      // Incorrect CRC character
                    "/some !a3D4"              // correct package
                    "/garbage garbage garbage" // buffer overflow
                    "/some !a3D4";             // correct package

  std::vector<DsmrUnencryptedTelegram> received_packets;
  PacketAccumulator accumulator(buffer, true);
  for (const auto& byte : msg) {
    auto dsmrTelegram = accumulator.process_byte(static_cast<uint8_t>(byte));
    if (dsmrTelegram) {
      received_packets.push_back(*dsmrTelegram);
    }
  }

  REQUIRE(received_packets.size() == 4);
  REQUIRE(std::string(received_packets[0].content_without_crc()) == "/some !");
  REQUIRE(std::string(received_packets[1].content_without_crc()) == "/some !");
  REQUIRE(std::string(received_packets[2].content_without_crc()) == "/some !");
  REQUIRE(std::string(received_packets[3].content_without_crc()) == "/some !");
}

TEST_CASE_FIXTURE(LogFixture, "Parse data with different packets. No CRC check") {
  std::vector<uint8_t> buffer(15);
  const auto& msg = "garbage /some !"          // correct package
                    "garbage /so/some !"       // Packet start symbol '/' in the middle of the packet
                    "/some !"                  // correct package
                    "/garbage garbage garbage" // buffer overflow
                    "/some !";                 // correct package

  std::vector<DsmrUnencryptedTelegram> received_packets;

  PacketAccumulator accumulator(buffer, false);
  for (const auto& byte : msg) {
    auto dsmrTelegram = accumulator.process_byte(static_cast<uint8_t>(byte));
    if (dsmrTelegram) {
      received_packets.push_back(*dsmrTelegram);
    }
  }

  REQUIRE(std::string(received_packets[0].content_without_crc()) == "/some !");
  REQUIRE(std::string(received_packets[1].content_without_crc()) == "/some !");
  REQUIRE(std::string(received_packets[2].content_without_crc()) == "/some !");
  REQUIRE(std::string(received_packets[3].content_without_crc()) == "/some !");
}

TEST_CASE_FIXTURE(LogFixture, "Packet with correct CRC upper case") {
  std::vector<uint8_t> buffer(1000);
  const auto& msg = "/some !A3D4";

  PacketAccumulator accumulator(buffer, true);
  std::optional<DsmrUnencryptedTelegram> dsmrTelegram;
  for (const auto& byte : msg) {
    dsmrTelegram = accumulator.process_byte(static_cast<uint8_t>(byte));
    if (dsmrTelegram)
      break;
  }
  REQUIRE(dsmrTelegram);
  REQUIRE(log.contains("Successfully received the telegram with correct CRC"));
}

TEST_CASE_FIXTURE(LogFixture, "Buffer overflow discards accumulated data") {
  std::vector<uint8_t> buffer(5);
  const auto& msg = "/123456";

  PacketAccumulator accumulator(buffer, true);
  std::optional<DsmrUnencryptedTelegram> dsmrTelegram;
  for (const auto& byte : msg) {
    dsmrTelegram = accumulator.process_byte(static_cast<uint8_t>(byte));
    if (dsmrTelegram)
      break;
  }
  REQUIRE(dsmrTelegram == std::nullopt);
  REQUIRE(log.contains("Found telegram start symbol '/'"));
  REQUIRE(log.contains("Buffer overflow. Discarding the accumulated data"));
}

TEST_CASE_FIXTURE(LogFixture, "Only garbage data produces no packet") {
  std::vector<uint8_t> buffer(1000);
  const auto& msg = "garbage data with no start symbol";

  PacketAccumulator accumulator(buffer, true);
  std::optional<DsmrUnencryptedTelegram> dsmrTelegram;
  for (const auto& byte : msg) {
    dsmrTelegram = accumulator.process_byte(static_cast<uint8_t>(byte));
    if (dsmrTelegram)
      break;
  }
  REQUIRE(dsmrTelegram == std::nullopt);
}

TEST_CASE_FIXTURE(LogFixture, "Start symbol during CRC accumulation resets state") {
  std::vector<uint8_t> buffer(1000);
  // First packet has incomplete CRC ("a3"), then '/' starts a new packet
  const auto& msg = "/some !a3/some !a3D4";

  std::vector<DsmrUnencryptedTelegram> received_packets;
  PacketAccumulator accumulator(buffer, true);
  for (const auto& byte : msg) {
    auto dsmrTelegram = accumulator.process_byte(static_cast<uint8_t>(byte));
    if (dsmrTelegram) {
      received_packets.push_back(*dsmrTelegram);
    }
  }
  REQUIRE(received_packets.size() == 1);
  REQUIRE(std::string(received_packets[0].content_without_crc()) == "/some !");
  REQUIRE(log.contains("Successfully received the telegram with correct CRC"));
}

TEST_CASE_FIXTURE(LogFixture, "Incomplete CRC at end of stream produces no packet") {
  std::vector<uint8_t> buffer(1000);
  // Only 2 of 4 CRC nibbles provided
  const auto& msg = "/some !a3";

  PacketAccumulator accumulator(buffer, true);
  std::optional<DsmrUnencryptedTelegram> dsmrTelegram;
  for (const auto& byte : msg) {
    dsmrTelegram = accumulator.process_byte(static_cast<uint8_t>(byte));
    if (dsmrTelegram)
      break;
  }
  REQUIRE(dsmrTelegram == std::nullopt);
  REQUIRE(log.contains("Found telegram start symbol '/'"));
  REQUIRE(log.contains("Found telegram end symbol '!'"));
}
