/**
 * Boo - A ghostly Tamagotchi for M5Stack Cardputer
 * Ported from MicroHydra/MicroPython to Arduino/C++
 */

#include <M5Cardputer.h>
#include <Preferences.h>
#include <algorithm>
#include "soc/rtc_cntl_reg.h"
#include "esp_system.h"

// Reboot into download/flash mode (press 'F' to activate)
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
    REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
    esp_restart();
}

// Forward declarations
void saveStats();

// ============== Constants ==============
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 135
#define GHOST_SIZE 32

#define DEFAULT_HAPPINESS 100
#define DEFAULT_HUNGER 100
#define HAPPINESS_DECREASE_PER_DAY 50
#define HUNGER_DECREASE_PER_DAY 50
#define SECONDS_PER_DAY 86400

// Colors - cute pink/purple theme!
#define COLOR_BG 0x180818           // Dark purple
#define COLOR_GHOST 0xFCFC          // Light pink
#define COLOR_GHOST_CHEEKS 0xFACA   // Rosy pink
#define COLOR_TEXT 0xFFFF           // White
#define COLOR_HIGHLIGHT 0xF81F      // Magenta
#define COLOR_HEART 0xF810          // Red/pink
#define COLOR_STAR 0xFFE0           // Yellow
#define COLOR_SPARKLE 0xCFFF        // Light cyan

// ============== Ghost Sprites (32x32 1-bit) ==============
// Simple ghost shape - will be drawn procedurally
// In a full port, convert the .raw files to C arrays

// ============== Game State ==============
struct Stats {
    uint32_t ageStartTimestamp;
    uint32_t lastGameTimestamp;
    uint32_t lastFeedTimestamp;
    int happiness;
    int hunger;
};

Stats stats;
Preferences prefs;

// Ghost animation state
float ghostX, ghostY;
float velX, velY;
bool isBlinking = false;
unsigned long lastBlinkTime = 0;
unsigned long blinkDuration = 200;
unsigned long blinkInterval = 3000;

// ============== Stats Functions ==============

void loadStats() {
    prefs.begin("boo", true);  // read-only
    stats.ageStartTimestamp = prefs.getUInt("age_start", 0);
    stats.lastGameTimestamp = prefs.getUInt("last_game", 0);
    stats.lastFeedTimestamp = prefs.getUInt("last_feed", 0);
    stats.happiness = prefs.getInt("happiness", DEFAULT_HAPPINESS);
    stats.hunger = prefs.getInt("hunger", DEFAULT_HUNGER);
    prefs.end();

    // Initialize if first run
    if (stats.ageStartTimestamp == 0) {
        uint32_t now = millis() / 1000;  // Simple timestamp
        stats.ageStartTimestamp = now;
        stats.lastGameTimestamp = now;
        stats.lastFeedTimestamp = now;
        stats.happiness = DEFAULT_HAPPINESS;
        stats.hunger = DEFAULT_HUNGER;
        saveStats();
    }
}

void saveStats() {
    prefs.begin("boo", false);  // read-write
    prefs.putUInt("age_start", stats.ageStartTimestamp);
    prefs.putUInt("last_game", stats.lastGameTimestamp);
    prefs.putUInt("last_feed", stats.lastFeedTimestamp);
    prefs.putInt("happiness", stats.happiness);
    prefs.putInt("hunger", stats.hunger);
    prefs.end();
}

int calculateAge() {
    uint32_t now = millis() / 1000;
    return (now - stats.ageStartTimestamp) / SECONDS_PER_DAY;
}

void updateStats() {
    uint32_t now = millis() / 1000;
    int daysSinceGame = (now - stats.lastGameTimestamp) / SECONDS_PER_DAY;
    int daysSinceFeed = (now - stats.lastFeedTimestamp) / SECONDS_PER_DAY;

    stats.happiness = max(0, stats.happiness - daysSinceGame * HAPPINESS_DECREASE_PER_DAY);
    stats.hunger = max(0, stats.hunger - daysSinceFeed * HUNGER_DECREASE_PER_DAY);
}

// ============== Drawing Functions ==============

// Draw a cute sparkle
void drawSparkle(int x, int y, uint16_t color) {
    M5Cardputer.Display.drawPixel(x, y, color);
    M5Cardputer.Display.drawPixel(x-1, y, color);
    M5Cardputer.Display.drawPixel(x+1, y, color);
    M5Cardputer.Display.drawPixel(x, y-1, color);
    M5Cardputer.Display.drawPixel(x, y+1, color);
}

