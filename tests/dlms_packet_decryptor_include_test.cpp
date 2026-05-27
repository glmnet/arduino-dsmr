// This code tests that the encrypted_packet_accumulator header has all necessary dependencies included in its headers.
// We check that the code compiles.

#include "dsmr_parser/decryption/aes128gcm_mbedtls.h"
#include "dsmr_parser/dlms_packet_decryptor.h"

using namespace dsmr_parser;

void DlmsPacketDecryptor_some_function() {
  std::array<uint8_t, 1000> packet_buffer;
  Aes128GcmMbedTls gcm_decryptor;
  gcm_decryptor.set_encryption_key(*Aes128GcmDecryptionKey::from_hex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"));
  DlmsPacketDecryptor decryptor(gcm_decryptor, false);
  decryptor.decrypt_inplace(packet_buffer);
}
