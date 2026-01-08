#ifdef SIMULATOR

#include "M5Cardputer.h"
#include <SDL2/SDL.h>
#include <iostream>
#include <chrono>
#include <vector>
#include <stdarg.h>

extern void setup();
extern void loop();

int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IOLBF, 0); // Line buffering
    printf("Sim: Starting...\n");
    setup();
    printf("Sim: Setup done. Entering loop...\n");
    while (true) {
        loop();
    }
    return 0;
}

// ================= Globals =================
M5Cardputer_Class M5Cardputer;
M5_Class M5;
SerialClass Serial;

// SDL State
static SDL_Window* window = nullptr;
static SDL_Renderer* renderer = nullptr;
static SDL_Texture* texture = nullptr; // For sprite buffer
static bool sdl_initialized = false;
static uint32_t* pixelBuffer = nullptr; // 240x135 buffer (ARGB8888)
static int screenW = 240;
static int screenH = 135;
static int scale = 3; // Scale up for visibility

// Audio State
struct AudioState {
    int frequency = 0;
    unsigned long endTime = 0;
    uint8_t volume = 128; // 0-255
    double phase = 0.0;
};
static AudioState audioState;
static SDL_AudioDeviceID audioDevice;

// Input State
static std::vector<char> pendingKeys; // Accumulates keys during delay/pump_events
static std::vector<char> currentKeys; // Exposed to the app for the current frame
static bool keyChanged = false;

// Time State
static auto startTime = std::chrono::steady_clock::now();

// Font Data (5x7 basic ASCII)
static const unsigned char font5x7[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, // space
    0x00, 0x00, 0x5F, 0x00, 0x00, // !
    0x00, 0x07, 0x00, 0x07, 0x00, // "
    0x14, 0x7F, 0x14, 0x7F, 0x14, // #
    0x24, 0x2A, 0x7F, 0x2A, 0x12, // $
    0x23, 0x13, 0x08, 0x64, 0x62, // %
    0x36, 0x49, 0x55, 0x22, 0x50, // &
    0x00, 0x05, 0x03, 0x00, 0x00, // '
    0x00, 0x1C, 0x22, 0x41, 0x00, // (
    0x00, 0x41, 0x22, 0x1C, 0x00, // )
    0x14, 0x08, 0x3E, 0x08, 0x14, // *
    0x08, 0x08, 0x3E, 0x08, 0x08, // +
    0x00, 0x50, 0x30, 0x00, 0x00, // ,
    0x08, 0x08, 0x08, 0x08, 0x08, // -
    0x00, 0x60, 0x60, 0x00, 0x00, // .
    0x20, 0x10, 0x08, 0x04, 0x02, // /
    0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
    0x00, 0x42, 0x7F, 0x40, 0x00, // 1
    0x42, 0x61, 0x51, 0x49, 0x46, // 2
    0x21, 0x41, 0x45, 0x4B, 0x31, // 3
    0x18, 0x14, 0x12, 0x7F, 0x10, // 4
    0x27, 0x45, 0x45, 0x45, 0x39, // 5
    0x3C, 0x4A, 0x49, 0x49, 0x30, // 6
    0x01, 0x71, 0x09, 0x05, 0x03, // 7
    0x36, 0x49, 0x49, 0x49, 0x36, // 8
    0x06, 0x49, 0x49, 0x29, 0x1E, // 9
    0x00, 0x36, 0x36, 0x00, 0x00, // :
    0x00, 0x56, 0x36, 0x00, 0x00, // ;
    0x08, 0x14, 0x22, 0x41, 0x00, // <
    0x14, 0x14, 0x14, 0x14, 0x14, // =
    0x00, 0x41, 0x22, 0x14, 0x08, // >
    0x02, 0x01, 0x51, 0x09, 0x06, // ?
    0x32, 0x49, 0x79, 0x41, 0x3E, // @
    0x7E, 0x11, 0x11, 0x11, 0x7E, // A
    0x7F, 0x49, 0x49, 0x49, 0x36, // B
    0x3E, 0x41, 0x41, 0x41, 0x22, // C
    0x7F, 0x41, 0x41, 0x22, 0x1C, // D
    0x7F, 0x49, 0x49, 0x49, 0x41, // E
    0x7F, 0x09, 0x09, 0x09, 0x01, // F
    0x3E, 0x41, 0x49, 0x49, 0x7A, // G
    0x7F, 0x08, 0x08, 0x08, 0x7F, // H
    0x00, 0x41, 0x7F, 0x41, 0x00, // I
    0x20, 0x40, 0x41, 0x3F, 0x01, // J
    0x7F, 0x08, 0x14, 0x22, 0x41, // K
    0x7F, 0x40, 0x40, 0x40, 0x40, // L
    0x7F, 0x02, 0x0C, 0x02, 0x7F, // M
    0x7F, 0x04, 0x08, 0x10, 0x7F, // N
    0x3E, 0x41, 0x41, 0x41, 0x3E, // O
    0x7F, 0x09, 0x09, 0x09, 0x06, // P
    0x3E, 0x41, 0x51, 0x21, 0x5E, // Q
    0x7F, 0x09, 0x19, 0x29, 0x46, // R
    0x46, 0x49, 0x49, 0x49, 0x31, // S
    0x01, 0x01, 0x7F, 0x01, 0x01, // T
    0x3F, 0x40, 0x40, 0x40, 0x3F, // U
    0x1F, 0x20, 0x40, 0x20, 0x1F, // V
    0x3F, 0x40, 0x38, 0x40, 0x3F, // W
    0x63, 0x14, 0x08, 0x14, 0x63, // X
    0x07, 0x08, 0x70, 0x08, 0x07, // Y
    0x61, 0x51, 0x49, 0x45, 0x43, // Z
    0x00, 0x7F, 0x41, 0x41, 0x00, // [
    0x02, 0x04, 0x08, 0x10, 0x20, // \ (escaped)
    0x00, 0x41, 0x41, 0x7F, 0x00, // ]
    0x04, 0x02, 0x01, 0x02, 0x04, // ^
    0x40, 0x40, 0x40, 0x40, 0x40  // _
};