// Draw a heart
void drawHeart(int x, int y, uint16_t color) {
    M5Cardputer.Display.fillCircle(x - 3, y, 4, color);
    M5Cardputer.Display.fillCircle(x + 3, y, 4, color);
    M5Cardputer.Display.fillTriangle(x - 7, y + 2, x + 7, y + 2, x, y + 10, color);
}

// Draw a star
void drawStar(int x, int y, int size, uint16_t color) {
    M5Cardputer.Display.fillTriangle(x, y - size, x - size/2, y + size/2, x + size/2, y + size/2, color);
    M5Cardputer.Display.fillTriangle(x, y + size, x - size/2, y - size/2, x + size/2, y - size/2, color);
}

void drawGhost(int x, int y, bool blinking) {
    // Cute ghost body - bigger and rounder!
    M5Cardputer.Display.fillCircle(x + 16, y + 14, 16, COLOR_GHOST);
    M5Cardputer.Display.fillRect(x, y + 14, 32, 18, COLOR_GHOST);

    // Wavy bottom - cuter waves
    for (int i = 0; i < 4; i++) {
        int bx = x + i * 8;
        M5Cardputer.Display.fillCircle(bx + 4, y + 30, 5, COLOR_GHOST);
    }

    // Rosy cheeks!
    M5Cardputer.Display.fillCircle(x + 6, y + 18, 4, COLOR_GHOST_CHEEKS);
    M5Cardputer.Display.fillCircle(x + 26, y + 18, 4, COLOR_GHOST_CHEEKS);

    // Eyes - big sparkly eyes!
    if (!blinking) {
        // Big cute eyes
        M5Cardputer.Display.fillCircle(x + 10, y + 12, 5, COLOR_BG);
        M5Cardputer.Display.fillCircle(x + 22, y + 12, 5, COLOR_BG);
        // Pupils with sparkle
        M5Cardputer.Display.fillCircle(x + 11, y + 13, 2, COLOR_GHOST);
        M5Cardputer.Display.fillCircle(x + 23, y + 13, 2, COLOR_GHOST);
        // Eye sparkles (white dots)
        M5Cardputer.Display.fillCircle(x + 8, y + 10, 1, COLOR_TEXT);
        M5Cardputer.Display.fillCircle(x + 20, y + 10, 1, COLOR_TEXT);
    } else {
        // Happy closed eyes (curved lines) ^_^
        M5Cardputer.Display.drawLine(x + 6, y + 14, x + 10, y + 10, COLOR_BG);
        M5Cardputer.Display.drawLine(x + 10, y + 10, x + 14, y + 14, COLOR_BG);
        M5Cardputer.Display.drawLine(x + 18, y + 14, x + 22, y + 10, COLOR_BG);
        M5Cardputer.Display.drawLine(x + 22, y + 10, x + 26, y + 14, COLOR_BG);
    }

    // Cute little smile
    M5Cardputer.Display.drawLine(x + 12, y + 22, x + 16, y + 24, COLOR_BG);
    M5Cardputer.Display.drawLine(x + 16, y + 24, x + 20, y + 22, COLOR_BG);
}

void drawGhostEating(int x, int y) {
    drawGhost(x, y, false);
    // Open mouth
    M5Cardputer.Display.fillCircle(x + 16, y + 20, 4, COLOR_BG);
}

// ============== Screens ==============

void showStatsScreen() {
    M5Cardputer.Display.fillScreen(COLOR_BG);
    updateStats();

    M5Cardputer.Display.setTextColor(COLOR_TEXT);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(10, 10);
    M5Cardputer.Display.println("Boo Stats:");

    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(10, 40);
    M5Cardputer.Display.printf("Age: %d days\n", calculateAge());
    M5Cardputer.Display.setCursor(10, 55);
    M5Cardputer.Display.printf("Happiness: %d\n", stats.happiness);
    M5Cardputer.Display.setCursor(10, 70);
    M5Cardputer.Display.printf("Hunger: %d\n", stats.hunger);

    M5Cardputer.Display.setCursor(10, 110);
    M5Cardputer.Display.println("Press any key to return");

    // Wait for key
    while (true) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            break;
        }
        delay(50);
    }
}

