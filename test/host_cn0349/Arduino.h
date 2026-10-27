#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <string.h>
#ifdef _MSC_VER
#define strtok_r strtok_s
#endif
#include <sstream>
#include <iomanip>
#define HEX 16
inline uint32_t& hostClock() { static uint32_t t=0; return t; }
inline uint32_t millis() { return hostClock(); }
class Stream {
 public:
  std::string input, output;
  int available() { return static_cast<int>(input.size()); }
  int read() { if(input.empty()) return -1; unsigned char c=input.front(); input.erase(0,1); return c; }
  template<class T> size_t print(T value) { std::ostringstream o; o << value; output += o.str(); return 1; }
  size_t print(uint8_t value) { return print(static_cast<unsigned>(value)); }
  template<class T> size_t print(T value, int digits) { (void)digits; return print(value); }
  size_t println() { output += "\n"; return 1; }
  template<class T> size_t println(T value) { print(value); return println(); }
  template<class T> size_t println(T value, int digits) { print(value,digits); return println(); }
};