extern void setup();

// ================= Arduino Mocks =================

// Helper to keep UI responsive
void pump_events() {
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) exit(0);
        
        if (e.type == SDL_KEYDOWN) {
            char key = 0;
            if (e.key.keysym.sym >= SDLK_a && e.key.keysym.sym <= SDLK_z) key = 'a' + (e.key.keysym.sym - SDLK_a);
            if (e.key.keysym.sym >= SDLK_0 && e.key.keysym.sym <= SDLK_9) key = '0' + (e.key.keysym.sym - SDLK_0);
            
            // Volume Controls
            if (e.key.keysym.sym == SDLK_MINUS) key = '-';
            if (e.key.keysym.sym == SDLK_EQUALS) key = '=';

            if (key != 0) pendingKeys.push_back(key);
            
            if (e.key.keysym.sym == SDLK_1 && (e.key.keysym.mod & KMOD_SHIFT)) {
                 pendingKeys.push_back('!');
            }
        }
    }
}

unsigned long millis() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
}

// Audio Callback
void audio_callback(void* userdata, Uint8* stream, int len) {
    AudioState* state = (AudioState*)userdata;
    int16_t* buffer = (int16_t*)stream;
    int length = len / 2; // 16-bit samples

    unsigned long now = millis();
    
    for (int i = 0; i < length; i++) {
        if (state->frequency > 0 && (state->endTime == 0 || now < state->endTime)) {
            // Square wave generation
            // Sample rate 44100
            double sampleRate = 44100.0;
            double samplesPerCycle = sampleRate / state->frequency;
            
            // Toggle based on phase
            state->phase += 1.0;
            if (state->phase >= samplesPerCycle) state->phase -= samplesPerCycle;
            
            int16_t sampleValue = (state->phase < samplesPerCycle / 2.0) ? 3000 : -3000;
            
            // Apply volume (simple scaling)
            buffer[i] = (sampleValue * state->volume) / 255;
        } else {
            buffer[i] = 0;
            state->phase = 0; // Reset phase on silence
        }
    }
}

void delay(unsigned long ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        pump_events();
        SDL_Delay(1);
    }
}

long random(long max) {
    return rand() % max;
}

long random(long min, long max) {
    return min + (rand() % (max - min));
}

void randomSeed(long seed) {
    srand(seed);
}

int analogRead(uint8_t pin) {
    return 0; 
}

