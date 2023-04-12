
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <vector>
#include <string_view>

uint64_t nano() {
  return std::chrono::duration_cast<::std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

__attribute__ ((noinline))
std::string_view grab_static(size_t idx) {
  const static std::string_view options[] = {"https", "http", "ftp",    "file",
                                        "ws",    "wss",  
                                        "httpr", "filer"};
  return options[idx];
}

__attribute__ ((noinline))
std::string_view grab(size_t idx) {
  constexpr static std::string_view options[] = {"https", "http", "ftp",    "file",
                                        "ws",    "wss", 
                                        "httpr", "filer"};
  return options[idx];
}

std::tuple<double, double> simulation(const size_t N) {
  double t1, t2;
  volatile size_t counter = 0;

  {

    uint64_t start = nano();
    for (size_t i = 0; i < N; i++) {
      counter += grab(i%8).size();
    }
    uint64_t finish = nano();
    t1 = double(finish - start) / N;
  }
  {

    uint64_t start = nano();
    for (size_t i = 0; i < N; i++) {
      counter += grab_static(i%8).size();
    }
    uint64_t finish = nano();
    t2 = double(finish - start) / N;
  }
  (void) counter;
  return {t1, t2};
}

void demo() {
  double avg1 = 0;
  double avg2 = 0;

  size_t times = 100;

  for (size_t i = 0; i < times; i++) {
    auto [t1, t2] = simulation(10000);
    avg1 += t1;
    avg2 += t2;
  }
  avg1 /= times;
  avg2 /= times;

  std::cout << avg1 << " " << avg2  << std::endl;
}

int main() {
  puts("We report ns/string_view (first non-static, then static_).\n");
  demo();
  demo();
  demo();
  demo();

}
