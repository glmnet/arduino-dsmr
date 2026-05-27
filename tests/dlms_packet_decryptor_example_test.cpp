// Include one of the decryption implementations depending on what encryption library you use:
#include "dsmr_parser/decryption/aes128gcm_mbedtls.h"
// or "dsmr_parser/decryption/aes128gcm_bearssl.h"
// or "dsmr_parser/decryption/aes128gcm_tfpsa.h"

#include "dsmr_parser/dlms_packet_decryptor.h"
#include "dsmr_parser/fields.h"
#include "dsmr_parser/parser.h"
#include <doctest.h>
#include <iostream>

// Dummy functions to make the example compile
long millis() { return 0; }
struct Uart {
  size_t available() { return 0; }
  uint8_t readByte() { return 0; }
};

using namespace dsmr_parser;

std::array<uint8_t, 4000> dlms_packet_buffer; // Buffer to store the incoming bytes from the P1 port and the decrypted dsmr telegram
size_t dlms_packet_buffer_position = 0;       // needed to accumulate bytes

// Create the decryptor from the decryption implementation header that you included above.
// Available implementations:
// * Aes128GcmBearSsl (from "dsmr_parser/decryption/aes128gcm_bearssl.h" header)
// * Aes128GcmMbedTls (from "dsmr_parser/decryption/aes128gcm_mbedtls.h" header)
// * Aes128GcmTfPsa (from "dsmr_parser/decryption/aes128gcm_tfpsa.h" header)
Aes128GcmMbedTls gcm_decryptor;

// Create the DLMS packet decryption. You only need to create it once. It is stateless.
DlmsPacketDecryptor decryptor(gcm_decryptor, /* crc_check */ true);

long last_read_timestamp = 0; // timestamp of the last byte received. Needed to detect inter-frame gaps.

Uart uart; // UART connected to P1 port

// This encryption key is unique per smart meter and must be provided by the electricity company.
const auto encryption_key = Aes128GcmDecryptionKey::from_hex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA").value();

// Optional. You can provide the authentication key to verify the GCM tag. Without this key, the decryption will skip the tag verification.
const auto authentication_key = Aes128GcmAuthenticationKey::from_hex("BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB").value();

// You must set the encryption key before calling `decryptor.decrypt_inplace` method.
inline void set_encryption_key() { gcm_decryptor.set_encryption_key(encryption_key); }
// Optional. Set the authentication key if you want to verify the GCM tag.
inline void set_authentication_key() { gcm_decryptor.set_authentication_key(authentication_key); }

// Main loop that reads data from the P1 port and decrypts packets.
inline void loop() {
  // To receive the encrypted packets, we need to rely on the "inter-frame delay" technique.
  // The packets don't have any start and stop bytes. However, there is a guaranteed 10 second delay between packets.

  while (uart.available()) {
    // reset buffer if overflow.
    if (dlms_packet_buffer_position >= dlms_packet_buffer.size()) {
      dlms_packet_buffer_position = 0;
    }

    dlms_packet_buffer[dlms_packet_buffer_position] = uart.readByte();
    dlms_packet_buffer_position++;

    // save the time when we received the last byte
    last_read_timestamp = millis();
  }

  // detect inter-frame delay. If no byte is received for more than 1 second, then the packet is complete
  if ((millis() - last_read_timestamp) > 1000 && dlms_packet_buffer_position > 0) {
    // Decrypt the received packet. The decrypted telegram contains the CRC (checked according to the crc_check flag passed to the decryptor constructor).
    std::optional<DsmrUnencryptedTelegram> dsmr_telegram = decryptor.decrypt_inplace({dlms_packet_buffer.data(), dlms_packet_buffer_position});
    dlms_packet_buffer_position = 0; // reset for the next packet

    // check that decryption was successful
    if (!dsmr_telegram) {
      printf("Failed to decrypt packet\n");
      return;
    }

    // Parse it using P1Parser::parse() method. Look at "packet_accumulator_example_test.cpp"
  }
}
