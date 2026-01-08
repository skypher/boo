/**
 * Boo - A ghostly Tamagotchi for M5Stack Cardputer
 * Ported from MicroHydra/MicroPython to Arduino/C++
 */

#include <M5Cardputer.h>
#include <Preferences.h>
#include <cstring>
#include <cstdlib>

#if ESP32
#include "soc/rtc_cntl_reg.h"
#include "esp_system.h"
#endif

#include "BooGame.h" // Include our verified game logic

// Double buffer sprite to prevent flickering
M5Canvas canvas(&M5Cardputer.Display);

// ============== Constants ==============
// SCREEN_WIDTH, SCREEN_HEIGHT, GHOST_SIZE are now in BooGame.h

// Colors - cute pink/purple theme!
#define COLOR_BG 0x2808           // Dark purple (darker)
#define COLOR_GHOST 0xFDFF        // Light pink
#define COLOR_GHOST_CHEEKS 0xFACF // Rosy pink
#define COLOR_TEXT 0xFFFF         // White
#define COLOR_HIGHLIGHT 0xF81F    // Magenta
#define COLOR_HEART 0xF88F        // Pink/red
#define COLOR_STAR 0xFFE0         // Yellow
#define COLOR_SPARKLE 0xCFFF      // Light cyan
#define COLOR_FOOD_RED 0xF800     // Red
#define COLOR_FOOD_GREEN 0x07E0   // Green
#define COLOR_FOOD_ORANGE 0xFD20  // Orange
#define COLOR_FOOD_BROWN 0xA145   // Brown
#define COLOR_FOOD_PURPLE 0x780F  // Purple
#define COLOR_FOOD_BLUE 0x001F    // Blue
#define COLOR_FOOD_YELLOW COLOR_STAR
#define COLOR_FOOD_WHITE COLOR_TEXT
#define COLOR_FOOD_PINK COLOR_HIGHLIGHT
#define COLOR_FOOD_BLACK 0x0000

// ============== Game State ==============
Preferences prefs;
BooGame game; // Use the library class

// Sparkle positions (persistent between frames)
struct Sparkle { int x, y; uint16_t color; int life; };
Sparkle sparkles[8];

// ============== Music Notes ==============
#define NOTE_G3  196
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_Bb4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784

// Happy Birthday melody (with rests as 0)
const int happyBirthday[] = {
    // "Hap-py birth-day to you"
    NOTE_C4, NOTE_C4, NOTE_D4, NOTE_C4, NOTE_F4, NOTE_E4, 0,
    // "Hap-py birth-day to you"
    NOTE_C4, NOTE_C4, NOTE_D4, NOTE_C4, NOTE_G4, NOTE_F4, 0,
    // "Hap-py birth-day dear Boo-oo"
    NOTE_C4, NOTE_C4, NOTE_C5, NOTE_A4, NOTE_F4, NOTE_E4, NOTE_D4, 0,
    // "Hap-py birth-day to you"
    NOTE_Bb4, NOTE_Bb4, NOTE_A4, NOTE_F4, NOTE_G4, NOTE_F4, 0, 0
};
const int happyBirthdayDurations[] = {
    150, 150, 300, 300, 300, 600, 400,
    150, 150, 300, 300, 300, 600, 400,
    150, 150, 300, 300, 300, 300, 600, 400,
    150, 150, 300, 300, 300, 800, 600, 800
};
const int happyBirthdayLen = 30;
int musicIndex = 0;
unsigned long lastNoteTime = 0;

// Volume control
int volume = 255;
bool muted = false;
bool smokeMode = false;
bool smokeDone = false;
const unsigned long smokeSceneMs = 5000;

// ============== Helper Functions ==============

void enterDownloadMode() {
    M5Cardputer.Display.fillScreen(0x0000);
    M5Cardputer.Display.setTextColor(0xFFFF);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(20, 50);
    M5Cardputer.Display.println("FLASH MODE");
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(20, 80);
    M5Cardputer.Display.println("Ready for esptool...");
    delay(500);
    #if ESP32
    REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
    esp_restart();
    #else
    M5Cardputer.Display.println("Sim: Restarting...");
    #endif
}

void playNote(int freq, int duration) {
    M5Cardputer.Speaker.tone(freq, duration);
    delay(duration + 20);
}

inline bool smokeTimedOut(unsigned long startMs) {
    return smokeMode && (millis() - startMs >= smokeSceneMs);
}

void smokeHold(unsigned long startMs) {
    while (!smokeTimedOut(startMs)) {
        M5Cardputer.update();
        delay(33);
    }
}

// ============== Drawing Functions (use canvas) ==============

void drawSparkle(int x, int y, uint16_t color) {
    canvas.drawPixel(x, y, color);
    canvas.drawPixel(x-1, y, color);
    canvas.drawPixel(x+1, y, color);
    canvas.drawPixel(x, y-1, color);
    canvas.drawPixel(x, y+1, color);
}

