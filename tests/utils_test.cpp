#include "dsmr_parser/util.h"
#include "test_util.h"
#include <doctest.h>
#include <ostream>
#include <string>

using namespace dsmr_parser;

namespace {

constexpr std::string_view kTelegramBody = "/some !";
constexpr std::string_view kValidCrc = "a3D4";

std::string telegram_with_crc(std::string_view crc) {
  std::string s(kTelegramBody);
  s.append(crc);
  return s;
}

} // namespace

TEST_CASE_FIXTURE(LogFixture, "DsmrUnencryptedTelegram::from_bytes returns nullopt when telegram does not start with '/'") {
  const auto result = DsmrUnencryptedTelegram::from_bytes("XAAA\r\n!", false);
  CHECK(!result.has_value());
  CHECK(log.contains("does not start with"));
}

TEST_CASE_FIXTURE(LogFixture, "DsmrUnencryptedTelegram::from_bytes returns nullopt when telegram has no '!'") {
  const auto result = DsmrUnencryptedTelegram::from_bytes("/AAA\r\nfoo", false);
  CHECK(!result.has_value());
  CHECK(log.contains("does not end with"));
}

TEST_CASE_FIXTURE(LogFixture, "DsmrUnencryptedTelegram without CRC suffix: full_content equals content_without_crc") {
  const auto result = DsmrUnencryptedTelegram::from_bytes(kTelegramBody, false);
  REQUIRE(result.has_value());
  CHECK(result->full_content() == kTelegramBody);
  CHECK(result->content_without_crc() == kTelegramBody);
}

TEST_CASE_FIXTURE(LogFixture, "DsmrUnencryptedTelegram with CRC suffix (no crc check): full_content includes CRC, content_without_crc strips it") {
  const std::string input = telegram_with_crc(kValidCrc);
  const auto result = DsmrUnencryptedTelegram::from_bytes(input, false);
  REQUIRE(result.has_value());
  CHECK(result->full_content() == input);
  CHECK(result->content_without_crc() == kTelegramBody);
}

TEST_CASE_FIXTURE(LogFixture, "DsmrUnencryptedTelegram with trailing bytes beyond CRC keeps only body + 4 CRC chars in full_content") {
  const std::string input = telegram_with_crc(kValidCrc) + "\r\n";
  const auto result = DsmrUnencryptedTelegram::from_bytes(input, false);
  REQUIRE(result.has_value());
  CHECK(result->full_content() == telegram_with_crc(kValidCrc));
  CHECK(result->content_without_crc() == kTelegramBody);
}

TEST_CASE_FIXTURE(LogFixture, "DsmrUnencryptedTelegram with valid CRC and crc_check=true succeeds") {
  const std::string input = telegram_with_crc(kValidCrc);
  const auto result = DsmrUnencryptedTelegram::from_bytes(input, true);
  REQUIRE(result.has_value());
  CHECK(result->full_content() == input);
  CHECK(result->content_without_crc() == kTelegramBody);
}

TEST_CASE_FIXTURE(LogFixture, "DsmrUnencryptedTelegram with invalid CRC and crc_check=true fails") {
  const std::string input = telegram_with_crc("0000");
  const auto result = DsmrUnencryptedTelegram::from_bytes(input, true);
  CHECK(!result.has_value());
  CHECK(log.contains("CRC mismatch"));
}

TEST_CASE_FIXTURE(LogFixture, "DsmrUnencryptedTelegram with non-hex CRC characters and crc_check=true fails") {
  const std::string input = telegram_with_crc("ZZZZ");
  const auto result = DsmrUnencryptedTelegram::from_bytes(input, true);
  CHECK(!result.has_value());
  CHECK(log.contains("Incorrect CRC character"));
}

TEST_CASE_FIXTURE(LogFixture, "DsmrUnencryptedTelegram with crc_check=true but no CRC bytes fails") {
  const auto result = DsmrUnencryptedTelegram::from_bytes(kTelegramBody, true);
  CHECK(!result.has_value());
  CHECK(log.contains("not have enough bytes for CRC"));
}

TEST_CASE_FIXTURE(LogFixture, "DsmrUnencryptedTelegram accepts lowercase hex CRC") {
  const std::string input = telegram_with_crc("a3d4");
  const auto result = DsmrUnencryptedTelegram::from_bytes(input, true);
  REQUIRE(result.has_value());
  CHECK(result->content_without_crc() == kTelegramBody);
  CHECK(result->full_content() == input);
}
