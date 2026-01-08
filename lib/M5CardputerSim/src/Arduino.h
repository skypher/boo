#ifndef Arduino_h
#define Arduino_h

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <stdarg.h>
#include <stdio.h> // for vsnprintf

// Basic Types
typedef bool boolean;
typedef uint8_t byte;

// String class mock (simplified wrapper around std::string)
class String : public std::string {
public:
    String(const char* s) : std::string(s) {}
    String(std::string s) : std::string(s) {}
    String(int i) : std::string(std::to_string(i)) {}
    String() : std::string("") {}
};

// Math / Utils
#undef min
#undef max
#undef abs
#undef round
#undef constrain

template <typename T, typename U>
auto min(T a, U b) -> decltype(a < b ? a : b) {
    return (a < b) ? a : b;
}

template <typename T, typename U>
auto max(T a, U b) -> decltype(a > b ? a : b) {
    return (a > b) ? a : b;
}

template <typename T>
T abs(T x) {
    return (x > 0) ? x : -x;
}

template <typename T>
long round(T x) {
    return (x >= 0) ? (long)(x + 0.5) : (long)(x - 0.5);
}

template <typename T, typename L, typename H>
T constrain(T amt, L low, H high) {
    if (amt < low) return low;
    if (amt > high) return high;
    return amt;
}

#define PI 3.1415926535897932384626433832795

// Time
unsigned long millis();
void delay(unsigned long ms);

// Random
long random(long max);
long random(long min, long max);
void randomSeed(long seed);

// IO (Stubbed)
int analogRead(uint8_t pin);

// Serial Mock
class SerialClass {
public:
    void begin(long baud) { (void)baud; }
    void print(const char* s) { std::cout << s; }
    void print(int n) { std::cout << n; }
    void println(const char* s) { std::cout << s << std::endl; }
    void println(int n) { std::cout << n << std::endl; }
    void printf(const char* format, ...) {
        char buf[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buf, sizeof(buf), format, args);
        va_end(args);
        std::cout << buf;
    }
};

extern SerialClass Serial;

#endif