void drawHeart(int x, int y, uint16_t color) {
    canvas.fillCircle(x - 3, y, 4, color);
    canvas.fillCircle(x + 3, y, 4, color);
    canvas.fillTriangle(x - 7, y + 2, x + 7, y + 2, x, y + 10, color);
}

void drawStar(int x, int y, int size, uint16_t color) {
    canvas.fillTriangle(x, y - size, x - size/2, y + size/2, x + size/2, y + size/2, color);
    canvas.fillTriangle(x, y + size, x - size/2, y - size/2, x + size/2, y - size/2, color);
}

void drawGhost(int x, int y, bool blinking, bool dancing = false, int danceFrame = 0) {
    int wobble = dancing ? (danceFrame % 2 == 0 ? -3 : 3) : 0;
    int squish = dancing ? (danceFrame % 4 < 2 ? 2 : -2) : 0;

    // Ghost body
    canvas.fillCircle(x + 16 + wobble, y + 14 - squish, 16, COLOR_GHOST);
    canvas.fillRect(x + wobble, y + 14 - squish, 32, 18 + squish, COLOR_GHOST);

    // Wavy bottom
    for (int i = 0; i < 4; i++) {
        int bx = x + i * 8 + wobble;
        int waveOffset = dancing ? ((i + danceFrame) % 2) * 2 : 0;
        canvas.fillCircle(bx + 4, y + 30 + squish + waveOffset, 5, COLOR_GHOST);
    }

    // Rosy cheeks
    canvas.fillCircle(x + 6 + wobble, y + 18, 4, COLOR_GHOST_CHEEKS);
    canvas.fillCircle(x + 26 + wobble, y + 18, 4, COLOR_GHOST_CHEEKS);

    // Eyes
    if (!blinking) {
        canvas.fillCircle(x + 10 + wobble, y + 12, 5, COLOR_BG);
        canvas.fillCircle(x + 22 + wobble, y + 12, 5, COLOR_BG);
        canvas.fillCircle(x + 11 + wobble, y + 13, 2, COLOR_GHOST);
        canvas.fillCircle(x + 23 + wobble, y + 13, 2, COLOR_GHOST);
        canvas.fillCircle(x + 8 + wobble, y + 10, 1, COLOR_TEXT);
        canvas.fillCircle(x + 20 + wobble, y + 10, 1, COLOR_TEXT);
    } else {
        // Happy ^_^ eyes
        canvas.drawLine(x + 6 + wobble, y + 14, x + 10 + wobble, y + 10, COLOR_BG);
        canvas.drawLine(x + 10 + wobble, y + 10, x + 14 + wobble, y + 14, COLOR_BG);
        canvas.drawLine(x + 18 + wobble, y + 14, x + 22 + wobble, y + 10, COLOR_BG);
        canvas.drawLine(x + 22 + wobble, y + 10, x + 26 + wobble, y + 14, COLOR_BG);
    }

    // Smile (bigger when dancing)
    if (dancing) {
        canvas.fillCircle(x + 16 + wobble, y + 22, 4, COLOR_BG);
        canvas.fillRect(x + 12 + wobble, y + 18, 8, 4, COLOR_GHOST);
    } else {
        canvas.drawLine(x + 12, y + 22, x + 16, y + 24, COLOR_BG);
        canvas.drawLine(x + 16, y + 24, x + 20, y + 22, COLOR_BG);
    }
}

void drawGhostEating(int x, int y, int frame) {
    drawGhost(x, y, frame % 3 == 0);
    // Open/close mouth
    if (frame % 2 == 0) {
        canvas.fillCircle(x + 16, y + 22, 5, COLOR_BG);
    }
}

// ============== Food Pixel Art ==============

struct FoodPainter {
    int x;
    int y;
    int s;
    void pixel(int px, int py, uint16_t color) {
        canvas.fillRect(x + px * s, y + py * s, s, s, color);
    }
    void rect(int px, int py, int w, int h, uint16_t color) {
        canvas.fillRect(x + px * s, y + py * s, w * s, h * s, color);
    }
    void circle(int px, int py, int r, uint16_t color) {
        canvas.fillCircle(x + px * s, y + py * s, r * s, color);
    }
    void line(int x0, int y0, int x1, int y1, uint16_t color) {
        int sx0 = x + x0 * s;
        int sy0 = y + y0 * s;
        int sx1 = x + x1 * s;
        int sy1 = y + y1 * s;
        canvas.drawLine(sx0, sy0, sx1, sy1, color);
        if (s > 1) {
            for (int t = 1; t < s; t++) {
                canvas.drawLine(sx0 + t, sy0, sx1 + t, sy1, color);
                canvas.drawLine(sx0, sy0 + t, sx1, sy1 + t, color);
            }
        }
    }
    void tri(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color) {
        canvas.fillTriangle(x + x0 * s, y + y0 * s, x + x1 * s, y + y1 * s, x + x2 * s, y + y2 * s, color);
    }
};