void feedAnimation() {
    // Yummy eating animation with floating hearts!
    const char* texts[] = {"YUM!", "YUM!", "YUM!"};

    // Eating phase with hearts floating up
    for (int frame = 0; frame < 12; frame++) {
        M5Cardputer.Display.fillScreen(COLOR_BG);

        // Draw ghost eating
        if (frame % 2 == 0) {
            drawGhostEating(100, 50);
        } else {
            drawGhost(100, 50, true);  // Happy blink while eating
        }

        // Floating hearts!
        for (int h = 0; h < 3; h++) {
            int heartY = 80 - (frame * 5) - (h * 20);
            int heartX = 150 + (h * 15) + (frame % 2) * 3;
            if (heartY > 5 && heartY < 130) {
                drawHeart(heartX, heartY, COLOR_HEART);
            }
        }

        // Sparkles around ghost
        if (frame % 3 == 0) {
            drawSparkle(80 + random(60), 40 + random(50), COLOR_SPARKLE);
            drawSparkle(90 + random(50), 30 + random(60), COLOR_STAR);
        }

        // Show "YUM!" text
        M5Cardputer.Display.setTextColor(COLOR_HIGHLIGHT);
        M5Cardputer.Display.setTextSize(2);
        M5Cardputer.Display.setCursor(20, 60);
        if (frame > 2) M5Cardputer.Display.println(texts[0]);
        if (frame > 5) {
            M5Cardputer.Display.setCursor(20, 85);
            M5Cardputer.Display.println(texts[1]);
        }
        if (frame > 8) {
            M5Cardputer.Display.setCursor(20, 110);
            M5Cardputer.Display.println(texts[2]);
        }

        // Beep sound (cute!)
        if (frame % 4 == 0) {
            M5Cardputer.Speaker.tone(800 + frame * 50, 50);
        }

        delay(200);
    }

    // Happy celebration!
    for (int i = 0; i < 5; i++) {
        M5Cardputer.Display.fillScreen(COLOR_BG);
        drawGhost(100, 50, i % 2 == 0);

        // Stars everywhere!
        for (int s = 0; s < 5; s++) {
            drawStar(random(240), random(100), 4, COLOR_STAR);
        }

        M5Cardputer.Display.setTextColor(COLOR_STAR);
        M5Cardputer.Display.setTextSize(2);
        M5Cardputer.Display.setCursor(60, 10);
        M5Cardputer.Display.println("SO YUMMY!");

        M5Cardputer.Speaker.tone(1000 + i * 100, 100);
        delay(200);
    }

    // Update stats
    stats.hunger = DEFAULT_HUNGER;
    stats.lastFeedTimestamp = millis() / 1000;
    saveStats();
}

