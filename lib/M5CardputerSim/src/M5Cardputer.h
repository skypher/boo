#ifndef M5Cardputer_h
#define M5Cardputer_h

#include "Arduino.h"
#include <vector>

// Forward declare for M5Canvas
class M5Canvas;

// ================= Graphics Classes =================

class M5Display {
public:
    int width() { return 240; }
    int height() { return 135; }
    void fillScreen(uint16_t color);
    void setRotation(int r);
    void setTextColor(uint16_t color);
    void setTextSize(int size);
    void setCursor(int x, int y);
    void print(const char* s);
    void print(int n);
    void println(const char* s);
    void printf(const char* format, ...);
};

class M5Canvas {
public:
    M5Canvas(M5Display* display);
    void createSprite(int w, int h);
    void pushSprite(int x, int y);
    void deleteSprite();

    // Drawing
    void fillSprite(uint16_t color);
    void drawPixel(int x, int y, uint16_t color);
    void fillCircle(int x, int y, int r, uint16_t color);
    void fillRect(int x, int y, int w, int h, uint16_t color);
    void drawLine(int x0, int y0, int x1, int y1, uint16_t color);
    void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color);

    // Text
    void setTextColor(uint16_t color);
    void setTextSize(int size);
    void setCursor(int x, int y);
    void print(const char* s);
    void print(int n);
    void printf(const char* format, ...);
};

// ================= Input Classes =================

class Keyboard_Class {
public:
    struct KeysState {
        std::vector<char> word;
    };

    bool isChange();
    bool isPressed();
    KeysState keysState();
    
    // Sim helper
    void setKeys(std::vector<char> keys); 
};

// ================= Audio Classes =================

class Speaker_Class {
public:
    void setVolume(uint8_t volume);
    void tone(uint16_t frequency, uint32_t duration);
    void stop();
};

// ================= Main Hardware Class =================

class M5Cardputer_Class {
public:
    M5Display Display;
    Keyboard_Class Keyboard;
    Speaker_Class Speaker;

    struct Config {
        // dummy
    };

    void begin(Config config, bool enableSerial = true);
    void update();
};

extern M5Cardputer_Class M5Cardputer;

// M5 global for M5.config()
class M5_Class {
public:
    M5Cardputer_Class::Config config() { return M5Cardputer_Class::Config(); }
};
extern M5_Class M5;

#endif