void drawFoodApple(int x, int y, int scale) {
    FoodPainter p{x, y, scale};
    p.circle(8, 12, 6, COLOR_FOOD_RED);
    p.circle(14, 12, 6, COLOR_FOOD_RED);
    p.tri(6, 8, 16, 8, 11, 2, COLOR_FOOD_RED);
    p.rect(10, 1, 2, 4, COLOR_FOOD_BROWN);
    p.tri(12, 2, 18, 6, 12, 6, COLOR_FOOD_GREEN);
}

void drawFoodBanana(int x, int y, int scale) {
    FoodPainter p{x, y, scale};
    p.rect(4, 12, 14, 3, COLOR_FOOD_YELLOW);
    p.rect(6, 10, 12, 3, COLOR_FOOD_YELLOW);
    p.rect(8, 8, 10, 3, COLOR_FOOD_YELLOW);
    p.rect(10, 6, 6, 3, COLOR_FOOD_YELLOW);
    p.pixel(3, 12, COLOR_FOOD_BROWN);
    p.pixel(18, 12, COLOR_FOOD_BROWN);
}

void drawFoodCherry(int x, int y, int scale) {
    FoodPainter p{x, y, scale};
    p.circle(6, 14, 4, COLOR_FOOD_RED);
    p.circle(14, 14, 4, COLOR_FOOD_RED);
    p.line(6, 10, 10, 4, COLOR_FOOD_GREEN);
    p.line(14, 10, 10, 4, COLOR_FOOD_GREEN);
    p.circle(10, 4, 2, COLOR_FOOD_GREEN);
}

void drawFoodGrape(int x, int y, int scale) {
    FoodPainter p{x, y, scale};
    p.circle(10, 5, 3, COLOR_FOOD_PURPLE);
    p.circle(6, 9, 3, COLOR_FOOD_PURPLE);
    p.circle(14, 9, 3, COLOR_FOOD_PURPLE);
    p.circle(6, 13, 3, COLOR_FOOD_PURPLE);
    p.circle(10, 13, 3, COLOR_FOOD_PURPLE);
    p.circle(14, 13, 3, COLOR_FOOD_PURPLE);
    p.circle(10, 17, 3, COLOR_FOOD_PURPLE);
    p.line(10, 2, 10, 5, COLOR_FOOD_GREEN);
    p.circle(13, 3, 2, COLOR_FOOD_GREEN);
}

void drawFoodMango(int x, int y, int scale) {
    FoodPainter p{x, y, scale};
    p.circle(11, 12, 7, COLOR_FOOD_ORANGE);
    p.circle(15, 10, 5, COLOR_FOOD_ORANGE);
    p.tri(9, 4, 16, 4, 12, 0, COLOR_FOOD_GREEN);
}

void drawFoodPizza(int x, int y, int scale) {
    FoodPainter p{x, y, scale};
    p.tri(4, 4, 20, 12, 4, 20, COLOR_FOOD_YELLOW);
    p.tri(4, 4, 8, 6, 4, 20, COLOR_FOOD_BROWN);
    p.line(4, 4, 20, 12, COLOR_FOOD_BROWN);
    p.circle(10, 10, 2, COLOR_FOOD_RED);
    p.circle(14, 13, 2, COLOR_FOOD_RED);
    p.circle(8, 14, 2, COLOR_FOOD_RED);
}

void drawFoodBurger(int x, int y, int scale) {
    FoodPainter p{x, y, scale};
    p.rect(4, 6, 16, 4, COLOR_FOOD_ORANGE);
    p.rect(4, 10, 16, 3, COLOR_FOOD_BROWN);
    p.rect(4, 13, 16, 2, COLOR_FOOD_GREEN);
    p.rect(4, 15, 16, 4, COLOR_FOOD_ORANGE);
    p.rect(8, 9, 8, 1, COLOR_FOOD_YELLOW);
}

void drawFoodTaco(int x, int y, int scale) {
    FoodPainter p{x, y, scale};
    p.tri(6, 16, 18, 16, 12, 6, COLOR_FOOD_ORANGE);
    p.rect(9, 12, 6, 2, COLOR_FOOD_GREEN);
    p.rect(10, 10, 4, 2, COLOR_FOOD_RED);
}

void drawFoodSushi(int x, int y, int scale) {
    FoodPainter p{x, y, scale};
    p.rect(5, 7, 14, 12, COLOR_FOOD_BLACK);
    p.rect(6, 8, 12, 10, COLOR_FOOD_WHITE);
    p.rect(6, 12, 12, 3, COLOR_FOOD_BLACK);
    p.rect(9, 9, 6, 3, COLOR_FOOD_RED);
}

void drawFoodRamen(int x, int y, int scale) {
    FoodPainter p{x, y, scale};
    p.rect(5, 14, 14, 6, COLOR_FOOD_BLUE);
    p.rect(6, 12, 12, 3, COLOR_FOOD_WHITE);
    p.line(6, 11, 17, 11, COLOR_FOOD_YELLOW);
    p.line(6, 9, 17, 9, COLOR_FOOD_YELLOW);
    p.line(6, 7, 17, 7, COLOR_FOOD_YELLOW);
}

