#include <cstdio>

#include "test/test_message_def.hpp"

int main(void) {
  ss::Enum1BytesTest msg;

  msg.enumeration = ss_c::kEnum1BytesValue120;

  auto packed = msg.Pack();

  for (auto x : *packed) {
    printf("%#04x,", x);
  }
  printf("\n");

  auto [status, unpacked_msg] = ss::UnpackMessage(packed->data());

  printf("Status: %d\n", static_cast<int>(status));
  if (std::holds_alternative<ss::Bitfield2BytesTest>(unpacked_msg)) {
    printf("Bitfield\n");
  }
  if (std::holds_alternative<ss::Enum1BytesTest>(unpacked_msg)) {
    printf("Enum\n");
  }

  return 0;
}