// ================= Helper: Color Conversion =================
// Convert RGB565 (uint16_t) to ARGB8888 (uint32_t)
uint32_t rgb565to8888(uint16_t color) {
    uint8_t r = (color >> 11) & 0x1F;
    uint8_t g = (color >> 5) & 0x3F;
    uint8_t b = color & 0x1F;

    // Expand to 8-bit
    r = (r * 255) / 31;
    g = (g * 255) / 63;
    b = (b * 255) / 31;

    return (0xFF000000 | (r << 16) | (g << 8) | b);
}

// ================= M5Cardputer Implementation =================

void M5Cardputer_Class::begin(Config config, bool enableSerial) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return;
    }

    window = SDL_CreateWindow("Boo Simulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                              screenW * scale, screenH * scale, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return;
    }

    // Enable VSync to reduce tearing/flickering
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    // Create texture for framebuffer
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, screenW, screenH);
    pixelBuffer = new uint32_t[screenW * screenH];

    // Init Audio
    SDL_AudioSpec want, have;
    SDL_memset(&want, 0, sizeof(want));
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 2048;
    want.callback = audio_callback;
    want.userdata = &audioState;

    audioDevice = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audioDevice == 0) {
        printf("Failed to open audio: %s\n", SDL_GetError());
    } else {
        SDL_PauseAudioDevice(audioDevice, 0); // Start audio
    }

    sdl_initialized = true;
    startTime = std::chrono::steady_clock::now();
}

void M5Cardputer_Class::update() {
    // Process inputs
    pump_events(); // Poll any final events before frame start
    
    // Move pending keys to current keys for this frame
    currentKeys = pendingKeys;
    pendingKeys.clear();
    
    // Update change flag
    keyChanged = !currentKeys.empty();
    
    // Update Keyboard class state
    Keyboard.setKeys(currentKeys);
}

// ================= Keyboard Implementation =================

bool Keyboard_Class::isChange() {
    return keyChanged;
}

bool Keyboard_Class::isPressed() {
    return !currentKeys.empty();
}

Keyboard_Class::KeysState Keyboard_Class::keysState() {
    KeysState state;
    state.word = currentKeys;
    return state;
}

void Keyboard_Class::setKeys(std::vector<char> keys) {
    currentKeys = keys;
}

// ================= Speaker Implementation =================

void Speaker_Class::setVolume(uint8_t volume) {
    audioState.volume = volume;
}

void Speaker_Class::tone(uint16_t frequency, uint32_t duration) { 
    if (audioDevice == 0) return;
    SDL_LockAudioDevice(audioDevice);
    audioState.frequency = frequency;
    audioState.endTime = millis() + duration;
    SDL_UnlockAudioDevice(audioDevice);
}

void Speaker_Class::stop() {
    if (audioDevice == 0) return;
    SDL_LockAudioDevice(audioDevice);
    audioState.frequency = 0;
    SDL_UnlockAudioDevice(audioDevice);
}

// ================= Graphics Implementation =================

void M5Display::fillScreen(uint16_t color) {
    if (!pixelBuffer) return;
    uint32_t c = rgb565to8888(color);
    for (int i = 0; i < screenW * screenH; i++) pixelBuffer[i] = c;
}

void M5Display::setRotation(int r) { }
void M5Display::setTextColor(uint16_t color) { }
void M5Display::setTextSize(int size) { }
void M5Display::setCursor(int x, int y) { }
void M5Display::print(const char* s) { printf("LCD: %s", s); }
void M5Display::print(int n) { printf("LCD: %d", n); }
void M5Display::println(const char* s) { printf("LCD: %s\n", s); }
void M5Display::printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

// M5Canvas (The Sprite Buffer)

M5Canvas::M5Canvas(M5Display* display) { }

void M5Canvas::createSprite(int w, int h) {
    // For this app, w=240, h=135. We just use the main buffer.
}
void M5Canvas::deleteSprite() { }

void M5Canvas::pushSprite(int x, int y) {
    // This is where we actually RENDER to the window!
    if (sdl_initialized && texture) {
        SDL_UpdateTexture(texture, NULL, pixelBuffer, screenW * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
}

void M5Canvas::fillSprite(uint16_t color) {
    M5Cardputer.Display.fillScreen(color);
}

void M5Canvas::drawPixel(int x, int y, uint16_t color) {
    if (!pixelBuffer) return;
    if (x < 0 || x >= screenW || y < 0 || y >= screenH) return;
    pixelBuffer[y * screenW + x] = rgb565to8888(color);
}

void M5Canvas::fillCircle(int x0, int y0, int r, uint16_t color) {
    // Bresenham-like or simple bounding box
    int r2 = r * r;
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x*x + y*y <= r2) drawPixel(x0+x, y0+y, color);
        }
    }
}