void drawFoodCookie(int x, int y, int scale) {
    FoodPainter p{x, y, scale};
    p.circle(12, 12, 7, COLOR_FOOD_BROWN);
    p.circle(9, 10, 1, COLOR_FOOD_BLACK);
    p.circle(14, 9, 1, COLOR_FOOD_BLACK);
    p.circle(12, 14, 1, COLOR_FOOD_BLACK);
    p.circle(7, 14, 1, COLOR_FOOD_BLACK);
}

void drawFoodCake(int x, int y, int scale) {
    FoodPainter p{x, y, scale};
    p.rect(6, 10, 12, 8, COLOR_FOOD_PINK);
    p.rect(6, 8, 12, 3, COLOR_FOOD_WHITE);
    p.rect(11, 4, 2, 4, COLOR_FOOD_YELLOW);
    p.pixel(12, 3, COLOR_FOOD_RED);
}

void drawFoodDonut(int x, int y, int scale) {
    FoodPainter p{x, y, scale};
    p.circle(12, 12, 7, COLOR_FOOD_ORANGE);
    p.circle(12, 12, 4, COLOR_BG);
    p.circle(12, 10, 6, COLOR_FOOD_PINK);
    p.circle(12, 10, 3, COLOR_BG);
}

void drawFoodCandy(int x, int y, int scale) {
    FoodPainter p{x, y, scale};
    p.rect(9, 10, 8, 6, COLOR_FOOD_PINK);
    p.tri(5, 13, 9, 10, 9, 16, COLOR_FOOD_PINK);
    p.tri(17, 10, 21, 13, 17, 16, COLOR_FOOD_PINK);
    p.line(10, 12, 16, 12, COLOR_FOOD_WHITE);
}

void drawFoodChoco(int x, int y, int scale) {
    FoodPainter p{x, y, scale};
    p.rect(6, 8, 12, 12, COLOR_FOOD_BROWN);
    p.line(10, 8, 10, 19, COLOR_FOOD_BLACK);
    p.line(14, 8, 14, 19, COLOR_FOOD_BLACK);
    p.line(6, 12, 17, 12, COLOR_FOOD_BLACK);
    p.line(6, 16, 17, 16, COLOR_FOOD_BLACK);
}

void drawFoodFries(int x, int y, int scale) {
    FoodPainter p{x, y, scale};
    p.rect(7, 14, 10, 6, COLOR_FOOD_RED);
    p.rect(6, 8, 2, 6, COLOR_FOOD_YELLOW);
    p.rect(9, 6, 2, 8, COLOR_FOOD_YELLOW);
    p.rect(12, 7, 2, 7, COLOR_FOOD_YELLOW);
    p.rect(15, 8, 2, 6, COLOR_FOOD_YELLOW);
}

void drawFoodSteak(int x, int y, int scale) {
    FoodPainter p{x, y, scale};
    p.circle(12, 12, 7, COLOR_FOOD_RED);
    p.circle(12, 12, 5, COLOR_FOOD_BROWN);
    p.line(8, 12, 16, 12, COLOR_FOOD_WHITE);
}

void drawFoodSalad(int x, int y, int scale) {
    FoodPainter p{x, y, scale};
    p.rect(6, 16, 12, 4, COLOR_FOOD_BROWN);
    p.circle(8, 12, 4, COLOR_FOOD_GREEN);
    p.circle(12, 10, 4, COLOR_FOOD_GREEN);
    p.circle(16, 12, 4, COLOR_FOOD_GREEN);
    p.pixel(12, 12, COLOR_FOOD_RED);
}

void drawFoodBread(int x, int y, int scale) {
    FoodPainter p{x, y, scale};
    p.rect(6, 10, 12, 8, COLOR_FOOD_ORANGE);
    p.circle(8, 10, 4, COLOR_FOOD_ORANGE);
    p.circle(16, 10, 4, COLOR_FOOD_ORANGE);
}

void drawFoodEgg(int x, int y, int scale) {
    FoodPainter p{x, y, scale};
    p.circle(12, 12, 7, COLOR_FOOD_WHITE);
    p.circle(12, 12, 3, COLOR_FOOD_YELLOW);
}

// ============== Scenes ==============

