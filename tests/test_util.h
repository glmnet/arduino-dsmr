#pragma once

#include "dsmr_parser/packet_accumulator.h"
#include "dsmr_parser/util.h"
#include <cstdio>
#include <string>
#include <vector>

class LogCapturer {
public:
  LogCapturer() {
    dsmr_parser::Logger::set_log_function([this](dsmr_parser::LogLevel, const char* fmt, va_list args) {
      char buf[1024];
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif
      vsnprintf(buf, sizeof(buf), fmt, args);
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
      messages.emplace_back(buf);
    });
  }

  ~LogCapturer() {
    dsmr_parser::Logger::set_log_function([](dsmr_parser::LogLevel, const char*, va_list) {});
  }

  bool contains(const std::string& substr) const {
    for (const auto& msg : messages) {
      if (msg.find(substr) != std::string::npos)
        return true;
    }
    return false;
  }

  void clear() { messages.clear(); }

  std::vector<std::string> messages;
};

struct LogFixture {
  LogCapturer log;
};