// Simple star-catching game for little kids!
void playCatchStarsGame() {
    int score = 0;
    int rounds = 5;
    int ghostX = 104;  // Center ghost

    // Instructions
    M5Cardputer.Display.fillScreen(COLOR_BG);
    M5Cardputer.Display.setTextColor(COLOR_STAR);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(30, 20);
    M5Cardputer.Display.println("CATCH STARS!");
    M5Cardputer.Display.setTextColor(COLOR_TEXT);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(20, 60);
    M5Cardputer.Display.println("Press ANY KEY when the");
    M5Cardputer.Display.setCursor(20, 75);
    M5Cardputer.Display.println("star is over the ghost!");
    M5Cardputer.Display.setCursor(50, 110);
    M5Cardputer.Display.println("Press key to start...");

    // Wait for start
    while (true) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) break;
        delay(50);
    }
    delay(300);

    // Play rounds
    for (int round = 0; round < rounds; round++) {
        int starX = 0;
        int starDir = 3 + random(3);  // Speed varies
        bool caught = false;
        bool missed = false;

        // Star moves across screen
        while (!caught && !missed && starX < 240) {
            M5Cardputer.Display.fillScreen(COLOR_BG);

            // Draw ghost at center bottom
            drawGhost(ghostX, 75, false);

            // Draw moving star at top
            drawStar(starX, 25, 8, COLOR_STAR);

            // Draw score
            M5Cardputer.Display.setTextColor(COLOR_HIGHLIGHT);
            M5Cardputer.Display.setTextSize(1);
            M5Cardputer.Display.setCursor(5, 5);
            M5Cardputer.Display.printf("Stars: %d", score);

            // Check for keypress
            M5Cardputer.update();
            if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
                // Check if star is over ghost (within 30 pixels)
                if (abs(starX - (ghostX + 16)) < 30) {
                    caught = true;
                    score++;
                } else {
                    missed = true;
                }
            }

            starX += starDir;
            delay(30);
        }

        // Show result for this round
        M5Cardputer.Display.fillScreen(COLOR_BG);
        drawGhost(ghostX, 75, true);

        if (caught) {
            // Celebration!
            M5Cardputer.Display.setTextColor(COLOR_STAR);
            M5Cardputer.Display.setTextSize(2);
            M5Cardputer.Display.setCursor(70, 20);
            M5Cardputer.Display.println("YAY!");

            // Stars and sparkles!
            for (int i = 0; i < 8; i++) {
                drawStar(random(240), random(60), 5, COLOR_STAR);
            }
            M5Cardputer.Speaker.tone(1200, 100);
            delay(100);
            M5Cardputer.Speaker.tone(1500, 100);
        } else {
            M5Cardputer.Display.setTextColor(COLOR_HIGHLIGHT);
            M5Cardputer.Display.setTextSize(2);
            M5Cardputer.Display.setCursor(50, 20);
            M5Cardputer.Display.println("TRY AGAIN!");
            M5Cardputer.Speaker.tone(400, 200);
        }
        delay(800);
    }

    // Final score celebration!
    for (int i = 0; i < 8; i++) {
        M5Cardputer.Display.fillScreen(COLOR_BG);
        drawGhost(ghostX, 50, i % 2 == 0);

        // Draw collected stars
        for (int s = 0; s < score; s++) {
            drawStar(30 + s * 40, 15, 6, COLOR_STAR);
        }

        M5Cardputer.Display.setTextColor(COLOR_STAR);
        M5Cardputer.Display.setTextSize(2);
        M5Cardputer.Display.setCursor(50, 100);
        M5Cardputer.Display.printf("%d STARS!", score);

        if (score >= 3) {
            M5Cardputer.Speaker.tone(800 + i * 100, 100);
        }
        delay(200);
    }

    M5Cardputer.Display.setTextColor(COLOR_TEXT);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(40, 120);
    M5Cardputer.Display.println("Press any key...");

    while (true) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) break;
        delay(50);
    }

    // Update stats - playing increases happiness
    stats.happiness = min(DEFAULT_HAPPINESS, stats.happiness + score * 5);
    stats.lastGameTimestamp = millis() / 1000;
    saveStats();
}

void showMenu() {
    int selection = 0;
    const char* options[] = {"Feed Boo", "Play Game", "Return"};
    int numOptions = 3;

    while (true) {
        M5Cardputer.Display.fillScreen(COLOR_BG);
        M5Cardputer.Display.setTextColor(COLOR_TEXT);
        M5Cardputer.Display.setTextSize(2);
        M5Cardputer.Display.setCursor(10, 10);
        M5Cardputer.Display.println("Menu");

        M5Cardputer.Display.setTextSize(1);
        for (int i = 0; i < numOptions; i++) {
            M5Cardputer.Display.setCursor(20, 40 + i * 20);
            if (i == selection) {
                M5Cardputer.Display.setTextColor(COLOR_HIGHLIGHT);
                M5Cardputer.Display.print("> ");
            } else {
                M5Cardputer.Display.setTextColor(COLOR_TEXT);
                M5Cardputer.Display.print("  ");
            }
            M5Cardputer.Display.println(options[i]);
        }

        M5Cardputer.Display.setTextColor(COLOR_TEXT);
        M5Cardputer.Display.setCursor(10, 110);
        M5Cardputer.Display.println("UP/DOWN: Select  ENTER: Confirm");

        // Wait for input
        while (true) {
            M5Cardputer.update();
            if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
                Keyboard_Class::KeysState keys = M5Cardputer.Keyboard.keysState();

                // Check for arrow keys or number keys
                bool upPressed = keys.del || std::find(keys.word.begin(), keys.word.end(), ';') != keys.word.end();
                bool downPressed = std::find(keys.word.begin(), keys.word.end(), '.') != keys.word.end();

                if (upPressed) {
                    selection = (selection - 1 + numOptions) % numOptions;
                    break;
                }
                if (downPressed) {
                    selection = (selection + 1) % numOptions;
                    break;
                }
                if (keys.enter) {
                    if (selection == 0) {
                        feedAnimation();
                    } else if (selection == 1) {
                        playCatchStarsGame();
                    } else {
                        return;  // Exit menu
                    }
                    break;
                }
                // Also support number keys
                for (auto key : keys.word) {
                    if (key == '1') { feedAnimation(); break; }
                    if (key == '2') { playCatchStarsGame(); break; }
                    if (key == '3') { return; }
                }
            }
            delay(50);
        }
    }
}