void feedScene() {
    // Happy feeding melody
    const int melody[] = {NOTE_C5, NOTE_E5, NOTE_G5, NOTE_E5, NOTE_C5};
    const int durations[] = {100, 100, 200, 100, 200};

    struct FoodItem { const char* name; void (*draw)(int, int, int); };
    const FoodItem foodItems[] = {
        {"APPLE", drawFoodApple},
        {"BANANA", drawFoodBanana},
        {"CHERRY", drawFoodCherry},
        {"GRAPE", drawFoodGrape},
        {"MANGO", drawFoodMango},
        {"PIZZA", drawFoodPizza},
        {"BURGER", drawFoodBurger},
        {"TACO", drawFoodTaco},
        {"SUSHI", drawFoodSushi},
        {"RAMEN", drawFoodRamen},
        {"COOKIE", drawFoodCookie},
        {"CAKE", drawFoodCake},
        {"DONUT", drawFoodDonut},
        {"CANDY", drawFoodCandy},
        {"CHOCOLATE", drawFoodChoco},
        {"FRIES", drawFoodFries},
        {"STEAK", drawFoodSteak},
        {"SALAD", drawFoodSalad},
        {"BREAD", drawFoodBread},
        {"EGG", drawFoodEgg},
    };
    const int foodCount = sizeof(foodItems) / sizeof(foodItems[0]);
    const FoodItem selectedFood = foodItems[random(foodCount)];

    unsigned long sceneStart = millis();
    const int foodFrames = 15;
    const int eatFrames = 20;
    const int foodScale = 2;
    const int foodBaseSize = 22;
    const int foodSize = foodBaseSize * foodScale;
    const int nameSize = 2;
    const int nameHeight = 8 * nameSize;
    const int groupSpacing = 6;
    const char* thanksText = "SO YUMMY!";
    const int thanksSize = 2;
    const int thanksHeight = 8 * thanksSize;

    for (int frame = 0; frame < foodFrames; frame++) {
        if (smokeTimedOut(sceneStart)) return;
        canvas.fillSprite(COLOR_BG);

        const int nameWidth = strlen(selectedFood.name) * 6 * nameSize;
        const int groupHeight = foodSize + groupSpacing + nameHeight;
        const int foodX = (SCREEN_WIDTH - foodSize) / 2;
        const int baseFoodY = (SCREEN_HEIGHT - groupHeight) / 2;
        const int textX = (SCREEN_WIDTH - nameWidth) / 2;
        const int textY = baseFoodY + foodSize + groupSpacing;
        int bounce = (frame % 6 < 3) ? 0 : 1;
        selectedFood.draw(foodX, baseFoodY + bounce * foodScale, foodScale);

        canvas.setTextColor(COLOR_HIGHLIGHT);
        canvas.setTextSize(nameSize);
        canvas.setCursor(textX, textY);
        canvas.print(selectedFood.name);

        canvas.pushSprite(0, 0);
        delay(100);
    }

    for (int frame = 0; frame < eatFrames; frame++) {
        if (smokeTimedOut(sceneStart)) return;
        canvas.fillSprite(COLOR_BG);

        // Ghost eating animation (same for all foods)
        drawGhostEating(104, 50, frame);

        // Floating hearts
        for (int h = 0; h < 4; h++) {
            int heartY = 90 - (frame * 4) - (h * 25);
            int heartX = 160 + sin(frame * 0.5 + h) * 15;
            if (heartY > 5 && heartY < 130) {
                drawHeart(heartX, heartY, COLOR_HEART);
            }
        }

        // Stars bursting out
        if (frame > 5) {
            for (int s = 0; s < 6; s++) {
                int angle = s * 60 + frame * 10;
                int dist = (frame - 5) * 8;
                int sx = 120 + cos(angle * 0.0174) * dist;
                int sy = 65 + sin(angle * 0.0174) * dist * 0.5;
                if (sx > 0 && sx < 240 && sy > 0 && sy < 135) {
                    drawStar(sx, sy, 4, COLOR_STAR);
                }
            }
        }

        if (frame > 2) {
            const int thanksWidth = strlen(thanksText) * 6 * thanksSize;
            const int thanksX = (SCREEN_WIDTH - thanksWidth) / 2;
            const int thanksY = SCREEN_HEIGHT - thanksHeight - 12;
            canvas.setTextColor(COLOR_STAR);
            canvas.setTextSize(thanksSize);
            canvas.setCursor(thanksX, thanksY);
            canvas.print(thanksText);
        }

        canvas.pushSprite(0, 0);

        // Play melody
        if (frame < 5) {
            playNote(melody[frame], durations[frame]);
        } else {
            delay(100);
        }
    }

    // Celebration
    for (int i = 0; i < 10; i++) {
        if (smokeTimedOut(sceneStart)) return;
        canvas.fillSprite(COLOR_BG);
        drawGhost(104, 50, i % 2 == 0);

        // Explosion of stars
        for (int s = 0; s < 12; s++) {
            drawStar(random(240), random(120), random(3, 7), COLOR_STAR);
        }

        canvas.setTextColor(COLOR_STAR);
        canvas.setTextSize(2);
        const char* yummyText = "SO YUMMY!";
        int yummyWidth = strlen(yummyText) * 6 * 2;
        int yummyX = (SCREEN_WIDTH - yummyWidth) / 2;
        canvas.setCursor(yummyX, 10);
        canvas.print(yummyText);

        canvas.pushSprite(0, 0);
        playNote(NOTE_C5 + i * 50, 80);
    }

    if (smokeMode) {
        smokeHold(sceneStart);
    }
}