void M5Canvas::fillRect(int x, int y, int w, int h, uint16_t color) {
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            drawPixel(x + i, y + j, color);
        }
    }
}

void M5Canvas::drawLine(int x0, int y0, int x1, int y1, uint16_t color) {
    // Bresenham's line algorithm
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    
    while (1) {
        drawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void M5Canvas::fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color) {
    // Simple scanline or bounding box (slow but works for small triangles)
    int minX = min(x0, min(x1, x2));
    int maxX = max(x0, max(x1, x2));
    int minY = min(y0, min(y1, y2));
    int maxY = max(y0, max(y1, y2));
    
    // Barycentric coordinates method could work too, but let's stick to bounding box for simplicity on modern CPU
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
             // Check if point in triangle
             int w0 = (x1 - x0) * (y - y0) - (y1 - y0) * (x - x0);
             int w1 = (x2 - x1) * (y - y1) - (y2 - y1) * (x - x1);
             int w2 = (x0 - x2) * (y - y2) - (y0 - y2) * (x - x2);
             
             if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                 drawPixel(x, y, color);
             }
        }
    }
}

// Text Rendering State
static int cursorX = 0;
static int cursorY = 0;
static uint16_t txtColor = 0xFFFF;
static int txtSize = 1;

void M5Canvas::setTextColor(uint16_t color) { txtColor = color; }
void M5Canvas::setTextSize(int size) { txtSize = size; }
void M5Canvas::setCursor(int x, int y) { cursorX = x; cursorY = y; }

void drawChar(int x, int y, char c, uint16_t color, int size) {
    if (c < 32 || c > 95) return;
    const unsigned char* bitmap = font5x7 + (c - 32) * 5;
    
    for (int i = 0; i < 5; i++) {
        unsigned char line = bitmap[i];
        for (int j = 0; j < 7; j++) {
            if (line & (1 << j)) {
                // Draw rect for scaled pixel
                for (int sx = 0; sx < size; sx++) {
                    for (int sy = 0; sy < size; sy++) {
                        // font data is usually LSb top? 
                        // The array I pasted:
                        // 0x00, 0x00, 0x5F, 0x00, 0x00 for '!'. 0x5F = 0101 1111. 
                        // It seems bit 0 is top.
                        // Let's verify.
                        // M5Stack fonts are usually column-major.
                        // font5x7 standard is usually column major.
                        M5Cardputer.Display.fillScreen(color); // Just kidding, calling drawPixel
                        // Need access to drawPixel. M5Canvas has it.
                        // But we are outside M5Canvas method here? No, we can make drawChar a helper.
                        // Or just inline it.
                    }
                }
            }
        }
    }
}

// Helper to draw char
void drawCharInternal(int x, int y, char c, uint16_t color, int size) {
    if (!pixelBuffer) return; // Prevent segfault if not initialized
    if (c < 32 || c > 95) return;
    const unsigned char* bitmap = font5x7 + (c - 32) * 5;
    
    for (int col = 0; col < 5; col++) {
        unsigned char line = bitmap[col];
        for (int row = 0; row < 7; row++) {
            if (line & (1 << row)) {
                // Draw scaled pixel
                for (int sx = 0; sx < size; sx++) {
                    for (int sy = 0; sy < size; sy++) {
                        // Drawing directly to buffer using the global pixelBuffer logic
                        int px = x + col * size + sx;
                        int py = y + row * size + sy;
                        if (px >= 0 && px < screenW && py >= 0 && py < screenH) {
                            pixelBuffer[py * screenW + px] = rgb565to8888(color);
                        }
                    }
                }
            }
        }
    }
}

void M5Canvas::print(const char* s) {
    // Debug print to console
    ::printf("LCD: %s\n", s); 
    
    while (*s) {
        drawCharInternal(cursorX, cursorY, *s, txtColor, txtSize);
        cursorX += 6 * txtSize; // 5 width + 1 spacing
        s++;
    }
}

void M5Canvas::print(int n) {
    char buf[32];
    sprintf(buf, "%d", n);
    print(buf);
}

void M5Canvas::printf(const char* format, ...) {
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    print(buf);
}

#endif
