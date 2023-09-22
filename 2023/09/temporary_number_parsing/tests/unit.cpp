#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "parse_integer.h"

bool basic_test() {
  printf("basic_test\n");
  std::string input = "9223372036854775808.";
  auto r = parse_unsigned(input.data());
  if(!r) { return false; }
  if(*r != 9223372036854775808ULL) { return false; }

  r = parse_unsigned_bounded(input.data(), input.data() + input.size() - 1);
  if(!r) { return false; }
  if(*r != 9223372036854775808ULL) { return false; }

  r = parse_unsigned_bounded_reverse(input.data(), input.data() + input.size() - 1);
  if(!r) { printf("bad\n");return false; }
  if(*r != 9223372036854775808ULL) {printf("bad %zu\n", *r); return false; }

  r = parse_unsigned_avx512(input.data(), input.data() + input.size() - 1);
  if(!r) { printf("bad\n");return false; }
  if(*r != 9223372036854775808ULL) {printf("bad %zu\n", *r); return false; }

  printf("SUCCESS\n");
  return true; 
}

int main() {
 return (basic_test()) ? EXIT_SUCCESS : EXIT_FAILURE;
}