void danceScene() {
    // Yankee Doodle melody
    const int melody[] = {
        // "Yankee Doodle went to town"
        NOTE_C4, NOTE_C4, NOTE_D4, NOTE_E4, NOTE_C4, NOTE_E4, NOTE_D4, 0,
        // "Riding on a pony"
        NOTE_C4, NOTE_C4, NOTE_D4, NOTE_E4, NOTE_C4, 0, NOTE_B4, 0,
        // "Stuck a feather in his cap"
        NOTE_C4, NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4, NOTE_E4, NOTE_D4, NOTE_C4,
        // "And called it macaroni"
        NOTE_B4, NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5, NOTE_C5, 0, 0
    };
    const int beats[] = {
        1, 1, 1, 1, 1, 1, 2, 1,
        1, 1, 1, 1, 2, 1, 2, 1,
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 2, 2, 1, 2
    };
    int tempo = 120;

    unsigned long sceneStart = millis();
    for (int frame = 0; frame < 60; frame++) {
        if (smokeTimedOut(sceneStart)) return;
        canvas.fillSprite(COLOR_BG);

        // Dancing ghost in center
        drawGhost(104, 55, frame % 8 < 2, true, frame);

        // Musical notes floating
        for (int n = 0; n < 4; n++) {
            int noteX = 40 + n * 50 + sin(frame * 0.2 + n) * 20;
            int noteY = 20 + cos(frame * 0.15 + n) * 15;
            canvas.setTextColor(n % 2 == 0 ? COLOR_STAR : COLOR_HEART);
            canvas.setTextSize(2);
            canvas.setCursor(noteX, noteY);
            canvas.print((char)14); // Musical note character
        }

        // Sparkles
        for (int s = 0; s < 5; s++) {
            int sx = random(240);
            int sy = random(110);
            drawSparkle(sx, sy, frame % 2 == 0 ? COLOR_SPARKLE : COLOR_STAR);
        }

        // Bouncing stars at bottom
        for (int s = 0; s < 5; s++) {
            int bounce = abs((frame + s * 3) % 10 - 5) * 2;
            drawStar(30 + s * 45, 115 - bounce, 5, COLOR_STAR);
        }

        canvas.pushSprite(0, 0);

        // Play melody note
        int noteIdx = frame % 32;
        if (melody[noteIdx] > 0 && !muted) {
            M5Cardputer.Speaker.tone(melody[noteIdx], beats[noteIdx] * tempo - 20);
        }
        delay(beats[noteIdx] * tempo);
    }

    // Final pose
    if (smokeTimedOut(sceneStart)) return;
    canvas.fillSprite(COLOR_BG);
    drawGhost(104, 55, true);
    for (int s = 0; s < 8; s++) {
        drawStar(30 + s * 28, 20, 6, COLOR_STAR);
    }
    canvas.setTextColor(COLOR_HIGHLIGHT);
    canvas.setTextSize(2);
    canvas.setCursor(70, 110);
    canvas.print("YAY!");
    canvas.pushSprite(0, 0);

    playNote(NOTE_C5, 150);
    playNote(NOTE_E5, 150);
    playNote(NOTE_G5, 300);
    delay(500);

    if (smokeMode) {
        smokeHold(sceneStart);
    }
}

void marchScene() {
    // "Johnny I Hardly Knew Ye" (When Johnny Comes Marching Home) in C
    const int melody[] = {
        // "When Johnny comes marching home again"
        NOTE_G4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_F4, NOTE_E4, NOTE_D4,
        // "Hurrah, hurrah"
        NOTE_C4, NOTE_E4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_F4, NOTE_E4,
        // "We'll give him a hearty welcome then"
        NOTE_D4, NOTE_D4, NOTE_E4, NOTE_F4, NOTE_G4, NOTE_E4, NOTE_C4, NOTE_D4,
        // "Hurrah, hurrah"
        NOTE_E4, NOTE_F4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_F4, NOTE_E4, NOTE_D4
    };
    const int beats[] = {
        1, 1, 1, 1, 1, 1, 1, 2,
        1, 1, 1, 1, 1, 1, 1, 2,
        1, 1, 1, 1, 1, 1, 1, 2,
        1, 1, 1, 1, 1, 1, 1, 2
    };
    const int melodyLen = sizeof(melody) / sizeof(melody[0]);
    int tempo = 160;

    float leadX = 0;
    
    // Animation loop (run indefinitely until key press)
    unsigned long startScene = millis();
    int noteIdx = 0;
    unsigned long nextNoteTime = 0;

    while (true) {
        unsigned long now = millis();
        if (smokeTimedOut(startScene)) break;

        // Music (Looping)
        if (!muted && now >= nextNoteTime) {
            if (noteIdx >= melodyLen) noteIdx = 0; // Loop melody
            
            int duration = beats[noteIdx] * tempo;
            if (melody[noteIdx] > 0) {
                M5Cardputer.Speaker.tone(melody[noteIdx], duration - 20);
            }
            nextNoteTime = now + duration;
            noteIdx++;
        }

        // Render
        canvas.fillSprite(COLOR_BG);

        int frame = (now - startScene) / 50; // Animation frame counter
        
        // Infinite stream of marching ghosts
        // Ghosts are positioned at: leadX - (i * 45)
        // We only draw those visible on screen (-40 to 280)
        
        // i * 45 < leadX + 40  -> i < (leadX + 40) / 45
        // i * 45 > leadX - 280 -> i > (leadX - 280) / 45
        
        int maxI = floor((leadX + 40) / 45.0);
        int minI = floor((leadX - 280) / 45.0);

        for (int i = minI; i <= maxI; i++) {
            float gx = leadX - (i * 45);
            // Bobbing motion linked to identity 'i'
            int marchBob = abs((frame + i * 2) % 8 - 4) * 3;
            drawGhost((int)gx, 60 - marchBob, frame % 4 < 2, false, frame + i);
        }

        canvas.setTextColor(COLOR_TEXT);
        canvas.setTextSize(2);
        canvas.setCursor(60, 20);
        canvas.print("MARCHING!");

        canvas.pushSprite(0, 0);

        // Update Position
        leadX += 1.5; // Walking speed

        // Handle Exit
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) break;

        delay(33);
    }
    if (!smokeMode) delay(200);
}

