#pragma once
// Just enough of the Arduino API to run the UI and grind logic on a PC
#include <cstdint>
#include <cmath>
#include <cstring>
#include <string>
#include <cstdio>
typedef bool boolean;
typedef uint8_t byte;
#define INPUT 0
#define OUTPUT 1
#define HIGH 1
#define LOW 0
extern unsigned long fakeMillis;
extern int fakePins[8];
extern int fakeDigitalWrites[8];
inline unsigned long millis() { return fakeMillis; }
inline int digitalRead(int p) { return fakePins[p]; }
inline void digitalWrite(int p, int v) { fakeDigitalWrites[p] = v; }
inline void pinMode(int, int) {}
inline void delay(unsigned long ms) { fakeMillis += ms; }
using std::fabs;
class String {
 public:
  std::string s;
  String() {}
  String(const char* c) : s(c) {}
  String(int v) : s(std::to_string(v)) {}
  String(float v, int d) { char b[32]; snprintf(b, sizeof b, "%.*f", d, v); s = b; }
  String operator+(const String& o) const { String r; r.s = s + o.s; return r; }
  friend String operator+(const char* a, const String& b) { String r; r.s = std::string(a) + b.s; return r; }
  bool operator==(const char* c) const { return s == c; }
  bool operator!=(const char* c) const { return s != c; }
  unsigned length() const { return s.size(); }
  const char* c_str() const { return s.c_str(); }
};
struct FakeSerial {
  template <class T> void print(T) {} template <class T> void print(T, int) {}
  template <class T> void println(T) {} template <class T> void println(T, int) {} void println() {}
};
extern FakeSerial Serial;