// ============== Main ==============

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);

    M5Cardputer.Display.setRotation(1);

    // Cute intro animation!
    for (int i = 0; i < 10; i++) {
        M5Cardputer.Display.fillScreen(COLOR_BG);

        // Draw cute ghost bouncing in
        int bounceY = 70 - abs(5 - i) * 6;
        drawGhost(104, bounceY, i % 3 == 0);

        // Sparkles!
        if (i > 3) {
            for (int s = 0; s < i - 3; s++) {
                drawStar(30 + s * 45, 20, 5, COLOR_STAR);
            }
        }

        // Title with hearts
        if (i > 2) {
            M5Cardputer.Display.setTextColor(COLOR_HIGHLIGHT);
            M5Cardputer.Display.setTextSize(3);
            M5Cardputer.Display.setCursor(85, 10);
            M5Cardputer.Display.println("Boo!");
        }

        if (i > 5) {
            drawHeart(50, 25, COLOR_HEART);
            drawHeart(190, 25, COLOR_HEART);
        }

        M5Cardputer.Speaker.tone(400 + i * 80, 50);
        delay(150);
    }

    M5Cardputer.Display.setTextColor(COLOR_TEXT);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(55, 115);
    M5Cardputer.Display.println("Your ghostly friend!");
    delay(1000);

    // Initialize
    loadStats();
    randomSeed(analogRead(0));

    // Starting position
    ghostX = SCREEN_WIDTH / 2 - GHOST_SIZE / 2;
    ghostY = SCREEN_HEIGHT / 2 - GHOST_SIZE / 2;
    velX = 1.5;
    velY = 1.0;

    Serial.begin(115200);
    Serial.println("Boo started!");
}

void loop() {
    unsigned long now = millis();

    // Update ghost position (bouncing)
    ghostX += velX;
    ghostY += velY;

    // Bounce off walls
    if (ghostX <= 0 || ghostX >= SCREEN_WIDTH - GHOST_SIZE) {
        velX = -velX;
        ghostX = constrain(ghostX, 0, SCREEN_WIDTH - GHOST_SIZE);
    }
    if (ghostY <= 0 || ghostY >= SCREEN_HEIGHT - GHOST_SIZE - 15) {  // Leave room for text
        velY = -velY;
        ghostY = constrain(ghostY, 0, SCREEN_HEIGHT - GHOST_SIZE - 15);
    }

    // Blinking logic
    if (now - lastBlinkTime > blinkInterval) {
        isBlinking = true;
        if (now - lastBlinkTime > blinkInterval + blinkDuration) {
            isBlinking = false;
            lastBlinkTime = now;
        }
    }

    // Draw
    M5Cardputer.Display.fillScreen(COLOR_BG);

    // Random sparkles in background (magical!)
    if (random(10) > 7) {
        drawSparkle(random(240), random(100), COLOR_SPARKLE);
    }
    if (random(15) > 12) {
        drawStar(random(240), random(80), 3, COLOR_STAR);
    }

    drawGhost((int)ghostX, (int)ghostY, isBlinking);

    // Draw UI - cute icons instead of just text
    drawHeart(15, SCREEN_HEIGHT - 8, COLOR_HEART);
    M5Cardputer.Display.setTextColor(COLOR_TEXT);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(30, SCREEN_HEIGHT - 12);
    M5Cardputer.Display.print("1:Feed");

    drawStar(100, SCREEN_HEIGHT - 8, 4, COLOR_STAR);
    M5Cardputer.Display.setCursor(115, SCREEN_HEIGHT - 12);
    M5Cardputer.Display.print("2:Play");

    // Check input
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState keys = M5Cardputer.Keyboard.keysState();
        for (auto key : keys.word) {
            if (key == '1') {
                showMenu();
            } else if (key == '2') {
                showStatsScreen();
            } else if (key == '!') {
                enterDownloadMode();  // Reboot to flash mode (Shift+1)
            }
        }
    }

    delay(50);  // ~20 FPS
}