void gameScene() {
    unsigned long sceneStart = millis();
    int score = 0;
    int rounds = 5;
    int gx = 104;

    // Instructions
    canvas.fillSprite(COLOR_BG);
    canvas.setTextColor(COLOR_STAR);
    canvas.setTextSize(2);
    canvas.setCursor(25, 20);
    canvas.print("CATCH STARS!");
    canvas.setTextColor(COLOR_TEXT);
    canvas.setTextSize(1);
    canvas.setCursor(15, 55);
    canvas.print("Press ANY KEY when star");
    canvas.setCursor(15, 70);
    canvas.print("is above the ghost!");
    canvas.setCursor(45, 110);
    canvas.print("Press key to start...");
    canvas.pushSprite(0, 0);

    if (!smokeMode) {
        while (true) {
            if (smokeTimedOut(sceneStart)) return;
            M5Cardputer.update();
            if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) break;
            delay(50);
        }
        delay(200);
    } else {
        delay(200);
    }

    for (int round = 0; round < rounds; round++) {
        if (smokeTimedOut(sceneStart)) return;
        int starX = -20;
        int speed = 4 + random(3);
        bool caught = false;
        bool done = false;

        while (!done && starX < 260) {
            if (smokeTimedOut(sceneStart)) return;
            canvas.fillSprite(COLOR_BG);
            drawGhost(gx, 75, false);
            drawStar(starX, 30, 10, COLOR_STAR);

            canvas.setTextColor(COLOR_HIGHLIGHT);
            canvas.setTextSize(1);
            canvas.setCursor(5, 5);
            canvas.printf("Stars: %d/%d", score, rounds);

            canvas.pushSprite(0, 0);

            M5Cardputer.update();
            if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
                if (abs(starX - (gx + 16)) < 35) {
                    caught = true;
                    score++;
                }
                done = true;
            }

            starX += speed;
            delay(25);
        }

        // Result
        if (smokeTimedOut(sceneStart)) return;
        canvas.fillSprite(COLOR_BG);
        drawGhost(gx, 75, true);
        if (caught) {
            canvas.setTextColor(COLOR_STAR);
            canvas.setTextSize(2);
            canvas.setCursor(85, 20);
            canvas.print("YAY!");
            for (int s = 0; s < 10; s++) {
                drawStar(random(240), random(60), 5, COLOR_STAR);
            }
            canvas.pushSprite(0, 0);
            playNote(NOTE_E5, 100);
            playNote(NOTE_G5, 150);
        } else {
            canvas.setTextColor(COLOR_HIGHLIGHT);
            canvas.setTextSize(2);
            canvas.setCursor(50, 20);
            canvas.print("MISSED!");
            canvas.pushSprite(0, 0);
            playNote(NOTE_C4, 200);
        }
        delay(600);
    }

    // Final score
    for (int i = 0; i < 15; i++) {
        if (smokeTimedOut(sceneStart)) return;
        canvas.fillSprite(COLOR_BG);
        drawGhost(gx, 50, i % 3 == 0, score >= 3, i);

        for (int s = 0; s < score; s++) {
            int bounce = abs((i + s) % 6 - 3) * 2;
            drawStar(25 + s * 42, 20 - bounce, 7, COLOR_STAR);
        }

        canvas.setTextColor(score >= 3 ? COLOR_STAR : COLOR_TEXT);
        canvas.setTextSize(2);
        canvas.setCursor(45, 105);
        canvas.printf("%d STARS!", score);

        canvas.pushSprite(0, 0);

        if (score >= 3) {
            playNote(NOTE_C5 + (i % 5) * 50, 60);
        } else {
            delay(100);
        }
    }
    delay(500);

    if (smokeMode) {
        smokeHold(sceneStart);
    }
}

