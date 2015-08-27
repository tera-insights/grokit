// Helper function for converting a 4 bit number to a hexadecimal character.
// Only values less than 16 should be given and the result is uppercase.

constexpr char ByteToHex(char byte) {
  return (byte < 10) ? byte + 48 : byte + 54;
}
