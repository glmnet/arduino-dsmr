This is a fork of [matthijskooijman/arduino-dsmr](https://github.com/matthijskooijman/arduino-dsmr).
The primary goal is to make the parser independent of the Arduino framework and usable on ESP32 with the ESP-IDF framework or any other platform.

# Features
* Combines all fixes from [matthijskooijman/arduino-dsmr](https://github.com/matthijskooijman/arduino-dsmr) and [glmnet/arduino-dsmr](https://github.com/glmnet/arduino-dsmr) including unmerged pull requests.
* Added an extensive unit test suite
* Code optimizations
* Supported compilers: MSVC, GCC, Clang
* Header-only library, no dependencies
* Code can be used on any platform, not only embedded

# Differences from the original arduino-dsmr
* Requires a C++20 compatible compiler.
* [P1Reader](https://github.com/matthijskooijman/arduino-dsmr/blob/master/src/dsmr/reader.h) class is replaced with the [PacketAccumulator](https://github.com/esphome-libs/dsmr_parser/blob/main/src/dsmr_parser/packet_accumulator.h) class with a different interface to allow usage on any platform.
* Added [DlmsPacketDecryptor](https://github.com/esphome-libs/dsmr_parser/blob/main/src/dsmr_parser/dlms_packet_decryptor.h) class to work with encrypted DSMR messages (like from "Luxembourg Smarty").

# How to use
## General usage
The library is header-only. Add the `src/dsmr_parser` folder to your project.<br>
Note: [dlms_packet_decryptor.h](https://github.com/esphome-libs/dsmr_parser/blob/main/src/dsmr_parser/dlms_packet_decryptor.h) requires one of the encryption libraries: [TF-PSA](https://github.com/Mbed-TLS/TF-PSA-Crypto), [Mbed TLS](https://github.com/Mbed-TLS/mbedtls) or [BearSsl](https://bearssl.org/).

## Usage from PlatformIO
The library is available on the PlatformIO registry:<br>
[esphome/dsmr_parser](https://registry.platformio.org/libraries/esphome/dsmr_parser)

# Examples
* Complete example using PacketAccumulator
  * [packet_accumulator_example_test.cpp](https://github.com/esphome-libs/dsmr_parser/blob/main/src/test/packet_accumulator_example_test.cpp)
* Example using DlmsPacketDecryptor
  * [dlms_packet_decryptor_example_test.cpp](https://github.com/esphome-libs/dsmr_parser/blob/main/src/test/dlms_packet_decryptor_example_test.cpp)
* Usage in EspHome project
  * [DSMR component](https://github.com/esphome/esphome/tree/dev/esphome/components/dsmr)

# History behind arduino-dsmr
[matthijskooijman](https://github.com/matthijskooijman) is the original creator of this DSMR parser.
[glmnet](https://github.com/glmnet) and [zuidwijk](https://github.com/zuidwijk) continued work on this parser in the fork [glmnet/arduino-dsmr](https://github.com/glmnet/arduino-dsmr). They used the parser to create [ESPHome DSMR](https://esphome.io/components/sensor/dsmr/) component.
After that, the work on the `arduino-dsmr` parser stopped.

## The reasons `dsmr_parser` fork was created
* Dependency on the Arduino framework limits the applicability of this parser. For example, it is not possible to use it on Linux.
* The Arduino framework on ESP32 inflates the FW size and doesn't allow usage of the latest version of ESP-IDF.
* Many pull requests and bug fixes needed to be integrated into the parser.
* Lack of support for encrypted DSMR messages.

# How to work with the code
* You can open the code using any IDE that supports CMake. It will allow you to modify the code and run the tests.
* Notes if you want to run the build scripts:
  * `build-win.ps1` needs `Visual Studio` to be installed.
  * `build-linux.sh` needs `clang` to be installed.

# References
* [DSMR parser in Python](https://github.com/ndokter/dsmr_parser/tree/master) - alternative DSMR parser implementation in Python.
* [SmartyReader](https://www.weigu.lu/microcontroller/smartyReader_P1/index.html) - open source hardware to communicate with P1 port.
* [SmartyReader. Chapter "Encryption"](https://www.weigu.lu/microcontroller/smartyReader/index.html) - how the encrypted DSMR protocol works.
