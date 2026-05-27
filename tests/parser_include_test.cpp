// This code tests that the parser has all necessary dependencies included in its headers.
// We check that the code compiles.

#include "dsmr_parser/fields.h"
#include "dsmr_parser/parser.h"

using namespace dsmr_parser;
using namespace fields;

void P1Parser_some_function() {
  ParsedData<identification, p1_version> data;
  DsmrParser::parse(data, *DsmrUnencryptedTelegram::from_bytes("msg", false), true);
}
