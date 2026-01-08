#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "Arduino.h"
#include <map>
#include <string>

class Preferences {
public:
    bool begin(const char * name, bool readOnly=false) { return true; }
    void end() {}

    // For this app we only need putInt/getInt/putFloat/getFloat etc if used.
    // main.cpp currently creates 'prefs' but doesn't seem to heavily use it yet 
    // in the code I read (it was mostly in the "Features Implemented" list but maybe not in main.cpp logic yet).
    // Let's check main.cpp usage. 
    // main.cpp has `Preferences prefs;` and `prefs.begin("boo", false);`
    // but looking at the code, it doesn't actually call put/get!
    // So empty stubs are fine for now.

    size_t putChar(const char* key, int8_t value) { return 1; }
    size_t putUChar(const char* key, uint8_t value) { return 1; }
    size_t putShort(const char* key, int16_t value) { return 1; }
    size_t putUShort(const char* key, uint16_t value) { return 1; }
    size_t putInt(const char* key, int32_t value) { return 1; }
    size_t putUInt(const char* key, uint32_t value) { return 1; }
    size_t putLong(const char* key, int32_t value) { return 1; }
    size_t putULong(const char* key, uint32_t value) { return 1; }
    size_t putLong64(const char* key, int64_t value) { return 1; }
    size_t putULong64(const char* key, uint64_t value) { return 1; }
    size_t putFloat(const char* key, float value) { return 1; }
    size_t putDouble(const char* key, double value) { return 1; }
    size_t putBool(const char* key, bool value) { return 1; }
    size_t putString(const char* key, const char* value) { return 1; }
    size_t putString(const char* key, String value) { return 1; }
    size_t putBytes(const char* key, const void* value, size_t len) { return 1; }

    int8_t getChar(const char* key, int8_t defaultValue = 0) { return defaultValue; }
    uint8_t getUChar(const char* key, uint8_t defaultValue = 0) { return defaultValue; }
    int16_t getShort(const char* key, int16_t defaultValue = 0) { return defaultValue; }
    uint16_t getUShort(const char* key, uint16_t defaultValue = 0) { return defaultValue; }
    int32_t getInt(const char* key, int32_t defaultValue = 0) { return defaultValue; }
    uint32_t getUInt(const char* key, uint32_t defaultValue = 0) { return defaultValue; }
    int32_t getLong(const char* key, int32_t defaultValue = 0) { return defaultValue; }
    uint32_t getULong(const char* key, uint32_t defaultValue = 0) { return defaultValue; }
    int64_t getLong64(const char* key, int64_t defaultValue = 0) { return defaultValue; }
    uint64_t getULong64(const char* key, uint64_t defaultValue = 0) { return defaultValue; }
    float getFloat(const char* key, float defaultValue = NAN) { return defaultValue; }
    double getDouble(const char* key, double defaultValue = NAN) { return defaultValue; }
    bool getBool(const char* key, bool defaultValue = false) { return defaultValue; }
    String getString(const char* key, String defaultValue = String()) { return defaultValue; }
    size_t getBytes(const char* key, void * buf, size_t maxLen) { return 0; }
};

#endif