void runSmokeSequence() {
    feedScene();
    danceScene();
    marchScene();
    gameScene();
}

// ============== Main ==============

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Speaker.setVolume(255);  // Max volume

#if !ESP32
    const char* smokeEnv = std::getenv("BOO_SMOKE");
    if (smokeEnv && smokeEnv[0] != '\0') {
        smokeMode = true;
        muted = true;
        volume = 0;
        M5Cardputer.Speaker.setVolume(0);
    }
#endif

    // Create sprite buffer
    canvas.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);

    // Initialize sparkles
    for (int i = 0; i < 8; i++) {
        sparkles[i] = {static_cast<int>(random(240)),
                       static_cast<int>(random(100)),
                       COLOR_SPARKLE,
                       0};
    }

    // Intro animation
    const int introMelody[] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5, NOTE_E5, NOTE_G5};
    for (int i = 0; i < 15; i++) {
        canvas.fillSprite(COLOR_BG);

        int bounceY = 60 - abs(7 - i) * 5;
        drawGhost(104, bounceY, i % 4 == 0);

        if (i > 4) {
            for (int s = 0; s < min(i - 4, 6); s++) {
                drawStar(25 + s * 38, 15, 5 + s % 2, COLOR_STAR);
            }
        }

        if (i > 3) {
            canvas.setTextColor(COLOR_HIGHLIGHT);
            canvas.setTextSize(3);
            canvas.setCursor(85, 8);
            canvas.print("BOO!");
        }

        if (i > 7) {
            drawHeart(45, 25, COLOR_HEART);
            drawHeart(195, 25, COLOR_HEART);
        }

        canvas.pushSprite(0, 0);

        if (i < 6) playNote(introMelody[i], 100);
        else delay(100);
    }

    canvas.setTextColor(COLOR_TEXT);
    canvas.setTextSize(1);
    canvas.setCursor(50, 118);
    canvas.print("Your ghostly friend!");
    canvas.pushSprite(0, 0);
    delay(800);

    // Init
    prefs.begin("boo", false);
    randomSeed(analogRead(0) + millis());

    game.init(); // Initialize using library

    Serial.begin(115200);
}

void loop() {
    if (smokeMode) {
        if (!smokeDone) {
            smokeDone = true;
            runSmokeSequence();
        }
#if !ESP32
        std::exit(0);
#else
        delay(1000);
#endif
        return;
    }

    unsigned long now = millis();

    // Update ghost physics
    game.update(); 

    // Update sparkles
    for (int i = 0; i < 8; i++) {
        if (sparkles[i].life > 0) {
            sparkles[i].life--;
        } else if (random(100) < 3) {
            sparkles[i] = {static_cast<int>(random(240)),
                           static_cast<int>(random(100)),
                           static_cast<uint16_t>(random(2) ? COLOR_SPARKLE : COLOR_STAR),
                           static_cast<int>(random(10, 30))};
        }
    }

    // Draw frame
    canvas.fillSprite(COLOR_BG);

    // Draw sparkles
    for (int i = 0; i < 8; i++) {
        if (sparkles[i].life > 0) {
            drawSparkle(sparkles[i].x, sparkles[i].y, sparkles[i].color);
        }
    }

    // Draw ghost using state from library
    drawGhost((int)game.getGhostX(), (int)game.getGhostY(), game.isBlinking());

    // Draw UI hints
    canvas.setTextColor(COLOR_TEXT);
    canvas.setTextSize(1);
    canvas.setCursor(5, SCREEN_HEIGHT - 12);
    canvas.printf("F:Feed D:Dance G:Game A:March M:%s", muted ? "OFF" : "ON");

    // Push to display
    canvas.pushSprite(0, 0);

    // Play Happy Birthday (non-blocking)
    if (!muted && now - lastNoteTime > (unsigned long)happyBirthdayDurations[musicIndex]) {
        musicIndex = (musicIndex + 1) % happyBirthdayLen;
        if (happyBirthday[musicIndex] > 0) {
            M5Cardputer.Speaker.tone(happyBirthday[musicIndex], happyBirthdayDurations[musicIndex] - 30);
        }
        lastNoteTime = now;
    }

    // Handle input
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState keys = M5Cardputer.Keyboard.keysState();
        for (auto key : keys.word) {
            if (key == 'f' || key == 'F') feedScene();
            else if (key == 'd' || key == 'D') danceScene();
            else if (key == 'g' || key == 'G') gameScene();
            else if (key == 'a' || key == 'A') marchScene();
            else if (key == 'm' || key == 'M') {
                muted = !muted;
                if (muted) M5Cardputer.Speaker.stop();
            }
            else if (key == '=' || key == '+') {
                volume = min(255, volume + 32);
                M5Cardputer.Speaker.setVolume(volume);
            }
            else if (key == '-' || key == '_') {
                volume = max(0, volume - 32);
                M5Cardputer.Speaker.setVolume(volume);
            }
            else if (key == '!') enterDownloadMode();
        }
    }

    delay(33);  // ~30 FPS
}
